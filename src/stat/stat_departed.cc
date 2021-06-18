#include "stat_departed.h"
#include "astra_utils.h"
#include "dev_consts.h"
#include "stat_common.h"
#include "passenger.h"
#include "stat_utils.h"
#include "baggage_ckin.h"

#define NICKNAME "DENIS"
#include "serverlib/slogger.h"

using namespace std;
using namespace ASTRA;
using namespace BASIC::date_time;

void arx_departed_flt(DB::TQuery &Qry, TEncodedFileStream &of)
{
    LogTrace5 << __func__;
    int point_id = Qry.FieldAsInteger("point_id");
    TDateTime part_key = NoExists;
    if(!Qry.FieldIsNULL("part_key")) {
        part_key = Qry.FieldAsDateTime("part_key");
    }
    TRegEvents events;
    events.fromArxDB(part_key, point_id);
    LogTrace5 << " reg_events size: " << events.size();
    DB::TQuery paxQry(PgOra::getROSession("ARX_PAX_GRP"), STDLOG);
    paxQry.CreateVariable("point_id", otInteger, point_id);
    paxQry.CreateVariable("part_key", otDate, part_key);

    string SQLText =
        "select \n"
        "   arx_pax.pax_id, \n"
        "   arx_pax.bag_pool_num, \n"
        "   arx_pax.name, \n"
        "   arx_pax.surname, \n"
        "   arx_pax.ticket_no, \n"
        "   arx_pax.coupon_no, \n"
        "   arx_pax.pax_id, \n"
        "   arx_pax.grp_id, \n"
        "   arx_pax.reg_no, \n"
        "   arx_pax_grp.client_type, \n"
        "   arx_pax_grp.airp_arv \n"
        "from arx_pax ,arx_pax_grp \n"
        "where arx_pax_grp.point_dep = :point_id and arx_pax_grp.grp_id = arx_pax.grp_id \n"
        "      and arx_pax.part_key = :part_key and arx_pax_grp.part_key = :part_key \n";

    string delim = ";";

    paxQry.SQLText = SQLText;
    paxQry.Execute();
    TAirpArvInfo airp_arv_info;
    for(; not paxQry.Eof; paxQry.Next()) {
        int pax_id = paxQry.FieldAsInteger("pax_id");
        int bag_pool_num = paxQry.FieldAsInteger("bag_pool_num");
        int grp_id = paxQry.FieldAsInteger("grp_id");
        int reg_no = paxQry.FieldAsInteger("reg_no");
        string name = paxQry.FieldAsString("name");
        string surname = paxQry.FieldAsString("surname");

        string airline = Qry.FieldAsString("airline");
        int flt_no = Qry.FieldAsInteger("flt_no");
        string suffix = Qry.FieldAsString("suffix");
        string airp = Qry.FieldAsString("airp");
        TDateTime scd_out = Qry.FieldAsDateTime("scd_out");
        ostringstream flt_str;
        flt_str
            << airline
            << setw(3) << setfill('0') << flt_no
            << (suffix.empty() ? "" : suffix);

        string ticket_no = paxQry.FieldAsString("ticket_no");
        string coupon_no = paxQry.FieldAsString("coupon_no");
        ostringstream ticket;
        if(not ticket_no.empty())
            ticket << ticket_no << (coupon_no.empty() ? "" : "/") << coupon_no;

        string pnr_addr;

        CheckIn::TPaxDocItem doc;
        LoadPaxDoc(part_key, paxQry.FieldAsInteger("pax_id"), doc);
        string birth_date = (doc.birth_date!=ASTRA::NoExists?DateTimeToStr(doc.birth_date, "dd.mmm.yy"):"");
        string gender = doc.gender;

        of
            // ���
            << name << (name.empty() ? "" : " ") << surname << delim
            // ��� ஦�����
            << birth_date << delim
            // ���
            << gender << delim
            // ��� ���㬥��
            << doc.type << delim
            // ���� � ����� ���㬥��
            << doc.no << delim
            // ����� �����
            << ticket.str() << delim
            // ����� �஭�஢����
            << pnr_addr << delim
            // ����
            << flt_str.str() << delim
            // ��� �뫥�
            << DateTimeToStr(scd_out, "ddmmm") << delim
            // ��
            << airp << delim
            // ��
            << airp_arv_info.get(paxQry) << delim
            // ����� ����
             <<  CKIN::get_bagAmount2(GrpId_t(grp_id), pax_id, bag_pool_num, DateTimeToBoost(part_key)).value_or(0) << delim
            // ����� ���
            << CKIN::get_bagWeight2(GrpId_t(grp_id), pax_id, bag_pool_num, DateTimeToBoost(part_key)).value_or(0) << delim;
        // �६� ॣ����樨
        TRegEvents::const_iterator evt = events.find(make_pair(grp_id, reg_no));
        if(evt != events.end())
            of << DateTimeToStr(evt->second.first, "dd.mm.yyyy hh:nn:ss") << delim;
        // ���ᮡ ॣ����樨
        of << paxQry.FieldAsString("client_type") << endl;
    }
}

