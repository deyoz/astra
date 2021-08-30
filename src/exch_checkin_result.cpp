#include "exch_checkin_result.h"
#include <serverlib/new_daemon.h>
#include <serverlib/posthooks.h>
#include <serverlib/ourtime.h>
#include <serverlib/perfom.h>
#include "astra_consts.h"
#include "astra_utils.h"
#include "misc.h"
#include "jms/jms.hpp"
#include "astra_misc.h"
#include "astra_utils.h"
#include "astra_main.h"
#include "exceptions.h"
#include "astra_context.h"
#include <serverlib/ourtime.h>
#include "astra_misc.h"
#include "passenger.h"
#include "remarks.h"
#include "points.h"
#include "stages.h"
#include "astra_date_time.h"
#include "astra_service.h"
#include "payment_base.h"
#include "rfisc.h"
#include "baggage_tags.h"
#include "baggage_ckin.h"

#include <serverlib/xml_stuff.h>

#define NICKNAME "DJEK"
#define NICKTRACE SYSTEM_TRACE
#include <serverlib/slogger.h>

using namespace ASTRA;
using namespace BASIC::date_time;
using namespace EXCEPTIONS;
using namespace std;
using namespace ServerFramework;

static void exch_Checkin_or_Flights_result_handler(const char* handler_id, const char* task);

class ExchCheckinResultDaemon : public ServerFramework::NewDaemon
{
public:
    ExchCheckinResultDaemon() : ServerFramework::NewDaemon("exch_checkin_result") {}
};


static int WAIT_INTERVAL()       //ᥪ㭤�
{
  static int VAR=NoExists;
  if (VAR==NoExists)
    VAR=getTCLParam("CHECKINRESULT_MQRABBIT_WAIT_INTERVAL",1,600,60);
  return VAR;
}

namespace MQRABBIT_TRANSPORT
{
  const std::string PARAM_NAME_ACTIONCODE = "ACTION_CODE";
  const int MAX_SEND_PAXS = 500;
  const int MAX_SEND_FLIGHTS = 500;

} //end namespace MQRABBIT_TRANSPORT

int main_exch_checkin_result_queue_tcl( int supervisorSocket, int argc, char *argv[] )
{
  try
  {
    sleep(10);
    InitLogTime(argc>0?argv[0]:NULL);
    ExchCheckinResultDaemon daemon;
    init_locale();

    for( ;; )
    {
      InitLogTime(argc>0?argv[0]:NULL);
      PerfomInit();
      base_tables.Invalidate();
      exch_Checkin_or_Flights_result_handler(argc > 0 ? argv[0] : NULL, argc > 1 ? argv[1] : NULL);
      sleep(WAIT_INTERVAL()); //� ᥪ㭤��
    };
  }
  catch( std::exception &E ) {
    ProgError( STDLOG, "std::exception: %s", E.what() );
  }
  catch( ... ) {
    ProgError( STDLOG, "Unknown error" );
  };
  return 0;
}

namespace EXCH_CHECKIN_RESULT
{
  struct Request: public MQRABBIT_TRANSPORT::MQRabbitRequest {
    std::string actionCode;
    enum Enum
    {
      ServicePayment,
      Docs,
      Baggage,
      LocalTime,
    };
    void clear() {
      MQRABBIT_TRANSPORT::MQRabbitRequest::clear();
      actionCode.clear();
    }
    bool isAction( Enum action ) const {
      switch ( action ) {
        case ServicePayment:
          return ( actionCode.find("S") != std::string::npos );
        case Docs:
          return ( actionCode.find("D") != std::string::npos );
        case Baggage:
          return ( actionCode.find("B") != std::string::npos );
        case LocalTime:
          return ( actionCode.find("L") != std::string::npos );
        default:
          return false;

      }
    }
    virtual ~Request(){}
    Request( const MQRABBIT_TRANSPORT::MQRabbitRequest& req ):MQRABBIT_TRANSPORT::MQRabbitRequest(req){}
  };

  inline bool isUTCUserSet()
  {
    return TReqInfo::Instance()->user.sets.time == ustTimeUTC;
  }

  inline void TimeToXML( xmlNodePtr node, const std::string& tag,
                         const TDateTime &time, const std::string &region,
                         const std::string& format="")
  {
    xmlNodePtr tnode = NewTextChild( node, tag.c_str(),
                                     DateTimeToStr( ASTRA::date_time::UTCToClient(time,region),
                                                    format.empty()?ServerFormatDateTimeAsString:format) );
    if ( !isUTCUserSet() )
      SetProp(tnode, "utc", DateTimeToStr(time,format.empty()?ServerFormatDateTimeAsString:format));

  }

  struct FlightData {
    TDateTime scd_in;
    TDateTime est_in;
    TDateTime act_in;
    TDateTime est_out;
    TDateTime act_out;
    std::string airp_arv;
    std::string craft;
    std::string bort;
    TCkinClients CkinClients;
    TAdvTripRoute route;
    std::string region;
    tstations stations;
    TFlightStages stages;
    TFlightDelays delays;
    FlightData( const TAdvTripRoute &vroute ) {
      scd_in = ASTRA::NoExists;
      est_in = ASTRA::NoExists;
      act_in = ASTRA::NoExists;
      est_out = ASTRA::NoExists;
      act_out = ASTRA::NoExists;
      route = vroute;
      region = AirpTZRegion( route.front().airp );
    }
    void fromDB( TQuery &FltQry );
    void toXML( xmlNodePtr flightNode );
  };

  struct PaxData {
     EXCH_CHECKIN_RESULT::Tids tids;
     int pax_id;
     int grp_id;
     int point_id;
     bool pr_del;
     std::string flight;
     TDateTime scd_out;
     std::string region;
     std::string name;
     std::string cl;
     std::string cabin_cl;
     std::string subclass;
     std::string pers_type;
     std::string gender;
     std::string airp_dep;
     std::string airp_arv;
     std::string seat_no;
     int seats;
     TBagKilos excess_wt;
     TBagPieces excess_pc;
     int bag_pool_num;
     multiset<TBagTagNumber> tags;
     int rkamount;
     int rkweight;
     int bagamount;
     int bagweight;
     std::string status;
     std::string client_type;
     TCkinRoute ckinRoute;
     CheckIn::TPaxTknItem tkn;
     std::multiset<CheckIn::TPaxRemItem> rems;
     CheckIn::TPaxDocItem doc;
     CheckIn::TPaxDocoItem doco;
     CheckIn::TPaidRFISCAndServicePaymentListWithAuto services;
     TDateTime time;
     TPnrAddrs pnrAddrs;
     PaxData( int vpax_id, int vpoint_id, TDateTime vmax_time ) :
       excess_wt(0), excess_pc(0)
     {
       pax_id = vpax_id;
       grp_id = ASTRA::NoExists;
       point_id = vpoint_id;
       seats = 0;
       bag_pool_num = ASTRA::NoExists;
       rkamount = 0;
       rkweight = 0;
       bagamount = 0;
       bagweight = 0;
       time = vmax_time;
     }
     void toXML( xmlNodePtr paxNode );
  };

  struct ChangePaxIdsDBData {
    int col_pax_id;
    int col_point_id;
    int col_time;
    void clear() {
      col_pax_id = ASTRA::NoExists;
      col_point_id = ASTRA::NoExists;
      col_time = ASTRA::NoExists;
    }
    ChangePaxIdsDBData() {
      clear();
    }
    void initChangePaxIdsData( DB::TQuery &ChangePaxIdsQry );
    void createSQLRequest( const Request &request, DB::TQuery &ChangePaxIdsQry  );
  };

  struct ChangeFlightsDBData {
    int col_point_id;
    int col_time;
    int col_tid;
    void clear() {
      col_point_id = ASTRA::NoExists;
      col_time = ASTRA::NoExists;
      col_tid = ASTRA::NoExists;
    }
    ChangeFlightsDBData() {
      clear();
    }
    void initChangeFlightsData( DB::TQuery &Qry );
    void createSQLRequest( const Request &request, DB::TQuery &Qry  );
  };


