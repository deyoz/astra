#include <stdlib.h>
#include "sopp.h"
#define NICKNAME "DJEK"
#include "setup.h"
#include "test.h"
#include "stages.h"
#include "astra_utils.h"
#include "stl_utils.h"
#include "oralib.h"
#include "xml_unit.h"
#include "basic.h"
#include "exceptions.h"
#include "sys/times.h"
#include <map>
#include <vector>
#include <string>
#include "season.h" //???
#include "tripinfo.h"
#include "telegram.h"
#include "boost/date_time/local_time/local_time.hpp"
#include "base_tables.h"
#include "docs.h"
#include "stat.h"


#include "perfom.h"

//#include "flight_cent_dbf.h" //!!!

using namespace std;
using namespace BASIC;
using namespace EXCEPTIONS;
using namespace ASTRA;
using namespace boost::local_time;

const char* points_SOPP_SQL =
    "SELECT points.move_id,points.point_id,point_num,airp,airp_fmt,first_point,airline,airline_fmt,flt_no,"
    "       suffix,suffix_fmt,craft,craft_fmt,bort,"
    "       scd_in,est_in,act_in,scd_out,est_out,act_out,trip_type,litera,park_in,park_out,remark,"
    "       pr_tranzit,pr_reg,points.pr_del pr_del,points.tid tid "
    " FROM points, "
    " (SELECT DISTINCT move_id FROM points "
    "   WHERE points.pr_del!=-1 AND "
    "         ( :first_date IS NULL OR "
    "           NVL(act_in,NVL(est_in,scd_in)) >= :first_date AND NVL(act_in,NVL(est_in,scd_in)) < :next_date OR "
    "           NVL(act_out,NVL(est_out,scd_out)) >= :first_date AND NVL(act_out,NVL(est_out,scd_out)) < :next_date ) ) p "
    "WHERE points.move_id = p.move_id AND "
    "      points.pr_del!=-1 "
    "ORDER BY points.move_id,point_num,point_id ";
const char* points_id_SOPP_SQL =
    "SELECT points.move_id,points.point_id,point_num,airp,airp_fmt,first_point,airline,airline_fmt,flt_no,"
    "       suffix,suffix_fmt,craft,craft_fmt,bort,"
    "       scd_in,est_in,act_in,scd_out,est_out,act_out,trip_type,litera,park_in,park_out,remark,"
    "       pr_tranzit,pr_reg,points.pr_del pr_del,points.tid tid "
    " FROM points, "
    " ( SELECT move_id FROM points WHERE point_id=:point_id AND pr_del!=-1 AND ROWNUM<2 ) a "
    "   WHERE pr_del!=-1 AND points.move_id=a.move_id "
    "ORDER BY points.move_id,point_num,point_id ";
const char* points_ISG_SQL =
    "SELECT points.move_id,points.point_id,point_num,airp,airp_fmt,first_point,airline,airline_fmt,flt_no,"
    "       suffix,suffix_fmt,craft,craft_fmt,bort,"
    "       scd_in,est_in,act_in,scd_out,est_out,act_out,trip_type,litera,park_in,park_out,remark,"
    "       pr_tranzit,pr_reg,points.pr_del pr_del,points.tid tid, reference ref "
    " FROM points, move_ref, "
    " (SELECT DISTINCT move_id FROM points "
    "   WHERE points.pr_del!=-1 AND "
    "         ( :first_date IS NULL OR "
    "           NVL(act_in,NVL(est_in,scd_in)) >= :first_date AND NVL(act_in,NVL(est_in,scd_in)) < :next_date OR "
    "           NVL(act_out,NVL(est_out,scd_out)) >= :first_date AND NVL(act_out,NVL(est_out,scd_out)) < :next_date OR "
    "            NVL(act_in,NVL(est_in,scd_in)) IS NULL AND NVL(act_out,NVL(est_out,scd_out)) IS NULL ) ) p "
    "WHERE points.move_id = p.move_id AND "
    "      move_ref.move_id = p.move_id AND "
    "      points.pr_del!=-1 "
    "ORDER BY points.move_id,point_num,point_id ";
const char * arx_points_SOPP_SQL =
    "SELECT arx_points.move_id,point_id,point_num,airp,airp_fmt,first_point,airline,airline_fmt,flt_no,"
    "       suffix,suffix_fmt,craft,craft_fmt,bort,"
    "       scd_in,est_in,act_in,scd_out,est_out,act_out,trip_type,litera,park_in,park_out,remark,"
    "       pr_tranzit,pr_reg,arx_points.pr_del pr_del,arx_points.tid tid "
    " FROM arx_points,"
    " (SELECT DISTINCT move_id FROM arx_points "
    "   WHERE part_key>=:first_date AND "
    "         pr_del!=-1 AND "
    "         ( :first_date IS NULL OR "
    "           NVL(act_in,NVL(est_in,scd_in)) >= :first_date AND NVL(act_in,NVL(est_in,scd_in)) < :next_date OR "
    "           NVL(act_out,NVL(est_out,scd_out)) >= :first_date AND NVL(act_out,NVL(est_out,scd_out)) < :next_date ) ) p "
    "WHERE arx_points.move_id = p.move_id AND "
    "      arx_points.pr_del!=-1 "
    "ORDER BY arx_points.move_id,point_num,point_id ";
const char * arx_points_ISG_SQL =
    "SELECT arx_points.move_id,point_id,point_num,airp,airp_fmt,first_point,airline,airline_fmt,flt_no,"
    "       suffix,suffix_fmt,craft,craft_fmt,bort,"
    "       scd_in,est_in,act_in,scd_out,est_out,act_out,trip_type,litera,park_in,park_out,remark,"
    "       pr_tranzit,pr_reg,arx_points.pr_del pr_del,arx_points.tid tid, reference ref "
    " FROM arx_points, arx_move_ref,"
    " (SELECT DISTINCT move_id FROM arx_points "
    "   WHERE part_key>=:first_date AND "
    "         pr_del!=-1 AND "
    "         ( :first_date IS NULL OR "
    "           NVL(act_in,NVL(est_in,scd_in)) >= :first_date AND NVL(act_in,NVL(est_in,scd_in)) < :next_date OR "
    "           NVL(act_out,NVL(est_out,scd_out)) >= :first_date AND NVL(act_out,NVL(est_out,scd_out)) < :next_date OR "
    "           NVL(act_in,NVL(est_in,scd_in)) IS NULL AND NVL(act_out,NVL(est_out,scd_out)) IS NULL ) ) p "
    "WHERE arx_points.move_id = p.move_id AND "
    "      arx_move_ref.move_id = p.move_id AND "
    "      arx_points.pr_del!=-1 "
    "ORDER BY arx_points.move_id,point_num,point_id ";
const char* classesSQL =
    "SELECT class,cfg "\
    " FROM trip_classes,classes "\
    "WHERE trip_classes.point_id=:point_id AND trip_classes.class=classes.code "\
    "ORDER BY priority";

const char* arx_classesSQL =
    "SELECT class,cfg "\
    " FROM arx_trip_classes,classes "\
    "WHERE part_key>=:arx_date AND arx_trip_classes.point_id=:point_id AND arx_trip_classes.class=classes.code "\
    "ORDER BY priority";
const char* regSQL =
    "SELECT SUM(pax.seats) as reg FROM pax_grp, pax "\
    " WHERE pax_grp.point_dep=:point_id AND pax_grp.grp_id=pax.grp_id AND pax.pr_brd IS NOT NULL ";
const char* arx_regSQL =
    "SELECT SUM(arx_pax.seats) as reg "
    " FROM arx_pax_grp, arx_pax "\
    " WHERE arx_pax_grp.part_key>=:arx_date AND arx_pax_grp.point_dep=:point_id AND "\
    "       arx_pax.part_key>=:arx_date AND "
    "       arx_pax_grp.grp_id=arx_pax.grp_id AND arx_pax.pr_brd IS NOT NULL ";
const char* resaSQL =
    "SELECT ckin.get_crs_ok(:point_id) as resa FROM dual ";
const char *stagesSQL =
    "SELECT stage_id,scd,est,act,pr_auto,pr_manual FROM trip_stages "
    " WHERE point_id=:point_id "
    " ORDER BY stage_id ";
const char *arx_stagesSQL =
    "SELECT stage_id,scd,est,act,pr_auto,pr_manual FROM arx_trip_stages "
    " WHERE part_key>=:arx_date AND point_id=:point_id "
    " ORDER BY stage_id ";
const char* stationsSQL =
    "SELECT stations.name,stations.work_mode,trip_stations.pr_main FROM stations,trip_stations "\
    " WHERE point_id=:point_id AND stations.desk=trip_stations.desk AND stations.work_mode=trip_stations.work_mode "\
    " ORDER BY stations.work_mode,stations.name";

const char* trfer_out_SQL =
    "SELECT 1 "
    " FROM tlg_binding,tlg_transfer "
    "WHERE tlg_binding.point_id_spp=:point_id AND tlg_binding.point_id_tlg=tlg_transfer.point_id_in AND rownum<2";

const char* trfer_in_SQL =
    "SELECT 1 "
    " FROM tlg_binding,tlg_transfer "
    "WHERE tlg_binding.point_id_spp=:point_id AND tlg_binding.point_id_tlg=tlg_transfer.point_id_out AND rownum<2";
const char* trfer_reg_SQL =
    "SELECT 1 FROM transfer, pax_grp "
    "WHERE pax_grp.point_dep=:point_id AND transfer.grp_id=pax_grp.grp_id AND bag_refuse=0 AND rownum<2";
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
  "       points.point_id=crs_displace2.point_id_spp "
  "ORDER BY points.point_id,points.airline,points.flt_no,points.suffix,points.scd_out ";
const char* arx_trip_delays_SQL =
  "SELECT delay_num,delay_code,time "
  " FROM arx_trip_delays "
  "WHERE arx_trip_delays.part_key>=:arx_date AND arx_trip_delays.point_id=:point_id "
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


void read_tripStages( vector<TSoppStage> &stages, bool arx, TDateTime first_date, int point_id );
void build_TripStages( const vector<TSoppStage> &stages, const string &region, xmlNodePtr tripNode, bool pr_isg );
string getCrsDisplace( int point_id, TDateTime local_time, bool to_local, TQuery &Qry );

void ChangeACT_OUT( int point_id, TDateTime old_act, TDateTime act );
void ChangeACT_IN( int point_id, TDateTime old_act, TDateTime act );


string GetRemark( string remark, TDateTime scd_out, TDateTime est_out, string region )
{
	string rem = remark;
	if ( est_out > NoExists && scd_out != est_out ) {
		rem = string( "����প� �� " ) + DateTimeToStr( UTCToClient( est_out, region ), "dd hh:nn" ) + remark;
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
  }

  trip.airp = id->airp;
  trip.airp_fmt = id->airp_fmt;
  trip.city = id->city;
  trip.pr_del = id->pr_del;

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
    catch(...) { ProgError( STDLOG, "id->point_id=%d", id->point_id ); };

    trip.pr_del_out = id->pr_del;
    trip.pr_reg = id->pr_reg;
  }
  trip.region = id->region;
  trip.delays = id->delays;
  return trip;
}

