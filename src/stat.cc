#include "stat.h"
#define NICKNAME "DENIS"
#include "test.h"
#include "oralib.h"
#include "cache.h"
#include "xml_unit.h"
#include "exceptions.h"
#include "stl_utils.h"
#include "docs.h"
#include "base_tables.h"
#include "tripinfo.h"
#include <fstream>

using namespace std;
using namespace EXCEPTIONS;
using namespace BASIC;

void GetPaxListSQL(TQuery &Qry);
void GetFltLogSQL(TQuery &Qry);
void GetSystemLogAgentSQL(TQuery &Qry);
void GetSystemLogStationSQL(TQuery &Qry);
void GetSystemLogModuleSQL(TQuery &Qry);

enum TScreenState {None,Stat,Pax,Log,DepStat,BagTagStat,PaxList,FltLog,SystemLog,PaxSrc,TlgArch};
typedef void (*TGetSQL)( TQuery &Qry );

const int depends_len = 3;
struct TCBox {
    TGetSQL GetSQL;
    string cbox, qry;
    string depends[depends_len];
};

const int cboxes_len = 5;
struct TCategory {
    TScreenState scr;
    TCBox cboxes[cboxes_len];
};

TCategory Category[] = {
    {},
    {},
    {},
    {},
    {
        DepStat,
        {
            {
                NULL,
                "Flt",

                "SELECT "
                "    points.point_id, "
                "    points.airp, "
                "    points.airline, "
                "    points.flt_no, "
                "    points.suffix, "
                "    points.scd_out, "
                "    NVL(points.act_out,NVL(points.est_out,points.scd_out)) AS real_out "
                "FROM "
                "    points "
                "WHERE "
                "    points.pr_del >= 0 and "
                "    points.scd_out >= :FirstDate AND points.scd_out < :LastDate "
                "union "
                "SELECT "
                "    arx_points.point_id, "
                "    arx_points.airp, "
                "    arx_points.airline, "
                "    arx_points.flt_no, "
                "    arx_points.suffix, "
                "    arx_points.scd_out, "
                "    NVL(arx_points.act_out,NVL(arx_points.est_out,arx_points.scd_out)) AS real_out "
                "FROM "
                "    arx_points "
                "WHERE "
                "    arx_points.pr_del >= 0 and "
                "    arx_points.scd_out >= :FirstDate AND arx_points.scd_out < :LastDate and "
                "    arx_points.part_key >= :FirstDate "
                "ORDER BY "
                "    real_out DESC ",

                {"Dest", "Awk", "Class"}
            },
            {
                NULL,
                "Awk",

                "select company from "
                "    astra.trips, astra.place, astra.pax_grp "
                "where "
                "    trips.scd >= :FirstDate AND trips.scd < :LastDate AND "
                "    trips.trip_id=place.trip_id AND "
                "    trips.trip_id = pax_grp.point_id(+) and "
                "    (:class is null or pax_grp.class = :class) and "
                "    place.num>0 AND "
                "    (:dest IS NULL OR place.city= :dest) and "
                "    (:flt IS NULL OR trips.trip= :flt) "
                "union "
                "select company from "
                "    arx.trips, arx.place, arx.pax_grp "
                "where "
                "    trips.part_key >= :FirstDate AND trips.part_key < :LastDate AND "
                "    trips.trip_id=place.trip_id AND "
                "    trips.trip_id = pax_grp.point_id(+) and "
                "    (:class is null or pax_grp.class = :class) and "
                "    place.num>0 AND "
                "    (:dest IS NULL OR place.city= :dest) and "
                "    (:flt IS NULL OR trips.trip= :flt) ",

                {"Flt",    "Dest", "Class"}
            },
            {
                NULL,
                "Dest",

                "SELECT place.city AS dest "
                "FROM astra.trips,astra.place,astra.pax_grp "
                "WHERE  "
                "      trips.trip_id=place.trip_id AND "
                "      trips.trip_id = pax_grp.point_id(+) and "
                "      trips.scd >= :FirstDate AND trips.scd < :LastDate AND "
                "      (:awk IS NULL OR trips.company = :awk) AND "
                "      place.num>0 AND "
                "      (:flt IS NULL OR trips.trip= :flt) and "
                "      (:class is null or pax_grp.class = :class) "
                "UNION "
                "SELECT place.city AS dest "
                "FROM arx.trips,arx.place,arx.pax_grp "
                "WHERE  "
                "      trips.trip_id=place.trip_id AND "
                "      trips.trip_id = pax_grp.point_id(+) and "
                "      trips.part_key >= :FirstDate AND trips.part_key < :LastDate AND "
                "      (:awk IS NULL OR trips.company = :awk) AND "
                "      place.num>0 AND "
                "      (:flt IS NULL OR trips.trip= :flt) and "
                "      (:class is null or pax_grp.class = :class) "
                "ORDER BY dest  ",

                {"Awk",    "Flt",  "Class"}
            },
            {
                NULL,
                "Class",

                "select "
                "    distinct pax_grp.class "
                "from "
                "    astra.trips, astra.pax_grp, astra.place "
                "WHERE "
                "    trips.trip_id=place.trip_id AND "
                "    trips.trip_id=pax_grp.point_id and "
                "    trips.scd>= :FirstDate AND "
                "    trips.scd< :LastDate AND "
                "    (:awk is null or trips.company = :awk) and "
                "    (:flt IS NULL OR trips.trip= :flt) and "
                "    place.num>0 AND "
                "    (:dest IS NULL OR place.city= :dest) "
                "union "
                "select "
                "    distinct pax_grp.class "
                "from "
                "    arx.trips, arx.pax_grp, arx.place "
                "WHERE "
                "    trips.trip_id=pax_grp.point_id AND "
                "    trips.trip_id=place.trip_id AND "
                "    trips.trip_id=pax_grp.point_id and "
                "    trips.scd>= :FirstDate AND "
                "    trips.scd< :LastDate AND "
                "    (:awk is null or trips.company = :awk) and "
                "    (:flt IS NULL OR trips.trip= :flt) and "
                "    place.num>0 AND "
                "    (:dest IS NULL OR place.city= :dest) ",

                {"Awk",    "Flt",  "Dest"}
            },
            {
                NULL,
                "Rem",

                "select "
                "    distinct pax_rem.rem_code "
                "from "
                "    astra.trips, astra.pax_grp, astra.pax, astra.pax_rem, astra.remark "
                "WHERE "
                "    trips.scd>= :FirstDate AND "
                "    trips.scd< :LastDate AND "
                "    trips.trip_id=pax_grp.point_id AND "
                "    pax_grp.grp_id = pax.grp_id and "
                "    pax.pax_id = pax_rem.pax_id and "
                "    pax_rem.rem_code is not null and "
                "    pax_rem.rem_code = remark.cod "
                "union "
                "select "
                "    distinct pax_rem.rem_code "
                "from "
                "    arx.trips, arx.pax_grp, arx.pax, arx.pax_rem, astra.remark "
                "WHERE "
                "    trips.trip_id=pax_grp.point_id AND "
                "    pax_grp.grp_id = pax.grp_id and "
                "    pax.pax_id = pax_rem.pax_id and "
                "    pax_rem.rem_code is not null and "
                "    pax_rem.rem_code = remark.cod and "
                "    pax_rem.part_key>= :FirstDate AND "
                "    pax_rem.part_key< :LastDate "
            },
        }
    },
    {
        BagTagStat,
        {
            {
                NULL,
                "Awk",

                "select company from "
                "    astra.trips "
                "where "
                "    trips.scd >= :FirstDate AND trips.scd < :LastDate "
                "union "
                "select company from "
                "    arx.trips "
                "where "
                "    trips.part_key >= :FirstDate AND trips.part_key < :LastDate "
            }
        }
    },
    {
        PaxList,
        {
            {
                GetPaxListSQL,
                "Flt",

                "SELECT "
                "    points.point_id, "
                "    points.airp, "
                "    points.airline, "
                "    points.flt_no, "
                "    points.suffix, "
                "    points.scd_out, "
                "    NVL(points.act_out,NVL(points.est_out,points.scd_out)) AS real_out "
                "FROM "
                "    points "
                "WHERE "
                "    points.pr_del >= 0 and "
                "    points.scd_out >= :FirstDate AND points.scd_out < :LastDate "
                "union "
                "SELECT "
                "    arx_points.point_id, "
                "    arx_points.airp, "
                "    arx_points.airline, "
                "    arx_points.flt_no, "
                "    arx_points.suffix, "
                "    arx_points.scd_out, "
                "    NVL(arx_points.act_out,NVL(arx_points.est_out,arx_points.scd_out)) AS real_out "
                "FROM "
                "    arx_points "
                "WHERE "
                "    arx_points.pr_del >= 0 and "
                "    arx_points.scd_out >= :FirstDate AND arx_points.scd_out < :LastDate and "
                "    arx_points.part_key >= :FirstDate "
                "ORDER BY "
                "    real_out DESC "
            }
        }
    },
    {
        FltLog,
        {
            {
                GetFltLogSQL,
                "Flt",


                "SELECT "
                "    points.point_id, "
                "    points.airp, "
                "    points.airline, "
                "    points.flt_no, "
                "    points.suffix, "
                "    points.scd_out, "
                "    NVL(points.act_out,NVL(points.est_out,points.scd_out)) AS real_out "
                "FROM "
                "    points, "
                "    events "
                "WHERE "
                "    events.type in ( "
                "    :evtFlt, "
                "    :evtGraph, "
                "    :evtTlg, "
                "    :evtPax, "
                "    :evtPay "
                "    ) and "
                "    events.id1 = points.point_id and "
                "    points.pr_del >= 0 and "
                "    points.scd_out >= :FirstDate AND points.scd_out < :LastDate "
                "union "
                "SELECT "
                "    arx_points.point_id, "
                "    arx_points.airp, "
                "    arx_points.airline, "
                "    arx_points.flt_no, "
                "    arx_points.suffix, "
                "    arx_points.scd_out, "
                "    NVL(arx_points.act_out,NVL(arx_points.est_out,arx_points.scd_out)) AS real_out "
                "FROM "
                "    arx_points, "
                "    arx_events "
                "WHERE "
                "    arx_events.part_key >= :FirstDate and "
                "    arx_events.type in ( "
                "    :evtFlt, "
                "    :evtGraph, "
                "    :evtTlg, "
                "    :evtPax, "
                "    :evtPay "
                "    ) and "
                "    arx_events.id1 = arx_points.point_id and "
                "    arx_points.pr_del >= 0 and "
                "    arx_points.scd_out >= :FirstDate AND arx_points.scd_out < :LastDate and "
                "    arx_points.part_key >= :FirstDate "
                "ORDER BY "
                "    real_out DESC "
            }
        }
    },
    {
        SystemLog,
        {
            {
                GetSystemLogAgentSQL,
                "Agent",

                "select '���⥬�' agent from dual where "
                "  (:station is null or :station = '���⥬�') and "
                "  (:module is null or :module = '���⥬�') "
                "union "
                "select ev_user agent from events, screen "
                "where "
                "    time >= :FirstDate and "
                "    time < :LastDate and "
                "    (:station is null or station = :station) and "
                "    events.screen = screen.exe(+) and "
                "    (:module is null or nvl(screen.name, events.screen) = :module) "
                "union "
                "select ev_user agent from arx_events, screen "
                "where "
                "    part_key >= :FirstDate and "
                "    time >= :FirstDate and "
                "    time < :LastDate and "
                "    (:station is null or station = :station) and "
                "    arx_events.screen = screen.exe(+) and "
                "    (:module is null or nvl(screen.name, arx_events.screen) = :module) "
                "order by "
                "    agent ",

                {"Station", "Module"}
            },
            {
                GetSystemLogStationSQL,
                "Station",

                "select '���⥬�' station from dual where "
                "  (:agent is null or :agent = '���⥬�') and "
                "  (:module is null or :module = '���⥬�') "
                "union "
                "select station from events, screen "
                "where "
                "    time >= :FirstDate and "
                "    time < :LastDate and "
                "    station is not null and "
                "    (:agent is null or ev_user = :agent) and "
                "    events.screen = screen.exe(+) and "
                "    (:module is null or nvl(screen.name, events.screen) = :module) "
                "union "
                "select station from arx_events, screen "
                "where "
                "    part_key >= :FirstDate and "
                "    time >= :FirstDate and "
                "    time < :LastDate and "
                "    station is not null and "
                "    (:agent is null or ev_user = :agent) and "
                "    arx_events.screen = screen.exe(+) and "
                "    (:module is null or nvl(screen.name, arx_events.screen) = :module) "
                "order by "
                "    station ",

                {"Agent", "Module"}
            },
            {
                GetSystemLogModuleSQL,
                "Module",

                "select '���⥬�' module from dual where "
                "  (:agent is null or :agent = '���⥬�') and "
                "  (:station is null or :station = '���⥬�') "
                "union "
                "select nvl(screen.name, events.screen) module from events, screen where "
                "    events.time >= :FirstDate and "
                "    events.time < :LastDate and "
                "    (:station is null or station = :station) and "
                "    (:agent is null or ev_user = :agent) and "
                "    events.screen is not null and "
                "    events.screen = screen.exe(+) "
                "union "
                "select nvl(screen.name, arx_events.screen) module from arx_events, screen where "
                "    part_key >= :FirstDate and "
                "    time >= :FirstDate and "
                "    time < :LastDate and "
                "    (:station is null or station = :station) and "
                "    (:agent is null or ev_user = :agent) and "
                "    arx_events.screen is not null and "
                "    arx_events.screen = screen.exe(+) "
                "order by "
                "    module ",

                {"Agent", "Station"}
            }
        }
    },
    {
        PaxSrc,
        {
            {
                NULL,
                "Flt",

                "SELECT trips.trip "
                "FROM astra.trips,astra.place "
                "WHERE trips.trip_id=place.trip_id AND "
                "      trips.scd >= :FirstDate AND trips.scd < :LastDate AND "
                "      (:awk IS NULL OR trips.company = :awk) AND "
                "      place.num>0 AND "
                "      (:dest IS NULL OR place.city= :dest) "
                "UNION "
                "SELECT trips.trip "
                "FROM arx.trips,arx.place "
                "WHERE "
                "      trips.part_key=place.part_key AND trips.trip_id=place.trip_id AND "
                "      place.part_key >= :FirstDate AND place.part_key < :LastDate AND "
                "      (:awk IS NULL OR trips.company = :awk) AND "
                "      place.num>0 AND "
                "      (:dest IS NULL OR place.city= :dest) "
                "ORDER BY trip  ",

                {"Dest",   "Awk"}
            },
            {
                NULL,
                "Awk",

                "select company from "
                "    astra.trips, astra.place "
                "where "
                "    trips.scd >= :FirstDate AND trips.scd < :LastDate AND "
                "    trips.trip_id=place.trip_id AND "
                "    place.num>0 AND "
                "    (:dest IS NULL OR place.city= :dest) and "
                "    (:flt IS NULL OR trips.trip= :flt) "
                "union "
                "select company from "
                "    arx.trips, arx.place "
                "where "
                "    trips.part_key >= :FirstDate AND trips.part_key < :LastDate AND "
                "    trips.trip_id=place.trip_id AND "
                "    place.num>0 AND "
                "    (:dest IS NULL OR place.city= :dest) and "
                "    (:flt IS NULL OR trips.trip= :flt) ",

                {"Flt",    "Dest"}
            },
            {
                NULL,
                "Dest",

                "SELECT place.city AS dest "
                "FROM astra.trips,astra.place "
                "WHERE  "
                "      trips.trip_id=place.trip_id AND "
                "      trips.scd >= :FirstDate AND trips.scd < :LastDate AND "
                "      (:awk IS NULL OR trips.company = :awk) AND "
                "      place.num>0 AND "
                "      (:flt IS NULL OR trips.trip= :flt) "
                "UNION "
                "SELECT place.city AS dest "
                "FROM arx.trips,arx.place "
                "WHERE  "
                "      trips.trip_id=place.trip_id AND "
                "      trips.part_key >= :FirstDate AND trips.part_key < :LastDate AND "
                "      (:awk IS NULL OR trips.company = :awk) AND "
                "      place.num>0 AND "
                "      (:flt IS NULL OR trips.trip= :flt) "
                "ORDER BY dest  ",

                {"Awk",    "Flt"}
            }
        }
    },
    {}
};