bool existsConfirmPrint(TDevOper::Enum op_type, int pax_id)
{
    DB::TCachedQuery Qry(
            PgOra::getROSession("CONFIRM_PRINT"),
            "SELECT 1 "
            "FROM confirm_print "
            "WHERE pax_id=:pax_id "
            "AND voucher IS NULL "
            "AND pr_print<>0 "
            "AND op_type = :op_type "
            "AND client_type='TERM' "
            "FETCH FIRST 1 ROWS ONLY ",
            QParams() << QParam("pax_id", otInteger, pax_id) << QParam("op_type", otString, DevOperTypes().encode(op_type)),
            STDLOG);
    Qry.get().Execute();
    return (not Qry.get().Eof);
}

void departed_flt(DB::TQuery &Qry, TEncodedFileStream &of)
{
    int point_id = Qry.FieldAsInteger("point_id");
    TDateTime part_key = NoExists;
    if(not Qry.FieldIsNULL("part_key")) {
        part_key = Qry.FieldAsDateTime("part_key");
    }
    if(part_key != NoExists) {
        return arx_departed_flt(Qry, of);
    }

    TRegEvents events;
    events.fromDB(part_key, point_id);
    TQuery paxQry(&OraSession);
    paxQry.CreateVariable("point_id", otInteger, point_id);

    string SQLText =
        "select \n"
        "   pax.name, \n"
        "   pax.surname, \n"
        "   pax.ticket_no, \n"
        "   pax.coupon_no, \n"
        "   pax.pax_id, \n"
        "   pax.grp_id, \n"
        "   pax.reg_no, \n"
        "   pax_grp.client_type, \n"
        "   pax_grp.airp_arv, \n"
        "   salons.get_seat_no(pax.pax_id,pax.seats,NULL,NULL,NULL,'list',NULL,0) AS seat_no, "
        "   NVL(ckin.get_bagAmount2(pax.grp_id,pax.pax_id,pax.bag_pool_num,rownum),0) bag_amount, \n"
        "   NVL(ckin.get_bagWeight2(pax.grp_id,pax.pax_id,pax.bag_pool_num,rownum),0) bag_weight \n";
    SQLText +=  "from pax, pax_grp \n"
                "where pax_grp.point_dep = :point_id and \n"
                "      pax_grp.grp_id = pax.grp_id \n";

    string delim = ";";

    paxQry.SQLText = SQLText;
    paxQry.Execute();
    TAirpArvInfo airp_arv_info;
    for(; not paxQry.Eof; paxQry.Next()) {
        int grp_id = paxQry.FieldAsInteger("grp_id");
        int reg_no = paxQry.FieldAsInteger("reg_no");
        string name = paxQry.FieldAsString("name");
        string surname = paxQry.FieldAsString("surname");

        string airline = Qry.FieldAsString("airline");
        int flt_no = Qry.FieldAsInteger("flt_no");
        string suffix = Qry.FieldAsString("suffix");
        string airp = Qry.FieldAsString("airp");
        TDateTime scd_out = Qry.FieldAsDateTime("scd_out");
        ostringstream flt_str;
        flt_str
            << airline
            << setw(3) << setfill('0') << flt_no
            << (suffix.empty() ? "" : suffix);

        string ticket_no = paxQry.FieldAsString("ticket_no");
        string coupon_no = paxQry.FieldAsString("coupon_no");
        ostringstream ticket;
        if(not ticket_no.empty())
            ticket << ticket_no << (coupon_no.empty() ? "" : "/") << coupon_no;

        string pnr_addr;
        pnr_addr=TPnrAddrs().firstAddrByPaxId(paxQry.FieldAsInteger("pax_id"), TPnrAddrInfo::AddrAndAirline);

        const bool term_bp = existsConfirmPrint(TDevOper::PrnBP, paxQry.FieldAsInteger("pax_id"));

        CheckIn::TPaxDocItem doc;
        LoadPaxDoc(part_key, paxQry.FieldAsInteger("pax_id"), doc);
        string birth_date = (doc.birth_date!=ASTRA::NoExists?DateTimeToStr(doc.birth_date, "dd.mmm.yy"):"");
        string gender = doc.gender;

        of
            // ���
            << name << (name.empty() ? "" : " ") << surname << delim
            // ��� ஦�����
            << birth_date << delim
            // ���
            << gender << delim
            // ��� ���㬥��
            << doc.type << delim
            // ���� � ����� ���㬥��
            << doc.no << delim
            // ����� �����
            << ticket.str() << delim
            // ����� �஭�஢����
            << pnr_addr << delim
            // ����
            << flt_str.str() << delim
            // ��� �뫥�
            << DateTimeToStr(scd_out, "ddmmm") << delim
            // ��
            << airp << delim
            // ��
            << airp_arv_info.get(paxQry) << delim
            // ����� ����
            << paxQry.FieldAsInteger("bag_amount") << delim
            // ����� ���
            << paxQry.FieldAsInteger("bag_weight") << delim;
        // �६� ॣ����樨
        TRegEvents::const_iterator evt = events.find(make_pair(grp_id, reg_no));
        if(evt != events.end())
            of << DateTimeToStr(evt->second.first, "dd.mm.yyyy hh:nn:ss") << delim;
        // ����� ����
        of << paxQry.FieldAsString("seat_no") << delim;
        // ���ᮡ ॣ����樨
        of << paxQry.FieldAsString("client_type") << delim;
        // ����� �� �� �⮩��
        of << (!term_bp ? "���" : "��") << endl;
    }
}

