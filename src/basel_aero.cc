#include "basel_aero.h"

#include <string>
#include <vector>
#include <tcl.h>
#include "base_tables.h"
#include "misc.h"
#include "stl_utils.h"
#include "astra_consts.h"
#include "astra_utils.h"
#include "xml_unit.h"
#include "cache.h"
#include "passenger.h"
#include "baggage.h"
#include "events.h"
#include "points.h"
#include "stl_utils.h"
#include "jxtlib/xml_stuff.h"
#include "serverlib/logger.h"

#include "arx_daily_pg.h"
#define NICKNAME "DJEK"
#define NICKTRACE DJEK_TRACE
#include "serverlib/test.h"

using namespace std;
using namespace BASIC::date_time;
using namespace EXCEPTIONS;
using namespace ASTRA;

const std::string BASEL_AERO = "BASEL_AERO";

TBaselAeroAirps *TBaselAeroAirps::Instance()
{
  static TBaselAeroAirps *instance_ = 0;
  if ( !instance_ )
    instance_ = new TBaselAeroAirps();
  return instance_;
}

TBaselAeroAirps::TBaselAeroAirps()
{
  TQuery Qry(&OraSession);
  Qry.SQLText =
    "SELECT airp, dir FROM file_sets WHERE code=:code AND pr_denial=0";
  Qry.CreateVariable( "code", otString, BASEL_AERO );
  Qry.Execute();
  for ( ; !Qry.Eof; Qry.Next() ) {
    insert( make_pair( Qry.FieldAsString( "airp" ), Qry.FieldAsString( "dir" ) ) );
  }
}


bool is_sync_basel_pax( const TTripInfo &tripInfo )
{
  TBaselAeroAirps *airps = TBaselAeroAirps::Instance();
  for ( map<string,string>::iterator i=airps->begin(); i!=airps->end(); i++ ) {
    if ( i->first == tripInfo.airp )
      return true;
  };
  return false;
}

struct TBaselStat {
    TDateTime viewDate;
    string viewFlight;
    string viewName;
    int viewGroup;
    int viewPCT;
    int viewWeight;
    int viewCarryon;
    int viewPayWeight;
    string viewTag;
    string viewUncheckin;
    string viewStatus;
    int viewCheckinNo;
    string viewStation;
    string viewClientType;
    TDateTime viewCheckinTime;
    TDateTime viewChekinDuration;
  TDateTime viewBoardingTime;
    TDateTime viewDeparturePlanTime;
    TDateTime viewDepartureRealTime;
    string viewBagNorms;
    string viewPCTWeightPaidByType;
    string viewClass;
    int point_id;
    string airp;
    int pax_id;
};


void get_basel_aero_flight_stat( TDateTime part_key, int point_id, std::vector<TBaselStat> &stats );
void write_basel_aero_stat( TDateTime time_create, const std::vector<TBaselStat> &stats );
void read_basel_aero_stat( const string &airp, ofstream &f );

