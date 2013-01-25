#include <stdlib.h>
#include "sopp.h"
#include "stages.h"
#include "points.h"
#include "pers_weights.h"
#include "astra_utils.h"
#include "astra_misc.h"
#include "stl_utils.h"
#include "oralib.h"
#include "xml_unit.h"
#include "basic.h"
#include "misc.h"
#include "exceptions.h"
#include "sys/times.h"
#include <map>
#include <vector>
#include <string>
#include "tripinfo.h"
#include "season.h" //???
#include "telegram.h"
#include "boost/date_time/local_time/local_time.hpp"
#include <boost/thread/thread.hpp>
//#include <boost/threadpool.hpp>
//#include "boost/asio.hpp"
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp> 
#include "base_tables.h"
#include "docs.h"
#include "stat.h"
#include "salons.h"
#include "seats.h"
#include "term_version.h"
#include "tlg/tlg_binding.h"

#include "aodb.h"
#include "serverlib/perfom.h"

#define NICKNAME "DJEK"
#include "serverlib/test.h"

using namespace std;
using namespace BASIC;
using namespace EXCEPTIONS;
using namespace AstraLocale;
using namespace ASTRA;
using namespace boost::local_time;

#define NOT_CHANGE_AIRLINE_FLT_NO_SCD_

enum TModule { tSOPP, tISG, tSPPCEK };

const char* points_SOPP_SQL =
    "SELECT points.move_id,points.point_id,point_num,airp,airp_fmt,first_point,airline,airline_fmt,flt_no,"
    "       suffix,suffix_fmt,craft,craft_fmt,bort, "
    "       scd_in,est_in,act_in,scd_out,est_out,act_out,trip_type,litera,park_in,park_out,remark,"
    "       pr_tranzit,pr_reg,points.pr_del pr_del,points.tid tid "
    " FROM points, "
    " (SELECT DISTINCT move_id FROM points "
    "   WHERE points.pr_del!=-1 "
    "         :where_sql AND "
    "         ( time_in >= :first_date AND time_in < :next_date OR "
    "           time_out >= :first_date AND time_out < :next_date ) ) p "
    "WHERE points.move_id = p.move_id AND "
    "      points.pr_del!=-1 "
    "ORDER BY points.move_id,point_num,point_id ";
const char* points_id_SOPP_SQL =
    "SELECT points.move_id,points.point_id,point_num,airp,airp_fmt,first_point,airline,airline_fmt,flt_no,"
    "       suffix,suffix_fmt,craft,craft_fmt,bort, "
    "       scd_in,est_in,act_in,scd_out,est_out,act_out,trip_type,litera,park_in,park_out,remark,"
    "       pr_tranzit,pr_reg,points.pr_del pr_del,points.tid tid "
    " FROM points, "
    " ( SELECT move_id FROM points WHERE point_id=:point_id AND pr_del!=-1 AND ROWNUM<2 ) a "
    "   WHERE pr_del!=-1 AND points.move_id=a.move_id "
    "ORDER BY points.move_id,point_num,point_id ";
const char* points_ISG_SQL =
    "SELECT points.move_id,points.point_id,point_num,airp,airp_fmt,first_point,airline,airline_fmt,flt_no,"
    "       suffix,suffix_fmt,craft,craft_fmt,bort, trip_crew.commander, trip_crew.cockpit, trip_crew.cabin, "
    "       scd_in,est_in,act_in,scd_out,est_out,act_out,trip_type,litera,park_in,park_out,remark,"
    "       pr_tranzit,pr_reg,points.pr_del pr_del,points.tid tid, reference ref "
    " FROM points, move_ref, trip_crew, "
    " (SELECT DISTINCT move_id FROM points "
    "   WHERE points.pr_del!=-1 "
    "         :where_sql AND "
    "         ( time_in >= :first_date AND time_in < :next_date OR "
    "           time_out >= :first_date AND time_out < :next_date OR "
    "           time_in = TO_DATE('01.01.0001','DD.MM.YYYY') AND time_out = TO_DATE('01.01.0001','DD.MM.YYYY') ) ) p "
    "WHERE points.move_id = p.move_id AND "
    "      points.point_id = trip_crew.point_id(+) and "
    "      move_ref.move_id = p.move_id AND "
    "      points.pr_del!=-1 "
    "ORDER BY points.move_id,point_num,point_id ";
const char * arx_points_SOPP_SQL =
    "SELECT arx_points.move_id,arx_points.point_id,point_num,airp,airp_fmt,first_point,airline,airline_fmt,flt_no, \n"
    "       suffix,suffix_fmt,craft,craft_fmt,bort, \n"
    "       scd_in,est_in,act_in,scd_out,est_out,act_out,trip_type,litera,park_in,park_out,remark, \n"
    "       pr_tranzit,pr_reg,arx_points.pr_del pr_del,arx_points.tid tid, arx_points.part_key \n"
    " FROM arx_points, \n"
    " (SELECT DISTINCT move_id, part_key FROM arx_points \n"
    "   WHERE part_key>=:first_date AND part_key<:next_date+:arx_trip_date_range AND \n"
    "         pr_del!=-1 \n"
    "         :where_sql AND \n"
    "         ( :first_date IS NULL OR \n"
    "           NVL(act_in,NVL(est_in,scd_in)) >= :first_date AND NVL(act_in,NVL(est_in,scd_in)) < :next_date OR \n"
    "           NVL(act_out,NVL(est_out,scd_out)) >= :first_date AND NVL(act_out,NVL(est_out,scd_out)) < :next_date ) \n"
    "  UNION \n"
    "  SELECT DISTINCT arx_points.move_id, arx_points.part_key \n"
    "  FROM arx_points, \n"
    "       (SELECT part_key, move_id FROM move_arx_ext \n"
    "        WHERE part_key >= :next_date+:arx_trip_date_range AND part_key <= :next_date+date_range) arx_ext \n"
    "   WHERE arx_points.part_key=arx_ext.part_key AND arx_points.move_id=arx_ext.move_id AND \n"
    "         pr_del!=-1 \n"
    "         :where_sql AND \n"
    "         ( :first_date IS NULL OR \n"
    "           NVL(act_in,NVL(est_in,scd_in)) >= :first_date AND NVL(act_in,NVL(est_in,scd_in)) < :next_date OR \n"
    "           NVL(act_out,NVL(est_out,scd_out)) >= :first_date AND NVL(act_out,NVL(est_out,scd_out)) < :next_date ) \n"
    " ) p \n"
    "WHERE arx_points.part_key = p.part_key AND \n"
    "      arx_points.move_id = p.move_id AND \n"
    "      arx_points.pr_del!=-1 \n"
    "ORDER BY arx_points.move_id,point_num,point_id";
const char * arx_points_ISG_SQL =
    "SELECT arx_points.move_id,arx_points.point_id,point_num,airp,airp_fmt,first_point,airline,airline_fmt,flt_no, \n"
    "       suffix,suffix_fmt,craft,craft_fmt,bort, \n"
    "       scd_in,est_in,act_in,scd_out,est_out,act_out,trip_type,litera,park_in,park_out,remark, \n"
    "       pr_tranzit,pr_reg,arx_points.pr_del pr_del,arx_points.tid tid, reference ref, arx_points.part_key \n"
    " FROM arx_points, arx_move_ref, \n"
    " (SELECT DISTINCT move_id, part_key FROM arx_points \n"
    "   WHERE part_key>=:first_date AND part_key<:next_date+:arx_trip_date_range AND \n"
    "         pr_del!=-1 \n"
    "         :where_sql AND \n"
    "         ( :first_date IS NULL OR \n"
    "           NVL(act_in,NVL(est_in,scd_in)) >= :first_date AND NVL(act_in,NVL(est_in,scd_in)) < :next_date OR \n"
    "           NVL(act_out,NVL(est_out,scd_out)) >= :first_date AND NVL(act_out,NVL(est_out,scd_out)) < :next_date OR \n"
    "           NVL(act_in,NVL(est_in,scd_in)) IS NULL AND NVL(act_out,NVL(est_out,scd_out)) IS NULL ) \n"
    "  UNION \n"
    "  SELECT DISTINCT arx_points.move_id, arx_points.part_key \n"
    "  FROM arx_points, \n"
    "       (SELECT part_key, move_id FROM move_arx_ext \n"
    "        WHERE part_key >= :next_date+:arx_trip_date_range AND part_key <= :next_date+date_range) arx_ext \n"
    "   WHERE arx_points.part_key=arx_ext.part_key AND arx_points.move_id=arx_ext.move_id AND \n"
    "         pr_del!=-1 \n"
    "         :where_sql AND \n"
    "         ( :first_date IS NULL OR \n"
    "           NVL(act_in,NVL(est_in,scd_in)) >= :first_date AND NVL(act_in,NVL(est_in,scd_in)) < :next_date OR \n"
    "           NVL(act_out,NVL(est_out,scd_out)) >= :first_date AND NVL(act_out,NVL(est_out,scd_out)) < :next_date OR \n"
    "           NVL(act_in,NVL(est_in,scd_in)) IS NULL AND NVL(act_out,NVL(est_out,scd_out)) IS NULL ) \n"
    " ) p \n"
    "WHERE arx_points.part_key=p.part_key AND \n"
    "      arx_points.move_id = p.move_id AND \n"
    "      arx_move_ref.part_key=p.part_key AND \n"
    "      arx_move_ref.move_id = p.move_id AND \n"
    "      arx_points.pr_del!=-1 \n"
    "ORDER BY arx_points.move_id,point_num,point_id";
const char* classesSQL =
    "SELECT class,cfg "\
    " FROM trip_classes,classes "\
    "WHERE trip_classes.point_id=:point_id AND trip_classes.class=classes.code "\
    "ORDER BY priority";

const char* arx_classesSQL =
    "SELECT class,cfg "\
    " FROM arx_trip_classes,classes "\
    "WHERE part_key=:part_key AND arx_trip_classes.point_id=:point_id AND arx_trip_classes.class=classes.code "\
    "ORDER BY priority";
const char* regSQL =
    "SELECT SUM(tranzit)+SUM(ok)+SUM(goshow) AS reg FROM counters2 "
    "WHERE point_dep=:point_id";
const char* arx_regSQL =
    "SELECT SUM(arx_pax.seats) as reg "
    " FROM arx_pax_grp, arx_pax "
    " WHERE arx_pax_grp.part_key=:part_key AND arx_pax_grp.point_dep=:point_id AND "\
    "       arx_pax.part_key=:part_key AND "
    "       arx_pax_grp.grp_id=arx_pax.grp_id AND arx_pax.pr_brd IS NOT NULL ";
const char* resaSQL =
    "SELECT ckin.get_crs_ok(:point_id) as resa FROM dual ";
const char *stagesSQL =
    "SELECT trip_stages.stage_id,scd,est,act,pr_auto,pr_manual"
    " FROM trip_stages "
    " WHERE trip_stages.point_id=:point_id";
const char *arx_stagesSQL =
    "SELECT stage_id,scd,est,act,pr_auto,pr_manual FROM arx_trip_stages "
    " WHERE part_key=:part_key AND point_id=:point_id "
    " ORDER BY stage_id ";
/*const char* stationsSQL =
    "SELECT stations.name,stations.work_mode,trip_stations.pr_main FROM stations,trip_stations "\
    " WHERE point_id=:point_id AND stations.desk=trip_stations.desk AND stations.work_mode=trip_stations.work_mode "\
    " ORDER BY stations.work_mode,stations.name";
*/
const char* trfer_out_SQL =
    "SELECT 1 "
    " FROM tlg_binding,tlg_transfer "
    "WHERE tlg_binding.point_id_spp=:point_id AND tlg_binding.point_id_tlg=tlg_transfer.point_id_in AND rownum<2";

const char* trfer_in_SQL =
    "SELECT 1 "
    " FROM tlg_binding,tlg_transfer "
    "WHERE tlg_binding.point_id_spp=:point_id AND tlg_binding.point_id_tlg=tlg_transfer.point_id_out AND rownum<2";
const char* crs_displace_from_SQL =
   "SELECT class_spp,airp_arv_spp,airline,flt_no,suffix,scd, "
   "       tlg_binding.point_id_spp,class_tlg,airp_arv_tlg "
   " FROM crs_displace2,tlg_binding "
   " WHERE crs_displace2.point_id_spp=:point_id_spp AND crs_displace2.point_id_tlg=tlg_binding.point_id_tlg(+)"
   " ORDER BY tlg_binding.point_id_spp,airline,flt_no,suffix,scd ";
const char* crs_displace_to_SQL =
  "SELECT class_tlg AS class_spp, airp_arv_tlg AS airp_arv_spp, "
  "       points.airp airp_dep,points.airline,points.flt_no,points.suffix,points.scd_out AS scd, "
  "       crs_displace2.point_id_spp AS point_id_spp,class_spp AS class_tlg,airp_arv_spp AS airp_arv_tlg "
  " FROM tlg_binding, crs_displace2, points "
  " WHERE tlg_binding.point_id_spp=:point_id_spp AND "
  "       tlg_binding.point_id_tlg=crs_displace2.point_id_tlg AND "
  "       points.point_id=crs_displace2.point_id_spp " //!!! points.pr_del != -1
  "ORDER BY points.point_id,points.airline,points.flt_no,points.suffix,points.scd_out ";
const char* arx_trip_delays_SQL =
  "SELECT delay_num,delay_code,time "
  " FROM arx_trip_delays "
  "WHERE arx_trip_delays.part_key=:part_key AND arx_trip_delays.point_id=:point_id "
  "ORDER BY delay_num";
const char* trip_delays_SQL =
  "SELECT delay_code,time "
  " FROM trip_delays "
  "WHERE point_id=:point_id "
  "ORDER BY delay_num";

typedef vector<TSoppStage> tstages;

typedef map<int,TSOPPDests> tmapds;

struct tcrs_displ {
  int point_id_spp;
  string airp_arv_spp;
  string class_spp;
  string airline;
  int flt_no;
  string suffix;
  TDateTime scd;
  int point_id_tlg;
  string to_airp_dep;
  string airp_arv_tlg;
  string class_tlg;
  string status;
};

struct change_act {
  int point_id;
  TDateTime old_act;
  TDateTime act;
  bool pr_land;
};

void read_tripStages( vector<TSoppStage> &stages, TDateTime part_key, int point_id );
void build_TripStages( const vector<TSoppStage> &stages, const string &region, xmlNodePtr tripNode, bool pr_isg );
string getCrsDisplace( int point_id, TDateTime local_time, bool to_local, TQuery &Qry );

void ChangeACT_OUT( int point_id, TDateTime old_act, TDateTime act );
void ChangeACT_IN( int point_id, TDateTime old_act, TDateTime act );

enum TSOPPTripChange { tsNew, tsDelete, tsAttr, tsAddFltOut, tsDelFltOut, tsCancelFltOut, tsRestoreFltOut };
void ChangeTrip( int point_id, TSOPPTrip tr1, TSOPPTrip tr2, BitSet<TSOPPTripChange> FltChange );

void get_TrferExists( int point_id, bool &trfer_exists );
void get_DesksGates( int point_id, string &ckin_desks, string &gates );
void check_DesksGates( int point_id );

string GetRemark( string remark, TDateTime scd_out, TDateTime est_out, string region )
{
	string rem = remark;
	if ( est_out > NoExists && scd_out != est_out ) {
		rem = AstraLocale::getLocaleText( "����প� ��" ) + " " + DateTimeToStr( UTCToClient( est_out, region ), "dd hh:nn" ) + remark;
	}
	return rem;
}

TSOPPTrip createTrip( int move_id, TSOPPDests::iterator &id, TSOPPDests &dests )
{
  TSOPPTrip trip;
  trip.move_id = move_id;
  trip.point_id = id->point_id;
  trip.tid = id->tid;
  int first_point;
  if ( id->pr_tranzit )
    first_point = id->first_point;
  else
    first_point = id->point_id;
  TSOPPDests::iterator pd = dests.end();
  bool next_airp = false;
  for ( TSOPPDests::iterator fd=dests.begin(); fd!=dests.end(); fd++ ) {

  	if ( fd->point_num < id->point_num ) {
  		if ( id->first_point == fd->first_point || id->first_point == fd->point_id ) {
  			if ( id->pr_del == 1 || id->pr_del == fd->pr_del ) {
   				trip.trfer_out_point_id = fd->point_id;
          trip.places_in.push_back( *fd );
          pd = fd;
        }
  		}
    }
    else
      if ( fd->point_num > id->point_num && fd->first_point == first_point )
      	if ( id->pr_del == 1 || id->pr_del == fd->pr_del ) {
      		if ( !next_airp ) {
      			next_airp = true;
          }
          trip.places_out.push_back( *fd );
        }
  }
  if ( !trip.places_in.empty() ) { // trip is landing
    trip.airline_in = pd->airline;
    trip.airline_in_fmt = pd->airline_fmt;
    trip.flt_no_in = pd->flt_no;
    trip.suffix_in = pd->suffix;
    trip.suffix_in_fmt = pd->suffix_fmt;
    trip.craft_in = pd->craft;
    trip.craft_in_fmt = pd->craft_fmt;
    trip.bort_in = pd->bort;
    trip.triptype_in = pd->triptype;
    trip.litera_in = pd->litera;
    trip.remark_in = pd->remark;
    trip.pr_del_in = pd->pr_del;
    trip.scd_in = id->scd_in;
    trip.est_in = id->est_in;
    trip.act_in = id->act_in;
    trip.park_in = id->park_in;
    trip.commander_in = pd->commander;
    trip.cockpit_in = pd->cockpit;
    trip.cabin_in = pd->cabin;
  }

  trip.airp = id->airp;
  trip.airp_fmt = id->airp_fmt;
  trip.city = id->city;
  trip.pr_del = id->pr_del;
  trip.part_key = id->part_key;

  if ( !trip.places_out.empty() ) { // trip is takeoffing
    trip.airline_out = id->airline;
    trip.airline_out_fmt = id->airline_fmt;
    trip.flt_no_out = id->flt_no;
    trip.suffix_out = id->suffix;
    trip.suffix_out_fmt = id->suffix_fmt;
    trip.craft_out = id->craft;
    trip.craft_out_fmt = id->craft_fmt;
    trip.bort_out = id->bort;
    trip.scd_out = id->scd_out;
    trip.est_out = id->est_out;
    trip.act_out = id->act_out;
    trip.triptype_out = id->triptype;
    trip.litera_out = id->litera;
    trip.park_out = id->park_out;

    try {
      trip.remark_out = GetRemark( id->remark, id->scd_out, id->est_out, id->region );
    }
    catch( std::exception &E ) {
      ProgError( STDLOG, "SOPP:GetRemark: id->point_id=%d, id->est_out=%f, id->region=%s, what=%s",
                 id->point_id, id->est_out, id->region.c_str(), E.what() );
    }
    catch(...) {
      ProgError( STDLOG, "id->point_id=%d, id->est_out=%f, id->region=%s",
                 id->point_id, id->est_out, id->region.c_str() );
    };

    trip.pr_del_out = id->pr_del;
    trip.pr_reg = id->pr_reg;
    trip.commander_out = id->commander;
    trip.cockpit_out = id->cockpit;
    trip.cabin_out = id->cabin;
  }
  trip.region = id->region;
  trip.delays = id->delays;
  return trip;
}

bool filter_time( TDateTime time, TSOPPTrip &tr, TDateTime first_date, TDateTime next_date, string &errcity )
{
  if ( TReqInfo::Instance()->user.sets.time == ustTimeLocalAirp ) {
    try {
      time = UTCToClient( time, tr.region );
    }
    catch( Exception &e ) {
   	 if ( errcity.empty() )
   	   errcity = tr.city;
       ProgError( STDLOG, "Exception: %s, point_id=%d", e.what(), tr.point_id );
       return false;
    }
  }
 	return ( time >= first_date && time < next_date );
}

bool FilterFlightDate( TSOPPTrip &tr, TDateTime first_date, TDateTime next_date,
                       string &errcity, bool pr_isg )
{
	// 2 ��ਠ�� ࠡ���. 1-�����쭮� �६� ���� ��ॢ����� � UTC, 2 ०�� LocalAll - ⮣��
  if ( first_date > NoExists ) {
    bool canuseTR = false;
    if ( tr.act_in > NoExists )
    	canuseTR = filter_time( tr.act_in, tr, first_date, next_date, errcity );
    else
      if ( tr.est_in > NoExists )
      	canuseTR = filter_time( tr.est_in, tr, first_date, next_date, errcity );
      else
        if ( tr.scd_in > NoExists )
        	canuseTR = filter_time( tr.scd_in, tr, first_date, next_date, errcity );
    if ( !canuseTR )
      if ( tr.act_out > NoExists )
      	canuseTR = filter_time( tr.act_out, tr, first_date, next_date, errcity );
      else
        if ( tr.est_out > NoExists )
        	canuseTR = filter_time( tr.est_out, tr, first_date, next_date, errcity );
        else
          if ( tr.scd_out > NoExists )
          	canuseTR = filter_time( tr.scd_out, tr, first_date, next_date, errcity );
          else canuseTR = pr_isg;
    return canuseTR;
  }
  return true;
}

void read_TripStages( vector<TSoppStage> &stages, TDateTime part_key, int point_id )
{
	TCkinClients ckin_clients;
	stages.clear();
  TQuery StagesQry( &OraSession );
  if ( part_key > NoExists ) {
  	StagesQry.SQLText = arx_stagesSQL;
  	StagesQry.CreateVariable( "part_key", otDate, part_key );
  }
  else { //���� ������ ᯨ᮪ �����⮢
  	TTripStages::ReadCkinClients( point_id, ckin_clients );
    StagesQry.SQLText = stagesSQL;
  }
  StagesQry.CreateVariable( "point_id", otInteger, point_id );
  StagesQry.Execute();
  int col_stage_id = StagesQry.FieldIndex( "stage_id" );
  int col_scd = StagesQry.FieldIndex( "scd" );
  int col_est = StagesQry.FieldIndex( "est" );
  int col_act = StagesQry.FieldIndex( "act" );
  int col_pr_manual = StagesQry.FieldIndex( "pr_manual" );
  int col_pr_auto = StagesQry.FieldIndex( "pr_auto" );
  while ( !StagesQry.Eof ) {
    TSoppStage stage;
    stage.stage_id = StagesQry.FieldAsInteger( col_stage_id );
    if ( StagesQry.FieldIsNULL( col_scd ) )
      stage.scd = NoExists;
    else
      stage.scd = StagesQry.FieldAsDateTime( col_scd );
    if ( StagesQry.FieldIsNULL( col_est ) )
      stage.est = NoExists;
    else
      stage.est = StagesQry.FieldAsDateTime( col_est );
    if ( StagesQry.FieldIsNULL( col_act ) )
      stage.act = NoExists;
    else
      stage.act = StagesQry.FieldAsDateTime( col_act );
    stage.pr_manual = StagesQry.FieldAsInteger( col_pr_manual );
    stage.pr_auto = StagesQry.FieldAsInteger( col_pr_auto );
    stage.pr_permit = !TStagesRules::Instance()->isClientStage( stage.stage_id ) ||
    	                TStagesRules::Instance()->canClientStage( ckin_clients, stage.stage_id );
    stages.push_back( stage );
    StagesQry.Next();
  }
}

void build_TripStages( const vector<TSoppStage> &stages, const string &region, xmlNodePtr tripNode, bool pr_isg )
{
  xmlNodePtr lNode = NULL;
  for ( tstages::const_iterator st=stages.begin(); st!=stages.end(); st++ ) {
  	if ( pr_isg && st->stage_id != sRemovalGangWay ||
         !CompatibleStage(  (TStage)st->stage_id ) )
  		continue;
    if ( !lNode )
      lNode = NewTextChild( tripNode, "stages" );
    xmlNodePtr stageNode = NewTextChild( lNode, "stage" );
    NewTextChild( stageNode, "stage_id", st->stage_id );
    if ( pr_isg ) {
      if ( st->act > NoExists )
        NewTextChild( stageNode, "scd", DateTimeToStr( UTCToClient( st->act, region ), ServerFormatDateTimeAsString ) );
      else
        if ( st->est > NoExists )
          NewTextChild( stageNode, "scd", DateTimeToStr( UTCToClient( st->est, region ), ServerFormatDateTimeAsString ) );
        else
         if ( st->scd > NoExists )
          NewTextChild( stageNode, "scd", DateTimeToStr( UTCToClient( st->scd, region ), ServerFormatDateTimeAsString ) );
      break;
    }
    if ( st->scd > NoExists )
      NewTextChild( stageNode, "scd", DateTimeToStr( UTCToClient( st->scd, region ), ServerFormatDateTimeAsString ) );
    if ( st->est > NoExists )
      NewTextChild( stageNode, "est", DateTimeToStr( UTCToClient( st->est, region ), ServerFormatDateTimeAsString ) );
    if ( st->act > NoExists )
      NewTextChild( stageNode, "act", DateTimeToStr( UTCToClient( st->act, region ), ServerFormatDateTimeAsString ) );
    NewTextChild( stageNode, "pr_auto", (int)st->pr_auto );
    NewTextChild( stageNode, "pr_manual", (int)st->pr_manual );
    if ( !st->pr_permit )
    	NewTextChild( stageNode, "pr_permit", 0 );
  }
}

void getDests( TSOPPTrip &tr, vector<string> &des )
{
	des.clear();
	for (TSOPPDests::iterator i=tr.places_in.begin(); i!=tr.places_in.end(); i++ ) {
		des.push_back( i->airp );
  }
  des.push_back( tr.airp );
	for (TSOPPDests::iterator i=tr.places_out.begin(); i!=tr.places_out.end(); i++ ) {
		des.push_back( i->airp );
  }
}

bool EqualTrips( TSOPPTrip &tr1, TSOPPTrip &tr2 )
{
	if ( tr1.move_id != tr2.move_id || tr1.airp != tr2.airp )
		return false;
	vector<string> des1, des2;
	getDests( tr1, des1 );
	getDests( tr2, des2 );
	if ( des1.size() != des2.size() )
	  return false;
	vector<string>::iterator j=des2.begin();
	for (vector<string>::iterator i=des1.begin(); i!=des1.end(); i++ ) {
		if ( *i != *j )
			return false;
		j++;
	}
	int flt1, flt2;
	if ( tr1.flt_no_out > NoExists )
		flt1 = tr1.flt_no_out;
	else
		flt1 = tr1.flt_no_in;
	if ( tr2.flt_no_out > NoExists )
		flt2 = tr2.flt_no_out;
	else
		flt2 = tr2.flt_no_in;
	return ( flt1 == flt2 );
}

string addCondition( const char *sql, bool pr_arx )
{
	TReqInfo *reqInfo = TReqInfo::Instance();
	bool pr_OR = ( !reqInfo->user.access.airlines.empty() && !reqInfo->user.access.airps.empty() );
  string where_sql, text_sql = sql;
  if ( !reqInfo->user.access.airlines.empty() ) {
   if ( pr_OR )
   	where_sql = " AND ( ";
   else
   	where_sql = " AND ";
   if ( reqInfo->user.access.airlines_permit ) {
   	 if ( pr_arx )
   	 	 where_sql += "arx_points.airline IN ";
   	 else
       where_sql += "points.airline IN ";
     where_sql += GetSQLEnum( reqInfo->user.access.airlines );
   }
   else {
   	 if ( pr_arx )
   	 	 where_sql += "arx_points.airline NOT IN ";
   	 else
       where_sql += "points.airline NOT IN ";
     where_sql += GetSQLEnum( reqInfo->user.access.airlines );
   }
  };
  if ( !reqInfo->user.access.airps.empty() ) {
  	if ( pr_OR )
  		where_sql += " OR ";
  	else
  		where_sql += " AND ";
    if ( reqInfo->user.access.airps_permit ) {
    	if ( pr_arx )
    		where_sql += "arx_points.airp IN ";
    	else
        where_sql += "points.airp IN ";
      where_sql += GetSQLEnum( reqInfo->user.access.airps );
    }
    else {
    	if ( pr_arx )
        where_sql += "arx_points.airp NOT IN ";
    	else
        where_sql += "points.airp NOT IN ";
      where_sql += GetSQLEnum( reqInfo->user.access.airps );
    }
    if ( pr_OR )
    	where_sql += " ) ";
  };
  string::size_type idx;
  while ( (idx = text_sql.find( ":where_sql" )) != string::npos )
  {
    text_sql.erase( idx, strlen( ":where_sql" ) );
    text_sql.insert( idx, where_sql );
  };
  //ProgTrace( TRACE5, "addCondition: SQL=\n%s", text_sql.c_str());
  return text_sql;
}

inline void convertStrToStations( string str_desks, const string &work_mode, tstations &stations )
{
  while ( !str_desks.empty() ) {
    string::size_type idx = str_desks.find( " " );
    TSOPPStation station;
    if ( idx == string::npos ) {
      station.name = str_desks;
      str_desks.clear();
    }
    else {
      //ProgTrace( TRACE5, "names=%s", str_desks.c_str() );
      station.name = str_desks.substr( 0, idx );
      //ProgTrace( TRACE5, "name=%s", station.name.c_str() );
      str_desks.erase( 0, idx + 1 );
      //ProgTrace( TRACE5, "names=%s", str_desks.c_str() );
    }
    station.work_mode = work_mode;
    station.pr_main = false;
    stations.push_back( station );
  }
}

