#include "stat_ovb.h"
#include "date_time.h"
#include "qrys.h"
#include "passenger.h"

#define NICKNAME "DENIS"
#include "serverlib/slogger.h"

using namespace std;
using namespace BASIC::date_time;
using namespace ASTRA;
using namespace EXCEPTIONS;

string getCountryARX(int point_id, TDateTime part_key, TDateTime &scd_in, TDateTime &scd_out, string &airp)
{
    LogTrace5 << __func__ << " point_id: " << point_id << " part_key: " << part_key;
    QParams QryParams;
    QryParams << QParam("point_id", otInteger, point_id);
    QryParams << QParam("part_key", otDate, part_key);
    std::string point_airp;
    TDateTime point_scd_in, point_scd_out;
    auto cur = make_db_curs("select airp, scd_in, scd_out from ARX_POINTS where point_id = :point_id and part_key = :part_key",
                            PgOra::getROSession("ARX_POINTS"));
    cur.def(point_airp)
       .def(point_scd_in)
       .def(point_scd_out)
       .bind(":point_id", point_id)
       .bind(":part_key", part_key)
       .EXfet();

    if(cur.err() == DbCpp::ResultCode::NoDataFound) {
        throw Exception("Not found airp for point_id = %d", point_id);
    }

    airp = point_airp;
    scd_in = point_scd_in;
    scd_out = point_scd_out;

    CountryCode_t country =  getCountryByAirp(AirportCode_t{airp});
    return country.get();
}