void sych_basel_aero_stat( TDateTime utcdate )
{
  TQuery Qry(&OraSession);
    Qry.SQLText =
    "SELECT points.point_id, NVL(act_out,NVL(est_out,scd_out)) real_time FROM points, trip_sets "
    "WHERE points.point_id=trip_sets.point_id AND points.time_out<=:vdate AND "
    "      pr_del=0 AND airp=:airp AND trip_sets.pr_basel_stat=0 AND pr_reg<>0";
  TDateTime trunc_date;
  modf( utcdate, &trunc_date );
  ProgTrace( TRACE5,"sych_basel_aero_stat: utcdate=%s, trunc_date=%s",
             DateTimeToStr( utcdate, "dd.mm.yyyy hh:nn" ).c_str(),
             DateTimeToStr(  trunc_date - 2, "dd.mm.yyyy hh:nn" ).c_str() );
  Qry.CreateVariable( "vdate", otDate, trunc_date - 2 );
  Qry.DeclareVariable( "airp", otString );
  TQuery TripSetsQry(&OraSession);
  TripSetsQry.SQLText =
    "UPDATE trip_sets SET pr_basel_stat=1 WHERE point_id=:point_id";
  TripSetsQry.DeclareVariable( "point_id", otInteger );
  TQuery ReWriteQry(&OraSession);
  ReWriteQry.SQLText =
    "SELECT last_create FROM file_sets WHERE airp=:airp AND code=:code";
  ReWriteQry.DeclareVariable( "airp", otString );
  ReWriteQry.CreateVariable( "code", otString, BASEL_AERO );
  TQuery DeleteQry(&OraSession);
  DeleteQry.SQLText =
    "DELETE basel_stat WHERE airp=:airp AND rownum <= 10000";
  DeleteQry.DeclareVariable( "airp", otString );
  TQuery FileSetsQry(&OraSession);
  FileSetsQry.SQLText =
    "UPDATE file_sets SET last_create=:vdate WHERE code=:code AND airp=:airp";
  FileSetsQry.CreateVariable( "vdate", otDate, utcdate );
  FileSetsQry.CreateVariable( "code", otString, BASEL_AERO );
  FileSetsQry.DeclareVariable( "airp", otString );

  std::vector<TBaselStat> stats;
  TBaselAeroAirps *airps = TBaselAeroAirps::Instance();
  if(!airps->size()) {
      LogTrace5 << " empty aero airps";
      return;
  }
  LogTrace5 << " basel airps size: " << airps->size();
  for ( map<string,string>::iterator iairp=airps->begin(); iairp!=airps->end(); iairp++ ) {
    ReWriteQry.SetVariable( "airp", iairp->first );
    ReWriteQry.Execute();
    bool clear_before_write = true;
    int last_year, last_month, last_day;
    int curr_year, curr_month, curr_day;
    if ( !ReWriteQry.FieldIsNULL( "last_create" ) ) { // �஢�ઠ �� �, �� ��᫥���� ����� �뫨 ᮡ࠭� � �।. ������
      DecodeDate( ReWriteQry.FieldAsDateTime( "last_create" ), last_year, last_month, last_day );
      DecodeDate( utcdate, curr_year, curr_month, curr_day );
      clear_before_write = ( curr_month != last_month || curr_year != last_year );
    }
    if ( clear_before_write ) { // ���� ������ ⠡���� �� ��ய����
       ProgTrace( TRACE5, "airp=%s, clear_before_write=%d, curr_month=%d, last_month=%d, curr_year=%d, last_year=%d",
                          iairp->first.c_str(), clear_before_write, curr_month, last_month, curr_year, last_year );
       DeleteQry.SetVariable( "airp", iairp->first );
       DeleteQry.Execute();
       while ( DeleteQry.RowsProcessed() ) {
         DeleteQry.Execute();
         ASTRA::commit();
       }
    }
    Qry.SetVariable( "airp", iairp->first );
    Qry.Execute();
    for ( ; !Qry.Eof; Qry.Next() ) {
      if ( Qry.FieldAsDateTime( "real_time" ) > utcdate - 2 ) {
        continue;
      }
      stats.clear();

          TFlights  flights;
          flights.Get( Qry.FieldAsInteger( "point_id" ), ftTranzit );
          flights.Lock(__FUNCTION__); //��稬 ���� �࠭���� ३�

      TripSetsQry.SetVariable( "point_id", Qry.FieldAsInteger( "point_id" ) );
      TripSetsQry.Execute(); //��稬 ३�
      get_basel_aero_flight_stat( ASTRA::NoExists, Qry.FieldAsInteger( "point_id" ), stats );
      if ( !stats.empty() ) {
        ProgTrace( TRACE5, "point_id=%d, stats.size()=%zu", Qry.FieldAsInteger( "point_id" ), stats.size() );
        write_basel_aero_stat( utcdate, stats );
      }
      ASTRA::commit();
    }
    ostringstream filename;
    filename <<iairp->second<<"ASTRA-" << ElemIdToElem(etAirp, iairp->first, efmtCodeNative, AstraLocale::LANG_EN)
               << "-" << DateTimeToStr(utcdate, "yyyymm") << ".csv";
    ofstream f;
    f.open( filename.str().c_str() );
    if (!f.is_open()) throw EXCEPTIONS::Exception("Can't open file '%s'",filename.str().c_str());
    try {
      read_basel_aero_stat( iairp->first, f );
      f.close();
      FileSetsQry.SetVariable( "airp", iairp->first );
      FileSetsQry.Execute();
      ASTRA::commit();
    }
    catch(...) {
      try { f.close(); } catch( ... ) { };
      try {
        //� ��砥 �訡�� ����襬 ���⮩ 䠩�
        f.open(filename.str().c_str());
        if (f.is_open()) f.close();
      }
      catch( ... ) { };
      throw;
    };
  }
}

