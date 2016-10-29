#include "collect.h"
#include <string>
#include <fstream>
#include "exceptions.h"
#define NICKNAME "DENIS"
#include "serverlib/slogger.h"
#include "xml_unit.h"
#include "jxtlib/xml_stuff.h"
#include "collect_data.h"
#include "astra_utils.h"
#include "serverlib/query_runner.h"
#include "jxtlib/jxt_cont.h"
#include "passenger.h"

using namespace std;
using namespace EXCEPTIONS;
using namespace BASIC;
using namespace BASIC::date_time;
using namespace ASTRA;

struct TRand{
    TRand() { srand(time(NULL)); };
    size_t get(size_t range) { return rand() % range; }
    static TRand *Instance()
    {
        static TRand *instance_ = 0;
        if(!instance_) instance_ = new TRand();
        return instance_;
    }
};

struct TPaxName {
    struct TItem {
        string name, surname;
        string str() const { return surname + "/" + name; };
        TItem(const string &aname, const string &asurname):
            name(aname), surname(asurname)
        {}
    };

    vector<TItem> items;

    const TItem &get();

    TPaxName();
    static TPaxName *Instance()
    {
        static TPaxName *instance_ = 0;
        if(!instance_) instance_ = new TPaxName();
        return instance_;
    }
};

const TPaxName::TItem &TPaxName::get()
{
    return items[TRand::Instance()->get(items.size())];
}

TPaxName::TPaxName()
{
    TQuery Qry(&OraSession);
    Qry.SQLText = "select name, surname from collect_pax";
    Qry.Execute();
    for(; not Qry.Eof; Qry.Next())
        items.push_back(TItem(
                    Qry.FieldAsString(0),
                    Qry.FieldAsString(1)
                    )
                );
}

struct TUsers {
    struct TItem {
        string desk;
        string login;
        string passwd;
        TItem() {}
        TItem(
                const string &adesk,
                const string &alogin,
                const string &apasswd
             ):
            desk(adesk),
            login(alogin),
            passwd(apasswd)
        {
        }
    };

    typedef vector<TItem> TItems;
    TItems items;
    TItems::iterator idx;

    const TItem &get() { return items[TRand::Instance()->get(items.size())]; }
    TUsers();
    static TUsers *Instance()
    {
        static TUsers *instance_ = 0;
        if(!instance_) instance_ = new TUsers();
        return instance_;
    }
};

struct TAfter {
    virtual void  Event(const XMLRequestCtxt *ctxt) = 0;
    virtual ~TAfter() { };
};

struct TAfterTagPacks:public TAfter {
    struct TTagPack {
        string tag_type;
        float no;
    };

    typedef map<string, map<string, TTagPack> > TTagPacks;

    TTagPacks items;

    virtual void  Event(const XMLRequestCtxt *ctxt)
    {
        xmlNodePtr tagPackNode =
            ctxt->resDoc->children // term
            ->children // answer
            ->children // tag_packs
            ->children; // tag_pack
        for(xmlNodePtr currNode = tagPackNode; currNode; currNode = currNode->next)
        {
            string airline = NodeAsString(currNode->children);
            string target = NodeAsString(currNode->next);
            TTagPack item;
            item.tag_type = NodeAsString(currNode->next);
            item.no = NodeAsFloat(currNode->next);
            items[airline][target] = item;
        }
    }
};

struct TAfterSavePax:public TAfter {
    int tid;
    int grp_id;
    virtual void  Event(const XMLRequestCtxt *ctxt)
    {
        tid = NodeAsInteger(
                ctxt->resDoc->children // term
                ->children // answer
                ->children // segments
                ->children // segment
                ->children // tripheader
                ->next // tripdata
                ->next // grp_id
                ->next // point_dep
                ->next // airp_dep
                ->next // point_arv
                ->next // airp_arv
                ->next // class
                ->next // status
                ->next // bag_refuse
                ->next // bag_types_id
                ->next // piece_concept
                ->next // tid
                );
        grp_id = NodeAsInteger(
                ctxt->resDoc->children // term
                ->children // answer
                ->children // segments
                ->children // segment
                ->children // tripheader
                ->next // tripdata
                ->next // grp_id
                );
    }
};