const int CategorySize = sizeof(Category)/sizeof(Category[0]);

void GetFltLogSQL(TQuery &Qry)
{
    TReqInfo &info = *(TReqInfo::Instance());
    string res =
        "SELECT "
        "    points.point_id, "
        "    points.airp, "
        "    points.airline, "
        "    points.flt_no, "
        "    nvl(points.suffix, ' ') suffix, "
        "    points.scd_out, "
        "    trunc(NVL(points.act_out,NVL(points.est_out,points.scd_out))) AS real_out, "
        "    move_id, "
        "    point_num "
        "FROM "
        "    points, "
        "    events ";
    if (!info.user.access.airlines.empty())
        res += ",aro_airlines ";
    if (!info.user.access.airps.empty())
        res += ",aro_airps ";
    res +=
        "WHERE "
        "    events.type in ( "
        "    :evtFlt, "
        "    :evtGraph, "
        "    :evtTlg, "
        "    :evtPax, "
        "    :evtPay "
        "    ) and "
        "    events.id1 = points.point_id and "
        "    points.pr_del >= 0 and "
        "    points.scd_out >= :FirstDate AND points.scd_out < :LastDate ";
    if (!info.user.access.airlines.empty())
        res += "AND aro_airlines.airline=points.airline AND aro_airlines.aro_id=:user_id ";
    if (!info.user.access.airps.empty())
        res += "AND aro_airps.airp=points.airp AND aro_airps.aro_id=:user_id ";
    res +=
        "union "
        "SELECT "
        "    arx_points.point_id, "
        "    arx_points.airp, "
        "    arx_points.airline, "
        "    arx_points.flt_no, "
        "    nvl(arx_points.suffix, ' ') suffix, "
        "    arx_points.scd_out, "
        "    trunc(NVL(arx_points.act_out,NVL(arx_points.est_out,arx_points.scd_out))) AS real_out, "
        "    move_id, "
        "    point_num "
        "FROM "
        "    arx_points, "
        "    arx_events ";
    if (!info.user.access.airlines.empty())
        res += ",aro_airlines ";
    if (!info.user.access.airps.empty())
        res += ",aro_airps ";
    res +=
        "WHERE "
        "    arx_events.part_key >= :FirstDate and "
        "    arx_events.type in ( "
        "    :evtFlt, "
        "    :evtGraph, "
        "    :evtTlg, "
        "    :evtPax, "
        "    :evtPay "
        "    ) and "
        "    arx_events.id1 = arx_points.point_id and "
        "    arx_points.pr_del >= 0 and "
        "    arx_points.scd_out >= :FirstDate AND arx_points.scd_out < :LastDate and "
        "    arx_points.part_key >= :FirstDate ";
    if (!info.user.access.airlines.empty())
        res += "AND aro_airlines.airline=arx_points.airline AND aro_airlines.aro_id=:user_id ";
    if (!info.user.access.airps.empty())
        res += "AND aro_airps.airp=arx_points.airp AND aro_airps.aro_id=:user_id ";
    res +=
        "ORDER BY "
        "   real_out DESC, "
        "   flt_no, "
        "   airline, "
        "   suffix, "
        "   move_id, "
        "   point_num ";
    if (!info.user.access.airlines.empty() || !info.user.access.airps.empty())
        Qry.CreateVariable( "user_id", otInteger, info.user.user_id );
    Qry.SQLText = res;
}

void GetPaxListSQL(TQuery &Qry)
{
    TReqInfo &info = *(TReqInfo::Instance());
    tst();
    string res =
        "SELECT "
        "    points.point_id, "
        "    points.airp, "
        "    points.airline, "
        "    points.flt_no, "
        "    nvl(points.suffix, ' ') suffix, "
        "    points.scd_out, "
        "    NVL(points.act_out,NVL(points.est_out,points.scd_out)) AS real_out, "
        "    move_id, "
        "    point_num "
        "FROM "
        "    points ";
    if (!info.user.access.airlines.empty())
        res += ",aro_airlines ";
    if (!info.user.access.airps.empty())
        res += ",aro_airps ";
    res +=
        "WHERE "
        "    points.pr_del >= 0 and "
        "    points.pr_reg <> 0 and "
        "    points.scd_out >= :FirstDate AND points.scd_out < :LastDate ";
    if (!info.user.access.airlines.empty())
        res += "AND aro_airlines.airline=points.airline AND aro_airlines.aro_id=:user_id ";
    if (!info.user.access.airps.empty())
        res += "AND aro_airps.airp=points.airp AND aro_airps.aro_id=:user_id ";
    res +=
        "union "
        "SELECT "
        "    arx_points.point_id, "
        "    arx_points.airp, "
        "    arx_points.airline, "
        "    arx_points.flt_no, "
        "    nvl(arx_points.suffix, ' ') suffix, "
        "    arx_points.scd_out, "
        "    NVL(arx_points.act_out,NVL(arx_points.est_out,arx_points.scd_out)) AS real_out, "
        "    move_id, "
        "    point_num "
        "FROM "
        "    arx_points ";
    if (!info.user.access.airlines.empty())
        res += ",aro_airlines ";
    if (!info.user.access.airps.empty())
        res += ",aro_airps ";
    res +=
        "WHERE "
        "    arx_points.pr_del >= 0 and "
        "    arx_points.pr_reg <> 0 and "
        "    arx_points.scd_out >= :FirstDate AND arx_points.scd_out < :LastDate and "
        "    arx_points.part_key >= :FirstDate ";
    if (!info.user.access.airlines.empty())
        res += "AND aro_airlines.airline=arx_points.airline AND aro_airlines.aro_id=:user_id ";
    if (!info.user.access.airps.empty())
        res += "AND aro_airps.airp=arx_points.airp AND aro_airps.aro_id=:user_id ";
    res +=
        "ORDER BY "
        "   flt_no, "
        "   airline, "
        "   suffix, "
        "   move_id, "
        "   point_num ";
    if (!info.user.access.airlines.empty() || !info.user.access.airps.empty())
        Qry.CreateVariable( "user_id", otInteger, info.user.user_id );
    Qry.SQLText = res;
}

void GetSystemLogAgentSQL(TQuery &Qry)
{
    TReqInfo &info = *(TReqInfo::Instance());
    string SQLText =
        "select '���⥬�' agent from dual where "
        "  (:station is null or :station = '���⥬�') and "
        "  (:module is null or :module = '���⥬�') "
        "union "
        "select ev_user agent from "
        "   events, "
        "   screen ";
    if (!info.user.access.airlines.empty())
        SQLText +=
            ", points p1"
            ",aro_airlines ";
    if (!info.user.access.airps.empty())
        SQLText +=
            ", points p2"
            ",aro_airps ";
    SQLText +=
        "where "
        "    time >= :FirstDate and "
        "    time < :LastDate and "
        "    (:station is null or station = :station) and "
        "    events.screen = screen.exe(+) and "
        "    (:module is null or nvl(screen.name, events.screen) = :module) ";
    if (!info.user.access.airlines.empty())
        SQLText +=
            "and p1.point_id = events.id1 "
            "AND aro_airlines.airline=p1.airline AND aro_airlines.aro_id=:user_id ";
    if (!info.user.access.airps.empty())
        SQLText +=
            "and p2.point_id = events.id1 "
            "AND aro_airps.airp=p2.airp AND aro_airps.aro_id=:user_id ";
    SQLText +=
        "union "
        "select ev_user agent from "
        "   arx_events, "
        "   screen ";
    if (!info.user.access.airlines.empty())
        SQLText +=
            ", arx_points a1"
            ",aro_airlines ";
    if (!info.user.access.airps.empty())
        SQLText +=
            ", arx_points a2"
            ",aro_airps ";
    SQLText +=
        "where "
        "    arx_events.part_key >= :FirstDate and "
        "    time >= :FirstDate and "
        "    time < :LastDate and "
        "    (:station is null or station = :station) and "
        "    arx_events.screen = screen.exe(+) and "
        "    (:module is null or nvl(screen.name, arx_events.screen) = :module) ";
    if (!info.user.access.airlines.empty())
        SQLText +=
            "and a1.point_id = arx_events.id1 "
            "AND aro_airlines.airline=a1.airline AND aro_airlines.aro_id=:user_id ";
    if (!info.user.access.airps.empty())
        SQLText +=
            "and a2.point_id = arx_events.id1 "
            "AND aro_airps.airp=a2.airp AND aro_airps.aro_id=:user_id ";
    SQLText +=
        "order by "
        "    agent ";
    if (!info.user.access.airlines.empty() || !info.user.access.airps.empty())
        Qry.CreateVariable( "user_id", otInteger, info.user.user_id );
    Qry.SQLText = SQLText;
}

void GetSystemLogStationSQL(TQuery &Qry)
{
    TReqInfo &info = *(TReqInfo::Instance());
    string SQLText =
        "select '���⥬�' station from dual where "
        "  (:agent is null or :agent = '���⥬�') and "
        "  (:module is null or :module = '���⥬�') "
        "union "
        "select station from events, screen ";
    if (!info.user.access.airlines.empty())
        SQLText +=
            ", points p1"
            ",aro_airlines ";
    if (!info.user.access.airps.empty())
        SQLText +=
            ", points p2"
            ",aro_airps ";
    SQLText +=
        "where "
        "    time >= :FirstDate and "
        "    time < :LastDate and "
//        "    station is not null and "
        "    (:agent is null or ev_user = :agent) and "
        "    events.screen = screen.exe(+) and "
        "    (:module is null or nvl(screen.name, events.screen) = :module) ";
    if (!info.user.access.airlines.empty())
        SQLText +=
            "and p1.point_id = events.id1 "
            "AND aro_airlines.airline=p1.airline AND aro_airlines.aro_id=:user_id ";
    if (!info.user.access.airps.empty())
        SQLText +=
            "and p2.point_id = events.id1 "
            "AND aro_airps.airp=p2.airp AND aro_airps.aro_id=:user_id ";
    SQLText +=
        "union "
        "select station from arx_events, screen ";
    if (!info.user.access.airlines.empty())
        SQLText +=
            ", arx_points a1"
            ",aro_airlines ";
    if (!info.user.access.airps.empty())
        SQLText +=
            ", arx_points a2"
            ",aro_airps ";
    SQLText +=
        "where "
        "    arx_events.part_key >= :FirstDate and "
        "    time >= :FirstDate and "
        "    time < :LastDate and "
//        "    station is not null and "
        "    (:agent is null or ev_user = :agent) and "
        "    arx_events.screen = screen.exe(+) and "
        "    (:module is null or nvl(screen.name, arx_events.screen) = :module) ";
    if (!info.user.access.airlines.empty())
        SQLText +=
            "and a1.point_id = arx_events.id1 "
            "AND aro_airlines.airline=a1.airline AND aro_airlines.aro_id=:user_id ";
    if (!info.user.access.airps.empty())
        SQLText +=
            "and a2.point_id = arx_events.id1 "
            "AND aro_airps.airp=a2.airp AND aro_airps.aro_id=:user_id ";
    SQLText +=
        "order by "
        "    station ";
    if (!info.user.access.airlines.empty() || !info.user.access.airps.empty())
        Qry.CreateVariable( "user_id", otInteger, info.user.user_id );
    Qry.SQLText = SQLText;
}