void write_basel_aero_stat( TDateTime time_create, const std::vector<TBaselStat> &stats )
{
  TQuery Qry(&OraSession);
  Qry.SQLText =
    "INSERT INTO basel_stat(viewdate,viewflight,viewname,viewgroup,viewpct,viewweight,"
    "                       viewcarryon,viewpayweight,viewtag,viewuncheckin,viewstatus,"
    "                       viewcheckinno,viewstation,viewclienttype,viewcheckintime,"
    "                       viewcheckinduration,viewboardingtime,viewdepartureplantime,"
    "                       viewdeparturerealtime,viewbagnorms,viewpctweightpaidbytype,"
    "                       viewclass,point_id,airp,pax_id,time_create) "
    "VALUES(:viewdate,:viewflight,:viewname,:viewgroup,:viewpct,:viewweight,"
    "       :viewcarryon,:viewpayweight,:viewtag,:viewuncheckin,:viewstatus,"
    "       :viewcheckinno,:viewstation,:viewclienttype,:viewcheckintime,"
    "       :viewcheckinduration,:viewboardingtime,:viewdepartureplantime,"
    "       :viewdeparturerealtime,:viewbagnorms,:viewpctweightpaidbytype,"
    "       :viewclass,:point_id,:airp,:pax_id,:time_create)";
  Qry.DeclareVariable( "viewdate", otDate );
  Qry.DeclareVariable( "viewflight", otString );
  Qry.DeclareVariable( "viewname", otString );
  Qry.DeclareVariable( "viewgroup", otInteger );
  Qry.DeclareVariable( "viewpct", otInteger );
  Qry.DeclareVariable( "viewweight", otInteger );
  Qry.DeclareVariable( "viewcarryon", otInteger );
  Qry.DeclareVariable( "viewpayweight", otInteger );
  Qry.DeclareVariable( "viewtag", otString );
  Qry.DeclareVariable( "viewuncheckin", otString );
  Qry.DeclareVariable( "viewstatus", otString );
  Qry.DeclareVariable( "viewcheckinno", otInteger );
  Qry.DeclareVariable( "viewstation", otString );
  Qry.DeclareVariable( "viewclienttype", otString );
  Qry.DeclareVariable( "viewcheckintime", otDate );
  Qry.CreateVariable( "viewcheckinduration", otDate, FNull );
  Qry.DeclareVariable( "viewboardingtime", otDate );
  Qry.DeclareVariable( "viewdepartureplantime", otDate );
  Qry.DeclareVariable( "viewdeparturerealtime", otDate );
  Qry.DeclareVariable( "viewbagnorms", otString );
  Qry.DeclareVariable( "viewpctweightpaidbytype", otString );
  Qry.DeclareVariable( "viewclass", otString );
  Qry.DeclareVariable( "point_id", otInteger );
  Qry.DeclareVariable( "airp", otString );
  Qry.DeclareVariable( "pax_id", otInteger );
  Qry.CreateVariable( "time_create", otDate, time_create );


  for ( std::vector<TBaselStat>::const_iterator i=stats.begin(); i!=stats.end(); i++ ) {
    i->viewDate == NoExists?
      Qry.SetVariable( "viewdate", FNull ):
      Qry.SetVariable( "viewdate", i->viewDate );
    Qry.SetVariable( "viewflight", i->viewFlight );
    Qry.SetVariable( "viewname", i->viewName );
    i->viewGroup == NoExists?
      Qry.SetVariable( "viewgroup", FNull ):
      Qry.SetVariable( "viewgroup", i->viewGroup );
    i->viewPCT == NoExists?
      Qry.SetVariable( "viewpct", FNull ):
      Qry.SetVariable( "viewpct", i->viewPCT );
    i->viewWeight == NoExists?
      Qry.SetVariable( "viewweight", FNull ):
      Qry.SetVariable( "viewweight", i->viewWeight );
    i->viewCarryon == NoExists?
      Qry.SetVariable( "viewcarryon", FNull ):
      Qry.SetVariable( "viewcarryon", i->viewCarryon );
    i->viewPayWeight == NoExists?
      Qry.SetVariable( "viewpayweight", FNull ):
      Qry.SetVariable( "viewpayweight", i->viewPayWeight );
    Qry.SetVariable( "viewtag", i->viewTag );
    Qry.SetVariable( "viewuncheckin", i->viewUncheckin );
    Qry.SetVariable( "viewstatus", i->viewStatus );
    i->viewCheckinNo == NoExists?
      Qry.SetVariable( "viewcheckinno", FNull ):
      Qry.SetVariable( "viewcheckinno", i->viewCheckinNo );
    Qry.SetVariable( "viewstation", i->viewStation );
    Qry.SetVariable( "viewclienttype", i->viewClientType );
    i->viewCheckinTime == NoExists?
      Qry.SetVariable( "viewcheckintime", FNull ):
      Qry.SetVariable( "viewcheckintime", i->viewCheckinTime );
    i->viewBoardingTime == NoExists?
      Qry.SetVariable( "viewboardingtime", FNull ):
      Qry.SetVariable( "viewboardingtime", i->viewBoardingTime );
    i->viewDeparturePlanTime == NoExists?
      Qry.SetVariable( "viewdepartureplantime", FNull ):
      Qry.SetVariable( "viewdepartureplantime", i->viewDeparturePlanTime );
    i->viewDepartureRealTime == NoExists?
      Qry.SetVariable( "viewdeparturerealtime", FNull ):
      Qry.SetVariable( "viewdeparturerealtime", i->viewDepartureRealTime );
    Qry.SetVariable( "viewbagnorms", i->viewBagNorms );
    Qry.SetVariable( "viewpctweightpaidbytype", i->viewPCTWeightPaidByType );
    Qry.SetVariable( "viewclass", i->viewClass );
    Qry.SetVariable( "point_id", i->point_id );
    Qry.SetVariable( "airp", i->airp );
    i->pax_id == NoExists?
      Qry.SetVariable( "pax_id", FNull ):
      Qry.SetVariable( "pax_id", i->pax_id );
    Qry.Execute();
  }
}

