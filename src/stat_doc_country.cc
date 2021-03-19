#include "oralib.h"
#include "exceptions.h"
#include "qrys.h"
#include "passenger.h"
#include "obrnosir.h"
#include "stat/stat_utils.h"

#define NICKNAME "VLAD"
#define NICKTRACE SYSTEM_TRACE
#include "serverlib/slogger.h"

using namespace BASIC::date_time;
using namespace std;

class TMoveIds : public std::set< std::pair<TDateTime, int> >
{
public:
    void get_for_airp(TDateTime first_date, TDateTime last_date, const std::string& airp);
};

void TMoveIds::get_for_airp(TDateTime first_date, TDateTime last_date, const std::string& airp)
{
    clear();

    TReqInfo &reqInfo = *(TReqInfo::Instance());
    reqInfo.user.access.set_total_permit();
    reqInfo.user.access.merge_airps(TAccessElems<std::string>(airp, true));

    vector<TPointsRow> points;
    GetFltCBoxList(ssPaxList, first_date, last_date, false, points);
    for(vector<TPointsRow>::const_iterator i=points.begin(); i!=points.end(); ++i)
        insert(make_pair(i->part_key, i->move_id));
}

int arx_stat_belgorod(TDateTime part_key, int move_id, int& processed,
                      std::ofstream& farv, std::ofstream& fdep)
{
    LogTrace5 << " part_key : " << part_key << " move_id: " << move_id;
    std::string arx_pax_grp_airp_dep, arx_pax_grp_airp_arv;
    int arx_points_point_id, arx_pax_grp_grp_id, arx_pax_doc_pax_id;
    Dates::DateTime_t arx_pax_grp_part_key, arx_pax_doc_part_key;

    auto cur = make_db_curs(
                "SELECT arx_pax_grp.airp_dep, arx_pax_grp.airp_arv, arx_points.point_id "
                "arx_pax_grp.part_key, arx_pax_grp.grp_id, arx_pax_doc.part_key, arx_pax_doc.pax_id "
                "FROM arx_points, arx_pax_grp, arx_pax_doc "
                "WHERE arx_points.part_key = arx_pax_grp.part_key AND "
                "      arx_points.point_id = arx_pax_grp.point_dep AND "
                "      arx_points.part_key = :part_key AND "
                "      arx_points.move_id = :move_id AND "
                "      arx_points.pr_del >= 0 AND "
                "      arx_pax_grp.status NOT IN ('E') AND (arx_pax_grp.airp_dep='���' OR arx_pax_grp.airp_arv='���') AND "
                "      AND arx_pax_doc.issue_country<>'RUS' ",
                PgOra::getROSession("ARX_POINTS"));
    cur.def(arx_pax_grp_airp_dep)
       .def(arx_pax_grp_airp_arv)
       .def(arx_points_point_id)
       .def(arx_pax_grp_part_key)
       .def(arx_pax_grp_grp_id)
       .def(arx_pax_doc_part_key)
       .def(arx_pax_doc_pax_id)
       .bind(":part_key", part_key)
       .bind(":move_id", move_id)
       .exec();

    map< pair<TDateTime, int>, TTripInfo > flights;
    const string delim="\t";
    const string endl="\r\n";

    dbo::Session session;
    while(!cur.fen())
    {
        std::optional<dbo::ARX_PAX> arx_pax = session.query<dbo::ARX_PAX>()
                .where(" part_key = :arx_pax_grp_part_key AND part_key = :arx_pax_doc_part_key"
                       " grp_id = :arx_pax_grp_grp_id  AND "
                       " pax_id = :arx_pax_doc_pax_id AND refuse IS NULL")
                .setBind({{":arx_pax_grp_part_key", arx_pax_grp_part_key},
                          {":arx_pax_grp_grp_id", arx_pax_grp_grp_id},
                          {":arx_pax_doc_part_key", arx_pax_doc_part_key},
                          {":arx_pax_doc_pax_id", arx_pax_doc_pax_id}});

        CheckIn::TSimplePaxItem pax;
        if(!arx_pax) {
            continue;
        }
        pax.fromPax(*arx_pax);
        CheckIn::TPaxDocItem doc;
        if (!CheckIn::LoadPaxDoc(part_key, pax.id, doc)) continue;

        map< pair<TDateTime, int>, TTripInfo >::iterator flt =
                flights.insert(make_pair(make_pair(part_key, arx_points_point_id), TTripInfo())).first;
        if (flt==flights.end())
            throw EXCEPTIONS::Exception("%s: strange situation flt==flights.end()!", __FUNCTION__);

        if (flt->second.airline.empty())
            flt->second.getByPointId(part_key, arx_points_point_id);



        for(int pass=0; pass<2; pass++)
        {
            nosir_wait(processed, false, 20, 1);
            if ((pass==0 && arx_pax_grp_airp_arv != "���") ||
                    (pass!=0 && arx_pax_grp_airp_dep != "���")) continue;

            std::ofstream &f=(pass==0?farv:fdep);
            //���
            if (!doc.surname.empty() && !doc.first_name.empty())
                f << doc.full_name();
            else
                f << pax.full_name();
            f << delim;
            //��� ஦�����
            if (doc.birth_date!=ASTRA::NoExists)
                f << DateTimeToStr(doc.birth_date, "dd.mm.yyyy");
            f << delim;
            //����� ��ᯮ��
            f << doc.no << delim;
            //��� ��࠭�, �뤠�襩 ��ᯮ��
            f << doc.issue_country << delim;
            //���ࠢ�����
            f << (pass==0 ? arx_pax_grp_airp_dep : arx_pax_grp_airp_arv) << delim;
            //����� ३�
            f << flt->second.flight_view(ecNone, false, false) << delim;
            //��� �뫥� ३� (UTC)
            if (flt->second.scd_out!=ASTRA::NoExists)
                f << DateTimeToStr(flt->second.scd_out, "dd.mm.yyyy");
            f << endl;
        };
        processed++;
    }
    return 1;
}

