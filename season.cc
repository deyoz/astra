#include "season.h"
#include "xml_unit.h"
#include "exceptions.h"
#define NICKNAME "DENIS"
#include "test.h"
#include "oralib.h"
#include "cache.h"
#include "misc.h"

using namespace BASIC;
using namespace EXCEPTIONS;
using namespace std;

int GetNextMove_id()
{
    TQuery Qry(&OraSession);        
    Qry.SQLText = "SELECT routes_move_id.nextval AS move_id FROM dual";
    Qry.Execute();
    return Qry.FieldAsInteger(0);
}

int GetNextTrip_id()
{
    TQuery Qry(&OraSession);        
    Qry.SQLText = "SELECT routes_trip_id.nextval AS trip_id FROM dual";
    Qry.Execute();
    return Qry.FieldAsInteger(0);
}

void SeasonInterface::Write(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{

    TQuery Qry(&OraSession);        
    int trip_id = 0;
    if(NodeIsNULL("trip_id", reqNode)) {
        trip_id = GetNextTrip_id();
    } else {
        Qry.SQLText =
            "BEGIN "
            "DELETE routes WHERE move_id IN (SELECT move_id FROM sched_days WHERE trip_id=:trip_id );"
            "DELETE sched_days WHERE trip_id=:trip_id; END; ";
        Qry.DeclareVariable("TRIP_ID", otInteger);
        trip_id = NodeAsInteger("trip_id", reqNode);
        Qry.SetVariable("TRIP_ID", trip_id);
        Qry.Execute();
    }


    xmlNodePtr curNode = NodeAsNode("SubRangeList/SubRange", reqNode);

    xmlNodePtr dataNode = NewTextChild(resNode, "data");
    NewTextChild(dataNode, "trip_id", trip_id);
    xmlNodePtr NewMovesNode = NewTextChild(dataNode, "NewMoves");
    while(curNode) {
        Qry.Clear();
        Qry.SQLText =
            "INSERT INTO sched_days(trip_id,move_id,num,first_day,last_day,days,pr_cancel,tlg,reference) "
            "VALUES(:trip_id,:move_id,:num,:first_day,:last_day,:days,:pr_cancel,:tlg,:reference) ";
        Qry.DeclareVariable( "TRIP_ID", otInteger );
        Qry.DeclareVariable( "MOVE_ID", otInteger );
        Qry.DeclareVariable( "NUM", otInteger );
        Qry.DeclareVariable( "FIRST_DAY", otDate );
        Qry.DeclareVariable( "LAST_DAY", otDate );
        Qry.DeclareVariable( "DAYS", otString );
        Qry.DeclareVariable( "PR_CANCEL", otInteger );
        Qry.DeclareVariable( "TLG", otString );
        Qry.DeclareVariable( "REFERENCE", otString );

        int FNewMove_id = NodeAsInteger("FNewMove_id", curNode);
        int move_id = NodeAsInteger("Move_id", curNode);
        int I = NodeAsInteger("NUM", curNode);
        TDateTime FFirst = NodeAsDateTime("FFirst", curNode);
        TDateTime FLast = NodeAsDateTime("FLast", curNode);
        string FDays = NodeAsString("FDays", curNode);
        int Cancel = NodeAsInteger("Cancel", curNode);
        string Tlg = NodeAsString("Tlg", curNode);
        string FReference = NodeAsString("FReference", curNode);



        if(FNewMove_id) {
            move_id = GetNextMove_id();
            xmlNodePtr NewMoveNode = NewTextChild(NewMovesNode, "NewMove");
            NewTextChild(NewMoveNode, "move_id", move_id);
            NewTextChild(NewMoveNode, "num", I);
        }
        Qry.SetVariable( "TRIP_ID", trip_id);
        Qry.SetVariable( "MOVE_ID", move_id);
        Qry.SetVariable( "NUM", I );
        Qry.SetVariable( "FIRST_DAY", FFirst );
        Qry.SetVariable( "LAST_DAY", FLast );
        Qry.SetVariable( "DAYS", FDays );
        Qry.SetVariable( "PR_CANCEL", Cancel);
        Qry.SetVariable( "TLG", Tlg );
        Qry.SetVariable( "REFERENCE", FReference );
        Qry.Execute();

        xmlNodePtr curDestNode = GetNode("DestList/Dest", curNode);
        if(curDestNode) {
            Qry.Clear();
            Qry.SQLText = 
                "INSERT INTO routes(move_id,num,cod,pr_cancel,land,company,trip,bc,takeoff,litera, "
                "triptype,f,c,y,unitrip,delta_in,delta_out,suffix) VALUES(:move_id,:num,:cod,:pr_cancel,:land, "
                ":company,:trip,:bc,:takeoff,:litera,:triptype,:f,:c,:y,:unitrip,:delta_in,:delta_out,:suffix) ";
            Qry.DeclareVariable( "MOVE_ID", otInteger );
            Qry.DeclareVariable( "NUM", otInteger );
            Qry.DeclareVariable( "COD", otString );
            Qry.DeclareVariable( "PR_CANCEL", otInteger );
            Qry.DeclareVariable( "LAND", otDate );
            Qry.DeclareVariable( "COMPANY", otString );
            Qry.DeclareVariable( "TRIP", otInteger );
            Qry.DeclareVariable( "BC", otString );
            Qry.DeclareVariable( "TAKEOFF", otDate );
            Qry.DeclareVariable( "LITERA", otString );
            Qry.DeclareVariable( "TRIPTYPE", otString );
            Qry.DeclareVariable( "F", otInteger );
            Qry.DeclareVariable( "C", otInteger );
            Qry.DeclareVariable( "Y", otInteger );
            Qry.DeclareVariable( "UNITRIP", otString );
            Qry.DeclareVariable( "DELTA_IN", otInteger );
            Qry.DeclareVariable( "DELTA_OUT", otInteger );
            Qry.DeclareVariable( "SUFFIX", otString );

            while(curDestNode) {
                int J = NodeAsInteger("NUM", curDestNode);
                string FCod = NodeAsString("FCod", curDestNode);
                int Pr_Cancel = NodeAsInteger("Pr_Cancel", curDestNode);
                TDateTime FLand;
                if(NodeIsNULL("FLand", curDestNode))
                    FLand = NoExists;
                else
                    FLand = NodeAsDateTime("FLand", curDestNode);
                string FCompany = NodeAsString("FCompany", curDestNode);
                TDateTime FTrip;
                if(NodeIsNULL("FTrip", curDestNode))
                    FTrip = NoExists;
                else
                    FTrip = NodeAsInteger("FTrip", curDestNode);
                string FBc = NodeAsString("FBc", curDestNode);
                TDateTime FTakeoff;
                if(NodeIsNULL("FTakeoff", curDestNode))
                    FTakeoff = NoExists;
                else
                    FTakeoff = NodeAsDateTime("FTakeoff", curDestNode);
                string FTriptype = NodeAsString("FTriptype", curDestNode);
                int FF = NodeAsInteger("FF", curDestNode);
                int FC = NodeAsInteger("FC", curDestNode);
                int FY = NodeAsInteger("FY", curDestNode);
                string Litera = NodeAsString("Litera", curDestNode);
                string FUnitrip = NodeAsString("FUnitrip", curDestNode);
                int delta_in = NodeAsInteger("delta_in", curDestNode);
                int delta_out = NodeAsInteger("delta_out", curDestNode);
                string FSuffix = NodeAsString("FSuffix", curDestNode);

                Qry.SetVariable( "MOVE_ID", move_id );
                Qry.SetVariable( "NUM", J );
                Qry.SetVariable( "COD", FCod );
                Qry.SetVariable( "PR_CANCEL", Pr_Cancel );
                if(FLand == NoExists)
                    Qry.SetVariable("LAND", FNull);
                else Qry.SetVariable("LAND", FLand);
                Qry.SetVariable( "COMPANY", FCompany );
                if(FTrip == NoExists)
                    Qry.SetVariable( "TRIP", FNull );
                else Qry.SetVariable("TRIP", FTrip);
                Qry.SetVariable( "BC", FBc );
                if(FTakeoff == NoExists)
                    Qry.SetVariable( "TAKEOFF", FNull );
                else Qry.SetVariable( "TAKEOFF", FTakeoff);
                Qry.SetVariable( "TRIPTYPE", FTriptype );
                Qry.SetVariable( "F", FF );
                Qry.SetVariable( "C", FC );
                Qry.SetVariable( "Y", FY );
                Qry.SetVariable( "LITERA", Litera );
                Qry.SetVariable( "UNITRIP", FUnitrip );
                Qry.SetVariable( "DELTA_IN", delta_in );
                Qry.SetVariable( "DELTA_OUT", delta_out );
                Qry.SetVariable( "SUFFIX", FSuffix );

                Qry.Execute();
                curDestNode = curDestNode->next;
            }
        }

        curNode = curNode->next;
    }
    TReqInfo::Instance()->MsgToLog("Изменение характеристик рейса ", evtSeason, trip_id);
}

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
        ProgTrace(TRACE5, "Executing qry %ld", tm.Print());

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
            ProgTrace(TRACE5, "Building idx %ld", tm.Print());

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
            ProgTrace(TRACE5, "Building resDoc %ld", tm.Print());
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