bool FilterFlightDate( TSOPPTrip &tr, TDateTime first_date, TDateTime next_date, bool LocalAll,
                       string &errcity, bool pr_isg )
{
  if ( LocalAll && first_date > NoExists ) {
    bool canuseTR = false;
    TDateTime d;
    if ( tr.act_in > NoExists ) {
    	try {
        d = UTCToClient( tr.act_in, tr.region );
      }
      catch( Exception &e ) {
      	if ( errcity.empty() )
      	  errcity = tr.city;
        ProgError( STDLOG, "Exception: %s, point_id=%d", e.what(), tr.point_id );
        return false;
      }
      canuseTR = ( d >= first_date && d < next_date );
    }
    else
      if ( tr.est_in > NoExists ) {
      	try {
          d = UTCToClient( tr.est_in, tr.region );
        }
        catch( Exception &e ) {
        	if ( errcity.empty() )
        	  errcity = tr.city;
          ProgError( STDLOG, "Exception: %s, point_id=%d", e.what(), tr.point_id );
          return false;
        }
        canuseTR = ( d >= first_date && d < next_date );
      }
      else
        if ( tr.scd_in > NoExists ) {
        	try {
            d = UTCToClient( tr.scd_in, tr.region );
          }
          catch( Exception &e ) {
          	if ( errcity.empty() )
          	  errcity = tr.city;
            ProgError( STDLOG, "Exception: %s, point_id=%d", e.what(), tr.point_id );
            return false;
          }
          canuseTR = ( d >= first_date && d < next_date );
        }
    if ( !canuseTR )
      if ( tr.act_out > NoExists ) {
      	try {
          d = UTCToClient( tr.act_out, tr.region );
        }
        catch( Exception &e ) {
        	if ( errcity.empty() )
        	  errcity = tr.city;
          ProgError( STDLOG, "Exception: %s, point_id=%d", e.what(), tr.point_id );
          return false;
        }
        canuseTR = ( d >= first_date && d < next_date );
      }
      else
        if ( tr.est_out > NoExists ) {
        	try {
            d = UTCToClient( tr.est_out, tr.region );
          }
          catch( Exception &e ) {
           	if ( errcity.empty() )
          	  errcity = tr.city;
            ProgError( STDLOG, "Exception: %s, point_id=%d", e.what(), tr.point_id );
            return false;
          }
          canuseTR = ( d >= first_date && d < next_date );
        }
        else
          if ( tr.scd_out > NoExists ) {
          	try {
              d = UTCToClient( tr.scd_out, tr.region );
            }
            catch( Exception &e ) {
             	if ( errcity.empty() )
      	        errcity = tr.city;
              ProgError( STDLOG, "Exception: %s, point_id=%d", e.what(), tr.point_id );
              return false;
            }
            canuseTR = ( d >= first_date && d < next_date );
          }
         else canuseTR = pr_isg;
    return canuseTR;
  }
  return true;
}

void read_TripStages( vector<TSoppStage> &stages, bool arx, TDateTime first_date, int point_id )
{
	stages.clear();
  TQuery StagesQry( &OraSession );
  if ( arx ) {
  	StagesQry.SQLText = arx_stagesSQL;
  	StagesQry.CreateVariable( "arx_date", otDate, first_date - 2 );
  }
  else
    StagesQry.SQLText = stagesSQL;
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

    stages.push_back( stage );
    StagesQry.Next();
  }
}

void build_TripStages( const vector<TSoppStage> &stages, const string &region, xmlNodePtr tripNode, bool pr_isg )
{
  xmlNodePtr lNode = NULL;
  for ( tstages::const_iterator st=stages.begin(); st!=stages.end(); st++ ) {
  	if ( pr_isg && st->stage_id != sRemovalGangWay )
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
    NewTextChild( stageNode, "pr_auto", st->pr_auto );
    NewTextChild( stageNode, "pr_manual", st->pr_manual );
  }
}