string internal_ReadData( TSOPPTrips &trips, TDateTime first_date, TDateTime next_date,
                          bool arx, TModule module, int point_id = NoExists )
{
	string errcity;
	TReqInfo *reqInfo = TReqInfo::Instance();

  if (reqInfo->user.access.airlines.empty() && reqInfo->user.access.airlines_permit ||
      reqInfo->user.access.airps.empty() && reqInfo->user.access.airps_permit) return errcity;

  TQuery PointsQry( &OraSession );
  TBaseTable &airps = base_tables.get( "airps" );
  TBaseTable &cities = base_tables.get( "cities" );

  if ( arx )
  	if ( module == tISG )
        PointsQry.SQLText = addCondition( arx_points_ISG_SQL, arx ).c_str();
    else
        PointsQry.SQLText = addCondition( arx_points_SOPP_SQL, arx ).c_str();
  else
  	if ( module == tISG )
  	  PointsQry.SQLText = addCondition( points_ISG_SQL, arx ).c_str();
    else
  		if ( point_id == NoExists )
	  		PointsQry.SQLText = addCondition( points_SOPP_SQL, arx ).c_str();
        else {
  			PointsQry.SQLText = points_id_SOPP_SQL;
  			PointsQry.CreateVariable( "point_id", otInteger, point_id );
  	  }

  if ( point_id == NoExists ) {
    if ( first_date != NoExists ) {
      if ( 	TReqInfo::Instance()->user.sets.time == ustTimeLocalAirp ) { // ������� �६��� ���� � first_date, next_date
      	ProgTrace( TRACE5, "ustTimeLocalAirp!!!, first_date=%s, next_date=%s", DateTimeToStr( first_date-1 ).c_str(), DateTimeToStr( next_date+1 ).c_str() );
// ���⠥� ��⪨, �.�. 䨫���� ���� �� UTC, � � ��砥 ०��� �������� �६�� ����� ���� ���室 ��
                          // ��⪨ � ������ ��� ३� ��䨫�����
                          // 20.04.2010 04:00 MEX -> 19.04.2010 22:00 LocalTime
        PointsQry.CreateVariable( "first_date", otDate, first_date-1 ); // UTC +- ��⪨
        PointsQry.CreateVariable( "next_date", otDate, next_date+1 );
      }
      else {
        TDateTime f, l;
        modf( first_date, &f );
        modf( next_date, &l );
        PointsQry.CreateVariable( "first_date", otDate, f );
        PointsQry.CreateVariable( "next_date", otDate, l+1 );
      }
        if ( arx )
          PointsQry.CreateVariable( "arx_trip_date_range", otInteger, ARX_TRIP_DATE_RANGE() );
/*      }*/
    }
    else {
      throw Exception( "internal_ReadData: invalid params" );
    }
  }
  TQuery ClassesQry( &OraSession );
  if ( arx ) {
  	ClassesQry.SQLText = arx_classesSQL;
  	ClassesQry.DeclareVariable( "part_key" ,otDate );
  }
  else
    ClassesQry.SQLText = classesSQL;
  ClassesQry.DeclareVariable( "point_id", otInteger );
  TQuery RegQry( &OraSession );
  if ( arx ) {
  	RegQry.SQLText = arx_regSQL;
  	RegQry.DeclareVariable( "part_key", otDate );
  }
  else
  	RegQry.SQLText = regSQL;
  RegQry.DeclareVariable( "point_id", otInteger );
  TQuery ResaQry( &OraSession );
  if ( !arx ) {
    ResaQry.SQLText = resaSQL;
    ResaQry.DeclareVariable( "point_id", otInteger );
  }
  /*TQuery StationsQry( &OraSession );
  if ( !arx ) {
    StationsQry.SQLText = stationsSQL;
    StationsQry.DeclareVariable( "point_id", otInteger );
  } */
  TQuery Trfer_outQry( &OraSession );
  TQuery Trfer_inQry( &OraSession );
  /*TQuery Trfer_regQry( &OraSession );*/
  if ( !arx ) {
  	Trfer_outQry.SQLText = trfer_out_SQL;
  	Trfer_outQry.DeclareVariable( "point_id", otInteger );
  	Trfer_inQry.SQLText = trfer_in_SQL;
  	Trfer_inQry.DeclareVariable( "point_id", otInteger );
  	/*Trfer_regQry.SQLText = trfer_reg_SQL;
  	Trfer_regQry.DeclareVariable( "point_id", otInteger );*/
  }
  TQuery CRS_DispltoQry( &OraSession );
  TQuery CRS_DisplfromQry( &OraSession );
  if ( !arx ) {
  	CRS_DispltoQry.SQLText = crs_displace_to_SQL;
  	CRS_DispltoQry.DeclareVariable( "point_id_spp", otInteger );
  	CRS_DisplfromQry.SQLText = crs_displace_from_SQL;
  	CRS_DisplfromQry.DeclareVariable( "point_id_spp", otInteger );
  }
  TQuery DelaysQry( &OraSession );
  if ( module == tISG ) {
    if ( arx ) {
      DelaysQry.SQLText = arx_trip_delays_SQL;
      DelaysQry.DeclareVariable( "part_key", otDate );
    }
    else {
      DelaysQry.SQLText = trip_delays_SQL;
    }
    DelaysQry.DeclareVariable( "point_id", otInteger );
  }
  PerfomTest( 666 );

  try {
    PointsQry.Execute();
  }
  catch (EOracleError E) {
    if ( arx && E.Code == 376 )
      throw AstraLocale::UserException("MSG.ONE_OF_DB_FILES_UNAVAILABLE.CALL_ADMIN");
    else
      throw;
  }
  if ( PointsQry.Eof ) {
  	if ( !errcity.empty() )
  	  ProgError(  STDLOG, "Invalid city errcity=%s", errcity.c_str() );
  	return errcity;
  }
  PerfomTest( 667 );
  TSOPPDests dests;
//  TCRS_Displaces crsd;
  double sd;
  modf( NowUTC(), &sd );
  string rgn;

  int move_id = NoExists;
  string ref;
  int col_move_id = PointsQry.FieldIndex( "move_id" );
  int col_ref;
  if ( module == tISG )
  	col_ref = PointsQry.FieldIndex( "ref" );
  int col_point_id = PointsQry.FieldIndex( "point_id" );
  int col_point_num = PointsQry.FieldIndex( "point_num" );
  int col_airp = PointsQry.FieldIndex( "airp" );
  int col_airp_fmt = PointsQry.FieldIndex( "airp_fmt" );
  int col_first_point = PointsQry.FieldIndex( "first_point" );
  int col_airline = PointsQry.FieldIndex( "airline" );
  int col_airline_fmt = PointsQry.FieldIndex( "airline_fmt" );
  int col_flt_no = PointsQry.FieldIndex( "flt_no" );
  int col_suffix = PointsQry.FieldIndex( "suffix" );
  int col_suffix_fmt = PointsQry.FieldIndex( "suffix_fmt" );
  int col_craft = PointsQry.FieldIndex( "craft" );
  int col_craft_fmt = PointsQry.FieldIndex( "craft_fmt" );
  int col_bort = PointsQry.FieldIndex( "bort" );
  int col_commander = PointsQry.GetFieldIndex( "commander" );
  int col_cockpit = PointsQry.GetFieldIndex( "cockpit" );
  int col_cabin = PointsQry.GetFieldIndex( "cabin" );
  int col_scd_in = PointsQry.FieldIndex( "scd_in" );
  int col_est_in = PointsQry.FieldIndex( "est_in" );
  int col_act_in = PointsQry.FieldIndex( "act_in" );
	int col_scd_out = PointsQry.FieldIndex( "scd_out" );
	int col_est_out = PointsQry.FieldIndex( "est_out" );
	int col_act_out = PointsQry.FieldIndex( "act_out" );
	int col_trip_type = PointsQry.FieldIndex( "trip_type" );
	int col_litera = PointsQry.FieldIndex( "litera" );
	int col_park_in = PointsQry.FieldIndex( "park_in" );
	int col_park_out = PointsQry.FieldIndex( "park_out" );
	int col_remark = PointsQry.FieldIndex( "remark" );
	int col_pr_tranzit = PointsQry.FieldIndex( "pr_tranzit" );
	int col_pr_reg = PointsQry.FieldIndex( "pr_reg" );
	int col_pr_del = PointsQry.FieldIndex( "pr_del" );
	int col_tid = PointsQry.FieldIndex( "tid" );
	int col_class = -1;
	int col_cfg = -1;
	int col_part_key = -1;
	if ( arx )
		col_part_key = PointsQry.FieldIndex( "part_key" );
  vector<TSOPPTrip> vtrips;
  while ( !PointsQry.Eof ) {
    if ( move_id != PointsQry.FieldAsInteger( col_move_id ) ) {
      if ( move_id > NoExists && dests.size() > 1 ) {
        //create trips
        string airline;
        vtrips.clear();
        for( TSOPPDests::iterator id=dests.begin(); id!=dests.end(); id++ ) {
        	if ( id != dests.end() - 1 || dests.size() < 2 )
        		airline = id->airline;
        	else {
        		TSOPPDests::iterator f = id;
        		f--;
        		airline = f->airline;
        	}

        	if ( reqInfo->CheckAirline( airline ) &&
        		   reqInfo->CheckAirp( id->airp ) ) {
        		TSOPPTrip ntr = createTrip( move_id, id, dests );
            ntr.ref = ref;
            if ( FilterFlightDate( ntr, first_date, next_date,
            	                     errcity, module == tISG ) ) {
            	vector<TSOPPTrip>::iterator v=vtrips.end();
            	if ( module == tISG && reqInfo->desk.city != "���" ) {
            	  for (v=vtrips.begin(); v!=vtrips.end(); v++) {
              		if ( EqualTrips( ntr, *v ) )
              			break;
              	}
              }
            	if ( v == vtrips.end() ) {
                trips.push_back( ntr );
                vtrips.push_back( ntr );
              }
            }
          }
        }
      }
      move_id = PointsQry.FieldAsInteger( col_move_id );
      if ( module == tISG )
        ref = PointsQry.FieldAsString( col_ref );
      dests.clear();
    }
    TSOPPDest d;
    d.point_id = PointsQry.FieldAsInteger( col_point_id );
    d.point_num = PointsQry.FieldAsInteger( col_point_num );

    d.airp = PointsQry.FieldAsString( col_airp );
    d.airp_fmt = (TElemFmt)PointsQry.FieldAsInteger( col_airp_fmt );
    d.city = ((TAirpsRow&)airps.get_row( "code", d.airp, true )).city;

    if ( PointsQry.FieldIsNULL( col_first_point ) )
      d.first_point = NoExists;
    else
      d.first_point = PointsQry.FieldAsInteger( col_first_point );
    d.airline = PointsQry.FieldAsString( col_airline );
    d.airline_fmt = (TElemFmt)PointsQry.FieldAsInteger( col_airline_fmt );
    if ( PointsQry.FieldIsNULL( col_flt_no ) )
      d.flt_no = NoExists;
    else
      d.flt_no = PointsQry.FieldAsInteger( col_flt_no );
    d.suffix = PointsQry.FieldAsString( col_suffix );
    d.suffix_fmt = (TElemFmt)PointsQry.FieldAsInteger( col_suffix_fmt );
    d.craft = PointsQry.FieldAsString( col_craft );
    d.craft_fmt = (TElemFmt)PointsQry.FieldAsInteger( col_craft_fmt );
    d.bort = PointsQry.FieldAsString( col_bort );
    if ( col_commander >= 0 )
      d.commander = PointsQry.FieldAsString( col_commander );
    if ( col_cockpit >= 0 )
        d.cockpit = PointsQry.FieldAsInteger( col_cockpit );
    if ( col_cabin >= 0 )
        d.cabin = PointsQry.FieldAsInteger( col_cabin );
    if ( PointsQry.FieldIsNULL( col_scd_in ) )
      d.scd_in = NoExists;
    else
      d.scd_in = PointsQry.FieldAsDateTime( col_scd_in );
    if ( PointsQry.FieldIsNULL( col_est_in ) )
      d.est_in = NoExists;
    else
      d.est_in = PointsQry.FieldAsDateTime( col_est_in );
    if ( PointsQry.FieldIsNULL( col_act_in ) )
      d.act_in = NoExists;
    else
      d.act_in = PointsQry.FieldAsDateTime( col_act_in );
    if ( PointsQry.FieldIsNULL( col_scd_out ) )
      d.scd_out = NoExists;
    else
      d.scd_out = PointsQry.FieldAsDateTime( col_scd_out );
    if ( PointsQry.FieldIsNULL( col_est_out ) )
      d.est_out = NoExists;
    else
      d.est_out = PointsQry.FieldAsDateTime( col_est_out );
    if ( PointsQry.FieldIsNULL( col_act_out ) )
      d.act_out = NoExists;
    else
      d.act_out = PointsQry.FieldAsDateTime( col_act_out );
    d.triptype = PointsQry.FieldAsString( col_trip_type );
    d.litera = PointsQry.FieldAsString( col_litera );
    d.park_in = PointsQry.FieldAsString( col_park_in );
    d.park_out = PointsQry.FieldAsString( col_park_out );
    d.remark = PointsQry.FieldAsString( col_remark );
    d.pr_tranzit = PointsQry.FieldAsInteger( col_pr_tranzit );
    d.pr_reg = PointsQry.FieldAsInteger( col_pr_reg );
    d.pr_del = PointsQry.FieldAsInteger( col_pr_del );
    d.tid = PointsQry.FieldAsInteger( col_tid );
    d.region = ((TCitiesRow&)cities.get_row( "code", d.city, true )).region;
    if ( arx )
    	d.part_key = PointsQry.FieldAsDateTime( col_part_key );
    else
    	d.part_key = NoExists;

    if ( module == tISG ) {
   	  DelaysQry.SetVariable( "point_id", d.point_id );
   	  if ( arx )
   	  	DelaysQry.SetVariable( "part_key", d.part_key );
      DelaysQry.Execute();
    	while ( !DelaysQry.Eof ) {
    		TSOPPDelay delay;
    		delay.code = DelaysQry.FieldAsString( "delay_code" );
    		delay.time = DelaysQry.FieldAsDateTime( "time" );
    		d.delays.push_back( delay );
    		DelaysQry.Next();
      }
    }
    dests.push_back( d );
    PointsQry.Next();
  } // end while !PointsQry.Eof
  if ( move_id > NoExists ) {
        //create trips
    string airline;
    vtrips.clear();
    for( TSOPPDests::iterator id=dests.begin(); id!=dests.end(); id++ ) {
      if ( id != dests.end() - 1 )
        airline = id->airline;
      else {
        TSOPPDests::iterator f = id;
        f--;
        airline = f->airline;
      }
      if ( reqInfo->CheckAirline( airline ) &&
        	 reqInfo->CheckAirp( id->airp ) ) {
         TSOPPTrip ntr = createTrip( move_id, id, dests );
         ntr.ref = ref;
         if ( FilterFlightDate( ntr, first_date, next_date,
         	                      errcity, module == tISG ) ) {
          	vector<TSOPPTrip>::iterator v=vtrips.end();
          	if ( module == tISG && reqInfo->desk.city != "���" ) {
           	  for (v=vtrips.begin(); v!=vtrips.end(); v++) {
           	  	if ( EqualTrips( ntr, *v ) )
           	  		break;
             	}
            }
           	if ( v == vtrips.end() ) {
              trips.push_back( ntr );
              vtrips.push_back( ntr );
            }
         }
      }
    }
  }
  // ३�� ᮧ����, ��३��� � ������ ���ଠ樨 �� ३ᠬ
  ////////////////////////// crs_displaces ///////////////////////////////
  PerfomTest( 669 );
  ProgTrace( TRACE5, "trips count %d", trips.size() );

  for ( TSOPPTrips::iterator tr=trips.begin(); tr!=trips.end(); tr++ ) {
    if ( !tr->places_out.empty() ) {
      // ����� ���ଠ樨
      if ( module != tISG ) {
        ClassesQry.SetVariable( "point_id", tr->point_id );
        if ( arx )
        	ClassesQry.SetVariable( "part_key", tr->part_key );
        ClassesQry.Execute();
        while ( !ClassesQry.Eof ) {
        	if ( col_class == -1 ) {
        		col_class = ClassesQry.FieldIndex( "class" );
        		col_cfg = ClassesQry.FieldIndex( "cfg" );
        	}
        	TSoppClass soppclass;
        	soppclass.cl = ClassesQry.FieldAsString( col_class );
        	soppclass.cfg = ClassesQry.FieldAsInteger( col_cfg );
        	tr->classes.push_back( soppclass );
          /*07.07.2010 if ( !tr->classes.empty() && point_id == NoExists )
            tr->classes += " ";
          tr->classes += ClassesQry.FieldAsString( col_class );
          if ( point_id == NoExists )
           tr->classes += string(ClassesQry.FieldAsString( col_cfg ));*/
          ClassesQry.Next();
        }
      } // module != tISG
      if ( module == tSPPCEK )
      	continue;

      if ( module != tISG ) {
        ///////////////////////////// reg /////////////////////////
        RegQry.SetVariable( "point_id", tr->point_id );
        if ( arx )
        	RegQry.SetVariable( "part_key", tr->part_key );
        RegQry.Execute();
        if ( !RegQry.Eof ) {
          tr->reg = RegQry.FieldAsInteger( "reg" );
        }
        ///////////////////////// resa ///////////////////////////
        if ( !arx ) {
//        	ProgTrace( TRACE5, "tr->point_id=%d", tr->point_id );
        	ResaQry.SetVariable( "point_id", tr->point_id );
        	ResaQry.Execute();
          if ( !ResaQry.Eof ) {
            tr->resa = ResaQry.FieldAsInteger( "resa" );
          }
        }
        ////////////////////// trfer  ///////////////////////////////
        if ( !arx ) {
      		Trfer_inQry.SetVariable( "point_id", tr->point_id );
       		Trfer_inQry.Execute();
          if ( !Trfer_inQry.Eof )
          	tr->TrferType.setFlag( trferOut );
          bool trferExists;
          get_TrferExists( tr->point_id, trferExists );
          if ( trferExists ) {
            tr->TrferType.setFlag( trferCkin );
          }
        }
      } // module != tISG
      ////////////////////// stages ///////////////////////////////
      if ( arx )
        read_TripStages( tr->stages, tr->part_key, tr->point_id );
      else
      	read_TripStages( tr->stages, NoExists, tr->point_id );
      ////////////////////////// stations //////////////////////////////
      if ( !arx && module != tISG ) {
        get_DesksGates( tr->point_id, tr->stations );
      }
      //crs_displace2
      if ( !arx ) {
       TDateTime local_time;
       try {
         modf(UTCToLocal( sd, tr->region ),&local_time);
  	     CRS_DispltoQry.SetVariable( "point_id_spp", tr->point_id );
  	     CRS_DispltoQry.Execute();
  	     tr->crs_disp_to = getCrsDisplace( tr->point_id, local_time, true, CRS_DispltoQry );
  	     CRS_DisplfromQry.SetVariable( "point_id_spp", tr->point_id );
  	     CRS_DisplfromQry.Execute();
  	     tr->crs_disp_from = getCrsDisplace( tr->point_id, local_time, false, CRS_DisplfromQry );
       }
       catch( Exception &e ) {
    	  if ( errcity.empty() )
     	    errcity = tr->city;
        ProgError( STDLOG, "Exception: %s, point_id=%d, errcity=%s", e.what(), tr->point_id, errcity.c_str() );
       }
      }
    } // end if (!place_out.empty())
   	if ( !arx && module != tISG && tr->trfer_out_point_id != -1 ) {
   		Trfer_outQry.SetVariable( "point_id", tr->trfer_out_point_id );
   		Trfer_outQry.Execute();
   		if ( !Trfer_outQry.Eof )
   			tr->TrferType.setFlag( trferIn );
   	}
   	if ( !arx && module == tSOPP && tr->pr_reg ) {
   		TripAlarms( tr->point_id, tr->Alarms );
    }
  }
  PerfomTest( 662 );
  return errcity;
}

void buildSOPP( TSOPPTrips &trips, string &errcity, xmlNodePtr dataNode )
{
  xmlNodePtr tripsNode = NULL;
  TDateTime fscd_in, fest_in, fact_in, fscd_out, fest_out, fact_out;
  for ( TSOPPTrips::iterator tr=trips.begin(); tr!=trips.end(); tr++ ) {
    if ( !tripsNode )
      tripsNode = NewTextChild( dataNode, "trips" );
    if ( tr->places_in.empty() && tr->places_out.empty() || tr->region.empty() ) // ⠪�� ३� �� �⮡ࠦ���
      continue;
    try {
      if ( tr->scd_in > NoExists )
    	  fscd_in = UTCToClient( tr->scd_in, tr->region );
      else
    	  fscd_in = NoExists;
      if ( tr->est_in > NoExists )
    	  fest_in = UTCToClient( tr->est_in, tr->region );
      else
    	  fest_in = NoExists;
      if ( tr->act_in > NoExists )
    	  fact_in = UTCToClient( tr->act_in, tr->region );
      else
    	  fact_in = NoExists;
      if ( tr->scd_out > NoExists )
    	  fscd_out = UTCToClient( tr->scd_out, tr->region );
      else
    	  fscd_out = NoExists;
      if ( tr->est_out > NoExists )
    	  fest_out = UTCToClient( tr->est_out, tr->region );
      else
    	  fest_out = NoExists;
      if ( tr->act_out > NoExists )
    	  fact_out = UTCToClient( tr->act_out, tr->region );
      else
    	  fact_out = NoExists;
    }
    catch( Exception &e ) {
     	if ( errcity.empty() )
    	  errcity = tr->city;
      ProgError( STDLOG, "Exception: %s, point_id=%d, errcity=%s", e.what(), tr->point_id, errcity.c_str() );
      continue;
    }
    xmlNodePtr tripNode = NewTextChild( tripsNode, "trip" );
    NewTextChild( tripNode, "move_id", tr->move_id );
    NewTextChild( tripNode, "point_id", tr->point_id );
    if ( tr->part_key > NoExists )
      NewTextChild( tripNode, "part_key", DateTimeToStr( tr->part_key, ServerFormatDateTimeAsString ) );
    if ( !tr->airline_in.empty() )
      NewTextChild( tripNode, "airline_in", ElemIdToElemCtxt( ecDisp, etAirline, tr->airline_in, tr->airline_in_fmt ) );
    if ( tr->flt_no_in > NoExists )
      NewTextChild( tripNode, "flt_no_in", tr->flt_no_in );
    if ( !tr->suffix_in.empty() )
      NewTextChild( tripNode, "suffix_in", ElemIdToElemCtxt( ecDisp, etSuffix, tr->suffix_in, tr->suffix_in_fmt ) );
    if ( tr->craft_in != tr->craft_out && !tr->craft_in.empty() )
      NewTextChild( tripNode, "craft_in", ElemIdToElemCtxt( ecDisp, etCraft, tr->craft_in, tr->craft_in_fmt ) );
    if ( tr->bort_in != tr->bort_out  && !tr->bort_in.empty() )
      NewTextChild( tripNode, "bort_in", tr->bort_in );
    if ( fscd_in > NoExists )
      NewTextChild( tripNode, "scd_in", DateTimeToStr( fscd_in, ServerFormatDateTimeAsString ) );
    if ( fest_in > NoExists )
      NewTextChild( tripNode, "est_in", DateTimeToStr( fest_in, ServerFormatDateTimeAsString ) );
    if ( fact_in > NoExists )
      NewTextChild( tripNode, "act_in", DateTimeToStr( fact_in, ServerFormatDateTimeAsString ) );
    if ( tr->triptype_in != tr->triptype_out && !tr->triptype_in.empty() )
      NewTextChild( tripNode, "triptype_in", ElemIdToCodeNative(etTripType,tr->triptype_in) );
    if ( tr->litera_in != tr->litera_out && !tr->litera_in.empty() )
      NewTextChild( tripNode, "litera_in", ElemIdToCodeNative(etTripLiter,tr->litera_in) );
    if ( !tr->park_in.empty() )
      NewTextChild( tripNode, "park_in", tr->park_in );
    NewTextChild(tripNode, "commander_in", tr->commander_in, "");
    NewTextChild(tripNode, "cockpit_in", tr->cockpit_in, 0);
    NewTextChild(tripNode, "cabin_in", tr->cabin_in, 0);
    if ( tr->remark_in != tr->remark_out && !tr->remark_in.empty() )
      NewTextChild( tripNode, "remark_in", tr->remark_in );
    if ( tr->pr_del_in )
      NewTextChild( tripNode, "pr_del_in", tr->pr_del_in );
    xmlNodePtr lNode = NULL;
    for ( TSOPPDests::iterator sairp=tr->places_in.begin(); sairp!=tr->places_in.end(); sairp++ ) {
      if ( !lNode )
        lNode = NewTextChild( tripNode, "places_in" );
      NewTextChild( lNode, "airp", ElemIdToElemCtxt( ecDisp, etAirp, sairp->airp, sairp->airp_fmt ) );
    }


    NewTextChild( tripNode, "airp", ElemIdToElemCtxt( ecDisp, etAirp, tr->airp, tr->airp_fmt ) );

    if ( !tr->airline_out.empty() )
      NewTextChild( tripNode, "airline_out", ElemIdToElemCtxt( ecDisp, etAirline, tr->airline_out, tr->airline_out_fmt ) );
    if ( tr->flt_no_out > NoExists )
      NewTextChild( tripNode, "flt_no_out", tr->flt_no_out );
    if ( !tr->suffix_out.empty() )
      NewTextChild( tripNode, "suffix_out", ElemIdToElemCtxt( ecDisp, etSuffix, tr->suffix_out, tr->suffix_out_fmt ) );
    if ( !tr->craft_out.empty() )
      NewTextChild( tripNode, "craft_out", ElemIdToElemCtxt( ecDisp, etCraft, tr->craft_out, tr->craft_out_fmt ) );
    if ( !tr->bort_out.empty() )
      NewTextChild( tripNode, "bort_out", tr->bort_out );
    if ( fscd_out > NoExists ) {
//    	ProgTrace( TRACE5, "tr->scd_out=%f, region=%s, point_id=%d",tr->scd_out, tr->region.c_str(), tr->point_id );
      NewTextChild( tripNode, "scd_out", DateTimeToStr( fscd_out, ServerFormatDateTimeAsString ) );
    }
    if ( fest_out > NoExists )
      NewTextChild( tripNode, "est_out", DateTimeToStr( fest_out, ServerFormatDateTimeAsString ) );
    if ( fact_out > NoExists )
      NewTextChild( tripNode, "act_out", DateTimeToStr( fact_out, ServerFormatDateTimeAsString ) );
    if ( !tr->triptype_out.empty() )
      NewTextChild( tripNode, "triptype_out", ElemIdToCodeNative(etTripType,tr->triptype_out) );
    if ( !tr->litera_out.empty() )
      NewTextChild( tripNode, "litera_out", ElemIdToCodeNative(etTripLiter,tr->litera_out) );
    if ( !tr->park_out.empty() )
      NewTextChild( tripNode, "park_out", tr->park_out );
    if ( !tr->remark_out.empty() )
      NewTextChild( tripNode, "remark_out", tr->remark_out );
    if ( tr->pr_del_out )
      NewTextChild( tripNode, "pr_del_out", tr->pr_del_out );
//    if ( tr->pr_reg )
      NewTextChild( tripNode, "pr_reg", tr->pr_reg );
    NewTextChild(tripNode, "commander_out", tr->commander_out, "");
    NewTextChild(tripNode, "cockpit_out", tr->cockpit_out, 0);
    NewTextChild(tripNode, "cabin_out", tr->cabin_out, 0);
   	int trfertype = 0x000;
   	if ( tr->TrferType.isFlag( trferIn ) )
   		trfertype += 0x00F;
   	if ( tr->TrferType.isFlag( trferOut ) )
   		trfertype += 0x0F0;
   	if ( tr->TrferType.isFlag( trferCkin ) )
   		trfertype += 0xF00;
   	if ( trfertype )
    	NewTextChild( tripNode, "trfertype", trfertype );
    if ( tr->TrferType.isFlag( trferCkin ) )
    	NewTextChild( tripNode, "trfer_to", "->" );
    if ( tr->TrferType.isFlag( trferOut ) || tr->TrferType.isFlag( trferIn ) )
    	NewTextChild( tripNode, "trfer_from", "->" );
    lNode = NULL;
    for ( TSOPPDests::iterator sairp=tr->places_out.begin(); sairp!=tr->places_out.end(); sairp++ ) {
      if ( !lNode )
        lNode = NewTextChild( tripNode, "places_out" );
      NewTextChild( lNode, "airp", ElemIdToElemCtxt( ecDisp, etAirp, sairp->airp, sairp->airp_fmt ) );
    }
    if ( !tr->classes.empty() ) {
    	lNode = NewTextChild( tripNode, "classes" );
    	string str;
    	for ( vector<TSoppClass>::iterator icl=tr->classes.begin(); icl!=tr->classes.end(); icl++ ) {
    		if ( TReqInfo::Instance()->desk.compatible( LATIN_VERSION ) )
    		  SetProp( NewTextChild( lNode, "class", ElemIdToCodeNative( etClass, icl->cl ) ), "cfg", icl->cfg );
    		else {
          if ( !str.empty() )
            str += " ";
    		  str += ElemIdToCodeNative( etClass, icl->cl ) + IntToString( icl->cfg );
        }
    	}
    	if ( !TReqInfo::Instance()->desk.compatible( LATIN_VERSION ) )
        NodeSetContent( lNode, str );
    }
    if ( tr->reg )
      NewTextChild( tripNode, "reg", tr->reg );
    if ( tr->resa )
      NewTextChild( tripNode, "resa", tr->resa );
    if ( !tr->crs_disp_from.empty() )
      NewTextChild( tripNode, "crs_disp_from", tr->crs_disp_from );
    if ( !tr->crs_disp_to.empty() )
      NewTextChild( tripNode, "crs_disp_to", tr->crs_disp_to );
    try {
      build_TripStages( tr->stages, tr->region, tripNode, false );
    }
    catch( Exception &e ) {
    	if ( errcity.empty() )
    		errcity = tr->city;
      ProgError( STDLOG, "Exception: %s, point_id=%d", e.what(), tr->point_id );
    }
    lNode = NULL;
    for ( tstations::iterator st=tr->stations.begin(); st!=tr->stations.end(); st++ ) {
      if ( !lNode )
        lNode = NewTextChild( tripNode, "stations" );
      xmlNodePtr stationNode = NewTextChild( lNode, "station" );
      NewTextChild( stationNode, "name", st->name );
      NewTextChild( stationNode, "work_mode", st->work_mode );
      if ( st->pr_main )
      	NewTextChild( stationNode, "pr_main" );
    }
    xmlNodePtr alarmsNode = NULL;
    for ( int ialarm=0; ialarm<atLength; ialarm++ ) {
    	TTripAlarmsType alarm = (TTripAlarmsType)ialarm;
      if ( tr->Alarms.isFlag( alarm ) ) {
      	if ( !alarmsNode )
      		alarmsNode = NewTextChild( tripNode, "alarms" );
      	xmlNodePtr an;
      	switch( alarm ) {
      		case atWaitlist:
      			an = NewTextChild( alarmsNode, "alarm", "Waitlist" );
      			SetProp( an, "text", TripAlarmString( alarm ) );
      			break;
      		case atOverload:
      			an = NewTextChild( alarmsNode, "alarm", "Overload" );
      			SetProp( an, "text", TripAlarmString( alarm ) );
      			break;
      	  case atBrd:
      			an = NewTextChild( alarmsNode, "alarm", "Brd" );
      			SetProp( an, "text", TripAlarmString( alarm ) );
      			break;
      	  case atSalon:
      			an = NewTextChild( alarmsNode, "alarm", "Salon" );
      			SetProp( an, "text", TripAlarmString( alarm ) );
      			break;
      	  case atSeance:
      			an = NewTextChild( alarmsNode, "alarm", "Seance" );
      			SetProp( an, "text", TripAlarmString( alarm ) );
      			break;
      	  case atDiffComps:
      			an = NewTextChild( alarmsNode, "alarm", "DiffComps" );
      			SetProp( an, "text", TripAlarmString( alarm ) );
      			break;
      	  case atTlgOut:
      			an = NewTextChild( alarmsNode, "alarm", "TlgOut" );
      			SetProp( an, "text", TripAlarmString( alarm ) );
      			break;
      	  case atSpecService:
      			an = NewTextChild( alarmsNode, "alarm", "SpecService" );
      			SetProp( an, "text", TripAlarmString( alarm ) );
      			break;
      		default:;
      	}
      }
    }
  } // end for trip
}