  class PaxDBData {
    TDateTime max_time;
    std::map<int,TTripInfo> trips;
    std::map<int,bool> syncs;
    int col_pax_id;
    int col_pax_tid;
    int col_grp_tid;
    int col_point_id;
    int col_grp_id;
    int col_name;
    int col_class;
    int col_cabin_class;
    int col_subclass;
    int col_pers_type;
    int col_is_female;
    int col_airp_dep;
    int col_airp_arv;
    int col_seat_no;
    int col_seats;
    int col_excess_wt_raw;
    int col_bag_pool_num;
    int col_pr_brd;
    int col_client_type;
    int col_time;
    bool pr_init;
  public:
    PaxDBData() {
      clear();
    }
    virtual ~PaxDBData() { }
    void clear() {
      max_time = ASTRA::NoExists;
      trips.clear();
      syncs.clear();
      col_pax_id = ASTRA::NoExists;
      col_pax_tid = ASTRA::NoExists;
      col_grp_tid = ASTRA::NoExists;
      col_point_id = ASTRA::NoExists;
      col_grp_id = ASTRA::NoExists;
      col_name = ASTRA::NoExists;
      col_class = ASTRA::NoExists;
      col_cabin_class = ASTRA::NoExists;
      col_subclass = ASTRA::NoExists;
      col_pers_type = ASTRA::NoExists;
      col_is_female = ASTRA::NoExists;
      col_airp_dep = ASTRA::NoExists;
      col_airp_arv = ASTRA::NoExists;
      col_seat_no = ASTRA::NoExists;
      col_seats = ASTRA::NoExists;
      col_excess_wt_raw = ASTRA::NoExists;
      col_bag_pool_num = ASTRA::NoExists;
      col_pr_brd = ASTRA::NoExists;
      col_client_type = ASTRA::NoExists;
      col_time = ASTRA::NoExists;
      pr_init = false;
    }
    void initPaxData( DB::TQuery &PaxQry );
    void createSQLRequest( DB::TQuery &PaxQry ) {
      PaxQry.SQLText =
          "SELECT pax.pax_id,pax.reg_no,RTRIM(COALESCE(pax.surname,'')||' '||COALESCE(pax.name,'')) name, "
          "       pax_grp.grp_id, "
          "       pax_grp.airp_arv,pax_grp.airp_dep, "
          "       pax_grp.class, COALESCE(pax.cabin_class, pax_grp.class) AS cabin_class, "
          "       pax.refuse, pax.pers_type, "
          "       COALESCE(pax.is_female,1) as is_female, "
          "       pax.subclass, "
          "       salons.get_seat_no(pax.pax_id,pax.seats,NULL,pax_grp.status,pax_grp.point_dep,'tlg',rownum) AS seat_no, "
          "       pax.seats seats, "
          "       pax.bag_pool_num, "
          "       pax.pr_brd, "
          "       pax_grp.status, "
          "       pax_grp.client_type, "
          "       pax.ticket_no, pax.tid pax_tid, pax_grp.tid grp_tid, "
          "       pax_grp.excess_wt, pax_grp.bag_refuse "
          " FROM pax_grp, pax "
          " WHERE pax_grp.grp_id=pax.grp_id AND "
          "       pax.pax_id=:pax_id AND "
          "       pax.wl_type IS NULL";
      PaxQry.DeclareVariable( "pax_id", otInteger );
    }
    int getFltNo( int point_id );
    bool getFlightInfo( int point_id, DB::TQuery &FltQry );
    void getPaxTids( DB::TQuery &PaxQry, PaxData &paxData );
    void getPaxData( const  Request &request, DB::TQuery &PaxQry, PaxData &paxData, const CKIN::BagReader& bag_reader );
    bool virtual is_sync( const TTripInfo &flight ) {
      return true;
    }
  };

  struct changeFlights: std::map<int, int> {//ᯨ᮪ ३ᮢ, sequence tids
    private:
      void fromContext( const Request &request );
      void toContext( const Request &request );
    public:
      bool processFlights( Request &request, xmlDocPtr &docFlights );
  };

  struct changePaxs: std::map<int,EXCH_CHECKIN_RESULT::Tids> {//ᯨ᮪ ���ᠦ�஢ ��।����� ࠭��
    private:
      void fromContext( const Request &request );
      void toContext( const Request &request );
    public:
      bool processPaxs( Request &request, xmlDocPtr &docPaxs );
  };

  void PaxDBData::initPaxData( DB::TQuery &PaxQry )
  {
    if ( pr_init ) {
      return;
    }
    pr_init = true;
    col_pax_id  = (col_pax_id = PaxQry.GetFieldIndex( "pax_id" )) >= 0 ?col_pax_id:ASTRA::NoExists;
    col_pax_tid = (col_pax_tid = PaxQry.GetFieldIndex( "pax_tid" )) >= 0 ?col_pax_tid:ASTRA::NoExists;
    col_grp_tid = (col_grp_tid = PaxQry.GetFieldIndex( "grp_tid" )) >= 0 ?col_grp_tid:ASTRA::NoExists;
    col_point_id = (col_point_id = PaxQry.GetFieldIndex( "point_id" )) >= 0 ?col_point_id:ASTRA::NoExists;
    col_grp_id = (col_grp_id = PaxQry.GetFieldIndex( "grp_id" )) >= 0 ?col_grp_id:ASTRA::NoExists;
    col_name = (col_name = PaxQry.GetFieldIndex( "name" )) >= 0 ?col_name:ASTRA::NoExists;
    col_class = (col_class = PaxQry.GetFieldIndex( "class" )) >= 0 ?col_class:ASTRA::NoExists;
    col_cabin_class = (col_cabin_class = PaxQry.GetFieldIndex( "cabin_class" )) >= 0 ?col_cabin_class:ASTRA::NoExists;
    col_subclass = (col_subclass = PaxQry.GetFieldIndex( "subclass" )) >= 0 ?col_subclass:ASTRA::NoExists;
    col_pers_type = (col_pers_type = PaxQry.GetFieldIndex( "pers_type" )) >= 0 ?col_pers_type:ASTRA::NoExists;
    col_is_female = (col_is_female = PaxQry.GetFieldIndex( "is_female" )) >= 0 ?col_is_female:ASTRA::NoExists;
    col_airp_dep = (col_airp_dep = PaxQry.GetFieldIndex( "airp_dep" )) >= 0 ?col_airp_dep:ASTRA::NoExists;
    col_airp_arv = (col_airp_arv = PaxQry.GetFieldIndex( "airp_arv" )) >= 0 ?col_airp_arv:ASTRA::NoExists;
    col_seat_no = (col_seat_no = PaxQry.GetFieldIndex( "seat_no" )) >= 0 ?col_seat_no:ASTRA::NoExists;
    col_seats = (col_seats = PaxQry.GetFieldIndex( "seats" )) >= 0 ?col_seats:ASTRA::NoExists;
    col_excess_wt_raw = (col_excess_wt_raw = PaxQry.GetFieldIndex( "excess_wt" )) >= 0 ?col_excess_wt_raw:ASTRA::NoExists;
    col_bag_pool_num = ( col_bag_pool_num = PaxQry.GetFieldIndex( "bag_pool_num" )) >= 0 ?col_bag_pool_num:ASTRA::NoExists;
    col_pr_brd = (col_pr_brd = PaxQry.GetFieldIndex( "pr_brd" )) >= 0 ?col_pr_brd:ASTRA::NoExists;
    col_client_type = (col_client_type = PaxQry.GetFieldIndex( "client_type" )) >= 0 ?col_client_type:ASTRA::NoExists;
    col_point_id = (col_point_id = PaxQry.GetFieldIndex( "point_id" )) >= 0 ?col_point_id:ASTRA::NoExists;
    col_time = (col_time = PaxQry.GetFieldIndex( "time" )) >= 0 ?col_time:ASTRA::NoExists;
  }

  void ChangePaxIdsDBData::initChangePaxIdsData( DB::TQuery &Qry )
  {
    clear();
    Qry.Execute();
    col_pax_id  = (col_pax_id = Qry.GetFieldIndex( "pax_id" )) >= 0 ?col_pax_id:ASTRA::NoExists;
    col_point_id  = (col_point_id = Qry.GetFieldIndex( "point_id" )) >= 0 ?col_point_id:ASTRA::NoExists;
    col_time  = (col_time = Qry.GetFieldIndex( "time" )) >= 0 ?col_time:ASTRA::NoExists;
  }