void GetSystemLogModuleSQL(TQuery &Qry)
{
    TReqInfo &info = *(TReqInfo::Instance());
    string SQLText =
        "select '���⥬�' module from dual where "
        "  (:agent is null or :agent = '���⥬�') and "
        "  (:station is null or :station = '���⥬�') "
        "union "
        "select nvl(screen.name, events.screen) module from events, screen ";
    if (!info.user.access.airlines.empty())
        SQLText +=
            ", points p1"
            ",aro_airlines ";
    if (!info.user.access.airps.empty())
        SQLText +=
            ", points p2"
            ",aro_airps ";
    SQLText +=
        "where "
        "    events.time >= :FirstDate and "
        "    events.time < :LastDate and "
        "    (:station is null or station = :station) and "
        "    (:agent is null or ev_user = :agent) and "
//        "    events.screen is not null and "
        "    events.screen = screen.exe(+) ";
    if (!info.user.access.airlines.empty())
        SQLText +=
            "and p1.point_id = events.id1 "
            "AND aro_airlines.airline=p1.airline AND aro_airlines.aro_id=:user_id ";
    if (!info.user.access.airps.empty())
        SQLText +=
            "and p2.point_id = events.id1 "
            "AND aro_airps.airp=p2.airp AND aro_airps.aro_id=:user_id ";
    SQLText +=
        "union "
        "select nvl(screen.name, arx_events.screen) module from arx_events, screen ";
    if (!info.user.access.airlines.empty())
        SQLText +=
            ", arx_points a1"
            ",aro_airlines ";
    if (!info.user.access.airps.empty())
        SQLText +=
            ", arx_points a2"
            ",aro_airps ";
    SQLText +=
        "where "
        "    arx_events.part_key >= :FirstDate and "
        "    time >= :FirstDate and "
        "    time < :LastDate and "
        "    (:station is null or station = :station) and "
        "    (:agent is null or ev_user = :agent) and "
//        "    arx_events.screen is not null and "
        "    arx_events.screen = screen.exe(+) ";
    if (!info.user.access.airlines.empty())
        SQLText +=
            "and a1.point_id = arx_events.id1 "
            "AND aro_airlines.airline=a1.airline AND aro_airlines.aro_id=:user_id ";
    if (!info.user.access.airps.empty())
        SQLText +=
            "and a2.point_id = arx_events.id1 "
            "AND aro_airps.airp=a2.airp AND aro_airps.aro_id=:user_id ";
    SQLText +=
        "order by "
        "    module ";
    if (!info.user.access.airlines.empty() || !info.user.access.airps.empty())
        Qry.CreateVariable( "user_id", otInteger, info.user.user_id );
    Qry.SQLText = SQLText;
}


void StatInterface::CommonCBoxDropDown(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    string cbox = NodeAsString("cbox", reqNode);

    TScreenState scr = TScreenState(NodeAsInteger("scr", reqNode));
    TCategory *Ctg = &Category[scr];
    int i = 0;
    for(; i < cboxes_len; i++)
        if(Ctg->cboxes[i].cbox + "CBox" == cbox) break;
    if(i == cboxes_len)throw Exception((string)"CommonCBoxDropDown: data not found for " + cbox);
    TCBox *cbox_data = &Ctg->cboxes[i];
    TQuery Qry(&OraSession);
    if(!cbox_data->GetSQL) throw Exception("GetSQL is NULL");
    cbox_data->GetSQL(Qry);
    ProgTrace(TRACE5, "%s", Qry.SQLText.SQLText());
    TReqInfo *reqInfo = TReqInfo::Instance();
    Qry.CreateVariable("FirstDate", otDate, ClientToUTC(NodeAsDateTime("FirstDate", reqNode), reqInfo->desk.tz_region));
    Qry.CreateVariable("LastDate", otDate, ClientToUTC(NodeAsDateTime("LastDate", reqNode), reqInfo->desk.tz_region));
    if(scr == FltLog) {
        Qry.CreateVariable("evtFlt", otString, EncodeEventType(evtFlt));
        Qry.CreateVariable("evtGraph", otString, EncodeEventType(evtGraph));
        Qry.CreateVariable("evtTlg", otString, EncodeEventType(evtTlg));
        Qry.CreateVariable("evtPax", otString, EncodeEventType(evtPax));
        Qry.CreateVariable("evtPay", otString, EncodeEventType(evtPay));
    }
    xmlNodePtr dependNode = GetNode("depends", reqNode);
    if(dependNode) {
        dependNode = dependNode->children;
        while(dependNode) {
            Qry.CreateVariable((char *)dependNode->name, otString, NodeAsString(dependNode));
            dependNode = dependNode->next;
        }
    }
    try {
        Qry.Execute();
    } catch (EOracleError E) {
        if(E.Code == 376)
            throw UserException("� �������� ��������� ��� ���� �� 䠩��� �� �⪫�祭. ������� � ������������");
        else
            throw;
    }
    xmlNodePtr cboxNode = NewTextChild(resNode, "cbox");
    if(cbox_data->cbox == "Flt") {
        //�᫨ �� �������� � ���⠬ �������稩 ��� - ���⮩ ᯨ᮪ ३ᮢ
        if (reqInfo->user.user_type==utAirport && reqInfo->user.access.airps.empty() ||
                reqInfo->user.user_type==utAirline && reqInfo->user.access.airlines.empty() ) return;
        while(!Qry.Eof) {
            TTripInfo info(Qry);
            string trip_name;
            try
            {
                trip_name = GetTripName(info,false,true);
            }
            catch(UserException &E)
            {
                showErrorMessage((string)E.what()+". ������� ३�� �� �⮡ࠦ�����");
                Qry.Next();
                continue;
            };
            xmlNodePtr fNode = NewTextChild(cboxNode, "f");
            NewTextChild(fNode, "key", Qry.FieldAsInteger("point_id"));
            NewTextChild( fNode, "value", trip_name);

            Qry.Next();
        }
    } else
        while(!Qry.Eof) {
            xmlNodePtr fNode = NewTextChild(cboxNode, "f");
            NewTextChild(fNode, "key", 0);
            NewTextChild(fNode, "value", Qry.FieldAsString(0));
            Qry.Next();
        }
}

void StatInterface::PaxLog(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    get_report_form("ArxPaxLog", resNode);
    STAT::set_variables(resNode);
    TQuery Qry(&OraSession);
    string tag = (char *)reqNode->name;
    char *qry = NULL;
    TReqInfo *reqInfo = TReqInfo::Instance();
    xmlNodePtr variablesNode = GetNode("form_data/variables", resNode);
    xmlNodePtr reportTitleNode = NewTextChild(variablesNode, "report_title");
    if(tag == "LogRun") {
        NodeSetContent(reportTitleNode, "����樨 �� ���ᠦ���");
        qry =
            "SELECT msg, time, id1 AS point_id, null as screen, id2 AS reg_no, id3 AS grp_id, "
            "       ev_user, station, ev_order "
            "FROM events "
            "WHERE type IN (:evtPax,:evtPay) AND "
            "      id1=:point_id AND "
            "      (id2 IS NULL OR id2=:reg_no) AND "
            "      (id3 IS NULL OR id3=:grp_id) "
            "UNION "
            "SELECT msg, time, id1 AS point_id, null as screen, id2 AS reg_no, id3 AS grp_id, "
            "       ev_user, station, ev_order "
            "FROM arx_events "
            "WHERE part_key >= :part_key AND "
            "      type IN (:evtPax,:evtPay) AND "
            "      id1=:point_id AND "
            "      (id2 IS NULL OR id2=:reg_no) AND "
            "      (id3 IS NULL OR id3=:grp_id) ";
        Qry.CreateVariable("evtPax",otString,EncodeEventType(ASTRA::evtPax));
        Qry.CreateVariable("evtPay",otString,EncodeEventType(ASTRA::evtPay));
        Qry.CreateVariable("part_key", otDate, ClientToUTC(NodeAsDateTime("FirstDate", reqNode), reqInfo->desk.tz_region));
        Qry.CreateVariable("point_id", otInteger, NodeAsInteger("point_id", reqNode));
        Qry.CreateVariable("reg_no", otInteger, NodeAsInteger("reg_no", reqNode));
        Qry.CreateVariable("grp_id", otInteger, NodeAsInteger("grp_id", reqNode));
    } else if(tag == "FltLogRun") {
        NodeSetContent(reportTitleNode, "��ୠ� ����権 ३�");
        qry =
            "SELECT msg, time, id1 AS point_id, "
            "       nvl(screen.name, events.screen) screen, "
            "       DECODE(type,:evtPax,id2,:evtPay,id2,NULL) AS reg_no, "
            "       DECODE(type,:evtPax,id3,:evtPay,id3,NULL) AS grp_id, "
            "       ev_user, station, ev_order "
            "FROM events, screen, "
            "     (SELECT point_id FROM points "
            "      WHERE points.scd_out>= :FirstDate AND points.scd_out < :LastDate AND points.point_id= :point_id "
            "      ) points "
            "WHERE events.type IN (:evtFlt,:evtGraph,:evtPax,:evtPay,:evtTlg) AND "
            "      events.screen = screen.exe(+) and "
            "      events.id1=points.point_id "
            "UNION "
            "SELECT msg, time, id1 AS point_id, "
            "       nvl(screen.name, arx_events.screen) screen, "
            "       DECODE(type,:evtPax,id2,:evtPay,id2,NULL) AS reg_no, "
            "       DECODE(type,:evtPax,id3,:evtPay,id3,NULL) AS grp_id, "
            "       ev_user, station, ev_order "
            "FROM arx_events, screen, "
            "     (SELECT part_key,point_id FROM arx_points "
            "      WHERE arx_points.part_key >= :FirstDate AND arx_points.point_id = :point_id and "
            "            arx_points.scd_out>= :FirstDate AND arx_points.scd_out < :LastDate "
            "      ) arx_points "
            "WHERE "
            "      arx_events.part_key >= :FirstDate and "
            "      arx_events.part_key=arx_points.part_key AND "
            "      arx_events.screen = screen.exe(+) and "
            "      arx_events.type IN (:evtFlt,:evtGraph,:evtPax,:evtPay,:evtTlg) AND "
            "      arx_events.id1=arx_points.point_id ";

        Qry.CreateVariable("evtFlt",otString,EncodeEventType(ASTRA::evtFlt));
        Qry.CreateVariable("evtGraph",otString,EncodeEventType(ASTRA::evtGraph));
        Qry.CreateVariable("evtPax",otString,EncodeEventType(ASTRA::evtPax));
        Qry.CreateVariable("evtPay",otString,EncodeEventType(ASTRA::evtPay));
        Qry.CreateVariable("evtTlg",otString,EncodeEventType(ASTRA::evtTlg));
        Qry.CreateVariable("FirstDate", otDate, ClientToUTC(NodeAsDateTime("FirstDate", reqNode), reqInfo->desk.tz_region));
        Qry.CreateVariable("LastDate", otDate, ClientToUTC(NodeAsDateTime("LastDate", reqNode), reqInfo->desk.tz_region));
        Qry.CreateVariable("point_id", otInteger, NodeAsInteger("point_id", reqNode));
    } else if(tag == "SystemLogRun") {
        NodeSetContent(reportTitleNode, "����樨 � ��⥬�");
        qry =
            "SELECT msg, time, id1 AS point_id, "
            "  nvl(screen.name, events.screen) screen, "
            "  DECODE(type,:evtPay,id2,:evtPax,id2,id2,NULL) AS reg_no, "
            "  DECODE(type,:evtPay,id2,:evtPax,id3,id3,NULL) AS grp_id, "
            "  ev_user, station, ev_order "
            "FROM events, screen "
            "WHERE "
            "  events.time >= :FirstDate and "
            "  events.time < :LastDate and "
            "  events.screen = screen.exe(+) and "
            "  (:agent is null or nvl(ev_user, '���⥬�') = :agent) and "
            "  (:module is null or nvl(screen.name, '���⥬�') = :module) and "
            "  (:station is null or nvl(station, '���⥬�') = :station) and "
            "  events.type IN ( "
            "    :evtFlt, "
            "    :evtPax, "
            "    :evtPay, "
            "    :evtGraph, "
            "    :evtTlg, "
            "    :evtComp, "
            "    :evtAccess, "
            "    :evtSystem, "
            "    :evtCodif, "
            "    :evtPeriod "
            "  ) "
            "UNION "
            "SELECT msg, time, id1 AS point_id, "
            "  nvl(screen.name, arx_events.screen) screen, "
            "  DECODE(type,:evtPax,id2,NULL) AS reg_no, "
            "  DECODE(type,:evtPax,id3,NULL) AS grp_id, "
            "  ev_user, station, ev_order "
            "FROM arx_events, screen "
            "WHERE "
            "  arx_events.part_key >= :FirstDate and "
            "  arx_events.part_key < :LastDate and "
            "  arx_events.screen = screen.exe(+) and "
            "  (:agent is null or nvl(ev_user, '���⥬�') = :agent) and "
            "  (:module is null or nvl(screen.name, '���⥬�') = :module) and "
            "  (:station is null or nvl(station, '���⥬�') = :station) and "
            "  arx_events.type IN ( "
            "    :evtFlt, "
            "    :evtPax, "
            "    :evtPay, "
            "    :evtGraph, "
            "    :evtTlg, "
            "    :evtComp, "
            "    :evtAccess, "
            "    :evtSystem, "
            "    :evtCodif, "
            "    :evtPeriod "
            "  ) ";

        Qry.CreateVariable("evtFlt", otString, NodeAsString("evtFlt", reqNode));
        Qry.CreateVariable("evtPax", otString, NodeAsString("evtPax", reqNode));
        {
            xmlNodePtr node = GetNode("evtPay", reqNode);
            string evtPay;
            if(node)
                evtPay = NodeAsString(node);
            Qry.CreateVariable("evtPay", otString, evtPay);
        }
        Qry.CreateVariable("evtGraph", otString, NodeAsString("evtGraph", reqNode));
        Qry.CreateVariable("evtTlg", otString, NodeAsString("evtTlg", reqNode));
        Qry.CreateVariable("evtComp", otString, NodeAsString("evtComp", reqNode));
        Qry.CreateVariable("evtAccess", otString, NodeAsString("evtAccess", reqNode));
        Qry.CreateVariable("evtSystem", otString, NodeAsString("evtSystem", reqNode));
        Qry.CreateVariable("evtCodif", otString, NodeAsString("evtCodif", reqNode));
        Qry.CreateVariable("evtPeriod", otString, NodeAsString("evtPeriod", reqNode));

        Qry.CreateVariable("FirstDate", otDate, ClientToUTC(NodeAsDateTime("FirstDate", reqNode), reqInfo->desk.tz_region));
        Qry.CreateVariable("LastDate", otDate, ClientToUTC(NodeAsDateTime("LastDate", reqNode), reqInfo->desk.tz_region));
        Qry.CreateVariable("agent", otString, NodeAsString("agent", reqNode));
        Qry.CreateVariable("station", otString, NodeAsString("station", reqNode));
        Qry.CreateVariable("module", otString, NodeAsString("module", reqNode));
    } else
        throw Exception((string)"PaxLog: unknown tag " + tag);
    Qry.SQLText = qry;
    try {
        Qry.Execute();
    } catch (EOracleError E) {
        if(E.Code == 376)
            throw UserException("� �������� ��������� ��� ���� �� 䠩��� �� �⪫�祭. ������� � ������������");
        else
            throw;
    }
    if(!Qry.Eof) {
        xmlNodePtr paxLogNode = NewTextChild(resNode, "PaxLog");
        xmlNodePtr headerNode = NewTextChild(paxLogNode, "header");
        xmlNodePtr colNode;


        colNode = NewTextChild(headerNode, "col", "�����");
        SetProp(colNode, "width", 73);
        SetProp(colNode, "align", taLeftJustify);

        colNode = NewTextChild(headerNode, "col", "�⮩��");
        SetProp(colNode, "width", 60);
        SetProp(colNode, "align", taLeftJustify);

        colNode = NewTextChild(headerNode, "col", "�६�");
        SetProp(colNode, "width", 90);
        SetProp(colNode, "align", taLeftJustify);

        colNode = NewTextChild(headerNode, "col", "����");
        SetProp(colNode, "width", 90);
        SetProp(colNode, "align", taLeftJustify);

        colNode = NewTextChild(headerNode, "col", "��� �");
        SetProp(colNode, "width", 45);
        SetProp(colNode, "align", taRightJustify);

        colNode = NewTextChild(headerNode, "col", "������");
        SetProp(colNode, "width", 750);
        SetProp(colNode, "align", taLeftJustify);




        xmlNodePtr rowsNode = NewTextChild(paxLogNode, "rows");
        while(!Qry.Eof) {
            string trip;
            {
                //trip name fetch
                TQuery tripQry(&OraSession);
                tripQry.SQLText =
                    "select "
                    "   airline, "
                    "   flt_no, "
                    "   suffix, "
                    "   airp, "
                    "   scd_out, "
                    "   NVL(points.act_out,NVL(points.est_out,points.scd_out)) AS real_out "
                    "from "
                    "   points "
                    "where "
                    "   point_id = :point_id";
                tripQry.CreateVariable("point_id", otInteger, Qry.FieldAsInteger("point_id"));
                tripQry.Execute();
                if(!tripQry.Eof) {
                    TTripInfo info(tripQry);
                    try {
                        trip = GetTripName(info, false, true);
                    } catch(UserException &E) {
                        if(tag != "SystemLogRun")
                            throw UserException(E.what());
                        showErrorMessage((string)E.what()+". ������� ३�� �� �⮡ࠦ�����");
                        Qry.Next();
                        continue;
                    }
                }
            }

            xmlNodePtr rowNode = NewTextChild(rowsNode, "row");

            NewTextChild(rowNode, "point_id", Qry.FieldAsInteger("point_id"));
            NewTextChild(rowNode, "ev_user", Qry.FieldAsString("ev_user"));
            NewTextChild(rowNode, "station", Qry.FieldAsString("station"));

            NewTextChild( rowNode, "time",
                    DateTimeToStr(
                        UTCToClient( Qry.FieldAsDateTime("time"), reqInfo->desk.tz_region),
                        ServerFormatDateTimeAsString
                        )
                    );

            NewTextChild(rowNode, "trip", trip);
            NewTextChild(rowNode, "grp_id", Qry.FieldAsInteger("grp_id"));
            NewTextChild(rowNode, "reg_no", Qry.FieldAsInteger("reg_no"));
            NewTextChild(rowNode, "msg", Qry.FieldAsString("msg"));
            NewTextChild(rowNode, "ev_order", Qry.FieldAsInteger("ev_order"));
            NewTextChild(rowNode, "screen", Qry.FieldAsString("screen"));

            Qry.Next();
        }
    } else
        throw UserException("�� ������� �� ����� ����樨.");
}

