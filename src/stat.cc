#include "stat.h"
#define NICKNAME "DENIS"
#include "test.h"
#include "oralib.h"
#include "cache.h"
#include "xml_unit.h"
#include "exceptions.h"
#include "stl_utils.h"
#include "docs.h"
#include <fstream>

using namespace std;
using namespace EXCEPTIONS;
using namespace BASIC;

enum TScreenState {None,Stat,Pax,Log,DepStat,BagTagStat,PaxList,FltLog,SystemLog,PaxSrc,TlgArch};

const int depends_len = 3;
struct TCBox {
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
                "Flt",

                "SELECT "
                "    points.point_id, "
                "    points.airp, "
                "    system.AirpTZRegion(points.airp, 0) AS tz_region, "
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
                "    system.AirpTZRegion(arx_points.airp, 0) AS tz_region, "
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
                "Flt",

                "SELECT trips.trip "
                "FROM astra.trips "
                "WHERE "
                "      trips.scd >= :FirstDate AND trips.scd < :LastDate "
                "UNION "
                "SELECT trips.trip "
                "FROM arx.trips "
                "WHERE "
                "      trips.part_key >= :FirstDate AND trips.part_key < :LastDate "
                "ORDER BY trip  "
            }
        }
    },
    {
        FltLog,
        {
            {
                "Flt",

                "SELECT trips.trip  "
                "FROM astra.events, "
                "     (SELECT trip, trip_id FROM astra.trips "
                "      WHERE trips.scd>= :FirstDate AND "
                "        trips.scd< :LastDate "
                "      ORDER BY trip_id) trips "
                "WHERE    "
                "    events.type in ( "
                "    :evtFlt, "
                "    :evtGraph, "
                "    :evtTlg, "
                "    :evtPax, "
                "    :evtPay "
                "    ) and "
                "    events.id1 = trips.trip_id "
                "UNION    "
                "SELECT trips.trip    "
                "FROM arx.events, "
                "     (SELECT trip, trip_id, part_key FROM arx.trips "
                "      WHERE trips.part_key>= :FirstDate AND "
                "        trips.part_key< :LastDate "
                "      ORDER BY trip_id) trips "
                "WHERE    "
                "    events.part_key=trips.part_key AND  "
                "    events.type in ( "
                "    :evtFlt, "
                "    :evtGraph, "
                "    :evtTlg, "
                "    :evtPax, "
                "    :evtPay "
                "    ) and "
                "    events.id1 = trips.trip_id "
                "ORDER BY trip "
            }
        }
    },
    {
        SystemLog,
        {
            {
                "Agent",

                "select 'Система' agent from dual where "
                "  (:station is null or :station = 'Система') and "
                "  (:module is null or :module = 'Система') "
                "union "
                "select ev_user agent from astra.events, astra.screen "
                "where "
                "    time >= :FirstDate and "
                "    time < :LastDate and "
                "    (:station is null or station = :station) and "
                "    events.screen = screen.exe(+) and "
                "    (:module is null or nvl(screen.name, events.screen) = :module) "
                "union "
                "select ev_user agent from arx.events, astra.screen "
                "where "
                "    time >= :FirstDate and "
                "    time < :LastDate and "
                "    (:station is null or station = :station) and "
                "    events.screen = screen.exe(+) and "
                "    (:module is null or nvl(screen.name, events.screen) = :module) "
                "order by "
                "    agent ",

                {"Station", "Module"}
            },
            {
                "Station",

                "select 'Система' station from dual where "
                "  (:agent is null or :agent = 'Система') and "
                "  (:module is null or :module = 'Система') "
                "union "
                "select station from astra.events, astra.screen "
                "where "
                "    time >= :FirstDate and "
                "    time < :LastDate and "
                "    station is not null and "
                "    (:agent is null or ev_user = :agent) and "
                "    events.screen = screen.exe(+) and "
                "    (:module is null or nvl(screen.name, events.screen) = :module) "
                "union "
                "select station from arx.events, astra.screen "
                "where "
                "    time >= :FirstDate and "
                "    time < :LastDate and "
                "    station is not null and "
                "    (:agent is null or ev_user = :agent) and "
                "    events.screen = screen.exe(+) and "
                "    (:module is null or nvl(screen.name, events.screen) = :module) "
                "order by "
                "    station ",

                {"Agent", "Module"}
            },
            {
                "Module",

                "select 'Система' module from dual where "
                "  (:agent is null or :agent = 'Система') and "
                "  (:station is null or :station = 'Система') "
                "union " 
                "select nvl(screen.name, events.screen) module from astra.events, astra.screen where "
                "    events.time >= :FirstDate and "
                "    events.time < :LastDate and "
                "    (:station is null or station = :station) and "
                "    (:agent is null or ev_user = :agent) and "
                "    events.screen is not null and "
                "    events.screen = screen.exe(+) "
                "union "
                "select nvl(screen.name, events.screen) module from arx.events, astra.screen where "
                "    events.part_key >= :FirstDate and "
                "    events.part_key < :LastDate and "
                "    (:station is null or station = :station) and "
                "    (:agent is null or ev_user = :agent) and "
                "    events.screen is not null and "
                "    events.screen = screen.exe(+) "
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
    Qry.SQLText = cbox_data->qry;
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
    Qry.Execute();
    xmlNodePtr cboxNode = NewTextChild(resNode, "cbox");
    if(cbox_data->cbox == "Flt") {
        TDateTime scd_out,real_out,desk_time;
        modf(reqInfo->desk.time,&desk_time);
        while(!Qry.Eof) {
            modf(UTCToClient(Qry.FieldAsDateTime("scd_out"),Qry.FieldAsString("tz_region")),&scd_out);
            modf(UTCToClient(Qry.FieldAsDateTime("real_out"),Qry.FieldAsString("tz_region")),&real_out);
            ostringstream trip;
            trip << Qry.FieldAsString("airline")
                << Qry.FieldAsInteger("flt_no")
                << Qry.FieldAsString("suffix");

            if (desk_time!=real_out)
            {
                if (DateTimeToStr(desk_time,"mm")==DateTimeToStr(real_out,"mm"))
                    trip << "/" << DateTimeToStr(real_out,"dd");
                else
                    trip << "/" << DateTimeToStr(real_out,"dd.mm");
            };
            if (scd_out!=real_out)
                trip << "(" << DateTimeToStr(scd_out,"dd") << ")";

            NewTextChild(cboxNode, "f", trip.str());
            Qry.Next();
        }
    } else
        while(!Qry.Eof) {
            NewTextChild(cboxNode, "f", Qry.FieldAsString(0));
            Qry.Next();
        }
}

void StatInterface::PaxLog(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    string tag = (char *)reqNode->name;
    char *qry = NULL;
    if(tag == "LogRun")
        qry =
            "SELECT msg, time, id1 AS point_id, null as screen, id2 AS reg_no, id3 AS grp_id, "
            "       ev_user, station, ev_order "
            "FROM astra.events "
            "WHERE type IN (:evtPax,:evtPay) AND "
            "      id1=:trip_id AND "
            "      (id2 IS NULL OR id2=:reg_no) AND "
            "      (id3 IS NULL OR id3=:grp_id) "
            "UNION "
            "SELECT msg, time, id1 AS point_id, null as screen, id2 AS reg_no, id3 AS grp_id, "
            "       ev_user, station, ev_order "
            "FROM arx.events "
            "WHERE part_key=:part_key AND "
            "      type IN (:evtPax,:evtPay) AND "
            "      id1=:trip_id AND "
            "      (id2 IS NULL OR id2=:reg_no) AND "
            "      (id3 IS NULL OR id3=:grp_id) ";
    else if(tag == "FltLogRun")
        qry =
            "SELECT msg, time, id1 AS point_id, "
            "       nvl(screen.name, events.screen) screen, "
            "       DECODE(type,:evtPax,id2,:evtPay,id2,NULL) AS reg_no, "
            "       DECODE(type,:evtPax,id3,:evtPay,id3,NULL) AS grp_id, "
            "       ev_user, station, ev_order "
            "FROM astra.events, astra.screen, "
            "     (SELECT trip_id FROM astra.trips "
            "      WHERE trips.scd>= :FirstDate AND trips.scd< :LastDate AND trips.trip= :trip "
            "      ORDER BY trip_id) trips "
            "WHERE events.type IN (:evtFlt,:evtGraph,:evtPax,:evtPay,:evtTlg) AND "
            "      events.screen = screen.exe(+) and "
            "      events.id1=trips.trip_id "
            "UNION "
            "SELECT msg, time, id1 AS point_id, "
            "       nvl(screen.name, events.screen) screen, "
            "       DECODE(type,:evtPax,id2,:evtPay,id2,NULL) AS reg_no, "
            "       DECODE(type,:evtPax,id3,:evtPay,id3,NULL) AS grp_id, "
            "       ev_user, station, ev_order "
            "FROM arx.events, astra.screen, "
            "     (SELECT part_key,trip_id FROM arx.trips "
            "      WHERE trips.part_key>= :FirstDate AND trips.part_key< :LastDate AND trips.trip= :trip "
            "      ORDER BY part_key,trip_id) trips "
            "WHERE events.part_key=trips.part_key AND "
            "      events.screen = screen.exe(+) and "
            "      events.type IN (:evtFlt,:evtGraph,:evtPax,:evtPay,:evtTlg) AND "
            "      events.id1=trips.trip_id ";
    else if(tag == "SystemLogRun")
        qry =
            "SELECT msg, time, id1 AS point_id, "
            "  nvl(screen.name, events.screen) screen, "
            "  DECODE(type,:evtPax,id2,id2,NULL) AS reg_no, "
            "  DECODE(type,:evtPax,id3,id3,NULL) AS grp_id, "
            "  ev_user, station, ev_order "
            "FROM astra.events, astra.screen "
            "WHERE "
            "  events.time >= :FirstDate and "
            "  events.time < :LastDate and "
            "  events.screen = screen.exe(+) and "
            "  (:agent is null or nvl(ev_user, 'Система') = :agent) and "
            "  (:module is null or nvl(screen.name, 'Система') = :module) and "
            "  (:station is null or nvl(station, 'Система') = :station) and "
            "  events.type IN ( "
            "    :evtFlt, "
            "    :evtPax, "
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
            "  nvl(screen.name, events.screen) screen, "
            "  DECODE(type,:evtPax,id2,NULL) AS reg_no, "
            "  DECODE(type,:evtPax,id3,NULL) AS grp_id, "
            "  ev_user, station, ev_order "
            "FROM arx.events, astra.screen "
            "WHERE "
            "  events.part_key >= :FirstDate and "
            "  events.part_key < :LastDate and "
            "  events.screen = screen.exe(+) and "
            "  (:agent is null or nvl(ev_user, 'Система') = :agent) and "
            "  (:module is null or nvl(screen.name, 'Система') = :module) and "
            "  (:station is null or nvl(station, 'Система') = :station) and "
            "  events.type IN ( "
            "    :evtFlt, "
            "    :evtPax, "
            "    :evtGraph, "
            "    :evtTlg, "
            "    :evtComp, "
            "    :evtAccess, "
            "    :evtSystem, "
            "    :evtCodif, "
            "    :evtPeriod "
            "  ) ";
    else
        throw Exception((string)"PaxLog: unknown tag " + tag);
    TQuery Qry(&OraSession);        
    Qry.SQLText = qry;
    TParams1 SQLParams;
    SQLParams.getParams(GetNode("sqlparams", reqNode));
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
    xmlNodePtr PaxLogNode = NewTextChild(dataNode, "PaxLog");
    while(!Qry.Eof) {
        xmlNodePtr rowNode = NewTextChild(PaxLogNode, "row");

        NewTextChild(rowNode, "trip_id", Qry.FieldAsInteger("point_id"));
        NewTextChild(rowNode, "idf", Qry.FieldAsString("ev_user"));
        NewTextChild(rowNode, "station", Qry.FieldAsString("station"));
        NewTextChild(rowNode, "time", DateTimeToStr(Qry.FieldAsDateTime("time")));
        NewTextChild(rowNode, "grp_id", Qry.FieldAsInteger("grp_id"));
        NewTextChild(rowNode, "n_reg", Qry.FieldAsInteger("reg_no"));
        NewTextChild(rowNode, "txt", Qry.FieldAsString("msg"));
        NewTextChild(rowNode, "ev_order", Qry.FieldAsInteger("ev_order"));
        NewTextChild(rowNode, "module", Qry.FieldAsString("screen"));

        Qry.Next();
    }
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
        "  astra.ckin.get_bagAmount(pax_grp.grp_id,pax.reg_no,rownum) AS bag_amount, "
        "  astra.ckin.get_bagWeight(pax_grp.grp_id,pax.reg_no,rownum) AS bag_weight, "
        "  astra.ckin.get_rkWeight(pax_grp.grp_id,pax.reg_no,rownum) AS rk_weight, "
        "  astra.ckin.get_birks(pax_grp.grp_id,pax.reg_no,0) AS tags, "
        "  astra.ckin.get_excess(pax_grp.grp_id,pax.reg_no) AS excess, "
        "  DECODE(pax.refuse,NULL,DECODE(pax_grp.pr_wl,0,DECODE(pax.pr_brd,0,'Зарег.','Посажен'),'ЛО'),'Аннул.') AS status, "
        "  pax_grp.class, "
        "  LPAD(pax.seat_no,3,'0')|| "
        "           DECODE(SIGN(1-pax.seats),-1,'+'||TO_CHAR(pax.seats-1),'') AS seat_no, "
        "  pax_grp.hall AS hall, "
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
        "  arx.ckin.get_bagAmount(pax_grp.part_key,pax_grp.grp_id,pax.reg_no,rownum) AS bag_amount, "
        "  arx.ckin.get_bagWeight(pax_grp.part_key,pax_grp.grp_id,pax.reg_no,rownum) AS bag_weight, "
        "  arx.ckin.get_rkWeight(pax_grp.part_key,pax_grp.grp_id,pax.reg_no,rownum) AS rk_weight, "
        "  arx.ckin.get_birks(pax_grp.part_key,pax_grp.grp_id,pax.reg_no) AS tags, "
        "  arx.ckin.get_excess(pax_grp.part_key,pax_grp.grp_id,pax.reg_no) AS excess, "
        "  DECODE(pax.refuse,NULL,DECODE(pax_grp.pr_wl,0,DECODE(pax.pr_brd,0,'Зарег.','Посажен'),'ЛО'),'Аннул.') AS status, "
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
    }
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
        "  'Всего' AS hall, "
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

void set_variables(xmlNodePtr resNode)
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
    string form;
    get_report_form("FullStat", form);
    NewTextChild(resNode, "form", form);

    string ak = Trim(NodeAsString("ak", reqNode));
    string ap = Trim(NodeAsString("ap", reqNode));

    TQuery Qry(&OraSession);        
    string SQLText = 
        "select "
        "  points.airp, "
        "  points.airline, "
        "  points.flt_no, "
        "  points.scd_out, "
        "  system.AirpTzRegion(airp) AS tz_region, "
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
        "  stat "
        "where "
        "  points.point_id = stat.point_id and "
        "  points.scd_out >= :FirstDate AND points.scd_out < :LastDate ";
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
        "  system.AirpTzRegion(airp) AS tz_region, "
        "  arx_stat.point_id, "
        "  substr(arch.get_airps(arx_stat.point_id),1,50) places, "
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
        "  arx_stat "
        "where "
        "  arx_points.point_id = arx_stat.point_id and "
        "  arx_points.scd_out >= :FirstDate AND arx_points.scd_out < :LastDate and "
        "  arx_stat.part_key >= :FirstDate ";
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
            colNode = NewTextChild(headerNode, "col", "Код а/п");
            SetProp(colNode, "width", 50);
            SetProp(colNode, "align", 0);

            colNode = NewTextChild(headerNode, "col", "Код а/к");
            SetProp(colNode, "width", 50);
            SetProp(colNode, "align", 0);
        } else {
            colNode = NewTextChild(headerNode, "col", "Код а/к");
            SetProp(colNode, "width", 50);
            SetProp(colNode, "align", 0);

            colNode = NewTextChild(headerNode, "col", "Код а/п");
            SetProp(colNode, "width", 50);
            SetProp(colNode, "align", 0);
        }
        colNode = NewTextChild(headerNode, "col", "Номер рейса");
        SetProp(colNode, "width", 75);
        SetProp(colNode, "align", 1);
        
        colNode = NewTextChild(headerNode, "col", "Дата");
        SetProp(colNode, "width", 50);
        SetProp(colNode, "align", 0);

        colNode = NewTextChild(headerNode, "col", "Направление");
        SetProp(colNode, "width", 90);
        SetProp(colNode, "align", 0);

        colNode = NewTextChild(headerNode, "col", "Кол-во пасс.");
        SetProp(colNode, "width", 75);
        SetProp(colNode, "align", 1);

        colNode = NewTextChild(headerNode, "col", "ВЗ");
        SetProp(colNode, "width", 30);
        SetProp(colNode, "align", 1);

        colNode = NewTextChild(headerNode, "col", "РБ");
        SetProp(colNode, "width", 30);
        SetProp(colNode, "align", 1);

        colNode = NewTextChild(headerNode, "col", "РМ");
        SetProp(colNode, "width", 30);
        SetProp(colNode, "align", 1);

        colNode = NewTextChild(headerNode, "col", "Р/кладь (вес)");
        SetProp(colNode, "width", 80);
        SetProp(colNode, "align", 1);

        colNode = NewTextChild(headerNode, "col", "Багаж (мест/вес)");
        SetProp(colNode, "width", 100);
        SetProp(colNode, "align", 1);

        colNode = NewTextChild(headerNode, "col", "Платн. (вес)");
        SetProp(colNode, "width", 70);
        SetProp(colNode, "align", 1);

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
                        UTCToClient(Qry.FieldAsDateTime("scd_out"), Qry.FieldAsString("tz_region")), "dd.mm.yy")
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
        NewTextChild(rowNode, "col", "Итого:");
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
    }
    set_variables(resNode);
}