string internal_ReadData( TSOPPTrips &trips, TDateTime first_date, TDateTime next_date,
                          bool arx, bool pr_isg, int point_id = NoExists )
{
	string errcity;
	TReqInfo *reqInfo = TReqInfo::Instance();
  TQuery PointsQry( &OraSession );
  TBaseTable &airps = base_tables.get( "airps" );
  TBaseTable &cities = base_tables.get( "cities" );
  if ( arx )
  	if ( pr_isg )
  		PointsQry.SQLText = arx_points_ISG_SQL;
  	else
  	  PointsQry.SQLText = arx_points_SOPP_SQL;
  else
  	if ( pr_isg )
  	  PointsQry.SQLText = points_ISG_SQL;
  	else
  		if ( point_id == NoExists )
	  		PointsQry.SQLText = points_SOPP_SQL;
  		else {
  			PointsQry.SQLText = points_id_SOPP_SQL;
  			PointsQry.CreateVariable( "point_id", otInteger, point_id );
  	  }
  if ( point_id == NoExists ) {
    if ( first_date != NoExists ) {
      if ( reqInfo->user.sets.time == ustTimeLocalAirp ) {
      	// �뢠�� ᤢ�� 25 �ᮢ ???
        PointsQry.CreateVariable( "first_date", otDate, first_date - 2 );
        PointsQry.CreateVariable( "next_date", otDate, next_date + 2 );
      }
      else {
        PointsQry.CreateVariable( "first_date", otDate, first_date );
        PointsQry.CreateVariable( "next_date", otDate, next_date );
      }
    }
    else {
    	PointsQry.CreateVariable( "first_date", otDate, FNull );
    	PointsQry.CreateVariable( "next_date", otDate, FNull );
    }
  }

  TQuery ClassesQry( &OraSession );
  if ( arx ) {
  	ClassesQry.SQLText = arx_classesSQL;
  	ClassesQry.CreateVariable( "arx_date" ,otDate, first_date - 2 );
  }
  else
    ClassesQry.SQLText = classesSQL;
  ClassesQry.DeclareVariable( "point_id", otInteger );
  TQuery RegQry( &OraSession );
  if ( arx ) {
  	RegQry.SQLText = arx_regSQL;
  	RegQry.CreateVariable( "arx_date", otDate, first_date - 2 );
  }
  else
  	RegQry.SQLText = regSQL;
  RegQry.DeclareVariable( "point_id", otInteger );
  TQuery ResaQry( &OraSession );
  if ( !arx ) {
    ResaQry.SQLText = resaSQL;
    ResaQry.DeclareVariable( "point_id", otInteger );
  }
  TQuery StationsQry( &OraSession );
  if ( !arx ) {
    StationsQry.SQLText = stationsSQL;
    StationsQry.DeclareVariable( "point_id", otInteger );
  }
  TQuery Trfer_outQry( &OraSession );
  TQuery Trfer_inQry( &OraSession );
  TQuery Trfer_regQry( &OraSession );
  if ( !arx ) {
  	Trfer_outQry.SQLText = trfer_out_SQL;
  	Trfer_outQry.DeclareVariable( "point_id", otInteger );
  	Trfer_inQry.SQLText = trfer_in_SQL;
  	Trfer_inQry.DeclareVariable( "point_id", otInteger );
  	Trfer_regQry.SQLText = trfer_reg_SQL;
  	Trfer_regQry.DeclareVariable( "point_id", otInteger );
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
  if ( pr_isg ) {
    if ( arx ) {
      DelaysQry.SQLText = arx_trip_delays_SQL;
      DelaysQry.CreateVariable( "arx_date", otDate, first_date - 2 );
    }
    else {
      DelaysQry.SQLText = trip_delays_SQL;
    }
    DelaysQry.DeclareVariable( "point_id", otInteger );
  }
  PerfomTest( 666 );
  PointsQry.Execute();
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
  if ( pr_isg )
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
	int col_name = -1;
	int col_work_mode = -1;
	int col_pr_main = -1;
  while ( !PointsQry.Eof ) {
    if ( move_id != PointsQry.FieldAsInteger( col_move_id ) ) {
      if ( move_id > NoExists && dests.size() > 1 ) {
        //create trips
        string airline;
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
            TSOPPTrip tr = createTrip( move_id, id, dests );
            tr.ref = ref;
            if ( FilterFlightDate( tr, first_date, next_date, reqInfo->user.sets.time == ustTimeLocalAirp,
            	                     errcity, pr_isg ) ) {
              trips.push_back( tr );
            }
          }
        }
      }
      move_id = PointsQry.FieldAsInteger( col_move_id );
      if ( pr_isg )
        ref = PointsQry.FieldAsString( col_ref );
      dests.clear();
    }
    TSOPPDest d;
    d.point_id = PointsQry.FieldAsInteger( col_point_id );
    d.point_num = PointsQry.FieldAsInteger( col_point_num );

    d.airp = PointsQry.FieldAsString( col_airp );
    d.airp_fmt = PointsQry.FieldAsInteger( col_airp_fmt );
    d.city = ((TAirpsRow&)airps.get_row( "code", d.airp )).city;

    if ( PointsQry.FieldIsNULL( col_first_point ) )
      d.first_point = NoExists;
    else
      d.first_point = PointsQry.FieldAsInteger( col_first_point );
    d.airline = PointsQry.FieldAsString( col_airline );
    d.airline_fmt = PointsQry.FieldAsInteger( col_airline_fmt );
    if ( PointsQry.FieldIsNULL( col_flt_no ) )
      d.flt_no = NoExists;
    else
      d.flt_no = PointsQry.FieldAsInteger( col_flt_no );
    d.suffix = PointsQry.FieldAsString( col_suffix );
    d.suffix_fmt = PointsQry.FieldAsInteger( col_suffix_fmt );
    d.craft = PointsQry.FieldAsString( col_craft );
    d.craft_fmt = PointsQry.FieldAsInteger( col_craft_fmt );
    d.bort = PointsQry.FieldAsString( col_bort );
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
    d.region = ((TCitiesRow&)cities.get_row( "code", d.city )).region;

    if ( pr_isg ) {
   	  DelaysQry.SetVariable( "point_id", d.point_id );
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
    for( TSOPPDests::iterator id=dests.begin(); id!=dests.end(); id++ ) {
      if ( id != dests.end() - 1 )
        airline = id->airline;
      else {
        TSOPPDests::iterator f = id;
        f--;
        airline = f->airline;
      }
      if ( reqInfo->CheckAirline( airline ) &&
        	 reqInfo->CheckAirp( id->airp ) )
      	/*(!reqInfo->user.access.airlines.empty() &&
             find( reqInfo->user.access.airlines.begin(),
                   reqInfo->user.access.airlines.end(),
                   airline
                 ) != reqInfo->user.access.airlines.end() ||
             reqInfo->user.access.airlines.empty() && reqInfo->user.user_type != utAirline) &&

            (!reqInfo->user.access.airps.empty() &&
             find( reqInfo->user.access.airps.begin(),
                   reqInfo->user.access.airps.end(),
                   id->airp
                 ) != reqInfo->user.access.airps.end() ||
             reqInfo->user.access.airps.empty() && reqInfo->user.user_type != utAirport) )*/ {
         TSOPPTrip tr = createTrip( move_id, id, dests );
         tr.ref = ref;
         if ( FilterFlightDate( tr, first_date, next_date, reqInfo->user.sets.time == ustTimeLocalAirp,
         	                      errcity, pr_isg ) ) {
           trips.push_back( tr );
         }
      }
    }
  }
  // ३�� ᮧ����, ��३��� � ������ ���ଠ樨 �� ३ᠬ
  ////////////////////////// crs_displaces ///////////////////////////////
  PerfomTest( 669 );

  for ( TSOPPTrips::iterator tr=trips.begin(); tr!=trips.end(); tr++ ) {
    if ( !tr->places_out.empty() ) {
      // ����� ���ଠ樨
      if ( !pr_isg ) {
        ClassesQry.SetVariable( "point_id", tr->point_id );
        ClassesQry.Execute();
        while ( !ClassesQry.Eof ) {
        	if ( col_class == -1 ) {
        		col_class = ClassesQry.FieldIndex( "class" );
        		col_cfg = ClassesQry.FieldIndex( "cfg" );
        	}
          if ( !tr->classes.empty() && point_id == NoExists )
            tr->classes += " ";
          tr->classes += ClassesQry.FieldAsString( col_class );
          if ( point_id == NoExists )
           tr->classes += string(ClassesQry.FieldAsString( col_cfg ));
          ClassesQry.Next();
        }
      } // !pr_isg
      if ( point_id != NoExists )
      	continue;

      if ( !pr_isg ) {
        ///////////////////////////// reg /////////////////////////
        RegQry.SetVariable( "point_id", tr->point_id );
        RegQry.Execute();
        if ( !RegQry.Eof ) {
          tr->reg = RegQry.FieldAsInteger( "reg" );
        }
        ///////////////////////// resa ///////////////////////////
        if ( !arx ) {
        	ResaQry.SetVariable( "point_id", tr->point_id );
        	ResaQry.Execute();
          if ( !ResaQry.Eof )
            tr->resa = ResaQry.FieldAsInteger( "resa" );
        }
        ////////////////////// trfer  ///////////////////////////////
        if ( !arx ) {
      		Trfer_inQry.SetVariable( "point_id", tr->point_id );
       		Trfer_inQry.Execute();
          if ( !Trfer_inQry.Eof )
          	tr->TrferType.setFlag( trferOut );
    		  Trfer_regQry.SetVariable( "point_id", tr->point_id );
     		  Trfer_regQry.Execute();
          if ( !Trfer_regQry.Eof )
          	tr->TrferType.setFlag( trferCkin );
        }
      } //!pr_isg
      ////////////////////// stages ///////////////////////////////
      read_TripStages( tr->stages, arx, first_date, tr->point_id );
      ////////////////////////// stations //////////////////////////////
      if ( !arx && !pr_isg ) {
        StationsQry.SetVariable( "point_id", tr->point_id );
        StationsQry.Execute();

        while ( !StationsQry.Eof ) {
        	if ( col_name == -1 ) {
        		col_name = StationsQry.FieldIndex( "name" );
        		col_work_mode = StationsQry.FieldIndex( "work_mode" );
        		col_pr_main = StationsQry.FieldIndex( "pr_main" );
         	}
          TSOPPStation station;
          station.name = StationsQry.FieldAsString( col_name );
          station.work_mode = StationsQry.FieldAsString( col_work_mode );
          station.pr_main = StationsQry.FieldAsInteger( col_pr_main );
          tr->stations.push_back( station );
          StationsQry.Next();
        }
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
   	if ( !arx && !pr_isg && tr->trfer_out_point_id != -1 ) {
   		Trfer_outQry.SetVariable( "point_id", tr->trfer_out_point_id );
   		Trfer_outQry.Execute();
   		if ( !Trfer_outQry.Eof )
   			tr->TrferType.setFlag( trferIn );
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
      NewTextChild( tripNode, "triptype_in", tr->triptype_in );
    if ( tr->litera_in != tr->litera_out && !tr->litera_in.empty() )
      NewTextChild( tripNode, "litera_in", tr->litera_in );
    if ( !tr->park_in.empty() )
      NewTextChild( tripNode, "park_in", tr->park_in );
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
      NewTextChild( tripNode, "triptype_out", tr->triptype_out );
    if ( !tr->litera_out.empty() )
      NewTextChild( tripNode, "litera_out", tr->litera_out );
    if ( !tr->park_out.empty() )
      NewTextChild( tripNode, "park_out", tr->park_out );
    if ( !tr->remark_out.empty() )
      NewTextChild( tripNode, "remark_out", tr->remark_out );
    if ( tr->pr_del_out )
      NewTextChild( tripNode, "pr_del_out", tr->pr_del_out );
//    if ( tr->pr_reg )
      NewTextChild( tripNode, "pr_reg", tr->pr_reg );
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
    if ( !tr->classes.empty() )
      NewTextChild( tripNode, "classes", tr->classes );
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
      NewTextChild( tripNode, "triptype_in", tr->triptype_in );
    if ( tr->litera_in != tr->litera_out && !tr->litera_in.empty() )
      NewTextChild( tripNode, "litera_in", tr->litera_in );
    if ( !tr->park_in.empty() )
      NewTextChild( tripNode, "park_in", tr->park_in );
    if ( tr->remark_in != tr->remark_out && !tr->remark_in.empty() )
      NewTextChild( tripNode, "remark_in", tr->remark_in );
    xmlNodePtr lNode = NULL;
    for ( TSOPPDests::iterator sairp=tr->places_in.begin(); sairp!=tr->places_in.end(); sairp++ ) {
      if ( !lNode )
        lNode = NewTextChild( tripNode, "places_in" );
      xmlNodePtr destNode = NewTextChild( lNode, "dest" );
      NewTextChild( destNode, "point_id", sairp->point_id );
      NewTextChild( destNode, "airp", ElemIdToElemCtxt( ecDisp, etAirp, sairp->airp, sairp->airp_fmt ) );
      if ( sairp->pr_del )
      	NewTextChild( destNode, "pr_del", sairp->pr_del );
      if ( sairp->scd_in > NoExists )
        NewTextChild( destNode, "scd_in", DateTimeToStr( sairp->scd_in, ServerFormatDateTimeAsString ) );
      if ( sairp->est_in > NoExists )
        NewTextChild( destNode, "est_in", DateTimeToStr( sairp->est_in, ServerFormatDateTimeAsString ) );
      if ( sairp->act_in > NoExists )
        NewTextChild( destNode, "scd_in", DateTimeToStr( sairp->act_in, ServerFormatDateTimeAsString ) );
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
  		  NewTextChild( fnode, "delay_code", delay->code );
  		  NewTextChild( fnode, "time", DateTimeToStr( delay->time, ServerFormatDateTimeAsString ) );
      }
    }

    NewTextChild( tripNode, "airp", ElemIdToElemCtxt( ecDisp, etAirp, tr->airp, tr->airp_fmt ) );
    NewTextChild( tripNode, "pr_del", tr->pr_del );
   	dnode = NULL;
   	for ( vector<TSOPPDelay>::iterator delay=tr->delays.begin(); delay!=tr->delays.end(); delay++ ) {
   		ProgTrace( TRACE5, "point_id=%d, delay->code=%s", tr->point_id, delay->code.c_str() );
 	  	if ( !dnode )
 		  	dnode = NewTextChild( tripNode, "delays" );
 		  xmlNodePtr fnode = NewTextChild( dnode, "delay" );
 		  NewTextChild( fnode, "delay_code", delay->code );
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
      NewTextChild( tripNode, "triptype_out", tr->triptype_out );
    if ( !tr->litera_out.empty() )
      NewTextChild( tripNode, "litera_out", tr->litera_out );
    if ( !tr->park_out.empty() )
      NewTextChild( tripNode, "park_out", tr->park_out );
    if ( !tr->remark_out.empty() )
      NewTextChild( tripNode, "remark_out", tr->remark_out );
/*    if ( tr->pr_del_out )
      NewTextChild( tripNode, "pr_del_out", tr->pr_del_out );*/
    NewTextChild( tripNode, "pr_reg", tr->pr_reg );
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
  		  NewTextChild( fnode, "delay_code", delay->code );
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

void SoppInterface::ReadTrips(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
//  createCentringFile( 13672, "ASTRA", "DMDTST" );
  ProgTrace( TRACE5, "ReadTrips" );
  xmlNodePtr dataNode = NewTextChild( resNode, "data" );
  bool pr_isg = GetNode( "disp_isg", reqNode );
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
  	}
  	else
  	  first_date = ClientToUTC( f, TReqInfo::Instance()->desk.tz_region );
    next_date = first_date + 1; // ������塞 ��⪨
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
  string errcity = internal_ReadData( trips, first_date, next_date, arx, pr_isg );
  if ( pr_isg )
  	buildISG( trips, errcity, dataNode );
  else
    buildSOPP( trips, errcity, dataNode );
  if ( !errcity.empty() )
    showErrorMessage( string("��� ��த� ") + errcity + " �� ����� ॣ���. ������� ३�� �� �⮡ࠦ�����" );
}

void SoppInterface::GetTransfer(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode,
                                bool pr_bag)
{
  TQuery Qry(&OraSession);
  TQuery PaxQry(&OraSession);
  TQuery TagQry(&OraSession);

  bool pr_out=NodeAsInteger("pr_out",reqNode)!=0;
  bool pr_tlg=true;
  if (GetNode("pr_tlg",reqNode)!=NULL)
    pr_tlg=NodeAsInteger("pr_tlg",reqNode)!=0;
  int point_id=NodeAsInteger("point_id",reqNode);
  Qry.Clear();
  if (pr_out)
    Qry.SQLText =
      "SELECT point_id,airp,airline,flt_no,suffix,craft,bort,scd_out, "
      "       NVL(act_out,NVL(est_out,scd_out)) AS real_out "
      "FROM points "
      "WHERE point_id=:point_id AND pr_del>=0";

  else
    Qry.SQLText =
      "SELECT p2.point_id,p2.airp,p2.airline,p2.flt_no,p2.suffix,p2.scd_out, "
      "       NVL(p2.act_out,NVL(p2.est_out,p2.scd_out)) AS real_out "
      "FROM points p1,points p2 "
      "WHERE p1.point_id=:point_id AND p1.pr_del>=0 AND "
      "      p1.first_point IN (p2.first_point,p2.point_id) AND "
      "      p1.point_num>p2.point_num AND p2.pr_del=0 "
      "ORDER BY p2.point_num DESC";
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.Execute();
  if (Qry.Eof) throw UserException("���� �� ������");
  point_id=Qry.FieldAsInteger("point_id");

  TTripInfo info;
  info.airline=Qry.FieldAsString("airline");
  info.flt_no=Qry.FieldAsInteger("flt_no");
  info.suffix=Qry.FieldAsString("suffix");
  info.airp=Qry.FieldAsString("airp");
  info.scd_out=Qry.FieldAsDateTime("scd_out");
  info.real_out=Qry.FieldAsDateTime("real_out");

  NewTextChild(resNode,"trip",GetTripName(info));

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
        "      tlg_transfer.trfer_id=trfer_grp.trfer_id "
        "ORDER BY tlg_trips.scd,tlg_trips.airline,tlg_trips.flt_no,tlg_trips.suffix, "
        "         tlg_trips.airp_dep,tlg_transfer.trfer_id ";
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
        "      tlg_transfer.trfer_id=trfer_grp.trfer_id "
        "ORDER BY tlg_trips.scd,tlg_trips.airline,tlg_trips.flt_no,tlg_trips.suffix, "
        "         tlg_trips.airp_dep,tlg_transfer.trfer_id ";
    Qry.CreateVariable("point_id",otInteger,point_id);
    if (pr_bag)
      Qry.CreateVariable("tlg_type",otString,"BTM");
    else
      Qry.CreateVariable("tlg_type",otString,"PTM");

    PaxQry.SQLText="SELECT surname,name FROM trfer_pax WHERE grp_id=:grp_id ORDER BY surname,name";
    PaxQry.DeclareVariable("grp_id",otInteger);

    TagQry.SQLText=
      "SELECT TRUNC(no/1000) AS pack, "
      "       MOD(no,1000) AS no "
      "FROM trfer_tags WHERE grp_id=:grp_id ORDER BY trfer_tags.no";
    TagQry.DeclareVariable("grp_id",otInteger);
  }
  else
  {
    Qry.SQLText=
      "SELECT trfer_trips.airline,trfer_trips.flt_no,trfer_trips.suffix,trfer_trips.scd, "
      "       pax_grp.airp_arv AS airp_dep,transfer.airp_arv,transfer.subclass, "
      "       pax_grp.grp_id,pax_grp.class AS subcl, "
      "       NVL(ckin.get_bagAmount(pax_grp.grp_id,NULL,rownum),0) AS bag_amount, "
      "       NVL(ckin.get_bagWeight(pax_grp.grp_id,NULL,rownum),0) AS bag_weight, "
      "       NVL(ckin.get_rkWeight(pax_grp.grp_id,NULL,rownum),0) AS rk_weight, "
      "       'K' AS weight_unit "
      "FROM pax_grp,transfer,trfer_trips "
      "WHERE pax_grp.grp_id=transfer.grp_id AND "
      "      transfer.point_id_trfer=trfer_trips.point_id AND "
      "      transfer.transfer_num=1 AND "
      "      pax_grp.point_dep=:point_id AND bag_refuse=0 AND pax_grp.status<>'T' "
      "ORDER BY trfer_trips.scd,trfer_trips.airline,trfer_trips.flt_no, "
      "         trfer_trips.suffix,pax_grp.airp_arv,transfer.airp_arv ";
    Qry.CreateVariable("point_id",otInteger,point_id);

    PaxQry.SQLText=
      "SELECT surname,name,seats FROM pax WHERE grp_id=:grp_id AND pr_brd IS NOT NULL";
    PaxQry.DeclareVariable("grp_id",otInteger);

    TagQry.SQLText=
      "SELECT TRUNC(no/1000) AS pack, "
      "       MOD(no,1000) AS no "
      "FROM bag_tags WHERE grp_id=:grp_id ORDER BY bag_tags.no";
    TagQry.DeclareVariable("grp_id",otInteger);
  };

  Qry.Execute();
  xmlNodePtr trferNode=NewTextChild(resNode,"transfer");
  xmlNodePtr grpNode,paxNode,node,node2;
  int grp_id;
  string prev_trip;
  char subcl[2],airp_dep[4],airp_arv[4];
  *subcl=0;
  *airp_dep=0;
  *airp_arv=0;
  for(;!Qry.Eof;Qry.Next())
  {
    ostringstream trip;
    trip << Qry.FieldAsString("airline")
         << Qry.FieldAsInteger("flt_no")
         << Qry.FieldAsString("suffix") << "/"
         << DateTimeToStr(Qry.FieldAsDateTime("scd"),"dd");

    if (prev_trip!=trip.str() ||
        strcmp(airp_dep,Qry.FieldAsString("airp_dep"))!=0 ||
        strcmp(airp_arv,Qry.FieldAsString("airp_arv"))!=0 ||
        strcmp(subcl,Qry.FieldAsString("subcl"))!=0)
    {
      node=NewTextChild(trferNode,"trfer_flt");

      NewTextChild(node,"trip",trip.str());
      if (pr_tlg) NewTextChild(node,"airp",Qry.FieldAsString("airp"));
      NewTextChild(node,"airp_dep",Qry.FieldAsString("airp_dep"));
      NewTextChild(node,"airp_arv",Qry.FieldAsString("airp_arv"));
      NewTextChild(node,"subcl",Qry.FieldAsString("subcl"));
      grpNode=NewTextChild(node,"grps");

      prev_trip=trip.str();
      strcpy(airp_dep,Qry.FieldAsString("airp_dep"));
      strcpy(airp_arv,Qry.FieldAsString("airp_arv"));
      strcpy(subcl,Qry.FieldAsString("subcl"));
    };

    node=NewTextChild(grpNode,"grp");
    if (!Qry.FieldIsNULL("bag_amount"))
      NewTextChild(node,"bag_amount",Qry.FieldAsInteger("bag_amount"));
    else
      NewTextChild(node,"bag_amount");
    if (!Qry.FieldIsNULL("bag_weight"))
      NewTextChild(node,"bag_weight",Qry.FieldAsInteger("bag_weight"));
    else
      NewTextChild(node,"bag_weight");
    if (!Qry.FieldIsNULL("rk_weight"))
      NewTextChild(node,"rk_weight",Qry.FieldAsInteger("rk_weight"));
    else
      NewTextChild(node,"rk_weight");
    NewTextChild(node,"weight_unit",Qry.FieldAsString("weight_unit"));

    grp_id=Qry.FieldAsInteger("grp_id");

    PaxQry.SetVariable("grp_id",grp_id);
    PaxQry.Execute();
    int seats=0;
    if (!PaxQry.Eof)
    {
      paxNode=NewTextChild(node,"passengers");
      for(;!PaxQry.Eof;PaxQry.Next())
      {
        node2=NewTextChild(paxNode,"pax");
        NewTextChild(node2,"surname",PaxQry.FieldAsString("surname"));
        NewTextChild(node2,"name",PaxQry.FieldAsString("name"),"");
        if (!pr_tlg)
        {
          if (PaxQry.FieldAsInteger("seats")>0) seats++;
          for(int i=PaxQry.FieldAsInteger("seats");i>=2;i--)
          {
            node2=NewTextChild(paxNode,"pax");
            NewTextChild(node2,"surname",PaxQry.FieldAsString("surname"));
            NewTextChild(node2,"name","EXST",""); //���᫥��⢨� ���� ���뢠�� STCR
          };
        };
      };
    };

    if (pr_tlg)
    {
      if (!Qry.FieldIsNULL("seats"))
        NewTextChild(node,"seats",Qry.FieldAsInteger("seats"));
      else
        NewTextChild(node,"seats");
    }
    else
    {
      NewTextChild(node,"seats",seats);
    };

    if (pr_bag)
    {
      TagQry.SetVariable("grp_id",grp_id);
      TagQry.Execute();
      if (!TagQry.Eof)
      {
        int first_no,first_pack,curr_no,curr_pack;
        first_no=TagQry.FieldAsInteger("no");
        first_pack=TagQry.FieldAsInteger("pack");
        int num=0;
        node2=NewTextChild(node,"tag_ranges");
        for(;;TagQry.Next())
        {
          if (!TagQry.Eof)
          {
            curr_no=TagQry.FieldAsInteger("no");
            curr_pack=TagQry.FieldAsInteger("pack");
          };

          if (TagQry.Eof||
              first_pack!=curr_pack||
              first_no+num!=curr_no)
          {
            ostringstream range;
            range.setf(ios::fixed);
            range << setw(10) << setfill('0') << setprecision(0)
                  << (first_pack*1000.0+first_no);
            if (num!=1)
              range << "-"
                    << setw(3)  << setfill('0')
                    << (first_no+num-1);
            NewTextChild(node2,"range",range.str());

            if (TagQry.Eof) break;
            first_no=curr_no;
            first_pack=curr_pack;
            num=0;
          };
          num++;
        };
      };
    };
  };
};

void SoppInterface::GetPaxTransfer(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  GetTransfer(ctxt,reqNode,resNode,false);
};

void SoppInterface::GetBagTransfer(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  GetTransfer(ctxt,reqNode,resNode,true);
};


void SoppInterface::DeleteAllPassangers(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  int point_id = NodeAsInteger( "point_id", reqNode );

	TQuery Qry(&OraSession);
	Qry.Clear();
	Qry.SQLText=
	  "SELECT airline,flt_no,airp,point_num, "
    "       DECODE(pr_tranzit,0,point_id,first_point) AS first_point "
    "FROM points "
    "WHERE point_id=:point_id AND pr_reg<>0 AND pr_del=0 FOR UPDATE";
  Qry.CreateVariable("point_id",otInteger,point_id);
  Qry.Execute();
  if (Qry.Eof) throw UserException("���� �������. ������� �����");

  TTypeBSendInfo sendInfo;
  sendInfo.airline=Qry.FieldAsString("airline");
  sendInfo.flt_no=Qry.FieldAsInteger("flt_no");
  sendInfo.airp_dep=Qry.FieldAsString("airp");
  sendInfo.point_num=Qry.FieldAsInteger("point_num");
  sendInfo.first_point=Qry.FieldAsInteger("first_point");
  sendInfo.tlg_type="BSM";

  //BSM
  map<bool,string> BSMaddrs;
  map<int,TBSMContent> BSMContentBefore;
  bool BSMsend=TelegramInterface::IsBSMSend(sendInfo,BSMaddrs);

  if (BSMsend)
  {
    Qry.Clear();
    Qry.SQLText=
      "SELECT grp_id FROM pax_grp WHERE point_dep=:point_id";
    Qry.CreateVariable( "point_id", otInteger, point_id );
    Qry.Execute();
    for(;!Qry.Eof;Qry.Next())
    {
      TBSMContent BSMContent;
      TelegramInterface::LoadBSMContent(Qry.FieldAsInteger("grp_id"),BSMContent);
      BSMContentBefore[Qry.FieldAsInteger("grp_id")]=BSMContent;
    };
  };

  Qry.Clear();
	Qry.SQLText =
	 "BEGIN "
   " DECLARE "
   " CURSOR cur IS "
   "   SELECT grp_id FROM pax_grp WHERE point_dep=:point_id; "
   " curRow      cur%ROWTYPE; "
   " BEGIN "
   "  UPDATE trip_comp_elems SET pr_free=1 WHERE point_id=:point_id; "
   "  FOR curRow IN cur LOOP "
   "    UPDATE pax SET refuse='�',pr_brd=NULL,seat_no=NULL WHERE grp_id=curRow.grp_id; "
   "    mvd.sync_pax_grp(curRow.grp_id,:term); "
   "    ckin.check_grp(curRow.grp_id); "
   "  END LOOP; "
   "  ckin.recount(:point_id); "
   " END; "
   "END;";

  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.CreateVariable( "term", otString, TReqInfo::Instance()->desk.code );
  Qry.Execute();
  TReqInfo::Instance()->MsgToLog( "�� ���ᠦ��� ࠧॣ����஢���", evtPax, point_id );
  showMessage( "�� ���ᠦ��� ࠧॣ����஢���" );

  //BSM
  if (BSMsend)
  {
    map<int,TBSMContent>::iterator i;
    for(i=BSMContentBefore.begin();i!=BSMContentBefore.end();i++)
    {
      TelegramInterface::SendBSM(point_id,i->first,i->second,BSMaddrs);
    };
  };
}

void SoppInterface::WriteTrips(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
	xmlNodePtr node = NodeAsNode( "trips", reqNode );
	node = node->children;
	TQuery Qry(&OraSession);
	xmlNodePtr n, stnode;
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
      	while ( stnode ) {
      		name = NodeAsString( stnode );
      		Qry.SetVariable( "name", name );
      		pr_main = GetNode( "pr_main", stnode );
      		Qry.SetVariable( "pr_main", pr_main );
      		Qry.Execute();
      		if ( !tolog.empty() )
      				tolog += ", ";
      			tolog += name;
      		if ( pr_main )
      			tolog += " (�������)";
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
 		    Qry.Clear();
  	    Qry.SQLText =
  	     "UPDATE trip_sets "
  	     " SET max_commerce=:max_commerce, "
  	     "     overload_alarm=DECODE(max_commerce,:max_commerce,overload_alarm,0) "
  	     " WHERE point_id=:point_id";
  	    Qry.CreateVariable( "point_id", otInteger, point_id );
  	    int max_commerce = NodeAsInteger( max_cNode );
  	    Qry.CreateVariable( "max_commerce", otInteger, max_commerce );
  	    Qry.Execute();
  	    TReqInfo::Instance()->MsgToLog( string( "����. �������᪠� ����㧪�: " ) + IntToString( max_commerce ) + "��.", evtFlt, point_id );
  		}
  		xmlNodePtr trip_load = GetNode( "trip_load", luggageNode );
  		if ( trip_load ) {
  			Qry.Clear();
  			Qry.SQLText =
  			 "BEGIN "\
  			 " UPDATE trip_load SET cargo=:cargo,mail=:mail"\
  			 "  WHERE point_dep=:point_id AND airp_arv=:airp_arv AND point_arv=:point_arv; "\
  			 " IF SQL%NOTFOUND THEN "\
  			 "  INSERT INTO trip_load(point_dep,airp_dep,point_arv,airp_arv,cargo,mail)  "\
  			 "   SELECT point_id,airp,:point_arv,:airp_arv,:cargo,:mail FROM points "\
  			 "    WHERE point_id=:point_id; "\
  			 " END IF;"\
  			 "END;";
  			Qry.CreateVariable( "point_id", otInteger, point_id );
  			Qry.DeclareVariable( "point_arv", otInteger );
  			Qry.DeclareVariable( "airp_arv", otString );
  			Qry.DeclareVariable( "cargo", otInteger );
  			Qry.DeclareVariable( "mail", otInteger );
  			xmlNodePtr load = trip_load->children;
  			while( load ) {
  				xmlNodePtr x = load->children;
  				Qry.SetVariable( "point_arv", NodeAsIntegerFast( "point_arv", x ) );
  				string airp_arv = NodeAsStringFast( "airp_arv", x );
  				Qry.SetVariable( "airp_arv", airp_arv );
  				int cargo = NodeAsIntegerFast( "cargo", x );
  				Qry.SetVariable( "cargo", cargo );
  				int mail = NodeAsIntegerFast( "mail", x );
  				Qry.SetVariable( "mail", mail );
  				Qry.Execute();
          TReqInfo::Instance()->MsgToLog(
          	string( "���ࠢ����� " ) + airp_arv + ": " +
            "��� " + IntToString( cargo ) + " ��., " +
            "���� " + IntToString( mail ) + " ��.", evtFlt, point_id );
  			  load = load->next;
  			}
  		}
  	}
		node = node->next;
	}
	showMessage( "����� �ᯥ譮 ��࠭���" );
}

void GetBirks( int point_id, xmlNodePtr dataNode )
{
	TQuery Qry(&OraSession);
	Qry.SQLText =
	 "SELECT COUNT(*) AS nobrd, sopp.get_birks( :point_id ) AS birks "\
	 " FROM pax_grp,pax "\
	 "WHERE pax_grp.grp_id=pax.grp_id AND point_dep=:point_id AND pr_brd=0 ";
	Qry.CreateVariable( "point_id", otInteger, point_id );
	Qry.Execute();
	xmlNodePtr node = NewTextChild( dataNode, "birks", Qry.FieldAsString( "birks" ) );
	NewTextChild( node, "nobrd", Qry.FieldAsInteger( "nobrd" ) );
	NewTextChild( node, "birks", Qry.FieldAsString( "birks" ) );
}

void GetLuggage( int point_id, Luggage &lug )
{
	GetLuggage( point_id, lug, true );
}


void GetLuggage( int point_id, Luggage &lug, bool pr_brd )
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
     "    FROM bag2, "
     "       (SELECT DISTINCT pax_grp.grp_id,point_arv,class FROM pax_grp,pax "
     "        WHERE pax_grp.grp_id=pax.grp_id AND "
     "              point_dep=:point_id AND bag_refuse=0 AND ";
  if (pr_brd)
    sql << "        pr_brd=1 ";
  else
    sql << "        pr_brd IS NOT NULL ";
  sql <<
     "        UNION "
     "        SELECT pax_grp.grp_id,point_arv,class FROM pax_grp "
     "        WHERE point_dep=:point_id AND bag_refuse=0 AND class IS NULL "
     "       ) pax_grp "
     "    WHERE bag2.grp_id=pax_grp.grp_id "
     "    GROUP BY pax_grp.point_arv, NVL(class,' ')) b, "

     //�������� �� ����稢������ ����:
     "   (SELECT pax_grp.point_arv, NVL(class,' ') AS class, "
     "	         SUM(excess) AS excess "
     "	  FROM "
     "	     (SELECT DISTINCT pax_grp.grp_id,point_arv,class,excess FROM pax_grp,pax "
     "        WHERE pax_grp.grp_id=pax.grp_id AND "
     "              point_dep=:point_id AND bag_refuse=0 AND ";
  if (pr_brd)
    sql << "        pr_brd=1 ";
  else
    sql << "        pr_brd IS NOT NULL ";
  sql <<
     "        UNION "
     "        SELECT pax_grp.grp_id,point_arv,class,excess FROM pax_grp "
     "        WHERE point_dep=:point_id AND bag_refuse=0 AND class IS NULL "
     "       ) pax_grp "
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
   "SELECT airp,act_out,points.pr_del pr_del,max_commerce,pr_tranzit,first_point,point_num "
   " FROM points,trip_sets "
    "WHERE points.point_id=:point_id AND trip_sets.point_id=:point_id ";
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.Execute();
  lug.max_commerce = Qry.FieldAsInteger( "max_commerce" );
  lug.pr_edit = !Qry.FieldIsNULL( "act_out" ) || Qry.FieldAsInteger( "pr_del" ) != 0;
  lug.region = AirpTZRegion( Qry.FieldAsString( "airp" ) );
  int pr_tranzit = Qry.FieldAsInteger( "pr_tranzit" );
  int first_point = Qry.FieldAsInteger( "first_point" );
  int point_num = Qry.FieldAsInteger( "point_num" );
	if ( !pr_tranzit )
    first_point = point_id;
	Qry.Clear();
	Qry.SQLText =
	 "SELECT cargo,mail,a.airp airp_arv,a.point_id point_arv, a.point_num "\
	 " FROM trip_load, "\
	 "( SELECT point_id, point_num, airp FROM points "
	 "   WHERE first_point=:first_point AND point_num>:point_num AND pr_del=0 ) a, "\
	 "( SELECT MIN(point_num) as point_num FROM points "\
	 "   WHERE first_point=:first_point AND point_num>:point_num AND pr_del=0 "\
	 "  GROUP BY airp ) b "\
	 "WHERE a.point_num=b.point_num AND trip_load.point_dep(+)=:point_id AND "\
	 "      trip_load.point_arv(+)=a.point_id "\
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
  	cargo.airp_arv = Qry.FieldAsString( "airp_arv" );
  	lug.vcargo.push_back( cargo );
  	Qry.Next();
  }
}