int stat_belgorod(int argc, char **argv)
{
    TDateTime min_date, max_date;
    if (!getDateRangeFromArgs(argc, argv, min_date, max_date))
        return 1;

    TReqInfo::Instance()->Initialize("���");

    const string belgorod_airp="���";

    ostringstream fname_arv;
    fname_arv << "belgorod_frgn_arr_"
              << DateTimeToStr(min_date, "yyyy_mm_dd") << "-" << DateTimeToStr(max_date-1.0, "yyyy_mm_dd")
              << ".csv";
    ostringstream fname_dep;
    fname_dep << "belgorod_frgn_dep_"
              << DateTimeToStr(min_date, "yyyy_mm_dd") << "-" << DateTimeToStr(max_date-1.0, "yyyy_mm_dd")
              << ".csv";

    const string delim="\t";
    const string endl="\r\n";

    set<string> moscow_airps;
    TQuery Qry(&OraSession);
    Qry.Clear();
    Qry.SQLText =
            "SELECT code FROM airps WHERE city=:city AND pr_del=0";
    Qry.CreateVariable("city", otString, "���");
    Qry.Execute();
    for(;!Qry.Eof;Qry.Next())
        moscow_airps.insert(Qry.FieldAsString("code"));

    TCachedQuery PointsQry(
                "SELECT pax.*, pax_grp.airp_dep, pax_grp.airp_arv, points.point_id "
                "FROM points, pax_grp, pax, pax_doc "
                "WHERE points.point_id=pax_grp.point_dep AND "
                "      pax_grp.grp_id=pax.grp_id AND "
                "      pax.pax_id=pax_doc.pax_id AND "
                "      points.move_id=:move_id AND "
                "      points.pr_del>=0 AND "
                "      pax_grp.status NOT IN ('E') AND (pax_grp.airp_dep=:airp OR pax_grp.airp_arv=:airp) AND "
                "      pax.refuse IS NULL AND pax_doc.issue_country<>'RUS' ",
                QParams() << QParam("move_id", otInteger)
                << QParam("airp", otString, belgorod_airp));


    std::ofstream farv(fname_arv.str().c_str());
    std::ofstream fdep(fname_dep.str().c_str());
    try
    {
        if(!farv.is_open())
            throw EXCEPTIONS::Exception("Can't open file %s", fname_arv.str().c_str());

        if(!fdep.is_open())
            throw EXCEPTIONS::Exception("Can't open file %s", fname_dep.str().c_str());

        for(int pass=0; pass<2; pass++)
        {
            (pass==0?farv:fdep)
                    << "���" << delim
                    << "��� ஦�����" << delim
                    << "����� ��ᯮ��" << delim
                    << "��� ��࠭�, �뤠�襩 ��ᯮ��" << delim
                    << "���ࠢ�����" << delim
                    << "����� ३�" << delim
                    << "��� �뫥� ३� (UTC)" << endl;
        }

        int processed=0;
        for(; min_date<max_date; min_date+=1.0)
        {
            TMoveIds move_ids;
            try
            {
                move_ids.get_for_airp(min_date, min_date+1.0, belgorod_airp);
            }
            catch(AstraLocale::UserException) {}

            printf("%s: move_ids.size()=%zu\n", DateTimeToStr(min_date, "dd.mm.yyyy").c_str(), move_ids.size());
            for(TMoveIds::const_iterator i=move_ids.begin(); i!=move_ids.end(); ++i)
            {
                map< pair<TDateTime, int>, TTripInfo > flights;

                TDateTime part_key=i->first;
                int move_id=i->second;

                if(part_key != ASTRA::NoExists) {
                    arx_stat_belgorod(part_key, move_id, processed, farv, fdep);
                }
                else {

                    TQuery &PQry = PointsQry.get();
                    PQry.SetVariable("move_id", move_id);
                    PQry.Execute();
                    for(;!PQry.Eof;PQry.Next())
                    {
                        CheckIn::TSimplePaxItem pax;
                        pax.fromDB(PQry);
                        CheckIn::TPaxDocItem doc;
                        if (!CheckIn::LoadPaxDoc(part_key, pax.id, doc)) continue;

                        string airp_dep=PQry.FieldAsString("airp_dep");
                        string airp_arv=PQry.FieldAsString("airp_arv");
                        int point_id=PQry.FieldAsInteger("point_id");

                        map< pair<TDateTime, int>, TTripInfo >::iterator flt=flights.insert(make_pair(make_pair(part_key, point_id), TTripInfo())).first;
                        if (flt==flights.end())
                            throw EXCEPTIONS::Exception("%s: strange situation flt==flights.end()!", __FUNCTION__);

                        if (flt->second.airline.empty())
                            flt->second.getByPointId(part_key, point_id);

                        for(int pass=0; pass<2; pass++)
                        {
                            nosir_wait(processed, false, 20, 1);
                            if ((pass==0 && airp_arv!=belgorod_airp) ||
                                    (pass!=0 && airp_dep!=belgorod_airp)) continue;

                            std::ofstream &f=(pass==0?farv:fdep);
                            //���
                            if (!doc.surname.empty() && !doc.first_name.empty())
                                f << doc.full_name();
                            else
                                f << pax.full_name();
                            f << delim;
                            //��� ஦�����
                            if (doc.birth_date!=ASTRA::NoExists)
                                f << DateTimeToStr(doc.birth_date, "dd.mm.yyyy");
                            f << delim;
                            //����� ��ᯮ��
                            f << doc.no << delim;
                            //��� ��࠭�, �뤠�襩 ��ᯮ��
                            f << doc.issue_country << delim;
                            //���ࠢ�����
                            f << (pass==0?airp_dep:airp_arv) << delim;
                            //����� ३�
                            f << flt->second.flight_view(ecNone, false, false) << delim;
                            //��� �뫥� ३� (UTC)
                            if (flt->second.scd_out!=ASTRA::NoExists)
                                f << DateTimeToStr(flt->second.scd_out, "dd.mm.yyyy");
                            f << endl;
                        };
                        processed++;
                    }
                }
            }
        }
        if (farv.is_open()) farv.close();
        if (fdep.is_open()) fdep.close();
    }
    catch(...)
    {
        if (farv.is_open()) farv.close();
        if (fdep.is_open()) fdep.close();
        throw;
    }

    return 1;
}