struct THallItem {
    int id;
    string name;
};

class THalls: public vector<THallItem> {
    public:
        void Init();
};

void THalls::Init()
{
    TQuery Qry(&OraSession);
    Qry.SQLText = "SELECT id,name FROM astra.halls2,astra.options WHERE halls2.airp=options.cod ORDER BY id";
    Qry.Execute();
    while(!Qry.Eof) {
        THallItem hi;
        hi.id = Qry.FieldAsInteger("id");
        hi.name = Qry.FieldAsString("name");
        this->push_back(hi);
        Qry.Next();
    }
}

void StatInterface::PaxListRun(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    get_report_form("ArxPaxList", resNode);
    {
    TQuery Qry(&OraSession);
    string SQLText =
        "SELECT "
        "   pax_grp.point_dep point_id, "
        "   points.airline, "
        "   points.flt_no, "
        "   points.suffix, "
        "   points.airp, "
        "   points.scd_out, "
        "   NVL(points.act_out,NVL(points.est_out,points.scd_out)) AS real_out, "
        "   pax.reg_no, "
        "   pax_grp.airp_arv, "
        "   pax.surname||' '||pax.name full_name, "
        "   NVL(ckin.get_bagAmount(pax.grp_id,pax.pax_id,rownum),0) bag_amount, "
        "   NVL(ckin.get_bagWeight(pax.grp_id,pax.pax_id,rownum),0) bag_weight, "
        "   NVL(ckin.get_rkWeight(pax.grp_id,pax.pax_id,rownum),0) rk_weight, "
        "   NVL(ckin.get_excess(pax.grp_id,pax.pax_id),0) excess, "
        "   pax_grp.grp_id, "
        "   ckin.get_birks(pax.grp_id,pax.pax_id,0) tags, "
        "   DECODE(pax.refuse,NULL,DECODE(pax.pr_brd,0,'��ॣ.','��ᠦ��'),'���ॣ.('||pax.refuse||')') AS status, "
        "   classes.code class, "
        "   LPAD(seat_no,3,'0')|| "
        "       DECODE(SIGN(1-seats),-1,'+'||TO_CHAR(seats-1),'') seat_no, "
        "   pax_grp.hall hall, "
        "   pax.document, "
        "   pax.ticket_no "
        "FROM  pax_grp,pax, points, "
        "( "
        "  select  "
        "     code,  "
        "     code_lat,  "
        "     name,  "
        "     name_lat, "
        "     priority "
        "  from  "
        "     classes  "
        "  union  "
        "  select "
        "     '�',  "
        "     'M',  "
        "     '�������',  "
        "     'COMFORT' , "
        "     4 "
        "  from  "
        "     dual "
        ") classes "
        "WHERE "
        "   points.point_id = :point_id and "
        "   points.point_id = pax_grp.point_dep and "
        "   pax_grp.grp_id=pax.grp_id AND "
        "   report.get_grp_class(pax_grp.grp_id, 1)=classes.code AND "
        "   points.scd_out >= :FirstDate AND points.scd_out < :LastDate "
        "union "
        "SELECT "
        "   arx_pax_grp.point_dep point_id, "
        "   arx_points.airline, "
        "   arx_points.flt_no, "
        "   arx_points.suffix, "
        "   arx_points.airp, "
        "   arx_points.scd_out, "
        "   NVL(arx_points.act_out,NVL(arx_points.est_out,arx_points.scd_out)) AS real_out, "
        "   arx_pax.reg_no, "
        "   arx_pax_grp.airp_arv, "
        "   arx_pax.surname||' '||arx_pax.name full_name, "
        "   NVL(arch.get_bagAmount(:FirstDate,arx_pax.grp_id,arx_pax.pax_id,rownum),0) bag_amount, "
        "   NVL(arch.get_bagWeight(:FirstDate,arx_pax.grp_id,arx_pax.pax_id,rownum),0) bag_weight, "
        "   NVL(arch.get_rkWeight(:FirstDate,arx_pax.grp_id,arx_pax.pax_id,rownum),0) rk_weight, "
        "   NVL(arch.get_excess(:FirstDate,arx_pax.grp_id,arx_pax.pax_id),0) excess, "
        "   arx_pax_grp.grp_id, "
        "   arch.get_birks(:FirstDate,arx_pax.grp_id,arx_pax.pax_id,0) tags, "
        "   DECODE(arx_pax.refuse,NULL,DECODE(arx_pax.pr_brd,0,'��ॣ.','��ᠦ��'), "
        "       '���ॣ.('||arx_pax.refuse||')') AS status, "
        "   classes.code class, "
        "   LPAD(seat_no,3,'0')|| "
        "       DECODE(SIGN(1-seats),-1,'+'||TO_CHAR(seats-1),'') seat_no, "
        "   arx_pax_grp.hall hall, "
        "   arx_pax.document, "
        "   arx_pax.ticket_no "
        "FROM  arx_pax_grp,arx_pax, arx_points, "
        "( "
        "  select  "
        "     code,  "
        "     code_lat,  "
        "     name,  "
        "     name_lat, "
        "     priority "
        "  from  "
        "     classes  "
        "  union  "
        "  select "
        "     '�',  "
        "     'M',  "
        "     '�������',  "
        "     'COMFORT' , "
        "     4 "
        "  from  "
        "     dual "
        ") classes "
        "WHERE "
        "   arx_points.point_id = :point_id and "
        "   arx_points.point_id = arx_pax_grp.point_dep and "
        "   arx_pax_grp.grp_id=arx_pax.grp_id AND "
        "   arch.get_grp_class(arx_pax_grp.grp_id,:FirstDate, 1)=classes.code AND "
        "   arx_points.scd_out >= :FirstDate AND arx_points.scd_out < :LastDate and "
        "   arx_points.part_key >= :FirstDate and "
        "   arx_pax_grp.part_key >= :FirstDate and "
        "   arx_pax.part_key >= :FirstDate and "
        "   pr_brd IS NOT NULL ";

    Qry.SQLText = SQLText;

    TReqInfo *reqInfo = TReqInfo::Instance();

    Qry.CreateVariable("FirstDate", otDate, ClientToUTC(NodeAsDateTime("FirstDate", reqNode), reqInfo->desk.tz_region));
    Qry.CreateVariable("LastDate", otDate, ClientToUTC(NodeAsDateTime("LastDate", reqNode), reqInfo->desk.tz_region));
    Qry.CreateVariable("point_id", otInteger, NodeAsInteger("point_id", reqNode));

    Qry.Execute();
    if(!Qry.Eof) {
        xmlNodePtr paxListNode = NewTextChild(resNode, "paxList");
        xmlNodePtr headerNode = NewTextChild(paxListNode, "header");
        xmlNodePtr colNode;

        colNode = NewTextChild(headerNode, "col", "����");
        SetProp(colNode, "width", 53);
        SetProp(colNode, "align", taLeftJustify);

        colNode = NewTextChild(headerNode, "col", "���");
        SetProp(colNode, "width", 61);
        SetProp(colNode, "align", taLeftJustify);

        colNode = NewTextChild(headerNode, "col", "�");
        SetProp(colNode, "width", 25);
        SetProp(colNode, "align", taRightJustify);

        colNode = NewTextChild(headerNode, "col", "�������");
        SetProp(colNode, "width", 173);
        SetProp(colNode, "align", taLeftJustify);

        colNode = NewTextChild(headerNode, "col", "�/�");
        SetProp(colNode, "width", 32);
        SetProp(colNode, "align", taLeftJustify);

        colNode = NewTextChild(headerNode, "col", "����");
        SetProp(colNode, "width", 40);
        SetProp(colNode, "align", taRightJustify);

        colNode = NewTextChild(headerNode, "col", "���");
        SetProp(colNode, "width", 40);
        SetProp(colNode, "align", taRightJustify);

        colNode = NewTextChild(headerNode, "col", "�/�");
        SetProp(colNode, "width", 40);
        SetProp(colNode, "align", taRightJustify);

        colNode = NewTextChild(headerNode, "col", "����");
        SetProp(colNode, "width", 40);
        SetProp(colNode, "align", taRightJustify);

        colNode = NewTextChild(headerNode, "col", "��ન");
        SetProp(colNode, "width", 163);
        SetProp(colNode, "align", taLeftJustify);

        colNode = NewTextChild(headerNode, "col", "�����");
        SetProp(colNode, "width", 93);
        SetProp(colNode, "align", taLeftJustify);

        colNode = NewTextChild(headerNode, "col", "��.");
        SetProp(colNode, "width", 25);
        SetProp(colNode, "align", taLeftJustify);

        colNode = NewTextChild(headerNode, "col", "� �");
        SetProp(colNode, "width", 40);
        SetProp(colNode, "align", taLeftJustify);

        colNode = NewTextChild(headerNode, "col", "���");
        SetProp(colNode, "width", 78);
        SetProp(colNode, "align", taLeftJustify);

        colNode = NewTextChild(headerNode, "col", "���㬥��");
        SetProp(colNode, "width", 114);
        SetProp(colNode, "align", taLeftJustify);

        colNode = NewTextChild(headerNode, "col", "� �����");
        SetProp(colNode, "width", 101);
        SetProp(colNode, "align", taLeftJustify);

        xmlNodePtr rowsNode = NewTextChild(paxListNode, "rows");
        while(!Qry.Eof) {
            TTripInfo info(Qry);
            string trip = GetTripName(info);
            xmlNodePtr paxNode = NewTextChild(rowsNode, "pax");

            NewTextChild(paxNode, "point_id", Qry.FieldAsInteger("point_id"));
            NewTextChild(paxNode, "airline", Qry.FieldAsString("airline"));
            NewTextChild(paxNode, "flt_no", Qry.FieldAsInteger("flt_no"));
            NewTextChild(paxNode, "suffix", Qry.FieldAsString("suffix"));
            NewTextChild(paxNode, "trip", trip);

            NewTextChild( paxNode, "scd_out",
                    DateTimeToStr(
                        UTCToClient( Qry.FieldAsDateTime("scd_out"), reqInfo->desk.tz_region),
                        ServerFormatDateTimeAsString
                        )
                    );

            NewTextChild(paxNode, "reg_no", Qry.FieldAsInteger("reg_no"));
            NewTextChild(paxNode, "airp_arv", Qry.FieldAsString("airp_arv"));
            NewTextChild(paxNode, "full_name", Qry.FieldAsString("full_name"));
            NewTextChild(paxNode, "bag_amount", Qry.FieldAsInteger("bag_amount"));
            NewTextChild(paxNode, "bag_weight", Qry.FieldAsInteger("bag_weight"));
            NewTextChild(paxNode, "rk_weight", Qry.FieldAsInteger("rk_weight"));
            NewTextChild(paxNode, "excess", Qry.FieldAsInteger("excess"));
            NewTextChild(paxNode, "grp_id", Qry.FieldAsInteger("grp_id"));
            NewTextChild(paxNode, "tags", Qry.FieldAsString("tags"));
            NewTextChild(paxNode, "status", Qry.FieldAsString("status"));
            NewTextChild(paxNode, "class", Qry.FieldAsString("class"));
            NewTextChild(paxNode, "seat_no", Qry.FieldAsString("seat_no"));
            NewTextChild(paxNode, "hall", Qry.FieldAsInteger("hall"));
            NewTextChild(paxNode, "document", Qry.FieldAsString("document"));
            NewTextChild(paxNode, "ticket_no", Qry.FieldAsString("ticket_no"));

            Qry.Next();
        }
    } else
        throw UserException("�� ������� �� ������ ���ᠦ��");

    STAT::set_variables(resNode);
    ProgTrace(TRACE5, "%s", GetXMLDocText(resNode->doc).c_str());

    return;
    }
    ////////////////////////////////////////////////////////////////////////////////////////////


/*


    string tag = (char *)reqNode->name;
    xmlNodePtr paramsNode = GetNode("sqlparams", reqNode);
    string grp_id, grp, arx_grp_id, arx_grp;
    if(tag == "PaxListRun")
        ; // do nothing
    else if(tag == "PaxSrcRun") {
        xmlNodePtr curNode = paramsNode->children;

        string family, pass, ticketno;
        family = NodeAsStringFast("family", curNode, "");
        pass = NodeAsStringFast("pass", curNode, "");
        ticketno = NodeAsStringFast("ticketno", curNode, "");

        if(family.size() || pass.size() || ticketno.size()) {
            TDateTime FirstDate = NodeAsDateTime("FirstDate", paramsNode);
            TDateTime LastDate = NodeAsDateTime("LastDate", paramsNode);
            grp_id =
                ",(SELECT DISTINCT grp_id FROM astra.pax "
                "  WHERE (:family is null or surname ";
            if(FirstDate + 1 < LastDate && family.size() < 4)
                grp_id += " = :family) and ";
            else
                grp_id += " LIKE :family||'%') and ";
            grp_id +=
                "  (:ticketno is null or ticket_no = :ticketno) and  "
                "  (:pass is null or document = :pass)  "
                "  ORDER BY grp_id) grps ";
            grp = " pax_grp.grp_id=grps.grp_id AND ";

            arx_grp_id =
                ",(SELECT DISTINCT part_key,grp_id FROM arx.pax "
                "  WHERE pax.part_key>= :FirstDate AND pax.part_key< :LastDate AND ";
            if(FirstDate + 1 < LastDate && family.size() < 4)
                arx_grp_id += "        (:family is null or surname= :family) and ";
            else
                arx_grp_id += "        (:family is null or surname LIKE :family||'%') and ";
            arx_grp_id +=
                "  (:ticketno is null or ticket_no = :ticketno) and  "
                "  (:pass is null or document = :pass)  "
                "  ORDER BY part_key,grp_id) grps ";
            arx_grp = "pax.part_key=grps.part_key AND pax.grp_id=grps.grp_id AND ";
        } else {
            grp_id =
                ",(SELECT DISTINCT grp_id FROM astra.bag_tags "
                "  WHERE no=:n_birk ORDER BY grp_id) grps ";
            grp = "pax_grp.grp_id=grps.grp_id AND ";
            arx_grp_id =
                ",(SELECT DISTINCT part_key,grp_id FROM arx.bag_tags "
                "  WHERE bag_tags.part_key>= :FirstDate AND bag_tags.part_key< :LastDate AND "
                "        no=:n_birk ORDER BY part_key,grp_id) grps ";
            arx_grp = "pax.part_key=grps.part_key AND pax.grp_id=grps.grp_id AND ";
        }
    } else
        throw Exception((string)"PaxLog: unknown tag " + tag);
    string qry = (string)
        "SELECT "
        "  pax.pax_id,pax_grp.point_id, "
        "  pax_grp.grp_id, "
        "  trips.company AS airline,trips.flt_no,trips.suffix, "
        "  trips.scd, "
        "  pax.reg_no, "
        "  pax.surname,pax.name, "
        "  pax_grp.target, "
        "  astra.ckin.get_bagAmount(pax_grp.grp_id,pax.pax_id,rownum) AS bag_amount, "
        "  astra.ckin.get_bagWeight(pax_grp.grp_id,pax.pax_id,rownum) AS bag_weight, "
        "  astra.ckin.get_rkWeight(pax_grp.grp_id,pax.pax_id,rownum) AS rk_weight, "
        "  astra.ckin.get_birks(pax_grp.grp_id,pax.pax_id,0) AS tags, "
        "  astra.ckin.get_excess(pax_grp.grp_id,pax.pax_id) AS excess, "
        "  DECODE(pax.refuse,NULL,DECODE(pax_grp.pr_wl,0,DECODE(pax.pr_brd,0,'��ॣ.','��ᠦ��'),'��'),'���ॣ.('||pax.refuse||')') AS status, "
        "  pax_grp.class, "
        "  LPAD(pax.seat_no,3,'0')|| "
        "           DECODE(SIGN(1-pax.seats),-1,'+'||TO_CHAR(pax.seats-1),'') AS seat_no, "
        "  pax.document, "
        "  pax.ticket_no "
        "FROM astra.trips,astra.pax_grp,astra.pax " +
        grp_id +
        "WHERE trips.trip_id=pax_grp.point_id AND pax_grp.grp_id=pax.grp_id AND " +
        grp +
        "      trips.scd>= :FirstDate AND trips.scd< :LastDate AND "
        "      (:trip IS NULL OR trips.trip= :trip) and "
        "      (:dest IS NULL OR pax_grp.target IN (SELECT cod FROM astra.place WHERE place.trip_id=trips.trip_id AND place.city=:dest)) "
        "UNION "
        "SELECT "
        "  pax.pax_id,pax_grp.point_id, "
        "  pax_grp.grp_id, "
        "  trips.company AS airline,trips.flt_no,trips.suffix, "
        "  trips.scd, "
        "  pax.reg_no, "
        "  pax.surname,pax.name, "
        "  pax_grp.target, "
        "  arx.ckin.get_bagAmount(pax_grp.part_key,pax_grp.grp_id,pax.pax_id,rownum) AS bag_amount, "
        "  arx.ckin.get_bagWeight(pax_grp.part_key,pax_grp.grp_id,pax.pax_id,rownum) AS bag_weight, "
        "  arx.ckin.get_rkWeight(pax_grp.part_key,pax_grp.grp_id,pax.pax_id,rownum) AS rk_weight, "
        "  arx.ckin.get_birks(pax_grp.part_key,pax_grp.grp_id,pax.pax_id) AS tags, "
        "  arx.ckin.get_excess(pax_grp.part_key,pax_grp.grp_id,pax.pax_id) AS excess, "
        "  DECODE(pax.refuse,NULL,DECODE(pax_grp.pr_wl,0,DECODE(pax.pr_brd,0,'��ॣ.','��ᠦ��'),'��'),'���ॣ.('||pax.refuse||')') AS status, "
        "  pax_grp.class, "
        "  LPAD(pax.seat_no,3,'0')|| "
        "           DECODE(SIGN(1-pax.seats),-1,'+'||TO_CHAR(pax.seats-1),'') AS seat_no, "
        "  pax_grp.hall AS hall, "
        "  pax.document, "
        "  pax.ticket_no "
        "FROM arx.trips,arx.pax_grp,arx.pax " +
        arx_grp_id +
        "WHERE trips.part_key=pax_grp.part_key AND trips.trip_id=pax_grp.point_id AND pax_grp.part_key=pax.part_key AND pax_grp.grp_id=pax.grp_id AND " +
        arx_grp +
        "      trips.part_key>= :FirstDate AND trips.part_key< :LastDate AND "
        "      (:trip IS NULL OR trips.trip= :trip) and "
        "      (:dest IS NULL OR pax_grp.target IN (SELECT cod FROM arx.place WHERE place.part_key=trips.part_key AND place.trip_id=trips.trip_id AND place.city=:dest)) ";

    THalls halls;
    halls.Init();
    TQuery Qry(&OraSession);
    Qry.SQLText = qry;
    TParams1 SQLParams;
    SQLParams.getParams(paramsNode);
    SQLParams.setSQL(&Qry);
    try {
        Qry.Execute();
    } catch (EOracleError E) {
        if(E.Code == 376)
            throw UserException(376);
        else
            throw;
    }
    xmlNodePtr dataNode = NewTextChild(resNode, "data");
    xmlNodePtr PaxesNode = NewTextChild(dataNode, "Paxes");
    while(!Qry.Eof) {
        xmlNodePtr rowNode = NewTextChild(PaxesNode, "row");

        string airline = Qry.FieldAsString("airline");
        int flt_no = Qry.FieldAsInteger("flt_no");
        string suffix = Qry.FieldAsString("suffix");

        NewTextChild(rowNode, "trip_id", Qry.FieldAsInteger("point_id"));
        NewTextChild(rowNode, "airline", airline);
        NewTextChild(rowNode, "flt_no", flt_no);
        NewTextChild(rowNode, "suffix", suffix);
        NewTextChild(rowNode, "trip", airline+IntToString(flt_no)+suffix);
        NewTextChild(rowNode, "scd", DateTimeToStr(Qry.FieldAsDateTime("scd")));
        NewTextChild(rowNode, "n_reg", Qry.FieldAsInteger("reg_no"));
        NewTextChild(rowNode, "family", (string)Qry.FieldAsString("surname")+" "+Qry.FieldAsString("name"));
        NewTextChild(rowNode, "bagAmount", Qry.FieldAsInteger("bag_amount"));
        NewTextChild(rowNode, "bagWeight", Qry.FieldAsInteger("bag_weight"));
        NewTextChild(rowNode, "rkWeight", Qry.FieldAsInteger("rk_weight"));
        NewTextChild(rowNode, "excess", Qry.FieldAsInteger("excess"));
        NewTextChild(rowNode, "grp_id", Qry.FieldAsInteger("grp_id"));
        NewTextChild(rowNode, "target", Qry.FieldAsString("target"));
        NewTextChild(rowNode, "tags", Qry.FieldAsString("tags"));
        NewTextChild(rowNode, "status", Qry.FieldAsString("status"));
        NewTextChild(rowNode, "class", Qry.FieldAsString("class"));
        NewTextChild(rowNode, "seat_no", Qry.FieldAsString("seat_no"));
        {
            string hall;
            if(!Qry.FieldIsNULL("hall")) {
                int hall_id = Qry.FieldAsInteger("hall");
                THalls::iterator ih = halls.begin();
                for(; ih != halls.end(); ih++) {
                    if(ih->id == hall_id) break;
                }
                if(ih == halls.end())
                    hall = IntToString(hall_id);
                else
                    hall = ih->name;
            }
            NewTextChild(rowNode, "hall", hall);
        }
        NewTextChild(rowNode, "document", Qry.FieldAsString("document"));
        NewTextChild(rowNode, "ticket_no", Qry.FieldAsString("ticket_no"));

        Qry.Next();
    }*/
}