void buildISG( TSOPPTrips &trips, string &errcity, xmlNodePtr dataNode )
{
	ProgTrace( TRACE5, "buildISG" );
  xmlNodePtr tripsNode = NULL;
  xmlNodePtr dnode;
  TDateTime fscd_in, fest_in, fact_in, fscd_out, fest_out, fact_out;
  string ecity;
  for ( TSOPPTrips::iterator tr=trips.begin(); tr!=trips.end(); tr++ ) {
    if ( !tripsNode )
      tripsNode = NewTextChild( dataNode, "trips" );
    if ( tr->places_in.empty() && tr->places_out.empty() || tr->region.empty() ) // ⠪�� ३� �� �⮡ࠦ���
      continue;
    try {
    	ecity = tr->city;
      if ( tr->scd_in > NoExists )
    	  fscd_in = UTCToClient( tr->scd_in, tr->region );
      else
    	  fscd_in = NoExists;
      if ( tr->est_in > NoExists )
    	  fest_in = UTCToClient( tr->est_in, tr->region );
      else
    	  fest_in = NoExists;
      if ( tr->act_in > NoExists )
    	  fact_in = UTCToClient( tr->act_in, tr->region );
      else
    	  fact_in = NoExists;
      if ( tr->scd_out > NoExists )
    	  fscd_out = UTCToClient( tr->scd_out, tr->region );
      else
    	  fscd_out = NoExists;
      if ( tr->est_out > NoExists )
    	  fest_out = UTCToClient( tr->est_out, tr->region );
      else
    	  fest_out = NoExists;
      if ( tr->act_out > NoExists )
    	  fact_out = UTCToClient( tr->act_out, tr->region );
      else
    	  fact_out = NoExists;
      for ( TSOPPDests::iterator sairp=tr->places_in.begin(); sairp!=tr->places_in.end(); sairp++ ) {
      	ecity = sairp->city;
        if ( sairp->scd_in > NoExists )
        	sairp->scd_in = UTCToClient( sairp->scd_in, sairp->region );
        if ( sairp->est_in > NoExists )
        	sairp->est_in = UTCToClient( sairp->est_in, sairp->region );
        if ( sairp->act_in > NoExists )
        	sairp->act_in = UTCToClient( sairp->act_in, sairp->region );
        if ( sairp->scd_out > NoExists )
        	sairp->scd_out = UTCToClient( sairp->scd_out, sairp->region );
        if ( sairp->est_out > NoExists )
        	sairp->est_out = UTCToClient( sairp->est_out, sairp->region );
        if ( sairp->act_out > NoExists )
        	sairp->act_out = UTCToClient( sairp->act_out, sairp->region );
    	  for ( vector<TSOPPDelay>::iterator delay=sairp->delays.begin(); delay!=sairp->delays.end(); delay++ ) {
  		    delay->time = UTCToClient( delay->time, sairp->region );
        }
      }
  	  for ( vector<TSOPPDelay>::iterator delay=tr->delays.begin(); delay!=tr->delays.end(); delay++ ) {
  		  delay->time = UTCToClient( delay->time, tr->region );
      }
      for ( TSOPPDests::iterator sairp=tr->places_out.begin(); sairp!=tr->places_out.end(); sairp++ ) {
      	ecity = sairp->city;
        if ( sairp->scd_in > NoExists )
        	sairp->scd_in = UTCToClient( sairp->scd_in, sairp->region );
        if ( sairp->est_in > NoExists )
        	sairp->est_in = UTCToClient( sairp->est_in, sairp->region );
        if ( sairp->act_in > NoExists )
        	sairp->act_in = UTCToClient( sairp->act_in, sairp->region );
        if ( sairp->scd_out > NoExists )
        	sairp->scd_out = UTCToClient( sairp->scd_out, sairp->region );
        if ( sairp->est_out > NoExists )
        	sairp->est_out = UTCToClient( sairp->est_out, sairp->region );
        if ( sairp->act_out > NoExists )
        	sairp->act_out = UTCToClient( sairp->act_out, sairp->region );
  	    for ( vector<TSOPPDelay>::iterator delay=sairp->delays.begin(); delay!=sairp->delays.end(); delay++ ) {
  		    delay->time = UTCToClient( delay->time, sairp->region );
        }
      }
    }
    catch( Exception &e ) {
    	if ( errcity.empty() )
     	  errcity = ecity;
      ProgError( STDLOG, "Exception: %s, point_id=%d, errcity=%s", e.what(), tr->point_id, errcity.c_str() );
      continue;
    }
    xmlNodePtr tripNode = NewTextChild( tripsNode, "trip" );
    NewTextChild( tripNode, "move_id", tr->move_id );
    NewTextChild( tripNode, "point_id", tr->point_id );
    if ( tr->part_key > NoExists )
      NewTextChild( tripNode, "part_key", DateTimeToStr( tr->part_key, ServerFormatDateTimeAsString ) );
    if ( !tr->airline_in.empty() )
      NewTextChild( tripNode, "airline_in", ElemIdToElemCtxt( ecDisp, etAirline, tr->airline_in, tr->airline_in_fmt ) );
    if ( tr->flt_no_in > NoExists )
      NewTextChild( tripNode, "flt_no_in", tr->flt_no_in );
    if ( !tr->suffix_in.empty() )
      NewTextChild( tripNode, "suffix_in", ElemIdToElemCtxt( ecDisp, etSuffix, tr->suffix_in, tr->suffix_in_fmt ) );
    if ( tr->craft_in != tr->craft_out && !tr->craft_in.empty() )
      NewTextChild( tripNode, "craft_in", ElemIdToElemCtxt( ecDisp, etCraft, tr->craft_in, tr->craft_in_fmt ) );
    if ( tr->bort_in != tr->bort_out  && !tr->bort_in.empty() )
      NewTextChild( tripNode, "bort_in", tr->bort_in );
    if ( fscd_in > NoExists )
      NewTextChild( tripNode, "scd_in", DateTimeToStr( fscd_in, ServerFormatDateTimeAsString ) );
    if ( fest_in > NoExists )
      NewTextChild( tripNode, "est_in", DateTimeToStr( fest_in, ServerFormatDateTimeAsString ) );
    if ( fact_in > NoExists )
      NewTextChild( tripNode, "act_in", DateTimeToStr( fact_in, ServerFormatDateTimeAsString ) );
    if ( tr->triptype_in != tr->triptype_out && !tr->triptype_in.empty() )
      NewTextChild( tripNode, "triptype_in", ElemIdToCodeNative(etTripType,tr->triptype_in) );
    if ( tr->litera_in != tr->litera_out && !tr->litera_in.empty() )
      NewTextChild( tripNode, "litera_in", ElemIdToCodeNative(etTripLiter,tr->litera_in) );
    if ( !tr->park_in.empty() )
      NewTextChild( tripNode, "park_in", tr->park_in );
    if ( tr->remark_in != tr->remark_out && !tr->remark_in.empty() )
      NewTextChild( tripNode, "remark_in", tr->remark_in );
    NewTextChild( tripNode, "commander_in", tr->commander_in, "");
    NewTextChild( tripNode, "cockpit_in", tr->cockpit_in, 0);
    NewTextChild( tripNode, "cabin_in", tr->cabin_in, 0);
    xmlNodePtr lNode = NULL;
    for ( TSOPPDests::iterator sairp=tr->places_in.begin(); sairp!=tr->places_in.end(); sairp++ ) {
      if ( !lNode )
        lNode = NewTextChild( tripNode, "places_in" );
      xmlNodePtr destNode = NewTextChild( lNode, "dest" );
      NewTextChild( destNode, "point_id", sairp->point_id );
      NewTextChild( destNode, "commander", sairp->commander, "" );
      NewTextChild( destNode, "cockpit", sairp->cockpit, 0 );
      NewTextChild( destNode, "cabin", sairp->cabin, 0 );
      NewTextChild( destNode, "airp", ElemIdToElemCtxt( ecDisp, etAirp, sairp->airp, sairp->airp_fmt ) );
      if ( sairp->pr_del )
      	NewTextChild( destNode, "pr_del", sairp->pr_del );
      if ( sairp->scd_in > NoExists )
        NewTextChild( destNode, "scd_in", DateTimeToStr( sairp->scd_in, ServerFormatDateTimeAsString ) );
      if ( sairp->est_in > NoExists )
        NewTextChild( destNode, "est_in", DateTimeToStr( sairp->est_in, ServerFormatDateTimeAsString ) );
      if ( sairp->act_in > NoExists )
        NewTextChild( destNode, "act_in", DateTimeToStr( sairp->act_in, ServerFormatDateTimeAsString ) );
      if ( sairp->scd_out > NoExists )
        NewTextChild( destNode, "scd_out", DateTimeToStr( sairp->scd_out, ServerFormatDateTimeAsString ) );
      if ( sairp->est_out > NoExists )
        NewTextChild( destNode, "est_out", DateTimeToStr( sairp->est_out, ServerFormatDateTimeAsString ) );
      if ( sairp->act_out > NoExists )
        NewTextChild( destNode, "act_out", DateTimeToStr( sairp->act_out, ServerFormatDateTimeAsString ) );
    	dnode = NULL;
    	for ( vector<TSOPPDelay>::iterator delay=sairp->delays.begin(); delay!=sairp->delays.end(); delay++ ) {
  	  	if ( !dnode )
  		  	dnode = NewTextChild( destNode, "delays" );
  		  xmlNodePtr fnode = NewTextChild( dnode, "delay" );
  		  NewTextChild( fnode, "delay_code", ElemIdToCodeNative(etDelayType,delay->code) );
  		  NewTextChild( fnode, "time", DateTimeToStr( delay->time, ServerFormatDateTimeAsString ) );
      }
    }

    NewTextChild( tripNode, "airp", ElemIdToElemCtxt( ecDisp, etAirp, tr->airp, tr->airp_fmt ) );
    NewTextChild( tripNode, "pr_del", tr->pr_del );
   	dnode = NULL;
   	for ( vector<TSOPPDelay>::iterator delay=tr->delays.begin(); delay!=tr->delays.end(); delay++ ) {
   		//ProgTrace( TRACE5, "point_id=%d, delay->code=%s", tr->point_id, delay->code.c_str() );
 	  	if ( !dnode )
 		  	dnode = NewTextChild( tripNode, "delays" );
 		  xmlNodePtr fnode = NewTextChild( dnode, "delay" );
 		  NewTextChild( fnode, "delay_code", ElemIdToCodeNative(etDelayType,delay->code) );
 		  NewTextChild( fnode, "time", DateTimeToStr( delay->time, ServerFormatDateTimeAsString ) );
    }

    if ( !tr->ref.empty() )
    	NewTextChild( tripNode, "ref", tr->ref );
    if ( !tr->airline_out.empty() )
      NewTextChild( tripNode, "airline_out", ElemIdToElemCtxt( ecDisp, etAirline, tr->airline_out, tr->airline_out_fmt ) );
    if ( tr->flt_no_out > NoExists )
      NewTextChild( tripNode, "flt_no_out", tr->flt_no_out );
    if ( !tr->suffix_out.empty() )
      NewTextChild( tripNode, "suffix_out", ElemIdToElemCtxt( ecDisp, etSuffix, tr->suffix_out, tr->suffix_out_fmt ) );
    if ( !tr->craft_out.empty() )
      NewTextChild( tripNode, "craft_out", ElemIdToElemCtxt( ecDisp, etCraft, tr->craft_out, tr->craft_out_fmt ) );
    if ( !tr->bort_out.empty() )
      NewTextChild( tripNode, "bort_out", tr->bort_out );
    if ( fscd_out > NoExists )
      NewTextChild( tripNode, "scd_out", DateTimeToStr( fscd_out, ServerFormatDateTimeAsString ) );
    if ( fest_out > NoExists )
      NewTextChild( tripNode, "est_out", DateTimeToStr( fest_out, ServerFormatDateTimeAsString ) );
    if ( fact_out > NoExists )
      NewTextChild( tripNode, "act_out", DateTimeToStr( fact_out, ServerFormatDateTimeAsString ) );
    if ( !tr->triptype_out.empty() )
      NewTextChild( tripNode, "triptype_out", ElemIdToCodeNative(etTripType,tr->triptype_out) );
    if ( !tr->litera_out.empty() )
      NewTextChild( tripNode, "litera_out", ElemIdToCodeNative(etTripLiter,tr->litera_out) );
    if ( !tr->park_out.empty() )
      NewTextChild( tripNode, "park_out", tr->park_out );
    if ( !tr->remark_out.empty() )
      NewTextChild( tripNode, "remark_out", tr->remark_out );
/*    if ( tr->pr_del_out )
      NewTextChild( tripNode, "pr_del_out", tr->pr_del_out );*/
    NewTextChild( tripNode, "pr_reg", tr->pr_reg );
    NewTextChild( tripNode, "commander_out", tr->commander_out, "");
    NewTextChild( tripNode, "cockpit_out", tr->cockpit_out, 0);
    NewTextChild( tripNode, "cabin_out", tr->cabin_out, 0);
   	int trfertype = 0x000;
   	if ( tr->TrferType.isFlag( trferIn ) )
   		trfertype += 0x00F;
   	if ( tr->TrferType.isFlag( trferOut ) )
   		trfertype += 0x0F0;
   	if ( tr->TrferType.isFlag( trferCkin ) )
   		trfertype += 0xF00;
   	if ( trfertype )
    	NewTextChild( tripNode, "trfertype", trfertype );
    if ( tr->TrferType.isFlag( trferCkin ) )
    	NewTextChild( tripNode, "trfer_to", "->" );
    if ( tr->TrferType.isFlag( trferOut ) || tr->TrferType.isFlag( trferIn ) )
    	NewTextChild( tripNode, "trfer_from", "->" );
    lNode = NULL;
    for ( TSOPPDests::iterator sairp=tr->places_out.begin(); sairp!=tr->places_out.end(); sairp++ ) {
      if ( !lNode )
        lNode = NewTextChild( tripNode, "places_out" );
      xmlNodePtr destNode = NewTextChild( lNode, "dest" );
      NewTextChild( destNode, "point_id", sairp->point_id );
      NewTextChild( destNode, "commander", sairp->commander, "" );
      NewTextChild( destNode, "cockpit", sairp->cockpit, 0 );
      NewTextChild( destNode, "cabin", sairp->cabin, 0 );
      NewTextChild( destNode, "airp", ElemIdToElemCtxt( ecDisp, etAirp, sairp->airp, sairp->airp_fmt ) );
      if ( sairp->pr_del )
      	NewTextChild( destNode, "pr_del", sairp->pr_del );
      if ( sairp->scd_in > NoExists )
        NewTextChild( destNode, "scd_in", DateTimeToStr( sairp->scd_in, ServerFormatDateTimeAsString ) );
      if ( sairp->est_in > NoExists )
        NewTextChild( destNode, "est_in", DateTimeToStr( sairp->est_in, ServerFormatDateTimeAsString ) );
      if ( sairp->act_in > NoExists )
        NewTextChild( destNode, "act_in", DateTimeToStr( sairp->act_in, ServerFormatDateTimeAsString ) );
      if ( sairp->scd_out > NoExists )
        NewTextChild( destNode, "scd_out", DateTimeToStr( sairp->scd_out, ServerFormatDateTimeAsString ) );
      if ( sairp->est_out > NoExists )
        NewTextChild( destNode, "est_out", DateTimeToStr( sairp->est_out, ServerFormatDateTimeAsString ) );
      if ( sairp->act_out > NoExists )
        NewTextChild( destNode, "act_out", DateTimeToStr( sairp->act_out, ServerFormatDateTimeAsString ) );
      if ( sairp->pr_del )
      	NewTextChild( destNode, "pr_del", sairp->pr_del );
    	dnode = NULL;
    	for ( vector<TSOPPDelay>::iterator delay=sairp->delays.begin(); delay!=sairp->delays.end(); delay++ ) {
  	  	if ( !dnode )
  		  	dnode = NewTextChild( destNode, "delays" );
  		  xmlNodePtr fnode = NewTextChild( dnode, "delay" );
  		  NewTextChild( fnode, "delay_code", ElemIdToCodeNative(etDelayType,delay->code) );
  		  NewTextChild( fnode, "time", DateTimeToStr( delay->time, ServerFormatDateTimeAsString ) );
      }
    }

    if ( !tr->crs_disp_from.empty() )
      NewTextChild( tripNode, "crs_disp_from", tr->crs_disp_from );
    if ( !tr->crs_disp_to.empty() )
      NewTextChild( tripNode, "crs_disp_to", tr->crs_disp_to );
    try {
      build_TripStages( tr->stages, tr->region, tripNode, true );
    }
    catch( Exception &e ) {
    	if ( errcity.empty() )
    		errcity = tr->city;
      ProgError( STDLOG, "Exception: %s, point_id=%d", e.what(), tr->point_id );
    }
  } // end for trip
}

TDateTime Approached_ClientUTC( TDateTime f, string tz_region, bool pr_max )
{
	TDateTime d1, d2, d;
	try {
	  d = ClientToUTC( f, TReqInfo::Instance()->desk.tz_region );
	  return d;
	}
  catch( boost::local_time::ambiguous_result ) {
  	tst();
  	d1 = ClientToUTC( f, TReqInfo::Instance()->desk.tz_region, 0 );
  	d2 = ClientToUTC( f, TReqInfo::Instance()->desk.tz_region, 1 );
  }
  catch( boost::local_time::time_label_invalid ) {
  	tst();
  	d1 = ClientToUTC( f-1, TReqInfo::Instance()->desk.tz_region )+1;
  	d2 = ClientToUTC( f+1, TReqInfo::Instance()->desk.tz_region )-1;
  }
	if ( pr_max )
	  return max(d1,d2);
	else
	  return min(d1,d2);
}


void SoppInterface::ReadTrips(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
//  createCentringFile( 13672, "ASTRA", "DMDTST" );
  ProgTrace( TRACE5, "ReadTrips" );
  xmlNodePtr dataNode = NewTextChild( resNode, "data" );
  TModule module;
  if ( GetNode( "disp_isg", reqNode ) )
  	module = tISG;
  else
  	module = tSOPP;
  if ( GetNode( "CorrectStages", reqNode ) ) {
    TStagesRules::Instance()->Build( NewTextChild( dataNode, "CorrectStages" ) );
  }
  TDateTime vdate, first_date, next_date;
  xmlNodePtr dNode = GetNode( "flight_date", reqNode );
  bool arx = false;
  if ( !dNode ) {
  	dNode = GetNode( "arx_date", reqNode );
  	arx = dNode;
  }
  if ( dNode ) {
  	double f;
  	vdate = NodeAsDateTime( dNode );
  	modf( (double)vdate, &f );
  	if ( TReqInfo::Instance()->user.sets.time == ustTimeLocalAirp ) {
  		first_date = f;
  		next_date = f + 1;
    }
    else { // ࠡ�⠥� � UTC �६����� ���� ���� �ନ����
    	first_date = Approached_ClientUTC( f, TReqInfo::Instance()->desk.tz_region, false );
    	next_date = Approached_ClientUTC( f+1, TReqInfo::Instance()->desk.tz_region, true );
    }

    ProgTrace( TRACE5, "ustTimeLocalAirp=%d, first_date=%s, next_date=%s",
               TReqInfo::Instance()->user.sets.time == ustTimeLocalAirp,
               	DateTimeToStr( first_date ).c_str(), DateTimeToStr( next_date ).c_str() );

    if ( arx )
    	NewTextChild( dataNode, "arx_date", DateTimeToStr( vdate, ServerFormatDateTimeAsString ) );
    else
  	  NewTextChild( dataNode, "flight_date", DateTimeToStr( vdate, ServerFormatDateTimeAsString ) );
  }
  else {
    first_date = NoExists;
    next_date = NoExists;
  }
  TSOPPTrips trips;
  string errcity = internal_ReadData( trips, first_date, next_date, arx, module );
  if ( module == tISG )
  	buildISG( trips, errcity, dataNode );
  else
    buildSOPP( trips, errcity, dataNode );
  if ( !errcity.empty() )
    AstraLocale::showErrorMessage( "MSG.CITY.REGION_NOT_DEFINED.NOT_ALL_FLIGHTS_ARE_SHOWN",
    	                             LParams() << LParam("city", ElemIdToCodeNative(etCity,errcity)));
}

namespace sopp
{
class TTransferPaxItem
{
  public:
    string surname, name;
    int seats;
    int bag_amount, bag_weight, rk_weight;
    vector<TBagTagNumber> tags;
    TTransferPaxItem()
    {
      seats=NoExists;
      bag_amount=NoExists;
      bag_weight=NoExists;
      rk_weight=NoExists;
    };
};

class TTransferGrpItem
{
  public:
    int grp_id;
    int inbound_point_dep; //⮫쪮 ��� GetInboundTCkin
    string inbound_trip;   //⮫쪮 ��� GetInboundTCkin
    string airline_view;
    int flt_no;
    string suffix_view;
    TDateTime scd_local;
    string airp_dep_view, airp_arv_view, subcl_view;
    string tlg_airp_view;
    int subcl_priority;
    int seats;
    int bag_amount, bag_weight, rk_weight;
    string weight_unit;
    vector<TTransferPaxItem> pax;
    vector<TBagTagNumber> tags;
    
    bool operator < (const TTransferGrpItem &grp) const
    {
      if (scd_local!=grp.scd_local)
        return scd_local<grp.scd_local;
      if (airline_view!=grp.airline_view)
        return airline_view<grp.airline_view;
      if (flt_no!=grp.flt_no)
        return flt_no<grp.flt_no;
      if (suffix_view!=grp.suffix_view)
        return suffix_view<grp.suffix_view;
      if (airp_dep_view!=grp.airp_dep_view)
        return airp_dep_view<grp.airp_dep_view;
      if (inbound_point_dep!=grp.inbound_point_dep)
        return inbound_point_dep<grp.inbound_point_dep;
      if (airp_arv_view!=grp.airp_arv_view)
        return airp_arv_view<grp.airp_arv_view;
      if (subcl_priority!=grp.subcl_priority)
        return subcl_priority<grp.subcl_priority;
      if (subcl_view!=grp.subcl_view)
        return subcl_view<grp.subcl_view;
      return grp_id<grp.grp_id;
    };
    
};

};

void SoppInterface::GetTransfer(bool pr_inbound_tckin,
                                bool pr_out,
                                bool pr_tlg,
                                bool pr_bag,
                                int point_id,
                                xmlNodePtr resNode)
{
  TReqInfo *reqInfo = TReqInfo::Instance();

  TQuery Qry(&OraSession);
  TQuery PointsQry(&OraSession);
  TQuery PaxQry(&OraSession);
  TQuery RemQry(&OraSession);
  TQuery TagQry(&OraSession);
  
  PointsQry.Clear();
  PointsQry.SQLText =
    "SELECT point_id,airp,airline,flt_no,suffix,craft,bort,scd_out, "
    "       NVL(act_out,NVL(est_out,scd_out)) AS real_out "
    "FROM points "
    "WHERE point_id=:point_id AND pr_del>=0";
  PointsQry.DeclareVariable( "point_id", otInteger );

  if (!pr_out)
  {
    //point_id ᮤ�ন� �㭪� �ਫ�� � ��� �㦥� �।��騩 �㭪� �뫥�
    TTripRouteItem priorAirp;
    TTripRoute().GetPriorAirp(NoExists,point_id,trtNotCancelled,priorAirp);
    if (priorAirp.point_id==NoExists) throw AstraLocale::UserException("MSG.FLIGHT.NOT_FOUND");
    point_id=priorAirp.point_id;
  };
  PointsQry.SetVariable( "point_id", point_id );
  PointsQry.Execute();
  if (PointsQry.Eof) throw AstraLocale::UserException("MSG.FLIGHT.NOT_FOUND");
  TTripInfo info(PointsQry);

  Qry.Clear();
  if (pr_tlg)
  {
    if (pr_out)
      Qry.SQLText=
        "SELECT tlg_trips.point_id,tlg_trips.airline,tlg_trips.flt_no,tlg_trips.suffix, "
        "       tlg_trips.scd,tlg_trips.airp_dep AS airp,tlg_trips.airp_dep,tlg_trips.airp_arv, "
        "       tlg_transfer.trfer_id,tlg_transfer.subcl_in AS subcl, "
        "       trfer_grp.grp_id,seats,bag_amount,bag_weight,rk_weight,weight_unit "
        "FROM trfer_grp,tlgs_in,tlg_trips,tlg_transfer,tlg_binding "
        "WHERE tlg_transfer.tlg_id=tlgs_in.id AND tlgs_in.num=1 AND "
        "      tlgs_in.type=:tlg_type AND "
        "      tlg_transfer.point_id_out=tlg_binding.point_id_tlg AND "
        "      tlg_binding.point_id_spp=:point_id AND "
        "      tlg_trips.point_id=tlg_transfer.point_id_in AND "
        "      tlg_transfer.trfer_id=trfer_grp.trfer_id ";
    else
      Qry.SQLText=
        "SELECT tlg_trips.point_id,tlg_trips.airline,tlg_trips.flt_no,tlg_trips.suffix, "
        "       tlg_trips.scd,tlg_trips.airp_arv AS airp,tlg_trips.airp_dep,tlg_trips.airp_arv, "
        "       tlg_transfer.trfer_id,tlg_transfer.subcl_out AS subcl, "
        "       trfer_grp.grp_id,seats,bag_amount,bag_weight,rk_weight,weight_unit "
        "FROM trfer_grp,tlgs_in,tlg_trips,tlg_transfer,tlg_binding "
        "WHERE tlg_transfer.tlg_id=tlgs_in.id AND tlgs_in.num=1 AND "
        "      tlgs_in.type=:tlg_type AND "
        "      tlg_transfer.point_id_in=tlg_binding.point_id_tlg AND "
        "      tlg_binding.point_id_spp=:point_id AND "
        "      tlg_trips.point_id=tlg_transfer.point_id_out AND "
        "      tlg_transfer.trfer_id=trfer_grp.trfer_id ";
    Qry.CreateVariable("point_id",otInteger,point_id);
    if (pr_bag)
      Qry.CreateVariable("tlg_type",otString,"BTM");
    else
      Qry.CreateVariable("tlg_type",otString,"PTM");

    PaxQry.SQLText="SELECT surname,name FROM trfer_pax WHERE grp_id=:grp_id ORDER BY surname,name";
    PaxQry.DeclareVariable("grp_id",otInteger);

    TagQry.SQLText=
      "SELECT no FROM trfer_tags WHERE grp_id=:grp_id";
    TagQry.DeclareVariable("grp_id",otInteger);
  }
  else
  {
    if (pr_inbound_tckin)
    {
      Qry.SQLText=
        "SELECT tckin_pax_grp.tckin_id,tckin_pax_grp.seg_no, "
        "       pax_grp.grp_id,pax_grp.airp_arv,pax_grp.class AS subcl "
        "FROM pax_grp,tckin_pax_grp "
        "WHERE pax_grp.grp_id=tckin_pax_grp.grp_id AND "
        "      pax_grp.point_dep=:point_id AND bag_refuse=0 AND pax_grp.status<>'T' ";
    }
    else
    {
      Qry.SQLText=
        "SELECT trfer_trips.airline,trfer_trips.flt_no,trfer_trips.suffix,trfer_trips.scd, "
        "       trfer_trips.airp_dep,transfer.airp_arv, "
        "       pax_grp.grp_id,pax_grp.class AS subcl "
        "FROM pax_grp,transfer,trfer_trips "
        "WHERE pax_grp.grp_id=transfer.grp_id AND "
        "      transfer.point_id_trfer=trfer_trips.point_id AND "
        "      transfer.transfer_num=1 AND "
        "      pax_grp.point_dep=:point_id AND bag_refuse=0 AND pax_grp.status<>'T' ";
    };
    Qry.CreateVariable("point_id",otInteger,point_id);
    
    PaxQry.SQLText=
      "SELECT pax_id,surname,name,seats,bag_pool_num, "
      "       ckin.get_bag_pool_pax_id(pax.grp_id,pax.bag_pool_num) AS bag_pool_pax_id, "
      "       NVL(ckin.get_bagAmount2(pax_grp.grp_id,pax.pax_id,pax.bag_pool_num,rownum),0) AS bag_amount, "
      "       NVL(ckin.get_bagWeight2(pax_grp.grp_id,pax.pax_id,pax.bag_pool_num,rownum),0) AS bag_weight, "
      "       NVL(ckin.get_rkWeight2(pax_grp.grp_id,pax.pax_id,pax.bag_pool_num,rownum),0) AS rk_weight "
      "FROM pax_grp, pax "
      "WHERE pax_grp.grp_id=pax.grp_id(+) AND pax_grp.grp_id=:grp_id AND pax.pr_brd(+) IS NOT NULL";
    PaxQry.DeclareVariable("grp_id",otInteger);

    RemQry.SQLText=
      "SELECT rem_code FROM pax_rem "
      "WHERE pax_id=:pax_id AND rem_code IN ('STCR', 'EXST') "
      "ORDER BY DECODE(rem_code,'STCR',0,'EXST',1,2) ";
    RemQry.DeclareVariable("pax_id",otInteger);

    TagQry.SQLText=
      "SELECT bag_tags.no "
      "FROM bag_tags,bag2 "
      "WHERE bag_tags.grp_id=bag2.grp_id(+) AND "
      "      bag_tags.bag_num=bag2.num(+) AND "
      "      bag_tags.grp_id=:grp_id AND "
      "      NVL(bag2.bag_pool_num,1)=:bag_pool_num";
    TagQry.DeclareVariable("bag_pool_num",otInteger);
    TagQry.DeclareVariable("grp_id",otInteger);
  };

  Qry.Execute();
  vector<sopp::TTransferGrpItem> grps;
  for(;!Qry.Eof;Qry.Next())
  {
    sopp::TTransferGrpItem grp;
    grp.grp_id=Qry.FieldAsInteger("grp_id");
    if (!pr_tlg && pr_inbound_tckin)
    {
      TCkinRouteItem inboundSeg;
      TCkinRoute().GetPriorSeg(Qry.FieldAsInteger("tckin_id"),
                               Qry.FieldAsInteger("seg_no"),
                               crtIgnoreDependent, inboundSeg);
      if (inboundSeg.grp_id==NoExists) continue;

      TTripRouteItem priorAirp;
      TTripRoute().GetPriorAirp(NoExists,inboundSeg.point_arv,trtNotCancelled,priorAirp);
      if (priorAirp.point_id==NoExists) continue;

      PointsQry.SetVariable( "point_id", priorAirp.point_id );
      PointsQry.Execute();
      if (PointsQry.Eof) continue;
      TTripInfo inFlt(PointsQry);

      grp.inbound_point_dep=priorAirp.point_id;
      grp.inbound_trip=GetTripName(inFlt,ecNone);
      grp.airline_view=ElemIdToCodeNative(etAirline,inFlt.airline);
      grp.flt_no=inFlt.flt_no;
      grp.suffix_view=ElemIdToCodeNative(etSuffix,inFlt.suffix);
      grp.scd_local=UTCToLocal(inFlt.scd_out, AirpTZRegion(inFlt.airp));
      grp.airp_dep_view=ElemIdToCodeNative(etAirp,inboundSeg.airp_dep);
      grp.airp_arv_view=ElemIdToCodeNative(etAirp,Qry.FieldAsString("airp_arv"));
    }
    else
    {
      grp.inbound_point_dep=NoExists;
      grp.inbound_trip="";
      grp.airline_view=ElemIdToCodeNative(etAirline,Qry.FieldAsString("airline"));
      grp.flt_no=Qry.FieldAsInteger("flt_no");
      grp.suffix_view=ElemIdToCodeNative(etSuffix,Qry.FieldAsString("suffix"));
      grp.scd_local=Qry.FieldAsDateTime("scd");
      grp.airp_dep_view=ElemIdToCodeNative(etAirp,Qry.FieldAsString("airp_dep"));
      grp.airp_arv_view=ElemIdToCodeNative(etAirp,Qry.FieldAsString("airp_arv"));
    };

    if (pr_tlg)
    {
      grp.tlg_airp_view=ElemIdToCodeNative(etAirp,Qry.FieldAsString("airp"));
      grp.seats=!Qry.FieldIsNULL("seats")?Qry.FieldAsInteger("seats"):NoExists;
      grp.bag_amount=!Qry.FieldIsNULL("bag_amount")?Qry.FieldAsInteger("bag_amount"):NoExists;
      grp.bag_weight=!Qry.FieldIsNULL("bag_weight")?Qry.FieldAsInteger("bag_weight"):NoExists;
      grp.rk_weight=!Qry.FieldIsNULL("rk_weight")?Qry.FieldAsInteger("rk_weight"):NoExists;
      grp.weight_unit=Qry.FieldAsString("weight_unit");
      if (pr_bag)
      {
        TagQry.SetVariable("grp_id",grp.grp_id);
        TagQry.Execute();
        for(;!TagQry.Eof;TagQry.Next())
          grp.tags.push_back(TBagTagNumber("",TagQry.FieldAsFloat("no")));
      };
    }
    else
    {
      grp.seats=0;
      grp.bag_amount=0;
      grp.bag_weight=0;
      grp.rk_weight=0;
      grp.weight_unit="K";
    };
    
    //ࠧ��६�� � ����ᮬ
    string subcl=Qry.FieldAsString("subcl");
    if (!subcl.empty())
    {
      grp.subcl_view=ElemIdToCodeNative(etSubcls,subcl); //���⮩ ��� ��ᮯ஢��������� ������
      grp.subcl_priority=0;
      try
      {
        TSubclsRow &subclsRow=(TSubclsRow&)base_tables.get("subcls").get_row("code",subcl);
        grp.subcl_priority=((TClassesRow&)base_tables.get("classes").get_row("code",subclsRow.cl)).priority;
      }
      catch(EBaseTableError){};
    }
    else
    {
      grp.subcl_priority=pr_tlg?0:10;
    };
    
    //���ᠦ���
    PaxQry.SetVariable("grp_id",grp.grp_id);
    PaxQry.Execute();
    if (PaxQry.Eof) continue; //����� ��㯯� - �� ����頥� � grps
    if (pr_tlg)
    {
      for(;!PaxQry.Eof;PaxQry.Next())
      {
        sopp::TTransferPaxItem pax;
        //�࠭᫨���� �� �㦭� ⠪ ��� ⥫��ࠬ�� PTM, BTM ������ ��室��� �� ��⨭᪮�
        //� ॣ������ (ᯨ᪨) ������ �஢������� �� ��⨭᪮�
        pax.surname=PaxQry.FieldAsString("surname");
        pax.name=PaxQry.FieldAsString("name");
        grp.pax.push_back(pax);
      };
    }
    else
    {
      for(;!PaxQry.Eof;PaxQry.Next())
      {
        sopp::TTransferPaxItem pax;
        if (subcl.empty())
        {
          pax.surname="UNACCOMPANIED";
          pax.seats=0;
        }
        else
        {
          pax.surname=PaxQry.FieldAsString("surname");
          pax.name=PaxQry.FieldAsString("name");
          pax.seats=PaxQry.FieldAsInteger("seats");
          if (pax.seats>1)
          {
            RemQry.SetVariable("pax_id",PaxQry.FieldAsInteger("pax_id"));
            RemQry.Execute();
            for(int i=2; i<=pax.seats; i++)
            {
              pax.name+="/";
              if (!RemQry.Eof)
                pax.name+=RemQry.FieldAsString("rem_code");
              else
                pax.name+="EXST";
            };
          };
        };
        //����� ���ᠦ��
        pax.bag_amount=PaxQry.FieldAsInteger("bag_amount");
        pax.bag_weight=PaxQry.FieldAsInteger("bag_weight");
        pax.rk_weight=PaxQry.FieldAsInteger("rk_weight");
        
        grp.seats+=pax.seats;
        grp.bag_amount+=pax.bag_amount;
        grp.bag_weight+=pax.bag_weight;
        grp.rk_weight+=pax.rk_weight;
        
        if (pr_bag &&
            (subcl.empty() ||
             !PaxQry.FieldIsNULL("bag_pool_num") &&
             !PaxQry.FieldIsNULL("pax_id") &&
             !PaxQry.FieldIsNULL("bag_pool_pax_id") &&
             PaxQry.FieldAsInteger("pax_id")==PaxQry.FieldAsInteger("bag_pool_pax_id")))
        {
          TagQry.SetVariable("grp_id",grp.grp_id);
          if (subcl.empty())
            TagQry.SetVariable("bag_pool_num", 1);
          else
            TagQry.SetVariable("bag_pool_num", PaxQry.FieldAsInteger("bag_pool_num"));
          TagQry.Execute();
          for(;!TagQry.Eof;TagQry.Next())
          {
            pax.tags.push_back(TBagTagNumber("",TagQry.FieldAsFloat("no")));
            grp.tags.push_back(TBagTagNumber("",TagQry.FieldAsFloat("no")));
          };
        };
        
        grp.pax.push_back(pax);
      };
    };
    
    grps.push_back(grp);
  };

  sort(grps.begin(),grps.end());

  //�ନ�㥬 XML

  NewTextChild(resNode,"trip",GetTripName(info,ecNone));

  xmlNodePtr trferNode=NewTextChild(resNode,"transfer");

  xmlNodePtr grpsNode;
  vector<sopp::TTransferGrpItem>::const_iterator iGrpPrior=grps.end();
  for(vector<sopp::TTransferGrpItem>::const_iterator iGrp=grps.begin();iGrp!=grps.end();++iGrp)
  {
    if (iGrpPrior==grps.end() ||
        iGrpPrior->airline_view!=iGrp->airline_view ||
        iGrpPrior->flt_no!=iGrp->flt_no ||
        iGrpPrior->suffix_view!=iGrp->suffix_view ||
        iGrpPrior->scd_local!=iGrp->scd_local ||
        iGrpPrior->airp_dep_view!=iGrp->airp_dep_view ||
        iGrpPrior->inbound_point_dep!=iGrp->inbound_point_dep ||
        iGrpPrior->airp_arv_view!=iGrp->airp_arv_view ||
        iGrpPrior->subcl_view!=iGrp->subcl_view)
    {
      xmlNodePtr node=NewTextChild(trferNode,"trfer_flt");

      ostringstream trip;
      trip << iGrp->airline_view
           << setw(3) << setfill('0') << iGrp->flt_no
           << iGrp->suffix_view << "/"
           << DateTimeToStr(iGrp->scd_local,"dd");

      NewTextChild(node,"trip",trip.str());
      if (pr_tlg) NewTextChild(node,"airp",iGrp->tlg_airp_view);
      NewTextChild(node,"airp_dep",iGrp->airp_dep_view);
      NewTextChild(node,"airp_arv",iGrp->airp_arv_view);
      NewTextChild(node,"subcl",iGrp->subcl_view);
      NewTextChild(node,"point_dep",iGrp->inbound_point_dep,NoExists); //⮫쪮 ��� GetInboundTCkin
      NewTextChild(node,"trip2",iGrp->inbound_trip,"");                //⮫쪮 ��� GetInboundTCkin
      grpsNode=NewTextChild(node,"grps");
    };

    xmlNodePtr grpNode=NewTextChild(grpsNode,"grp");
    if (pr_tlg || !reqInfo->desk.compatible(VERSION_WITH_BAG_POOLS))
    {
      if (iGrp->bag_amount!=NoExists)
        NewTextChild(grpNode,"bag_amount",iGrp->bag_amount);
      else
        NewTextChild(grpNode,"bag_amount");
      if (iGrp->bag_weight!=NoExists)
        NewTextChild(grpNode,"bag_weight",iGrp->bag_weight);
      else
        NewTextChild(grpNode,"bag_weight");
      if (iGrp->rk_weight!=NoExists)
        NewTextChild(grpNode,"rk_weight",iGrp->rk_weight);
      else
        NewTextChild(grpNode,"rk_weight");
      NewTextChild(grpNode,"weight_unit",iGrp->weight_unit);
      NewTextChild(grpNode,"seats",iGrp->seats);

      vector<string> tagRanges;
      GetTagRanges(iGrp->tags, tagRanges);
      if (!tagRanges.empty())
      {
        xmlNodePtr node=NewTextChild(grpNode,"tag_ranges");
        for(vector<string>::const_iterator r=tagRanges.begin(); r!=tagRanges.end(); ++r)
          NewTextChild(node,"range",*r);
      };
    };

    if (!iGrp->pax.empty())
    {
      xmlNodePtr paxsNode=NewTextChild(grpNode,"passengers");
      for(vector<sopp::TTransferPaxItem>::const_iterator iPax=iGrp->pax.begin(); iPax!=iGrp->pax.end(); ++iPax)
      {
        xmlNodePtr paxNode=NewTextChild(paxsNode,"pax");
        NewTextChild(paxNode,"surname",iPax->surname);
        NewTextChild(paxNode,"name",iPax->name,"");
        if (!(pr_tlg || !reqInfo->desk.compatible(VERSION_WITH_BAG_POOLS)))
        {
          if (iPax->bag_amount!=NoExists)
            NewTextChild(paxNode,"bag_amount",iPax->bag_amount,0);
          if (iPax->bag_weight!=NoExists)
            NewTextChild(paxNode,"bag_weight",iPax->bag_weight,0);
          if (iPax->rk_weight!=NoExists)
            NewTextChild(paxNode,"rk_weight",iPax->rk_weight,0);
          NewTextChild(paxNode,"seats",iPax->seats,1);

          vector<string> tagRanges;
          GetTagRanges(iPax->tags, tagRanges);
          if (!tagRanges.empty())
          {
            xmlNodePtr node=NewTextChild(paxNode,"tag_ranges");
            for(vector<string>::const_iterator r=tagRanges.begin(); r!=tagRanges.end(); ++r)
              NewTextChild(node,"range",*r);
          };
        };
      };
    };

    iGrpPrior=iGrp;
  };
};

