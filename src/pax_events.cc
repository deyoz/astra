#include "pax_events.h"
#include "qrys.h"
#include "astra_utils.h"
#include "db_tquery.h"
#include "PgOraConfig.h"

#define NICKNAME "DENIS"
#include "serverlib/slogger.h"

using namespace std;
using namespace BASIC::date_time;

bool TPaxEvent::fromDB(int pax_id, TPaxEventTypes::Enum pax_event)
{
    clear();
    DB::TCachedQuery Qry(
          PgOra::getROSession("PAX_EVENTS"),
          "SELECT * FROM pax_events "
          "WHERE pax_id = :pax_id "
          "AND pax_event = :pax_event "
          "ORDER BY ev_order DESC ",
          QParams() << QParam("pax_id", otInteger, pax_id)
                    << QParam("pax_event", otString, TPaxEventTypesCode().encode(pax_event)),
          STDLOG);
    bool result = false;
    Qry.get().Execute();
    if(not Qry.get().Eof) {
        result = true;
        this->pax_id = pax_id;
        this->pax_event = pax_event;
        time = Qry.get().FieldAsDateTime("time");
        desk = Qry.get().FieldAsString("desk");
        station = Qry.get().FieldAsString("station");
    }
    return result;
}

void TPaxEvent::toDB(int pax_id, TPaxEventTypes::Enum pax_event)
{
    DB::TCachedQuery gateQry(
          PgOra::getROSession("STATIONS"),
          "SELECT name FROM stations "
          "WHERE desk = :desk "
          "AND work_mode = 'è'",
          QParams() << QParam("desk", otString, TReqInfo::Instance()->desk.code),
          STDLOG);
    gateQry.get().Execute();
    string gate;
    if(not gateQry.get().Eof)
        gate = gateQry.get().FieldAsString("name");
    DB::TCachedQuery Qry(
          PgOra::getROSession("PAX_EVENTS"),
          "INSERT INTO pax_events ( "
          "   ev_order, "
          "   pax_id, "
          "   pax_event, "
          "   time, "
          "   desk, "
          "   station "
          ") VALUES ( "
          "   :ev_order, "
          "   :pax_id, "
          "   :pax_event, "
          "   :time, "
          "   :desk, "
          "   :station "
          ") ",
          QParams() << QParam("ev_order", otInteger, PgOra::getSeqNextVal_int("EVENTS__SEQ"))
                    << QParam("pax_id", otInteger, pax_id)
                    << QParam("pax_event", otString, TPaxEventTypesCode().encode(pax_event))
                    << QParam("time", otDate, NowUTC())
                    << QParam("desk", otString, TReqInfo::Instance()->desk.code)
                    << QParam("station", otString, gate),
          STDLOG);
    Qry.get().Execute();
}
