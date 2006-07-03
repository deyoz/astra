#include "brd.h"
#include "xml_unit.h"
#include "exceptions.h"
#define NICKNAME "DENIS"
#include "test.h"
#include "oralib.h"
#include "cache.h"

using namespace EXCEPTIONS;
using namespace std;

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
        "    trip_stations.work_mode='П' AND "
        "    gtimer.is_final_stage(  trips.trip_id, :brd_stage_type, :brd_open_stage_id) <> 0 "
        "ORDER BY "
        "    NVL(act,NVL(est,scd)) ";
    TParams1 SQLParams;
    SQLParams.getParams(GetNode("sqlparams", reqNode));
    SQLParams.setSQL(Qry);
    Qry->Execute();

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