void read_basel_aero_stat( const string &airp, ofstream &f )
{
  TQuery Qry(&OraSession);
  Qry.SQLText =
    "SELECT viewdate,viewflight,viewname,viewgroup,viewpct,viewweight,"
    "       viewcarryon,viewpayweight,viewtag,viewuncheckin,viewstatus,"
    "       viewcheckinno,viewstation,viewclienttype,viewcheckintime,"
    "       viewcheckinduration,viewboardingtime,viewdepartureplantime,"
    "       viewdeparturerealtime,viewbagnorms,viewpctweightpaidbytype,"
    "       viewclass FROM basel_stat "
    " WHERE airp=:airp ";
  Qry.CreateVariable( "airp", otString, airp );
  Qry.Execute();
  int viewdate_idx = Qry.FieldIndex( "viewdate" );
  int viewflight_idx = Qry.FieldIndex( "viewflight" );
  int viewname_idx = Qry.FieldIndex( "viewname" );
  int viewgroup_idx = Qry.FieldIndex( "viewgroup" );
  int viewpct_idx = Qry.FieldIndex( "viewpct" );
  int viewweight_idx = Qry.FieldIndex( "viewweight" );
  int viewcarryon_idx = Qry.FieldIndex( "viewcarryon" );
  int viewpayweight_idx = Qry.FieldIndex( "viewpayweight" );
  int viewtag_idx = Qry.FieldIndex( "viewtag" );
  int viewuncheckin_idx = Qry.FieldIndex( "viewuncheckin" );
  int viewstatus_idx = Qry.FieldIndex( "viewstatus" );
  int viewcheckinno_idx = Qry.FieldIndex( "viewcheckinno" );
  int viewstation_idx = Qry.FieldIndex( "viewstation" );
  int viewclienttype_idx = Qry.FieldIndex( "viewclienttype" );
  int viewcheckintime_idx = Qry.FieldIndex( "viewcheckintime" );
  int viewcheckinduration_idx = Qry.FieldIndex( "viewcheckinduration" );
  int viewboardingtime_idx = Qry.FieldIndex( "viewboardingtime" );
  int viewdepartureplantime_idx = Qry.FieldIndex( "viewdepartureplantime" );
  int viewdeparturerealtime_idx = Qry.FieldIndex( "viewdeparturerealtime" );
  int viewbagnorms_idx = Qry.FieldIndex( "viewbagnorms" );
  int viewpctweightpaidbytype_idx = Qry.FieldIndex( "viewpctweightpaidbytype" );
  int viewclass_idx = Qry.FieldIndex( "viewclass" );
  int fields_count = Qry.FieldsCount();
  for ( int idx=0; idx<fields_count; idx++ ) {
    f << string(Qry.FieldName( idx ));
    if ( idx < fields_count - 1 )
      f << ";";
  }
  f << endl;
  string region = AirpTZRegion( airp );
  ostringstream strline;
  for ( ; !Qry.Eof; Qry.Next() ) {
    if (Qry.FieldIsNULL( viewdate_idx )) strline << ";";
    else
      strline << (Qry.FieldAsDateTime( viewdate_idx ) == NoExists?"":DateTimeToStr(UTCToLocal(Qry.FieldAsDateTime( viewdate_idx ),region), "dd.mm.yyyy")) << ";";
    strline << Qry.FieldAsString( viewflight_idx ) << ";";
    strline << Qry.FieldAsString( viewname_idx ) << ";";
    strline << Qry.FieldAsInteger( viewgroup_idx ) << ";";
    if (Qry.FieldIsNULL( viewpct_idx )) strline << ";";
    else
      strline << (Qry.FieldAsInteger( viewpct_idx ) == 0?"":Qry.FieldAsString( viewpct_idx )) << ";";
    if (Qry.FieldIsNULL( viewweight_idx )) strline << ";";
    else
      strline << (Qry.FieldAsInteger( viewweight_idx ) == 0?"":Qry.FieldAsString( viewweight_idx )) << ";";
    if (Qry.FieldIsNULL( viewcarryon_idx )) strline << ";";
    else
      strline << (Qry.FieldAsInteger( viewcarryon_idx ) == 0?"":Qry.FieldAsString( viewcarryon_idx )) << ";";
    if (Qry.FieldIsNULL( viewpayweight_idx )) strline << ";";
    else
      strline << (Qry.FieldAsInteger( viewpayweight_idx ) == 0?"":Qry.FieldAsString( viewpayweight_idx )) << ";";
    strline << Qry.FieldAsString( viewtag_idx ) << ";";
    strline << Qry.FieldAsString( viewuncheckin_idx ) << ";";
    strline << Qry.FieldAsString( viewstatus_idx ) << ";";
    if (Qry.FieldIsNULL( viewcheckinno_idx )) strline << ";";
    else
      strline << (Qry.FieldAsInteger( viewcheckinno_idx ) == NoExists?"":Qry.FieldAsString( viewcarryon_idx )) << ";";
    strline << Qry.FieldAsString( viewstation_idx ) << ";";
    strline << Qry.FieldAsString( viewclienttype_idx ) << ";";
    if (Qry.FieldIsNULL( viewcheckintime_idx )) strline << ";";
    else
      strline << (Qry.FieldAsDateTime( viewcheckintime_idx ) == NoExists?"":DateTimeToStr(UTCToLocal(Qry.FieldAsDateTime( viewcheckintime_idx ),region), "dd.mm.yyyy hh:nn")) << ";";
    if (Qry.FieldIsNULL( viewcheckinduration_idx )) strline << ";";
    else
      strline << (Qry.FieldAsDateTime( viewcheckinduration_idx ) == NoExists?"":DateTimeToStr(UTCToLocal(Qry.FieldAsDateTime( viewcheckinduration_idx ),region), "dd.mm.yyyy hh:nn")) << ";";
    if (Qry.FieldIsNULL( viewboardingtime_idx )) strline << ";";
    else
      strline << (Qry.FieldAsDateTime( viewboardingtime_idx ) == NoExists?"":DateTimeToStr(UTCToLocal(Qry.FieldAsDateTime( viewboardingtime_idx ),region), "dd.mm.yyyy hh:nn")) << ";";
    if (Qry.FieldIsNULL( viewdepartureplantime_idx )) strline << ";";
    else
      strline << (Qry.FieldAsDateTime( viewdepartureplantime_idx ) == NoExists?"":DateTimeToStr(UTCToLocal(Qry.FieldAsDateTime( viewdepartureplantime_idx ),region), "dd.mm.yyyy hh:nn")) << ";";
    if (Qry.FieldIsNULL( viewdeparturerealtime_idx )) strline << ";";
    else
      strline << (Qry.FieldAsDateTime( viewdeparturerealtime_idx ) == NoExists?"":DateTimeToStr(UTCToLocal(Qry.FieldAsDateTime( viewdeparturerealtime_idx ),region), "dd.mm.yyyy hh:nn")) << ";";
    strline << Qry.FieldAsString( viewbagnorms_idx ) << ";";
    strline << Qry.FieldAsString( viewpctweightpaidbytype_idx ) << ";";
    strline << Qry.FieldAsString( viewclass_idx );
    f << ConvertCodepage( strline.str(), "CP866", "WINDOWS-1251" ) << endl;
    strline.str(string());
    strline.str();
  }
}