int arx_ego_stat(int argc,char **argv)
{
    tst();
    TDateTime FirstDate, LastDate;
    if (!getDateRangeFromArgs(argc, argv, FirstDate, LastDate))
        return 1;

    DB::TQuery Qry(PgOra::getROSession("ARX_POINTS"));
    int processed=0;

    const string delim = ";";
    TEncodedFileStream of("cp1251",
                          (string)"ego_stat." +
                          DateTimeToStr(FirstDate, "ddmmyy") + "-" +
                          DateTimeToStr(LastDate, "ddmmyy") +
                          ".csv");
    of
            << "���" << delim
            << "��� ஦�����" << delim
            << "��ᯮ��" << delim
            << "���ࠢ����� �����" << delim
            << "��� ��५��" << endl;

    for(int step = 1; step < 2; step++) {
        string SQLText =
                "SELECT point_id, scd_out ";
        if(step == 1)
            SQLText += "   ,part_key ";
        SQLText += "FROM arx_points "
                   "WHERE scd_out>=:FirstDate AND scd_out<:LastDate AND pr_reg<>0 AND pr_del>=0";
        Qry.ClearParams();
        Qry.SQLText= SQLText;
        Qry.CreateVariable("FirstDate", otDate, FirstDate);
        Qry.CreateVariable("LastDate", otDate, LastDate);
        Qry.Execute();
        list< pair<int, pair<TDateTime, TDateTime> > > point_ids;
        for(;!Qry.Eof;Qry.Next()) {
            TDateTime part_key = ASTRA::NoExists;
            if(step == 1)
                part_key = Qry.FieldAsDateTime("part_key");
            point_ids.push_back(
                        make_pair(
                            Qry.FieldAsInteger("point_id"),
                            make_pair(
                                Qry.FieldAsDateTime("scd_out"),
                                part_key
                                )
                            )
                        );
        }

        Qry.ClearParams();
        SQLText =
                "SELECT "
                "   arx_pax.surname, "
                "   arx_pax.name, "
                "   arx_pax_doc.birth_date, "
                "   arx_pax_doc.no, "
                "   arx_pax_grp.airp_arv "
                "FROM "
                "   arx_pax_grp , "
                "   arx_pax , "
                "   arx_pax_doc  "
                "WHERE ";
        if(step == 1) {
            SQLText +=
                    "   arx_pax_grp.part_key = :part_key and "
                    "   arx_pax.part_key = :part_key and "
                    "   arx_pax_doc.part_key = :part_key and ";
            Qry.DeclareVariable("part_key", otDate);
        }
        SQLText +=
                "   arx_pax_grp.grp_id=pax.grp_id AND arx_pax.pax_id=arx_pax_doc.pax_id AND "
                "   arx_pax_grp.airp_dep = '���' and "
                "   arx_pax_doc.no like '20%' and "
                "   arx_pax_grp.status NOT IN ('E') AND arx_pax_grp.point_dep=:point_id ";

        Qry.SQLText= SQLText;
        Qry.DeclareVariable("point_id", otInteger);
        for(auto && [point_id, date_pair] :  point_ids)
        {
            Qry.SetVariable("point_id", point_id);
            if(step == 1) {
                Qry.SetVariable("part_key", date_pair.second);
            }
            Qry.Execute();
            for(;!Qry.Eof;Qry.Next())
            {
                of
                        << (string) Qry.FieldAsString("surname") + " " +Qry.FieldAsString("name") << delim
                        << DateTimeToStr(Qry.FieldAsDateTime("birth_date"), "dd.mm.yyyy") << delim
                        << Qry.FieldAsString("no") << delim
                        << ElemIdToNameLong(etAirp, Qry.FieldAsString("airp_arv")) << delim
                        << DateTimeToStr(date_pair.first, "dd.mm.yyyy") << endl;
            }
            processed++;
            nosir_wait(processed, false, 10, 0);
        }
    }
    return 0;
}

