#include "brd.h"
#include "xml_unit.h"
#include "exceptions.h"
#define NICKNAME "DENIS"
#include "test.h"
#include "oralib.h"
#include "cache.h"

using namespace EXCEPTIONS;
using namespace std;

void BrdInterface::BrdList(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    TQuery *Qry = OraSession.CreateQuery();
    Qry->SQLText =
        "SELECT "
        "    pax_id, "
        "    pax.grp_id, "
        "    point_id, "
        "    pr_brd, "
        "    reg_no, "
        "    surname, "
        "    name, "
        "    pers_type, "
        "    seat_no, "
        "    seats, "
        "    doc_check, "
        "    pax.tid, "
        "    LPAD(seat_no,3,'0')||DECODE(SIGN(1-seats),-1,'+'||TO_CHAR(seats-1),'') AS seat_no_str, "
        "    ckin.get_remarks(pax_id,', ',0) AS remarks, "
        "    NVL(ckin.get_rkAmount(pax_grp.grp_id,NULL,rownum),0) AS rk_amount, "
        "    NVL(ckin.get_rkWeight(pax_grp.grp_id,NULL,rownum),0) AS rk_weight, "
        "    /*  NVL(ckin.get_excess(pax_grp.grp_id,NULL),0) AS excess,*/ NVL(pax_grp.excess,0) AS excess, "
        "    NVL(value_bag.count,0) AS value_bag_count, "
        "    ckin.get_birks(pax_grp.grp_id,NULL) AS tags, "
        "    kassa.pr_payment(pax_grp.grp_id) AS pr_payment  "
        "FROM "
        "    pax_grp, "
        "    pax, "
        "    ( "
        "     SELECT "
        "        grp_id, "
        "        COUNT(*) AS count "
        "     FROM "
        "        value_bag "
        "     GROUP BY "
        "        grp_id "
        "    ) value_bag "
        "WHERE "
        "    pax_grp.grp_id=pax.grp_id AND "
        "    pax_grp.grp_id=value_bag.grp_id(+)  "
        "    AND point_id= :point_id AND pr_brd= :pr_brd "
        "--���. �᫮���     "
        "ORDER BY reg_no ";
    TParams1 SQLParams;
    SQLParams.getParams(GetNode("sqlparams", reqNode));
    SQLParams.setSQL(Qry);
    Qry->Execute();
    xmlNodePtr dataNode = NewTextChild(resNode, "data");
    xmlNodePtr listNode = NewTextChild(dataNode, "brd_list");
    while(!Qry->Eof) {
        xmlNodePtr paxNode = NewTextChild(listNode, "pax");
        NewTextChild(paxNode, "pax_id",            Qry->FieldAsString("pax_id"));
        NewTextChild(paxNode, "grp_id",            Qry->FieldAsString("grp_id"));
        NewTextChild(paxNode, "point_id",          Qry->FieldAsString("point_id"));
        NewTextChild(paxNode, "pr_brd",            Qry->FieldAsString("pr_brd"));
        NewTextChild(paxNode, "reg_no",            Qry->FieldAsString("reg_no"));
        NewTextChild(paxNode, "surname",           Qry->FieldAsString("surname"));
        NewTextChild(paxNode, "name",              Qry->FieldAsString("name"));
        NewTextChild(paxNode, "pers_type",         Qry->FieldAsString("pers_type"));
        NewTextChild(paxNode, "seat_no",           Qry->FieldAsString("seat_no"));
        NewTextChild(paxNode, "seats",             Qry->FieldAsString("seats"));
        NewTextChild(paxNode, "doc_check",         Qry->FieldAsString("doc_check"));
        NewTextChild(paxNode, "tid",               Qry->FieldAsString("tid"));
        NewTextChild(paxNode, "seat_no_str",       Qry->FieldAsString("seat_no_str"));
        NewTextChild(paxNode, "remarks",           Qry->FieldAsString("remarks"));
        NewTextChild(paxNode, "rk_amount",         Qry->FieldAsString("rk_amount"));
        NewTextChild(paxNode, "rk_weight",         Qry->FieldAsString("rk_weight"));
        NewTextChild(paxNode, "excess",            Qry->FieldAsString("excess"));
        NewTextChild(paxNode, "value_bag_count",   Qry->FieldAsString("value_bag_count"));
        NewTextChild(paxNode, "tags",              Qry->FieldAsString("tags"));
        NewTextChild(paxNode, "pr_payment",        Qry->FieldAsString("pr_payment"));
        Qry->Next();
    }
    OraSession.DeleteQuery(*Qry);
}