struct TTagQryParts {
    string
        select,
        order_by,
        astra_select,
        arx_select,
        astra_from,
        arx_from,
        where,
        astra_group_by,
        arx_group_by,
        get_birks;
    TTagQryParts(int state);
    bool TagDetailCheck(int i);
    bool checked[5];
    private:
    bool cscd, cflt, ctarget, ctarget_trfer, ccls;
    string fgroup_by, fuselect;
};

bool TTagQryParts::TagDetailCheck(int i)
{
    if(i > 4 || i == 1)
        return true;
    else if(i == 2)
        return checked[1];
    else if(i == 3)
        return checked[2] || checked[3];
    else
        return checked[i];
}

TTagQryParts::TTagQryParts(int state)
{
    cscd = state & 16;
    cflt = state & 8;
    ctarget = state & 4;
    ctarget_trfer = state & 2;
    ccls = state & 1;

    checked[0] = cscd;
    checked[1] = cflt;
    checked[2] = ctarget;
    checked[3] = ctarget_trfer;
    checked[4] = ccls;

    get_birks = "trip_company, ";
    order_by = "trip_company, ";
    fuselect = "trips.company   trip_company, ";
    fgroup_by = "trips.company, ";

    if(cflt) {
        get_birks = get_birks +
            "trip_flt_no, " +
            "nvl(trip_suffix, ' '), ";
        order_by = order_by +
            "trip_flt_no, " +
            "trip_suffix, ";
        fuselect = fuselect +
            "trips.flt_no    trip_flt_no, " +
            "trips.suffix    trip_suffix, ";
        fgroup_by = fgroup_by +
            "trips.flt_no, " +
            "trips.suffix, ";
    } else
        get_birks = get_birks +
            "NULL, " +
            "NULL, ";


    if(cflt && ctarget_trfer) {
        get_birks = get_birks +
            "nvl(transfer_company, ' '), " +
            "nvl(transfer_flt_no, -1), " +
            "nvl(transfer_suffix, ' '), ";
        order_by = order_by +
            "transfer_company, " +
            "transfer_flt_no, " +
            "transfer_suffix, ";
        fuselect = fuselect +
            "lt.airline      transfer_company, " +
            "lt.flt_no       transfer_flt_no, " +
            "lt.suffix       transfer_suffix, ";
        fgroup_by = fgroup_by +
            "lt.airline, " +
            "lt.flt_no, " +
            "lt.suffix, ";
    } else
        get_birks = get_birks +
            "NULL, " +
            "NULL, " +
            "NULL, ";

    if(ctarget) {
        get_birks = get_birks + "target, ";
        order_by = order_by + "target, ";
        fuselect = fuselect + "pax_grp.target  target, ";
        fgroup_by = fgroup_by + "pax_grp.target, ";
    } else
        get_birks = get_birks + "NULL, ";

    if(ctarget_trfer) {
        get_birks = get_birks + "nvl(transfer_target, ' '), ";
        order_by = order_by + "transfer_target, ";
        fuselect = fuselect + "lt.airp_arv     transfer_target, ";
        fgroup_by = fgroup_by + "lt.airp_arv, ";
    } else
        get_birks = get_birks + "NULL, ";

    if(ccls) {
        get_birks = get_birks + "cls, ";
        order_by = order_by + "cls, ";
        fuselect = fuselect + "pax_grp.class   cls, ";
        fgroup_by = fgroup_by + "pax_grp.class, ";
    } else
        get_birks = get_birks + "NULL, ";

    if(cscd) {
        select = "scd, " + order_by;
        astra_select =
            "to_char(trips.scd, 'dd.mm.yy')   scd, " +
            fuselect;
        arx_select =
            "to_char(trips.part_key, 'dd.mm.yy')   scd, " +
            fuselect;
        astra_group_by =
            fgroup_by;
        arx_group_by =
            fgroup_by;
    } else {
        select = order_by;
        astra_select = fuselect;
        arx_select = fuselect;
        astra_group_by = fgroup_by;
        arx_group_by = fgroup_by;
    }

    if(ctarget_trfer) {
        astra_from =
            "( "
            "  SELECT transfer.grp_id, "
            "         airline,flt_no,suffix,airp_arv,subclass "
            "  FROM astra.transfer, "
            "      (SELECT grp_id,MAX(transfer_num) AS transfer_num "
            "       FROM astra.transfer WHERE transfer_num>0 GROUP BY grp_id) last_transfer "
            "  WHERE "
            "      transfer.grp_id=last_transfer.grp_id AND "
            "      transfer.transfer_num=last_transfer.transfer_num "
            ") lt, ";
        arx_from =
            "( "
            "  SELECT transfer.grp_id, "
            "         airline,flt_no,suffix,airp_arv,subclass "
            "  FROM arx.transfer, "
            "      (SELECT grp_id,MAX(transfer_num) AS transfer_num "
            "       FROM arx.transfer WHERE transfer_num>0 GROUP BY grp_id) last_transfer "
            "  WHERE "
            "      transfer.part_key >= :FirstDate and "
            "      transfer.part_key < :LastDate and "
            "      transfer.grp_id=last_transfer.grp_id AND "
            "      transfer.transfer_num=last_transfer.transfer_num "
            ") lt, ";
        where = "pax_grp.grp_id = lt.grp_id(+) and ";
    }
}