void SoppInterface::GetTransfer(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  bool pr_inbound_tckin=(strcmp((char *)reqNode->name, "GetInboundTCkin") == 0);

  bool pr_out=NodeAsInteger("pr_out",reqNode)!=0;
  bool pr_tlg=true;
  if (GetNode("pr_tlg",reqNode)!=NULL)
    pr_tlg=NodeAsInteger("pr_tlg",reqNode)!=0;
    
  bool pr_bag=(strcmp((char *)reqNode->name, "GetInboundTCkin") == 0) ||
              (strcmp((char *)reqNode->name, "GetBagTransfer") == 0);
  int point_id=NodeAsInteger("point_id",reqNode);

  GetTransfer(pr_inbound_tckin, pr_out, pr_tlg, pr_bag, point_id, resNode);
  
  get_new_report_form("SOPPTrfer", reqNode, resNode);
  STAT::set_variables(resNode);
};

struct TSegBSMInfo
{
  BSM::TBSMAddrs BSMaddrs;
  map<int/*grp_id*/,BSM::TTlgContent> BSMContentBefore;
};

void DeletePaxGrp( const TTypeBSendInfo &sendInfo, int grp_id, bool toLog,
                   TQuery &PaxQry, TQuery &DelQry, map<int/*point_id*/,TSegBSMInfo> &BSMsegs )
{
  int point_id=sendInfo.point_id;

  const char* pax_sql=
    "SELECT pax_id,surname,name,pers_type,reg_no,pr_brd FROM pax WHERE grp_id=:grp_id";
  const char* del_sql=
    "BEGIN "
    "  DELETE FROM "
    "   (SELECT * "
    "    FROM trip_comp_layers,pax,pax_grp,grp_status_types "
    "    WHERE trip_comp_layers.pax_id=pax.pax_id AND "
    "          trip_comp_layers.layer_type=grp_status_types.layer_type AND "
    "          pax.grp_id=pax_grp.grp_id AND "
    "          pax_grp.status=grp_status_types.code AND "
    "          pax_grp.grp_id=:grp_id); "
    "  UPDATE pax SET refuse=:refuse,pr_brd=NULL WHERE grp_id=:grp_id; "
    "  mvd.sync_pax_grp(:grp_id,:term); "
    "  ckin.check_grp(:grp_id); "
    "END;";
    
  if (strcmp(PaxQry.SQLText.SQLText(),pax_sql)!=0)
  {
    PaxQry.Clear();
    PaxQry.SQLText=pax_sql;
    PaxQry.DeclareVariable("grp_id",otInteger);
  };
  
  if (strcmp(DelQry.SQLText.SQLText(),del_sql)!=0)
  {
    DelQry.Clear();
    DelQry.SQLText=del_sql;
    DelQry.CreateVariable( "refuse", otString, refuseAgentError );
    DelQry.CreateVariable( "term", otString, TReqInfo::Instance()->desk.code );
    DelQry.DeclareVariable("grp_id",otInteger);
  };
  
  //����ࠥ� ����� BSMsegs
  if (BSMsegs.find(sendInfo.point_id)==BSMsegs.end())
  {
    BSM::IsSend(sendInfo,BSMsegs[sendInfo.point_id].BSMaddrs);
  };
  
  TSegBSMInfo &BSMseg=BSMsegs[sendInfo.point_id];
  if (!BSMseg.BSMaddrs.empty())
  {
    BSM::TTlgContent BSMContent;
    BSM::LoadContent(grp_id,BSMContent);
    BSMseg.BSMContentBefore[grp_id]=BSMContent;
  };
  //AODB
  bool SyncAODB=is_sync_aodb(point_id);

  PaxQry.SetVariable("grp_id",grp_id);
  PaxQry.Execute();
  for(;!PaxQry.Eof;PaxQry.Next())
  {
    const char* surname=PaxQry.FieldAsString("surname");
    const char* name=PaxQry.FieldAsString("name");
    const char* pers_type=PaxQry.FieldAsString("pers_type");
    int pax_id=PaxQry.FieldAsInteger("pax_id");
    int reg_no=PaxQry.FieldAsInteger("reg_no");
    bool boarded=!PaxQry.FieldIsNULL("pr_brd") && PaxQry.FieldAsInteger("pr_brd")!=0;

    if (SyncAODB)
    {
      update_aodb_pax_change(point_id, pax_id, reg_no, "�");
      if (boarded)
        update_aodb_pax_change(point_id, pax_id, reg_no, "�");
    };

    if (toLog)
      TReqInfo::Instance()->MsgToLog((string)"���ᠦ�� "+surname+(*name!=0?" ":"")+name+" ("+pers_type+") ࠧॣ����஢��. "+
                                     "��稭� �⪠�� � ॣ����樨: "+refuseAgentError+". ",
                                     ASTRA::evtPax,
                                     point_id,
                                     reg_no,
                                     grp_id);
  };

  DelQry.SetVariable("grp_id",grp_id);
  DelQry.Execute();
};

void DeletePassengers( int point_id, const TDeletePaxFilter &filter,
                       map<int,TTripInfo> &segs )
{
  segs.clear();
  TReqInfo *reqInfo = TReqInfo::Instance();

  TQuery Qry(&OraSession);
	Qry.Clear();
	Qry.SQLText=
	  "SELECT airline,flt_no,suffix,airp,scd_out, "
	  "       point_num,first_point,pr_tranzit "
    "FROM points "
    "WHERE point_id=:point_id AND pr_reg<>0 AND pr_del=0 FOR UPDATE";
  Qry.CreateVariable("point_id",otInteger,point_id);
  Qry.Execute();
  if (Qry.Eof) throw AstraLocale::UserException("MSG.FLIGHT.CHANGED.REFRESH_DATA");
  TTripInfo fltInfo(Qry);

  TTypeBSendInfo sendInfo(fltInfo);
  sendInfo.point_id=point_id;
  sendInfo.point_num=Qry.FieldAsInteger("point_num");
  sendInfo.first_point=Qry.FieldIsNULL("first_point")?NoExists:Qry.FieldAsInteger("first_point");
  sendInfo.pr_tranzit=Qry.FieldAsInteger("pr_tranzit")!=0;
  sendInfo.tlg_type="BSM";

  map<int/*point_id*/,TSegBSMInfo> BSMsegs;

  TQuery DelQry(&OraSession);
  TQuery PaxQry(&OraSession);

  TQuery TCkinQry(&OraSession);
  TCkinQry.Clear();
  TCkinQry.SQLText=
    "SELECT points.point_id,points.airline,points.flt_no,points.suffix, "
    "       points.airp,points.scd_out, "
    "       points.point_num,points.first_point,points.pr_tranzit, "
    "       pax_grp.grp_id,pr_depend "
    "FROM tckin_pax_grp,pax_grp,points "
    "WHERE tckin_pax_grp.grp_id=pax_grp.grp_id AND "
    "      pax_grp.point_dep=points.point_id AND "
    "      tckin_id=:tckin_id AND seg_no>:seg_no "
    "ORDER BY seg_no FOR UPDATE";  //?����� ���� ����� �⤥�쭮?
  TCkinQry.DeclareVariable("tckin_id",otInteger);
  TCkinQry.DeclareVariable("seg_no",otInteger);

  ostringstream sql;
  sql << "SELECT pax_grp.grp_id, \n"
         "       tckin_pax_grp.tckin_id, tckin_pax_grp.seg_no \n"
         "FROM pax_grp, tckin_pax_grp \n"
         "WHERE pax_grp.point_dep=:point_id \n";
  if ( filter.inbound_point_dep!=NoExists )
    sql << "      AND pax_grp.grp_id=tckin_pax_grp.grp_id \n";
  else
    sql << "      AND pax_grp.grp_id=tckin_pax_grp.grp_id(+) \n";
  if ( !filter.status.empty() )
    sql << "      AND pax_grp.status=:status \n";
  if ( filter.inbound_point_dep!=NoExists )
    sql << "      AND pax_grp.bag_refuse=0 AND pax_grp.status<>'T' \n";

  Qry.Clear();
  Qry.SQLText=sql.str().c_str();
  if ( !filter.status.empty() )
    Qry.CreateVariable( "status", otString, filter.status );
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.Execute();
  if (Qry.Eof) return;
  
  segs[point_id]=fltInfo;
  for(;!Qry.Eof;Qry.Next())
  {
    int tckin_id=Qry.FieldIsNULL("tckin_id")?NoExists:Qry.FieldAsInteger("tckin_id");
    int tckin_seg_no=Qry.FieldIsNULL("seg_no")?NoExists:Qry.FieldAsInteger("seg_no");
  
    if ( filter.inbound_point_dep!=NoExists )
    {
      if (tckin_id==NoExists || tckin_seg_no==NoExists) continue;
    
      TCkinRouteItem inboundSeg;
      TCkinRoute().GetPriorSeg(tckin_id, tckin_seg_no, crtIgnoreDependent, inboundSeg);
      if (inboundSeg.grp_id==NoExists) continue;
      
      TTripRouteItem priorAirp;
      TTripRoute().GetPriorAirp(NoExists,inboundSeg.point_arv,trtNotCancelled,priorAirp);
      if (priorAirp.point_id==NoExists) continue;
      
      if (priorAirp.point_id!=filter.inbound_point_dep) continue;
    };
  
    //�஡������� �� �ᥬ ��㯯�� ३�
    int grp_id=Qry.FieldAsInteger("grp_id");

    //��殮� ᪢���类� �� �।���� ᥣ���⮢
    if (tckin_id!=NoExists && tckin_seg_no!=NoExists)
    {
      if (SeparateTCkin(grp_id,cssAllPrevCurr,cssNone,NoExists)!=NoExists)
      {
        //ࠧॣ�����㥬 �� ᪢���� ᥣ����� ��᫥ ��襣�
        TCkinQry.SetVariable("tckin_id",tckin_id);
        TCkinQry.SetVariable("seg_no",tckin_seg_no);
        TCkinQry.Execute();
        for(;!TCkinQry.Eof;TCkinQry.Next())
        {
          //if (TCkinQry.FieldAsInteger("pr_depend")==0) break;
          //�� ���뢠�� �易������ ��᫥����� ᪢����� ᥣ���⮢ - �㯮 ࠧॣ�����㥬 ���

          int tckin_point_id=TCkinQry.FieldAsInteger("point_id");
          int tckin_grp_id=TCkinQry.FieldAsInteger("grp_id");

          if (segs.find(tckin_point_id)==segs.end())
          {
            fltInfo.Init(TCkinQry);
            segs[tckin_point_id]=fltInfo;
          };

          TTypeBSendInfo tckinSendInfo(fltInfo);
          tckinSendInfo.point_id=tckin_point_id;
          tckinSendInfo.point_num=TCkinQry.FieldAsInteger("point_num");
          tckinSendInfo.first_point=TCkinQry.FieldIsNULL("first_point")?NoExists:TCkinQry.FieldAsInteger("first_point");
          tckinSendInfo.pr_tranzit=TCkinQry.FieldAsInteger("pr_tranzit")!=0;
          tckinSendInfo.tlg_type="BSM";

          DeletePaxGrp( tckinSendInfo, tckin_grp_id, true, PaxQry, DelQry, BSMsegs);
        };
      };
    };
    
    DeletePaxGrp( sendInfo, grp_id, filter.inbound_point_dep!=NoExists, PaxQry, DelQry, BSMsegs);
  };

  //�����⠥� ���稪� �� �ᥬ ३ᠬ, ������ ᪢���� ᥣ�����
  Qry.Clear();
	Qry.SQLText=
	 "BEGIN "
	 "  ckin.recount(:point_id); "
   "END;";
  Qry.DeclareVariable("point_id",otInteger);
  for(map<int,TTripInfo>::iterator i=segs.begin();i!=segs.end();++i)
  {
    Qry.SetVariable("point_id",i->first);
    Qry.Execute();
    check_overload_alarm( i->first );
    check_waitlist_alarm( i->first );
    check_brd_alarm( i->first );
    check_TrferExists( i->first );
  };

  if ( filter.inbound_point_dep==NoExists )
  {
    if ( filter.status.empty() )
      reqInfo->MsgToLog( "�� ���ᠦ��� ࠧॣ����஢���", evtPax, point_id );
    else
    	reqInfo->MsgToLog( string("�� ���ᠦ��� � ����ᮬ ") + filter.status + " ࠧॣ����஢���", evtPax, point_id );
  };

  //BSM
  for(map<int/*point_id*/,TSegBSMInfo>::const_iterator s=BSMsegs.begin();
                                                       s!=BSMsegs.end(); ++s)
  {
    if (s->second.BSMaddrs.empty()) continue; //��� ᥣ���� �� ������ ���� ��ࠢ��
    for(map<int/*grp_id*/,BSM::TTlgContent>::const_iterator i=s->second.BSMContentBefore.begin();
                                                            i!=s->second.BSMContentBefore.end(); ++i)
    {
      BSM::Send(s->first,i->first,i->second,s->second.BSMaddrs);
    };
  };
}

void DeletePassengersAnswer( map<int,TTripInfo> &segs, xmlNodePtr resNode )
{
	TQuery Qry(&OraSession);
	Qry.Clear();
  Qry.SQLText=
    "SELECT pr_etstatus FROM trip_sets WHERE point_id=:point_id ";
  Qry.DeclareVariable("point_id",otInteger);

	xmlNodePtr segsNode=NewTextChild(resNode,"segments");
	for(map<int,TTripInfo>::iterator i=segs.begin();i!=segs.end();i++)
  {
    bool pr_etl_only=GetTripSets(tsETLOnly,i->second);
    Qry.SetVariable("point_id",i->first);
    Qry.Execute();
    int pr_etstatus=-1;
    if (!Qry.Eof)
      pr_etstatus=Qry.FieldAsInteger("pr_etstatus");

      //����� �ନ�㥬 ᯨ᮪ ३ᮢ, �� ������ ���� ᤥ���� ᬥ�� ����� ��
    xmlNodePtr segNode=NewTextChild(segsNode,"segment");
    NewTextChild( segNode, "point_id", i->first);
    NewTextChild( segNode, "pr_etl_only", (int)pr_etl_only );
    NewTextChild( segNode, "pr_etstatus", pr_etstatus );
  }
}

void SoppInterface::DeleteAllPassangers(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  int point_id = NodeAsInteger( "point_id", reqNode );
  TDeletePaxFilter filter;
  if (GetNode( "inbound_point_dep", reqNode )!=NULL)
    filter.inbound_point_dep = NodeAsInteger( "inbound_point_dep", reqNode );
  map<int,TTripInfo> segs;
	DeletePassengers( point_id, filter, segs );
  DeletePassengersAnswer( segs, resNode );
  if (filter.inbound_point_dep==NoExists)
  {
    //�� ࠧॣ������ ��� ���ᠦ�஢ ३�
    TSOPPTrips trips;
    string errcity = internal_ReadData( trips, NoExists, NoExists, false, tSOPP, point_id );
    xmlNodePtr dataNode = NewTextChild( resNode, "data" );
    buildSOPP( trips, errcity, dataNode );
    if ( !errcity.empty() )
    {
      AstraLocale::showErrorMessage( "MSG.CITY.REGION_NOT_DEFINED.NOT_ALL_FLIGHTS_ARE_SHOWN",
      	                             LParams() << LParam("city", ElemIdToCodeNative(etCity,errcity)));
      return;
    };
  }
  else
  {
    //�� ࠧॣ������ ���ᠦ�஢, �ਡ뢠��� ��몮���� ३ᮬ
    //��ࠢ�塞 ���������� �����
    GetTransfer(true, true, false, true, point_id, resNode);
  };
  AstraLocale::showMessage( "MSG.UNREGISTRATION_ALL_PASSENGERS" );
}

void SoppInterface::WriteTrips(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
	xmlNodePtr node = NodeAsNode( "trips", reqNode );
	node = node->children;
	TQuery Qry(&OraSession);
	xmlNodePtr n, stnode;
	TTripInfo fltInfo;
	while ( node ) {
		n = node->children;
		int point_id = NodeAsIntegerFast( "point_id", n );
		ProgTrace( TRACE5, "point_id=%d", point_id );
		xmlNodePtr ddddNode = GetNodeFast( "stations", n );
		
		if ( ddddNode ) {
			ddddNode = ddddNode->children;
			while ( ddddNode ) {
				xmlNodePtr x = GetNode( "@mode", ddddNode );
				string work_mode = NodeAsString( x );
		    Qry.Clear();
  	    Qry.SQLText = "DELETE trip_stations	WHERE point_id=:point_id AND work_mode=:work_mode";
    	  Qry.CreateVariable( "point_id", otInteger, point_id );
    	  Qry.CreateVariable( "work_mode", otString, work_mode );
  	    Qry.Execute();
    	  Qry.Clear();
    	  Qry.SQLText = "INSERT INTO trip_stations(point_id,desk,work_mode,pr_main) "\
    	                " SELECT :point_id,desk,:work_mode,:pr_main FROM stations,points "\
    	                "  WHERE points.point_id=:point_id AND stations.airp=points.airp AND name=:name";
    	  Qry.CreateVariable( "point_id", otInteger, point_id );
    	  Qry.DeclareVariable( "name", otString );
    	  Qry.DeclareVariable( "pr_main", otInteger );
    	  Qry.CreateVariable( "work_mode", otString, work_mode );
    	  stnode = ddddNode->children; //tag name
    	  string tolog;
    	  string name;
    	  bool pr_main;
	      vector<string> terms;
      	while ( stnode ) {
      		name = NodeAsString( stnode );
          if ( find( terms.begin(), terms.end(), name ) == terms.end() ) {
            terms.push_back( name );
      		  Qry.SetVariable( "name", name );
      	  	pr_main = GetNode( "pr_main", stnode );
        		Qry.SetVariable( "pr_main", pr_main );
        		Qry.Execute();
        		if ( !tolog.empty() )
        				tolog += ", ";
        			tolog += name;
        		if ( pr_main )
        			tolog += " (�������)";
          }
    		  stnode = stnode->next;
      	}
      	if ( work_mode == "�" ) {
      	  if ( tolog.empty() )
      		  tolog = "�� �����祭� �⮩�� ॣ����樨";
      	  else
      	  	tolog = "�����祭� �⮩�� ॣ����樨: " + tolog;
      	}
      	if ( work_mode == "�" ) {
        	if ( tolog.empty() )
      		  tolog = "�� �����祭� ��室� �� ��ᠤ��";
      	  else
      	  	tolog = "�����祭� ��室� �� ��ᠤ��: " + tolog;
      	}
      	TReqInfo::Instance()->MsgToLog( tolog, evtFlt, point_id );
				ddddNode = ddddNode->next;
			}
      check_DesksGates( point_id );
		}
  	xmlNodePtr stagesNode = GetNode( "tripstages", node );
    if ( stagesNode ) {
  	  TMapTripStages stages;
  	  TTripStages::ParseStages( stagesNode, stages );
  	  TTripStages::WriteStages( point_id, stages );
  	}
  	xmlNodePtr luggageNode = GetNode( "luggage", node );
  	TReqInfo *r = TReqInfo::Instance();
  	if ( luggageNode &&
	       find( r->user.access.rights.begin(),
               r->user.access.rights.end(), 370 ) != r->user.access.rights.end() ) {
  		xmlNodePtr max_cNode = GetNode( "max_commerce", luggageNode );
  		if ( max_cNode ) {
  		  TFlightMaxCommerce maxCommerce;
  		  int mc = NodeAsInteger( max_cNode );
  		  if ( mc == 0 )
  			  maxCommerce.SetValue( ASTRA::NoExists );
        else
          maxCommerce.SetValue( mc );
        maxCommerce.Save( point_id );
  		}
  		xmlNodePtr trip_loadNode = GetNode( "trip_load", luggageNode );
  		if ( trip_loadNode ) {
  			Qry.Clear();
  			Qry.SQLText =
  			 "BEGIN "
  			 " SELECT airp INTO :airp_arv FROM points WHERE point_id=:point_arv; "
  			 " UPDATE trip_load SET cargo=:cargo,mail=:mail"
  			 "  WHERE point_dep=:point_id AND point_arv=:point_arv; "
  			 " IF SQL%NOTFOUND THEN "
  			 "  INSERT INTO trip_load(point_dep,airp_dep,point_arv,airp_arv,cargo,mail)  "
  			 "   SELECT point_id,airp,:point_arv,:airp_arv,:cargo,:mail FROM points "
  			 "    WHERE point_id=:point_id; "
  			 " END IF;"
  			 "END;";
  			Qry.CreateVariable( "point_id", otInteger, point_id );
  			Qry.DeclareVariable( "point_arv", otInteger );
  			Qry.DeclareVariable( "airp_arv", otString );
  			Qry.DeclareVariable( "cargo", otInteger );
  			Qry.DeclareVariable( "mail", otInteger );
  			xmlNodePtr load = trip_loadNode->children;
  			while( load ) {
  				xmlNodePtr x = load->children;
  				Qry.SetVariable( "point_arv", NodeAsIntegerFast( "point_arv", x ) );
  				int cargo = NodeAsIntegerFast( "cargo", x );
  				Qry.SetVariable( "cargo", cargo );
  				int mail = NodeAsIntegerFast( "mail", x );
  				Qry.SetVariable( "mail", mail );
  				Qry.Execute();
          TReqInfo::Instance()->MsgToLog(
          	string( "���ࠢ����� " ) + Qry.GetVariableAsString( "airp_arv" ) + ": " +
            "��� " + IntToString( cargo ) + " ��., " +
            "���� " + IntToString( mail ) + " ��.", evtFlt, point_id );
  			  load = load->next;
  			}
  		}
  		if ( max_cNode || trip_loadNode ) { // �뫨 ��������� � ���
  			//�஢�ਬ ���ᨬ����� ����㧪�
				check_overload_alarm( point_id );
  		}
  	}
		node = node->next;
	}
	AstraLocale::showMessage( "MSG.DATA_SAVED" );
}

void GetBirks( int point_id, xmlNodePtr dataNode )
{
  xmlNodePtr node = NewTextChild( dataNode, "birks" );
	TQuery Qry(&OraSession);
	Qry.SQLText =
	  "SELECT COUNT(*) AS nobrd "
	  "FROM pax_grp, pax "
	  "WHERE pax_grp.grp_id=pax.grp_id AND point_dep=:point_id AND pr_brd=0";
	Qry.CreateVariable( "point_id", otInteger, point_id );
	Qry.Execute();
	NewTextChild( node, "nobrd", Qry.FieldAsInteger( "nobrd" ) );
  Qry.SQLText =
    "SELECT sopp.get_birks(:point_id,:vlang) AS birks FROM dual";
  Qry.CreateVariable( "vlang", otString, TReqInfo::Instance()->desk.lang.empty() );
	Qry.Execute();
	NewTextChild( node, "birks", Qry.FieldAsString( "birks" ) );
}


/*void GetLuggage( int point_id, Luggage &lug, bool pr_brd )
{
	TQuery Qry(&OraSession);

	ostringstream sql;

	sql <<
  	 "SELECT a.point_arv,DECODE(a.class,' ',NULL,a.class) AS class, "
  	 "       a.seatsadult,a.seatschild,a.seatsbaby, "
  	 "       a.adult,a.child,a.baby, "
  	 "       b.bag_weight,b.rk_weight, "
  	 "       e.excess "
  	 "FROM "

  	 //�������� �� ���-�� ���ᠦ�஢:
     "	 (SELECT pax_grp.point_arv, NVL(class,' ') AS class, "
     "           SUM(DECODE(pers_type,'��',seats,0)) AS seatsadult, "
     "           SUM(DECODE(pers_type,'��',seats,0)) AS seatschild, "
     "           SUM(DECODE(pers_type,'��',seats,0)) AS seatsbaby, "
     "           SUM(DECODE(pers_type,'��',1,0)) AS adult, "
     "           SUM(DECODE(pers_type,'��',1,0)) AS child, "
     "           SUM(DECODE(pers_type,'��',1,0)) AS baby "
     "   FROM pax_grp,pax "
     "   WHERE pax_grp.grp_id=pax.grp_id(+) AND "
     "         point_dep=:point_id AND "
     "         (pax.grp_id IS NULL AND pax_grp.class IS NULL OR "
     "          pax.grp_id IS NOT NULL) AND ";
  if (pr_brd)
    sql << "   pr_brd(+)=1 ";
  else
    sql << "   pr_brd(+) IS NOT NULL ";
  sql <<
     "   GROUP BY pax_grp.point_arv, NVL(class,' ')) a, "

     //�������� �� ���� ������:
     "   (SELECT pax_grp.point_arv, NVL(class,' ') AS class, "
     "           SUM(DECODE(pr_cabin,0,weight,0)) AS bag_weight, "
     "           SUM(DECODE(pr_cabin,1,weight,0)) AS rk_weight "
     "    FROM pax_grp,bag2 "
     "    WHERE pax_grp.grp_id=bag2.grp_id AND "
     "          pax_grp.point_dep=:point_id AND ";
  if (pr_brd)
    sql << "    ckin.bag_pool_boarded(bag2.grp_id,bag2.bag_pool_num,pax_grp.class,pax_grp.bag_refuse)<>0 ";
  else
    sql << "    ckin.bag_pool_refused(bag2.grp_id,bag2.bag_pool_num,pax_grp.class,pax_grp.bag_refuse)=0 ";
  sql <<
     "    GROUP BY pax_grp.point_arv, NVL(class,' ')) b, "

     //�������� �� ����稢������ ����:
     "   (SELECT pax_grp.point_arv, NVL(class,' ') AS class, "
     "	         SUM(excess) AS excess "
     "	  FROM pax_grp "
     "    WHERE point_dep=:point_id AND ";
  if (pr_brd)
    sql << "    ckin.excess_boarded(grp_id,class,bag_refuse)<>0 ";
  else
    sql << "    bag_refuse=0 ";
  sql <<
     "    GROUP BY pax_grp.point_arv, NVL(class,' ')) e "
     "WHERE a.point_arv=b.point_arv(+) AND "
     "      a.class=b.class(+) AND "
     "      a.point_arv=e.point_arv(+) AND "
     "      a.class=e.class(+) ";
  Qry.SQLText = sql.str().c_str();
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.Execute();
  for(;!Qry.Eof;Qry.Next())
  {
    PaxLoad paxload;
    paxload.point_arv = Qry.FieldAsInteger( "point_arv" );
    paxload.cl = Qry.FieldAsString( "class" );
    paxload.seatsadult = Qry.FieldAsInteger( "seatsadult" );
    paxload.seatschild = Qry.FieldAsInteger( "seatschild" );
    paxload.seatsbaby = Qry.FieldAsInteger( "seatsbaby" );
    paxload.adult = Qry.FieldAsInteger( "adult" );
    paxload.child = Qry.FieldAsInteger( "child" );
    paxload.baby = Qry.FieldAsInteger( "baby" );
    paxload.bag_weight = Qry.FieldAsInteger( "bag_weight" );
    paxload.rk_weight = Qry.FieldAsInteger( "rk_weight" );
    paxload.excess = Qry.FieldAsInteger( "excess" );
    lug.vpaxload.push_back( paxload );
  };

  Qry.Clear();
  Qry.SQLText =
   "SELECT airp,scd_out,act_out,points.pr_del pr_del,max_commerce,pr_tranzit,first_point,point_num "
   " FROM points,trip_sets "
    "WHERE points.point_id=:point_id AND trip_sets.point_id(+)=points.point_id ";
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.Execute();
  lug.max_commerce = Qry.FieldAsInteger( "max_commerce" );
  lug.pr_edit = !Qry.FieldIsNULL( "act_out" ) || Qry.FieldAsInteger( "pr_del" ) != 0;
  lug.scd_out = Qry.FieldAsDateTime( "scd_out" );
  lug.region = AirpTZRegion( Qry.FieldAsString( "airp" ) );
  int pr_tranzit = Qry.FieldAsInteger( "pr_tranzit" );
  int first_point = Qry.FieldAsInteger( "first_point" );
  int point_num = Qry.FieldAsInteger( "point_num" );
	if ( !pr_tranzit )
    first_point = point_id;
	Qry.Clear();
	Qry.SQLText =
	 "SELECT cargo,mail,a.airp airp_arv,a.airp_fmt airp_arv_fmt, a.point_id point_arv, a.point_num "
	 " FROM trip_load, "
	 "( SELECT point_id, point_num, airp, airp_fmt FROM points "
	 "   WHERE first_point=:first_point AND point_num>:point_num AND pr_del=0 ) a, "
	 "( SELECT MIN(point_num) as point_num FROM points "
	 "   WHERE first_point=:first_point AND point_num>:point_num AND pr_del=0 "
	 "  GROUP BY airp ) b "
	 "WHERE a.point_num=b.point_num AND trip_load.point_dep(+)=:point_id AND "
	 "      trip_load.point_arv(+)=a.point_id "
	 "ORDER BY a.point_num ";
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.CreateVariable( "first_point", otInteger, first_point );
  Qry.CreateVariable( "point_num", otInteger, point_num );
  Qry.Execute();
  while ( !Qry.Eof ) {
  	Cargo cargo;
  	cargo.cargo = Qry.FieldAsInteger( "cargo" );
  	cargo.mail = Qry.FieldAsInteger( "mail" );
  	cargo.point_arv = Qry.FieldAsInteger( "point_arv" );
  	cargo.airp_arv =  Qry.FieldAsString( "airp_arv" );
  	cargo.airp_arv_fmt = (TElemFmt)Qry.FieldAsInteger( "airp_arv_fmt" );
  	lug.vcargo.push_back( cargo );
  	Qry.Next();
  }
}

void GetLuggage( int point_id, xmlNodePtr dataNode )
{



	Luggage lug;
//	GetLuggage( point_id, lug );
	xmlNodePtr node = NewTextChild( dataNode, "luggage" );
  NewTextChild( node, "max_commerce", lug.max_commerce );
  NewTextChild( node, "pr_edit", lug.pr_edit );

  // ��।��塞 ��� �� ᥩ��
  int summer = is_dst( NowUTC(), lug.region );
  TQuery Qry(&OraSession);
  Qry.SQLText =
   "SELECT "\
   " code, DECODE(:summer,1,weight_sum,weight_win) as weight "\
   " FROM pers_types";
  Qry.CreateVariable( "summer", otInteger, summer );
	Qry.Execute();
	xmlNodePtr wm = NewTextChild( node, "weightman" );
	while ( !Qry.Eof ) {
		xmlNodePtr weightNode = NewTextChild( wm, "weight" );
		NewTextChild( weightNode, "code", Qry.FieldAsString( "code" ) );
		NewTextChild( weightNode, "weight", Qry.FieldAsInteger( "weight" ) );
		Qry.Next();
	}
  int adult = 0;
	int child = 0;
	int baby = 0;
	int rk_weight = 0;
	int bag_weight = 0;
	for ( vector<PaxLoad>::iterator p=lug.vpaxload.begin(); p!=lug.vpaxload.end(); p++ ) {
	 	adult += p->adult;
	 	child += p->child;
	 	baby += p->baby;
	 	rk_weight += p->rk_weight;
	 	bag_weight += p->bag_weight;
	}

	NewTextChild( node, "bag_weight", bag_weight );
	NewTextChild( node, "rk_weight", rk_weight );
	NewTextChild( node, "adult", adult );
	NewTextChild( node, "child", child );
	NewTextChild( node, "baby", baby );

  xmlNodePtr loadNode = NewTextChild( node, "trip_load" );
  for ( vector<Cargo>::iterator c=lug.vcargo.begin(); c!=lug.vcargo.end(); c++ ) {
  	xmlNodePtr fn = NewTextChild( loadNode, "load" );
  	NewTextChild( fn, "cargo", c->cargo );
  	NewTextChild( fn, "mail", c->mail );
  	NewTextChild( fn, "airp_arv", ElemIdToElemCtxt( ecDisp, etAirp, c->airp_arv, c->airp_arv_fmt ) );
  	NewTextChild( fn, "point_arv", c->point_arv );
  }
} */


