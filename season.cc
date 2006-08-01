#include "season.h"
#include "xml_unit.h"
#include "exceptions.h"
#define NICKNAME "DENIS"
#include "test.h"
#include "oralib.h"
#include "cache.h"
#include "misc.h"

using namespace EXCEPTIONS;
using namespace std;

void SeasonInterface::Read(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    TPerfTimer tm;
    TQuery *Qry = OraSession.CreateQuery();
    try {
        Qry->SQLText =
            "SELECT a.trip_id,a.move_id,a.num as num,b.first_day,b.last_day,b.days,b.pr_cancel as cancel, "
            "b.tlg,b.reference,r.num as rnum,r.cod,airps.city,r.pr_cancel,r.land+r.delta_in as land, "
            "r.company,r.trip,r.bc,r.takeoff+r.delta_out as takeoff,r.litera,r.triptype,r.f,r.c,r.y,r.unitrip, "
            "r.suffix  "
            "FROM routes r, sched_days b, airps, "
            "(SELECT trip_id,move_id as move_id,MIN(num) as num  "
            "FROM sched_days GROUP BY trip_id, move_id) a  "
            "WHERE a.trip_id=b.trip_id AND r.move_id=b.move_id AND  "
            "a.move_id=b.move_id AND r.cod=airps.cod AND a.num=b.num  "
            "UNION  "
            "SELECT a.trip_id,a.move_id,a.num,a.first_day,a.last_day,a.days,a.pr_cancel, "
            "a.tlg,a.reference,0,NULL,NULL,0,TO_DATE(NULL),NULL,0,NULL,TO_DATE(NULL), "
            "NULL,' ',0,0,0,NULL,NULL  "
            "FROM sched_days a, "
            "(SELECT trip_id,move_id,MIN(num) as num FROM sched_days  "
            "GROUP BY trip_id,move_id) b  "
            "WHERE a.trip_id=b.trip_id AND a.move_id=b.move_id AND a.num!=b.num  "
            "ORDER BY trip_id, move_id,num";
        Qry->Execute();
        ProgTrace(TRACE5, "Executing qry %d", tm.Print());

        if(!Qry->Eof) {
            xmlNodePtr dataNode = NewTextChild(resNode, "data");
            xmlNodePtr rangeListNode = NewTextChild(dataNode, "rangeList");

            tm.Init();
            int idx_trip_id     = Qry->FieldIndex("trip_id");
            int idx_move_id     = Qry->FieldIndex("move_id");
            int idx_num         = Qry->FieldIndex("num");
            int idx_first_day   = Qry->FieldIndex("first_day");
            int idx_last_day    = Qry->FieldIndex("last_day");
            int idx_days        = Qry->FieldIndex("days");
            int idx_cancel      = Qry->FieldIndex("cancel");
            int idx_tlg         = Qry->FieldIndex("tlg");
            int idx_reference   = Qry->FieldIndex("reference");
            int idx_rnum        = Qry->FieldIndex("rnum");
            int idx_cod         = Qry->FieldIndex("cod");
            int idx_city        = Qry->FieldIndex("city");
            int idx_pr_cancel   = Qry->FieldIndex("pr_cancel");
            int idx_land        = Qry->FieldIndex("land");
            int idx_company     = Qry->FieldIndex("company");
            int idx_trip        = Qry->FieldIndex("trip");
            int idx_bc          = Qry->FieldIndex("bc");
            int idx_takeoff     = Qry->FieldIndex("takeoff");
            int idx_litera      = Qry->FieldIndex("litera");
            int idx_triptype    = Qry->FieldIndex("triptype");
            int idx_f           = Qry->FieldIndex("f");
            int idx_c           = Qry->FieldIndex("c");
            int idx_y           = Qry->FieldIndex("y");
            int idx_unitrip     = Qry->FieldIndex("unitrip");
            int idx_suffix      = Qry->FieldIndex("suffix");
            ProgTrace(TRACE5, "Building idx %d", tm.Print());

            tm.Init();
            while(!Qry->Eof) {
                xmlNodePtr rangeNode = NewTextChild(rangeListNode, "range");

                NewTextChild(rangeNode, "trip_id",      Qry->FieldAsString(idx_trip_id));
                NewTextChild(rangeNode, "move_id",      Qry->FieldAsString(idx_move_id));
                NewTextChild(rangeNode, "num",          Qry->FieldAsString(idx_num));
                NewTextChild(rangeNode, "first_day",    Qry->FieldAsString(idx_first_day));
                NewTextChild(rangeNode, "last_day",     Qry->FieldAsString(idx_last_day));
                NewTextChild(rangeNode, "days",         Qry->FieldAsString(idx_days));
                NewTextChild(rangeNode, "cancel",       Qry->FieldAsString(idx_cancel));
                NewTextChild(rangeNode, "tlg",          Qry->FieldAsString(idx_tlg));
                NewTextChild(rangeNode, "reference",    Qry->FieldAsString(idx_reference));
                NewTextChild(rangeNode, "rnum",         Qry->FieldAsString(idx_rnum));
                NewTextChild(rangeNode, "cod",          Qry->FieldAsString(idx_cod));
                NewTextChild(rangeNode, "city",         Qry->FieldAsString(idx_city));
                NewTextChild(rangeNode, "pr_cancel",    Qry->FieldAsString(idx_pr_cancel));
                NewTextChild(rangeNode, "land",         Qry->FieldAsString(idx_land));
                NewTextChild(rangeNode, "company",      Qry->FieldAsString(idx_company));
                NewTextChild(rangeNode, "trip",         Qry->FieldAsString(idx_trip));
                NewTextChild(rangeNode, "bc",           Qry->FieldAsString(idx_bc));
                NewTextChild(rangeNode, "takeoff",      Qry->FieldAsString(idx_takeoff));
                NewTextChild(rangeNode, "litera",       Qry->FieldAsString(idx_litera));
                NewTextChild(rangeNode, "triptype",     Qry->FieldAsString(idx_triptype));
                NewTextChild(rangeNode, "f",            Qry->FieldAsString(idx_f));
                NewTextChild(rangeNode, "c",            Qry->FieldAsString(idx_c));
                NewTextChild(rangeNode, "y",            Qry->FieldAsString(idx_y));
                NewTextChild(rangeNode, "unitrip",      Qry->FieldAsString(idx_unitrip));
                NewTextChild(rangeNode, "suffix",       Qry->FieldAsString(idx_suffix));  

                Qry->Next();
            }
            ProgTrace(TRACE5, "Building resDoc %d", tm.Print());
        }

    } catch(...) {
        OraSession.DeleteQuery(*Qry);
        throw;
    }
    OraSession.DeleteQuery(*Qry);
}

void SeasonInterface::Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{

}