const int BagTagColsSize = 8;

void StatInterface::BagTagStatRun(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    TTagQryParts qry_parts(NodeAsInteger("BagTagCBState", reqNode));
    string qry = (string)
        "select "
        "  tscd, " +
        qry_parts.select +
        "  color, "
        "  arx.test.get_birks(tscd, " +
        qry_parts.get_birks +
        "    nvl(color, ' ') "
        "  ) tags "
        "from ( "
        "select "
        "  trips.scd tscd, " +
        qry_parts.astra_select +
        "  bag_tags.color  color "
        "from "
        "  astra.trips, "
        "  astra.pax_grp, " +
        qry_parts.astra_from +
        "  astra.bag_tags "
        "where "
        "  trips.scd >= :FirstDate and "
        "  trips.scd < :LastDate and "
        "  (:awk is null or trips.company = :awk) and "
        "  trips.trip_id = pax_grp.point_id and " +
        qry_parts.where +
        "  pax_grp.grp_id = bag_tags.grp_id "
        "group by "
        "  trips.scd, " +
        qry_parts.astra_group_by +
        "  bag_tags.color "
        "union  "
        "select "
        "  trips.part_key tscd, " +
        qry_parts.arx_select +
        "  bag_tags.color  color "
        "from "
        "  arx.trips, "
        "  arx.pax_grp, " +
        qry_parts.arx_from +
        "  arx.bag_tags "
        "where "
        "  trips.part_key >= :FirstDate and "
        "  trips.part_key < :LastDate and "
        "  (:awk is null or trips.company = :awk) and "
        "  trips.trip_id = pax_grp.point_id and "
        "  trips.part_key = pax_grp.part_key and " +
        qry_parts.where +
        "  pax_grp.part_key = bag_tags.part_key and "
        "  pax_grp.grp_id = bag_tags.grp_id "
        "group by "
        "  trips.part_key, " +
        qry_parts.arx_group_by +
        "  bag_tags.color "
        ") "
        "order by "
        "  tscd, " +
        qry_parts.order_by +
        "color ";

    TQuery Qry(&OraSession);
    Qry.SQLText = qry;
    TParams1 SQLParams;
    SQLParams.getParams(GetNode("sqlparams", reqNode));
    SQLParams.setSQL(&Qry);
    Qry.Execute();

    xmlNodePtr dataNode = NewTextChild(resNode, "data");
    xmlNodePtr BagTagGrdNode = NewTextChild(dataNode, "BagTagGrd");
    while(!Qry.Eof) {
        xmlNodePtr rowNode = NewTextChild(BagTagGrdNode, "row");
        for(int i = 0; i < BagTagColsSize - 3; i++) {
            bool checked = qry_parts.TagDetailCheck(i);
            if(checked && i == 0)
                NewTextChild(rowNode, "col", Qry.FieldAsString("scd"));
            if(checked && i == 1)
                NewTextChild(rowNode, "col", Qry.FieldAsString("trip_company"));
            if(checked && i == 2) {
                string trip_company = Qry.FieldAsString("trip_company");
                string trip_flt_no = IntToString(Qry.FieldAsInteger("trip_flt_no"));
                string trip_suffix = Qry.FieldAsString("trip_suffix");
                string transfer_company;
                string transfer_flt_no;
                string transfer_suffix;

                if(qry_parts.checked[3]) {
                    transfer_company = Qry.FieldAsString("transfer_company");
                    transfer_flt_no = IntToString(Qry.FieldAsInteger("transfer_flt_no"));
                    transfer_suffix = Qry.FieldAsString("transfer_suffix");
                }
                string col2 = trip_company + trip_flt_no + trip_suffix;
                if(transfer_company.size())
                    col2 += "-" + transfer_company + transfer_flt_no + transfer_suffix;
                NewTextChild(rowNode, "col", col2);
            }
            if(checked && i == 3) {
                string target, transfer_target, col3;
                if(qry_parts.checked[2])
                    target = Qry.FieldAsString("target");
                if(qry_parts.checked[3])
                    transfer_target = Qry.FieldAsString("transfer_target");
                if(target.size()) {
                    col3 = target;
                    if(transfer_target.size())
                        col3 += "-" + transfer_target;
                } else
                    col3 = transfer_target;
                NewTextChild(rowNode, "col", col3);
            }
            if(checked && i == 4)
                NewTextChild(rowNode, "col", Qry.FieldAsString("cls"));
        }
        NewTextChild(rowNode, "col", Qry.FieldAsString("color"));
        string tags = Qry.FieldAsString("tags");
        string::size_type pos = tags.find(":");
        NewTextChild(rowNode, "col", tags.substr(pos + 1));
        NewTextChild(rowNode, "col", tags.substr(0, pos));
        Qry.Next();
    }
}