  void ChangePaxIdsDBData::createSQLRequest( const Request &request, DB::TQuery &ChangePaxIdsQry )
  {
    string sql_text =
      "SELECT pax_id,reg_no,work_mode,point_id,desk,client_type,time "
      "FROM aodb_pax_change "
      "WHERE time >= :time AND time <= :uptime ";
    if ( !request.airlines.empty() ||
         !request.airps.empty() ) {
      sql_text += " AND ( ";
      if ( !request.airlines.empty() ) {
        sql_text += " airline IN " + GetSQLEnum( request.airlines );
      }
      if ( !request.airps.empty() ) {
        if ( !request.airlines.empty() ) {
          sql_text += " OR ";
        }
        sql_text += " airp IN " + GetSQLEnum( request.airps );
      }
      sql_text += ") ";
    }
    sql_text += "ORDER BY time, pax_id, work_mode ";
    ProgTrace( TRACE5, "sql_text=%s", sql_text.c_str() );
    ChangePaxIdsQry.SQLText = sql_text;
    TDateTime nowUTC = NowUTC();
    ChangePaxIdsQry.CreateVariable( "time", otDate,request.lastRequestTime );
    ChangePaxIdsQry.CreateVariable( "uptime", otDate, nowUTC - 1.0/1440.0 );
    initChangePaxIdsData( ChangePaxIdsQry );
  }

  void ChangeFlightsDBData::initChangeFlightsData( DB::TQuery &Qry )
  {
    clear();
    Qry.Execute();
    col_point_id  = (col_point_id = Qry.GetFieldIndex( "point_id" )) >= 0 ?col_point_id:ASTRA::NoExists;
    col_time  = (col_time = Qry.GetFieldIndex( "time" )) >= 0 ?col_time:ASTRA::NoExists;
    col_tid  = (col_tid = Qry.GetFieldIndex( "tid" )) >= 0 ?col_tid:ASTRA::NoExists;
  }

  void ChangeFlightsDBData::createSQLRequest( const Request &request, DB::TQuery &Qry )
  {
    string sql_text =
      "SELECT exch_flights.point_id,time, exch_flights.tid "
      " FROM exch_flights ";
    if ( !request.airlines.empty() ||
         !request.airps.empty() ) {
      sql_text += ",points ";
    }
   sql_text += "WHERE time >= :time AND time <= :uptime ";
    if ( !request.airlines.empty() ||
         !request.airps.empty() ) {
      sql_text += " AND points.point_id=exch_flights.point_id AND ( ";
      if ( !request.airlines.empty() ) {
        sql_text += " airline IN " + GetSQLEnum( request.airlines );
      }
      if ( !request.airps.empty() ) {
        if ( !request.airlines.empty() ) {
          sql_text += " OR ";
        }
        sql_text += " airp IN " + GetSQLEnum( request.airps );
      }
      sql_text += ") ";
    }
    sql_text += "ORDER BY time, exch_flights.point_id ";
    ProgTrace( TRACE5, "sql_text=%s", sql_text.c_str() );
    Qry.SQLText = sql_text;
    TDateTime nowUTC = NowUTC();
    Qry.CreateVariable( "time", otDate,request.lastRequestTime );
    Qry.CreateVariable( "uptime", otDate, nowUTC - 1.0/1440.0 );
    initChangeFlightsData( Qry );
  }

  bool PaxDBData::getFlightInfo( int point_id, DB::TQuery &FltQry )
  {
    std::map<int,TTripInfo>::const_iterator iflt = trips.find( point_id );
    if ( iflt == trips.end() ) {
      FltQry.SetVariable( "point_id", point_id );
      FltQry.Execute();
      if ( FltQry.Eof ) {
        return false;
      }
      TTripInfo tripInfo( FltQry );
      trips.insert( make_pair( point_id, tripInfo ) );
    }
    if ( syncs.find( point_id ) == syncs.end() ) {
      syncs.insert( make_pair( point_id, is_sync( trips.find( point_id )->second ) ) );
    }
    if ( !syncs[ point_id ] ) { // � ����� ������ ������������ � ��ய���� �ਭ������騥 ������������
      return false;
    }
    return true;
  }

  int PaxDBData::getFltNo( int point_id )
  {
    std::map<int,TTripInfo>::const_iterator iflt = trips.find( point_id );
    if ( iflt == trips.end() ) {
      return -1;
    }
    return iflt->second.flt_no;
  }

  void PaxDBData::getPaxTids( DB::TQuery &PaxQry, PaxData &paxData )
  {
    EXCH_CHECKIN_RESULT::Tids tids;
    if ( !PaxQry.Eof ) {
      tst();
      tids.pax_tid = PaxQry.FieldAsInteger( col_pax_tid );
      tids.grp_tid = PaxQry.FieldAsInteger( col_grp_tid );
    }
    paxData.tids = tids;
    ProgTrace( TRACE5, "pax_tid=%d, grp_tid=%d", paxData.tids.pax_tid, paxData.tids.grp_tid );
  }

   void PaxDBData::getPaxData(const Request &request, DB::TQuery &PaxQry, PaxData &paxData , const CKIN::BagReader &bag_reader)
  {
    paxData.pr_del = PaxQry.Eof;
    if ( paxData.pr_del ) {
      return;
    }
    paxData.flight = trips[ paxData.point_id ].airline + IntToString( trips[ paxData.point_id ].flt_no ) + trips[ paxData.point_id ].suffix;
    paxData.scd_out = trips[ paxData.point_id ].scd_out;
    paxData.region = AirpTZRegion( trips[ paxData.point_id ].airp );
    paxData.grp_id = PaxQry.FieldAsInteger( col_grp_id );
    paxData.name = PaxQry.FieldAsString( col_name );
    paxData.cl = PaxQry.FieldAsString( col_class );
    paxData.cabin_cl = PaxQry.FieldAsString( col_cabin_class );
    paxData.subclass = PaxQry.FieldAsString( col_subclass );
    paxData.pers_type = PaxQry.FieldAsString( col_pers_type );
    if ( DecodePerson( PaxQry.FieldAsString( col_pers_type ) ) == ASTRA::adult ) {
      paxData.gender = (PaxQry.FieldAsInteger(col_is_female)==0?"M":"F");
    }
    paxData.airp_dep = PaxQry.FieldAsString( col_airp_dep );
    paxData.airp_arv = PaxQry.FieldAsString( col_airp_arv );
    paxData.seat_no = PaxQry.FieldAsString( col_seat_no );
    paxData.seats = PaxQry.FieldAsInteger( col_seats );

    int excess_wt_raw = PaxQry.FieldAsInteger(col_excess_wt_raw);
    int bag_refuse = PaxQry.FieldAsInteger("bag_refuse");
    paxData.excess_wt = TBagKilos(CKIN::get_excess_wt(GrpId_t(paxData.grp_id), PaxId_t(paxData.pax_id),
                            excess_wt_raw, bag_refuse).value_or(NoExists));

    paxData.excess_pc = countPaidExcessPC(PaxId_t(PaxQry.FieldAsInteger(col_pax_id)));
    paxData.bag_pool_num = PaxQry.FieldAsInteger( col_bag_pool_num );

    std::optional<int> opt_bag_pool_num;
    if(!PaxQry.FieldIsNULL(col_bag_pool_num)) { opt_bag_pool_num = paxData.bag_pool_num;}

    paxData.rkamount = bag_reader.rkAmount(GrpId_t(paxData.grp_id), opt_bag_pool_num);
    paxData.rkweight =  bag_reader.rkWeight(GrpId_t(paxData.grp_id), opt_bag_pool_num);
    paxData.bagamount = bag_reader.bagAmount(GrpId_t(paxData.grp_id), opt_bag_pool_num);
    paxData.bagweight = bag_reader.bagWeight(GrpId_t(paxData.grp_id), opt_bag_pool_num);

    if ( PaxQry.FieldIsNULL( col_pr_brd ) )
      paxData.status = "uncheckin";
    else
      if ( PaxQry.FieldAsInteger( col_pr_brd ) == 0 )
        paxData.status = "checkin";
      else
        paxData.status = "boarded";
    paxData.client_type = PaxQry.FieldAsString( col_client_type );
    paxData.ckinRoute.getRouteAfter( GrpId_t(PaxQry.FieldAsInteger( col_grp_id )),
                                     TCkinRoute::NotCurrent,
                                     TCkinRoute::IgnoreDependence,
                                     TCkinRoute::WithoutTransit);
    LoadPaxTkn( paxData.pax_id, paxData.tkn );
    LoadPaxRem( paxData.pax_id, paxData.rems ); //�� ��⠫�� ६�ન
    if ( request.isAction( Request::ServicePayment ) ) {
      CheckIn::TServiceReport service_report;
      paxData.services=service_report.get(paxData.grp_id);
    }
    if ( request.isAction( Request::Docs ) ) {
      CheckIn::LoadPaxDoc( paxData.pax_id, paxData.doc );
      CheckIn::LoadPaxDoco( paxData.pax_id, paxData.doco );
    }
    if ( request.isAction( Request::Baggage ) ) {
      GetTagsByPool(paxData.grp_id, paxData.bag_pool_num, paxData.tags, false);
    }
    paxData.pnrAddrs.getByPaxIdFast( paxData.pax_id );
  }