void RunShortStat(xmlNodePtr reqNode, xmlNodePtr resNode)
{
    string form;
    get_report_form("ShortStat", form);
    NewTextChild(resNode, "form", form);

    string ak = Trim(NodeAsString("ak", reqNode));
    string ap = Trim(NodeAsString("ap", reqNode));

    TQuery Qry(&OraSession);        
    string SQLText = 
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
        "  stat "
        "where "
        "  points.point_id = stat.point_id and "
        "  points.scd_out >= :FirstDate AND points.scd_out < :LastDate ";
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
        "  arx_stat "
        "where "
        "  arx_points.point_id = arx_stat.point_id and "
        "  arx_points.scd_out >= :FirstDate AND arx_points.scd_out < :LastDate and "
        "  arx_stat.part_key >= :FirstDate ";
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
        "order by  ";
    if(ap.size())
        SQLText += 
        "    airp ";
    else
        SQLText += 
        "    airline ";

    ProgTrace(TRACE5, "%s", SQLText.c_str());

    Qry.SQLText = SQLText;
    Qry.CreateVariable("FirstDate", otDate, NodeAsDateTime("FirstDate", reqNode));
    Qry.CreateVariable("LastDate", otDate, NodeAsDateTime("LastDate", reqNode));
    Qry.Execute();

    if(!Qry.Eof) {
        xmlNodePtr grdNode = NewTextChild(resNode, "grd");
        xmlNodePtr headerNode = NewTextChild(grdNode, "header");
        xmlNodePtr colNode;
        if(ap.size()) {
            colNode = NewTextChild(headerNode, "col", "Код а/п");
            SetProp(colNode, "width", 50);
            SetProp(colNode, "align", 0);
        } else {
            colNode = NewTextChild(headerNode, "col", "Код а/к");
            SetProp(colNode, "width", 50);
            SetProp(colNode, "align", 0);
        }
        colNode = NewTextChild(headerNode, "col", "Кол-во рейсов");
        SetProp(colNode, "width", 85);
        SetProp(colNode, "align", 1);

        colNode = NewTextChild(headerNode, "col", "Кол-во пасс.");
        SetProp(colNode, "width", 85);
        SetProp(colNode, "align", 1);

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
        NewTextChild(rowNode, "col", "Итого:");
        NewTextChild(rowNode, "col", total_flt_amount);
        NewTextChild(rowNode, "col", total_pax_amount);
    }
    set_variables(resNode);
}