void StatInterface::DepStatRun(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    TQuery Qry(&OraSession);
    Qry.SQLText =
        "SELECT "
        "  0 AS priority, "
        "  halls2.name AS hall, "
        "  NVL(SUM(trips),0) AS trips, "
        "  NVL(SUM(pax),0) AS pax, "
        "  NVL(SUM(weight),0) AS weight, "
        "  NVL(SUM(unchecked),0) AS unchecked, "
        "  NVL(SUM(excess),0) AS excess "
        "FROM "
        " (SELECT     "
        "    0,stat.hall, "
        "    COUNT(DISTINCT trips.trip_id) AS trips, "
        "    SUM(f+c+y) AS pax, "
        "    SUM(weight) AS weight, "
        "    SUM(unchecked) AS unchecked, "
        "    SUM(stat.excess) AS excess "
        "  FROM astra.trips,astra.stat "
        "  WHERE trips.trip_id=stat.trip_id AND "
        "        trips.scd>= :FirstDate AND trips.scd< :LastDate AND "
        "        (:flt IS NULL OR trips.trip= :flt) AND "
        "        (:dest IS NULL OR stat.target IN (SELECT cod FROM astra.place WHERE place.trip_id=trips.trip_id AND place.city=:dest)) and "
        "        (:awk IS NULL OR trips.company = :awk) and "
        "        exists ( "
        "          select * from astra.pax_grp where "
        "            trips.trip_id = pax_grp.point_id(+) and "
        "            (:class is null or pax_grp.class = :class) "
        "        ) "
        "  GROUP BY stat.hall "
        "  UNION "
        "  SELECT "
        "    1,stat.hall, "
        "    COUNT(DISTINCT trips.trip_id) AS trips, "
        "    SUM(f+c+y) AS pax, "
        "    SUM(weight) AS weight, "
        "    SUM(unchecked) AS unchecked, "
        "    SUM(stat.excess) AS excess "
        "  FROM arx.trips,arx.stat "
        "  WHERE trips.part_key=stat.part_key AND trips.trip_id=stat.trip_id AND "
        "        stat.part_key>= :FirstDate AND stat.part_key< :LastDate AND "
        "        (:flt IS NULL OR trips.trip= :flt) AND "
        "        (:dest IS NULL OR stat.target IN (SELECT cod FROM arx.place WHERE place.part_key=trips.part_key AND place.trip_id=trips.trip_id AND place.city=:dest)) and "
        "        (:awk IS NULL OR trips.company = :awk) and "
        "        exists ( "
        "          select * from arx.pax_grp where "
        "            trips.trip_id = pax_grp.point_id(+) and "
        "            (:class is null or pax_grp.class = :class) "
        "        ) "
        "  GROUP BY stat.hall) stat,astra.halls2 "
        "WHERE stat.hall(+)=halls2.id "
        "GROUP BY halls2.name "
        "UNION "
        "SELECT "
        "  1 AS priority, "
        "  '�ᥣ�' AS hall, "
        "  NVL(SUM(trips),0) AS trips, "
        "  NVL(SUM(pax),0) AS pax, "
        "  NVL(SUM(weight),0) AS weight, "
        "  NVL(SUM(unchecked),0) AS unchecked, "
        "  NVL(SUM(excess),0) AS excess "
        "FROM "
        " (SELECT         "
        "    COUNT(DISTINCT trips.trip_id) AS trips, "
        "    SUM(f+c+y) AS pax, "
        "    SUM(weight) AS weight, "
        "    SUM(unchecked) AS unchecked, "
        "    SUM(stat.excess) AS excess "
        "  FROM astra.trips,astra.stat "
        "  WHERE trips.trip_id=stat.trip_id AND "
        "        trips.scd>= :FirstDate AND trips.scd< :LastDate AND "
        "        (:flt IS NULL OR trips.trip= :flt) AND "
        "        (:dest IS NULL OR stat.target IN (SELECT cod FROM astra.place WHERE place.trip_id=trips.trip_id AND place.city=:dest)) and "
        "        (:awk IS NULL OR trips.company = :awk) and "
        "        exists ( "
        "          select * from astra.pax_grp where "
        "            trips.trip_id = pax_grp.point_id(+) and "
        "            (:class is null or pax_grp.class = :class) "
        "        ) "
        "union "
        "  SELECT "
        "    COUNT(DISTINCT trips.trip_id) AS trips, "
        "    SUM(f+c+y) AS pax, "
        "    SUM(weight) AS weight, "
        "    SUM(unchecked) AS unchecked, "
        "    SUM(stat.excess) AS excess "
        "  FROM arx.trips,arx.stat "
        "  WHERE trips.part_key=stat.part_key AND trips.trip_id=stat.trip_id AND "
        "        stat.part_key>= :FirstDate AND stat.part_key< :LastDate AND "
        "        (:flt IS NULL OR trips.trip= :flt) AND "
        "        (:dest IS NULL OR stat.target IN (SELECT cod FROM arx.place WHERE place.part_key=trips.part_key AND place.trip_id=trips.trip_id AND place.city=:dest)) and "
        "        (:awk IS NULL OR trips.company = :awk) and "
        "        exists ( "
        "          select * from arx.pax_grp where "
        "            trips.trip_id = pax_grp.point_id(+) and "
        "            (:class is null or pax_grp.class = :class) "
        "        ) "
        ") stat "
        "ORDER BY priority,hall ";

    TParams1 SQLParams;

    xmlNodePtr paramsNode = GetNode("sqlparams", reqNode);
    TDateTime FirstDate = NodeAsDateTime("FirstDate", paramsNode);
    TDateTime LastDate = NodeAsDateTime("LastDate", paramsNode);
    SQLParams.getParams(paramsNode);

    SQLParams.setSQL(&Qry);
    try {
        Qry.Execute();
    } catch (EOracleError E) {
        if(E.Code == 376)
            throw UserException(376);
        else
            throw;
    }

    xmlNodePtr dataNode = NewTextChild(resNode, "data");
    xmlNodePtr DepStatNode = NewTextChild(dataNode, "DepStat");
    while(!Qry.Eof) {
        int pax = Qry.FieldAsInteger("pax");
        xmlNodePtr rowNode = NewTextChild(DepStatNode, "row");
        NewTextChild(rowNode, "hall", Qry.FieldAsString("hall"));
        NewTextChild(rowNode, "trips", Qry.FieldAsInteger("trips"));
        NewTextChild(rowNode, "pax", pax);
        NewTextChild(rowNode, "weight", Qry.FieldAsInteger("weight"));
        NewTextChild(rowNode, "unchecked", Qry.FieldAsInteger("unchecked"));
        NewTextChild(rowNode, "excess", Qry.FieldAsInteger("excess"));
        char avg[50];
        sprintf(avg, "%.2f", pax/(LastDate - FirstDate));
        NewTextChild(rowNode, "avg", avg);
        Qry.Next();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void STAT::set_variables(xmlNodePtr resNode)
{

    xmlNodePtr formDataNode = GetNode("form_data", resNode);
    if(!formDataNode)
        formDataNode = NewTextChild(resNode, "form_data");
    xmlNodePtr variablesNode = GetNode("variables", formDataNode);
    if(!variablesNode)
        variablesNode = NewTextChild(formDataNode, "variables");

    TReqInfo *reqInfo = TReqInfo::Instance();
    TDateTime issued = UTCToLocal(NowUTC(),reqInfo->desk.tz_region);
    string tz;
    if(reqInfo->user.time_form == tfUTC)
        tz = "(GMT)";
    else if(
            reqInfo->user.time_form == tfLocalDesk ||
            reqInfo->user.time_form == tfLocalAll
           )
        tz = "(" + reqInfo->desk.city + ")";

    NewTextChild(variablesNode, "print_date",
            DateTimeToStr(issued, "dd.mm.yyyy hh:nn:ss ") + tz);
    NewTextChild(variablesNode, "print_oper", reqInfo->user.login);
    NewTextChild(variablesNode, "print_term", reqInfo->desk.code);
}

void RunFullStat(xmlNodePtr reqNode, xmlNodePtr resNode)
{
    TReqInfo &info = *(TReqInfo::Instance());
    get_report_form("FullStat", resNode);

    string ak = Trim(NodeAsString("ak", reqNode));
    string ap = Trim(NodeAsString("ap", reqNode));

    TQuery Qry(&OraSession);
    string SQLText =
        "select "
        "  airp, "
        "  airline, "
        "  flt_no, "
        "  scd_out, "
        "  point_id, "
        "  places, "
        "  sum(pax_amount) pax_amount, "
        "  sum(adult) adult, "
        "  sum(child) child, "
        "  sum(baby) baby, "
        "  sum(rk_weight) rk_weight, "
        "  sum(bag_amount) bag_amount, "
        "  sum(bag_weight) bag_weight, "
        "  sum(excess) excess "
        "from "
        "( "
        "select "
        "  points.airp, "
        "  points.airline, "
        "  points.flt_no, "
        "  points.scd_out, "
        "  stat.point_id, "
        "  substr(ckin.get_airps(stat.point_id),1,50) places, "
        "  sum(adult + child + baby) pax_amount, "
        "  sum(adult) adult, "
        "  sum(child) child, "
        "  sum(baby) baby, "
        "  sum(unchecked) rk_weight, "
        "  sum(pcs) bag_amount, "
        "  sum(weight) bag_weight, "
        "  sum(excess) excess "
        "from "
        "  points, "
        "  stat ";
    if (!info.user.access.airlines.empty())
        SQLText += ",aro_airlines ";
    if (!info.user.access.airps.empty())
        SQLText += ",aro_airps ";
    SQLText +=
        "where "
        "  points.point_id = stat.point_id and "
        "  points.scd_out >= :FirstDate AND points.scd_out < :LastDate ";
    if (!info.user.access.airlines.empty())
        SQLText += "AND aro_airlines.airline=points.airline AND aro_airlines.aro_id=:user_id ";
    if (!info.user.access.airps.empty())
        SQLText += "AND aro_airps.airp=points.airp AND aro_airps.aro_id=:user_id ";
    if(ap.size()) {
        SQLText +=
            " and points.airp = :ap ";
        Qry.CreateVariable("ap", otString, ap);
    } else if(ak.size()) {
        SQLText +=
            " and points.airline = :ak ";
        Qry.CreateVariable("ak", otString, ak);
    }
        SQLText +=
        "group by "
        "  points.airp, "
        "  points.airline, "
        "  points.flt_no, "
        "  points.scd_out, "
        "  stat.point_id "
        "union "
        "select "
        "  arx_points.airp, "
        "  arx_points.airline, "
        "  arx_points.flt_no, "
        "  arx_points.scd_out, "
        "  arx_stat.point_id, "
        "  substr(arch.get_airps(arx_stat.point_id, :FirstDate),1,50) places, "
        "  sum(adult + child + baby) pax_amount, "
        "  sum(adult) adult, "
        "  sum(child) child, "
        "  sum(baby) baby, "
        "  sum(unchecked) rk_weight, "
        "  sum(pcs) bag_amount, "
        "  sum(weight) bag_weight, "
        "  sum(excess) excess "
        "from "
        "  arx_points, "
        "  arx_stat ";
    if (!info.user.access.airlines.empty())
        SQLText += ",aro_airlines ";
    if (!info.user.access.airps.empty())
        SQLText += ",aro_airps ";
    SQLText +=
        "where "
        "  arx_points.point_id = arx_stat.point_id and "
        "  arx_points.scd_out >= :FirstDate AND arx_points.scd_out < :LastDate and "
        "  arx_points.part_key >= :FirstDate and "
        "  arx_stat.part_key >= :FirstDate ";
    if (!info.user.access.airlines.empty())
        SQLText += "AND aro_airlines.airline=arx_points.airline AND aro_airlines.aro_id=:user_id ";
    if (!info.user.access.airps.empty())
        SQLText += "AND aro_airps.airp=arx_points.airp AND aro_airps.aro_id=:user_id ";
    if(ap.size()) {
        SQLText +=
            " and arx_points.airp = :ap ";
        Qry.CreateVariable("ap", otString, ap);
    } else if(ak.size()) {
        SQLText +=
            " and arx_points.airline = :ak ";
        Qry.CreateVariable("ak", otString, ak);
    }
        SQLText +=
        "group by "
        "  arx_points.airp, "
        "  arx_points.airline, "
        "  arx_points.flt_no, "
        "  arx_points.scd_out, "
        "  arx_stat.point_id "
        ") "
        "group by "
        "  airp, "
        "  airline, "
        "  flt_no, "
        "  scd_out, "
        "  point_id, "
        "  places "
        "order by ";
    if(ap.size())
        SQLText +=
            "    airp, "
            "    airline, ";
    else
        SQLText +=
            "    airline, "
            "    airp, ";
    SQLText +=
        "    flt_no, "
        "    scd_out, "
        "    point_id, "
        "    places ";

    ProgTrace(TRACE5, "%s", SQLText.c_str());

    if (!info.user.access.airlines.empty() || !info.user.access.airps.empty())
        Qry.CreateVariable( "user_id", otInteger, info.user.user_id );

    Qry.SQLText = SQLText;
    TReqInfo *reqInfo = TReqInfo::Instance();
    Qry.CreateVariable("FirstDate", otDate, ClientToUTC(NodeAsDateTime("FirstDate", reqNode), reqInfo->desk.tz_region));
    Qry.CreateVariable("LastDate", otDate, ClientToUTC(NodeAsDateTime("LastDate", reqNode), reqInfo->desk.tz_region));
    Qry.Execute();

    if(!Qry.Eof) {
        xmlNodePtr grdNode = NewTextChild(resNode, "grd");
        xmlNodePtr headerNode = NewTextChild(grdNode, "header");
        xmlNodePtr colNode;
        if(ap.size()) {
            colNode = NewTextChild(headerNode, "col", "��� �/�");
            SetProp(colNode, "width", 50);
            SetProp(colNode, "align", taLeftJustify);

            colNode = NewTextChild(headerNode, "col", "��� �/�");
            SetProp(colNode, "width", 50);
            SetProp(colNode, "align", taLeftJustify);
        } else {
            colNode = NewTextChild(headerNode, "col", "��� �/�");
            SetProp(colNode, "width", 50);
            SetProp(colNode, "align", taLeftJustify);

            colNode = NewTextChild(headerNode, "col", "��� �/�");
            SetProp(colNode, "width", 50);
            SetProp(colNode, "align", taLeftJustify);
        }
        colNode = NewTextChild(headerNode, "col", "����� ३�");
        SetProp(colNode, "width", 75);
        SetProp(colNode, "align", taRightJustify);

        colNode = NewTextChild(headerNode, "col", "���");
        SetProp(colNode, "width", 50);
        SetProp(colNode, "align", taLeftJustify);

        colNode = NewTextChild(headerNode, "col", "���ࠢ�����");
        SetProp(colNode, "width", 90);
        SetProp(colNode, "align", taLeftJustify);

        colNode = NewTextChild(headerNode, "col", "���-�� ����.");
        SetProp(colNode, "width", 75);
        SetProp(colNode, "align", taRightJustify);

        colNode = NewTextChild(headerNode, "col", "��");
        SetProp(colNode, "width", 30);
        SetProp(colNode, "align", taRightJustify);

        colNode = NewTextChild(headerNode, "col", "��");
        SetProp(colNode, "width", 30);
        SetProp(colNode, "align", taRightJustify);

        colNode = NewTextChild(headerNode, "col", "��");
        SetProp(colNode, "width", 30);
        SetProp(colNode, "align", taRightJustify);

        colNode = NewTextChild(headerNode, "col", "�/����� (���)");
        SetProp(colNode, "width", 80);
        SetProp(colNode, "align", taRightJustify);

        colNode = NewTextChild(headerNode, "col", "����� (����/���)");
        SetProp(colNode, "width", 100);
        SetProp(colNode, "align", taCenter);

        colNode = NewTextChild(headerNode, "col", "����. (���)");
        SetProp(colNode, "width", 70);
        SetProp(colNode, "align", taRightJustify);

        xmlNodePtr rowsNode = NewTextChild(grdNode, "rows");
        xmlNodePtr rowNode;
        int total_pax_amount = 0;
        int total_adult = 0;
        int total_child = 0;
        int total_baby = 0;
        int total_rk_weight = 0;
        int total_bag_amount = 0;
        int total_bag_weight = 0;
        int total_excess = 0;
        while(!Qry.Eof) {
            string region;
            try
            {
                region = AirpTZRegion(Qry.FieldAsString("airp"));
            }
            catch(UserException &E)
            {
                showErrorMessage((string)E.what()+". ������� ३�� �� �⮡ࠦ�����");
                Qry.Next();
                continue;
            };

            rowNode = NewTextChild(rowsNode, "row");
            if(ap.size()) {
                NewTextChild(rowNode, "col", Qry.FieldAsString("airp"));
                NewTextChild(rowNode, "col", Qry.FieldAsString("airline"));
            } else {
                NewTextChild(rowNode, "col", Qry.FieldAsString("airline"));
                NewTextChild(rowNode, "col", Qry.FieldAsString("airp"));
            }

            int pax_amount = Qry.FieldAsInteger("pax_amount");
            int adult = Qry.FieldAsInteger("adult");
            int child = Qry.FieldAsInteger("child");
            int baby = Qry.FieldAsInteger("baby");
            int rk_weight = Qry.FieldAsInteger("rk_weight");
            int bag_amount = Qry.FieldAsInteger("bag_amount");
            int bag_weight = Qry.FieldAsInteger("bag_weight");
            int excess = Qry.FieldAsInteger("excess");

            total_pax_amount += pax_amount;
            total_adult += adult;
            total_child += child;
            total_baby += baby;
            total_rk_weight += rk_weight;
            total_bag_amount += bag_amount;
            total_bag_weight += bag_weight;
            total_excess += excess;

            NewTextChild(rowNode, "col", Qry.FieldAsInteger("flt_no"));
            NewTextChild(rowNode, "col", DateTimeToStr(
                        UTCToClient(Qry.FieldAsDateTime("scd_out"), region), "dd.mm.yy")
                    );
            NewTextChild(rowNode, "col", Qry.FieldAsString("places"));
            NewTextChild(rowNode, "col", pax_amount);
            NewTextChild(rowNode, "col", adult);
            NewTextChild(rowNode, "col", child);
            NewTextChild(rowNode, "col", baby);
            NewTextChild(rowNode, "col", rk_weight);
            NewTextChild(rowNode, "col", IntToString(bag_amount) + "/" + IntToString(bag_weight));
            NewTextChild(rowNode, "col", excess);
            Qry.Next();
        }
        rowNode = NewTextChild(rowsNode, "row");
        NewTextChild(rowNode, "col", "�⮣�:");
        NewTextChild(rowNode, "col");
        NewTextChild(rowNode, "col");
        NewTextChild(rowNode, "col");
        NewTextChild(rowNode, "col");
        NewTextChild(rowNode, "col", total_pax_amount);
        NewTextChild(rowNode, "col", total_adult);
        NewTextChild(rowNode, "col", total_child);
        NewTextChild(rowNode, "col", total_baby);
        NewTextChild(rowNode, "col", total_rk_weight);
        NewTextChild(rowNode, "col", IntToString(total_bag_amount) + "/" + IntToString(total_bag_weight));
        NewTextChild(rowNode, "col", total_excess);
    } else
        throw UserException("�� ������� �� ����� ����樨.");
    STAT::set_variables(resNode);
}

void RunShortStat(xmlNodePtr reqNode, xmlNodePtr resNode)
{
    TReqInfo &info = *(TReqInfo::Instance());
    get_report_form("ShortStat", resNode);

    string ak = Trim(NodeAsString("ak", reqNode));
    string ap = Trim(NodeAsString("ap", reqNode));

    TQuery Qry(&OraSession);
    string SQLText =
        "select  ";
    if(ap.size())
        SQLText +=
        "    airp,  ";
    else
        SQLText +=
        "    airline,  ";
    SQLText +=
        "    sum(flt_amount) flt_amount, "
        "    sum(pax_amount) pax_amount "
        "from  "
        "( "
        "select  ";
    if(ap.size())
        SQLText +=
        "    points.airp,  ";
    else
        SQLText +=
        "    points.airline,  ";
    SQLText +=
        "    count(distinct stat.point_id) flt_amount, "
        "    sum(adult + child + baby) pax_amount "
        "from  "
        "  points, "
        "  stat ";
    if (!info.user.access.airlines.empty())
        SQLText += ",aro_airlines ";
    if (!info.user.access.airps.empty())
        SQLText += ",aro_airps ";
    SQLText +=
        "where "
        "  points.point_id = stat.point_id and "
        "  points.scd_out >= :FirstDate AND points.scd_out < :LastDate ";
    if (!info.user.access.airlines.empty())
        SQLText += "AND aro_airlines.airline=points.airline AND aro_airlines.aro_id=:user_id ";
    if (!info.user.access.airps.empty())
        SQLText += "AND aro_airps.airp=points.airp AND aro_airps.aro_id=:user_id ";
    if(ap.size()) {
        SQLText +=
            " and points.airp = :ap ";
        Qry.CreateVariable("ap", otString, ap);
    } else if(ak.size()) {
        SQLText +=
            " and points.airline = :ak ";
        Qry.CreateVariable("ak", otString, ak);
    }
        SQLText +=
        "group by  ";
    if(ap.size())
        SQLText +=
        "    points.airp ";
    else
        SQLText +=
        "    points.airline ";
    SQLText +=
        "union "
        "select  ";
    if(ap.size())
        SQLText +=
        "    arx_points.airp,  ";
    else
        SQLText +=
        "    arx_points.airline,  ";
    SQLText +=
        "    count(distinct arx_stat.point_id) flt_amount, "
        "    sum(adult + child + baby) pax_amount "
        "from  "
        "  arx_points, "
        "  arx_stat ";
    if (!info.user.access.airlines.empty())
        SQLText += ",aro_airlines ";
    if (!info.user.access.airps.empty())
        SQLText += ",aro_airps ";
    SQLText +=
        "where "
        "  arx_points.point_id = arx_stat.point_id and "
        "  arx_points.scd_out >= :FirstDate AND arx_points.scd_out < :LastDate and "
        "  arx_points.part_key >= :FirstDate and "
        "  arx_stat.part_key >= :FirstDate ";
    if (!info.user.access.airlines.empty())
        SQLText += "AND aro_airlines.airline=arx_points.airline AND aro_airlines.aro_id=:user_id ";
    if (!info.user.access.airps.empty())
        SQLText += "AND aro_airps.airp=arx_points.airp AND aro_airps.aro_id=:user_id ";
    if(ap.size()) {
        SQLText +=
            " and arx_points.airp = :ap ";
        Qry.CreateVariable("ap", otString, ap);
    } else if(ak.size()) {
        SQLText +=
            " and arx_points.airline = :ak ";
        Qry.CreateVariable("ak", otString, ak);
    }
        SQLText +=
        "group by  ";
    if(ap.size())
        SQLText +=
        "    arx_points.airp ";
    else
        SQLText +=
        "    arx_points.airline ";
        SQLText +=
        ") group by  ";
    if(ap.size())
        SQLText +=
        "    airp ";
    else
        SQLText +=
        "    airline ";
    SQLText +=
        "order by  ";
    if(ap.size())
        SQLText +=
        "    airp ";
    else
        SQLText +=
        "    airline ";

    ProgTrace(TRACE5, "%s", SQLText.c_str());

    if (!info.user.access.airlines.empty() || !info.user.access.airps.empty())
        Qry.CreateVariable( "user_id", otInteger, info.user.user_id );

    Qry.SQLText = SQLText;
    Qry.CreateVariable("FirstDate", otDate, NodeAsDateTime("FirstDate", reqNode));
    Qry.CreateVariable("LastDate", otDate, NodeAsDateTime("LastDate", reqNode));
    Qry.Execute();

    if(!Qry.Eof) {
        xmlNodePtr grdNode = NewTextChild(resNode, "grd");
        xmlNodePtr headerNode = NewTextChild(grdNode, "header");
        xmlNodePtr colNode;
        if(ap.size()) {
            colNode = NewTextChild(headerNode, "col", "��� �/�");
            SetProp(colNode, "width", 50);
            SetProp(colNode, "align", taLeftJustify);
        } else {
            colNode = NewTextChild(headerNode, "col", "��� �/�");
            SetProp(colNode, "width", 50);
            SetProp(colNode, "align", taLeftJustify);
        }
        colNode = NewTextChild(headerNode, "col", "���-�� ३ᮢ");
        SetProp(colNode, "width", 85);
        SetProp(colNode, "align", taRightJustify);

        colNode = NewTextChild(headerNode, "col", "���-�� ����.");
        SetProp(colNode, "width", 85);
        SetProp(colNode, "align", taRightJustify);

        xmlNodePtr rowsNode = NewTextChild(grdNode, "rows");
        xmlNodePtr rowNode;
        int total_flt_amount = 0;
        int total_pax_amount = 0;
        while(!Qry.Eof) {
            rowNode = NewTextChild(rowsNode, "row");
            if(ap.size()) {
                NewTextChild(rowNode, "col", Qry.FieldAsString("airp"));
            } else {
                NewTextChild(rowNode, "col", Qry.FieldAsString("airline"));
            }

            int flt_amount = Qry.FieldAsInteger("flt_amount");
            int pax_amount = Qry.FieldAsInteger("pax_amount");

            total_flt_amount += flt_amount;
            total_pax_amount += pax_amount;

            NewTextChild(rowNode, "col", flt_amount);
            NewTextChild(rowNode, "col", pax_amount);
            Qry.Next();
        }
        rowNode = NewTextChild(rowsNode, "row");
        NewTextChild(rowNode, "col", "�⮣�:");
        NewTextChild(rowNode, "col", total_flt_amount);
        NewTextChild(rowNode, "col", total_pax_amount);
    }
    STAT::set_variables(resNode);
}

void RunDetailStat(xmlNodePtr reqNode, xmlNodePtr resNode)
{
    TReqInfo &info = *(TReqInfo::Instance());
    get_report_form("DetailStat", resNode);

    string ak = Trim(NodeAsString("ak", reqNode));
    string ap = Trim(NodeAsString("ap", reqNode));

    TQuery Qry(&OraSession);
    string SQLText =
        "select "
        "    airp, "
        "    airline, "
        "    sum(flt_amount) flt_amount, "
        "    sum(pax_amount) pax_amount "
        "from "
        "( "
        "select "
        "  points.airp, "
        "  points.airline, "
        "  count(distinct stat.point_id) flt_amount, "
        "  sum(adult + child + baby) pax_amount "
        "from "
        "  points, "
        "  stat ";
    if (!info.user.access.airlines.empty())
        SQLText += ",aro_airlines ";
    if (!info.user.access.airps.empty())
        SQLText += ",aro_airps ";
    SQLText +=
        "where "
        "  points.point_id = stat.point_id and "
        "  points.scd_out >= :FirstDate AND points.scd_out < :LastDate ";
    if (!info.user.access.airlines.empty())
        SQLText += "AND aro_airlines.airline=points.airline AND aro_airlines.aro_id=:user_id ";
    if (!info.user.access.airps.empty())
        SQLText += "AND aro_airps.airp=points.airp AND aro_airps.aro_id=:user_id ";
    if(ap.size()) {
        SQLText +=
            " and points.airp = :ap ";
        Qry.CreateVariable("ap", otString, ap);
    } else if(ak.size()) {
        SQLText +=
            " and points.airline = :ak ";
        Qry.CreateVariable("ak", otString, ak);
    }
        SQLText +=
        "group by "
        "  points.airp, "
        "  points.airline "
        "union "
        "select "
        "  arx_points.airp, "
        "  arx_points.airline, "
        "  count(distinct arx_stat.point_id) flt_amount, "
        "  sum(adult + child + baby) pax_amount "
        "from "
        "  arx_points, "
        "  arx_stat ";
    if (!info.user.access.airlines.empty())
        SQLText += ",aro_airlines ";
    if (!info.user.access.airps.empty())
        SQLText += ",aro_airps ";
    SQLText +=
        "where "
        "  arx_points.point_id = arx_stat.point_id and "
        "  arx_points.scd_out >= :FirstDate AND arx_points.scd_out < :LastDate and "
        "  arx_points.part_key >= :FirstDate and "
        "  arx_stat.part_key >= :FirstDate ";
    if (!info.user.access.airlines.empty())
        SQLText += "AND aro_airlines.airline=arx_points.airline AND aro_airlines.aro_id=:user_id ";
    if (!info.user.access.airps.empty())
        SQLText += "AND aro_airps.airp=arx_points.airp AND aro_airps.aro_id=:user_id ";
    if(ap.size()) {
        SQLText +=
            " and arx_points.airp = :ap ";
        Qry.CreateVariable("ap", otString, ap);
    } else if(ak.size()) {
        SQLText +=
            " and arx_points.airline = :ak ";
        Qry.CreateVariable("ak", otString, ak);
    }
        SQLText +=
        "group by "
        "  arx_points.airp, "
        "  arx_points.airline "
        ") "
        "group by "
        "    airp, "
        "    airline "
        "order by  ";
    if(ap.size())
        SQLText +=
        "    airp, "
        "    airline ";
    else
        SQLText +=
        "    airline, "
        "    airp ";

    Qry.SQLText = SQLText;
    Qry.CreateVariable("FirstDate", otDate, NodeAsDateTime("FirstDate", reqNode));
    Qry.CreateVariable("LastDate", otDate, NodeAsDateTime("LastDate", reqNode));
    if (!info.user.access.airlines.empty() || !info.user.access.airps.empty())
        Qry.CreateVariable( "user_id", otInteger, info.user.user_id );
    Qry.Execute();

    if(!Qry.Eof) {
        xmlNodePtr grdNode = NewTextChild(resNode, "grd");
        xmlNodePtr headerNode = NewTextChild(grdNode, "header");
        xmlNodePtr colNode;
        if(ap.size()) {
            colNode = NewTextChild(headerNode, "col", "��� �/�");
            SetProp(colNode, "width", 50);
            SetProp(colNode, "align", taLeftJustify);

            colNode = NewTextChild(headerNode, "col", "��� �/�");
            SetProp(colNode, "width", 50);
            SetProp(colNode, "align", taLeftJustify);
        } else {
            colNode = NewTextChild(headerNode, "col", "��� �/�");
            SetProp(colNode, "width", 50);
            SetProp(colNode, "align", taLeftJustify);

            colNode = NewTextChild(headerNode, "col", "��� �/�");
            SetProp(colNode, "width", 50);
            SetProp(colNode, "align", taLeftJustify);
        }
        colNode = NewTextChild(headerNode, "col", "���-�� ३ᮢ");
        SetProp(colNode, "width", 85);
        SetProp(colNode, "align", taRightJustify);

        colNode = NewTextChild(headerNode, "col", "���-�� ����.");
        SetProp(colNode, "width", 85);
        SetProp(colNode, "align", taRightJustify);

        xmlNodePtr rowsNode = NewTextChild(grdNode, "rows");
        xmlNodePtr rowNode;
        int total_flt_amount = 0;
        int total_pax_amount = 0;
        while(!Qry.Eof) {
            rowNode = NewTextChild(rowsNode, "row");
            string airp = Qry.FieldAsString("airp");
            string airline = Qry.FieldAsString("airline");
            if(ap.size()) {
                NewTextChild(rowNode, "col", airp);
                NewTextChild(rowNode, "col", airline);
            } else {
                NewTextChild(rowNode, "col", Qry.FieldAsString("airline"));
                NewTextChild(rowNode, "col", Qry.FieldAsString("airp"));
            }

            int flt_amount = Qry.FieldAsInteger("flt_amount");
            int pax_amount = Qry.FieldAsInteger("pax_amount");

            total_flt_amount += flt_amount;
            total_pax_amount += pax_amount;

            NewTextChild(rowNode, "col", flt_amount);
            NewTextChild(rowNode, "col", pax_amount);
            Qry.Next();
        }
        rowNode = NewTextChild(rowsNode, "row");
        NewTextChild(rowNode, "col", "�⮣�:");
        NewTextChild(rowNode, "col");
        NewTextChild(rowNode, "col", total_flt_amount);
        NewTextChild(rowNode, "col", total_pax_amount);
    }
    STAT::set_variables(resNode);
}

void StatInterface::RunStat(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    string name = NodeAsString("stat_mode", reqNode);

    try {
        if(name == "���஡���") RunFullStat(reqNode, resNode);
        else if(name == "����") RunShortStat(reqNode, resNode);
        else if(name == "��⠫���஢�����") RunDetailStat(reqNode, resNode);
        else throw Exception("Unknown stat mode " + name);
    } catch (EOracleError E) {
        if(E.Code == 376)
            throw UserException("� �������� ��������� ��� ���� �� 䠩��� �� �⪫�祭. ������� � ������������");
        else
            throw;
    }

    ProgTrace(TRACE5, "%s", GetXMLDocText(resNode->doc).c_str());
}

void StatInterface::Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{

}
