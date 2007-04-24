#include "events.h"
#include "basic.h"
#include "exceptions.h"
#include "oralib.h"
#include "stl_utils.h"
#include "xml_unit.h"
#include "astra_consts.h"
#include "astra_utils.h"
#include "astra_consts.h"
#include "docs.h"

#define NICKNAME "DJEK"
#include "test.h"

using namespace std;
using namespace BASIC;
using namespace EXCEPTIONS;
using namespace ASTRA;

void EventsInterface::GetEvents(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    TReqInfo *reqInfo = TReqInfo::Instance();
    TQuery Qry(&OraSession);
    xmlNodePtr arx_date = ( GetNode( "arx_date", reqNode ) );
    Qry.Clear();
    double f=NoExists;
    if ( arx_date ) {
      Qry.SQLText=
          "SELECT arx_events.type type, msg, time, id1 AS point_id, "
          "       DECODE(type,:evtPax,id2,:evtPay,id2,-1) AS reg_no, "
          "       DECODE(type,:evtPax,id3,:evtPay,id3,-1) AS grp_id, "
          "       ev_user, station, ev_order "
          "FROM arx_events "
          "WHERE part_key>=:arx_date AND DECODE(type,:evtDisp,arx_events.id2,arx_events.id1)=:point_id "
          " ORDER BY ev_order";    	
  	  modf( (double)NodeAsDateTime( arx_date ), &f );
  	  f=f-2;
  	  Qry.CreateVariable( "arx_date", otDate, f );
          
    }
    else
      Qry.SQLText=
          "SELECT events.type type, msg, time, id1 AS point_id, "
          "       DECODE(type,:evtPax,id2,:evtPay,id2,-1) AS reg_no, "
          "       DECODE(type,:evtPax,id3,:evtPay,id3,-1) AS grp_id, "
          "       ev_user, station, ev_order "
          "FROM events "
          "WHERE DECODE(type,:evtDisp,events.id2,events.id1)=:point_id "
          " ORDER BY ev_order";
    //events.type IN (:evtFlt,:evtGraph,:evtPax,:evtPay,:evtTlg) AND
    int point_id = NodeAsInteger("point_id",reqNode);
    Qry.CreateVariable("point_id",otInteger,point_id);
    Qry.CreateVariable("evtPax",otString,EncodeEventType(ASTRA::evtPax));
    Qry.CreateVariable("evtPay",otString,EncodeEventType(ASTRA::evtPay));
    Qry.CreateVariable("evtDisp",otString,EncodeEventType(ASTRA::evtDisp));
    xmlNodePtr etNode = GetNode( "EventsTypes", reqNode );
    vector<string> eventsTypes;
    if ( etNode ) {
        etNode = etNode->children;
        while ( etNode ) {
            eventsTypes.push_back( NodeAsString( etNode->children ) );
            etNode = etNode->next;
        }
    }

    /*  Qry.CreateVariable("evtFlt",otString,EncodeEventType(ASTRA::evtFlt));
        Qry.CreateVariable("evtGraph",otString,EncodeEventType(ASTRA::evtGraph));
        Qry.CreateVariable("evtPax",otString,EncodeEventType(ASTRA::evtPax));
        Qry.CreateVariable("evtPay",otString,EncodeEventType(ASTRA::evtPay));
        Qry.CreateVariable("evtTlg",otString,EncodeEventType(ASTRA::evtTlg));*/
    Qry.Execute();

    xmlNodePtr logNode = NewTextChild(resNode, "events_log");

    for(;!Qry.Eof;Qry.Next())
    {
        if ( !eventsTypes.empty() &&
                find( eventsTypes.begin(), eventsTypes.end(), Qry.FieldAsString( "type" ) ) == eventsTypes.end() )
            continue;

        xmlNodePtr rowNode=NewTextChild(logNode,"row");
        NewTextChild(rowNode,"point_id",Qry.FieldAsInteger("point_id"));
        NewTextChild(rowNode,"ev_user",Qry.FieldAsString("ev_user"));
        NewTextChild(rowNode,"station",Qry.FieldAsString("station"));

        TDateTime time = UTCToClient(Qry.FieldAsDateTime("time"),reqInfo->desk.tz_region);

        NewTextChild(rowNode,"time",DateTimeToStr(time));
        NewTextChild(rowNode,"fmt_time",DateTimeToStr(time, "dd.mm.yy hh:nn"));
        NewTextChild(rowNode,"grp_id",Qry.FieldAsInteger("grp_id"),-1);
        NewTextChild(rowNode,"reg_no",Qry.FieldAsInteger("reg_no"),-1);
        NewTextChild(rowNode,"msg",Qry.FieldAsString("msg"));
        NewTextChild(rowNode,"ev_order",Qry.FieldAsInteger("ev_order"));
    };
    if(GetNode("LoadForm", reqNode)) {
        if ( GetNode( "seasonvars", reqNode ) )
            get_report_form("SeasonEventsLog", resNode);
        else
            get_report_form("EventsLog", resNode);
    }
    logNode = NewTextChild(resNode, "variables");
    if ( GetNode( "seasonvars", reqNode ) )
        SeasonListVars( point_id, 0, logNode, reqNode );
    else
        PaxListVars(point_id, 0, logNode, f);
    ProgTrace(TRACE5, "%s", GetXMLDocText(resNode->doc).c_str());
};
