#include "stat.h"
#define NICKNAME "DENIS"
#include "test.h"
#include "oralib.h"
#include "cache.h"
#include "xml_unit.h"
#include "exceptions.h"
#include "stl_utils.h"

using namespace EXCEPTIONS;
using namespace BASIC;


const char *StatQry =
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


void StatInterface::DepStatRun(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    ProgTrace(TRACE5, "DepStatRun");
    TQuery Qry(&OraSession);        
    Qry.SQLText = StatQry;
    TParams1 SQLParams;

    xmlNodePtr paramsNode = GetNode("sqlparams", reqNode);
    TDateTime FirstDate = NodeAsDateTime("FirstDate", paramsNode);
    TDateTime LastDate = NodeAsDateTime("LastDate", paramsNode);
    SQLParams.getParams(paramsNode);

    SQLParams.setSQL(&Qry);
    Qry.Execute();
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

void StatInterface::Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{

}