  void FlightData::fromDB( TQuery &FltQry ) {
    stations.fromDB( route.front().point_id );
    stages.Load( route.front().point_id );
    delays.Load( route.front().point_id );
    TTripStages::ReadCkinClients( route.front().point_id, CkinClients );
    FltQry.SetVariable( "point_id", route.front().point_id );
    FltQry.Execute();
    if ( !FltQry.Eof ) {
      est_out = FltQry.FieldIsNULL( "est_out" )?ASTRA::NoExists:FltQry.FieldAsDateTime( "est_out" );
      act_out = FltQry.FieldIsNULL( "act_out" )?ASTRA::NoExists:FltQry.FieldAsDateTime( "act_out" );
      craft = FltQry.FieldAsString( "craft" );
      bort = FltQry.FieldAsString( "bort" );
    }
    std::vector<TAdvTripRouteItem>::const_iterator iarr_route = route.begin();
    iarr_route++;
    if ( iarr_route != route.end() ) {
      airp_arv = iarr_route->airp;
      scd_in = iarr_route->scd_in;
      FltQry.SetVariable( "point_id", iarr_route->point_id );
      FltQry.Execute();
      if ( !FltQry.Eof ) {
        scd_in = FltQry.FieldIsNULL( "scd_in" )?ASTRA::NoExists:FltQry.FieldAsDateTime( "scd_in" );
        est_in = FltQry.FieldIsNULL( "est_in" )?ASTRA::NoExists:FltQry.FieldAsDateTime( "est_in" );
        act_in = FltQry.FieldIsNULL( "act_in" )?ASTRA::NoExists:FltQry.FieldAsDateTime( "act_in" );
      }
    }

  }

  void FlightData::toXML( xmlNodePtr flightNode )
  {
    NewTextChild( flightNode, "point_id", route.front().point_id );
    NewTextChild( flightNode, "airline", route.front().airline_out );
    NewTextChild( flightNode, "flt_no", route.front().flt_num_out );
    if ( !route.front().suffix_out.empty() ) {
      NewTextChild( flightNode, "suffix", route.front().suffix_out );
    }
    TimeToXML(flightNode,"scd_out",route.front().scd_out,region,"dd.mm.yyyy hh:nn");

    if ( !craft.empty() ) {
      NewTextChild( flightNode, "craft", craft );
    }
    if ( !bort.empty() ) {
      NewTextChild( flightNode, "bort", bort );
    }
    if ( est_out != ASTRA::NoExists ) {
      TimeToXML(flightNode,"est_out",est_out,region,"dd.mm.yyyy hh:nn");
    }
    if ( route.front().act_out != ASTRA::NoExists ) {
      TimeToXML(flightNode,"act_out",act_out,region,"dd.mm.yyyy hh:nn");
    }
    NewTextChild( flightNode, "pr_cancel", route.front().pr_cancel );
    NewTextChild( flightNode, "airp_dep", route.front().airp );
    if ( !airp_arv.empty() ) {
      NewTextChild( flightNode, "airp_arv", airp_arv );
      std::string region_arr = AirpTZRegion( airp_arv );
      if ( scd_in != ASTRA::NoExists ) {
        TimeToXML(flightNode,"scd_in",scd_in,region_arr,"dd.mm.yyyy hh:nn");
      }
      if ( est_in != ASTRA::NoExists ) {
        TimeToXML(flightNode,"est_in",est_in,region_arr,"dd.mm.yyyy hh:nn");
      }
      if ( act_in != ASTRA::NoExists ) {
        TimeToXML(flightNode,"act_in",act_in,region_arr,"dd.mm.yyyy hh:nn");
      }
    }
    std::vector<TPointsDestDelay> vdelays;
    if ( !delays.Empty() ) {
      xmlNodePtr node = NewTextChild( flightNode, "delays" );
      delays.Get( vdelays );
      for( std::vector<TPointsDestDelay>::const_iterator idel=vdelays.begin(); idel!=vdelays.end(); idel++ ) {
        xmlNodePtr delNode = NewTextChild( node, "delay" );
        SetProp( delNode, "code", idel->code );
        SetProp( delNode, "time", DateTimeToStr( ASTRA::date_time::UTCToClient( idel->time, region ), "dd.mm.yyyy hh:nn" ) );
        if (!isUTCUserSet())
          SetProp(delNode,"utc",DateTimeToStr(idel->time,"dd.mm.yyyy hh:nn"));
      }
    }
    if ( !stages.Empty() ) {
      xmlNodePtr node = NewTextChild( flightNode, "stages" );
      CreateXMLStage( CkinClients, sPrepCheckIn, stages.GetStage( sPrepCheckIn ), node, region, !isUTCUserSet() );
      CreateXMLStage( CkinClients, sOpenCheckIn, stages.GetStage( sOpenCheckIn ), node, region, !isUTCUserSet() );
      CreateXMLStage( CkinClients, sCloseCheckIn, stages.GetStage( sCloseCheckIn ), node, region, !isUTCUserSet() );
      CreateXMLStage( CkinClients, sOpenBoarding, stages.GetStage( sOpenBoarding ), node, region, !isUTCUserSet() );
      CreateXMLStage( CkinClients, sCloseBoarding, stages.GetStage( sCloseBoarding ), node, region, !isUTCUserSet() );
      CreateXMLStage( CkinClients, sOpenWEBCheckIn, stages.GetStage( sOpenWEBCheckIn ), node, region, !isUTCUserSet() );
      CreateXMLStage( CkinClients, sCloseWEBCheckIn, stages.GetStage( sCloseWEBCheckIn ), node, region, !isUTCUserSet() );
      CreateXMLStage( CkinClients, sOpenKIOSKCheckIn, stages.GetStage( sOpenKIOSKCheckIn ), node, region, !isUTCUserSet() );
      CreateXMLStage( CkinClients, sCloseKIOSKCheckIn, stages.GetStage( sCloseKIOSKCheckIn ), node, region, !isUTCUserSet() );
    }
    stations.toXML(flightNode);
  }

