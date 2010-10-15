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
#include "serverlib/test.h"

using namespace std;
using namespace BASIC;
using namespace AstraLocale;
using namespace EXCEPTIONS;
using namespace ASTRA;

void EventsInterface::GetEvents(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    TReqInfo *reqInfo = TReqInfo::Instance();
    TQuery Qry(&OraSession);
    xmlNodePtr arx_date = GetNode( "arx_date", reqNode ); // old version 02.07.2009
    xmlNodePtr node_part_key = GetNode( "part_key", reqNode );
    xmlNodePtr etNode = GetNode( "EventsTypes", reqNode );
    vector<string> eventsTypes;
    if ( etNode ) {
        etNode = etNode->children;
        while ( etNode ) {
            eventsTypes.push_back( NodeAsString( etNode->children ) );
            etNode = etNode->next;
        }
    }
    Qry.Clear();
    double f=NoExists;
    int point_id = NodeAsInteger("point_id",reqNode);
    int move_id = NoExists;
    TDateTime part_key = NoExists;
    if ( find( eventsTypes.begin(), eventsTypes.end(), EncodeEventType(ASTRA::evtDisp) ) != eventsTypes.end() ) {
    	if ( node_part_key ) {
    		tst();
    		Qry.SQLText=
    		  "SELECT move_id, part_key FROM arx_points "
    		  " WHERE part_key=:part_key AND point_id=:point_id AND pr_del>=0";
    	  Qry.CreateVariable( "part_key", otDate, NodeAsDateTime( node_part_key ) );
    	}
    	else
    	  if ( arx_date ) {// old version 02.07.2009
          Qry.SQLText=
           "SELECT move_id, part_key FROM arx_points "
           " WHERE part_key>=:arx_date AND part_key<=:arx_date+:arx_trip_date_range AND "
           "       point_id=:point_id AND pr_del>=0";
  	      modf( (double)NodeAsDateTime( arx_date ), &f );
  	      f=f-2;
          Qry.CreateVariable( "arx_date", otDate, f );
          Qry.CreateVariable( "arx_trip_date_range", otInteger, arx_trip_date_range);
      	}
    	  else {
          Qry.SQLText="SELECT move_id FROM points WHERE point_id=:point_id AND pr_del>=0";
    	  }
      Qry.CreateVariable( "point_id", otInteger, point_id );
      Qry.Execute();
      if ( Qry.RowCount() )
      {
      	move_id = Qry.FieldAsInteger( "move_id" );
      	if ( arx_date || node_part_key )
      	  part_key = Qry.FieldAsDateTime( "part_key" );
      };
    }

    Qry.Clear();
    if ( part_key > NoExists ) {
      Qry.SQLText =
          "SELECT arx_events.type type, msg, time, id2 AS point_id, "
          "       DECODE(type,:evtPax,id2,:evtPay,id2,-1) AS reg_no, "
          "       DECODE(type,:evtPax,id3,:evtPay,id3,-1) AS grp_id, "
          "       ev_user, station, ev_order "
          "FROM arx_events "
          "WHERE part_key=:part_key AND "
          " type=:evtDisp AND arx_events.id1=:move_id " //--AND arx_events.id2=:point_id
          "UNION "
          "SELECT arx_events.type type, msg, time, id1 AS point_id, "
          "       DECODE(type,:evtPax,id2,:evtPay,id2,-1) AS reg_no, "
          "       DECODE(type,:evtPax,id3,:evtPay,id3,-1) AS grp_id, "
          "       ev_user, station, ev_order "
          "FROM arx_events "
          "WHERE part_key=:part_key AND "
          " type IN(:evtSeason,:evtFlt,:evtGraph,:evtPax,:evtPay,:evtTlg) AND arx_events.id1=:point_id "
          " ORDER BY ev_order";
  	  Qry.CreateVariable( "part_key", otDate, part_key );
    }
    else
      Qry.SQLText=
          "SELECT events.type type, msg, time, id2 AS point_id, "
          "       DECODE(type,:evtPax,id2,:evtPay,id2,-1) AS reg_no, "
          "       DECODE(type,:evtPax,id3,:evtPay,id3,-1) AS grp_id, "
          "       ev_user, station, ev_order "
          "FROM events "
          "WHERE type=:evtDisp AND events.id1=:move_id " //--AND events.id2=:point_id
          "UNION "
          "SELECT events.type type, msg, time, id1 AS point_id, "
          "       DECODE(type,:evtPax,id2,:evtPay,id2,-1) AS reg_no, "
          "       DECODE(type,:evtPax,id3,:evtPay,id3,-1) AS grp_id, "
          "       ev_user, station, ev_order "
          "FROM events "
          "WHERE type IN(:evtSeason,:evtFlt,:evtGraph,:evtPax,:evtPay,:evtTlg) AND events.id1=:point_id "
          " ORDER BY ev_order";
    if ( move_id > NoExists )
    	Qry.CreateVariable("move_id",otInteger,move_id);
    else
    	Qry.CreateVariable("move_id",otInteger,FNull);
    Qry.CreateVariable("point_id",otInteger,point_id);
    Qry.CreateVariable("evtDisp",otString,EncodeEventType(ASTRA::evtDisp));
    Qry.CreateVariable("evtSeason",otString,EncodeEventType(ASTRA::evtSeason));
    Qry.CreateVariable("evtFlt",otString,EncodeEventType(ASTRA::evtFlt));
    Qry.CreateVariable("evtGraph",otString,EncodeEventType(ASTRA::evtGraph));
    Qry.CreateVariable("evtPax",otString,EncodeEventType(ASTRA::evtPax));
    Qry.CreateVariable("evtPay",otString,EncodeEventType(ASTRA::evtPay));
    Qry.CreateVariable("evtTlg",otString,EncodeEventType(ASTRA::evtTlg));

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
    logNode = NewTextChild(resNode, "variables");
    if ( GetNode( "seasonvars", reqNode ) ) {
        SeasonListVars( point_id, 0, logNode, reqNode );
        NewTextChild(logNode, "caption", getLocaleText("CAP.DOC.SEASON_EVENTS_LOG",
                    LParams() << LParam("trip", NodeAsString("trip", logNode))));
    } else {
        PaxListVars(point_id, 0, logNode, part_key);
        NewTextChild(logNode, "caption", getLocaleText("CAP.DOC.EVENTS_LOG",
                    LParams() << LParam("flight", get_flight(logNode)) //!!!den param 100%error
                    << LParam("day_issue", NodeAsString("day_issue", logNode)
                        )));
    }
    if(GetNode("LoadForm", reqNode))
            get_report_form("EventsLog", resNode);
    NewTextChild(logNode, "short_page_number_fmt", getLocaleText("CAP.SHORT_PAGE_NUMBER_FMT"));
    ProgTrace(TRACE5, "%s", GetXMLDocText(resNode->doc).c_str());
};
