#include "events.h"
#include "basic.h"
#include "exceptions.h"
#include "oralib.h"
#include "stl_utils.h"
#include "xml_unit.h"
#include "astra_consts.h"
#include "astra_utils.h"
#include "docs.h"

#define NICKNAME "DJEK"

using namespace std;
using namespace BASIC;
using namespace EXCEPTIONS;

void EventsInterface::GetEvents(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    TReqInfo *reqInfo = TReqInfo::Instance();
    TQuery Qry(&OraSession);
    Qry.Clear();
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

    {
        string form;
        get_report_form("EventsLog", form);
        NewTextChild(resNode, "form", form);
    }

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

        TDateTime time;
        if (reqInfo->user.time_form==tfUTC)
            time = Qry.FieldAsDateTime("time");
        else
            time = UTCToLocal(Qry.FieldAsDateTime("time"), reqInfo->desk.tz_region);

        NewTextChild(rowNode,"time",DateTimeToStr(time));
        NewTextChild(rowNode,"fmt_time",DateTimeToStr(time, "dd.mm.yy hh:nn"));
        NewTextChild(rowNode,"grp_id",Qry.FieldAsInteger("grp_id"),-1);
        NewTextChild(rowNode,"reg_no",Qry.FieldAsInteger("reg_no"),-1);
        NewTextChild(rowNode,"msg",Qry.FieldAsString("msg"));
        NewTextChild(rowNode,"ev_order",Qry.FieldAsInteger("ev_order"));
    };
    logNode = NewTextChild(resNode, "variables");
    PaxListVars(point_id, 0, logNode);
};