void BrdInterface::Trip(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    TQuery *Qry = OraSession.CreateQuery();
    Qry->SQLText =
        "SELECT "
        "    trips.trip_id, "
        "    trips.bc, "
        "    SUBSTR(ckin.get_classes(trips.trip_id),1,255) AS classes, "
        "    SUBSTR(ckin.get_places(trips.trip_id),1,255) AS places, "
        "    TO_CHAR(gtimer.get_stage_time(trips.trip_id,:brd_close_stage_id),'HH24:MI') AS brd_to, "
        "    TO_CHAR(NVL(NVL(trips.act,trips.est),trips.scd),'HH24:MI') AS takeoff, "
        "    trips.triptype, "
        "    trips.litera, "
        "    stages.brd_stage, "
        "    trips.remark "
        "FROM "
        "    trips, "
        "    trip_stations, "
        "    ( "
        "     SELECT "
        "        gtimer.get_stage( :trip_id, :brd_stage_type ) as brd_stage "
        "     FROM dual "
        "    ) stages "
        "WHERE "
        "    trip_stations.trip_id= :trip_id AND "
        "    trips.act IS NULL AND "
        "    trip_stations.name= :station AND "
        "    trip_stations.work_mode='�' AND "
        "    trips.trip_id= :trip_id AND "
        "    trips.status=0 AND  "
        "    stages.brd_stage = :brd_open_stage_id ";
    TParams1 SQLParams;
    SQLParams.getParams(GetNode("sqlparams", reqNode));
    SQLParams.setSQL(Qry);
    Qry->Execute();
    xmlNodePtr dataNode = NewTextChild(resNode, "data");
    if(!Qry->Eof) {
        xmlNodePtr fltNode = NewTextChild(dataNode, "flt");
        NewTextChild(fltNode, "trip_id", Qry->FieldAsString("trip_id"));
        NewTextChild(fltNode, "bc", Qry->FieldAsString("bc"));
        NewTextChild(fltNode, "classes", Qry->FieldAsString("classes"));
        NewTextChild(fltNode, "places", Qry->FieldAsString("places"));
        NewTextChild(fltNode, "brd_to", Qry->FieldAsString("brd_to"));
        NewTextChild(fltNode, "takeoff", Qry->FieldAsString("takeoff"));
        NewTextChild(fltNode, "triptype", Qry->FieldAsString("triptype"));
        NewTextChild(fltNode, "litera", Qry->FieldAsString("litera"));
        NewTextChild(fltNode, "brd_stage", Qry->FieldAsString("brd_stage"));
        NewTextChild(fltNode, "remark", Qry->FieldAsString("remark"));
    }
    Qry->Clear();
    Qry->SQLText =
        "SELECT "
        "    COUNT(*) AS reg, "
        "    NVL(SUM(DECODE(pr_brd,0,0,1)),0) AS brd "
        "FROM "
        "    pax_grp, "
        "    pax "
        "WHERE "
        "    pax_grp.grp_id=pax.grp_id AND "
        "    point_id=:trip_id AND "
        "    pr_brd IS NOT NULL ";
    Qry->DeclareVariable("trip_id", otInteger);
    Qry->SetVariable("trip_id", NodeAsInteger("sqlparams/trip_id", reqNode));
    Qry->Execute();
    if(!Qry->Eof) {
        xmlNodePtr countersNode = NewTextChild(dataNode, "counters");
        NewTextChild(countersNode, "reg", Qry->FieldAsString("reg"));
        NewTextChild(countersNode, "brd", Qry->FieldAsString("brd"));
    }
    OraSession.DeleteQuery(*Qry);
}

void BrdInterface::Trips(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    xmlNodePtr dataNode = NewTextChild(resNode, "data");

    TQuery *Qry = OraSession.CreateQuery();
    Qry->SQLText =
        "SELECT "
        "    trips.trip_id, "
        "    trip|| "
        "    DECODE(TRUNC(SYSDATE),TRUNC(NVL(act,NVL(est,scd))),'', "
        "        TO_CHAR(NVL(act,NVL(est,scd)),'/DD'))|| "
        "    DECODE(TRUNC(NVL(act,NVL(est,scd))),TRUNC(scd),'', "
        "        TO_CHAR(scd,'(DD)')) AS str "
        "FROM "
        "    trips, "
        "    trip_stations "
        "WHERE "
        "    trips.trip_id=trip_stations.trip_id AND "
        "    trips.act IS NULL AND "
        "    trips.status=0 AND  "
        "    trip_stations.name= :station AND "
        "    trip_stations.work_mode='�' AND "
        "    gtimer.is_final_stage(  trips.trip_id, :brd_stage_type, :brd_open_stage_id) <> 0 "
        "ORDER BY "
        "    NVL(act,NVL(est,scd)) ";
    TParams1 SQLParams;
    SQLParams.getParams(GetNode("sqlparams", reqNode));
    SQLParams.setSQL(Qry);
    Qry->Execute();

    ProgTrace(TRACE5, "rows: %d", Qry->RowCount());

    xmlNodePtr tripsNode = NewTextChild(dataNode, "trips");
    while(!Qry->Eof) {
        xmlNodePtr tripNode = NewTextChild(tripsNode, "trip");
        NewTextChild(tripNode, "trip_id", Qry->FieldAsString(0));
        NewTextChild(tripNode, "str", Qry->FieldAsString(1));
        Qry->Next();
    }
    OraSession.DeleteQuery(*Qry);
}

void BrdInterface::Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{

}