void GetLuggage( int point_id, xmlNodePtr dataNode )
{
	xmlNodePtr node = NewTextChild( dataNode, "luggage" );
  PersWeightRules r;
  ClassesPersWeight weight;
  r.read( point_id );
	TFlightWeights w;
	w.read( point_id, withBrd );
	TPointsDest dest;
	BitSet<TUseDestData> UseData;
	UseData.setFlag( udCargo );
	UseData.setFlag( udMaxCommerce );
	dest.Load( point_id, UseData );
  TFlightCargos cargos;
  cargos.Load( point_id, dest.pr_tranzit, dest.first_point, dest.point_num, dest.pr_del ); //  dest.pr_del == 1 ???
  std::vector<TPointsDestCargo> cargs;
  cargos.Get( cargs );
	int max_commerce = dest.max_commerce.GetValue();
	if ( max_commerce == ASTRA::NoExists )
	  max_commerce = 0;
	NewTextChild( node, "max_commerce", max_commerce );
  NewTextChild( node, "pr_edit", dest.act_out != NoExists || dest.pr_del != 0 );
  if ( TReqInfo::Instance()->desk.compatible( PERS_WEIGHT_VERSION ) ) {
    tst();
    int weight_cargos = 0;
    for ( vector<TPointsDestCargo>::iterator c=cargs.begin(); c!=cargs.end(); c++ ) {
      weight_cargos += c->cargo;
      weight_cargos += c->mail;
    }
    NewTextChild( node, "adult", w.male + w.female );
    NewTextChild( node, "child", w.child );
    NewTextChild( node, "infant", w.infant );
    NewTextChild( node, "weight_adult", w.weight_male + w.weight_female );
    NewTextChild( node, "weight_child", w.weight_child );
    NewTextChild( node, "weight_infant", w.weight_infant );
    NewTextChild( node, "weight_bag", w.weight_bag );
    NewTextChild( node, "weight_cabin_bag", w.weight_cabin_bag );
    NewTextChild( node, "weight_commerce", w.weight_male +
                                           w.weight_female +
                                           w.weight_child +
                                           w.weight_infant +
                                           w.weight_bag +
                                           w.weight_cabin_bag +
                                           weight_cargos );
  }
  else {
    tst();
    r.weight( "", "", weight );
    xmlNodePtr wm = NewTextChild( node, "weightman" );
		xmlNodePtr weightNode = NewTextChild( wm, "weight" );
		NewTextChild( weightNode, "code", string(EncodePerson( ASTRA::adult )) );
		NewTextChild( weightNode, "weight", weight.male );
		weightNode = NewTextChild( wm, "weight" );
		NewTextChild( weightNode, "code", string(EncodePerson( ASTRA::child )) );
		NewTextChild( weightNode, "weight", weight.child );
		weightNode = NewTextChild( wm, "weight" );
		NewTextChild( weightNode, "code", string(EncodePerson( ASTRA::baby )) );
		NewTextChild( weightNode, "weight", weight.infant );
  	NewTextChild( node, "bag_weight", w.weight_bag );
	  NewTextChild( node, "rk_weight", w.weight_cabin_bag );
	  NewTextChild( node, "adult", w.male + w.female  );
	  NewTextChild( node, "child", w.child );
	  NewTextChild( node, "baby", w.infant );
		// ᮮ�饭�� � �ନ���� � ��ᮮ⢥��⢨� 䠪��᪮� ����㧪� � �ନ���� ॠ�쭮� � ��⥬�
		int commerce_weight = 0, newcommerce_weight = 0;
    commerce_weight += (w.male + w.female)*weight.male;
		commerce_weight += w.child*weight.child;
		commerce_weight += w.infant*weight.infant;
		commerce_weight += w.weight_cabin_bag;
		commerce_weight += w.weight_bag;
		newcommerce_weight += w.weight_male;
		newcommerce_weight += w.weight_female;
		newcommerce_weight += w.weight_child;
		newcommerce_weight += w.weight_infant;
		newcommerce_weight += w.weight_cabin_bag;
		newcommerce_weight += w.weight_bag;
		if ( commerce_weight != newcommerce_weight ) {
      ProgTrace( TRACE5, "commerce_weight=%d, newcommerce_weight=%d, w.male=%d, weight.male=%d, w.weight_male=%d, w.child=%d, weight.child=%d, w.infant=%d, weight.infant=%d,"
                 "w.weight_bag=%d, w.weight_male=%d, w.weight_female=%d, w.weight_child=%d, "
                 "w.weight_infant=%d, w.weight_cabin_bag=%d",
                 commerce_weight, newcommerce_weight, w.male, weight.male, w.weight_male, w.child, weight.child, w.infant, weight.infant,
                 w.weight_bag, w.weight_male, w.weight_female, w.weight_child,
                 w.weight_infant, w.weight_cabin_bag );
      showErrorMessage( "MSG.INVALID_CALC_PERS_WEIGHTS" );
		}
  }
  xmlNodePtr loadNode = NewTextChild( node, "trip_load" );
  for ( vector<TPointsDestCargo>::iterator c=cargs.begin(); c!=cargs.end(); c++ ) {
  	xmlNodePtr fn = NewTextChild( loadNode, "load" );
  	NewTextChild( fn, "cargo", c->cargo );
  	NewTextChild( fn, "mail", c->mail );
  	NewTextChild( fn, "airp_arv", ElemIdToElemCtxt( ecDisp, etAirp, c->airp_arv, c->airp_arv_fmt ) );
  	NewTextChild( fn, "point_arv", c->point_arv );
  }
}

void SoppInterface::ReadTripInfo(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
	int point_id = NodeAsInteger( "point_id", reqNode );
	xmlNodePtr dataNode = NewTextChild( resNode, "data" );
	ProgTrace( TRACE5, "point_id=%d", point_id );
  if ( GetNode( "tripcounters", reqNode ) )
    readPaxLoad( point_id, reqNode, dataNode );
  if ( GetNode( "birks", reqNode ) ) {
  	GetBirks( point_id, dataNode );
  }
  if ( GetNode( "luggage", reqNode ) ) {
  	GetLuggage( point_id, dataNode );
  }
  TSOPPTrips trips;

  string errcity = internal_ReadData( trips, NoExists, NoExists, false, tSOPP, point_id );

  if ( !errcity.empty() )
    AstraLocale::showErrorMessage( "MSG.CITY.REGION_NOT_DEFINED.NOT_ALL_FLIGHTS_ARE_SHOWN",
    	                             LParams() << LParam("city", ElemIdToCodeNative(etCity,errcity)));

	TQuery Qry(&OraSession );
 	Qry.SQLText = "SELECT airp FROM points WHERE point_id=:point_id";
 	Qry.CreateVariable( "point_id", otInteger, point_id );
 	Qry.Execute();
 	string airp = Qry.FieldAsString( "airp" );

  if ( GetNode( "stages", reqNode ) ) {
  	if ( !Qry.Eof ) {
  	  vector<TSoppStage> stages;
  	  read_TripStages( stages, NoExists, point_id );
      string region = AirpTZRegion( airp );
      try {
  	    build_TripStages( stages, region, dataNode, false );
  	  }
      catch( Exception &e ) {
        ProgError( STDLOG, "Exception: %s, point_id=%d", e.what(), point_id );
        throw;
      }
    }
  }
  if ( GetNode( "UpdateGraph_Stages", reqNode ) ) {
  	TStagesRules::Instance()->UpdateGraph_Stages();
  	TStagesRules::Instance()->BuildGraph_Stages( airp, dataNode );
  }
  xmlNodePtr headerNode = NewTextChild( dataNode, "header" );
  for (TSOPPTrips::iterator i=trips.begin(); i!=trips.end(); i++ ) {
  	if ( i->point_id == point_id ) {
      NewTextChild( headerNode, "remark_out", i->remark_out );
      string stralarms;
  	  for ( int ialarm=0; ialarm<atLength; ialarm++ ) {
        TTripAlarmsType alarm = (TTripAlarmsType)ialarm;
        if ( !i->Alarms.isFlag( alarm ) )
      	  continue;
        if ( !stralarms.empty() )
        	stralarms += " ";
        stralarms += "!" + TripAlarmString( alarm );
      }
      NewTextChild( headerNode, "alarms", stralarms );
      break;
    }
  }
}

void internal_ReadDests( int move_id, TSOPPDests &dests, string &reference, TDateTime part_key )
{
	TReqInfo *reqInfo = TReqInfo::Instance();
  TQuery Qry(&OraSession);
  if ( part_key != NoExists ) {
      Qry.SQLText =
        "SELECT reference FROM arx_move_ref WHERE part_key=:part_key AND move_id=:move_id";
      Qry.CreateVariable( "part_key", otDate, part_key );
  }
  else
  {
      Qry.SQLText =
        "SELECT reference FROM move_ref WHERE move_id=:move_id";
  };
  Qry.CreateVariable( "move_id", otInteger, move_id );
  Qry.Execute();
  if ( !Qry.Eof )  reference = Qry.FieldAsString( "reference" );
  dests.clear();
  Qry.Clear();
  if ( part_key > NoExists ) {
	  Qry.SQLText =
    "SELECT point_id,point_num,first_point,airp,airp_fmt,airline,airline_fmt,flt_no,suffix,suffix_fmt,craft,craft_fmt,bort,"
    "       scd_in,est_in,act_in,scd_out,est_out,act_out,trip_type,litera,park_in,park_out,remark,"
    "       pr_tranzit,pr_reg,arx_points.pr_del pr_del "
    " FROM arx_points "
    " WHERE arx_points.part_key=:part_key AND arx_points.move_id=:move_id AND "
    "       arx_points.pr_del!=-1 "
    " ORDER BY point_num ";
    Qry.CreateVariable( "part_key", otDate, part_key );
  }
  else
	  Qry.SQLText =
    "SELECT point_id,point_num,first_point,airp,airp_fmt,airline,airline_fmt,flt_no,suffix,suffix_fmt,craft,craft_fmt,bort,"\
    "       scd_in,est_in,act_in,scd_out,est_out,act_out,trip_type,litera,park_in,park_out,remark,"\
    "       pr_tranzit,pr_reg,points.pr_del pr_del "\
    " FROM points "
    "WHERE points.move_id=:move_id AND "\
    "      points.pr_del!=-1 "\
    "ORDER BY point_num ";
  Qry.CreateVariable( "move_id", otInteger, move_id );
  Qry.Execute();
  TQuery DQry(&OraSession);
  if ( part_key > NoExists ) {
    DQry.SQLText = arx_trip_delays_SQL;
    DQry.CreateVariable( "part_key", otDate, part_key );
  }
  else {
    DQry.SQLText = trip_delays_SQL;
  }
  DQry.DeclareVariable( "point_id", otInteger );
  string region;
  while ( !Qry.Eof ) {
  	TSOPPDest d;
  	d.point_id = Qry.FieldAsInteger( "point_id" );
  	d.point_num = Qry.FieldAsInteger( "point_num" );
  	if ( !Qry.FieldIsNULL( "first_point" ) )
  	  d.first_point = Qry.FieldAsInteger( "first_point" );
  	else
  		d.first_point = NoExists;
  	d.airp = Qry.FieldAsString( "airp" );
  	d.airp_fmt = (TElemFmt)Qry.FieldAsInteger( "airp_fmt" );
 	  d.airline = Qry.FieldAsString( "airline" );
 	  d.airline_fmt = (TElemFmt)Qry.FieldAsInteger( "airline_fmt" );
  	if ( !Qry.FieldIsNULL( "flt_no" ) )
  	  d.flt_no = Qry.FieldAsInteger( "flt_no" );
  	else
  		d.flt_no = NoExists;
 	  d.suffix = Qry.FieldAsString( "suffix" );
 	  d.suffix_fmt = (TElemFmt)Qry.FieldAsInteger( "suffix_fmt" );
 	  d.craft = Qry.FieldAsString( "craft" );
 	  d.craft_fmt = (TElemFmt)Qry.FieldAsInteger( "craft_fmt" );
 	  d.bort = Qry.FieldAsString( "bort" );
  	if ( reqInfo->user.sets.time == ustTimeLocalAirp )
  	  d.region = AirpTZRegion( Qry.FieldAsString( "airp" ) );
  	else
  		d.region.clear();
	  if ( !Qry.FieldIsNULL( "scd_in" ) )
	    d.scd_in = Qry.FieldAsDateTime( "scd_in" );
 	  else
 	  	d.scd_in = NoExists;
 	  if ( !Qry.FieldIsNULL( "est_in" ) )
 	    d.est_in = Qry.FieldAsDateTime( "est_in" );
 	  else
 	  	d.est_in = NoExists;
 	  if ( !Qry.FieldIsNULL( "act_in" ) )
 	    d.act_in = Qry.FieldAsDateTime( "act_in" );
 	  else
 	  	d.act_in = NoExists;
 	  if ( !Qry.FieldIsNULL( "scd_out" ) )
 	    d.scd_out = Qry.FieldAsDateTime( "scd_out" );
 	  else
 	  	d.scd_out = NoExists;
 	  if ( !Qry.FieldIsNULL( "est_out" ) )
 	    d.est_out = Qry.FieldAsDateTime( "est_out" );
 	  else
 	  	d.est_out = NoExists;
 	  if ( !Qry.FieldIsNULL( "act_out" ) )
 	    d.act_out = Qry.FieldAsDateTime( "act_out" );
 	  else
 	  	d.act_out = NoExists;
 	  DQry.SetVariable( "point_id", d.point_id );
    DQry.Execute();
  	while ( !DQry.Eof ) {
  		TSOPPDelay delay;
  		delay.code = DQry.FieldAsString( "delay_code" );
  		delay.time = DQry.FieldAsDateTime( "time" );
  		d.delays.push_back( delay );
  		DQry.Next();
    }
	  d.triptype = Qry.FieldAsString( "trip_type" );
  	d.litera = Qry.FieldAsString( "litera" );
  	d.park_in = Qry.FieldAsString( "park_in" );
  	d.park_out = Qry.FieldAsString( "park_out" );
  	d.pr_tranzit = Qry.FieldAsInteger( "pr_tranzit" );
    d.pr_reg = Qry.FieldAsInteger( "pr_reg" );
    d.pr_del = Qry.FieldAsInteger( "pr_del" );
    dests.push_back( d );
  	Qry.Next();
  }
}

void ReBindTlgs( int move_id, TSOPPDests &dests )
{
  vector<int> point_ids;
  for (TSOPPDests::const_iterator i=dests.begin(); i!=dests.end(); i++) {
     point_ids.push_back( i->point_id );
  }
  unbind_tlg(point_ids);

  vector<TTripInfo> flts;
	TSOPPDests vdests;
	string reference;
	internal_ReadDests( move_id, vdests, reference, NoExists);
  // ᮧ���� �� �������� ३�� �� ������ ������� �᪫��� 㤠����� �㭪��
  for( TSOPPDests::iterator i=vdests.begin(); i!=vdests.end(); i++ ) {
    /*ProgTrace( TRACE5, "move_id=%d, point_id=%d, airline=%s, flt_no=%d, scd_out=%f",
               move_id, i->point_id, i->airline.c_str(), i->flt_no, i->scd_out );*/
  	if ( i->pr_del == -1 ) continue;
  	if ( i->airline.empty() ||
         i->flt_no == NoExists ||
         i->scd_out == NoExists )
      continue;
    TTripInfo tripInfo;
    tripInfo.airline = i->airline;
    tripInfo.flt_no = i->flt_no;
    tripInfo.suffix = i->suffix;
    tripInfo.airp = i->airp;
    tripInfo.scd_out = i->scd_out;
    flts.push_back( tripInfo );
  }
  bind_tlg_oper(flts, true);
}

void SoppInterface::ReadDests(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  xmlNodePtr node = NewTextChild( resNode, "data" );
	int move_id = NodeAsInteger( "move_id", reqNode );
	TDateTime part_key = NoExists;
	xmlNodePtr pNode = GetNode( "part_key", reqNode );
	if ( pNode )
		part_key = NodeAsDateTime( pNode );
	NewTextChild( node, "move_id", move_id );
	TSOPPDests dests;
	string reference;
	internal_ReadDests( move_id, dests, reference, part_key);
  if ( !reference.empty() )
  	NewTextChild( node, "reference", reference );
  node = NewTextChild( node, "dests" );
  xmlNodePtr snode, dnode;
  string region;
  for ( TSOPPDests::iterator d=dests.begin(); d!=dests.end(); d++ ) {
  	snode = NewTextChild( node, "dest" );
  	NewTextChild( snode, "point_id", d->point_id );
  	NewTextChild( snode, "point_num", d->point_num );
  	if ( d->first_point > NoExists )
  	  NewTextChild( snode, "first_point", d->first_point );
  	NewTextChild( snode, "airp", ElemIdToElemCtxt( ecDisp, etAirp, d->airp, d->airp_fmt ) );
  	NewTextChild( snode, "airpId", d->airp );
  	if ( !d->airline.empty() )
  	  NewTextChild( snode, "airline", ElemIdToElemCtxt( ecDisp, etAirline, d->airline, d->airline_fmt ) );
  	if ( d->flt_no > NoExists )
  	  NewTextChild( snode, "flt_no", d->flt_no );
  	if ( !d->suffix.empty() )
  	  NewTextChild( snode, "suffix", ElemIdToElemCtxt( ecDisp, etSuffix, d->suffix, d->suffix_fmt ) );
  	if ( !d->craft.empty() )
  	  NewTextChild( snode, "craft", ElemIdToElemCtxt( ecDisp, etCraft, d->craft, d->craft_fmt ) );
  	if ( !d->bort.empty() )
  	  NewTextChild( snode, "bort", d->bort );
  	try {
  	  if ( d->scd_in > NoExists )
  	    NewTextChild( snode, "scd_in", DateTimeToStr( UTCToClient( d->scd_in, d->region ), ServerFormatDateTimeAsString ) );
  	  if ( d->est_in > NoExists )
  	    NewTextChild( snode, "est_in", DateTimeToStr( UTCToClient( d->est_in, d->region ), ServerFormatDateTimeAsString ) );
  	  if ( d->act_in > NoExists )
  	    NewTextChild( snode, "act_in", DateTimeToStr( UTCToClient( d->act_in, d->region ), ServerFormatDateTimeAsString ) );
  	  if ( d->scd_out > NoExists )
  	    NewTextChild( snode, "scd_out", DateTimeToStr( UTCToClient( d->scd_out, d->region ), ServerFormatDateTimeAsString ) );
  	  if ( d->est_out > NoExists )
  	    NewTextChild( snode, "est_out", DateTimeToStr( UTCToClient( d->est_out, d->region ), ServerFormatDateTimeAsString ) );
  	  if ( d->act_out > NoExists )
  	    NewTextChild( snode, "act_out", DateTimeToStr( UTCToClient( d->act_out, d->region ), ServerFormatDateTimeAsString ) );
  	}
  	catch( Exception &e ) {
  		ProgError( STDLOG, "Exception %s, move_id=%d", e.what(), move_id );
  		throw AstraLocale::UserException( "MSG.REGION_NOT_FOUND_IN_ROUTE", LParams() << LParam("airp", ElemIdToCodeNative(etAirp,d->airp)));
  	}
  	dnode = NULL;
  	for ( vector<TSOPPDelay>::iterator delay=d->delays.begin(); delay!=d->delays.end(); delay++ ) {
  		if ( !dnode )
  			dnode = NewTextChild( snode, "delays" );
  		xmlNodePtr fnode = NewTextChild( dnode, "delay" );
  		NewTextChild( fnode, "delay_code", ElemIdToCodeNative(etDelayType,delay->code) );
  		NewTextChild( fnode, "time", DateTimeToStr( UTCToClient( delay->time, d->region ), ServerFormatDateTimeAsString ) );
    }
  	if ( !d->triptype.empty() )
  	  NewTextChild( snode, "trip_type", ElemIdToCodeNative(etTripType,d->triptype) );
  	if ( !d->litera.empty() )
  	  NewTextChild( snode, "litera", ElemIdToCodeNative(etTripLiter,d->litera) );
  	if ( !d->park_in.empty() )
  	  NewTextChild( snode, "park_in", d->park_in );
  	if ( !d->park_out.empty() )
  	  NewTextChild( snode, "park_out", d->park_out );
  	NewTextChild( snode, "pr_tranzit", d->pr_tranzit );
    NewTextChild( snode, "pr_reg", d->pr_reg );
    if ( d->pr_del )
    	NewTextChild( snode, "pr_del", d->pr_del );
  }
}