struct TAfterCreateFlt:public TAfter {
    int point_id;
    virtual void  Event(const XMLRequestCtxt *ctxt)
    {
        point_id = NodeAsInteger(
                ctxt->resDoc->children // term
                ->children // answer
                ->children // command
                ->next // data
                ->children // move_id
                ->next // dests
                ->children // dest
                ->children // point_id
                );
    }
};

struct TExec {
    string pult, opr;
    TExec(const string &apult, const string &aopr): pult(apult), opr(aopr) {}
    void exec(const string &req, bool check_user_logon = true, TAfter *after = NULL);
};

void TExec::exec(const string &req, bool check_user_logon, TAfter *after)
{
    // Здесь должен быть код, похожий на содержимое  JXTLibCallbacks::Main (файл jxtlib/src/jxtlib.cc)
    // Напр. ф-ии UserBefore и UserAfter переопределены в astra_callbacks.cc
    // Возможно, нужно вставить что-то оттуда? Напр. base_tables.Invalidate()

    TReqInfoInitData reqInfoData;
    reqInfoData.pult = pult;
    reqInfoData.opr = opr;
    reqInfoData.checkUserLogon = check_user_logon;
    reqInfoData.checkCrypt = true;
    reqInfoData.pr_web = false;
    reqInfoData.duplicate = false;

    jxtlib::JXTLib::Instance()->GetCallbacks()->initJxtContext(reqInfoData.pult); // init context

    XMLRequestCtxt *ctxt = getXmlCtxt();
    ctxt->Init(req, reqInfoData.pult, reqInfoData.opr);

    ProgTrace(TRACE5, "%s", GetXMLDocText(ctxt->reqDoc).c_str());

    reqInfoData.screen = NodeAsString("@screen", ctxt->reqDoc->children->children);
    reqInfoData.mode = NodeAsString("@mode", ctxt->reqDoc->children->children);
    reqInfoData.lang = NodeAsString("@lang", ctxt->reqDoc->children->children);
    reqInfoData.term_id = NodeAsFloat("@term_id", ctxt->reqDoc->children->children);

    string iface = NodeAsString("@id", ctxt->reqDoc->children->children);
    string evt = (char *)ctxt->reqDoc->children->children->children->name;


    TReqInfo *reqInfo = TReqInfo::Instance();
    reqInfo->Initialize( reqInfoData );
    JxtInterfaceMng::Instance()->
        GetInterface(iface)->
        OnEvent(evt,  ctxt,
                ctxt->reqDoc->children->children->children,
                ctxt->resDoc->children->children);
    ProgTrace(TRACE5, "%s", GetXMLDocText(ctxt->resDoc).c_str());
    if(after) after->Event(ctxt);
    jxtlib::JXTLib::Instance()->GetCallbacks()->GetXmlRequestCtxt(true); // reset context
    OraSession.Commit();
}

void replace_holder(string &buf, const string &tag, const string &val, bool pr_all = false)
{
    while(true) {
        size_t idx = buf.find(tag);
        if(idx == string::npos) break;
        buf.replace(idx, tag.size(), val);
        if(not pr_all) break;
    }
}

void replace_holder(string &buf, const string &tag, int val, bool pr_all = false)
{
    replace_holder(buf, tag, IntToString(val), pr_all);
}

void replace(string &buf, const string &tag, const string &val, size_t count = 1)
{
    string begin = "<" + tag + ">";
    string end = "</" + tag + ">";
    size_t start_idx = 0;
    for(; count > 0; count--) {
        size_t idx_begin = buf.find(begin, start_idx);
        size_t idx_end = buf.find(end, start_idx + end.size());

        if(idx_begin == string::npos) break;

        buf.replace(idx_begin + begin.size(), idx_end - (idx_begin + begin.size()), val);

        start_idx = idx_end;
    }
}