void arx_departed_month(const pair<TDateTime, TDateTime> &interval, TEncodedFileStream &of)
{
    LogTrace5 << __func__;
    DB::TQuery Qry(PgOra::getROSession("ARX_POINTS"), STDLOG);
    Qry.CreateVariable("FirstDate", otDate, interval.first);
    Qry.CreateVariable("LastDate", otDate, interval.second);
    for(int pass = 1; pass <= 2; pass++) {
        Qry.CreateVariable("arx_trip_date_range", otDate,  interval.second+ARX_TRIP_DATE_RANGE());
        ostringstream sql;
        sql << "SELECT arx_points.part_key, point_id, airline, flt_no, suffix, airp, scd_out \n"
               "FROM arx_points \n";
        if (pass==2) {
            sql << getMoveArxQuery();
        }
        sql << "WHERE \n";
        if (pass==1)
            sql << " arx_points.part_key >= :FirstDate AND arx_points.part_key < :arx_trip_date_range AND \n";
        if (pass==2)
            sql << " arx_points.part_key=arx_ext.part_key AND arx_points.move_id=arx_ext.move_id AND \n";
        sql <<
            "   scd_out>=:FirstDate AND scd_out<:LastDate AND airline='��' AND "
            "      pr_reg<>0 AND pr_del>=0";

        Qry.SQLText = sql.str().c_str();
        LogTrace5 << "query: " << sql.str();
        Qry.Execute();
        for(; not Qry.Eof; Qry.Next())
            arx_departed_flt(Qry, of);
    }
}