void internal_WriteDests( int &move_id, TSOPPDests &dests, const string &reference, bool canExcept,
                          XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode )
{
  TPersWeights persWeights;
  vector<change_act> vchangeAct;
  TSOPPDests voldDests;
	bool ch_point_num = false;
  for( TSOPPDests::iterator id=dests.begin(); id!=dests.end(); id++ )
  	if ( id->point_num == NoExists || id->pr_del == -1 ) { // ��⠢�� ��� 㤠����� �㭪� ��ᠤ��
  		ch_point_num = true;
  		break;
  	}
  bool ch_craft = false;
  TQuery Qry(&OraSession);
  TQuery DelQry(&OraSession);
  DelQry.SQLText =
   " UPDATE points SET point_num=point_num-1 WHERE point_num<=:point_num AND move_id=:move_id AND pr_del=-1 ";
  DelQry.DeclareVariable( "move_id", otInteger );
  DelQry.DeclareVariable( "point_num", otInteger );

  TReqInfo *reqInfo = TReqInfo::Instance();
  bool existsTrip = false;
  bool pr_last;
  bool pr_other_airline = false;
  int notCancel = (int)dests.size();
  if ( notCancel < 2 )
  	throw AstraLocale::UserException( "MSG.CHECK_FLIGHT.ROUTE_LEAST_TWO_POINTS" );
  // �஢�ન
  // �᫨ �� ࠡ�⭨� ��ய���, � � ������� ������ ���� ��� ����,
  // �᫨ ࠡ�⭨� ������������, � ������������
  if ( reqInfo->user.user_type != utSupport ) {
    bool canDo = reqInfo->user.user_type == utAirline;
    for( TSOPPDests::iterator id=dests.begin(); id!=dests.end(); id++ ) {
    	if ( id->pr_del == -1 )
    		continue;
      if ( reqInfo->CheckAirp( id->airp ) ) {
        canDo = true;
      }
      pr_last = true;
      for ( TSOPPDests::iterator ir=id + 1; ir!=dests.end(); ir++ ) {
      	if ( ir->pr_del != -1 ) {
      		pr_last = false;
      		break;
      	}
      }
      if ( !pr_last &&
      	   !reqInfo->CheckAirline( id->airline ) ) {
        if ( !id->airline.empty() )
          throw AstraLocale::UserException( "MSG.AIRLINE.ACCESS_DENIED",
          	                                LParams() << LParam("airline", ElemIdToElemCtxt(ecDisp,etAirline,id->airline,id->airline_fmt)) );
        else
        	throw AstraLocale::UserException( "MSG.AIRLINE.NOT_SET" );
      }
    } // end for
    if ( !canDo )
    	if ( reqInfo->user.access.airps_permit ) {
    	  if ( reqInfo->user.access.airps.size() == 1 )
    	    throw AstraLocale::UserException( "MSG.ROUTE.MUST_CONTAIN_AIRP",
    	    	                                LParams() << LParam("airp", ElemIdToCodeNative(etAirp,*reqInfo->user.access.airps.begin())));
    	  else {
    		  string airps;
    		  for ( vector<string>::iterator s=reqInfo->user.access.airps.begin(); s!=reqInfo->user.access.airps.end(); s++ ) {
    		    if ( !airps.empty() )
    		      airps += " ";
    		    airps += ElemIdToCodeNative(etAirp,*s);
    		  }
    		  if ( airps.empty() )
    		  	throw AstraLocale::UserException( "MSG.AIRP.ALL_ACCESS_DENIED" );
    		  else
    		    throw AstraLocale::UserException( "MSG.ROUTE.MUST_CONTAIN_ONE_OF_AIRPS", LParams() << LParam("list", airps));
    	  }
    	}
    	else { // ᯨ᮪ ����饭��� ��ய��⮢
    		string airps;
    		for ( vector<string>::iterator s=reqInfo->user.access.airps.begin(); s!=reqInfo->user.access.airps.end(); s++ ) {
    		  if ( !airps.empty() )
    		    airps += " ";
    		  airps += ElemIdToCodeNative(etAirp,*s);
    		}
            throw AstraLocale::UserException( "MSG.ROUTE.MUST_CONTAIN_ONE_OF_AIRPS_OTHER_THAN", LParams() << LParam("list", airps));
    	}
  }
  try {
    // �஢�ઠ �� �⬥�� + � ������� �砢���� �ᥣ� ���� ������������
    string old_airline;
    for( TSOPPDests::iterator id=dests.begin(); id!=dests.end(); id++ ) {
      if ( id->pr_del == 1 ) {
        notCancel--;
      }
      if ( id->pr_del == 0 ) {
      	if ( old_airline.empty() )
      		old_airline = id->airline;
      	if ( !id->airline.empty() && old_airline != id->airline )
      		pr_other_airline = true;
      }
    }

    // �⬥�塞 �� �.�., �.�. � ������� �ᥣ� ���� �� �⬥�����
    if ( notCancel == 1 ) {
      for( TSOPPDests::iterator id=dests.begin(); id!=dests.end(); id++ ) {
       	if ( !id->pr_del ) {
      		id->pr_del = 1;
      		id->modify = true;
        }
      }
    }
    // �஢�ઠ 㯮�冷祭���� �६�� + �㡫�஢���� ३�, �᫨ move_id == NoExists
    TDateTime oldtime, curtime = NoExists;
    bool pr_time=false;
    TDoubleTrip doubletrip;
    for( TSOPPDests::iterator id=dests.begin(); id!=dests.end(); id++ ) {
  	  if ( id->scd_in > NoExists || id->scd_out > NoExists )
  	  	pr_time = true;
    	if ( id->pr_del )
  	    continue;
  	  if ( id->scd_in > NoExists && id->act_in == NoExists ) {
  	  	oldtime = curtime;
  	  	curtime = id->scd_in;
  	  	if ( oldtime > NoExists && oldtime > curtime ) {
  	  		throw AstraLocale::UserException( "MSG.ROUTE.IN_OUT_TIMES_NOT_ORDERED" );
  	  	}
      }
      if ( id->scd_out > NoExists && id->act_out == NoExists ) {
      	oldtime = curtime;
  	  	curtime = id->scd_out;
  	  	if ( oldtime > NoExists && oldtime > curtime ) {
  	  		throw AstraLocale::UserException( "MSG.ROUTE.IN_OUT_TIMES_NOT_ORDERED" );
  	  	}
      }
      if ( id->craft.empty() ) {
        for ( TSOPPDests::iterator xd=id+1; xd!=dests.end(); xd++ ) {
        	if ( xd->pr_del )
        		continue;
          throw AstraLocale::UserException( "MSG.CRAFT.NOT_SET" );
        }
      }
      int point_id;
      if ( !existsTrip &&
      	   id != dests.end() - 1 &&
      	   doubletrip.IsExists( move_id, id->airline, id->flt_no, id->suffix, id->airp, id->scd_in, id->scd_out, point_id ) ) { //??? ��祬� ���� �ࠢ����� �������� �६�� � ����??? � ������� �ࠢ����� � UTC!!!
      	existsTrip = true;
      	break;
      }
      if ( id->pr_del != -1 && id != dests.end() &&
           ( !id->delays.empty() || id->est_out != NoExists && id->est_out != id->scd_out ) ) {
        TTripInfo info;
        info.airline = id->airline;
        info.flt_no = id->flt_no;
        info.airp = id->airp;
        if ( GetTripSets( tsCheckMVTDelays, info ) ) { //�஢�ઠ ����থ� �� ᮢ���⨬���� � ⥫��ࠬ����
          if ( id->delays.empty() )
            throw AstraLocale::UserException( "MSG.MVTDELAY.INVALID_CODE" );
          vector<TSOPPDelay>::iterator q = id->delays.end() - 1;
          if ( q->time != id->est_out )
            throw AstraLocale::UserException( "MSG.MVTDELAY.INVALID_CODE" );
          for ( q=id->delays.begin(); q!=id->delays.end(); q++ ) {
            if ( !check_delay_code( q->code ) )
              throw AstraLocale::UserException( "MSG.MVTDELAY.INVALID_CODE" );
            ProgTrace( TRACE5, "%f", q->time - id->scd_out );
            if ( q->time != id->scd_out && !check_delay_value( q->time - id->scd_out ) )
              throw AstraLocale::UserException( "MSG.MVTDELAY.INVALID_TIME" );
          }
        }
      }
    } // end for
    if ( !pr_time )
    	throw AstraLocale::UserException( "MSG.ROUTE.IN_OUT_TIMES_NOT_SPECIFIED" );
  }
  catch( AstraLocale::UserException &e ) {
  	if ( canExcept ) {
  		NewTextChild( NewTextChild( resNode, "data" ), "notvalid" );
  		AstraLocale::showErrorMessage( "MSG.ERR_MSG.REPEAT_F9_SAVE", LParams() << LParam("msg", getLocaleText(e.getLexemaData())));
  		return;
    }
  }

  if ( pr_other_airline )
    throw AstraLocale::UserException( "MSG.CHECK_FLIGHT.ROUTE_CANNOT_BELONG_TO_DIFFERENT_AIRLINES" );

  if ( existsTrip )
    throw AstraLocale::UserException( "MSG.FLIGHT.DUPLICATE.ALREADY_EXISTS" );
  // ������� ��ࠬ��஢ pr_tranzit, pr_reg, first_point
  TSOPPDests::iterator pid=dests.end();
  for( TSOPPDests::iterator id=dests.begin(); id!=dests.end(); id++ ) {
  	if ( id->pr_del == -1 ) continue;
  	if( pid == dests.end() || id + 1 == dests.end() )
  		id->pr_tranzit = 0;
  	else {
      ProgTrace( TRACE5, "id->point_id=%d, ch_point_num=%d", id->point_id, ch_point_num );
      if ( id->point_id == NoExists || ch_point_num ) //???
      id->pr_tranzit=( pid->airline + IntToString( pid->flt_no ) + pid->suffix /*+ p->triptype ???*/ ==
                       id->airline + IntToString( id->flt_no ) + id->suffix /*+ id->triptype*/ );
        id->modify = true;
    }

    id->pr_reg = ( id->scd_out > NoExists &&
                   ((TTripTypesRow&)base_tables.get("trip_types").get_row( "code", id->triptype, true )).pr_reg!=0 &&
                   /*!id->pr_del &&*/ id != dests.end() - 1 );
/*    if ( id->pr_reg ) {
      TSOPPDests::iterator r=id;
      r++;
      for ( ;r!=dests.end(); r++ ) {
        if ( !r->pr_del )
          break;
      }
      if ( r == dests.end() ) {
        id->pr_reg = 0;
      }
    }*/
  	pid = id;
  }
//  } //end move_id==NoExists

  Qry.Clear();
  bool insert = ( move_id == NoExists );
  if ( insert ) {
  /* ����室��� ᤥ���� �஢��� �� �� ����⢮����� ३�*/
    Qry.SQLText =
     "BEGIN "\
     " SELECT move_id.nextval INTO :move_id from dual; "\
     " INSERT INTO move_ref(move_id,reference)  SELECT :move_id, :reference FROM dual; "\
     "END;";
    Qry.DeclareVariable( "move_id", otInteger );
    Qry.CreateVariable( "reference", otString, reference );
    Qry.Execute();
    move_id = Qry.GetVariableAsInteger( "move_id" );
  }
  else {
    Qry.SQLText =
     "BEGIN "\
     " UPDATE points SET move_id=move_id WHERE move_id=:move_id; "\
     " UPDATE move_ref SET reference=:reference WHERE move_id=:move_id; "\
     "END;";
    Qry.CreateVariable( "move_id", otInteger, move_id );
    Qry.CreateVariable( "reference", otString, reference );
    Qry.Execute();
  }
  bool ch_dests = false;
  int new_tid;
  bool init_trip_stages;
  bool set_act_out;
  bool set_pr_del;
  int point_num = 0;
  int first_point;
  bool insert_point;
  bool pr_begin = true;
  bool change_stages_out;
  bool pr_change_tripinfo;
  vector<int> setcraft_points;
  bool reSetCraft;
  bool reSetWeights;
  string change_dests_msg;
  TBaseTable &baseairps = base_tables.get( "airps" );
  vector<int> points_MVTdelays;
  for( TSOPPDests::iterator id=dests.begin(); id!=dests.end(); id++ ) {
  	set_pr_del = false;
  	set_act_out = false;
  	if ( ch_point_num )
  	  id->point_num = point_num;
  	if ( id->modify ) {
  	  Qry.Clear();
  	  Qry.SQLText =
  	   "SELECT cycle_tid__seq.nextval n FROM dual ";
    	Qry.Execute();
    	new_tid = Qry.FieldAsInteger( "n" );

      insert_point = id->point_id == NoExists;
    	if ( insert_point ) { //insert
    		if ( !insert )
    			ch_dests = true;
    		Qry.Clear();
    		Qry.SQLText =
    		 "SELECT point_id.nextval point_id FROM dual";
    		Qry.Execute();
    		id->point_id = Qry.FieldAsInteger( "point_id" );
    	}
    }
    if ( id->pr_del != -1 ) {
 		  if ( pr_begin ) {
 		  	  pr_begin = false;
      	  first_point = id->point_id;
      	  if ( id->first_point != NoExists ) {
      	    id->first_point = NoExists;
      	    id->modify = true;
      	  }
      }
      else {
        if ( !id->pr_tranzit ) {
        	if ( id->first_point != first_point ) {
            id->first_point = first_point;
            id->modify = true;
          }
          first_point = id->point_id;
        }
        else
        	if ( id->first_point != first_point ) {
            id->first_point = first_point;
            id->modify = true;
          }
        ProgTrace( TRACE5, "id->point_id=%d, id->first_point=%d, id->pr_tranzit=%d, id->modify=%d",
                   id->point_id, id->first_point, id->pr_tranzit, id->modify );
      }
    }

    ProgTrace( TRACE5, "point_id=%d, id->modify=%d", id->point_id, id->modify );
    if ( !id->modify ) { //??? remark
  	  voldDests.push_back( *id );
    	point_num++;
    	continue;
    }
  	change_stages_out = false;
  	reSetCraft = false;
  	reSetWeights = false;
  	pr_change_tripinfo = false;
    TSOPPDest old_dest;
    if ( id->pr_del != -1 ) {
  	  if ( change_dests_msg.empty() )
        if ( insert )
          change_dests_msg = "���� ������ ३�: ";
        else
          change_dests_msg = "��������� ������� ३�: ";
      else
        change_dests_msg += "-";
        if ( id->flt_no != NoExists )
          change_dests_msg += id->airline + IntToString(id->flt_no) + id->suffix + " " + id->airp;
        else
          change_dests_msg += id->airp;
    }

    if ( insert_point ) {
    	ch_craft = false;
      Qry.Clear();
      Qry.SQLText =
       "INSERT INTO points(move_id,point_id,point_num,airp,airp_fmt,pr_tranzit,first_point,"
       "                   airline,airline_fmt,flt_no,suffix,suffix_fmt,craft,craft_fmt,"
       "                   bort,scd_in,est_in,act_in,scd_out,est_out,act_out,trip_type,litera,"
       "                   park_in,park_out,pr_del,tid,remark,pr_reg) "
       " VALUES(:move_id,:point_id,:point_num,:airp,:airp_fmt,:pr_tranzit,:first_point,"
       "        :airline,:airline_fmt,:flt_no,:suffix,:suffix_fmt,:craft,:craft_fmt,"
       "        :bort,:scd_in,:est_in,:act_in,:scd_out,:est_out,:act_out,:trip_type,:litera,"
       "        :park_in,:park_out,:pr_del,:tid,:remark,:pr_reg)";
       init_trip_stages = id->pr_reg;
       if ( id->flt_no != NoExists )
         reqInfo->MsgToLog( string( "���� ������ �㭪� " ) + id->airline + IntToString(id->flt_no) + id->suffix + " " + id->airp, evtDisp, move_id, id->point_id );
       else
         reqInfo->MsgToLog( string( "���� ������ �㭪� " ) + id->airp, evtDisp, move_id, id->point_id );
       reSetCraft = true;
       reSetWeights = true;
  	}
  	else { //update
  	 Qry.Clear();
  	 Qry.SQLText =
  	  "SELECT point_num,airp,pr_tranzit,first_point,airline,flt_no,suffix, "
  	  "       craft,bort,scd_in,est_in,act_in,scd_out,est_out,act_out,trip_type,litera,"
  	  "       pr_del,pr_reg,remark "\
  	  " FROM points WHERE point_id=:point_id ";
  	  Qry.CreateVariable( "point_id", otInteger, id->point_id );
  	  Qry.Execute();
  	  if ( Qry.FieldIsNULL( "point_num" ) )
  	  	old_dest.point_num = NoExists;
  	  else
  	  	old_dest.point_num = Qry.FieldAsInteger( "point_num" );
  	  old_dest.airp = Qry.FieldAsString( "airp" );
  	  old_dest.pr_tranzit = Qry.FieldAsInteger( "pr_tranzit" );
  	  if ( Qry.FieldIsNULL( "first_point" ) )
  	  	old_dest.first_point = NoExists;
  	  else
  	  	old_dest.first_point = Qry.FieldAsInteger( "first_point" );
  	  if ( Qry.FieldIsNULL( "airline" ) )
  	  	old_dest.airline.clear();
  	  else
  	  	old_dest.airline = Qry.FieldAsString( "airline" );
  	  if ( Qry.FieldIsNULL( "flt_no" ) )
  	  	old_dest.flt_no = NoExists;
  	  else
  	  	old_dest.flt_no = Qry.FieldAsInteger( "flt_no" );
  	  if ( Qry.FieldIsNULL( "suffix" ) )
  	  	old_dest.suffix.clear();
  	  else
  	  	old_dest.suffix = Qry.FieldAsString( "suffix" );
  	  old_dest.craft = Qry.FieldAsString( "craft" );
  	  old_dest.bort = Qry.FieldAsString( "bort" );
  	  if ( Qry.FieldIsNULL( "scd_in" ) )
  	  	old_dest.scd_in = NoExists;
  	  else
  	  	old_dest.scd_in = Qry.FieldAsDateTime( "scd_in" );
  	  if ( Qry.FieldIsNULL( "est_in" ) )
  	  	old_dest.est_in = NoExists;
  	  else
  	  	old_dest.est_in = Qry.FieldAsDateTime( "est_in" );
  	  if ( Qry.FieldIsNULL( "act_in" ) )
  	  	old_dest.act_in = NoExists;
  	  else
  	  	old_dest.act_in = Qry.FieldAsDateTime( "act_in" );
  	  if ( Qry.FieldIsNULL( "scd_out" ) )
  	  	old_dest.scd_out = NoExists;
  	  else
  	  	old_dest.scd_out = Qry.FieldAsDateTime( "scd_out" );
  	  if ( Qry.FieldIsNULL( "est_out" ) )
  	  	old_dest.est_out = NoExists;
  	  else
  	  	old_dest.est_out = Qry.FieldAsDateTime( "est_out" );
  	  if ( Qry.FieldIsNULL( "act_out" ) )
  	  	old_dest.act_out = NoExists;
  	  else
  	  	old_dest.act_out = Qry.FieldAsDateTime( "act_out" );
  	  old_dest.triptype = Qry.FieldAsString( "trip_type" );
  	  old_dest.litera = Qry.FieldAsString( "litera" );
  	  old_dest.pr_del = Qry.FieldAsInteger( "pr_del" );
  	  old_dest.pr_reg = Qry.FieldAsInteger( "pr_reg" );
  	  old_dest.remark = Qry.FieldAsString( "remark" );
  	  old_dest.point_id = id->point_id;
  	  old_dest.region = CityTZRegion( ((TAirpsRow&)baseairps.get_row( "code", old_dest.airp, true )).city );
  	  voldDests.push_back( old_dest );

  	  change_stages_out = ( !insert_point && (id->est_out != old_dest.est_out || id->scd_out != old_dest.scd_out && old_dest.scd_out > NoExists) );

  	  if ( !old_dest.pr_reg && id->pr_reg && id->pr_del != -1 ) {
  	    Qry.Clear();
  	    Qry.SQLText = "SELECT COUNT(*) c FROM trip_stages WHERE point_id=:point_id AND rownum<2";
  	    Qry.CreateVariable( "point_id", otInteger, id->point_id );
  	    Qry.Execute();
  	    init_trip_stages = !Qry.FieldAsInteger( "c" );
  	    ProgTrace( TRACE5, "init_trip_stages=%d", init_trip_stages );
  	  }
  	  else
  	  	init_trip_stages = false;

  	  #ifdef NOT_CHANGE_AIRLINE_FLT_NO_SCD
  	  if ( id->pr_del!=-1 && !id->airline.empty() && !old_dest.airline.empty() && id->airline != old_dest.airline ) {
  	  	throw AstraLocale::UserException( "MSG.ROUTE.CANNOT_CHANGE_AIRLINE" );
  	  	//reqInfo->MsgToLog( string( "��������� ������������ � " ) + old_dest.airline + " �� " + id->airline + " ���� " + id->airp, evtDisp, move_id, id->point_id );
  	  }
  	  if ( id->pr_del!=-1 && id->flt_no > NoExists && old_dest.flt_no > NoExists && id->flt_no != old_dest.flt_no ) {
  	  	throw AstraLocale::UserException( "MSG.ROUTE.CANNOT_CHANGE_FLT_NO" );
  	  }
  	  if ( id->pr_del!=-1 && id->scd_out > NoExists && old_dest.scd_out > NoExists && id->scd_out != old_dest.scd_out ) {
  	  	throw AstraLocale::UserException( "MSG.ROUTE.CANNOT_CHANGE_SCD_OUT" );
  	  }
  	  #endif

  	  if ( id->pr_del != -1 && !id->airline.empty() && id->flt_no != NoExists &&
           id->airline+IntToString(id->flt_no)+id->suffix != old_dest.airline+IntToString(old_dest.flt_no)+old_dest.suffix ) {
  	  	reSetCraft = true;
  	  	reSetWeights = true;
  	  }
  	  if ( id->pr_del != -1 && !id->craft.empty() && id->craft != old_dest.craft && !old_dest.craft.empty() ) {
  	  	ch_craft = true;
  	  	if ( !old_dest.craft.empty() ) {
  	  		if ( !id->remark.empty() )
  	  			id->remark += " ";
  	  	  id->remark += "���. ⨯� �� � " + old_dest.craft; //!!!locale
  	  	  if ( !id->craft.empty() )
  	  	    reqInfo->MsgToLog( string( "��������� ⨯� �� �� " ) + id->craft + " ���� " + id->airp, evtDisp, move_id, id->point_id );
  	  	}
  	  	else {
  	  		reqInfo->MsgToLog( string( "�����祭�� �� " ) + id->craft + " ���� " + id->airp, evtDisp, move_id, id->point_id );
  	  	}
  	  	reSetCraft = true;
  	  	reSetWeights = true;
  	  }
  	  if ( id->pr_del != -1 && id->bort != old_dest.bort ) {
  	  	if ( !old_dest.bort.empty() ) {
  	  		if ( !id->remark.empty() )
  	  			id->remark += " ";
  	  	  id->remark += "���. ���� � " + old_dest.bort; //!!!locale
  	  	  if ( !id->bort.empty() )
  	  	    reqInfo->MsgToLog( string( "��������� ���� �� " ) + id->bort + " ���� " + id->airp, evtDisp, move_id, id->point_id );
  	  	}
  	  	else {
  	  		reqInfo->MsgToLog( string( "�����祭�� ���� " ) + id->bort + " ���� " + id->airp, evtDisp, move_id, id->point_id );
  	  	}
  	  	reSetCraft = true;
  	  	reSetWeights = true;
      }
      if ( id->pr_reg != old_dest.pr_reg || id->pr_del != old_dest.pr_del ||
           id->pr_tranzit != old_dest.pr_tranzit ) {
        reSetCraft = true;
        reSetWeights = true;
      }
  	  if ( id->pr_del != old_dest.pr_del ) {
  	  	if ( id->pr_del == 1 )
  	  		reqInfo->MsgToLog( string( "�⬥�� �㭪� " ) + id->airp, evtDisp, move_id, id->point_id );
  	  	else
  	  		if ( id->pr_del == 0 )
	  	  		reqInfo->MsgToLog( string( "������ �㭪� " ) + id->airp, evtDisp, move_id, id->point_id );
	  	  	else
	  	  		if ( id->pr_del == -1 ) {
   	      	  id->point_num = 0-id->point_num-1;
    	      	ProgTrace( TRACE5, "point_num=%d", id->point_num );
          		DelQry.SetVariable( "move_id", move_id );
    	      	DelQry.SetVariable( "point_num", id->point_num );
    		      DelQry.Execute();
              if ( id->flt_no != NoExists )
                reqInfo->MsgToLog( string( "�������� �㭪� " ) + id->airline + IntToString(id->flt_no) + id->suffix + " " + id->airp, evtDisp, move_id, id->point_id );
              else
                reqInfo->MsgToLog( string( "�������� �㭪� " ) + id->airp, evtDisp, move_id, id->point_id );
	  	  		}
  	  }
  	  else
  	    if ( !id->pr_del && id->act_out != old_dest.act_out && old_dest.act_out > NoExists ) {
  	    	reqInfo->MsgToLog( string( "��������� �६��� 䠪��᪮�� �뫥� " ) + DateTimeToStr( id->act_out, "hh:nn dd.mm.yy (UTC)" ) + " ���� " + id->airp, evtDisp, move_id, id->point_id );
   		    change_act A;
  		    A.point_id = id->point_id;
  		    A.old_act = old_dest.act_out;
  		    A.act = id->act_out;
  		    A.pr_land = false;
  		    vchangeAct.push_back( A );
  	    }
     if ( !id->airline.empty() && !old_dest.airline.empty() && id->airline != old_dest.airline ||
          !id->airline.empty() && !old_dest.airline.empty() && id->flt_no != old_dest.flt_no ||
          !id->airline.empty() && !old_dest.airline.empty() && id->suffix != old_dest.suffix ) {
       reqInfo->MsgToLog( string( "��������� ��ਡ�⮢ ३� � " ) + old_dest.airline + IntToString(old_dest.flt_no) + old_dest.suffix +
                          " �� " + id->airline + IntToString(id->flt_no) + id->suffix + " ���� " + id->airp, evtDisp, move_id, id->point_id );
     }
 	   if ( !insert_point && id->pr_del!=-1 && id->scd_out > NoExists && old_dest.scd_out > NoExists && id->scd_out != old_dest.scd_out ) {
  	  	reqInfo->MsgToLog( string( "��������� ��������� �६��� �뫥� �� ") + DateTimeToStr( id->scd_out, "hh:nn dd.mm.yy (UTC)" ) + " ���� " + id->airp, evtDisp, move_id, id->point_id );
  	  	reSetWeights = true;
  	 }
 	   if ( !insert_point && id->pr_del!=-1 && id->scd_out == NoExists && old_dest.scd_out > NoExists ) {
  	  	reqInfo->MsgToLog( string( "�������� ��������� �६��� �뫥� ") + DateTimeToStr( old_dest.scd_out, "hh:nn dd.mm.yy (UTC)" ) + " ���� " + id->airp, evtDisp, move_id, id->point_id );
  	  	reSetWeights = true;
  	 }
 	   if ( !insert_point && id->pr_del!=-1 && id->scd_out > NoExists && old_dest.scd_out == NoExists ) {
  	  	reqInfo->MsgToLog( string( "����� ��������� �६��� �뫥� ") + DateTimeToStr( id->scd_out, "hh:nn dd.mm.yy (UTC)" ) + " ���� " + id->airp, evtDisp, move_id, id->point_id );
  	  	reSetWeights = true;
  	 }

    	set_pr_del = ( !old_dest.pr_del && id->pr_del );
    	//ProgTrace( TRACE5, "set_pr_del=%d", set_pr_del );
  	  set_act_out = ( !id->pr_del && old_dest.act_out == NoExists && id->act_out > NoExists );
    	if ( !id->pr_del && old_dest.act_in == NoExists && id->act_in > NoExists ) {
    		reqInfo->MsgToLog( string( "���⠢����� 䠪�. �ਫ�� " ) + DateTimeToStr( id->act_in, "hh:nn dd.mm.yy (UTC)" ) + " ���� " + id->airp, evtDisp, move_id, id->point_id );
    		change_act A;
    		A.point_id = id->point_id;
    		A.old_act = old_dest.act_in;
    		A.act = id->act_in;
    		A.pr_land = true;
    		vchangeAct.push_back( A );
    	}
    	if ( !id->pr_del && id->act_in != old_dest.act_in && old_dest.act_in > NoExists ) {
    		reqInfo->MsgToLog( string( "��������� �६��� 䠪��᪮�� �ਫ�� " ) + DateTimeToStr( id->act_in, "hh:nn dd.mm.yy (UTC)" ) + " ���� " + id->airp, evtDisp, move_id, id->point_id );
    		change_act A;
    		A.point_id = id->point_id;
    		A.old_act = old_dest.act_in;
    		A.act = id->act_in;
    		A.pr_land = true;
    		vchangeAct.push_back( A );
    	}
    	if ( id->pr_del != -1 && old_dest.pr_del != -1 && id->pr_tranzit != old_dest.pr_tranzit ) {
        if ( id->pr_tranzit )
          reqInfo->MsgToLog( string( "���⠢����� �ਧ���� �࠭��� " ) + " ���� " + id->airp, evtDisp, move_id, id->point_id );
        else
          reqInfo->MsgToLog( string( "�⬥�� �ਧ���� �࠭��� " ) + " ���� " + id->airp, evtDisp, move_id, id->point_id );
    	}
  	  Qry.Clear();
      Qry.SQLText =
       "UPDATE points "
       " SET point_num=:point_num,airp=:airp,airp_fmt=:airp_fmt,pr_tranzit=:pr_tranzit,"\
       "     first_point=:first_point,airline=:airline,airline_fmt=:airline_fmt,flt_no=:flt_no,"
       "     suffix=:suffix,suffix_fmt=:suffix_fmt,craft=:craft,craft_fmt=:craft_fmt,"\
       "     bort=:bort,scd_in=:scd_in,est_in=:est_in,act_in=:act_in,"\
       "     scd_out=:scd_out,est_out=:est_out,act_out=:act_out,trip_type=:trip_type,"\
       "     litera=:litera,park_in=:park_in,park_out=:park_out,pr_del=:pr_del,tid=:tid,"\
       "     remark=:remark,pr_reg=:pr_reg "
       " WHERE point_id=:point_id AND move_id=:move_id ";
  	} // end update else

  	ProgTrace( TRACE5, "ch_point_num=%d,move_id=%d,point_id=%d,point_num=%d,first_point=%d,flt_no=%d",
  	           ch_point_num, move_id,id->point_id,id->point_num,id->first_point,id->flt_no );
  	ProgTrace( TRACE5, "airp=%s,airp_fmt=%d,airline=%s,airline_fmt=%d,craft=%s,craft_fmt=%d,suffix=%s,suffix_fmt=%d",
               id->airp.c_str(), id->airp_fmt, id->airline.c_str(), id->airline_fmt, id->craft.c_str(), id->craft_fmt, id->suffix.c_str(), id->suffix_fmt );
  	Qry.CreateVariable( "move_id", otInteger, move_id );
  	Qry.CreateVariable( "point_id", otInteger, id->point_id );
  	if ( ch_point_num )
  	  Qry.CreateVariable( "point_num", otInteger, 999 - id->point_num );
  	else
  		Qry.CreateVariable( "point_num", otInteger, id->point_num );
  	Qry.CreateVariable( "airp", otString, id->airp );
  	Qry.CreateVariable( "airp_fmt", otInteger, (int)id->airp_fmt );
  	Qry.CreateVariable( "pr_tranzit", otInteger, id->pr_tranzit );
  	if ( id->first_point == NoExists )
  		Qry.CreateVariable( "first_point", otInteger, FNull );
  	else
  	  Qry.CreateVariable( "first_point", otInteger, id->first_point );
  	if ( id->airline.empty() ) {
  		Qry.CreateVariable( "airline", otString, FNull );
  		Qry.CreateVariable( "airline_fmt", otInteger, FNull );
    }
  	else {
  	  Qry.CreateVariable( "airline", otString, id->airline );
  	  Qry.CreateVariable( "airline_fmt", otInteger, (int)id->airline_fmt );
  	}
  	if ( id->flt_no == NoExists )
  		Qry.CreateVariable( "flt_no", otInteger, FNull );
    else
  		Qry.CreateVariable( "flt_no", otInteger, id->flt_no );
  	if ( id->suffix.empty() ) {
  		Qry.CreateVariable( "suffix", otString, FNull );
  		Qry.CreateVariable( "suffix_fmt", otInteger, FNull );
    }
  	else {
  		Qry.CreateVariable( "suffix", otString, id->suffix );
  		Qry.CreateVariable( "suffix_fmt", otInteger, (int)id->suffix_fmt );
  	}
  	if ( id->craft.empty() ) {
  		Qry.CreateVariable( "craft", otString, FNull );
  		Qry.CreateVariable( "craft_fmt", otInteger, FNull );
  	}
  	else {
  		Qry.CreateVariable( "craft", otString, id->craft );
  		Qry.CreateVariable( "craft_fmt", otInteger, (int)id->craft_fmt );
  	}
  	if ( id->bort.empty() )
  		Qry.CreateVariable( "bort", otString, FNull );
  	else
  		Qry.CreateVariable( "bort", otString, id->bort );
  	if ( id->scd_in == NoExists )
  		Qry.CreateVariable( "scd_in", otDate, FNull );
  	else
  		Qry.CreateVariable( "scd_in", otDate, id->scd_in );
  	if ( id->est_in == NoExists )
  		Qry.CreateVariable( "est_in", otDate, FNull );
  	else
  		Qry.CreateVariable( "est_in", otDate, id->est_in );
  	if ( id->act_in == NoExists )
  		Qry.CreateVariable( "act_in", otDate, FNull );
  	else
  		Qry.CreateVariable( "act_in", otDate, id->act_in );
  	if ( id->scd_out == NoExists )
  		Qry.CreateVariable( "scd_out", otDate, FNull );
  	else
  		Qry.CreateVariable( "scd_out", otDate, id->scd_out );
  	if ( id->est_out == NoExists )
  		Qry.CreateVariable( "est_out", otDate, FNull );
  	else
  		Qry.CreateVariable( "est_out", otDate, id->est_out );
   	//ProgTrace( TRACE5, "point_id=%d, est_out=%f", id->point_id, id->est_out );
  	if ( id->act_out == NoExists )
  		Qry.CreateVariable( "act_out", otDate, FNull );
  	else
  		Qry.CreateVariable( "act_out", otDate, id->act_out );
  	if ( id->triptype.empty() )
  		Qry.CreateVariable( "trip_type", otString, FNull );
  	else
  		Qry.CreateVariable( "trip_type", otString, id->triptype );
  	if ( id->litera.empty() )
  		Qry.CreateVariable( "litera", otString, FNull );
  	else
  		Qry.CreateVariable( "litera", otString, id->litera );
  	if ( id->park_in.empty() )
  		Qry.CreateVariable( "park_in", otString, FNull );
  	else
  		Qry.CreateVariable( "park_in", otString, id->park_in );
  	if ( id->park_out.empty() )
  		Qry.CreateVariable( "park_out", otString, FNull );
  	else
 		Qry.CreateVariable( "park_out", otString, id->park_out );
  	Qry.CreateVariable( "pr_del", otInteger, id->pr_del );
  	Qry.CreateVariable( "tid", otInteger, new_tid );
  	if ( !old_dest.remark.empty() )
  		if ( !id->remark.empty() )
  		  id->remark += " " + old_dest.remark;
  		else
  			id->remark = old_dest.remark;
  	if ( id->remark.size() > 250 )
  		id->remark = id->remark.substr( 0, 250 );
  	Qry.CreateVariable( "remark", otString, id->remark );
  	Qry.CreateVariable( "pr_reg", otInteger, id->pr_reg );
  	Qry.Execute();
  	TFlightDelays delays( id->delays );
  	TFlightDelays olddelays;
  	olddelays.Load( id->point_id );
  	if ( !delays.equal( olddelays ) ) {
      delays.Save( id->point_id );
      TTripInfo info;
      info.airline = id->airline;
      info.flt_no = id->flt_no;
      info.airp = id->airp;
      if( GetTripSets( tsSendMVTDelays, info ) && !delays.Empty() ) {
        ProgTrace( TRACE5, "points_MVTdelays insert point_id=%d", id->point_id );
        points_MVTdelays.push_back( id->point_id );
      }
  	}
  	if ( init_trip_stages ) {
  		Qry.Clear();
  		Qry.SQLText =
       "BEGIN "
       " sopp.set_flight_sets(:point_id,:use_seances);"
       "END;";
  		Qry.CreateVariable( "point_id", otInteger, id->point_id );
  		Qry.CreateVariable( "use_seances", otInteger, (int)USE_SEANCES() );
  		Qry.Execute();
  	}
  	else { //!!!�������� ��������� �ਧ��� ignore_auto � trip_stages
      if ( !insert_point ) {
        bool old_ignore_auto = ( old_dest.act_out != NoExists || old_dest.pr_del != 0 );
        bool new_ignore_auto = ( id->act_out != NoExists || id->pr_del != 0 );
        if ( old_ignore_auto != new_ignore_auto ) {
          SetTripStages_IgnoreAuto( id->point_id, new_ignore_auto );
        }
      }
  	}

  	if ( id->pr_del != -1 && id->pr_reg ) {
  	  TDateTime t1 = NoExists, t2 = NoExists;
  	   if ( !insert_point ) {
    	  if ( old_dest.est_out > NoExists )
       		t1 = old_dest.est_out;
       	else
     	  	t1 = old_dest.scd_out;
     	 }
     	if ( id->est_out > NoExists )
     		t2 = id->est_out;
     	else
     		t2 = id->scd_out;
     	if (  insert_point && id->est_out > NoExists && id->scd_out > NoExists ) {
     		t1 = id->scd_out;
     		t2 = id->est_out;
     	}
      if ( t1 > NoExists && t2 > NoExists && t1 != t2 ) {
      	ProgTrace( TRACE5, "trip_stages delay=%s", DateTimeToStr(t2-t1).c_str() );
  		  Qry.Clear();
  		  Qry.SQLText =
  		   "DECLARE "
  		   "  CURSOR cur IS "
  		   "   SELECT stage_id,scd,est FROM trip_stages "
  		   "    WHERE point_id=:point_id AND pr_manual=0; "
  		   "curRow			cur%ROWTYPE;"
  		   "vpr_permit 	ckin_client_sets.pr_permit%TYPE;"
  		   "vpr_first		NUMBER:=1;"
  		   "new_scd			points.scd_out%TYPE;"
  		   "new_est			points.scd_out%TYPE;"
  		   "BEGIN "
  		   "  FOR curRow IN cur LOOP "
  		   "   IF gtimer.IsClientStage(:point_id,curRow.stage_id,vpr_permit) = 0 THEN "
  		   "     vpr_permit := 1;"
  		   "    ELSE "
  		   "     IF vpr_permit!=0 THEN "
  		   "       SELECT NVL(MAX(pr_upd_stage),0) INTO vpr_permit "
  		   "        FROM trip_ckin_client,ckin_client_stages "
  		   "       WHERE point_id=:point_id AND "
  		   "             trip_ckin_client.client_type=ckin_client_stages.client_type AND "
  		   "             stage_id=curRow.stage_id; "
  		   "     END IF;"
  		   "   END IF;"
  		   "   IF vpr_permit!=0 THEN "
  		   "    curRow.est := NVL(curRow.est,curRow.scd)+(:vest-:vscd);"
  		   "    IF vpr_first != 0 THEN "
  		   "      vpr_first := 0; "
  		   "      new_est := curRow.est; "
  		   "      new_scd := curRow.scd; "
  		   "    END IF; "
  		   "    UPDATE trip_stages SET est=curRow.est WHERE point_id=:point_id AND stage_id=curRow.stage_id;"
  		   "   END IF;"
  		   "  END LOOP;"
  		   "  IF vpr_first != 0 THEN "
  		   "   :vscd := NULL;"
  		   "   :vest := NULL;"
  		   "  ELSE "
  		   "   :vscd := new_scd;"
  		   "   :vest := new_est;"
  		   "  END IF;"
  		   "END;";
  		  Qry.CreateVariable( "point_id", otInteger, id->point_id );
  		  Qry.CreateVariable( "vscd", otDate, t1 );
  		  Qry.CreateVariable( "vest", otDate, t2 );
  		  Qry.Execute();
  		  string tolog;
  	    double f;
  	    if ( !Qry.VariableIsNULL( "vscd" ) ) {
  	      t1 = Qry.GetVariableAsDateTime( "vscd" );
  	      t2 = Qry.GetVariableAsDateTime( "vest" );
  	      t1 = t2-t1;
  	      if ( t1 < 0 ) {
  	      	modf( t1, &f );
  		  	  tolog = "���०���� �믮������ �孮�����᪮�� ��䨪� �� ";
  		      if ( f )
    	  	    tolog += IntToString( (int)f ) + " ";
    	  	  tolog += DateTimeToStr( fabs(t1), "hh:nn" );
  	      }
  	      if ( t1 >= 0 ) {
  	      	modf( t1, &f );
  	      	if ( t1 ) {
  		    	  tolog = "����প� �믮������ �孮�����᪮�� ��䨪� �� ";
  		    	  if ( f )
  		    	  	tolog += IntToString( (int)f ) + " ";
  		    	  tolog += DateTimeToStr( t1, "hh:nn" );
  		    	}
  		    	else
  		    		tolog = "�⬥�� ����প� �믮������ �孮�����᪮�� ��䨪�";
  	      }
	        reqInfo->MsgToLog( tolog + " ���� " + id->airp, evtGraph, id->point_id );
	      }
      }
    }
    if ( set_act_out ) {
    	// �� point_num �� ����ᠭ
       try
       {
         exec_stage( id->point_id, sTakeoff );
       }
       catch( std::exception &E ) {
         ProgError( STDLOG, "std::exception: %s", E.what() );
       }
       catch( ... ) {
         ProgError( STDLOG, "Unknown error" );
       };
    	reqInfo->MsgToLog( string( "���⠢����� 䠪�. �뫥� " ) + DateTimeToStr( id->act_out, "hh:nn dd.mm.yy (UTC)" ) + " ���� " + id->airp, evtDisp, move_id, id->point_id );
  		change_act A;
  		A.point_id = id->point_id;
  		A.old_act = old_dest.act_out;
  		A.act = id->act_out;
  		A.pr_land = false;
  		vchangeAct.push_back( A );
 	  }
  	if ( set_pr_del ) {
  		ch_dests = true;
  		Qry.Clear();
  		Qry.SQLText =
  		"SELECT COUNT(*) c FROM "
  		"( SELECT 1 FROM pax_grp,points "
  		"   WHERE points.point_id=:point_id AND "
  		"         point_dep=:point_id AND bag_refuse=0 AND rownum<2 "
  		"  UNION "
  		" SELECT 2 FROM pax_grp,points "
  		"   WHERE points.point_id=:point_id AND "
  		"         point_arv=:point_id AND bag_refuse=0 AND rownum<2 ) ";
  		Qry.CreateVariable( "point_id", otInteger, id->point_id );
  		Qry.Execute();
  		if ( Qry.FieldAsInteger( "c" ) )
  			if ( id->pr_del == -1 )
  				throw AstraLocale::UserException( "MSG.ROUTE.UNABLE_DEL_AIRP.PAX_EXISTS",
  					                                 LParams() << LParam("airp", ElemIdToCodeNative(etAirp,id->airp)));
  			else
  				throw AstraLocale::UserException( "MSG.ROUTE.UNABLE_CANCEL_AIRP.PAX_EXISTS",
  					                                 LParams() << LParam("airp", ElemIdToCodeNative(etAirp,id->airp)));
  	}
   if ( reSetCraft ) {
     ProgTrace( TRACE5, "reSetCraft: point_id=%d", id->point_id );
     setcraft_points.push_back( id->point_id );
   }
   if ( reSetWeights ) {
     ProgTrace( TRACE5, "reSetWeights: point_id=%d", id->point_id );
     PersWeightRules newweights;
     persWeights.getRules( id->point_id, newweights );
     PersWeightRules oldweights;
     oldweights.read( id->point_id );
     if ( !oldweights.equal( &newweights ) )
       newweights.write( id->point_id );
  }
   point_num++;
  } // end for
  if ( ch_point_num ) {
  	Qry.Clear();
  	Qry.SQLText = "UPDATE points SET point_num=:point_num WHERE point_id=:point_id";
  	Qry.DeclareVariable( "point_id", otInteger );
  	Qry.DeclareVariable( "point_num", otInteger );
    for( TSOPPDests::iterator id=dests.begin(); id!=dests.end(); id++ ) {
    	ProgTrace( TRACE5, "point_id=%d, point_num=%d, pr_del=%d", id->point_id, id->point_num, id->pr_del );
    	Qry.SetVariable( "point_id", id->point_id );
    	Qry.SetVariable( "point_num", id->point_num );
    	Qry.Execute();
    }
  }
  
  reSetCraft = false;
  SALONS2::TSetsCraftPoints cpoints;
  for ( vector<int>::iterator i=setcraft_points.begin(); i!=setcraft_points.end(); i++ ) {
    if ( find( cpoints.begin(), cpoints.end(), *i ) != cpoints.end() ) {
      tst();
      continue;
    }
    SALONS2::TFindSetCraft res = SALONS2::AutoSetCraft( *i, cpoints );
    if ( ch_craft && res != SALONS2::rsComp_Found && res != SALONS2::rsComp_NoChanges ) {
 	 	  reSetCraft = true;
 	 	  ch_craft = false;
    }
  }
  for ( vector<int>::iterator i=setcraft_points.begin(); i!=setcraft_points.end(); i++ ) {
    SALONS2::check_diffcomp_alarm( *i );
  }
  
  if ( reSetCraft )
    AstraLocale::showErrorMessage( "MSG.DATA_SAVED.CRAFT_CHANGED.NEED_SET_COMPON" );
  else
    AstraLocale::showMessage( "MSG.DATA_SAVED" );

  for( vector<change_act>::iterator i=vchangeAct.begin(); i!=vchangeAct.end(); i++ ){
   if ( i->pr_land )
     ChangeACT_IN( i->point_id, i->old_act, i->act );
   else
     ChangeACT_OUT( i->point_id, i->old_act, i->act );
  }
  
  //ProgTrace( TRACE5, "ch_dests=%d, insert=%d, change_dests_msg=%s", ch_dests, insert, change_dests_msg.c_str() );
  if ( !ch_dests && !insert )
    change_dests_msg.clear();
  if ( !change_dests_msg.empty() )
    reqInfo->MsgToLog( change_dests_msg, evtDisp, move_id );

  vector<TSOPPTrip> trs1, trs2;

  // ᮧ���� �� �������� ३�� �� ������ ������� �᪫��� 㤠����� �㭪��
  for( TSOPPDests::iterator i=dests.begin(); i!=dests.end(); i++ ) {
  	if ( i->pr_del == -1 ) continue;
  	TSOPPTrip t = createTrip( move_id, i, dests );
  	ProgTrace( TRACE5, "t.pr_del=%d, t.point_id=%d, t.places_out.size()=%d,t.suffix_out=%s",
  	           t.pr_del, t.point_id, t.places_out.size(), t.suffix_out.c_str() );
  	trs1.push_back(t);
  }
  // ᮧ���� �ᥢ������� ३�� �� ��ண� ������� �᪫��� 㤠����� �㭪��
  for( TSOPPDests::iterator i=voldDests.begin(); i!=voldDests.end(); i++ ) {
  	if ( i->pr_del == -1 ) continue;
  	TSOPPTrip t = createTrip( move_id, i, voldDests );
  	ProgTrace( TRACE5, "t.pr_del=%d, t.point_id=%d, t.places_out.size()=%d", t.pr_del, t.point_id, t.places_out.size() );
  	trs2.push_back(t);
  }

  // �஡�� �� ���� ३ᠬ
  for (vector<TSOPPTrip>::iterator i=trs1.begin(); i!=trs1.end(); i++ ) {
  	vector<TSOPPTrip>::iterator j=trs2.begin();
  	for (; j!=trs2.end(); j++ ) // �饬 � ��஬ �㦭� ३�
  	  if ( i->point_id == j->point_id )
  	  	break;
    if ( j == trs2.end() && i->places_out.size() > 0 ) {
    	TSOPPTrip tr2;
    	BitSet<TSOPPTripChange> FltChange;
    	FltChange.setFlag( tsNew );
 	  	if ( i->pr_del >= 1 ) {
 	  		FltChange.setFlag( tsCancelFltOut ); // �⬥�� �뫥�
 	  	}
    	ChangeTrip( i->point_id, *i, tr2, FltChange ); // �� ���� ३� �� �뫥�
    }
    if ( j != trs2.end() ) { // �� �� ���� ३�
    	BitSet<TSOPPTripChange> FltChange;
    	bool pr_f=false;
    	if ( i->places_out.size() && j->places_out.size() ) { // ���� � �� �뫥�
    		if ( i->airline_out != j->airline_out ||
    		     i->flt_no_out != j->flt_no_out ||
    		     i->suffix_out != j->suffix_out ||
    		     i->airp != j->airp ||
    		     i->scd_out != j->scd_out ) {
    		  FltChange.setFlag( tsAttr );
    		  pr_f = true;
    	  }
    	  if ( i->pr_del != j->pr_del ) {
    	  	if ( i->pr_del >= 1 ) {
    	  		FltChange.setFlag( tsCancelFltOut ); // �⬥�� �뫥�
    	  		pr_f = true;
    	  	}
    	  	if ( j->pr_del >= 1 ) {
    	  		FltChange.setFlag( tsRestoreFltOut ); // ����⠭������� �뫥�
    	  		pr_f = true;
    	  	}
    	  }
      }
      if ( i->places_out.size() && !j->places_out.size() ) {
      	FltChange.setFlag( tsAddFltOut ); //�⠫ �뫥�
      	pr_f = true;
      	if ( i->pr_del >= 1 )
      		FltChange.setFlag( tsCancelFltOut ); // �뫥� �⬥���
      }
      if ( !i->places_out.size() && j->places_out.size() ) {
        FltChange.setFlag( tsDelFltOut ); // �� �⠫� �뫥�
        pr_f = true;
      }
      if ( pr_f )
        ChangeTrip( i->point_id, *i, *j, FltChange );
      trs2.erase( j ); // 㤠�塞 ��� �� ᯨ᪠
    }
  }
  // �஡�� �� ���� ३ᠬ, ������ ��� � �����
  for(vector<TSOPPTrip>::iterator j=trs2.begin(); j!=trs2.end(); j++ ) {
  	if ( j->places_out.size() && j->pr_del_out == 0 ) { // ३� �� �� �⬥���
  		TSOPPTrip tr2;
  		BitSet<TSOPPTripChange> FltChange;
  		FltChange.setFlag( tsDelete );
  	  ChangeTrip( j->point_id, tr2, *j, FltChange );  // ३� �� �뫥� 㤠���
  	}
  }
  //����� ��離� ⥫��ࠬ�
  ReBindTlgs( move_id, voldDests );
  // ��ࠢ�� ⥫��ࠬ� ����থ�
  vector<string> tlg_types;
  tlg_types.push_back("MVTC");
  for ( vector<int>::iterator pdel=points_MVTdelays.begin(); pdel!=points_MVTdelays.end(); pdel++ ) {
      try {
          TelegramInterface::SendTlg(*pdel,tlg_types);
      }
      catch(std::exception &E) {
          ProgError(STDLOG,"internal_WriteDests.SendTlg (point_id=%d): %s",*pdel,E.what());
      };
  }
}