string getCountry(int point_id, TDateTime part_key, TDateTime &scd_in, TDateTime &scd_out, string &airp)
{
    if(part_key != NoExists) {
        return getCountryARX(point_id, part_key, scd_in, scd_out, airp);
    }
    QParams QryParams;
    QryParams << QParam("point_id", otInteger, point_id);
    string SQL = (string)
            "select cities.country, scd_out, scd_in, airp from "
            "   points, "
            "   airps, "
            "   cities "
            "where "
            "   points.point_id = :point_id and " +
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

void collectArx(map<string, int> &result, TDateTime from, TDateTime to)
{
    TPerfTimer tm;
    tm.Init();
    cout << "process archive tables" << endl;
    string pointsSQL =
        "select point_id , part_key "
        "from "
        "   arx_points"
        "where "
        "   ((scd_out >= :from_date and "
        "   scd_out < :to_date) or "
        "   (scd_in >= :from_date and "
        "   scd_in < :to_date)) and "
        "   airp = :airp";

    string grpSQL =
        "select grp_id, point_dep, point_arv "
        "from "
        "   arx_pax_grp"
        "where "
        "   (point_dep = :point_id or point_arv = :point_id) and "
        "   status not in ('E') "
        " and part_key = :part_key";

    string paxSQL =
        "select pax_id "
        "from arx_pax "
        "where "
        "   grp_id = :grp_id and refuse is null and part_key = :part_key";

    QParams QryParams;
    QryParams
        << QParam("from_date", otDate, from)
        << QParam("to_date", otDate, to)
        << QParam("airp", otString, "íãó");
    DB::TCachedQuery Qry(PgOra::getROSession("ARX_POINTS"), pointsSQL, QryParams, STDLOG);
    Qry.get().Execute();
    int flights = 0;
    int pax_count = 0;
    for(; not Qry.get().Eof; Qry.get().Next(), flights++) {
        int point_id = Qry.get().FieldAsInteger("point_id");
        QParams grpParams;
        grpParams << QParam("point_id", otInteger, point_id);
        TDateTime part_key = Qry.get().FieldAsDateTime("part_key");
        grpParams << QParam("part_key", otDate, part_key);
        std::optional<Dates::DateTime_t> opt_part_key = DateTimeToBoost(part_key);
        DB::TCachedQuery grpQry(PgOra::getROSession("ARX_PAX_GRP"), grpSQL, grpParams, STDLOG);
        grpQry.get().Execute();
        for(; not grpQry.get().Eof; grpQry.get().Next()) {
            int grp_id = grpQry.get().FieldAsInteger("grp_id");
            int point_dep = grpQry.get().FieldAsInteger("point_dep");
            int point_arv = grpQry.get().FieldAsInteger("point_arv");
            TDateTime dep_scd_in, dep_scd_out;
            TDateTime arv_scd_in, arv_scd_out;
            string dep_airp, arv_airp;
            if(getCountry(point_dep, part_key, dep_scd_in, dep_scd_out, dep_airp) != "êî") continue;
            if(getCountry(point_arv, part_key, arv_scd_in, arv_scd_out, arv_airp) != "êî") continue;
            QParams paxParams;
            paxParams << QParam("grp_id", otInteger, grp_id);
            paxParams << QParam("part_key", otDate, part_key);

            DB::TCachedQuery paxQry(PgOra::getROSession("ARX_PAX"), paxSQL, paxParams, STDLOG);
            paxQry.get().Execute();
            for(; not paxQry.get().Eof; paxQry.get().Next()) {
                PaxId_t pax_id(paxQry.get().FieldAsInteger("pax_id"));
                CheckIn::TPaxDocItem doc;
                LoadPaxDoc(opt_part_key, pax_id, doc);
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
                        << setw(10) << pax_id.get()
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
    cout << "interval time: " << tm.PrintWithMessage() << endl;
}

void collect(map<string, int> &result, TDateTime from, TDateTime to)
{
    TPerfTimer tm;
    tm.Init();
    cout << "process operating tables" << endl;

    string pointsSQL = (string)
        "select point_id "
        "from points "
        " where "
        "   ((scd_out >= :from_date and "
        "   scd_out < :to_date) or "
        "   (scd_in >= :from_date and "
        "   scd_in < :to_date)) and "
        "   airp = :airp";

    string grpSQL =
        "SELECT grp_id, point_dep, point_arv "
        "FROM pax_grp "
        "WHERE "
        "  (point_dep = :point_id or point_arv = :point_id) and "
        "  status not in ('E') ";

    string paxSQL =
        "SELECT pax_id "
        "FROM pax "
        "WHERE "
        "  grp_id = :grp_id and "
        "  refuse is null ";

    QParams QryParams;
    QryParams
        << QParam("from_date", otDate, from)
        << QParam("to_date", otDate, to)
        << QParam("airp", otString, "íãó");
    TCachedQuery Qry(pointsSQL, QryParams);
    Qry.get().Execute();
    int flights = 0;
    int pax_count = 0;
    for(; not Qry.get().Eof; Qry.get().Next(), flights++) {
        int point_id = Qry.get().FieldAsInteger("point_id");
        TDateTime part_key = NoExists;
        QParams grpParams;
        grpParams << QParam("point_id", otInteger, point_id);

        DB::TCachedQuery grpQry(PgOra::getROSession("PAX_GRP"),grpSQL, grpParams, STDLOG);
        grpQry.get().Execute();
        for(; not grpQry.get().Eof; grpQry.get().Next()) {
            int grp_id = grpQry.get().FieldAsInteger("grp_id");
            int point_dep = grpQry.get().FieldAsInteger("point_dep");
            int point_arv = grpQry.get().FieldAsInteger("point_arv");
            TDateTime dep_scd_in, dep_scd_out;
            TDateTime arv_scd_in, arv_scd_out;
            string dep_airp, arv_airp;
            if(getCountry(point_dep, part_key, dep_scd_in, dep_scd_out, dep_airp) != "êî") continue;
            if(getCountry(point_arv, part_key, arv_scd_in, arv_scd_out, arv_airp) != "êî") continue;
            QParams paxParams;
            paxParams << QParam("grp_id", otInteger, grp_id);

            DB::TCachedQuery paxQry(PgOra::getROSession("PAX"), paxSQL, paxParams, STDLOG);
            paxQry.get().Execute();
            for(; not paxQry.get().Eof; paxQry.get().Next()) {
                int pax_id = paxQry.get().FieldAsInteger("pax_id");
                CheckIn::TPaxDocItem doc;
                LoadPaxDoc(pax_id, doc);
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
    collectArx(result, from, to);

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
    return 1; // 0 - ®ß¨•≠•≠®Ô ™Æ¨¨®‚Ô‚·Ô, 1 - rollback
}

