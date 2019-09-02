#include "stat_ovb.h"
#include "date_time.h"
#include "qrys.h"
#include "passenger.h"

using namespace std;
using namespace BASIC::date_time;
using namespace ASTRA;
using namespace EXCEPTIONS;

string getCountry(int point_id, TDateTime part_key, TDateTime &scd_in, TDateTime &scd_out, string &airp)
{
    QParams QryParams;
    QryParams << QParam("point_id", otInteger, point_id);
    if(part_key != NoExists) QryParams << QParam("part_key", otDate, part_key);
    string SQL = (string)
            "select cities.country, scd_out, scd_in, airp from "
            "   " + (part_key == NoExists ? "" : "arx_points") + " points, "
            "   airps, "
            "   cities "
            "where "
            "   points.point_id = :point_id and " +
            (part_key == NoExists ? "" : "   points.part_key = :part_key and ") +
            "   points.airp = airps.code and "
            "   airps.city = cities.code ";
    TCachedQuery Qry(SQL, QryParams);
    Qry.get().Execute();
    if(Qry.get().Eof)
        throw Exception("country code not found for point_id = %d", point_id);
    scd_in = Qry.get().FieldAsDateTime("scd_in");
    scd_out = Qry.get().FieldAsDateTime("scd_out");
    airp = Qry.get().FieldAsString("airp");
    return Qry.get().FieldAsString(0);
}

void collect(map<string, int> &result, TDateTime from, TDateTime to)
{
    TPerfTimer tm;
    tm.Init();
    for(int oper = 0; oper < 2; oper++) { // oper == 0 archive tables used, otherwise operating tables
        if(oper)
            cout << "process operating tables" << endl;
        else
            cout << "process archive tables" << endl;
        string pointsSQL = (string)
            "select point_id " +
            (oper ? "" : ", part_key ") +
            "from " +
            (oper ? "points" : "arx_points") +
            " where "
            "   ((scd_out >= :from_date and "
            "   scd_out < :to_date) or "
            "   (scd_in >= :from_date and "
            "   scd_in < :to_date)) and "
            "   airp = :airp";

        string grpSQL = (string)
            "select grp_id, point_dep, point_arv from " +
            (oper ? "pax_grp" : "arx_pax_grp") +
            " where "
            "   (point_dep = :point_id or point_arv = :point_id) and "
            "   status not in ('E') " +
            (oper ? "" : " and part_key = :part_key");

        string paxSQL = (string)
            "select pax_id from " + (oper ? "pax" : "arx_pax") + " where "
            "   grp_id = :grp_id and "
            "   refuse is null " +
            (oper ? "" : " and part_key = :part_key");

        QParams QryParams;
        QryParams
            << QParam("from_date", otDate, from)
            << QParam("to_date", otDate, to)
            << QParam("airp", otString, "ТЛЧ");
        TCachedQuery Qry(pointsSQL, QryParams);
        Qry.get().Execute();
        int flights = 0;
        int pax_count = 0;
        for(; not Qry.get().Eof; Qry.get().Next(), flights++) {
            int point_id = Qry.get().FieldAsInteger("point_id");
            TDateTime part_key = NoExists;
            QParams grpParams;
            grpParams << QParam("point_id", otInteger, point_id);
            if(not oper) {
                part_key = Qry.get().FieldAsDateTime("part_key");
                grpParams << QParam("part_key", otDate, part_key);
            }
            TCachedQuery grpQry(grpSQL, grpParams);
            grpQry.get().Execute();
            for(; not grpQry.get().Eof; grpQry.get().Next()) {
                int grp_id = grpQry.get().FieldAsInteger("grp_id");
                int point_dep = grpQry.get().FieldAsInteger("point_dep");
                int point_arv = grpQry.get().FieldAsInteger("point_arv");
                TDateTime dep_scd_in, dep_scd_out;
                TDateTime arv_scd_in, arv_scd_out;
                string dep_airp, arv_airp;
                if(getCountry(point_dep, part_key, dep_scd_in, dep_scd_out, dep_airp) != "РФ") continue;
                if(getCountry(point_arv, part_key, arv_scd_in, arv_scd_out, arv_airp) != "РФ") continue;
                QParams paxParams;
                paxParams << QParam("grp_id", otInteger, grp_id);
                if(not oper) paxParams << QParam("part_key", otDate, part_key);
                TCachedQuery paxQry(paxSQL, paxParams);
                paxQry.get().Execute();
                for(; not paxQry.get().Eof; paxQry.get().Next()) {
                    int pax_id = paxQry.get().FieldAsInteger("pax_id");
                    CheckIn::TPaxDocItem doc;
                    LoadPaxDoc(part_key, pax_id, doc);
                    if(doc.type != "P") continue;
                    string no_begin = doc.no.substr(0, 2);
                    if(
                            no_begin == "52" or
                            no_begin == "04" or
                            no_begin == "69" or
                            no_begin == "32" or
                            no_begin == "01"
                      ) {
                        cout
                            << setw(10) << pax_id
                            << setw(20) << doc.no
                            << setw(4) << dep_airp
                            << "(" << DateTimeToStr(dep_scd_out, ServerFormatDateTimeAsString) << ")"
                            << " -> "
                            << setw(4) << arv_airp
                            << "(" << DateTimeToStr(arv_scd_in, ServerFormatDateTimeAsString) << ")"
                            << endl;

                        result[no_begin]++;
                        pax_count++;
                        if(pax_count % 10 == 0) cout << pax_count << " pax processed" << endl;
                    }
                }
            }
        }
        if(pax_count % 10 != 0) cout << pax_count << " pax processed" << endl;
        cout << "flights: " << flights << endl;
    }
    cout << "interval time: " << tm.PrintWithMessage() << endl;
}

int STAT::ovb(int argc,char **argv)
{
    TPerfTimer tm;
    tm.Init();
    TDateTime from, to;
    StrToDateTime("01.01.2015 00:00:00","dd.mm.yyyy hh:nn:ss",from);
//    StrToDateTime("01.08.2015 00:00:00","dd.mm.yyyy hh:nn:ss",from);

    QParams QryParams;
    QryParams << QParam("from_date", otDate, from);
    TCachedQuery Qry("select last_day(:from_date) from dual", QryParams);
    map<string, int> result;
    for(int i = 0; i < 6; i++) {
//    for(int i = 0; i < 1; i++) {
        cout << "------ loop " << i << " ------" << endl;
        Qry.get().Execute();
        to = Qry.get().FieldAsDateTime(0) + 1;
        cout << "from: " << DateTimeToStr(from, ServerFormatDateTimeAsString) << endl;
        cout << "to: " << DateTimeToStr(to, ServerFormatDateTimeAsString) << endl;
        collect(result, from, to);
        from = to;
        Qry.get().SetVariable("from_date", from);
    }
    for(map<string, int>::iterator i = result.begin(); i != result.end(); i++) {
        cout << "'" << i->first << "' -> " << i->second << endl;
    }


    cout << "time: " << tm.PrintWithMessage() << endl;
    return 1; // 0 - изменения коммитятся, 1 - rollback
}