void replace(string &buf, const string &tag, int val, size_t count = 1)
{
    return replace(buf, tag, IntToString(val), count);
}

int collect(int point_id, int move_id, int flt_no, TDateTime scd_out)
{


    string buf_req_save_pax = req_save_pax;
    replace(buf_req_save_pax, "point_dep", point_id);

    TTripRoute route;
    route.GetRouteAfter(NoExists, point_id, trtWithCurrent, trtNotCancelled);
    replace(buf_req_save_pax, "point_arv", route[1].point_id);
    replace(buf_req_save_pax, "airp_dep", route[0].airp, 2);
    replace(buf_req_save_pax, "airp_arv", route[1].airp);
    replace(buf_req_save_pax, "flt_no", flt_no);
    replace(buf_req_save_pax, "scd", DateTimeToStr(scd_out, "dd.mm.yyyy 00:00:00"));

    string buf_req_bag = req_save_bag;
    replace(buf_req_bag, "point_arv", route[1].point_id);
    replace(buf_req_bag, "airp_dep", route[0].airp, 2);
    replace(buf_req_bag, "airp_arv", route[1].airp);
    replace(buf_req_bag, "flt_no", flt_no);
    replace(buf_req_bag, "scd", DateTimeToStr(scd_out, "dd.mm.yyyy 00:00:00"));

    TQuery Qry(&OraSession);
    Qry.SQLText =
        "select "
        " crs_pax.pax_id, "
        " crs_pax.surname, "
        " crs_pax.name "
        "from "
        "  crs_pnr, "
        "  tlg_binding, "
        "  crs_pax, "
        "  pax "
        "where "
        "  tlg_binding.point_id_spp = :point_id and "
        "  crs_pnr.point_id = tlg_binding.point_id_tlg and "
        "  crs_pnr.system = 'CRS' and "
        "  crs_pnr.pnr_id = crs_pax.pnr_id and "
        "  crs_pax.pax_id = pax.pax_id(+) and "
        "  pax.pax_id is null "
        "order by "
        "  crs_pax.surname ";
    Qry.CreateVariable("point_id", otInteger, point_id);
    Qry.Execute();
    int pax_count = 0;
    if(not Qry.Eof) {
        TUsers::TItem takeoff_user;
        for(; not Qry.Eof; Qry.Next(), pax_count++) {
            const TUsers::TItem &user = TUsers::Instance()->get();
            takeoff_user = user;
            TExec clnt(user.desk, user.login);

            string buf = req_logon;
            replace(buf, "userr", user.login);
            replace(buf, "passwd", user.passwd);
            clnt.exec(buf, false);

            int pax_id = Qry.FieldAsInteger("pax_id");
            string surname = Qry.FieldAsString("surname");
            string name = Qry.FieldAsString("name");

            CheckIn::TPaxTknItem tkn;
            LoadCrsPaxTkn(pax_id, tkn);

            replace(buf_req_save_pax, "pax_id", pax_id);
            replace(buf_req_save_pax, "surname", surname);
            replace(buf_req_save_pax, "name", name);
            replace(buf_req_save_pax, "ticket_no", tkn.no);
            replace(buf_req_save_pax, "coupon_no", tkn.coupon);
            TAfterSavePax after_save_pax;
            clnt.exec(buf_req_save_pax, true, &after_save_pax);

            replace(buf_req_bag, "pax_id", pax_id);
            replace(buf_req_bag, "surname", surname);
            replace(buf_req_bag, "name", name);
            replace(buf_req_bag, "ticket_no", tkn.no);
            replace(buf_req_bag, "coupon_no", tkn.coupon);
            replace(buf_req_bag, "grp_id", after_save_pax.grp_id);
            replace(buf_req_bag, "tid", after_save_pax.tid, 2);
            clnt.exec(buf_req_bag);
        }

        // Проставим вылет только если были зарегены пасы (not Qry.Eof)
        string buf_req_takeoff = req_takeoff;
        replace(buf_req_takeoff, "move_id", move_id);
        replace_holder(buf_req_takeoff, POINT_DEP, point_id, true);
        replace_holder(buf_req_takeoff, POINT_ARV, route[1].point_id, true);
        replace(buf_req_takeoff, "scd_out", DateTimeToStr(scd_out, ServerFormatDateTimeAsString));
        replace(buf_req_takeoff, "act_out", DateTimeToStr(scd_out, ServerFormatDateTimeAsString));
        replace(buf_req_takeoff, "scd_in", DateTimeToStr(scd_out + 1./24., ServerFormatDateTimeAsString));
        replace(buf_req_takeoff, "est_in", DateTimeToStr(scd_out + 1./24., ServerFormatDateTimeAsString));
        replace(buf_req_takeoff, "flt_no", flt_no);

        TExec clnt(takeoff_user.desk, takeoff_user.login);
        clnt.exec(buf_req_takeoff);
    }

    return pax_count;
}