  void PaxData::toXML( xmlNodePtr paxNode )
  {
    NewTextChild( paxNode, "pax_id", pax_id );
    NewTextChild( paxNode, "point_id", point_id );
    NewTextChild( paxNode, "time", DateTimeToStr( time, ServerFormatDateTimeAsString ) );
    if ( pr_del ) {
      NewTextChild( paxNode, "status", "delete" );
      return;
    }
    NewTextChild( paxNode, "flight", flight );
    TimeToXML(paxNode,"scd_out",scd_out,region);
    NewTextChild( paxNode, "grp_id", grp_id );
    NewTextChild( paxNode, "name", name );
    NewTextChild( paxNode, "class", cl );
    NewTextChild( paxNode, "cabin_class", cabin_cl );
    NewTextChild( paxNode, "subclass", subclass );
    NewTextChild( paxNode, "pers_type", pers_type );
    if ( !gender.empty() ) {
      NewTextChild( paxNode, "gender", gender );
    }
    NewTextChild( paxNode, "airp_dep", airp_dep );
    NewTextChild( paxNode, "airp_arv", airp_arv );
    NewTextChild( paxNode, "seat_no", seat_no );
    NewTextChild( paxNode, "seats", seats );
    NewTextChild( paxNode, "excess_wt", excess_wt.getQuantity() );
    NewTextChild( paxNode, "excess_pc", excess_pc.getQuantity() );
    NewTextChild( paxNode, "excess", TComplexBagExcess(excess_pc, excess_wt).getDeprecatedInt() );
    NewTextChild( paxNode, "rkamount", rkamount );
    NewTextChild( paxNode, "rkweight", rkweight );
    NewTextChild( paxNode, "bagamount", bagamount );
    NewTextChild( paxNode, "bagweight", bagweight );
    NewTextChild( paxNode, "status", status );
    NewTextChild( paxNode, "client_type", client_type);
    if ( !this->ckinRoute.empty() ) {
      xmlNodePtr rnode = NewTextChild( paxNode, "tckin_route" );
      int seg_no=1;
      for ( TCkinRoute::iterator i=ckinRoute.begin(); i!=ckinRoute.end(); i++ ) {
         xmlNodePtr inode = NewTextChild( rnode, "seg" );
         SetProp( inode, "num", seg_no );
         NewTextChild( inode, "flight", i->operFlt.airline + IntToString(i->operFlt.flt_no) + i->operFlt.suffix );
         NewTextChild( inode, "airp_dep", i->airp_dep );
         NewTextChild( inode, "airp_arv", i->airp_arv );
         TimeToXML(inode,"scd_out",i->operFlt.scd_out,AirpTZRegion( i->airp_dep ));
         seg_no++;
      }
    }
    if ( !doc.empty() ) {
      doc.toWebXML( paxNode, AstraLocale::OutputLang( "en" ) );
    }
    if ( !doco.empty() ) {
      doco.toWebXML( paxNode, AstraLocale::OutputLang( "en" ) );
    }
    if ( !rems.empty() ) {
      xmlNodePtr rnode = NewTextChild( paxNode, "rems" );
      for ( std::multiset<CheckIn::TPaxRemItem>::const_iterator irem=rems.begin(); irem!=rems.end(); irem++ ) {
        irem->toXML( rnode );
      }
    }
    if ( !tkn.empty() ) {
      tkn.toXML( NewTextChild( paxNode, "tkn" ) );
    }
    if ( !pnrAddrs.empty() ) {
      pnrAddrs.toSirenaXML(  NewTextChild( paxNode, "pnrAddrs" ), AstraLocale::OutputLang( "en" ) );
      //pnrAddrs.toXML( NewTextChild( paxNode, "pnrAddrs" ) );
    }
    xmlNodePtr servicesNode = nullptr;
    for(const auto &service: services) {
      const TPaidRFISCStatus &item =service.first;
      const boost::optional<CheckIn::TServicePaymentItem> &pay_info = service.second;
      if (item.pax_id != pax_id or item.trfer_num != 0) continue;
       if (service.first.list_item) {
         if ( servicesNode == nullptr ) {
           servicesNode = NewTextChild( paxNode, "services" );
         }
         xmlNodePtr itemNode = NewTextChild( servicesNode, "item" );
         NewTextChild( itemNode, "rfic", item.list_item->RFIC );
         NewTextChild( itemNode, "rfisc", item.list_item->RFISC );
         NewTextChild( itemNode, "desc", services.getRFISCName(item, AstraLocale::LANG_EN) );
         NewTextChild( itemNode, "status", ServiceStatuses().encode(item.status) );
         if (pay_info) {
           NewTextChild( itemNode, "num", pay_info->no_str() );
         }
       }
    }
    if ( !tags.empty() ) {
      xmlNodePtr LuggageTagNode = NewTextChild(paxNode, "LuggageTags");
      for(const auto &tag : tags) {
        NewTextChild(LuggageTagNode, "LuggageNumber", tag.str());
      }
    }
  }

  void changePaxs::fromContext( const Request &request )
  {
    clear();
    xmlDocPtr paxsDoc;
    xmlNodePtr node;
    std::string prior_paxs;
    TDateTime priorRequestTime;
    if ( !request.pr_reset && AstraContext::GetContext( request.Sender, 0, prior_paxs ) != NoExists ) {
      paxsDoc = TextToXMLTree( prior_paxs );
      try {
        xmlNodePtr nodePax = paxsDoc->children;
        node = GetNode( "@time", nodePax );
        if ( node == NULL )
          throw AstraLocale::UserException( "Tag '@time' not found in context" );
        if ( StrToDateTime( NodeAsString( node ), "dd.mm.yyyy hh:nn:ss", priorRequestTime ) == EOF )
              throw AstraLocale::UserException( "Invalid tag value '@time' in context" );
        if ( priorRequestTime == request.lastRequestTime ) { // ࠧ��� ��ॢ� �� �᫮���, �� �।��騩 ����� �� ��।�� ��� ���ᠦ�஢ �� ������� ������ �६���
          nodePax = nodePax->children;
          for ( ; nodePax!=NULL && string((const char*)nodePax->name) == "pax"; nodePax=nodePax->next ) {
            int pax_id = NodeAsInteger( "@pax_id", nodePax );
            EXCH_CHECKIN_RESULT::Tids tids( NodeAsInteger( "@pax_tid", nodePax ), NodeAsInteger( "@grp_tid", nodePax ) );
            std::pair< changePaxs::iterator, bool > ret;
            ret = insert( make_pair( pax_id, tids ) );
            if ( !ret.second ) {
              ret.first->second = tids;
            }
            ProgTrace( TRACE5, "pax_id=%d, pax_tid=%d, grp_tid=%d",
                       pax_id, tids.pax_tid, tids.grp_tid );
          }
        }
      }
      catch( ... ) {
        xmlFreeDoc( paxsDoc );
        throw;
      }
      xmlFreeDoc( paxsDoc );
    }
  }

  void changePaxs::toContext( const Request &request )
  {
    AstraContext::ClearContext( request.Sender, 0 );
    if ( !empty() ) { // ���� ���ᠦ��� - ��࠭塞 ��� ��।�����
      xmlDocPtr paxsDoc;
      xmlNodePtr node;
      paxsDoc = CreateXMLDoc( "paxs" );
      try {
        node = paxsDoc->children;
        SetProp( node, "time", DateTimeToStr( request.lastRequestTime, ServerFormatDateTimeAsString ) );
        for ( changePaxs::iterator p=begin(); p!=end(); p++ ) {
          xmlNodePtr n = NewTextChild( node, "pax" );
          SetProp( n, "pax_id", p->first );
          SetProp( n, "pax_tid", p->second.pax_tid );
          SetProp( n, "grp_tid", p->second.grp_tid );
        }
        AstraContext::SetContext( request.Sender, 0, XMLTreeToText( paxsDoc ) );
        //ProgTrace( TRACE5, "xmltreetotext=%s", XMLTreeToText( paxsDoc ).c_str() );
      }
      catch( ... ) {
        xmlFreeDoc( paxsDoc );
        throw;
      }
      xmlFreeDoc( paxsDoc );
    }
  }