void departed_month(const pair<TDateTime, TDateTime> &interval, TEncodedFileStream &of)
{
    DB::TQuery Qry(*get_main_ora_sess(STDLOG), STDLOG);
    Qry.CreateVariable("first_date", otDate, interval.first);
    Qry.CreateVariable("last_date", otDate, interval.second);
    ostringstream sql;
    sql << "SELECT null part_key, point_id, airline, flt_no, suffix, airp, scd_out  \n"
           "FROM points \n"
           "WHERE scd_out>=:first_date AND scd_out<:last_date AND airline='��' AND "
           "    pr_reg<>0 AND pr_del>=0";

    /*
       ofstream out(IntToString(pass) + ".sql");
       out << sql.str();
       out.close();
       */

    Qry.SQLText = sql.str().c_str();
    LogTrace5 << "query: " << sql.str();
    Qry.Execute();
    for(; not Qry.Eof; Qry.Next()) {
        departed_flt(Qry, of);
    }
    arx_departed_month(interval, of);
}

int nosir_departed(int argc, char **argv)
{
    if(argc != 3) {
        cout << "usage: " << argv[0] << " yyyymmdd yyyymmdd" << endl;
        return 1;
    }
    TPerfTimer tm;
    tm.Init();
    TDateTime FirstDate, LastDate;

    if(StrToDateTime(argv[1], "yyyymmdd", FirstDate) == EOF) {
        cout << "wrong first date: " << argv[1] << endl;
        return 1;
    }

    if(StrToDateTime(argv[2], "yyyymmdd", LastDate) == EOF) {
        cout << "wrong last date: " << argv[1] << endl;
        return 1;
    }

    TPeriods period;
    period.get(FirstDate, LastDate);
    string delim = ";";
    for(TPeriods::TItems::iterator i = period.items.begin(); i != period.items.end(); i++) {
        TEncodedFileStream of("cp1251", (string)"departed." + DateTimeToStr(i->first, "yymm") + ".csv");
        of
            << "���" << delim
            << "��� ஦�����" << delim
            << "���" << delim
            << "��� ���㬥��" << delim
            << "���� � ����� ���㬥��" << delim
            << "����� �����" << delim
            << "����� �஭�஢����" << delim
            << "����" << delim
            << "��� �뫥�" << delim
            << "��" << delim
            << "��" << delim
            << "����� ����" << delim
            << "����� ���" << delim
            << "�६� ॣ����樨 (UTC)" << delim
            << "����� ����" << delim
            << "���ᮡ ॣ����樨" << delim
            << "����� �� �� �⮩��" << endl;
        departed_month(*i, of);
    }
    LogTrace(TRACE5) << "time: " << tm.PrintWithMessage();
    return 1;
}