void RunDetailStat(xmlNodePtr reqNode, xmlNodePtr resNode)
{
    string form;
    get_report_form("DetailStat", form);
    NewTextChild(resNode, "form", form);

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
        "  stat "
        "where "
        "  points.point_id = stat.point_id and "
        "  points.scd_out >= :FirstDate AND points.scd_out < :LastDate ";
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
        "  arx_stat "
        "where "
        "  arx_points.point_id = arx_stat.point_id and "
        "  arx_points.scd_out >= :FirstDate AND arx_points.scd_out < :LastDate and "
        "  arx_stat.part_key >= :FirstDate ";
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
    Qry.Execute();

    if(!Qry.Eof) {
        xmlNodePtr grdNode = NewTextChild(resNode, "grd");
        xmlNodePtr headerNode = NewTextChild(grdNode, "header");
        xmlNodePtr colNode;
        if(ap.size()) {
            colNode = NewTextChild(headerNode, "col", "Код а/п");
            SetProp(colNode, "width", 50);
            SetProp(colNode, "align", 0);

            colNode = NewTextChild(headerNode, "col", "Код а/к");
            SetProp(colNode, "width", 50);
            SetProp(colNode, "align", 0);
        } else {
            colNode = NewTextChild(headerNode, "col", "Код а/к");
            SetProp(colNode, "width", 50);
            SetProp(colNode, "align", 0);

            colNode = NewTextChild(headerNode, "col", "Код а/п");
            SetProp(colNode, "width", 50);
            SetProp(colNode, "align", 0);
        }
        colNode = NewTextChild(headerNode, "col", "Кол-во рейсов");
        SetProp(colNode, "width", 85);
        SetProp(colNode, "align", 1);

        colNode = NewTextChild(headerNode, "col", "Кол-во пасс.");
        SetProp(colNode, "width", 85);
        SetProp(colNode, "align", 1);

        xmlNodePtr rowsNode = NewTextChild(grdNode, "rows");
        xmlNodePtr rowNode;
        int total_flt_amount = 0;
        int total_pax_amount = 0;
        TAirps airps;
        TAirlines airlines;
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
        NewTextChild(rowNode, "col", "Итого:");
        NewTextChild(rowNode, "col");
        NewTextChild(rowNode, "col", total_flt_amount);
        NewTextChild(rowNode, "col", total_pax_amount);
    }
    set_variables(resNode);
}

void StatInterface::RunStat(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    string name = NodeAsString("stat_mode", reqNode);

    if(name == "Подробная") RunFullStat(reqNode, resNode);
    else if(name == "Общая") RunShortStat(reqNode, resNode);
    else if(name == "Детализированная") RunDetailStat(reqNode, resNode);
    else throw Exception("Unknown stat mode " + name);

    ProgTrace(TRACE5, "%s", GetXMLDocText(resNode->doc).c_str());
}

void StatInterface::Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{

}