  bool changePaxs::processPaxs( Request &request, xmlDocPtr &docPaxs )
  {
    if ( docPaxs ) {
      xmlFreeDoc( docPaxs );
    }
    docPaxs = CreateXMLDoc( "paxs" );
    xmlNodePtr node = docPaxs->children;
    fromContext( request );
    ChangePaxIdsDBData changePaxIdsDBData;
    DB::TQuery changePaxIdsQry(PgOra::getROSession("AODB_PAX_CHANGE"), STDLOG);
    TDateTime nowUTC = NowUTC();
    request.lastRequestTime  = ((request.lastRequestTime < nowUTC - 1.0/24.0)? nowUTC - 1.0/24.0:request.lastRequestTime);
    changePaxIdsDBData.createSQLRequest(request, changePaxIdsQry);
    PaxDBData paxDBData;
    DB::TQuery PaxQry(PgOra::getROSession({"PAX_GRP","PAX"}), STDLOG);
    paxDBData.createSQLRequest( PaxQry );
    // �஡�� �� �ᥬ ���ᠦ�ࠬ � ������ �६� ����� ��� ࠢ�� ⥪�饬�
    int prior_pax_id = -1;
    int count_row = 0;
    int pax_count = 0;
    TDateTime max_time = ASTRA::NoExists;
    DB::TQuery FltQry(PgOra::getROSession("POINTS"), STDLOG);
    FltQry.SQLText =
      "SELECT airline,flt_no,suffix,airp,scd_out "
      "FROM points "
      "WHERE point_id=:point_id";
    FltQry.DeclareVariable( "point_id", otInteger );
    using namespace CKIN;
    std::map<PointId_t, BagReader> bag_readers;
    for ( ;!changePaxIdsQry.Eof && pax_count<=MQRABBIT_TRANSPORT::MAX_SEND_PAXS; changePaxIdsQry.Next() ) { //��-��襬� ��ਤ��� �������� �⭮襭�� � ���-ॣ����樨 �� �����
      count_row++;
      int pax_id = changePaxIdsQry.FieldAsInteger( changePaxIdsDBData.col_pax_id );
      if ( pax_id == prior_pax_id ) // 㤠�塞 �㡫�஢���� ��ப� � ����� � ⥬ �� pax_id ��� ॣ����樨 � ��ᠤ��
        continue; // �।��騩 ���ᠦ�� �� �� � ⥪�騩
      prior_pax_id = pax_id;
      int point_id = changePaxIdsQry.FieldAsInteger( changePaxIdsDBData.col_point_id );
      if(!algo::contains(bag_readers, PointId_t(point_id))) {
        bag_readers[PointId_t(point_id)] = BagReader(PointId_t(point_id), std::nullopt, READ::BAGS_AND_TAGS);
      }
      if ( !paxDBData.getFlightInfo( point_id, FltQry ) ) {
        continue;
      }
      //䨫��� �� ������ ३�
      int flt_no = paxDBData.getFltNo( point_id );
      if ( flt_no < 0 || ( !request.flts.empty() && std::find( request.flts.begin(), request.flts.end(), flt_no ) == request.flts.end() ) ) {
        continue;
      }
      if ( max_time != NoExists && max_time != changePaxIdsQry.FieldAsDateTime( changePaxIdsDBData.col_time ) ) { // �ࠢ����� �६��� � �।. ���祭���, �᫨ ����������, �
        ProgTrace( TRACE5, "Paxs.clear(), request.lastRequestTime=%s, max_time=%s",
                   DateTimeToStr( request.lastRequestTime, ServerFormatDateTimeAsString ).c_str(),
                   DateTimeToStr( max_time, ServerFormatDateTimeAsString ).c_str() );
        clear(); // ���������� �६� - 㤠�塞 ��� �।���� ���ᠦ�஢ � �।. �६����
      }
      max_time = changePaxIdsQry.FieldAsDateTime( changePaxIdsDBData.col_time );
      PaxData paxData( pax_id, point_id, max_time );
      PaxQry.SetVariable( "pax_id", pax_id );
      PaxQry.Execute();
      paxDBData.initPaxData( PaxQry );
      paxDBData.getPaxTids( PaxQry, paxData );
      // ���ᠦ�� ��।������ � �� ���������
      changePaxs::iterator itid = find( pax_id );
      ProgTrace( TRACE5, "itid != end() %d", itid != end() );
      if ( itid != end() &&
           itid->second.pax_tid == paxData.tids.pax_tid &&
           itid->second.grp_tid == paxData.tids.grp_tid ) {
        continue; // 㦥 ��।����� ���ᠦ��
      }
      // ���ᠦ�� �� ��।����� ��� �� ���������
      std::pair<std::map<int,EXCH_CHECKIN_RESULT::Tids>::iterator, bool> ret;
      ret = insert( make_pair( pax_id, paxData.tids ) );
      if ( !ret.second ) {
        ret.first->second = paxData.tids;
      }
      paxDBData.getPaxData( request, PaxQry, paxData, bag_readers[PointId_t(point_id)]);
      pax_count++;
      paxData.toXML( NewTextChild( node, "pax" ) );
    } //end for
    if ( max_time != NoExists )
      request.lastRequestTime = max_time;
    SetProp( node, "time", DateTimeToStr( request.lastRequestTime, ServerFormatDateTimeAsString ) );
    toContext( request );
    xml_encode_nodelist( docPaxs->children );
    ProgTrace( TRACE5, "count_row=%d, pax_count=%d", count_row, pax_count );
    return pax_count > 0;
  }

  void changeFlights::fromContext( const Request &request )
  {
    clear();
    xmlDocPtr flightsDoc;
    xmlNodePtr node;
    std::string prior_flights;
    TDateTime priorRequestTime;
    ProgTrace( TRACE5, "request.Sender=%s", request.Sender.c_str() );
    if ( !request.pr_reset && AstraContext::GetContext( request.Sender + ".flights", 0, prior_flights ) != NoExists ) {
      flightsDoc = TextToXMLTree( prior_flights );
      try {
        xmlNodePtr nodeFlight = flightsDoc->children;
        node = GetNode( "@time", nodeFlight );
        if ( node == NULL )
          throw AstraLocale::UserException( "Tag '@time' not found in context" );
        if ( StrToDateTime( NodeAsString( node ), "dd.mm.yyyy hh:nn:ss", priorRequestTime ) == EOF )
              throw AstraLocale::UserException( "Invalid tag value '@time' in context" );
        if ( priorRequestTime == request.lastRequestTime ) { // ࠧ��� ��ॢ� �� �᫮���, �� �।��騩 ����� �� ��।�� ��� ���ᠦ�஢ �� ������� ������ �६���
          nodeFlight = nodeFlight->children;
          for ( ; nodeFlight!=NULL && string((const char*)nodeFlight->name) == "flight"; nodeFlight=nodeFlight->next ) {
            int point_id = NodeAsInteger( "@point_id", nodeFlight );
            int tid = NodeAsInteger( "@tid", nodeFlight );
            std::pair< changeFlights::iterator, bool > ret;
            ret = insert( make_pair( point_id, tid ) );
            if ( !ret.second ) {
              ret.first->second = tid;
            }
            ProgTrace( TRACE5, "point_id=%d, tid=%d", point_id, tid );
          }
        }
      }
      catch( ... ) {
        xmlFreeDoc( flightsDoc );
        throw;
      }
      xmlFreeDoc( flightsDoc );
    }
  }

  void changeFlights::toContext( const Request &request )
  {
    AstraContext::ClearContext( request.Sender + ".flights", 0 );
    if ( !empty() ) { // ���� ���ᠦ��� - ��࠭塞 ��� ��।�����
      xmlDocPtr flightsDoc;
      xmlNodePtr node;
      flightsDoc = CreateXMLDoc( "flights" );
      try {
        node = flightsDoc->children;
        SetProp( node, "time", DateTimeToStr( request.lastRequestTime, ServerFormatDateTimeAsString ) );
        for ( changeFlights::iterator p=begin(); p!=end(); p++ ) {
          xmlNodePtr n = NewTextChild( node, "flight" );
          SetProp( n, "point_id", p->first );
          SetProp( n, "tid", p->second );
        }
        AstraContext::SetContext( request.Sender + ".flights", 0, XMLTreeToText( flightsDoc ) );
        //ProgTrace( TRACE5, "xmltreetotext=%s", XMLTreeToText( flightsDoc ).c_str() );
      }
      catch( ... ) {
        xmlFreeDoc( flightsDoc );
        throw;
      }
      xmlFreeDoc( flightsDoc );
    }
  }

