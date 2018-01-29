#include "pax_events.h"
#include "qrys.h"
#include "astra_utils.h"

#define NICKNAME "DENIS"
#include "serverlib/slogger.h"

using namespace std;
using namespace BASIC::date_time;

void TPaxEvent::toDB(int pax_id, TPaxEventTypes::Enum pax_event)
{
    TCachedQuery gateQry(
            "select name from stations where desk = :desk and work_mode = 'è'",
            QParams() << QParam("desk", otString, TReqInfo::Instance()->desk.code));
    gateQry.get().Execute();
    string gate;
    if(not gateQry.get().Eof)
        gate = gateQry.get().FieldAsString("name");
    TCachedQuery Qry(
            "insert into pax_events ( "
            "   ev_order, "
            "   pax_id, "
            "   pax_event, "
            "   time, "
            "   desk, "
            "   station "
            ") values ( "
            "   events__seq.nextval, "
            "   :pax_id, "
            "   :pax_event, "
            "   :time, "
            "   :desk, "
            "   :station "
            ") ",
            QParams()
            << QParam("pax_id", otInteger, pax_id)
            << QParam("pax_event", otString, TPaxEventTypesCode().encode(pax_event))
            << QParam("time", otDate, NowUTC())
            << QParam("desk", otString, TReqInfo::Instance()->desk.code)
            << QParam("station", otString, gate));
    Qry.get().Execute();
}