void SoppInterface::WriteDests(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  TBaseTable &baseairps = base_tables.get( "airps" );
	xmlNodePtr node = NodeAsNode( "data", reqNode );
	bool canExcept = NodeAsInteger( "canexcept", node );
	int move_id = NodeAsInteger( "move_id", node );
	xmlNodePtr snode = GetNode( "reference", node );
	string reference;
	if ( snode )
		reference = NodeAsString( snode );
	ProgTrace( TRACE5, "write dests move_id=%d, reference=%s", move_id, reference.c_str() );

	TSOPPDests dests;
	TSOPPDest d;
	node = GetNode( "dests", node );
	if ( !node )
		throw AstraLocale::UserException( "MSG.ROUTE.NOT_SPECIFIED" );
	node = node->children;
	xmlNodePtr fnode;
	string city, region;
	while ( node ) {
		snode = node->children;
		d.modify = GetNodeFast( "modify", snode );
		fnode = GetNodeFast( "point_id", snode );
		if ( fnode )
		  d.point_id = NodeAsInteger( fnode );
		else
			d.point_id = NoExists;
		fnode = GetNodeFast( "point_num", snode );
		if ( fnode )
		  d.point_num = NodeAsInteger( fnode );
		else
			d.point_num = NoExists;
		fnode = GetNodeFast( "first_point", snode );
		if ( fnode )
		  d.first_point = NodeAsInteger( fnode );
		else
			d.first_point = NoExists;
   	try {
      d.airp = ElemCtxtToElemId( ecDisp, etAirp, NodeAsStringFast( "airp", snode ), d.airp_fmt, false );
    }
    catch( EConvertError &e ) {
      throw AstraLocale::UserException( "MSG.AIRP.INVALID_GIVEN_CODE" );
    }
		city = ((TAirpsRow&)baseairps.get_row( "code", d.airp, true )).city;
		region = CityTZRegion( city );
		d.region = region;
		fnode = GetNodeFast( "airline", snode );
		if ( fnode ) {
     	try {
        d.airline = ElemCtxtToElemId( ecDisp, etAirline, NodeAsString( fnode ), d.airline_fmt, false );
      }
      catch( EConvertError &e ) {
    	  throw AstraLocale::UserException( "MSG.AIRLINE.INVALID_GIVEN_CODE" );
      }
	  }
		else
			d.airline.clear();
	  fnode = GetNodeFast( "flt_no", snode );
	  if ( fnode )
	  	d.flt_no = NodeAsInteger( fnode );
	  else
	  	d.flt_no = NoExists;
		fnode = GetNodeFast( "suffix", snode );
		if ( fnode ) {
     	try {
        d.suffix = ElemCtxtToElemId( ecDisp, etSuffix, NodeAsString( fnode ), d.suffix_fmt, false );
      }
      catch( EConvertError &e ) {
    	  throw AstraLocale::UserException( "MSG.SUFFIX.INVALID.NO_PARAM" );
      }
	  }
		else
			d.suffix.clear();
		fnode = GetNodeFast( "craft", snode );
		if ( fnode ) {
     	try {
        d.craft = ElemCtxtToElemId( ecDisp, etCraft, NodeAsString( fnode ), d.craft_fmt, false );
      }
      catch( EConvertError &e ) {
    	  throw AstraLocale::UserException( "MSG.CRAFT.WRONG_SPECIFIED" );
      }
	  }
		else
			d.craft.clear();
		fnode = GetNodeFast( "bort", snode );
		if ( fnode ) {
			d.bort = NodeAsString( fnode );
			d.bort = TrimString( d.bort );
			//㤠�塞 �� �������� ᬨ����
      for ( string::iterator istr=d.bort.begin(); istr!=d.bort.end(); istr++ ) {
        if ( *istr >= 0 && *istr < ' ' )
          *istr = ' ';
      }
      char prior_char = 0;
      string::iterator istr=d.bort.begin();
      while ( istr != d.bort.end() ) {
        if ( !IsDigitIsLetter( *istr ) ) {
          if ( *istr != '-' && *istr != ' ' )
            throw AstraLocale::UserException( "MSG.INVALID_CHARS_IN_BOARD_NUM",
                                              LParams() << LParam("symbol", string(1,*istr)) );
          if ( *istr == prior_char && prior_char == ' ' ) {
            istr = d.bort.erase( istr );
            continue;
          }
        }
        prior_char = *istr;
        istr++;
      }
    }
		else
			d.bort.clear();
		fnode = GetNodeFast( "scd_in", snode );
		if ( fnode )
			try {
			  d.scd_in = ClientToUTC( NodeAsDateTime( fnode ), region );
			}
      catch( boost::local_time::ambiguous_result ) {
        //throw UserException( "�������� �६� �ਡ��� � �㭪� %s �� ��।����� �������筮", d.airp.c_str() );
        d.scd_in = ClientToUTC( NodeAsDateTime( fnode ) - 1 , region ) + 1;
      }
		else
			d.scd_in = NoExists;
		fnode = GetNodeFast( "est_in", snode );
		if ( fnode )
			try {
			  d.est_in = ClientToUTC( NodeAsDateTime( fnode ), region );
			}
      catch( boost::local_time::ambiguous_result ) {
        //throw UserException( "����⭮� �६� �ਡ��� � �㭪� %s �� ��।����� �������筮", d.airp.c_str() );
        d.est_in = ClientToUTC( NodeAsDateTime( fnode ) - 1 , region ) + 1;
      }
		else
			d.est_in = NoExists;
		fnode = GetNodeFast( "act_in", snode );
		if ( fnode )
			try {
			  d.act_in = ClientToUTC( NodeAsDateTime( fnode ), region );
			}
      catch( boost::local_time::ambiguous_result ) {
//        throw UserException( "�����᪮� �६� �ਡ��� � �㭪� %s �� ��।����� �������筮", d.airp.c_str() );
        d.act_in = ClientToUTC( NodeAsDateTime( fnode ) - 1 , region ) + 1;
      }
		else
			d.act_in = NoExists;
		fnode = GetNodeFast( "scd_out", snode );
		if ( fnode )
			try {
			  d.scd_out = ClientToUTC( NodeAsDateTime( fnode ), region );
			}
      catch( boost::local_time::ambiguous_result ) {
        //throw UserException( "�������� �६� �뫥� � �㭪� %s �� ��।����� �������筮", d.airp.c_str() );
        d.scd_out = ClientToUTC( NodeAsDateTime( fnode ) - 1 , region ) + 1;
      }
		else
			d.scd_out = NoExists;
		fnode = GetNodeFast( "est_out", snode );
		if ( fnode )
			try {
			  d.est_out = ClientToUTC( NodeAsDateTime( fnode ), region );
			}
      catch( boost::local_time::ambiguous_result ) {
        //throw UserException( "����⭮� �६� �뫥� � �㭪� %s �� ��।����� �������筮", d.airp.c_str() );
        d.est_out = ClientToUTC( NodeAsDateTime( fnode ) - 1 , region ) + 1;
      }
		else
			d.est_out = NoExists;
		fnode = GetNodeFast( "act_out", snode );
		if ( fnode ) {
			try {
			  d.act_out = ClientToUTC( NodeAsDateTime( fnode ), region );
			}
      catch( boost::local_time::ambiguous_result ) {
        //throw UserException( "�����᪮� �६� �뫥� � �㭪� %s �� ��।����� �������筮", d.airp.c_str() );
        d.act_out = ClientToUTC( NodeAsDateTime( fnode ) - 1 , region ) + 1;
      }
		}
		else
			d.act_out = NoExists;
		fnode = GetNodeFast( "delays", snode );
		d.delays.clear();
		if ( fnode ) {
			fnode = fnode->children;
			xmlNodePtr dnode;
			TElemFmt fmt;
			while ( fnode ) {
				dnode = fnode->children;
				TSOPPDelay delay;
				delay.code = ElemToElemId( etDelayType, NodeAsStringFast( "delay_code", dnode ), fmt );
				if ( fmt == efmtUnknown )
					throw AstraLocale::UserException( "MSG.CHECK_FLIGHT.INVALID_DELAY" );
				try {
				  delay.time = ClientToUTC( NodeAsDateTimeFast( "time", dnode ), region );
				}
        catch( boost::local_time::ambiguous_result ) {
          //throw UserException( "�६� ����প� � �㭪� %s �� ��।����� �������筮", d.airp.c_str() );
          delay.time = ClientToUTC( NodeAsDateTimeFast( "time", dnode ) - 1 , region ) + 1;
        }
				d.delays.push_back( delay );
				fnode = fnode->next;
		  }
		}
		fnode = GetNodeFast( "trip_type", snode );
		TElemFmt fmt;
		if ( fnode ) {
			d.triptype = ElemToElemId(etTripType,NodeAsString( fnode ),fmt);
			if ( fmt == efmtUnknown )
				throw AstraLocale::UserException( "MSG.CHECK_FLIGHT.INVALID_TYPE" );
		}
		else
			d.triptype.clear();

		fnode = GetNodeFast( "litera", snode );
		if ( fnode ) {
			d.litera = ElemToElemId(etTripLiter,NodeAsString( fnode ),fmt);
			if ( fmt == efmtUnknown )
				throw AstraLocale::UserException( "MSG.CHECK_FLIGHT.INVALID_LITERA" );
		}
		else
			d.litera.clear();
		fnode = GetNodeFast( "park_in", snode );
		if ( fnode )
			d.park_in = NodeAsString( fnode );
		else
			d.park_in.clear();
		fnode = GetNodeFast( "park_out", snode );
		if ( fnode )
			d.park_out = NodeAsString( fnode );
		else
			d.park_out.clear();
		d.pr_tranzit = NodeAsIntegerFast( "pr_tranzit", snode );
//		d.pr_reg = NodeAsIntegerFast( "pr_reg", snode );
    d.pr_reg = 0;
		fnode = GetNodeFast( "pr_del", snode );
		if ( fnode )
			d.pr_del = NodeAsInteger( fnode );
		else
			d.pr_del = 0;
		dests.push_back( d );
		node = node->next;
  } // end while
  internal_WriteDests( move_id, dests, reference, canExcept, ctxt, reqNode, resNode );
  if ( GetNode( "data/notvalid", resNode ) ) {
  	return;
  }
  NewTextChild( reqNode, "move_id", move_id );
  ReadDests( ctxt, reqNode, resNode );
}

void SoppInterface::DropFlightFact(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  int point_id = NodeAsInteger( "point_id", reqNode );
  TQuery Qry(&OraSession);
	Qry.SQLText=
	  "SELECT move_id,airline||flt_no||suffix as trip,act_out,point_num,pr_del "
    "FROM points "
    "WHERE point_id=:point_id AND pr_del!=-1 AND act_out IS NOT NULL FOR UPDATE";
  Qry.CreateVariable("point_id",otInteger,point_id);
  Qry.Execute();
  if (Qry.Eof) throw AstraLocale::UserException("MSG.FLIGHT.CHANGED.REFRESH_DATA");
	int point_num = Qry.FieldAsInteger( "point_num" );
	int move_id = Qry.FieldAsInteger( "move_id" );
	string trip = Qry.FieldAsString( "trip" );
	TDateTime act_out = Qry.FieldAsDateTime( "act_out" );
	int pr_del = Qry.FieldAsInteger( "pr_del" );
	Qry.Clear();
	Qry.SQLText =
	 "UPDATE points "
	 "  SET act_out=NULL, act_in=DECODE(point_num,:point_num,act_in,NULL) "
	 " WHERE move_id=:move_id AND point_num>=:point_num";
	Qry.CreateVariable( "move_id", otInteger, move_id );
	Qry.CreateVariable( "point_num", otInteger, point_num );
	Qry.Execute();
	TReqInfo *reqInfo = TReqInfo::Instance();
	reqInfo->MsgToLog( string( "�⬥�� 䠪�. �뫥� ३� " ) + trip, evtDisp, move_id, point_id );
	ChangeACT_OUT( point_id, act_out, NoExists );
	SetTripStages_IgnoreAuto( point_id, pr_del != 0 );
	ReadTrips( ctxt, reqNode, resNode );
}

void SoppInterface::ReadCRS_Displaces(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode) /*???*/
{
	int point_id = NodeAsInteger( "point_id", reqNode );
	xmlNodePtr crsdNode = NewTextChild( NewTextChild( resNode, "data" ), "crs_displaces" );
	xmlNodePtr displnode = NULL;
	NewTextChild( crsdNode, "point_id", point_id );

	TQuery Qry( &OraSession );
	Qry.SQLText = "SELECT airp,first_point,pr_tranzit,airp_fmt FROM points WHERE point_id=:point_id AND pr_del!=-1";
	Qry.CreateVariable( "point_id", otInteger, point_id );
	Qry.Execute();
	if ( Qry.Eof )
		throw AstraLocale::UserException( "MSG.FLIGHT.NOT_FOUND" );
	int pr_tranzit = ( !Qry.FieldIsNULL( "first_point" ) && Qry.FieldAsInteger( "pr_tranzit" ) );
	string airp_dep = Qry.FieldAsString( "airp" );
	string region = AirpTZRegion( airp_dep, true );
	TDateTime local_time;
	modf(UTCToLocal( NowUTC(), region ),&local_time);
	NewTextChild( crsdNode, "airp_time", DateTimeToStr( local_time ) );

	Qry.Clear();
	Qry.SQLText =
   "SELECT p2.airp FROM points p1, points p2 "
   "WHERE p1.point_id=:point_id AND p1.pr_del!=-1 AND "
   "      p2.first_point IN (p1.first_point,p1.point_id) AND "
   "      p2.point_num>p1.point_num AND p2.pr_del!=-1 "
   "ORDER BY p2.point_num ASC";
	Qry.CreateVariable( "point_id", otInteger, point_id );
	Qry.Execute();
	while ( !Qry.Eof ) {
		if ( !displnode )
			displnode = NewTextChild( crsdNode, "airps_arv" );
		NewTextChild( displnode, "airp_arv", ElemIdToCodeNative( etAirp, Qry.FieldAsString( "airp" ) ) );
		Qry.Next();
	}
	NewTextChild( crsdNode, "airp_dep", ElemIdToCodeNative( etAirp, airp_dep ) );
	if ( pr_tranzit )
	  NewTextChild( crsdNode, "pr_tranzit", pr_tranzit );
	Qry.Clear();
	Qry.SQLText =
	 "SELECT airp_arv_spp,class_spp,airp_dep,"
	 "       airline,flt_no,suffix,scd,"
	 "       airp_arv_tlg,class_tlg, status "
	 "FROM crs_displace2 "
	 "WHERE crs_displace2.point_id_spp = :point_id";
	Qry.CreateVariable( "point_id", otInteger, point_id );
	Qry.Execute();
  displnode = NULL;
  while ( !Qry.Eof ) {
  	if ( !displnode )
  		displnode = NewTextChild( crsdNode, "displaces" );
  	xmlNodePtr snode = NewTextChild( displnode, "displace" );
		NewTextChild( snode, "airp_arv_spp", ElemIdToCodeNative( etAirp, Qry.FieldAsString( "airp_arv_spp" ) ) );
		NewTextChild( snode, "class_spp", ElemIdToCodeNative( etClass, Qry.FieldAsString( "class_spp" ) ) );
		NewTextChild( snode, "airline", ElemIdToCodeNative( etAirline, Qry.FieldAsString( "airline" ) ) );
		NewTextChild( snode, "flt_no", Qry.FieldAsString( "flt_no" ) );
		NewTextChild( snode, "suffix", ElemIdToCodeNative( etSuffix, Qry.FieldAsString( "suffix" ) ) );
		NewTextChild( snode, "scd", DateTimeToStr( Qry.FieldAsDateTime( "scd" ) ) );
		NewTextChild( snode, "airp_dep", ElemIdToCodeNative( etAirp, Qry.FieldAsString( "airp_dep" ) ) );
		NewTextChild( snode, "airp_arv_tlg", ElemIdToCodeNative( etAirp, Qry.FieldAsString( "airp_arv_tlg" ) ) );
		NewTextChild( snode, "class_tlg", ElemIdToCodeNative( etClass, Qry.FieldAsString( "class_tlg" ) ) );
    NewTextChild( snode, "pr_goshow", (int)(DecodePaxStatus( Qry.FieldAsString( "status" ) ) != ASTRA::psCheckin) ); //!!!��⮬ ���
	  NewTextChild( snode, "status", Qry.FieldAsString( "status" ) );
  	Qry.Next();
  }
  /* pr_utc */
	Qry.Clear();
	Qry.SQLText =
	 "SELECT DISTINCT airline,flt_no,suffix,scd,airp_arv FROM tlg_trips "
	 " WHERE pr_utc=0 AND airp_dep=:airp_dep AND "
	 "       airp_arv IS NULL AND "
	 "       scd >= TRUNC(sysdate) - 1 AND scd < TRUNC(sysdate) + 2 "
	 " ORDER BY scd,airline,flt_no,suffix,airp_arv ";
	Qry.CreateVariable( "airp_dep", otString, airp_dep );
	Qry.Execute();
	displnode = NULL;
	while ( !Qry.Eof ) {
		if ( !displnode )
			displnode = NewTextChild( crsdNode, "targets" );
		xmlNodePtr snode = NewTextChild( displnode, "target" );
		NewTextChild( snode, "airline", ElemIdToCodeNative(etAirline, Qry.FieldAsString( "airline" ) ) );
		NewTextChild( snode, "flt_no", Qry.FieldAsString( "flt_no" ) );
		NewTextChild( snode, "suffix", ElemIdToCodeNative(etSuffix, Qry.FieldAsString( "suffix" ) ) );
		NewTextChild( snode, "scd", DateTimeToStr( Qry.FieldAsDateTime( "scd" ) ) );	  	//???
	  Qry.Next();
	}
}

string getCrsDisplace( int point_id, TDateTime local_time, bool to_local, TQuery &Qry )
{

  bool ch_class = false;
  bool ch_dest = false;
  string str_to, trip;
  TTripInfo info;
  while ( !Qry.Eof ) {
    TDateTime scd = Qry.FieldAsDateTime( "scd" );
    if ( to_local ) {
    	string region = AirpTZRegion( Qry.FieldAsString( "airp_dep" ), true );
    	scd = UTCToLocal( scd, region );
    }
  	if ( Qry.FieldAsInteger( "point_id_spp" ) == point_id ) {
  		if ( !to_local && string(Qry.FieldAsString( "class_spp" )) != string(Qry.FieldAsString( "class_tlg" )) )
  			ch_class = true;
  		if ( !to_local && string(Qry.FieldAsString( "airp_arv_spp" )) != string(Qry.FieldAsString( "airp_arv_tlg" )) )
  			ch_dest = true;
  	}
  	else {
  		trip = ElemIdToCodeNative( etAirline, Qry.FieldAsString( "airline" ) ) +
  		       Qry.FieldAsString( "flt_no" ) +
  		       ElemIdToCodeNative( etSuffix, Qry.FieldAsString( "suffix" ) );
  		TDateTime  f1, f2;
  		modf( local_time, &f1 );
  		modf( scd, &f2 );
  		if ( f1 != f2 ) {
  			if ( DateTimeToStr( f1, "mm" ) == DateTimeToStr( f2, "mm" ) )
  				trip += string("/") + DateTimeToStr( f2, "dd" );
  			else
  				trip += string("/") + DateTimeToStr( f2, "dd.mm" );
  		}
      if ( str_to.find( trip ) == string::npos ) {
        if ( !str_to.empty() )
          str_to += " ";
        str_to += trip;
      }
  	}
  	Qry.Next();
  }
  if ( ch_class )
    str_to = AstraLocale::getLocaleText("���. �����") + " " + str_to;
  if ( ch_dest )
    str_to = AstraLocale::getLocaleText("���. �㭪�") + " " + str_to;
 	return str_to;
}