  bool changeFlights::processFlights( Request &request, xmlDocPtr &docFlights )
  {
    if ( docFlights ) {
      xmlFreeDoc( docFlights );
    }
    docFlights = CreateXMLDoc( "flights" );
    xmlNodePtr node = docFlights->children;
    fromContext( request );
    ChangeFlightsDBData changeFlightsDBData;
    DB::TQuery changeFlightsQry(PgOra::getROSession("EXCH_FLIGHTS"),STDLOG);
    TDateTime nowUTC = NowUTC();
    request.lastRequestTime  = ((request.lastRequestTime < nowUTC - 1.0/24.0)? nowUTC - 1.0/24.0:request.lastRequestTime);
    changeFlightsDBData.createSQLRequest( request, changeFlightsQry );

    // �஡�� �� �ᥬ ���ᠦ�ࠬ � ������ �६� ����� ��� ࠢ�� ⥪�饬�
    int count_row = 0;
    int flight_count = 0;
    TDateTime max_time = ASTRA::NoExists;
    TQuery FltQry(&OraSession);
    FltQry.SQLText =
      "SELECT airline,flt_no,suffix,airp,craft,bort,act_in,est_in,scd_in,scd_out,est_out,act_out FROM points WHERE point_id=:point_id";
    FltQry.DeclareVariable( "point_id", otInteger );
    for ( ;!changeFlightsQry.Eof && flight_count<=MQRABBIT_TRANSPORT::MAX_SEND_FLIGHTS; changeFlightsQry.Next() ) {
      count_row++;
      int point_id = changeFlightsQry.FieldAsInteger( changeFlightsDBData.col_point_id );
      TAdvTripRoute route;
      if ( !route.GetRouteAfter(NoExists, point_id, trtWithCurrent, trtNotCancelled) ||
           route.empty() ) {
        continue;
      }
      //䨫��� �� ������ ३�
      int flt_no = route.front().flt_num_out;
      if ( flt_no < 0 || ( !request.flts.empty() && std::find( request.flts.begin(), request.flts.end(), flt_no ) == request.flts.end() ) ) {
        continue;
      }
      if ( max_time != NoExists && max_time != changeFlightsQry.FieldAsDateTime( changeFlightsDBData.col_time ) ) { // �ࠢ����� �६��� � �।. ���祭���, �᫨ ����������, �
        ProgTrace( TRACE5, "Flights.clear(), request.lastRequestTime=%s, max_time=%s",
                   DateTimeToStr( request.lastRequestTime, ServerFormatDateTimeAsString ).c_str(),
                   DateTimeToStr( max_time, ServerFormatDateTimeAsString ).c_str() );
        clear(); // ���������� �६� - 㤠�塞 ��� �।���� ���ᠦ�஢ � �।. �६����
      }
      max_time = changeFlightsQry.FieldAsDateTime( changeFlightsDBData.col_time );
      int tid = changeFlightsQry.FieldAsDateTime( changeFlightsDBData.col_tid );
      //���⪠ ������ �� ३��
/*      if ( !reqInfo->user.access.airlines().permitted( airline ) ||
           !reqInfo->user.access.airps().permitted( airp_dep ) )
        throw UserException( "MSG.FLIGHT.ACCESS_DENIED" );*/
/*      PaxData paxData( pax_id, point_id, max_time );
      PaxQry.SetVariable( "pax_id", pax_id );
      PaxQry.Execute();
      paxDBData.initPaxData( PaxQry );
      paxDBData.getPaxTids( PaxQry, paxData );*/
      // ���ᠦ�� ��।������ � �� ���������
      changeFlights::iterator ipoint_id = find( point_id );
      ProgTrace( TRACE5, "ipoint_id != end() %d", ipoint_id != end() );
      if ( ipoint_id != end() &&
           ipoint_id->second == tid ) {
        //ProgTrace( TRACE5, "itid->second.pax_tid=%d, itid->second.grp_tid =%d",  itid->second.pax_tid, itid->second.grp_tid );
        continue; // 㦥 ��।����� ���ᠦ��
      }
      // ३� �� ��।�����
      std::pair<std::map<int,int>::iterator, bool> ret;
      ret = insert( make_pair( point_id, tid ) );
      if ( !ret.second ) {
        ret.first->second = tid;
      }
      FlightData flightData( route );
      flight_count++;
      flightData.fromDB( FltQry );
      flightData.toXML( NewTextChild( node, "flight" ) );
    } //end for
    if ( max_time != NoExists )
      request.lastRequestTime = max_time;
    SetProp( node, "time", DateTimeToStr( request.lastRequestTime, ServerFormatDateTimeAsString ) );
    toContext( request );
    xml_encode_nodelist( docFlights->children );
    ProgTrace( TRACE5, "count_row=%d, flight_count=%d", count_row, flight_count );
    return flight_count > 0;
  }

} //end namespace EXCH_CHECKIN_RESULT