TUsers::TUsers()
{
    TQuery desksQry(&OraSession);
    desksQry.SQLText = "select code from desks where grp_id in (select grp_id from desk_grp where city = 'МОВ')";
    desksQry.Execute();
    TQuery usersQry(&OraSession);
    usersQry.SQLText = 
        "select login, passwd from users2 where "
        "    ((type = 2 and user_id in(select aro_id from  aro_airlines where airline = 'ЮТ') and user_id not in(select aro_id from aro_airps where airp = 'ДМД')) "
        "    or (type = 1 and user_id in(select aro_id from  aro_airps where airp = 'ДМД') and user_id not in(select aro_id from aro_airlines where airline = 'ЮТ'))) and "
        "    login is not null and passwd is not null and pr_denial = 0 ";
    usersQry.Execute();
    for(; not desksQry.Eof; desksQry.Next()) {
        for(; not usersQry.Eof; usersQry.Next()) {
            string login = usersQry.FieldAsString(0);
            if(login == "DEN2") continue;
            items.push_back(
                    TItem(
                        desksQry.FieldAsString(0),
                        login,
                        usersQry.FieldAsString(1)
                        )
                    );
        }
    }

    /*
    // logon them all
    for(TItems::iterator i = items.begin(); i != items.end(); i++) {
        LogTrace(TRACE5) << "logon " << i->desk << " : " << i-> login << " started at " << DateTimeToStr(NowUTC(), ServerFormatDateTimeAsString);
        TExec clnt(i->desk, i->login);
        string buf = req_logon;
        replace(buf, "userr", i->login);
        replace(buf, "passwd", i->passwd);
        clnt.exec(buf, false);
        LogTrace(TRACE5) << "logon " << i->desk << " : " << i-> login << " finished at " << DateTimeToStr(NowUTC(), ServerFormatDateTimeAsString);
    }
    idx = items.begin();
    */
}


int nosir_logon_all(int argc,char **argv)
{
    TUsers::Instance();
    return 0;
}