void SoppInterface::WriteCRS_Displaces(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  int point_id = NodeAsInteger( "point_id", reqNode );
  TQuery TQry(&OraSession);
  TQry.SQLText =
   "SELECT point_id FROM tlg_trips "
   " WHERE pr_utc=0 AND bind_type=0 AND airp_dep=:airp_dep AND airp_arv IS NULL AND "
   "       airline=:airline AND flt_no=:flt_no AND NVL(suffix,' ')=NVL(:suffix,' ') AND "
   "       scd >= TRUNC(TO_DATE(:scd)) AND scd < TRUNC(TO_DATE(:scd))+1";
  TQry.DeclareVariable( "airp_dep", otString );
  TQry.DeclareVariable( "airline", otString );
  TQry.DeclareVariable( "flt_no", otInteger );
  TQry.DeclareVariable( "suffix", otString );
  TQry.DeclareVariable( "scd", otDate );
  TQuery Qry(&OraSession);
  Qry.SQLText = "SELECT airp FROM points WHERE point_id=:point_id AND pr_del!=-1";
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.Execute();
  if ( Qry.Eof )
  	throw AstraLocale::UserException( "MSG.FLIGHT.NOT_FOUND" );
  string airp_dep = Qry.FieldAsString( "airp" );
	string region = AirpTZRegion( Qry.FieldAsString( "airp" ), true );
	TDateTime local_time;
	modf(UTCToLocal( NowUTC(), region ),&local_time);
  Qry.Clear();
  Qry.SQLText =
   "SELECT point_id_spp,airp_arv_spp,class_spp,airline,flt_no,suffix,scd,"
   "       point_id_tlg,airp_arv_tlg,class_tlg,status,airp_dep "
   " FROM crs_displace2 WHERE point_id_spp=:point_id_spp";
  Qry.CreateVariable( "point_id_spp", otInteger, point_id );
  Qry.Execute();
  vector<tcrs_displ> crs_displaces;
  while ( !Qry.Eof ) {
  	tcrs_displ t;
  	t.point_id_spp = Qry.FieldAsInteger( "point_id_spp" );
  	t.airp_arv_spp = Qry.FieldAsString( "airp_arv_spp" );
  	t.class_spp = Qry.FieldAsString( "class_spp" );
  	t.airline = Qry.FieldAsString( "airline" );
  	t.flt_no = Qry.FieldAsInteger( "flt_no" );
  	t.suffix = Qry.FieldAsString( "suffix" );
  	t.scd = Qry.FieldAsDateTime( "scd" );
  	if ( Qry.FieldIsNULL( "point_id_tlg" ) )
  		t.point_id_tlg = NoExists;
  	else
  		t.point_id_tlg = Qry.FieldAsInteger( "point_id_tlg" );
  	t.to_airp_dep = Qry.FieldAsString( "airp_dep" );
  	t.airp_arv_tlg = Qry.FieldAsString( "airp_arv_tlg" );
  	t.class_tlg = Qry.FieldAsString( "class_tlg" );
  	t.status = Qry.FieldAsString( "status" );
  	crs_displaces.push_back( t );
  	Qry.Next();
  }
  Qry.Clear();
  Qry.SQLText = "DELETE crs_displace2 WHERE point_id_spp=:point_id_spp";
  Qry.CreateVariable( "point_id_spp", otInteger, point_id );
  Qry.Execute();
  Qry.Clear();
  Qry.SQLText =
   "  INSERT INTO crs_displace2(point_id_spp,airp_arv_spp,class_spp,airline,flt_no,suffix,scd,airp_dep, "
   "                            point_id_tlg,airp_arv_tlg,class_tlg,status) "
   "   SELECT :point_id_spp,:airp_arv_spp,:class_spp,:airline,:flt_no,:suffix,:scd,:airp_dep, "
   "          :point_id_tlg,:airp_arv_tlg,:class_tlg,:status FROM dual ";
  Qry.CreateVariable( "point_id_spp", otInteger, point_id );
  Qry.DeclareVariable( "airp_arv_spp", otString );
  Qry.DeclareVariable( "class_spp", otString );
  Qry.DeclareVariable( "airline", otString );
  Qry.DeclareVariable( "flt_no", otInteger );
  Qry.DeclareVariable( "suffix", otString );
  Qry.DeclareVariable( "scd", otDate );
  Qry.DeclareVariable( "airp_dep", otString );
  Qry.DeclareVariable( "airp_arv_tlg", otString );
  Qry.DeclareVariable( "class_tlg", otString );
  Qry.DeclareVariable( "status", otString );
  Qry.DeclareVariable( "point_id_tlg", otInteger );
  xmlNodePtr node = NodeAsNode( "crs_displace", reqNode );
  xmlNodePtr snode;
  node = node->children;
  string airp_arv_spp, class_spp, airline, suffix, airp_arv_tlg, class_tlg;
  int flt_no;
  string status;
  int point_id_tlg;
  TDateTime scd;
  string tolog;
  vector<int>::iterator r;
  xmlNodePtr dataNode = NewTextChild( resNode, "data" );
  xmlNodePtr airp_depNode;
  string to_airp_dep;
  TElemFmt fmt;
  while ( node ) {
  	snode = node->children;
  	airp_arv_spp = ElemToElemId( etAirp, NodeAsStringFast( "airp_arv_spp", snode ), fmt );
  	if ( fmt == efmtUnknown )
  		throw AstraLocale::UserException( "MSG.INVALID_ARRIVAL_AIRP" );
  	class_spp = ElemToElemId( etClass, NodeAsStringFast( "class_spp", snode ), fmt );
    if ( fmt == efmtUnknown )
  		throw AstraLocale::UserException( "MSG.INVALID_CLASS" );
  	airline = ElemToElemId( etAirline, NodeAsStringFast( "airline", snode ), fmt );
    if ( fmt == efmtUnknown )
  		throw AstraLocale::UserException( "MSG.INVALID_AIRLINE" );
  	flt_no = NodeAsIntegerFast( "flt_no", snode );
  	if ( flt_no <= 0 || flt_no > 99999 )
  		throw AstraLocale::UserException( "MSG.CHECK_FLIGHT.INVALID_FLT_NO" );
  	suffix = NodeAsStringFast( "suffix", snode );
  	if ( !suffix.empty() )
  		suffix = ElemToElemId( etSuffix, suffix, fmt );
    if ( fmt == efmtUnknown )
  		throw AstraLocale::UserException( "MSG.CHECK_FLIGHT.INVALID_SUFFIX" );
  	scd = NodeAsDateTimeFast( "scd", snode );
  	airp_depNode = GetNodeFast( "airp_dep", snode );
  	if ( airp_depNode ) {
  		to_airp_dep = ElemToElemId( etAirp, NodeAsString( airp_depNode ), fmt );
  	  if ( fmt == efmtUnknown )
  		  throw AstraLocale::UserException( "MSG.AIRP.INVALID_DEPARTURE" );
  	}
  	else
  		to_airp_dep = airp_dep;
  	airp_arv_tlg = ElemToElemId( etAirp, NodeAsStringFast( "airp_arv_tlg", snode ), fmt );
  	if ( fmt == efmtUnknown )
  		throw AstraLocale::UserException( "MSG.INVALID_ARRIVAL_AIRP" );
  	class_tlg = ElemToElemId( etClass, NodeAsStringFast( "class_tlg", snode ), fmt );
    if ( fmt == efmtUnknown )
  		throw AstraLocale::UserException( "MSG.INVALID_CLASS" );
  	if ( GetNodeFast( "pr_goshow", snode ) )
  		status = EncodePaxStatus( (TPaxStatus)NodeAsIntegerFast( "pr_goshow", snode ) );
  	else
  	  status = NodeAsStringFast( "status", snode );
  	Qry.SetVariable( "airp_arv_spp", airp_arv_spp );
  	Qry.SetVariable( "class_spp", class_spp );
  	Qry.SetVariable( "airline", airline );
  	Qry.SetVariable( "flt_no", flt_no );
  	Qry.SetVariable( "suffix", suffix );
  	Qry.SetVariable( "scd", scd );
  	Qry.SetVariable( "airp_dep", to_airp_dep );
  	Qry.SetVariable( "airp_arv_tlg", airp_arv_tlg );
  	Qry.SetVariable( "class_tlg", class_tlg );
  	Qry.SetVariable( "status", status );

    TQry.SetVariable( "airp_dep", to_airp_dep );
  	TQry.SetVariable( "airline", airline );
  	TQry.SetVariable( "flt_no", flt_no );
  	if ( suffix.empty() )
  	  TQry.SetVariable( "suffix", FNull );
  	else
  		TQry.SetVariable( "suffix", suffix );
  	TQry.SetVariable( "scd", scd );
  	TQry.Execute();
  	if ( !TQry.Eof )
  	  point_id_tlg = TQry.FieldAsInteger( "point_id" );
  	else
  		point_id_tlg = NoExists;
  	if ( point_id_tlg == NoExists )
  		Qry.SetVariable( "point_id_tlg", FNull );
  	else
  		Qry.SetVariable( "point_id_tlg", point_id_tlg );
  	Qry.Execute();
  	vector<tcrs_displ>::iterator r = crs_displaces.end();
  	for ( r=crs_displaces.begin(); r!=crs_displaces.end(); r++ ) {
  		if ( r->point_id_spp == point_id &&
  			   r->point_id_tlg == point_id_tlg )
  			break;
  	}
  	if ( r != crs_displaces.end() ) {
  		if ( r->airp_arv_spp != airp_arv_spp ||
  			   r->class_spp != class_spp ||
  			   r->airline != airline ||
  			   r->flt_no != flt_no ||
  			   r->suffix != suffix ||
  			   r->scd != scd ||
  			   r->to_airp_dep != to_airp_dep ||
  			   r->airp_arv_tlg != airp_arv_tlg ||
  			   r->class_tlg != class_tlg ||
  			   r->status != status ) {
        tolog = "��������� ���ᠤ�� ���ᠦ�஢: ";
        tolog += string("�ਫ�� ") + airp_arv_spp;
        if ( airp_arv_spp != r->airp_arv_spp )
          tolog +=  string("(") + r->airp_arv_spp + ")";
        tolog += ",����� " + class_spp;
        if ( class_spp != r->class_spp )
          tolog += string("(") + r->class_spp + ")";
        tolog += ",�� ३� " + airline + IntToString( flt_no ) + suffix + "/" + DateTimeToStr( scd, "dd.mm" );
        if ( airline + IntToString( flt_no ) + suffix + "/" + DateTimeToStr( scd, "dd.mm" ) !=
        	   r->airline + IntToString( r->flt_no ) + r->suffix + "/" + DateTimeToStr( r->scd, "dd.mm" ) )
          tolog += string("(") + r->airline + IntToString( r->flt_no ) + r->suffix + "/" + DateTimeToStr( r->scd, "dd.mm" ) + ")";
        tolog += string(",�뫥� ") + to_airp_dep + string(",�ਫ�� ") + airp_arv_tlg;
        if ( airp_arv_tlg != r->airp_arv_tlg )
          tolog += string("(") + r->airp_arv_tlg + ")";
       	tolog += string(",����� ") + class_tlg;
       	if ( class_tlg != r->class_tlg )
       		tolog += string("(") + r->class_tlg + ")";
       	tolog += string(",����� '") + status + "'";
        if ( status != r->status )
        	  tolog += string(" ('" ) + status + "')";
        TReqInfo::Instance()->MsgToLog( tolog, evtFlt, point_id );
  	  }
  		crs_displaces.erase( r );
  	}
  	else {
      tolog = "���������� ���ᠤ�� ���ᠦ�஢: ";
      tolog += string("�ਫ�� ") + airp_arv_spp + ",����� " + class_spp + ",�� ३� ";
      tolog += airline + IntToString( flt_no ) + suffix + "/" + DateTimeToStr( scd, "dd.mm" );
      tolog += string( ",�뫥� ") + to_airp_dep + string(",�ਫ�� ") + airp_arv_tlg +
               ",����� " + class_tlg + ", ����� '" + status + "'";
      TReqInfo::Instance()->MsgToLog( tolog, evtFlt, point_id );
    }
  	node = node->next;
  }
  for ( vector<tcrs_displ>::iterator r=crs_displaces.begin(); r!=crs_displaces.end(); r++ ) {
    tolog = "�������� ���ᠤ�� ���ᠦ�஢: ";
    tolog += string("�ਫ�� ") + r->airp_arv_spp + ",����� " + r->class_spp + ",�� ३� ";
    tolog += r->airline + IntToString( r->flt_no ) + r->suffix + "/" + DateTimeToStr( r->scd, "dd.mm" );
    tolog += string(",�뫥� ") + r->to_airp_dep + string(",�ਫ�� ") + r->airp_arv_tlg +
             ",����� " + r->class_tlg + ",����� '" + r->status + "'";
    TReqInfo::Instance()->MsgToLog( tolog, evtFlt, r->point_id_spp );
  }
  Qry.Clear();
  Qry.SQLText = crs_displace_to_SQL;
  Qry.CreateVariable( "point_id_spp", otInteger, point_id );
  Qry.Execute();
  xmlNodePtr tripNode = NewTextChild( dataNode, "tripDispl" );
  NewTextChild( tripNode, "to", getCrsDisplace( point_id, local_time, true, Qry ) );
  Qry.Clear();
  Qry.SQLText = crs_displace_from_SQL;
  Qry.CreateVariable( "point_id_spp", otInteger, point_id );
  Qry.Execute();
  NewTextChild( tripNode, "from", getCrsDisplace( point_id, local_time, false, Qry ) );
  AstraLocale::showMessage( "MSG.DATA_SAVED" );
}

inline void setDestTime( xmlNodePtr timeNode, TDateTime &vtime, const string &region )
{
  if ( timeNode )
	  try {
	    vtime = ClientToUTC( NodeAsDateTime( timeNode ), region );
		}
    catch( boost::local_time::ambiguous_result ) {
      vtime = ClientToUTC( NodeAsDateTime( timeNode ) - 1, region ) + 1;
    }
	else
	  vtime = NoExists;
}

void SoppInterface::WriteISGTrips(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
	TSOPPDests dests;
	string reference;
	xmlNodePtr node = NodeAsNode( "data", reqNode );
	bool canExcept = NodeAsInteger( "canexcept", node );
	node = NodeAsNode( "trips", node );
	node = node->children;
	xmlNodePtr tripNode;
	while ( node ) {
	  tripNode = node->children;
	  int move_id = NodeAsIntegerFast( "move_id", tripNode );
    int point_id = NodeAsIntegerFast( "point_id", tripNode );
   internal_ReadDests( move_id, dests, reference, NoExists );
	  xmlNodePtr snode = GetNodeFast( "reference", tripNode );
	  if ( snode )
	  	reference = NodeAsString( snode );
	  bool ex = false;
	  TSOPPDests::iterator l;
	  if ( dests.size() > 1 ) {
	    for( TSOPPDests::iterator d=dests.begin(); d!=dests.end(); d++ ) {
	    	d->modify = false;
	    	if ( d->point_id == point_id ) {
	    		ex = true;
	    		if ( d == dests.end() - 1 )
	    			l = d - 1;
	    		else
	    			l = d;
	    		snode = GetNodeFast( "craft", tripNode );
	    		if ( snode ) {
           	try {
              l->craft = ElemCtxtToElemId( ecDisp, etCraft, NodeAsString( snode ), l->craft_fmt, false );
            }
            catch( EConvertError &e ) {
    	        throw AstraLocale::UserException( "MSG.CRAFT.WRONG_SPECIFIED" );
            }
	    		  l->modify = true;
	    		  for( TSOPPDests::iterator b=dests.begin(); b!=dests.end()-1; b++ ) {
	    		  	if ( b->craft.empty() ) { // set craft in all dests
	    		  		b->craft = l->craft;
	    		  		b->modify = true;
	    		  	}
	    		  }
	    		}
	    		snode = GetNodeFast( "bort", tripNode );
	    		if ( snode ) {
	    		  l->bort = NodeAsString( snode );
	    		  l->modify = true;
	    		  for( TSOPPDests::iterator b=dests.begin(); b!=dests.end()-1; b++ ) {
	    		  	if ( b->bort.empty() ) { // set bort in all dests
	    		  		b->bort = l->bort;
	    		  		b->modify = true;
	    		  	}
	    		  }
	    		}
	    		snode = GetNodeFast( "park_in", tripNode );
	    		if ( snode ) {
	    		  d->park_in = NodeAsString( snode );
	    		  d->modify = true;
	    		}
	    		snode = GetNodeFast( "park_out", tripNode );
	    		if ( snode ) {
	    		  d->park_out = NodeAsString( snode );
	    		  d->modify = true;
	    		}
	    		snode = GetNodeFast( "dests", tripNode );
	    		if ( snode ) {
	    			snode = snode->children;
	    			xmlNodePtr tmNode;
	    			TElemFmt fmt;
	    			while ( snode ) {
	    				xmlNodePtr destNode = snode->children;
 	    				point_id = NodeAsIntegerFast( "point_id", destNode );
	            for( TSOPPDests::iterator d=dests.begin(); d!=dests.end(); d++ ) {
	    	        if ( d->point_id == point_id ) {
     		    		  d->modify = true;
	    	        	setDestTime( GetNodeFast( "scd_in", destNode ), d->scd_in, d->region );
	    	        	setDestTime( GetNodeFast( "est_in", destNode ), d->est_in, d->region );
	    	        	setDestTime( GetNodeFast( "act_in", destNode ), d->act_in, d->region );
	    	        	setDestTime( GetNodeFast( "scd_out", destNode ), d->scd_out, d->region );
	    	        	setDestTime( GetNodeFast( "est_out", destNode ), d->est_out, d->region );
    	        	  setDestTime( GetNodeFast( "act_out", destNode ), d->act_out, d->region );
	    	        	tmNode = GetNodeFast( "pr_del", destNode );
	    	        	if ( tmNode )
	    	        	  d->pr_del = NodeAsInteger( tmNode );
	    	        	else
	    	        		d->pr_del = 0;
	    	          d->delays.clear();
	    	        	tmNode = GetNodeFast( "delays", destNode );
	    	        	if ( tmNode ) {
	    	        		tmNode = tmNode->children;
	    	        		while ( tmNode ) {
                      TSOPPDelay delay;
	    	        		  xmlNodePtr N = tmNode->children;
	    	    	    		delay.code = ElemToElemId( etDelayType, NodeAsStringFast( "code", N ), fmt );
    	    						if ( fmt == efmtUnknown )
              					throw AstraLocale::UserException( "MSG.CHECK_FLIGHT.INVALID_DELAY" );
	    	        			setDestTime( GetNodeFast( "time", N ), delay.time, d->region );
	    	        			d->delays.push_back( delay );
	    	        			tmNode = tmNode->next;
	    	        	  }
	    	        	}
	    	        	break;
	    	        }
	    				}
	    				snode = snode->next;
	    			}
	    		}
   			  internal_WriteDests( move_id, dests, reference, canExcept, ctxt, reqNode, resNode );
	      }
	    }
	  }
	  if ( !ex )
	  	throw AstraLocale::UserException( "MSG.FLIGHT.NOT_FOUND.REFRESH_DATA" );
	  node = node->next;
	}
}

void SoppInterface::DeleteISGTrips(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
	xmlNodePtr node = NodeAsNode( "data", reqNode );
	int move_id = NodeAsInteger( "move_id", node );
	TSOPPDests dests_del;
	string reference;
	internal_ReadDests( move_id, dests_del, reference, NoExists );
  vector<TSOPPTrip> trs;
  // ᮧ���� �� �������� ३�� �� ������ ������� �᪫��� 㤠����� �㭪��
  for( TSOPPDests::iterator i=dests_del.begin(); i!=dests_del.end(); i++ ) {
  	if ( i->pr_del == -1 ) continue;
  	TSOPPTrip t = createTrip( move_id, i, dests_del );
  	ProgTrace( TRACE5, "t.pr_del=%d, t.point_id=%d, t.places_out.size()=%d,t.suffix_out=%s",
  	           t.pr_del, t.point_id, t.places_out.size(), t.suffix_out.c_str() );
  	trs.push_back(t);
  };

	TQuery Qry(&OraSession);
  // �஢�ઠ �� �।��� ⮣�, �� �� ��� �� �⮨� ����� ����⨢�� ���� �㣠����
	Qry.Clear();
	Qry.SQLText = "SELECT COUNT(*) c, point_dep FROM pax_grp WHERE point_dep IN "
	              "( SELECT point_id FROM points WHERE move_id=:move_id ) "
	              "GROUP BY point_dep ";
	Qry.CreateVariable( "move_id", otInteger, move_id );
	Qry.Execute();
	if ( !Qry.Eof && Qry.FieldAsInteger( "c" ) > 0 ) {
		int point_id = Qry.FieldAsInteger( "point_dep" );
		Qry.Clear();
		Qry.SQLText = "SELECT airp FROM points WHERE point_id=:point_id";
		Qry.CreateVariable( "point_id", otInteger, point_id );
		Qry.Execute();
		ProgTrace( TRACE5, "airp=%s", ElemIdToCodeNative(etAirp,Qry.FieldAsString("airp")).c_str() );
		throw AstraLocale::UserException( "MSG.FLIGHT.UNABLE_DEL.PAX_EXISTS", LParams() << LParam("airp", ElemIdToNameLong(etAirp,Qry.FieldAsString("airp"))));
	}
	Qry.Clear();
	Qry.SQLText = "SELECT point_id,airline,flt_no,airp,scd_out,pr_reg FROM points WHERE move_id=:move_id AND pr_del!=-1 ORDER BY point_num";
	Qry.CreateVariable( "move_id", otInteger, move_id );
	Qry.Execute();
	string prior_airline, prior_flt_no, prior_date, str_d, prior_name, name, dests;
	while ( !Qry.Eof ) {
    if ( Qry.FieldAsInteger( "pr_reg" ) && !Qry.FieldIsNULL( "scd_out" ) ) {
       TTripStages ts( Qry.FieldAsInteger( "point_id" ) );
       if ( ts.getStage( stCheckIn ) != sNoActive )
         throw AstraLocale::UserException( "MSG.FLIGHT.UNABLE_DEL.STATUS_ACTIVE", LParams() << LParam("airp", ElemIdToNameLong(etAirp,Qry.FieldAsString("airp"))));
    }
	  if ( !prior_name.empty() ) {
      if ( !name.empty() )
        name += "/";
      name +=  prior_name;
      prior_name.clear();
    }
		if ( prior_airline.empty() || prior_airline != Qry.FieldAsString( "airline" ) ) {
			prior_airline = Qry.FieldAsString( "airline" );
			prior_name += prior_airline;
			prior_flt_no = Qry.FieldAsString( "flt_no" );
			prior_name += prior_flt_no;
		}
		if ( prior_flt_no.empty() || prior_flt_no != Qry.FieldAsString( "flt_no" ) ) {
		  prior_flt_no = Qry.FieldAsString( "flt_no" );
		  prior_name += prior_flt_no;
		}
		str_d.clear();
		if ( !Qry.FieldIsNULL( "scd_out" ) ) {
      str_d = DateTimeToStr( Qry.FieldAsDateTime( "scd_out" ), "dd.mm" );
    }
    if ( prior_date.empty() || prior_date != str_d ) {
      prior_date = str_d;
      prior_name += " ";
      prior_name += prior_date;
    }
		if ( !dests.empty() )
		 dests += "-";
		dests += Qry.FieldAsString( "airp" );
		Qry.Next();
	}
	
  for(vector<TSOPPTrip>::iterator j=trs.begin(); j!=trs.end(); j++ ) {
  	if ( j->places_out.size() ) { // ३� �� �� �⬥���
  		TSOPPTrip tr;
  		BitSet<TSOPPTripChange> FltChange;
  		FltChange.setFlag( tsDelete );
  	  ChangeTrip( j->point_id, tr, *j, FltChange );  // ३� �� �뫥� 㤠���
  	}
  }
  
	Qry.Clear();
	Qry.SQLText = "UPDATE points SET pr_del=-1 WHERE move_id=:move_id";
	Qry.CreateVariable( "move_id", otInteger, move_id );
	Qry.Execute();
  TReqInfo::Instance()->MsgToLog( "���� " + name + " �������(" + dests + ") 㤠���", evtDisp, move_id );
  ReBindTlgs( move_id, dests_del );
}


void SoppInterface::GetReportForm(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    get_compatible_report_form(NodeAsString("name", reqNode), reqNode, resNode);
    STAT::set_variables(resNode);
}


//////////////////////////////////////////////////////////////////
void createSOPPTrip( int point_id, TSOPPTrips &trips )
{
	internal_ReadData( trips, NoExists, NoExists, false, tSPPCEK, point_id );
}

void ChangeACT_OUT( int point_id, TDateTime old_act, TDateTime act )
{
  if ( act != NoExists )
  {
    //��������� 䠪��᪮�� �६��� �뫥�
    try
    {
      vector<string> tlg_types;
      tlg_types.push_back("MVTA");
      TelegramInterface::SendTlg(point_id,tlg_types);
    }
    catch(std::exception &E)
    {
      ProgError(STDLOG,"ChangeACT_OUT.SendTlg (point_id=%d): %s",point_id,E.what());
    };
  };
  if ( old_act != NoExists && act == NoExists )
  {
    //�⬥�� �뫥�
    try
    {
      TQuery Qry(&OraSession);
  	  Qry.SQLText =
  	    "UPDATE trip_sets SET pr_etstatus=0,et_final_attempt=0 WHERE point_id=:point_id";
      Qry.CreateVariable("point_id",otInteger,point_id);
      Qry.Execute();
    }
    catch(std::exception &E)
    {
      ProgError(STDLOG,"ChangeACT_OUT.ETStatus (point_id=%d): %s",point_id,E.what());
    };
  };
}

void ChangeACT_IN( int point_id, TDateTime old_act, TDateTime act )
{
  if ( act != NoExists )
  {
    //��������� 䠪��᪮�� �६��� �ਫ��
    try
    {
      //⥫��ࠬ�� �� �ਫ��
      TTripRoute route;
      TTripRouteItem prior_airp;
      route.GetPriorAirp(NoExists, point_id, trtNotCancelled, prior_airp);
      if (prior_airp.point_id!=NoExists)
  	  {
        vector<string> tlg_types;
        tlg_types.push_back("MVTB");
        TelegramInterface::SendTlg(prior_airp.point_id,tlg_types);
      };
    }
    catch(std::exception &E)
    {
      ProgError(STDLOG,"ChangeACT_IN.SendTlg (point_id=%d): %s",point_id,E.what());
    };
  };
}

void ChangeTrip( int point_id, TSOPPTrip tr1, TSOPPTrip tr2, BitSet<TSOPPTripChange> FltChange )
{
  ProgTrace( TRACE5, "point_id=%d", point_id );
  string flags;
  if ( FltChange.isFlag( tsNew ) )
  	flags += " tsNew";
  if ( FltChange.isFlag( tsDelete ) )
  	flags += " tsDelete";
  if ( FltChange.isFlag( tsAttr ) )
  	flags += " tsAttr";
  if ( FltChange.isFlag( tsAddFltOut ) )
  	flags += " tsAddFltOut";
  if ( FltChange.isFlag( tsDelFltOut ) )
  	flags += " tsDelFltOut";
  if ( FltChange.isFlag( tsCancelFltOut ) )
  	flags += " tsCancelFltOut";
  if ( FltChange.isFlag( tsRestoreFltOut ) )
  	flags += " tsRestoreFltOut";
  ProgTrace( TRACE5, "flag=%s", flags.c_str() );
  if ( (FltChange.isFlag( tsNew ) ||
  	    FltChange.isFlag( tsAddFltOut ) ||
  	    FltChange.isFlag( tsAttr ) )/* && !FltChange.isFlag( tsCancelFltOut ) ||

  	    FltChange.isFlag( tsRestoreFltOut )*/ ) { // ����⠭������� (����)
    if ( tr1.pr_del_out != -1 &&
         tr1.scd_out != NoExists && tr1.flt_no_out != NoExists && !tr1.airline_out.empty() ) {
      try {
        TDateTime locale_scd_out = UTCToLocal( tr1.scd_out, tr1.region );
        bindingAODBFlt( tr1.airline_out, tr1.flt_no_out, tr1.suffix_out, locale_scd_out, tr1.airp );
      }
      catch(std::exception &E) {
        ProgError(STDLOG,"BindAODBFlt: point_id=%d, %s",tr1.point_id,E.what());
      };
    }
  	ProgTrace( TRACE5, "point_id=%d,airline=%s, flt_no=%d, suffix=%s, scd_out=%f, airp=%s, pr_del=%d",
  	           tr1.point_id, tr1.airline_out.c_str(), tr1.flt_no_out, tr1.suffix_out.c_str(), tr1.scd_out, tr1.airp.c_str(), tr1.pr_del_out );
  }
}



void SoppInterface::ReadCrew(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
	int point_id = NodeAsInteger( "point_id", reqNode );
	TQuery Qry(&OraSession);
	Qry.SQLText =
	  "SELECT commander, cockpit, cabin "
	  "FROM trip_crew WHERE point_id=:point_id";
	Qry.CreateVariable( "point_id", otInteger, point_id );
	Qry.Execute();
	xmlNodePtr dataNode = NewTextChild( NewTextChild( resNode, "data" ), "crew" );
  NewTextChild( dataNode, "commander" );
  NewTextChild( dataNode, "cockpit" );
  NewTextChild( dataNode, "cabin" );

	if ( !Qry.Eof )
	{
		ReplaceTextChild( dataNode, "commander", Qry.FieldAsString( "commander" ) );
		if (!Qry.FieldIsNULL("cockpit"))
		  ReplaceTextChild( dataNode, "cockpit", Qry.FieldAsInteger( "cockpit" ) );
		if (!Qry.FieldIsNULL("cabin"))
		  ReplaceTextChild( dataNode, "cabin", Qry.FieldAsInteger( "cabin" ) );
  };
}

void SoppInterface::WriteCrew(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
	TQuery Qry(&OraSession);
	Qry.SQLText =
	  "BEGIN "
	  "  UPDATE trip_crew "
	  "  SET commander=SUBSTR(:commander,1,100), cockpit=:cockpit, cabin=:cabin "
	  "  WHERE point_id=:point_id; "
	  "  IF SQL%NOTFOUND THEN "
	  "    INSERT INTO trip_crew(point_id, commander, cockpit, cabin) "
	  "    VALUES(:point_id, SUBSTR(:commander,1,100), :cockpit, :cabin); "
	  "  END IF;"
	  "END;";
	xmlNodePtr dataNode = NodeAsNode( "data/crew", reqNode );
	Qry.CreateVariable( "point_id", otInteger, NodeAsInteger( "point_id", dataNode ) );
	Qry.CreateVariable( "commander", otString, NodeAsString( "commander", dataNode ) );
	if (GetNode( "cockpit", dataNode )!=NULL && !NodeIsNULL( "cockpit", dataNode ))
	  Qry.CreateVariable( "cockpit", otInteger, NodeAsInteger( "cockpit", dataNode ) );
	else
	  Qry.CreateVariable( "cockpit", otInteger, FNull );
	if (GetNode( "cabin", dataNode )!=NULL && !NodeIsNULL( "cabin", dataNode ))
	  Qry.CreateVariable( "cabin", otInteger, NodeAsInteger( "cabin", dataNode ) );
	else
	  Qry.CreateVariable( "cabin", otInteger, FNull );
	Qry.Execute();
}

void SoppInterface::GetTime(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
	TQuery Qry(&OraSession);
	Qry.SQLText = "SELECT system.UTCSYSDATE time FROM dual";
	Qry.Execute();
	TElemFmt fmt;
	string airp = ElemToElemId( etAirp, NodeAsString( "airp", reqNode ), fmt, true);
	if ( fmt == efmtUnknown )
		throw AstraLocale::UserException( "MSG.AIRP.INVALID_GIVEN_CODE" );
	string region = AirpTZRegion( airp, true );
 	TDateTime time = UTCToClient( Qry.FieldAsDateTime( "time" ), region );
	NewTextChild( resNode, "time", DateTimeToStr( time, ServerFormatDateTimeAsString ) );
}

bool trip_calc_data( int point_id, BitSet<TTrip_Calc_Data> &whatcalc,
                     bool &trfer_exists, string &ckin_desks, string &gates )
{
	if ( point_id == ASTRA::NoExists )
    throw Exception( "MSG.FLIGHT.NOT_FOUND" );
  TQuery Qry(&OraSession);
  Qry.SQLText =
    "SELECT trfer_exists, ckin_desks, gates FROM trip_calc_data "
    " WHERE point_id=:point_id";
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.Execute();
  bool pr_empty = Qry.Eof;
  if ( !pr_empty ) {
    trfer_exists = Qry.FieldAsInteger( "trfer_exists" );
    ckin_desks = Qry.FieldAsString( "ckin_desks" );
    gates = Qry.FieldAsString( "gates" );
  }

  bool new_trfer_exists;
  string new_ckin_desks;
  string new_gates;
  Qry.Clear();
  if ( pr_empty || whatcalc.isFlag( tDesksGates ) ) {
    Qry.SQLText =
      "SELECT stations.name,stations.work_mode FROM stations,trip_stations "
      " WHERE point_id=:point_id AND stations.desk=trip_stations.desk AND stations.work_mode=trip_stations.work_mode "
      " ORDER BY stations.work_mode,stations.name";
    Qry.CreateVariable( "point_id", otInteger, point_id );
    Qry.Execute();
    int col_name = Qry.FieldIndex( "name" );
	  int col_work_mode = Qry.FieldIndex( "work_mode" );

    while ( !Qry.Eof ) {
      if ( string( "�" ) == Qry.FieldAsString( col_work_mode ) ) {
        if ( !new_ckin_desks.empty() )
          new_ckin_desks += " ";
        new_ckin_desks += Qry.FieldAsString( col_name );
      }
      else {
        if ( !new_gates.empty() )
          new_gates += " ";
        new_gates += Qry.FieldAsString( col_name );
      }
      Qry.Next();
    }
  }
  if ( pr_empty || whatcalc.isFlag( tTrferExists ) ) {
    Qry.SQLText =
      "SELECT 1 FROM transfer, pax_grp "
      "WHERE pax_grp.point_dep=:point_id AND transfer.grp_id=pax_grp.grp_id AND bag_refuse=0 AND rownum<2";
    Qry.CreateVariable( "point_id", otInteger, point_id );
    Qry.Execute();
    new_trfer_exists = !Qry.Eof;
  }
  bool pr_update = false;
  if ( !pr_empty ) {
    if ( whatcalc.isFlag( tDesksGates ) &&
         ( ckin_desks != new_ckin_desks ||
           gates != new_gates  ) )
      pr_update = true;
    if ( whatcalc.isFlag( tTrferExists ) &&
         trfer_exists != new_trfer_exists )
      pr_update = true;
  }
  if ( !pr_empty && !pr_update )
    return false;
    
  if ( pr_empty || whatcalc.isFlag( tDesksGates ) ) {
    ckin_desks = new_ckin_desks;
    gates = new_gates;
  }
  if ( pr_empty || whatcalc.isFlag( tTrferExists ) ) {
    trfer_exists = new_trfer_exists;
  }
  Qry.Clear();
  if ( pr_empty ) {
    Qry.SQLText =
      "BEGIN "
      " INSERT INTO trip_calc_data(point_id,trfer_exists,ckin_desks,gates) "
      "  VALUES(:point_id,:trfer_exists,:ckin_desks,:gates); "
      "EXCEPTION WHEN DUP_VAL_ON_INDEX THEN "
      " UPDATE trip_calc_data "
      "  SET trfer_exists=:trfer_exists, ckin_desks=:ckin_desks, gates=:gates "
      "  WHERE point_id=:point_id; "
      "END;";
  }
  else
    Qry.SQLText =
      "UPDATE trip_calc_data "
      " SET trfer_exists=:trfer_exists, ckin_desks=:ckin_desks, gates=:gates "
      " WHERE point_id=:point_id";
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.CreateVariable( "trfer_exists", otInteger, trfer_exists );
  Qry.CreateVariable( "ckin_desks", otString, ckin_desks );
  Qry.CreateVariable( "gates", otString, gates );
  Qry.Execute();
  return true;
}

void get_DesksGates( int point_id, string &ckin_desks, string &gates )
{
  BitSet<TTrip_Calc_Data> calcType;
  bool trfer_exists;
  trip_calc_data( point_id, calcType, trfer_exists, ckin_desks, gates );
}

void get_DesksGates( int point_id, tstations &stations )
{
  string ckin_desks, gates;
  get_DesksGates( point_id, ckin_desks, gates );
  stations.clear();
  convertStrToStations( ckin_desks, "�", stations );
  convertStrToStations( gates, "�", stations );
}

void get_TrferExists( int point_id, bool &trfer_exists )
{
  BitSet<TTrip_Calc_Data> calcType;
  string ckin_desks, gates;
  trip_calc_data( point_id, calcType, trfer_exists, ckin_desks, gates );
}

void check_DesksGates( int point_id )
{
  BitSet<TTrip_Calc_Data> calcType;
  calcType.setFlag( tDesksGates );
  bool trfer_exists;
  string ckin_desks, gates;
  trip_calc_data( point_id, calcType, trfer_exists, ckin_desks, gates );
}

void check_TrferExists( int point_id )
{
  BitSet<TTrip_Calc_Data> calcType;
  calcType.setFlag( tTrferExists );
  string ckin_desks, gates;
  bool trfer_exists;
  trip_calc_data( point_id, calcType, trfer_exists, ckin_desks, gates );
}