CheckIn::TBagItem fromDBO(const dbo::ARX_BAG2 & bag)
{
    CheckIn::TBagItem bagItem;
    if (dbo::isNotNull(bag.rfisc))
    {
        TRFISCKey tkey{};
        tkey.RFISC = bag.rfisc;
        tkey.service_type = ServiceTypes().decode(bag.service_type);
        tkey.airline = bag.airline;
        if (dbo::isNotNull(bag.list_id)) tkey.list_id = bag.list_id;
        tkey.getListItem();
        bagItem.pc = tkey;
    }
    else
    {
        TBagTypeKey bkey;
        if(dbo::isNotNull(bag.bag_type_str)) {
            bkey.bag_type = bag.bag_type_str;
        } else if (dbo::isNotNull(bag.bag_type)) {
          ostringstream s;
          s << setw(2) << setfill('0') << bag.bag_type;
          bkey.bag_type = s.str();
        }

        bkey.airline = bag.airline;
        if (dbo::isNotNull(bag.list_id))
          bkey.list_id = bag.list_id;

      bkey.getListItem();
      bagItem.wt = bkey;
    }
    bagItem.amount = bag.amount;
    bagItem.weight = bag.weight;

    bagItem.id = bag.id;
    bagItem.num = bag.num;
    bagItem.pr_cabin = bag.pr_cabin!=0;
    if (dbo::isNotNull(bag.value_bag_num)) {
        bagItem.value_bag_num = bag.value_bag_num;
    }
    bagItem.pr_liab_limit = bag.pr_liab_limit != 0;
    bagItem.to_ramp = bag.to_ramp != 0;
    bagItem.using_scales = bag.using_scales != 0;
    bagItem.bag_pool_num = bag.bag_pool_num;
    if (dbo::isNotNull(bag.hall)) {
        bagItem.hall = bag.hall;
    }
    if (dbo::isNotNull(bag.user_id)) {
        bagItem.user_id = bag.user_id;
    }
    bagItem.desk = bag.desk;
    if (dbo::isNotNull(bag.time_create)) {
        bagItem.time_create = BoostToDateTime(bag.time_create);
    }
    bagItem.is_trfer = bag.is_trfer != 0;
    bagItem.handmade = bag.handmade != 0;
    return bagItem;
};

std::string baselStatPaidInfo(const TBaselStat & stat, TDateTime part_key, int point_id, int grp_id,
                              bool piece_concept)
{
    LogTrace5 << " part_key: " << DateTimeToBoost(part_key) << " point_id: " << point_id;
    TPaidToLogInfo paidInfo;
    dbo::Session session;
    std::vector<dbo::ARX_BAG2> arx_bags = session.query<dbo::ARX_BAG2>()
            .from("ARX_PAX_GRP, ARX_BAG2")
            .where("arx_pax_grp.part_key=arx_bag2.part_key AND "
                   "      arx_pax_grp.grp_id=arx_bag2.grp_id AND "
                   "      arx_pax_grp.part_key=:part_key AND "
                   "      arx_pax_grp.point_dep=:point_id AND "
                   "      arx_pax_grp.grp_id=:grp_id ")
            .setBind({{":part_key", DateTimeToBoost(part_key)}, {":grp_id",stat.viewGroup}, {":point_id", point_id}});
    std::string gclass;
    int gbag_refuse;
    auto cur = make_db_curs("select CLASS, BAG_REFUSE from ARX_PAX_GRP "
                               "where PART_KEY=:part_key AND POINT_DEP=:point_id AND "
                               " GRP_ID=:grp_id",
                               PgOra::getROSession("ARX_PAX_GRP"));
    cur.stb()
       .def(gclass).def(gbag_refuse)
       .bind(":part_key", DateTimeToBoost(part_key)).bind(":point_id", point_id).bind(":grp_id",grp_id)
       .EXfet();
    std::vector<dbo::ARX_BAG2> ref_bags = algo::filter(arx_bags, [&](const auto & bag)
    {return PG_ARX::bag_pool_refused(bag.part_key, bag.grp_id, bag.bag_pool_num, gclass, gbag_refuse) == 0;});

    ostringstream str;
    if(!ref_bags.empty()) {
        for(const auto & bag : ref_bags) {
            paidInfo.add(fromDBO(bag));
        }

        if (!piece_concept)
        {
          WeightConcept::TPaidBagList paid;
          WeightConcept::PaidBagFromDB(part_key, stat.viewGroup, paid);
          for(const auto p : paid) {
            paidInfo.add(p);
          }
        }
        for(const auto & [key, item] : paidInfo.bag) {
            if(item.empty())                continue;
            if (!str.str().empty())         str << ", ";
            if (!key.bag_type_view.empty()) str << key.bag_type_view << ":";
            if (key.is_trfer)               str << "T:";
            str << item.amount << "/" << item.weight << "/" << item.paid;
        }
    }
    return str.str();
}