namespace MQRABBIT_TRANSPORT {
  MQRabbitParams::MQRabbitParams( const std::string &connect_str ) {
   std::size_t np = connect_str.find( ";" );
   if ( np != std::string::npos ) {
        addr = connect_str.substr( 0, np );
        queue = connect_str.substr( np + 1 );
    }
    LogTrace(TRACE5) << "addr=" << addr << ",queue=" << queue;
    if ( addr.empty() ||
         queue.empty() ) {
      ProgError( STDLOG, "MQRabbitParams: invalid connect string in file_params_sets" );
    }
  }
}

  void putMQRabbitPaxs( EXCH_CHECKIN_RESULT::Request &request, const std::map<std::string,std::string> &params )
  {
    TReqInfo::Instance()->user.sets.time = ustTimeUTC;
    std::map<std::string,std::string>::const_iterator iparam = params.find( MQRABBIT_TRANSPORT::PARAM_NAME_ADDR );
    if ( iparam == params.end() ||
         iparam->second.empty() ) {
      ProgError( STDLOG, "putMQRabbitPaxs: invalid connect string in file_params_sets" );
      return;
    }
    MQRABBIT_TRANSPORT::MQRabbitParams p( iparam->second );
    if ( p.addr.empty() ||
         p.queue.empty() ) {
      LogTrace(TRACE5) << "addr is empty";
      return;
    }
    std::map<std::string,std::string>::const_iterator it;
    if ( (it = params.find( MQRABBIT_TRANSPORT::PARAM_NAME_ACTIONCODE )) != params.end() ) {
      request.actionCode = it->second;
      if ( request.isAction( EXCH_CHECKIN_RESULT::Request::LocalTime ) ) {
        TReqInfo::Instance()->user.sets.time = ustTimeLocalAirp;
      }
    }
    try {
      //emptyHookTables();
      EXCH_CHECKIN_RESULT::changePaxs chPaxs;
      xmlDocPtr docPaxs = NULL;
      try {
        std::string lasttime;
        request.pr_reset = ( AstraContext::GetContext( request.Sender + ".lastReqTime", 0, lasttime ) == NoExists );
        if ( !request.pr_reset ) {
          request.pr_reset = ( StrToDateTime( lasttime.c_str(), "dd.mm.yyyy hh:nn:ss", request.lastRequestTime ) == EOF );
        }
        ProgTrace( TRACE5, "pr_reset=%d, lastRequestTime=%s", request.pr_reset, DateTimeToStr( request.lastRequestTime, "dd.mm.yyyy hh:nn:ss").c_str() );
        if ( !chPaxs.processPaxs( request, docPaxs ) ) {
           xmlFreeDoc( docPaxs );
           docPaxs = NULL;
        }
        //put to queue
        if ( docPaxs ) {
          jms::text_message in1;
          jms::connection cl( p.addr, false );
          jms::text_queue queue = cl.create_text_queue( p.queue );//"astra_exch/CRM_DATA/astra.tst.crm");
          in1.text = XMLTreeToText( docPaxs );
          queue.enqueue(in1);
          cl.commit();
        }
      }
      catch(...) {
        if ( docPaxs ) {
          xmlFreeDoc( docPaxs );
          docPaxs = NULL;
        }
        throw;
      }
      if ( docPaxs ) {
        xmlFreeDoc( docPaxs );
        docPaxs = NULL;
      }
      AstraContext::ClearContext( request.Sender + ".lastReqTime", 0 );
      if ( request.lastRequestTime != ASTRA::NoExists ) {
        AstraContext::SetContext( request.Sender + ".lastReqTime", 0, DateTimeToStr( request.lastRequestTime, "dd.mm.yyyy hh:nn:ss") );
      }
      callPostHooksBefore();
      ASTRA::commit();
      callPostHooksAfter();
    }
    catch( EOracleError &E ) {
      try { ASTRA::rollback(); } catch(...) {};
      LogError(STDLOG) << __FUNCTION__;
      E.showProgError();
    }
    catch( EXCEPTIONS::Exception &E ) {
      try { ASTRA::rollback(); } catch(...) {};
      LogError(STDLOG) << __FUNCTION__;
      ProgError( STDLOG, "Exception: %s", E.what());
    }
    catch( std::exception &E ) {
      try { ASTRA::rollback(); } catch(...) {};
      LogError(STDLOG) << __FUNCTION__;
      ProgError( STDLOG, "std::exception: %s", E.what());
    }
    catch( ... ) {
      try { ASTRA::rollback(); } catch(...) {};
      LogError(STDLOG) << __FUNCTION__;
      ProgError( STDLOG, "Unknown error");
    };
    callPostHooksAlways();
  }

  void putMQRabbitFlights( EXCH_CHECKIN_RESULT::Request &request, const std::map<std::string,std::string> &params )
  {
    TReqInfo::Instance()->user.sets.time = ustTimeUTC;
    std::map<std::string,std::string>::const_iterator iparam = params.find( MQRABBIT_TRANSPORT::PARAM_NAME_ADDR );
    if ( iparam == params.end() ||
         iparam->second.empty() ) {
      ProgError( STDLOG, "putMQRabbitFlights: invalid connect string in file_params_sets" );
      return;
    }
    MQRABBIT_TRANSPORT::MQRabbitParams p( iparam->second );
    if ( p.addr.empty() ||
         p.queue.empty() ) {
      LogTrace(TRACE5) << "addr is empty";
      return;
    }
    std::map<std::string,std::string>::const_iterator it;
    if ( (it = params.find( MQRABBIT_TRANSPORT::PARAM_NAME_ACTIONCODE )) != params.end() ) {
      request.actionCode = it->second;
      if ( request.isAction( EXCH_CHECKIN_RESULT::Request::LocalTime ) ) {
        TReqInfo::Instance()->user.sets.time = ustTimeLocalAirp;
      }
    }
    try {
      EXCH_CHECKIN_RESULT::changeFlights chFlights;
      xmlDocPtr docFlights = NULL;
      try {
        std::string lasttime;
        request.pr_reset = ( AstraContext::GetContext( request.Sender + "fl.lastReqTime", 0, lasttime ) == NoExists );
        if ( !request.pr_reset ) {
          request.pr_reset = ( StrToDateTime( lasttime.c_str(), "dd.mm.yyyy hh:nn:ss", request.lastRequestTime ) == EOF );
        }
        ProgTrace( TRACE5, "pr_reset=%d, lastRequestTime=%s", request.pr_reset, DateTimeToStr( request.lastRequestTime, "dd.mm.yyyy hh:nn:ss").c_str() );
        if ( !chFlights.processFlights( request, docFlights ) ) {
           xmlFreeDoc( docFlights );
           docFlights = NULL;
        }
        //put to queue
        if ( docFlights ) {
          jms::text_message in1;
          jms::connection cl( p.addr, false );
          jms::text_queue queue = cl.create_text_queue( p.queue );//"astra_exch/CRM_DATA/astra.tst.crm");
          in1.text = XMLTreeToText( docFlights );
          queue.enqueue(in1);
          cl.commit();
        }
      }
      catch(...) {
        if ( docFlights ) {
          xmlFreeDoc( docFlights );
          docFlights = NULL;
        }
        throw;
      }
      if ( docFlights ) {
        xmlFreeDoc( docFlights );
        docFlights = NULL;
      }
      AstraContext::ClearContext( request.Sender + "fl.lastReqTime", 0 );
      if ( request.lastRequestTime != ASTRA::NoExists ) {
        AstraContext::SetContext( request.Sender + "fl.lastReqTime", 0, DateTimeToStr( request.lastRequestTime, "dd.mm.yyyy hh:nn:ss") );
      }
      callPostHooksBefore();
      ASTRA::commit();
      callPostHooksAfter();
    }
    catch( EOracleError &E ) {
      try { ASTRA::rollback(); } catch(...) {};
      LogError(STDLOG) << __FUNCTION__;
      E.showProgError();
    }
    catch( EXCEPTIONS::Exception &E ) {
      try { ASTRA::rollback(); } catch(...) {};
      LogError(STDLOG) << __FUNCTION__;
      ProgError( STDLOG, "Exception: %s", E.what());
    }
    catch( std::exception &E ) {
      try { ASTRA::rollback(); } catch(...) {};
      LogError(STDLOG) << __FUNCTION__;
      ProgError( STDLOG, "std::exception: %s", E.what());
    }
    catch( ... ) {
      try { ASTRA::rollback(); } catch(...) {};
      LogError(STDLOG) << __FUNCTION__;
      ProgError( STDLOG, "Unknown error");
    };
    callPostHooksAlways();
  }

  void MQRABBIT_TRANSPORT::MQRSender::execute( const std::string& senderType )
  {
    TQuery Qry(&OraSession);
    Qry.SQLText =
      "SELECT point_addr, airline, airp, flt_no, param_name, param_value FROM file_param_sets, desks "
      " WHERE type=:type AND own_point_addr=:own_point_addr AND pr_send=1 AND "
      "       desks.code=file_param_sets.point_addr "
      "ORDER BY point_addr ";
    Qry.CreateVariable( "type", otString, senderType );
    Qry.CreateVariable( "own_point_addr", otString, OWN_POINT_ADDR() );
    Qry.Execute();
    MQRabbitRequest request;
    std::map<std::string,std::string> params;
    for ( ;!Qry.Eof; Qry.Next() ) {// �� ���� ��।��, ��� ���� ���� �⤠�� १����� ॣ����樨
      if ( request.Sender.empty() ) {
        request.Sender = Qry.FieldAsString( "point_addr" );
      }
      if ( request.Sender != Qry.FieldAsString( "point_addr" ) ) {
        send( senderType,
              request,
              params );
        request.clear();
        request.Sender = Qry.FieldAsString( "point_addr" );
        params.clear();
      }
      if ( MQRABBIT_TRANSPORT::PARAM_NAME_ADDR == Qry.FieldAsString( "param_name" ) ) {
        if ( !Qry.FieldIsNULL( "airp" ) ) {
          request.airps.push_back( Qry.FieldAsString( "airp" ) );
        }
        if ( !Qry.FieldIsNULL( "airline" ) ) {
          request.airlines.push_back( Qry.FieldAsString( "airline" ) );
        }
        if ( !Qry.FieldIsNULL( "flt_no" ) ) {
          request.flts.push_back( Qry.FieldAsInteger( "flt_no" ) );
        }
      }
      if ( params.find( Qry.FieldAsString( "param_name" ) ) == params.end() ) {
        params.insert( make_pair( Qry.FieldAsString( "param_name" ), Qry.FieldAsString( "param_value" ) ) );
      }
    }
    if ( !request.Sender.empty() ) {
      send( senderType,
            request,
            params );
      request.clear();
    }
  }

  static void exch_Checkin_or_Flights_result_handler(const char* handler_id, const char* task)
  {
    LogTrace(TRACE5) << "exch_Checkin_or_Flights_result_handler started, handler_id=" << handler_id << ",task=" << task;
    TReqInfo::Instance()->clear();
    TReqInfo::Instance()->user.sets.time = ustTimeUTC;
    emptyHookTables();
    TDateTime nowUTC=NowUTC();

    class ExchMQRSender: public MQRABBIT_TRANSPORT::MQRSender {
     public:
      virtual void send( const std::string& senderType,
                         const MQRABBIT_TRANSPORT::MQRabbitRequest &request,
                         std::map<std::string,std::string>& params ) {
        EXCH_CHECKIN_RESULT::Request req(request);
        if ( senderType == MQRABBIT_TRANSPORT::MQRABBIT_CHECK_IN_RESULT_OUT_TYPE ) {
          putMQRabbitPaxs( req, params );
        }
        else {
          putMQRabbitFlights( req, params );
        }
      }
    };
    ExchMQRSender exchMQRSender;
    exchMQRSender.execute( std::string( task ) == "paxs"?MQRABBIT_TRANSPORT::MQRABBIT_CHECK_IN_RESULT_OUT_TYPE:MQRABBIT_TRANSPORT::MQRABBIT_FLIGHTS_RESULT_OUT_TYPE );
    LogTrace(TRACE5) << "checkin_result_mqrabbit ended , handler_id=" << handler_id << ",task=" << task <<", executed " << DateTimeToStr( NowUTC() - nowUTC, "nn:ss" );
  }

/* DB */
/* CREATE TABLE exch_flights( POINT_ID NUMBER(9) NOT NULL,
                              TIME DATE NOT NULL,
                              TID  NUMBER(9) NOT NULL);
 * CREATE INDEX EXCH_FLIGHTS__IDX1  ON exch_flights(time,point_id) online;
 * INSERT INTO FILE_TYPES(code, name, in_order,thread_type) VALUES( 'MQRO', '��ࠢ�� ������� ॣ����樨 � Rabbit MQ', 1, NULL);
 * INSERT INTO FILE_TYPES(code, name, in_order,thread_type) VALUES( 'MQRF', '��ࠢ�� ������� �� ३�� � Rabbit MQ', 1, NULL);
 * CREATE SEQUENCE exch_flights__seq
        INCREMENT BY 1
        START WITH 1
        MAXVALUE 999999999
        CYCLE
        ORDER;


 *
*/
/*
 *
<document>
      <type>A</type>
      <issue_country>ABW</issue_country>
      <no>���������</no>
      <nationality>ABW</nationality>
      <birth_date>12.12.2010 00:00:00</birth_date>
      <gender>F</gender>
      <expiry_date>12.12.2020 00:00:00</expiry_date>
      <surname>����</surname>
      <pr_multi>1</pr_multi>
    </document>
    <doco>
      <type>V</type>
      <no>222222</no>
      <issue_place>WQDAD</issue_place>
      <issue_date>12.12.2010 00:00:00</issue_date>
      <expiry_date>12.12.2020 00:00:00</expiry_date>
      <applic_country>ABW</applic_country>
    </doco>
    <LuggageTag>
      <LuggageNumber>0298726413</LuggageNumber>
    </LuggageTag>

*/