int nosir_collect(int argc,char **argv)
{
    TQuery closeQry(&OraSession);
    closeQry.SQLText = "update created_flts set pr_collect = 1 where point_id = :point_id";
    closeQry.DeclareVariable("point_id", otInteger);

    TQuery Qry(&OraSession);
    if(argc > 1) {
        Qry.SQLText =
            "select point_id, flt_no, scd_out, move_id from points where point_id = :point_id";
        Qry.CreateVariable("point_id", otInteger, ToInt(argv[1]));
    } else
        Qry.SQLText =
            "select "
            "   cf.point_id, "
            "   cf.flt_no, "
            "   cf.scd_out, "
            "   points.move_id "
            "from "
            "   created_flts cf, "
            "   points "
            "where "
            "   pr_collect = 0 and "
            "   cf.point_id = points.point_id ";
    Qry.Execute();
    for(; not Qry.Eof; Qry.Next()) {
        int point_id = Qry.FieldAsInteger("point_id");
        int flt_no = Qry.FieldAsInteger("flt_no");
        TDateTime scd_out = Qry.FieldAsDateTime("scd_out");
        int move_id = Qry.FieldAsInteger("move_id");

        LogTrace(TRACE5) << "collect flight started at " << DateTimeToStr(NowUTC(), ServerFormatDateTimeAsString);
        int pax_count = collect(point_id, move_id, flt_no, scd_out);
        LogTrace(TRACE5) << "collect flight stopped at " << DateTimeToStr(NowUTC(), ServerFormatDateTimeAsString);

        if(pax_count) {
            closeQry.SetVariable("point_id", point_id);
            closeQry.Execute();
            OraSession.Commit();
        }
    }


    return 0;
}

void fill_pax(string &req)
{
    static const string name_prefix = "\n1";
    size_t prev_idx = 0;
    while(true) {
        size_t idx = req.find(name_prefix, prev_idx);
        if(idx == string::npos) break;
        size_t end_idx = req.find("\n", idx + name_prefix.size());
        string pax_name = TPaxName::Instance()->get().str();
        string name = req.substr(idx + name_prefix.size(), end_idx - idx - name_prefix.size());
        if(
                name.find("ВОЛОТКОВИЧ") == string::npos and
                name.find("ФОМИНЫХ") == string::npos
                )
            req.replace(idx + name_prefix.size(), end_idx - idx - name_prefix.size(), pax_name);
        prev_idx = idx + 1;
    }
    LogTrace(TRACE5) << req;
}

int nosir_pax(int argc,char **argv)
{
    string buf = req_load_pnl;
    fill_pax(buf);
    return 1;
}


void etkt_gen(string &req)
{
    size_t curr_no = 0;

    {
        ifstream curr_no_file("curr_no");
        if(curr_no_file.good())
            curr_no_file >> curr_no;
    }

    size_t prev_idx = 0;
    while(true) {
        string tkne_prefix = ".R/TKNE HK1 298";
        size_t idx = req.find(tkne_prefix, prev_idx);
        if(idx == string::npos) break;
        ostringstream buf;
        buf << setw(10) << setfill('0') << curr_no;
        req.replace(idx + tkne_prefix.size(), 10, buf.str());
        curr_no++;
        prev_idx = idx + 1;
    }

    {
        ofstream curr_no_file("curr_no");
        curr_no_file << curr_no;
    }
}

int nosir_pnl(int argc,char **argv)
{
    TDateTime scd_out;
    StrToDateTime("01.02.2015 11:00:00","dd.mm.yyyy hh:nn:ss",scd_out);

    string pult = "МОВДЕН";
    string opr = "DEN2";
    string passwd = "DEN2";
    TExec clnt(pult, opr);

    string buf = req_logon;
    replace(buf, "userr", opr);
    replace(buf, "passwd", passwd);

    clnt.exec(buf, false);

    for(int i = 2000; i <= 2170; i++) {
        string buf = req_load_pnl;
        ostringstream s;
        s << "UT" << i << "/" << DateTimeToStr(scd_out, "ddmmm") << " DME";
        replace_holder(buf, FLT, s.str());
        fill_pax(buf);
        etkt_gen(buf);
        clnt.exec(buf);
        OraSession.Commit();
    }
    return 0;
}

int nosir_etkt_gen(int argc,char **argv)
{
    string buf = req_load_pnl;
    etkt_gen(buf);
    cout << buf;
    return 1;
}