void get_basel_aero_arx_flight_stat(TDateTime part_key, int point_id, std::vector<TBaselStat> &stats )
{
    LogTrace5 << " part_key: " << DateTimeToBoost(part_key) << " point_id: " << point_id;
    stats.clear();
    TTripInfo operFlt;
    if (!operFlt.getByPointId(part_key, point_id, FlightProps(FlightProps::NotCancelled,
                                                  FlightProps::WithCheckIn))) return;
    TRegEvents events;
    events.fromDB(part_key, point_id);

    int grp_id = 0;
    std::string grp_class;
    int piece_concept = 0;
    int pax_id = NoExists;
    std::string surname;
    std::string name;
    std::string refuse;
    int pr_brd;
    int reg_no;
    int excess_pc;
    int bag_pool_num;
    int excess_wt;
    int excess;
    int bag_refuse;

    auto cur  = make_db_curs(
"SELECT arx_pax_grp.grp_id, arx_pax_grp.class, COALESCE(arx_pax_grp.piece_concept, 0) AS piece_concept, "
"       arx_pax.pax_id, arx_pax.surname, arx_pax.name, "
"       arx_pax.refuse, arx_pax.pr_brd, arx_pax.reg_no, "
"       arx_pax.excess_pc, arx_pax.bag_pool_num, arx_pax_grp.excess_wt, arx_pax_grp.excess, arx_pax_grp.bag_refuse "
"FROM arx_pax_grp  "
"LEFT OUTER JOIN arx_pax  ON arx_pax_grp.part_key = arx_pax.part_key AND "
"                            arx_pax_grp.grp_id   = arx_pax.grp_id "
"WHERE "
"      arx_pax_grp.part_key  =:part_key AND "
"      arx_pax_grp.point_dep =:point_id AND "
"      arx_pax_grp.status NOT IN ('E') "
"ORDER BY arx_pax.reg_no NULLS LAST, arx_pax.seats DESC NULLS LAST",
                PgOra::getROSession("ARX_PAX_GRP"));
    cur.stb()
       .def(grp_id)
       .defNull(grp_class, "")
       .defNull(piece_concept, ASTRA::NoExists)
       .defNull(pax_id, ASTRA::NoExists)
       .def(surname)
       .defNull(name, "")
       .defNull(refuse,"")
       .defNull(pr_brd, ASTRA::NoExists)
       .def(reg_no)
       .defNull(excess_pc, 0)
       .defNull(bag_pool_num, ASTRA::NoExists)
       .defNull(excess_wt, ASTRA::NoExists)
       .defNull(excess, ASTRA::NoExists)
       .def(bag_refuse)
       .bind(":part_key", DateTimeToBoost(part_key))
       .bind(":point_id", point_id)
       .exec();
    while(!cur.fen()) {
        PG_ARX::TBagInfo bag_info = PG_ARX::get_bagInfo2(DateTimeToBoost(part_key), grp_id, pax_id, bag_pool_num);
        int arch_excess_wt = PG_ARX::get_excess_wt(DateTimeToBoost(part_key), grp_id, pax_id, excess_wt, excess, bag_refuse).value_or(0);
        std::string tags  = PG_ARX::get_birks2(DateTimeToBoost(part_key), grp_id, pax_id, bag_pool_num,"RU").value_or("");
        TBaselStat stat;
        stat.point_id = point_id;
        stat.airp = operFlt.airp;
        stat.viewGroup = grp_id;
        stat.pax_id = pax_id;
        stat.viewDate = operFlt.scd_out;
        stat.viewFlight = operFlt.airline;
        string tmp = std::to_string(operFlt.flt_no);
        while ( tmp.size() < 3 ) tmp = "0" + tmp;
        stat.viewFlight += tmp + operFlt.suffix;
        if (stat.pax_id!=NoExists)
          stat.viewName = surname + '/' + name;
        else
          stat.viewName = "����� ��� �������������";
        stat.viewName = stat.viewName.substr(0,130);
        stat.viewPCT = bag_info.bagAmount;
        stat.viewWeight = bag_info.bagWeight;
        stat.viewCarryon = bag_info.rkWeight;
        stat.viewPayWeight = TComplexBagExcess(TBagPieces(excess_pc),TBagKilos(arch_excess_wt)).getDeprecatedInt();
        stat.viewTag = tags.substr(0,100);
        pair<TDateTime, TDateTime> times(NoExists, NoExists);
        WeightConcept::TPaxNormComplexContainer norms;
        if (stat.pax_id != NoExists)
        {
          stat.viewUncheckin = ElemIdToNameLong(etRefusalType, refuse).substr(0,50);
          stat.viewStatus = std::string(refuse.empty() ? (pr_brd==0?"��ॣ����஢��":"��襫 ��ᠤ��")
                                                       : "ࠧॣ����஢��").substr(0,30);
          stat.viewCheckinNo = reg_no;
          if (const auto it=events.find(make_pair(stat.viewGroup,stat.viewCheckinNo)); it!=events.end()) {
              times=it->second;
          }
          if (!refuse.empty() || pr_brd==0) times.second=NoExists;
          if (!piece_concept) {
            WeightConcept::PaxNormsFromDB(part_key, stat.pax_id, norms);
          }
        }
        else
        {
          stat.viewCheckinNo = NoExists;
          if (const auto it=events.find(make_pair(stat.viewGroup,NoExists)); it!=events.end()) {
              times=it->second;
          }
          if (!piece_concept) {
            WeightConcept::GrpNormsFromDB(part_key, stat.viewGroup, norms);
          }
        }
        stat.viewCheckinTime = times.first;
        stat.viewChekinDuration = NoExists;
        stat.viewBoardingTime = times.second;
        stat.viewDeparturePlanTime = operFlt.scd_out;
        stat.viewDepartureRealTime = operFlt.act_out?operFlt.act_out.get():NoExists;

        if (!piece_concept)
        {
          for(const WeightConcept::TPaxNormComplex& n : norms)
          {
            if (n.normNotExists()) continue;
            if (!stat.viewBagNorms.empty()) stat.viewBagNorms += ", ";
            if (!n.bag_type.empty()) stat.viewBagNorms += n.bag_type + ": ";
            stat.viewBagNorms += n.normStr(AstraLocale::LANG_RU);
          }
        }
        stat.viewPCTWeightPaidByType = baselStatPaidInfo(stat, part_key, point_id, grp_id, piece_concept);
        stat.viewClass = ElemIdToNameLong(etClass, grp_class);
        stats.push_back(stat);
    }
}