int nosir_departed_pax(int argc, char **argv)
{
    TDateTime FirstDate, LastDate;
    StrToDateTime("01.04.2016 00:00:00", "dd.mm.yyyy hh:nn:ss", FirstDate);
    LastDate = NowUTC();
    TQuery Qry(&OraSession);
    Qry.SQLText =
    "SELECT point_id, airline, flt_no, suffix, airp, scd_out  FROM points "
    "WHERE scd_out>=:FirstDate AND scd_out<:LastDate AND airline='��' AND "
    "      pr_reg<>0 AND pr_del>=0";
    Qry.CreateVariable("FirstDate", otDate, FirstDate);
    Qry.CreateVariable("LastDate", otDate, LastDate);

    TQuery paxQry(&OraSession);
    paxQry.SQLText =
        "select "
        "   pax.name, "
        "   pax.surname, "
        "   pax.ticket_no, "
        "   pax.coupon_no, "
        "   crs_pnr.pnr_id, "
        "   pax.pax_id "
        "from "
        "   pax, pax_grp, crs_pnr, crs_pax where "
        "   pax.pax_id = crs_pax.pax_id(+) and "
        "   crs_pax.pr_del(+)=0 and "
        "   crs_pax.pnr_id = crs_pnr.pnr_id(+) and "
        "   pax_grp.point_dep = :point_id and "
        "   pax_grp.grp_id = pax.grp_id ";
    paxQry.DeclareVariable("point_id", otInteger);

    Qry.Execute();
    bool pr_header = true;
    string delim = ";";
    ofstream of;
    TTripRoute route;
    for(; not Qry.Eof; Qry.Next()) {
        int point_id = Qry.FieldAsInteger("point_id");
        route.GetRouteAfter(NoExists, point_id, trtNotCurrent, trtNotCancelled);
        string airp_arv;
        if(not route.empty())
            airp_arv = route.begin()->airp;

        paxQry.SetVariable("point_id", point_id);
        paxQry.Execute();
        for(; not paxQry.Eof; paxQry.Next()) {
            if(pr_header) {
                pr_header = false;
                of.open("pax_departed.csv");
                of
                    << "���" << delim
                    << "����� �����" << delim
                    << "����� �஭�஢����" << delim
                    << "����" << delim
                    << "��� �뫥�" << delim
                    << "���ࠢ�����" << delim
                    << "��� ஦�����" << delim
                    << "���" << endl;
            }
            string name = paxQry.FieldAsString("name");
            string surname = paxQry.FieldAsString("surname");

            string airline = Qry.FieldAsString("airline");
            int flt_no = Qry.FieldAsInteger("flt_no");
            string suffix = Qry.FieldAsString("suffix");
            string airp = Qry.FieldAsString("airp");
            TDateTime scd_out = Qry.FieldAsDateTime("scd_out");
            ostringstream flt_str;
            flt_str
                << airline
                << setw(3) << setfill('0') << flt_no
                << (suffix.empty() ? "" : suffix);

            string ticket_no = paxQry.FieldAsString("ticket_no");
            string coupon_no = paxQry.FieldAsString("coupon_no");
            ostringstream ticket;
            if(not ticket_no.empty())
                ticket << ticket_no << (coupon_no.empty() ? "" : "/") << coupon_no;

            string pnr_addr=TPnrAddrs().firstAddrByPnrId(paxQry.FieldAsInteger("pnr_id"), TPnrAddrInfo::AddrAndAirline);

            CheckIn::TPaxDocItem doc;
            LoadPaxDoc(paxQry.FieldAsInteger("pax_id"), doc);
            string birth_date = (doc.birth_date!=ASTRA::NoExists?DateTimeToStr(doc.birth_date, "dd.mmm.yy"):"");
            string gender = doc.gender;

            of
                // ���
                << name << (name.empty() ? "" : " ") << surname << delim
                // ��� ஦�����
                << birth_date << delim
                // ���
                << gender << delim
                // ��� ���㬥��
                << delim
                // ���� ���㬥��
                << delim
                // ����� ���㬥��
                << delim
                // ����� �����
                << ticket.str() << delim
                // ����� �஭�஢����
                << pnr_addr << delim
                // ����
                << flt_str.str() << delim
                // ��� �뫥�
                << DateTimeToStr(scd_out, "ddmmm") << delim
                // ��
                << delim
                // ��
                << route.begin()->airp << delim;
        }
    }
    return 1;
}

int nosir_departed_sql(int argc, char **argv)
{
    TQuery Qry(&OraSession);
    Qry.SQLText = "select :part_key part_key from dual";
    Qry.CreateVariable("part_key", otDate, FNull);
    Qry.Execute();
//    departed_flt(Qry);
    Qry.SetVariable("part_key", NowUTC());
    Qry.Execute();
//    departed_flt(Qry);
    return 1;
}