int ego_stat(int argc,char **argv)
{
    TDateTime FirstDate, LastDate;
    if (!getDateRangeFromArgs(argc, argv, FirstDate, LastDate))
        return 1;

    TQuery Qry(&OraSession);
    int processed=0;

    const string delim = ";";
    TEncodedFileStream of("cp1251",
                          (string)"ego_stat." +
                          DateTimeToStr(FirstDate, "ddmmyy") + "-" +
                          DateTimeToStr(LastDate, "ddmmyy") +
                          ".csv");
    of
            << "���" << delim
            << "��� ஦�����" << delim
            << "��ᯮ��" << delim
            << "���ࠢ����� �����" << delim
            << "��� ��५��" << endl;

    string SQLText = "SELECT point_id, scd_out "
                     "FROM points "
                     "WHERE scd_out>=:FirstDate AND scd_out<:LastDate AND pr_reg<>0 AND pr_del>=0";
    Qry.Clear();
    Qry.SQLText= SQLText;
    Qry.CreateVariable("FirstDate", otDate, FirstDate);
    Qry.CreateVariable("LastDate", otDate, LastDate);
    Qry.Execute();
    list< pair<int, pair<TDateTime, TDateTime> > > point_ids;
    for(;!Qry.Eof;Qry.Next()) {
        TDateTime part_key = ASTRA::NoExists;
        point_ids.push_back(
                    make_pair(
                        Qry.FieldAsInteger("point_id"),
                        make_pair(
                            Qry.FieldAsDateTime("scd_out"),
                            part_key
                            )
                        )
                    );
    }

    Qry.Clear();
    SQLText =
            "SELECT "
            "   pax.surname, "
            "   pax.name, "
            "   pax_doc.birth_date, "
            "   pax_doc.no, "
            "   pax_grp.airp_arv "
            "FROM "
            "   pax_grp, "
            "   pax, "
            "   pax_doc "
            "WHERE "
            "   pax_grp.grp_id=pax.grp_id AND pax.pax_id=pax_doc.pax_id AND "
            "   pax_grp.airp_dep = '���' and "
            "   pax_doc.no like '20%' and "
            "   pax_grp.status NOT IN ('E') AND pax_grp.point_dep=:point_id ";

    Qry.SQLText= SQLText;
    Qry.DeclareVariable("point_id", otInteger);
    for(list< pair<int, pair<TDateTime, TDateTime> > >::const_iterator i=point_ids.begin(); i!=point_ids.end(); ++i)
    {
        Qry.SetVariable("point_id", i->first);
        Qry.Execute();
        for(;!Qry.Eof;Qry.Next())
        {
            of
                    << (string)
                       Qry.FieldAsString("surname") + " " +
                       Qry.FieldAsString("name") << delim
                    << DateTimeToStr(Qry.FieldAsDateTime("birth_date"), "dd.mm.yyyy") << delim
                    << Qry.FieldAsString("no") << delim
                    << ElemIdToNameLong(etAirp, Qry.FieldAsString("airp_arv")) << delim
                    << DateTimeToStr(i->second.first, "dd.mm.yyyy") << endl;
        }
        processed++;
        nosir_wait(processed, false, 10, 0);
    }

    arx_ego_stat(argc, argv);

    return 0;
}