void get_basel_aero_flight_stat(TDateTime part_key, int point_id, std::vector<TBaselStat> &stats )
{
    if (part_key!=NoExists)
    {
        return get_basel_aero_arx_flight_stat(part_key, point_id, stats);
    }
  stats.clear();

  TTripInfo operFlt;
  if (!operFlt.getByPointId(part_key, point_id, FlightProps(FlightProps::NotCancelled,
                                                            FlightProps::WithCheckIn))) return;

  TRegEvents events;
  events.fromDB(part_key, point_id);
  string bag_sql;
  string bag_pc_sql;
  TQuery BagQry(&OraSession);
  TQuery TimeQry(&OraSession);

    bag_sql=
      "SELECT bag2.* "
      "FROM pax_grp,bag2 "
      "WHERE pax_grp.grp_id=bag2.grp_id AND "
      "      pax_grp.grp_id=:grp_id AND "
      "      ckin.bag_pool_refused(bag2.grp_id,bag2.bag_pool_num,pax_grp.class,pax_grp.bag_refuse)=0";
    bag_pc_sql=
      "SELECT bag2.* "
      "FROM bag2 "
      "WHERE bag2.grp_id=:grp_id AND "
      "      ckin.get_bag_pool_pax_id(bag2.grp_id,bag2.bag_pool_num,0)=:pax_id";

    TimeQry.SQLText =
      "SELECT time,NVL(stations.name,aodb_pax_change.desk) station, client_type, stations.airp "
      " FROM aodb_pax_change,stations "
      " WHERE pax_id=:pax_id AND aodb_pax_change.reg_no=:reg_no AND "
      "       aodb_pax_change.work_mode=:work_mode AND "
      "       aodb_pax_change.desk=stations.desk(+) AND aodb_pax_change.work_mode=stations.work_mode(+)";
    TimeQry.DeclareVariable( "pax_id", otInteger );
    TimeQry.DeclareVariable( "reg_no", otInteger );
    TimeQry.CreateVariable( "work_mode", otString, "�" );

  TQuery Qry(&OraSession);
  Qry.Clear();
  ostringstream sql;
  sql <<
    "SELECT pax_grp.grp_id, pax_grp.class, NVL(pax_grp.piece_concept, 0) AS piece_concept, "
    "       pax.pax_id, pax.surname, pax.name, "
    "       pax.refuse, pax.pr_brd, pax.reg_no, "
    "         "
    "ckin.get_bagAmount2(pax_grp.grp_id,pax.pax_id,pax.bag_pool_num,rownum) AS bag_amount, "
    "ckin.get_bagWeight2(pax_grp.grp_id,pax.pax_id,pax.bag_pool_num,rownum) AS bag_weight, "
    "ckin.get_rkWeight2(pax_grp.grp_id,pax.pax_id,pax.bag_pool_num,rownum) AS rk_weight, "
    "ckin.get_excess_wt(pax.grp_id, pax.pax_id, pax_grp.excess_wt, pax_grp.bag_refuse) AS excess_wt, "
    "ckin.get_excess_pc(pax.grp_id, pax.pax_id) AS excess_pc, "
    "ckin.get_birks2(pax_grp.grp_id,pax.pax_id,pax.bag_pool_num,'RU') AS tags, "
    "ckin.get_main_pax_id2(pax_grp.grp_id) AS main_pax_id "
    "FROM pax_grp, pax "
    "WHERE pax_grp.grp_id=pax.grp_id(+) AND "
    "      pax_grp.point_dep=:point_id AND "
    "      pax_grp.status NOT IN ('E') "
    "ORDER BY pax.reg_no NULLS LAST, pax.seats DESC NULLS LAST";

  Qry.SQLText=sql.str().c_str();
  Qry.CreateVariable("point_id", otInteger, point_id);
  Qry.Execute();
  for(;!Qry.Eof;Qry.Next())
  {
    bool piece_concept=Qry.FieldAsInteger("piece_concept")!=0;
    int main_pax_id=Qry.FieldIsNULL("main_pax_id")?NoExists:Qry.FieldAsInteger("main_pax_id");

    TBaselStat stat;
    stat.point_id = point_id;
    stat.airp = operFlt.airp;
    stat.viewGroup = Qry.FieldAsInteger("grp_id");
    stat.pax_id = Qry.FieldIsNULL("pax_id")?NoExists:Qry.FieldAsInteger("pax_id");
    stat.viewDate = operFlt.scd_out;
    stat.viewFlight = operFlt.airline;
    string tmp = IntToString( operFlt.flt_no );
    while ( tmp.size() < 3 ) tmp = "0" + tmp;
    stat.viewFlight += tmp + operFlt.suffix;
    if (stat.pax_id!=NoExists)
      stat.viewName = string(Qry.FieldAsString("surname")) + "/" + Qry.FieldAsString("name");
    else
      stat.viewName = "����� ��� �������������";
    stat.viewName = stat.viewName.substr(0,130);
    stat.viewPCT = Qry.FieldAsInteger("bag_amount");
    stat.viewWeight = Qry.FieldAsInteger("bag_weight");
    stat.viewCarryon = Qry.FieldAsInteger("rk_weight");
    stat.viewPayWeight = TComplexBagExcess(TBagPieces(Qry.FieldAsInteger("excess_pc")),
                                           TBagKilos(Qry.FieldAsInteger("excess_wt"))).getDeprecatedInt();
    stat.viewTag = string(Qry.FieldAsString("tags")).substr(0,100);
    pair<TDateTime, TDateTime> times(NoExists, NoExists);
    WeightConcept::TPaxNormComplexContainer norms;
    if (stat.pax_id!=NoExists)
    {
      stat.viewUncheckin = ElemIdToNameLong(etRefusalType, Qry.FieldAsString("refuse")).substr(0,50);
      stat.viewStatus = string(Qry.FieldIsNULL("refuse")?(Qry.FieldAsInteger("pr_brd")==0?"��ॣ����஢��":"��襫 ��ᠤ��"):"ࠧॣ����஢��").substr(0,30);
      stat.viewCheckinNo = Qry.FieldAsInteger("reg_no");
      map< pair<int, int>, pair<TDateTime, TDateTime> >::const_iterator i=events.find(make_pair(stat.viewGroup,stat.viewCheckinNo));
      if (i!=events.end()) times=i->second;
      if (!Qry.FieldIsNULL("refuse") || Qry.FieldAsInteger("pr_brd")==0) times.second=NoExists;

      if (!piece_concept)
        WeightConcept::PaxNormsFromDB(part_key, stat.pax_id, norms);
    }
    else
    {
      stat.viewCheckinNo = NoExists;
      map< pair<int, int>, pair<TDateTime, TDateTime> >::const_iterator i=events.find(make_pair(stat.viewGroup,NoExists));
      if (i!=events.end()) times=i->second;

      if (!piece_concept)
        WeightConcept::GrpNormsFromDB(part_key, stat.viewGroup, norms);
    };
    stat.viewCheckinTime = times.first;
    stat.viewChekinDuration = NoExists;
    stat.viewBoardingTime = times.second;
    stat.viewDeparturePlanTime = operFlt.scd_out;
    stat.viewDepartureRealTime = operFlt.act_out?operFlt.act_out.get():NoExists;

    if (!piece_concept)
    {
      for(const WeightConcept::TPaxNormComplex& n : norms)
      {
        if (n.normNotExists()) continue;
        if (!stat.viewBagNorms.empty()) stat.viewBagNorms += ", ";
        if (!n.bag_type.empty())
          stat.viewBagNorms += n.bag_type + ": ";
        stat.viewBagNorms += n.normStr(AstraLocale::LANG_RU);
      }
    };

    TPaidToLogInfo paidInfo;
    BagQry.Clear();
    BagQry.CreateVariable("grp_id", otInteger, stat.viewGroup);
    if (piece_concept && stat.pax_id!=NoExists)
    {
      BagQry.SQLText=bag_pc_sql;
      BagQry.CreateVariable("pax_id", otInteger, stat.pax_id);
    }
    else if (stat.pax_id==NoExists || stat.pax_id==main_pax_id)
    {
      BagQry.SQLText = bag_sql;
    };
    if (!BagQry.SQLText.IsEmpty())
    {
      BagQry.Execute();
      if (!BagQry.Eof)
      {
        for(;!BagQry.Eof;BagQry.Next())
        {
          CheckIn::TBagItem bagItem;
          bagItem.fromDB(BagQry);
          paidInfo.add(bagItem);
        };
        if (!piece_concept)
        {
          WeightConcept::TPaidBagList paid;
          WeightConcept::PaidBagFromDB(part_key, stat.viewGroup, paid);
          for(WeightConcept::TPaidBagList::const_iterator p=paid.begin(); p!=paid.end(); ++p)
            paidInfo.add(*p);
        }
        else
        {
            TPaidRFISCListWithAuto paid;
            paid.fromDB(stat.pax_id==NoExists?stat.viewGroup:stat.pax_id, stat.pax_id==NoExists);
            for(TPaidRFISCListWithAuto::const_iterator p=paid.begin(); p!=paid.end(); ++p)
            {
              if (p->second.trfer_num!=0) continue;
              paidInfo.add(p->second);
            };
        };
        ostringstream str;
        for(map<TEventsSumBagKey, TEventsSumBagItem>::const_iterator b=paidInfo.bag.begin(); b!=paidInfo.bag.end(); ++b)
        {
          if (b->second.empty()) continue;

          if (!str.str().empty()) str << ", ";
          if (!b->first.bag_type_view.empty())
            str << b->first.bag_type_view << ":";
          if (b->first.is_trfer)
            str << "T:";
          str << b->second.amount << "/" << b->second.weight << "/" << b->second.paid;
        };
        stat.viewPCTWeightPaidByType=str.str();
      };
    };

    stat.viewClass = ElemIdToNameLong(etClass, Qry.FieldAsString("class"));
    if ( stat.pax_id!=NoExists ) {
      TimeQry.SetVariable( "pax_id", stat.pax_id );
      TimeQry.SetVariable( "reg_no", stat.viewCheckinNo );
      TimeQry.Execute();
      if ( !TimeQry.Eof ) {
        if ( string(TimeQry.FieldAsString( "airp" )) == stat.airp )
          stat.viewStation = TimeQry.FieldAsString( "station" );
        stat.viewClientType = TimeQry.FieldAsString( "client_type" );
      }
    }
    stats.push_back( stat );
  };
};