void make_flt(TExec &clnt, TDateTime scd_out, int flt_no)
{
    string airline = "ЮТ";
    string buf = req_create_flt;
    replace(buf, "flt_no", flt_no);
    replace(buf, "scd_out", DateTimeToStr(scd_out, ServerFormatDateTimeAsString));
    replace(buf, "scd_in", DateTimeToStr(scd_out + 1./24., ServerFormatDateTimeAsString));
    TAfterCreateFlt after_create_flt;
    clnt.exec(buf, true, &after_create_flt);

    buf = req_create_salon;
    replace(buf, "trip_id", after_create_flt.point_id);
    clnt.exec(buf);

    buf = req_load_pnl;
    ostringstream s;
    s << "UT" << flt_no << "/" << DateTimeToStr(scd_out, "ddmmm") << " DME";
    replace_holder(buf, FLT, s.str());
    fill_pax(buf);
    etkt_gen(buf);
    clnt.exec(buf);

    buf = req_open_reg;
    replace(buf, "point_id", after_create_flt.point_id);
    replace(buf, "act", DateTimeToStr(scd_out, ServerFormatDateTimeAsString), 2);
    clnt.exec(buf);

    buf = req_prep_reg;
    replace(buf, "point_id", after_create_flt.point_id);
    clnt.exec(buf);

    buf = req_brd_with_reg;
    replace_holder(buf, POINT_ID, after_create_flt.point_id, true);
    clnt.exec(buf);

    buf = req_tag_type;
    replace_holder(buf, POINT_ID, after_create_flt.point_id, true);
    clnt.exec(buf);

    buf = req_trip_exam_with_brd;
    replace_holder(buf, POINT_ID, after_create_flt.point_id, true);
    clnt.exec(buf);

    TQuery Qry(&OraSession);
    Qry.SQLText =
        "insert into created_flts( "
        "   time_created, "
        "   point_id, "
        "   flt_no, "
        "   scd_out, "
        "   pr_collect "
        ") values ( "
        "   :time_created, "
        "   :point_id, "
        "   :flt_no, "
        "   :scd_out, "
        "   :pr_collect "
        ")";
    Qry.CreateVariable("time_created", otDate, NowUTC());
    Qry.CreateVariable("point_id", otInteger, after_create_flt.point_id);
    Qry.CreateVariable("flt_no", otInteger, flt_no);
    Qry.CreateVariable("scd_out", otDate, scd_out);
    Qry.CreateVariable("pr_collect", otInteger, false);
    Qry.Execute();
    OraSession.Commit();
}

int nosir_den(int argc,char **argv)
{
    for(int i = 0; i < argc; i++)
        LogTrace(TRACE5) << argv[i];
    return 1;
}

int bp_tst(int argc,char **argv)
{
    string pult = "MOVDEN";
    string opr = "DEN";
    string passwd = "DEN";
    string term_version = "201609-0184743";
    TExec clnt(pult, opr);

    string buf = req_logon;
    replace(buf, "userr", opr);
    replace(buf, "passwd", passwd);
    replace(buf, "term_version", term_version);

    clnt.exec(buf, false);

    buf = req_print_bp;
    replace(buf, "grp_id", 2591139);
    clnt.exec(buf, true);

    return 0;
}

int nosir_make_flt(int argc,char **argv)
{
    string pult = "MOVDEN";
    string opr = "DEN";
    string passwd = "DEN";
    TExec clnt(pult, opr);

    string buf = req_logon;
    replace(buf, "userr", opr);
    replace(buf, "passwd", passwd);

    clnt.exec(buf, false);

    TDateTime day;

    StrToDateTime("01.02.2015 11:00:00","dd.mm.yyyy hh:nn:ss",day);

    LogTrace(TRACE5) << "day " << DateTimeToStr(day, "dd.mm.yyyy") << " started";
    for(int i = 2118; i <= 2170; i++) {
        make_flt(clnt, day, i);
    }
    LogTrace(TRACE5) << "day " << DateTimeToStr(day, "dd.mm.yyyy") << " finished";

    return 0;
}