void GetLuggage( int point_id, xmlNodePtr dataNode )
{
	Luggage lug;
	GetLuggage( point_id, lug );
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
  	NewTextChild( fn, "airp_arv", ElemIdToElemCtxt( ecDisp, etAirp, c->airp_arv, TReqInfo::Instance()->user.sets.disp_airp ) );
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
  if ( GetNode( "stages", reqNode ) ) {
  	TQuery Qry(&OraSession );
  	Qry.SQLText = "SELECT airp FROM points WHERE point_id=:point_id";
  	Qry.CreateVariable( "point_id", otInteger, point_id );
  	Qry.Execute();
  	if ( !Qry.Eof ) {
  	  vector<TSoppStage> stages;
  	  read_TripStages( stages, false, 0, point_id );
      string region = AirpTZRegion( Qry.FieldAsString( "airp" ) );
      try {
  	    build_TripStages( stages, region, dataNode, false );
  	  }
      catch( Exception &e ) {
        ProgError( STDLOG, "Exception: %s, point_id=%d", e.what(), point_id );
        throw;
      }
    }
  }
}

void internal_ReadDests( int move_id, TDateTime arx_date, TSOPPDests &dests, string &reference )
{
	TReqInfo *reqInfo = TReqInfo::Instance();
  TQuery Qry(&OraSession);
  if ( arx_date > NoExists ) {
  	ProgTrace( TRACE5, "arx_date=%s, move_id=%d", DateTimeToStr( arx_date, "dd.mm.yyyy hh:nn" ).c_str(), move_id );
    Qry.SQLText =
      "SELECT reference FROM arx_move_ref WHERE part_key>=:arx_date AND move_id=:move_id";
    Qry.CreateVariable( "arx_date", otDate, arx_date );
  }
  else
    Qry.SQLText =
      "SELECT reference FROM move_ref WHERE move_id=:move_id";
  Qry.CreateVariable( "move_id", otInteger, move_id );
  Qry.Execute();
  if ( Qry.RowCount() && !Qry.FieldIsNULL( "reference" ) )
  	reference = Qry.FieldAsString( "reference" );
  dests.clear();
  Qry.Clear();
  if ( arx_date > NoExists ) {
	  Qry.SQLText =
    "SELECT point_id,point_num,first_point,airp,airp_fmt,airline,airline_fmt,flt_no,suffix,suffix_fmt,craft,craft_fmt,bort,"
    "       scd_in,est_in,act_in,scd_out,est_out,act_out,trip_type,litera,park_in,park_out,remark,"
    "       pr_tranzit,pr_reg,arx_points.pr_del pr_del "
    " FROM arx_points "
    " WHERE arx_points.part_key>=:arx_date AND arx_points.move_id=:move_id AND "
    "       arx_points.pr_del!=-1 "
    " ORDER BY point_num ";
    Qry.CreateVariable( "arx_date", otDate, arx_date );
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
  if ( arx_date > NoExists ) {
    DQry.SQLText = arx_trip_delays_SQL;
    DQry.CreateVariable( "arx_date", otDate, arx_date );
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
  	d.airp_fmt = Qry.FieldAsInteger( "airp_fmt" );
 	  d.airline = Qry.FieldAsString( "airline" );
 	  d.airline_fmt = Qry.FieldAsInteger( "airline_fmt" );
  	if ( !Qry.FieldIsNULL( "flt_no" ) )
  	  d.flt_no = Qry.FieldAsInteger( "flt_no" );
  	else
  		d.flt_no = NoExists;
 	  d.suffix = Qry.FieldAsString( "suffix" );
 	  d.suffix_fmt = Qry.FieldAsInteger( "suffix_fmt" );
 	  d.craft = Qry.FieldAsString( "craft" );
 	  d.craft_fmt = Qry.FieldAsInteger( "craft_fmt" );
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

void SoppInterface::ReadDests(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  xmlNodePtr node = NewTextChild( resNode, "data" );
	int move_id = NodeAsInteger( "move_id", reqNode );
	xmlNodePtr pNode = GetNode( "arx_date", reqNode );
	TDateTime arx_date;
	if ( pNode )
		arx_date = NodeAsDateTime( pNode ) - 3;
	else
		arx_date = NoExists;
	NewTextChild( node, "move_id", move_id );
	TSOPPDests dests;
	string reference;
	internal_ReadDests( move_id, arx_date, dests, reference );
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
  		throw UserException( "�� ������ ॣ��� � ������� ३�, %s", d->airp.c_str() );
  	}
  	dnode = NULL;
  	for ( vector<TSOPPDelay>::iterator delay=d->delays.begin(); delay!=d->delays.end(); delay++ ) {
  		if ( !dnode )
  			dnode = NewTextChild( snode, "delays" );
  		xmlNodePtr fnode = NewTextChild( dnode, "delay" );
  		NewTextChild( fnode, "delay_code", delay->code );
  		NewTextChild( fnode, "time", DateTimeToStr( UTCToClient( delay->time, d->region ), ServerFormatDateTimeAsString ) );
    }
  	if ( !d->triptype.empty() )
  	  NewTextChild( snode, "trip_type", d->triptype );
  	if ( !d->litera.empty() )
  	  NewTextChild( snode, "litera", d->litera );
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
  vector<change_act> vchangeAct;
	bool ch_point_num = false;
  for( TSOPPDests::iterator id=dests.begin(); id!=dests.end(); id++ )
  	if ( id->point_num == NoExists ) {
  		ch_point_num = true;
  		break;
  	}
  bool ch_craft = false;
  TQuery Qry(&OraSession);
  TQuery DelQry(&OraSession);
  DelQry.SQLText =
   " UPDATE points SET point_num=point_num-1 WHERE point_num<=-1-:point_num AND move_id=:move_id AND pr_del=-1 ";
  DelQry.DeclareVariable( "move_id", otInteger );
  DelQry.DeclareVariable( "point_num", otInteger );
  TReqInfo *reqInfo = TReqInfo::Instance();
  bool existsTrip = false;
  bool pr_last;
  try {
    int notCancel = (int)dests.size();
    if ( notCancel < 2 )
    	throw UserException( "������� ������ ᮤ�ঠ�� �� ����� ���� ��ய��⮢" );
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
            throw UserException( string("��� ����㯠 � ������������ ") + id->airline );
          else
          	throw UserException( "�� ������ ������������" );
        }
      } // end for
      if ( !canDo )
      	if ( reqInfo->user.access.airps_permit ) {
      	  if ( reqInfo->user.access.airps.size() == 1 )
      	    throw UserException( string( "������� ������ ᮤ�ঠ�� ��ய��� " ) + *reqInfo->user.access.airps.begin() );
      	  else {
      		  string airps;
      		  for ( vector<string>::iterator s=reqInfo->user.access.airps.begin(); s!=reqInfo->user.access.airps.end(); s++ ) {
      		    if ( !airps.empty() )
      		      airps += " ";
      		    airps += *s;
      		  }
      		  if ( airps.empty() )
      		  	throw UserException( "��� ����㯠 �� � ������ ��ய����" );
      		  else
      		    throw UserException( string( "������� ������ ᮤ�ঠ�� ��� �� ���� �� ��ய��⮢ " ) + airps );
      	  }
      	}
      	else { // ᯨ᮪ ����饭��� ��ய��⮢
      		string airps;
      		for ( vector<string>::iterator s=reqInfo->user.access.airps.begin(); s!=reqInfo->user.access.airps.end(); s++ ) {
      		  if ( !airps.empty() )
      		    airps += " ";
      		  airps += *s;
      		}
      		throw UserException( string( "������� ������ ᮤ�ঠ�� ��� �� ���� �� ��ய��⮢ �⫨��� �� " ) + airps );
      	}
    }
    // �஢�ઠ �� �⬥��
    for( TSOPPDests::iterator id=dests.begin(); id!=dests.end(); id++ ) {
      if ( id->pr_del == 1 ) {
        notCancel--;
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
    Qry.Clear();
    Qry.SQLText =
     "SELECT scd_in, scd_out FROM points "
     " WHERE airline=:airline AND flt_no=:flt_no AND NVL(suffix,' ')=NVL(:suffix,' ') AND "
     "       move_id!=:move_id AND airp=:airp AND pr_del!=-1 AND "
     "       ( scd_in BETWEEN :scd_in-2 AND :scd_in+2 OR "
     "         scd_out BETWEEN :scd_out-2 AND :scd_out+2 )";
    Qry.CreateVariable( "move_id", otInteger, move_id );
    Qry.DeclareVariable( "airp", otString );
    Qry.DeclareVariable( "airline", otString );
    Qry.DeclareVariable( "flt_no", otInteger );
    Qry.DeclareVariable( "suffix", otString );
    Qry.DeclareVariable( "scd_in", otDate );
    Qry.DeclareVariable( "scd_out", otDate );
    TDateTime oldtime, curtime = NoExists;
    bool pr_time=false;
    for( TSOPPDests::iterator id=dests.begin(); id!=dests.end(); id++ ) {
  	  if ( id->scd_in > NoExists || id->scd_out > NoExists )
  	  	pr_time = true;
    	if ( id->pr_del )
  	    continue;
  	  if ( id->scd_in > NoExists && id->act_in == NoExists ) {
  	  	oldtime = curtime;
  	  	curtime = id->scd_in;
  	  	if ( oldtime > NoExists && oldtime > curtime ) {
  	  		throw UserException( string("�६��� �뫥�/�ਫ�� � ������� �� 㯮�冷祭� ") );
  	  	}
      }
      if ( id->scd_out > NoExists && id->act_out == NoExists ) {
      	oldtime = curtime;
  	  	curtime = id->scd_out;
  	  	if ( oldtime > NoExists && oldtime > curtime ) {
  	  		throw UserException( string("�६��� �뫥�/�ਫ�� � ������� �� 㯮�冷祭�") );
  	  	}
      }
      if ( id->craft.empty() ) {
        for ( TSOPPDests::iterator xd=id+1; xd!=dests.end(); xd++ ) {
        	if ( xd->pr_del )
        		continue;
          throw UserException( string("�� ����� ��� �����譮�� �㤭�") );
        }
      }
      if ( !existsTrip && id != dests.end() - 1 ) {
        Qry.SetVariable( "airline", id->airline );
        Qry.SetVariable( "flt_no", id->flt_no );
        Qry.SetVariable( "suffix", id->suffix );
        Qry.SetVariable( "airp", id->airp );
        if ( id->scd_in > NoExists )
          Qry.SetVariable( "scd_in", id->scd_in );
        else
          Qry.SetVariable( "scd_in", FNull );
        if ( id->scd_out > NoExists )
          Qry.SetVariable( "scd_out", id->scd_out );
        else
          Qry.SetVariable( "scd_out", FNull );
        Qry.Execute();
        tst();
        double scd_in,scd_out,d1;
        string region;
        if ( id->scd_in > NoExists ) {
    		  region = AirpTZRegion( id->airp );
        	ProgTrace( TRACE5, "id->airp=%s, region=%s, id->point_id=%d", id->airp.c_str(), region.c_str(), id->point_id );
          d1 = UTCToLocal( id->scd_in, region );
          tst();
          modf( d1, &scd_in );
        }
        else scd_in = NoExists;
        if ( id->scd_out > NoExists ) {
        	if ( region.empty () )
            region = AirpTZRegion( id->airp );
        	ProgTrace( TRACE5, "id->airp=%s, region=%s, id->point_id=%d", id->airp.c_str(), region.c_str(), id->point_id );
          d1 = UTCToLocal( id->scd_out, region );
          tst();
          modf( d1, &scd_out );
        }
        else scd_out = NoExists;
        while ( !Qry.Eof ) {
        	if ( !Qry.FieldIsNULL( "scd_in" ) && scd_in > NoExists ) {
        		modf( (double)UTCToLocal( Qry.FieldAsDateTime( "scd_in" ), region ), &d1 );
        		if ( d1 == scd_in ) {
        			ProgTrace( TRACE5, "d1=%f, scd_in=%f", d1, scd_in );
        			existsTrip = true;
        			break;
        		}
        	}
        	if ( !Qry.FieldIsNULL( "scd_out" ) && scd_out > NoExists ) {
        		modf( (double)UTCToLocal( Qry.FieldAsDateTime( "scd_out" ), region ), &d1 );
        		if ( d1 == scd_out ) {
        			ProgTrace( TRACE5, "d1=%f, scd_out=%f", d1, scd_out );
        			existsTrip = true;
        			break;
        		}
        	}
        	Qry.Next();
        }
      }

    } // end for
    if ( !pr_time )
    	throw UserException( string("� ������� �� ������ �६��� �ਫ��/�뫥�") );
  }
  catch( UserException &e ) {
  	if ( canExcept ) {
  		NewTextChild( NewTextChild( resNode, "data" ), "notvalid" );
  		showErrorMessage( string(e.what()) + ". ����୮� ����⨥ ������ F9 - ������." );
  		return;
    }
  }

  if ( existsTrip )
    throw UserException( "�㡫�஢���� ३ᮢ. ���� 㦥 �������" );
  Qry.Clear();
  Qry.SQLText = "SELECT code FROM trip_types WHERE pr_reg=1";
  Qry.Execute();
  vector<string> triptypes;
  while ( !Qry.Eof ) {
    triptypes.push_back( Qry.FieldAsString( "code" ) );
    Qry.Next();
  }

  /*!!! �� ⮫쪮 ��� ������ */
//  if ( move_id == NoExists ) {
  // ������� ��ࠬ��஢ pr_tranzit, pr_reg, first_point
  for( TSOPPDests::iterator id=dests.begin(); id!=dests.end(); id++ ) {
  	if ( id != dests.begin() ) {
  		TSOPPDests::iterator p=id;
  			p--;
      id->pr_tranzit=( p->airline + IntToString( p->flt_no ) + p->suffix /*+ p->triptype ???*/ ==
                       id->airline + IntToString( id->flt_no ) + id->suffix /*+ id->triptype*/ );
  	}
  	else
  		id->pr_tranzit = 0;

    id->pr_reg = ( id->scd_out > NoExists /*&& id->act_out == NoExists*/ &&
                   find( triptypes.begin(), triptypes.end(), id->triptype ) != triptypes.end() &&
                   !id->pr_del && id != dests.end() - 1 );
    if ( id->pr_reg ) {
      TSOPPDests::iterator r=id;
      r++;
      for ( ;r!=dests.end(); r++ ) {
        if ( !r->pr_del )
          break;
      }
      if ( r == dests.end() ) {
        id->pr_reg = 0;
      }
    }
  }
//  } //end move_id==NoExists

  Qry.Clear();
  bool insert = ( move_id == NoExists );
  if ( insert ) {
  /* ����室��� ᤥ���� �஢��� �� �� ����⢮����� ३� !!!*/
    Qry.SQLText =
     "BEGIN "\
     " SELECT move_id.nextval INTO :move_id from dual; "\
     " INSERT INTO move_ref(move_id,reference)  SELECT :move_id, :reference FROM dual; "\
     "END;";
    Qry.DeclareVariable( "move_id", otInteger );
    Qry.CreateVariable( "reference", otString, reference );
    Qry.Execute();
    move_id = Qry.GetVariableAsInteger( "move_id" );
    reqInfo->MsgToLog( "���� ������ ३� ", evtDisp, move_id );
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
    //reqInfo->MsgToLog( "��������� ३� ", evtDisp, move_id );
  }
  TSOPPDest old_dest;
  bool ch_dests = false;
  int new_tid;
  bool init_trip_stages;
  bool set_act_out;
  bool set_pr_del;
  int point_num = 0;
  int first_point;
  bool insert_point;
  bool pr_begin = true;
  for( TSOPPDests::iterator id=dests.begin(); id!=dests.end(); id++ ) {
  	if ( ch_point_num )
  	  id->point_num = point_num;
  	if ( id->modify ) {
  	  Qry.Clear();
  	  Qry.SQLText =
  	   "SELECT tid__seq.nextval n FROM dual ";
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
    else {
    	point_num++;
    }
/* !!! */
    if ( id->pr_del != -1 ) {
 		  if ( pr_begin ) {
 		  	  pr_begin = false;
      	  first_point = id->point_id;
      	  if ( id->first_point != NoExists ) {
      	    id->first_point = NoExists;
      	    id->modify = true;
      	  }
      }
      else
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
    }
/*!!!end of*/

    if ( !id->modify ) { //??? remark
    	continue;
    }

    if ( insert_point ) {
    	ch_craft = false;
      reqInfo->MsgToLog( string( "���� ������ �㭪� " ) + id->airp, evtDisp, move_id );
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
  	  if ( !old_dest.pr_reg && id->pr_reg && !id->pr_del ) {
  	    Qry.Clear();
  	    Qry.SQLText = "SELECT COUNT(*) c FROM trip_stages WHERE point_id=:point_id AND rownum<2";
  	    Qry.CreateVariable( "point_id", otInteger, id->point_id );
  	    Qry.Execute();
  	    init_trip_stages = !Qry.FieldAsInteger( "c" );
  	    ProgTrace( TRACE5, "init_trip_stages=%d", init_trip_stages );
  	  }
  	  else
  	  	init_trip_stages = false;

  	  id->remark = old_dest.remark;
  	  ProgTrace( TRACE5, "id->remark=%s", id->remark.c_str() );
/*  	  if ( id->act_out == NoExists && id->est_out > NoExists && id->est_out != old_dest.est_out ) { //����প�
  	  	string::size_type idx = id->remark.find( "����প� �� " );
  	  	if ( idx != string::npos )
  	  		id->remark.replace( idx, string( "����প� �� dd hh:nn" ).size(),
  	  		                        string( "����প� �� " ) + DateTimeToStr( id->est_out, "dd hh:nn" ) );
        else
        	id->remark = string( "����প� �� " ) + DateTimeToStr( id->est_out, "dd hh:nn" ) + id->remark;
        reqInfo->MsgToLog( string( "��������� ���⭮�� �६��� �뫥� �� " ) + DateTimeToStr( id->est_out, "dd hh:nn" ), evtDisp, move_id, id->point_id );
  	  }*/
  	  if ( id->pr_del != -1 && !id->craft.empty() && id->craft != old_dest.craft && !old_dest.craft.empty() ) {
  	  	ch_craft = true;
  	  	if ( !old_dest.craft.empty() ) {
  	  	  id->remark += " ���. ⨯� �� � " + old_dest.craft;
  	  	  if ( !id->craft.empty() )
  	  	    reqInfo->MsgToLog( string( "��������� ⨯� �� �� " ) + id->craft + " ���� " + id->airp, evtDisp, move_id, id->point_id );
  	  	}
  	  	else {
  	  		reqInfo->MsgToLog( string( "�����祭�� �� " ) + id->craft + " ���� " + id->airp, evtDisp, move_id, id->point_id );
  	  	}
  	  }
  	  if ( id->bort != old_dest.bort ) {
  	  	if ( !old_dest.bort.empty() ) {
  	  	  id->remark += " ���. ���� � " + old_dest.bort;
  	  	  if ( !id->bort.empty() )
  	  	    reqInfo->MsgToLog( string( "��������� ���� �� " ) + id->bort + " ���� " + id->airp, evtDisp, move_id, id->point_id );
  	  	}
  	  	else {
  	  		reqInfo->MsgToLog( string( "�����祭�� ���� " ) + id->bort + " ���� " + id->airp, evtDisp, move_id, id->point_id );
  	  	}
  	  }
  	  if ( id->pr_del != old_dest.pr_del ) {
  	  	if ( id->pr_del == 1 )
  	  		reqInfo->MsgToLog( string( "�⬥�� �㭪� " ) + id->airp, evtDisp, move_id, id->point_id );
  	  	else
  	  		if ( id->pr_del == 0 )
	  	  		reqInfo->MsgToLog( string( "������ �㭪� " ) + id->airp, evtDisp, move_id, id->point_id );
	  	  	else
	  	  		if ( id->pr_del == -1 ) {
          		DelQry.SetVariable( "move_id", move_id );
    	      	DelQry.SetVariable( "point_num", id->point_num );
    		      DelQry.Execute();
    	      	id->point_num = -1-id->point_num;
    	      	ProgTrace( TRACE5, "point_num=%d", id->point_num );
	  	  			reqInfo->MsgToLog( string( "�������� �㭪� " ) + id->airp, evtDisp, move_id, id->point_id );
	  	  		}
  	  }
  	  else
  	    if ( !id->pr_del && id->act_out != old_dest.act_out && old_dest.act_out > NoExists ) {
  	    	reqInfo->MsgToLog( string( "��������� �६��� 䠪��᪮�� �뫥� " ) + DateTimeToStr( id->act_out, "hh:nn dd.mm.yy (UTC)" ), evtDisp, move_id, id->point_id );
   		    change_act A;
  		    A.point_id = id->point_id;
  		    A.old_act = old_dest.act_out;
  		    A.act = id->act_out;
  		    A.pr_land = false;
  		    vchangeAct.push_back( A );
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
       "     remark=SUBSTR(:remark,1,250),pr_reg=:pr_reg "
       " WHERE point_id=:point_id AND move_id=:move_id ";
  	} // end if
  	set_pr_del = ( !old_dest.pr_del && id->pr_del );
  	set_act_out = ( !id->pr_del && old_dest.act_out == NoExists && id->act_out > NoExists );
  	if ( !id->pr_del && old_dest.act_in == NoExists && id->act_in > NoExists ) {
  		reqInfo->MsgToLog( string( "���⠢����� 䠪�. �ਫ�� " ) + DateTimeToStr( id->act_in, "hh:nn dd.mm.yy (UTC)" ), evtDisp, move_id, id->point_id );
  		change_act A;
  		A.point_id = id->point_id;
  		A.old_act = old_dest.act_in;
  		A.act = id->act_in;
  		A.pr_land = true;
  		vchangeAct.push_back( A );
  	}
  	if ( !id->pr_del && id->act_in != old_dest.act_in && old_dest.act_in > NoExists ) {
  		reqInfo->MsgToLog( string( "��������� �६��� 䠪��᪮�� �ਫ�� " ) + DateTimeToStr( id->act_in, "hh:nn dd.mm.yy (UTC)" ), evtDisp, move_id, id->point_id );
  		change_act A;
  		A.point_id = id->point_id;
  		A.old_act = old_dest.act_in;
  		A.act = id->act_in;
  		A.pr_land = true;
  		vchangeAct.push_back( A );
  	}
  	ProgTrace( TRACE5, "move_id=%d,point_id=%d,point_num=%d,first_point=%d,flt_no=%d",
  	           move_id,id->point_id,id->point_num,id->first_point,id->flt_no );
  	ProgTrace( TRACE5, "airp=%s,airp_fmt=%d,airline=%s,airline_fmt=%d,craft=%s,craft_fmt=%d,suffix=%s,suffix_fmt=%d",
               id->airp.c_str(), id->airp_fmt, id->airline.c_str(), id->airline_fmt, id->craft.c_str(), id->craft_fmt, id->suffix.c_str(), id->suffix_fmt );
  	Qry.CreateVariable( "move_id", otInteger, move_id );
  	Qry.CreateVariable( "point_id", otInteger, id->point_id );
  	if ( ch_point_num )
  	  Qry.CreateVariable( "point_num", otInteger, 999 - id->point_num );
  	else
  		Qry.CreateVariable( "point_num", otInteger, id->point_num );
  	Qry.CreateVariable( "airp", otString, id->airp );
  	Qry.CreateVariable( "airp_fmt", otInteger, id->airp_fmt );
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
  	  Qry.CreateVariable( "airline_fmt", otInteger, id->airline_fmt );
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
  		Qry.CreateVariable( "suffix_fmt", otInteger, id->suffix_fmt );
  	}
  	if ( id->craft.empty() ) {
  		Qry.CreateVariable( "craft", otString, FNull );
  		Qry.CreateVariable( "craft_fmt", otInteger, FNull );
  	}
  	else {
  		Qry.CreateVariable( "craft", otString, id->craft );
  		Qry.CreateVariable( "craft_fmt", otInteger, id->craft_fmt );
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
   	ProgTrace( TRACE5, "point_id=%d, est_out=%f", id->point_id, id->est_out );
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
  	Qry.CreateVariable( "remark", otString, id->remark );
  	Qry.CreateVariable( "pr_reg", otInteger, id->pr_reg );
//  	ProgTrace( TRACE5, "sqltext=%s", Qry.SQLText.SQLText() );
  	Qry.Execute();
  	Qry.Clear();
  	Qry.SQLText = "DELETE trip_delays WHERE point_id=:point_id";
  	Qry.CreateVariable( "point_id", otInteger, id->point_id );
  	Qry.Execute();
  	if ( !id->delays.empty() ) {
  		Qry.Clear();
  		Qry.SQLText =
  		 "INSERT INTO trip_delays(point_id,delay_num,delay_code,time) "\
  		 " VALUES(:point_id,:delay_num,:delay_code,:time) ";
  		Qry.CreateVariable( "point_id", otInteger, id->point_id );
  		Qry.DeclareVariable( "delay_num", otInteger );
  		Qry.DeclareVariable( "delay_code", otString );
  		Qry.DeclareVariable( "time", otDate );
  		int r=0;
  		for ( vector<TSOPPDelay>::iterator q=id->delays.begin(); q!=id->delays.end(); q++ ) {
  			Qry.SetVariable( "delay_num", r );
  			Qry.SetVariable( "delay_code", q->code );
  			Qry.SetVariable( "time", q->time );
  			Qry.Execute();
  			r++;
  		}
  	}
  	if ( init_trip_stages ) {
  		Qry.Clear();
  		Qry.SQLText =
       "BEGIN "
       " INSERT INTO trip_sets(point_id,f,c,y,max_commerce,overload_alarm,pr_etstatus,pr_stat, "
       "    pr_tranz_reg,pr_check_load,pr_overload_reg,pr_exam,pr_check_pay,pr_trfer_reg) "
       "  VALUES(:point_id,0,0,0, NULL, 0, 0, 0, "
       "    NULL, 0, 1, 0, 0, 0); "
       " ckin.set_trip_sets(:point_id); "
       " gtimer.puttrip_stages(:point_id); "
       "END;";
  		Qry.CreateVariable( "point_id", otInteger, id->point_id );
  		Qry.Execute();
  	}
  	if ( !id->pr_del && id->pr_reg && id->est_out > NoExists && id->est_out != id->scd_out ) {
  		Qry.Clear();
  		Qry.SQLText =
  		 "UPDATE trip_stages SET est=scd+(:vest-:vscd) WHERE point_id=:point_id AND pr_manual=0 ";
  		Qry.CreateVariable( "point_id", otInteger, id->point_id );
  		Qry.CreateVariable( "vscd", otDate, id->scd_out );
  		Qry.CreateVariable( "vest", otDate, id->est_out );
  		Qry.Execute();
  		string tolog;
  	  double f;
  		if ( id->est_out > id->scd_out ) {
  			modf( id->est_out - id->scd_out, &f );
  			tolog = "����প� �믮������ �孮�����᪮�� ��䨪� �� ";
  			if ( f )
  				tolog += IntToString( (int)f ) + " ";
  			tolog += DateTimeToStr( id->est_out - id->scd_out, "hh:nn" );

  		}
  		else {
  			modf( id->scd_out - id->est_out, &f );
  			tolog = "���०���� �믮������ �孮�����᪮�� ��䨪� �� ";
  		  if ( f )
  		    tolog += IntToString( (int)f ) + " ";
  		  tolog += DateTimeToStr( id->scd_out - id->est_out, "hh:nn" );
  		}

  		reqInfo->MsgToLog( tolog, evtDisp, move_id, id->point_id );
  		ProgTrace( TRACE5, "point_id=%d,time=%s", id->point_id,DateTimeToStr( id->est_out - id->scd_out, "dd.hh:nn" ).c_str() );
  	}
    if ( set_act_out ) {
    	//!!! �� point_num �� ����ᠭ
       try
       {
         exec_stage( id->point_id, sTakeoff );
       }
       catch( std::exception &E ) {
         ProgError( STDLOG, "Exception: %s", E.what() );
       }
       catch( ... ) {
         ProgError( STDLOG, "Unknown error" );
       };
    	reqInfo->MsgToLog( string( "���⠢����� 䠪�. �뫥� " ) + DateTimeToStr( id->act_out, "hh:nn dd.mm.yy (UTC)" ), evtDisp, move_id, id->point_id );
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
  		"SELECT COUNT(*) c FROM pax_grp,points "\
  		" WHERE points.point_id=:point_id AND "\
  		"       point_dep=:point_id AND bag_refuse=0 ";
  		Qry.CreateVariable( "point_id", otInteger, id->point_id );
  		Qry.Execute();
  		if ( Qry.FieldAsInteger( "c" ) )
  			if ( id->pr_del == -1 )
  				throw UserException( string( "����� 㤠���� ��ய��� " ) + id->airp + ". " + "���� ��ॣ����஢���� ���ᠦ���." );
  			else
  				throw UserException( string( "����� �⬥���� ��ய��� " ) + id->airp + ". " + "���� ��ॣ����஢���� ���ᠦ���." );
  		if ( id->pr_del == -1 ) {
  			Qry.Clear();
  			Qry.SQLText =
  			 "UPDATE tlgs_in SET point_id=NULL,time_parse=NULL WHERE point_id=:point_id "; //!!! ���� ��!!!???
  			Qry.CreateVariable( "point_id", otInteger, id->point_id );
  			//!!!Qry.Execute();

  		}
  	}
/*  	if ( ch_dests ) {
  		Qry.Clear();
  		Qry.SQLText =
  		"BEGIN "\
  		" UPDATE points SET remark=:remark WHERE point_id=:point_id; "\
  		" ckin.recount(:point_id); "\
  		"END ";
  		Qry.CreateVariable( "point_id", otInteger, id->point_id );
  		Qry.Execute();
  	}	*/
  	point_num++;
  } // end for
  if ( ch_point_num ) {
  	Qry.Clear();
  	Qry.SQLText = "UPDATE points SET point_num=:point_num WHERE point_id=:point_id";
  	Qry.DeclareVariable( "point_id", otInteger );
  	Qry.DeclareVariable( "point_num", otInteger );
    for( TSOPPDests::iterator id=dests.begin(); id!=dests.end(); id++ ) {
    	Qry.SetVariable( "point_id", id->point_id );
    	Qry.SetVariable( "point_num", id->point_num );
    	Qry.Execute();
    }
  }

 if ( ch_craft ) {
 	 showErrorMessage( "����� �ᯥ譮 ��࠭���. �� ������� ⨯ ��. ����室��� �������� ����������." );
 }
 else
   showMessage( "����� �ᯥ譮 ��࠭���" );

 for( vector<change_act>::iterator i=vchangeAct.begin(); i!=vchangeAct.end(); i++ ){
  if ( i->pr_land )
    ChangeACT_IN( i->point_id, i->old_act, i->act );
  else
    ChangeACT_OUT( i->point_id, i->old_act, i->act );
 }


/*   "BEGIN "\
   " SELECT move_id.nextval INTO :move_id from dual; "\
   " INSERT INTO move_ref(move_id,reference)  SELECT :move_id, NULL FROM dual; "\
   "END;";*/
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
		throw UserException( "�� ����� �������" );
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
      throw UserException( "���ࠢ��쭮 ����� ��� ��ய���" );
    }
		city = ((TAirpsRow&)baseairps.get_row( "code", d.airp )).city;
		region = CityTZRegion( city );
		d.region = region;
		fnode = GetNodeFast( "airline", snode );
		if ( fnode ) {
     	try {
        d.airline = ElemCtxtToElemId( ecDisp, etAirline, NodeAsString( fnode ), d.airline_fmt, false );
      }
      catch( EConvertError &e ) {
    	  throw UserException( "���ࠢ��쭮 ����� ��� ������������" );
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
    	  throw UserException( "���ࠢ��쭮 ����� ���䨪� ३�" );
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
    	  throw UserException( "���ࠢ��쭮 ����� ��� �����譮�� �㤭�" );
      }
	  }
		else
			d.craft.clear();
		fnode = GetNodeFast( "bort", snode );
		if ( fnode )
			d.bort = NodeAsString( fnode );
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
			while ( fnode ) {
				dnode = fnode->children;
				TSOPPDelay delay;
				delay.code = NodeAsStringFast( "delay_code", dnode );
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
		if ( fnode )
			d.triptype = NodeAsString( fnode );
		else
			d.triptype.clear();
		fnode = GetNodeFast( "litera", snode );
		if ( fnode )
			d.litera = NodeAsString( fnode );
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
	  "SELECT move_id,airline||flt_no||suffix as trip,act_out,point_num "
    "FROM points "
    "WHERE point_id=:point_id AND pr_del=0 AND act_out IS NOT NULL FOR UPDATE";
  Qry.CreateVariable("point_id",otInteger,point_id);
  Qry.Execute();
  if (Qry.Eof) throw UserException("���� �������. ������� �����");
	int point_num = Qry.FieldAsInteger( "point_num" );
	int move_id = Qry.FieldAsInteger( "move_id" );
	string trip = Qry.FieldAsString( "trip" );
	TDateTime act_out = Qry.FieldAsDateTime( "act_out" );
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
	ReadTrips( ctxt, reqNode, resNode );
}

void SoppInterface::ReadCRS_Displaces(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode) /*???*/
{
	int point_id = NodeAsInteger( "point_id", reqNode );
	xmlNodePtr crsdNode = NewTextChild( NewTextChild( resNode, "data" ), "crs_displaces" );
	xmlNodePtr displnode = NULL;
	NewTextChild( crsdNode, "point_id", point_id );

	TQuery Qry( &OraSession );
	Qry.SQLText = "SELECT airp,first_point,pr_tranzit FROM points WHERE point_id=:point_id AND pr_del!=-1";
	Qry.CreateVariable( "point_id", otInteger, point_id );
	Qry.Execute();
	if ( Qry.Eof )
		throw UserException( "���� �� ������" );
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
		NewTextChild( displnode, "airp_arv", Qry.FieldAsString( "airp" ) );
		Qry.Next();
	}
	NewTextChild( crsdNode, "airp_dep", airp_dep );
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
		NewTextChild( snode, "airp_arv_spp", Qry.FieldAsString( "airp_arv_spp" ) );
		NewTextChild( snode, "class_spp", Qry.FieldAsString( "class_spp" ) );
		NewTextChild( snode, "airline", Qry.FieldAsString( "airline" ) );
		NewTextChild( snode, "flt_no", Qry.FieldAsString( "flt_no" ) );
		NewTextChild( snode, "suffix", Qry.FieldAsString( "suffix" ) );
		string region = AirpTZRegion( Qry.FieldAsString( "airp_dep" ), true );
		NewTextChild( snode, "scd", DateTimeToStr( Qry.FieldAsDateTime( "scd" ) ) );
		NewTextChild( snode, "airp_dep", Qry.FieldAsString( "airp_dep" ) );
		NewTextChild( snode, "airp_arv_tlg", Qry.FieldAsString( "airp_arv_tlg" ) );
		NewTextChild( snode, "class_tlg", Qry.FieldAsString( "class_tlg" ) );
    NewTextChild( snode, "pr_goshow", (int)(DecodePaxStatus( Qry.FieldAsString( "status" ) ) != ASTRA::psOk) ); //!!!���
	  NewTextChild( snode, "status", Qry.FieldAsString( "status" ) );
  	Qry.Next();
  }
  /* !!! pr_utc */
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
		NewTextChild( snode, "airline", Qry.FieldAsString( "airline" ) );
		NewTextChild( snode, "flt_no", Qry.FieldAsString( "flt_no" ) );
		NewTextChild( snode, "suffix", Qry.FieldAsString( "suffix" ) );
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
  		trip = ElemIdToElemCtxt( ecDisp, etAirline, Qry.FieldAsString( "airline" ),
  		                         TReqInfo::Instance()->user.sets.disp_airline ) +
  		       Qry.FieldAsString( "flt_no" ) +
  		       ElemIdToElemCtxt( ecDisp, etSuffix, Qry.FieldAsString( "suffix" ),
  		                         TReqInfo::Instance()->user.sets.disp_suffix );
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
    str_to = "���. ����� " + str_to;
  if ( ch_dest )
    str_to = "���. �㭪� " + str_to;
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
  	throw UserException( "���� �� ������" );
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
  while ( node ) {
  	snode = node->children;
  	airp_arv_spp = NodeAsStringFast( "airp_arv_spp", snode );
  	class_spp = NodeAsStringFast( "class_spp", snode );
  	airline = NodeAsStringFast( "airline", snode );
  	flt_no = NodeAsIntegerFast( "flt_no", snode );
  	suffix = NodeAsStringFast( "suffix", snode );
  	scd = NodeAsDateTimeFast( "scd", snode );
  	airp_depNode = GetNodeFast( "airp_dep", snode );
  	if ( airp_depNode )
  		to_airp_dep = NodeAsString( airp_depNode );
  	else
  		to_airp_dep = airp_dep;
  	airp_arv_tlg = NodeAsStringFast( "airp_arv_tlg", snode );
  	class_tlg = NodeAsStringFast( "class_tlg", snode );
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
  showMessage( "����� �ᯥ譮 ��࠭���" );
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
  	internal_ReadDests( move_id, NoExists, dests, reference );
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
    	        throw UserException( "���ࠢ��쭮 ����� ��� �����譮�� �㤭�" );
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
	    	    	    		delay.code = NodeAsStringFast( "code", N );
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
	  	throw UserException( "���� �� ������. ������� �����" );
	  node = node->next;
	}
}

void SoppInterface::DeleteISGTrips(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
	xmlNodePtr node = NodeAsNode( "data", reqNode );
	int move_id = NodeAsInteger( "move_id", node );
	TQuery Qry(&OraSession);
	Qry.SQLText = "SELECT COUNT(*) c FROM pax_grp WHERE point_dep IN "
	              "( SELECT point_id FROM points WHERE move_id=:move_id )";
	Qry.CreateVariable( "move_id", otInteger, move_id );
	Qry.Execute();
	if ( Qry.FieldAsInteger( "c" ) )
		throw UserException( "����� 㤠���� ३�. ���� ��ॣ����஢���� ���ᠦ���" );
	Qry.Clear();
	Qry.SQLText = "DELETE tlg_binding WHERE point_id_spp IN "
	              "( SELECT point_id FROM points WHERE move_id=:move_id )";
	Qry.CreateVariable( "move_id", otInteger, move_id );
	Qry.Execute();
	Qry.Clear();
	Qry.SQLText = "UPDATE points SET pr_del=-1 WHERE move_id=:move_id";
	Qry.CreateVariable( "move_id", otInteger, move_id );
	Qry.Execute();
  TReqInfo::Instance()->MsgToLog( "���� 㤠���", evtDisp, move_id );
}


void SoppInterface::GetReportForm(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    get_report_form(NodeAsString("name", reqNode), resNode);
    STAT::set_variables(resNode);
}


//////////////////////////////////////////////////////////////////
void createSOPPTrip( int point_id, TSOPPTrips &trips )
{
	internal_ReadData( trips, NoExists, NoExists, false, false, point_id );
}

void ChangeACT_OUT( int point_id, TDateTime old_act, TDateTime act )
{
  try
  {
    if ( act > NoExists ) {
      vector<string> tlg_types;
      tlg_types.push_back("MVTA");
      TelegramInterface::SendTlg(point_id,tlg_types);
    }
  }
  catch(std::exception &E)
  {
    ProgError(STDLOG,"ChangeACT_OUT.SendTlg (point_id=%d): %s",point_id,E.what());
  };
}

void ChangeACT_IN( int point_id, TDateTime old_act, TDateTime act )
{
  try
  {
    if ( act > NoExists ) {
      //⥫��ࠬ�� �� �ਫ��
      TQuery Qry(&OraSession);
  	  Qry.SQLText =
  	    "SELECT points.point_id "
  	    "FROM points, "
  	    "     (SELECT point_num, first_point "
        "      FROM points WHERE point_id=:point_id) a "
        "WHERE a.first_point IN (point_id,points.first_point) AND points.point_num<a.point_num AND pr_del=0 "
        "ORDER BY points.point_num DESC ";
      Qry.CreateVariable("point_id",otInteger,point_id);
      Qry.Execute();
  	  if (!Qry.Eof)
  	  {
  	    int point_dep=Qry.FieldAsInteger("point_id");
        vector<string> tlg_types;
        tlg_types.push_back("MVTB");
        TelegramInterface::SendTlg(point_dep,tlg_types);
      };
    };
  }
  catch(std::exception &E)
  {
    ProgError(STDLOG,"ChangeACT_IN.SendTlg (point_id=%d): %s",point_id,E.what());
  };
}