int arx_basel_stat(int argc, char **argv)
{
    TDateTime part_key=ASTRA::NoExists;
    int point_id=ASTRA::NoExists;
    if(argc >=3) {
        std::string date_time = std::string(argv[1]) +" "+ std::string(argv[2]);
        if(StrToDateTime(date_time.c_str(), "dd.mm.yyyy hh:nn:ss", part_key) == EOF) {
            cout << "wrong part_key: " << date_time << endl;
            return 1;
        }
        if(StrToInt(argv[3], point_id ) == EOF) {
            cout << "wrong integer point_id: " << argv[3] << endl;
            return 1;
        }
    } else {
        if(StrToInt(argv[1], point_id ) == EOF) {
            cout << "wrong integer point_id: " << argv[1] << endl;
            return 1;
        }
    }
    std::vector<TBaselStat> stats;
    get_basel_aero_flight_stat(part_key, point_id, stats);
    write_basel_aero_stat( NowUTC(), stats);

    return 0;
}

int basel_stat(int argc,char **argv)
{
  TDateTime part_key=ASTRA::NoExists;
  int point_id=ASTRA::NoExists;

  char buf[200];
  int res;
  char c;

  for(int pass=0; pass<3; pass++)
  {
    printf("input part_key(dd.mm.yy hh:nn:ss): ");
    *buf=0;
    c=0;
    res=scanf("%[^\n]", buf);
    scanf("%c", &c);
    if (res==1 && *buf!=0)
    {
      if (StrToDateTime(buf, "dd.mm.yy hh:nn:ss", part_key)==EOF)
      {
        printf("wrong part_key!\n");
        if (pass<2) continue; else return 0;
      }
    };
    break;
  };

  for(int pass=0; pass<3; pass++)
  {
    printf("input point_id: ");
    *buf=0;
    c=0;
    res=scanf("%[^\n]", buf);
    scanf("%c", &c);
    if (res==1 && *buf!=0)
    {
      if (StrToInt(buf, point_id)==EOF)
      {
        printf("wrong point_id!\n");
        if (pass<2) continue; else return 0;
      }
    };
    break;
  };

  std::vector<TBaselStat> stats;
  get_basel_aero_flight_stat(part_key, point_id, stats);
  write_basel_aero_stat( NowUTC(), stats);

  return 0;
};

