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
#include "typeb_utils.h"
#include "boost/date_time/local_time/local_time.hpp"
#include <boost/thread/thread.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp> 
#include "base_tables.h"
#include "docs.h"
#include "stat.h"
#include "salons.h"
#include "seats.h"
#include "term_version.h"
#include "flt_binding.h"
#include "rozysk.h"
#include "transfer.h"
#include "apis.h"
#include "trip_tasks.h"

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

/*const char* points_SOPP_SQL_N =
      "SELECT points.move_id,points.point_id,point_num,airp,airp_fmt,first_point,airline,airline_fmt,flt_no,"
      "       suffix,suffix_fmt,craft,craft_fmt,bort, "
      "       scd_in,est_in,act_in,scd_out,est_out,act_out,trip_type,litera,park_in,park_out,remark,"
      "       pr_tranzit,pr_reg,points.pr_del pr_del,points.tid tid "
      " FROM points, "
      "( "
      " SELECT DISTINCT move_id FROM points "
      "  WHERE points.pr_del!=-1 AND "
      "        (:where_spp_date_time_out_sql OR "
      "         :where_spp_date_time_in_sql) "
      "        :where_airline_sql "
      "        :where_airp_sql ) p "
      "WHERE points.move_id = p.move_id AND "
      "      points.pr_del!=-1 "
      "ORDER BY points.move_id,point_num";*/

/*
  A/k   Time_in    A/p  Time_out
  A/k   Time_in    A/p  NULL
  A/k   NULL       A/p  Time_out
  A/k   '01.01.01' A/p  '01.01.01'
  NULL  Time_in    A/p  NULL
  NULL  '01.01.01  A/p  '01.01.01
*/
const char* points_SOPP_SQL_N =
      "SELECT points.move_id,points.point_id,point_num,airp,airp_fmt,first_point,airline,airline_fmt,flt_no,"
      "       suffix,suffix_fmt,craft,craft_fmt,bort, "
      "       scd_in,est_in,act_in,scd_out,est_out,act_out,trip_type,litera,park_in,park_out,remark,"
      "       pr_tranzit,pr_reg,points.pr_del pr_del,points.tid tid "
      " FROM points, "
      "( "
      " SELECT DISTINCT move_id FROM points "
      "  WHERE points.pr_del!=-1 AND "
      "        (:where_spp_date_time_out_sql OR "
      "         :where_spp_date_time_in_sql OR "
      "         time_out=TO_DATE('01.01.0001','DD.MM.YYYY') ) "
      "        :or_where_airline_sql "
      "        :where_airp_sql "
      " ) p "
      "WHERE points.move_id = p.move_id AND "
      "      points.pr_del!=-1 "
      "ORDER BY points.move_id,point_num";
/*const char* points_SOPP_SQL_N =
      "SELECT points.move_id,points.point_id,point_num,airp,airp_fmt,first_point,airline,airline_fmt,flt_no,"
      "       suffix,suffix_fmt,craft,craft_fmt,bort, "
      "       scd_in,est_in,act_in,scd_out,est_out,act_out,trip_type,litera,park_in,park_out,remark,"
      "       pr_tranzit,pr_reg,points.pr_del pr_del,points.tid tid "
      " FROM points, "
      "( "
      " SELECT move_id FROM points "
      "  WHERE points.pr_del!=-1 AND "
      "        time_out in (:where_spp_date_sql,TO_DATE('01.01.0001','DD.MM.YYYY')) "
      "        :where_airp_sql :where_airline_sql "
      " UNION "
      " SELECT  move_id FROM points "
      "  WHERE points.pr_del!=-1 AND "
      "        time_in IN (:where_spp_date_sql,TO_DATE('01.01.0001','DD.MM.YYYY')) AND "
      "        ( airline IS NULL :or_where_airline_sql )"
      "        :where_airp_sql "
      " ) p "
      "WHERE points.move_id = p.move_id AND "
      "      points.pr_del!=-1 "
      "ORDER BY points.move_id,point_num,point_id";
const char* points_SOPP_SQL_N =
      "SELECT points.move_id,points.point_id,point_num,airp,airp_fmt,first_point,airline,airline_fmt,flt_no,"
      "       suffix,suffix_fmt,craft,craft_fmt,bort, "
      "       scd_in,est_in,act_in,scd_out,est_out,act_out,trip_type,litera,park_in,park_out,remark,"
      "       pr_tranzit,pr_reg,points.pr_del pr_del,points.tid tid "
      " FROM points, "
      "( "
      " SELECT move_id FROM points "
      "  WHERE points.pr_del!=-1 AND "
      "        time_out in (:where_spp_date_sql) "
      "        :where_airp_sql :where_airline_sql "
      " UNION "
      " SELECT  move_id FROM points "
      "  WHERE points.pr_del!=-1 AND "
      "        time_in IN (:where_spp_date_sql) AND "
      "        ( airline IS NULL :or_where_airline_sql )"
      "        :where_airp_sql "
      " UNION "
      " SELECT move_id FROM points "
      "  WHERE points.pr_del!=-1 AND "
      "        time_in=TO_DATE('01.01.0001','DD.MM.YYYY') AND "
      "        time_out=TO_DATE('01.01.0001','DD.MM.YYYY') AND "
      "        ( airline IS NULL :or_where_airline_sql )"
      "        :where_airp_sql "
      " ) p "
      "WHERE points.move_id = p.move_id AND "
      "      points.pr_del!=-1 "
      "ORDER BY points.move_id,point_num,point_id";
      "ORDER BY points.move_id,point_num,point_id";   */
/*const char* points_SOPP_SQL_N =
      "SELECT points.move_id,points.point_id,point_num,airp,airp_fmt,first_point,airline,airline_fmt,flt_no,"
      "       suffix,suffix_fmt,craft,craft_fmt,bort, "
      "       scd_in,est_in,act_in,scd_out,est_out,act_out,trip_type,litera,park_in,park_out,remark,"
      "       pr_tranzit,pr_reg,points.pr_del pr_del,points.tid tid "
      " FROM points WHERE move_id IN "
      "( "
      " SELECT move_id FROM points "
      "  WHERE points.pr_del!=-1 AND "
      "        time_out in (:where_spp_date_sql) "
      "        :where_airp_sql :where_airline_sql "
      " UNION ALL "
      " SELECT  move_id FROM points "
      "  WHERE points.pr_del!=-1 AND "
      "        time_in IN (:where_spp_date_sql) AND "
      "        ( airline IS NULL :or_where_airline_sql )"
      "        :where_airp_sql "
      " UNION ALL "
      " SELECT move_id FROM points "
      "  WHERE points.pr_del!=-1 AND "
      "        time_in=TO_DATE('01.01.0001','DD.MM.YYYY') AND "
      "        time_out=TO_DATE('01.01.0001','DD.MM.YYYY') AND "
      "        ( airline IS NULL :or_where_airline_sql )"
      "        :where_airp_sql "
      " )  "  */
      /*"WHERE points.move_id = p.move_id AND "*/
/*      "   AND   points.pr_del!=-1 "
      "ORDER BY points.move_id,point_num";*/
      //"ORDER BY points.move_id,point_num,point_id";

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
		rem = AstraLocale::getLocaleText( "задержка до" ) + " " + DateTimeToStr( UTCToClient( est_out, region ), "dd hh:nn" ) + remark;
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
	// 2 варианта работы. 1-локальное время пульта переведено в UTC, 2 режим LocalAll - тогда
  if ( first_date > NoExists ) {
    bool canuseTR = false;
    if ( tr.act_in > NoExists ) {
    	canuseTR = filter_time( tr.act_in, tr, first_date, next_date, errcity );
    }
    else {
      if ( tr.est_in > NoExists ) {
      	canuseTR = filter_time( tr.est_in, tr, first_date, next_date, errcity );
      }
      else {
        if ( tr.scd_in > NoExists ) {
        	canuseTR = filter_time( tr.scd_in, tr, first_date, next_date, errcity );
        }
      }
    }
    if ( !canuseTR ) {
      if ( tr.act_out > NoExists ) {
      	canuseTR = filter_time( tr.act_out, tr, first_date, next_date, errcity );
      }
      else {
        if ( tr.est_out > NoExists ) {
        	canuseTR = filter_time( tr.est_out, tr, first_date, next_date, errcity );
        }
        else {
          if ( tr.scd_out > NoExists ) {
          	canuseTR = filter_time( tr.scd_out, tr, first_date, next_date, errcity );
          }
          else {
            canuseTR = pr_isg;
          }
        }
      }
    }
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
  else { //надо набрать список клиентов
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
  	if ( ( pr_isg && st->stage_id != sRemovalGangWay ) ||
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

void addCondition_N( TQuery &PointsQry,
                     const std::string &sql,
                     TDateTime first_date,
                     TDateTime next_date,
                     bool pr_arx )
{
  string var_name;
  PointsQry.Clear();
	TReqInfo *reqInfo = TReqInfo::Instance();
  string where_airp_sql, where_airline_sql,
         where_spp_date_time_out_sql,
         where_spp_date_time_in_sql, text_sql = sql;
  if ( !reqInfo->user.access.airps.empty() ) {
    if ( pr_arx )
   	  where_airp_sql = " AND arx_points.airp";
   	else
      where_airp_sql = " AND points.airp";
    if ( !reqInfo->user.access.airps_permit ) {
      where_airp_sql += " NOT";
    }
    //where_airp_sql += " IN (";
    where_airp_sql += " IN ";
    where_airp_sql += GetSQLEnum( reqInfo->user.access.airps );
/*    if ( !reqInfo->user.access.airlines.empty()  ) {
      where_airp_sql += " OR ckin.next_airp(first_point,point_num) ";
      if ( !reqInfo->user.access.airps_permit ) {
        where_airp_sql += " NOT";
      }
      where_airp_sql += " IN ";
      where_airp_sql += GetSQLEnum( reqInfo->user.access.airps );
    }
    where_airp_sql += ") ";*/
/*    int num = 0;
    for ( vector<string>::const_iterator iairp=reqInfo->user.access.airps.begin();
          iairp!=reqInfo->user.access.airps.end(); iairp++, num++ ) {
      if ( iairp!=reqInfo->user.access.airps.begin() ) {
        where_airp_sql += ",";
      }
      var_name = string("airp") + IntToString( num );
      where_airp_sql += string(":") + var_name;
      PointsQry.CreateVariable( var_name, otString, *iairp );
      ProgTrace( TRACE5, "var_name=%s, value=%s",
                 var_name.c_str(), iairp->c_str() );
    }
    where_airp_sql += ") ";*/
  }
  if ( !reqInfo->user.access.airlines.empty() ) {
  	if ( pr_arx )
    	where_airline_sql = "arx_points.airline";
    else
      where_airline_sql = "points.airline";
    if ( !reqInfo->user.access.airlines_permit ) {
      where_airline_sql += " NOT";
    }
   // where_airline_sql += " IN (";
    where_airline_sql += " IN ";
    where_airline_sql += GetSQLEnum( reqInfo->user.access.airlines );
/*    int num=0;
    for ( vector<string>::const_iterator iairline=reqInfo->user.access.airlines.begin();
          iairline!=reqInfo->user.access.airlines.end(); iairline++, num++ ) {
      if ( iairline!=reqInfo->user.access.airlines.begin() ) {
        where_airline_sql += ",";
      }
      var_name = string("airline") + IntToString( num );
      where_airline_sql += string(":") + var_name;
      PointsQry.CreateVariable( var_name, otString, *iairline );
      ProgTrace( TRACE5, "var_name=%s, value=%s",
                 var_name.c_str(), iairline->c_str() );
    }
     where_airline_sql += ")";*/
  }
  
  if ( first_date == NoExists ) {
    throw Exception( "internal_ReadData: invalid params first_date == NoExists" );
  }
  if ( reqInfo->user.sets.time == ustTimeLocalAirp ) { // локальные времена пульта в first_date, next_date
    ProgTrace( TRACE5, "ustTimeLocalAirp!!!, first_date=%s, next_date=%s",
               DateTimeToStr( first_date-1 ).c_str(),
               DateTimeToStr( next_date+1 ).c_str() );
// вычитаем сутки, т.к. филтрация идет по UTC, а в случае режима локальных времен может быть переход на
                          // сутки и клиент этот рейс отфильтрует
                  // 20.04.2010 04:00 MEX -> 19.04.2010 22:00 LocalTime
    first_date = first_date - 1;
    next_date = next_date + 1;
  }
  TDateTime first_day, last_day;
  modf( first_date, &first_day );
  modf( next_date, &last_day );
//  int num=0;
/*  for ( TDateTime day=first_day; day<=last_day; day++, num++ ) {
    if ( !where_spp_date_sql.empty() ) {
      where_spp_date_sql += ",";
    }
    var_name = string("spp_date") + IntToString( num );
    where_spp_date_sql += string(":") + var_name;
    PointsQry.CreateVariable( var_name, otDate, day );
    ProgTrace( TRACE5, "var_name=%s, value=%s",
               var_name.c_str(), DateTimeToStr( day ).c_str() );
  }*/
  where_spp_date_time_out_sql = " time_out>=:first_day AND time_out<=:last_day ";
                                
  where_spp_date_time_in_sql = " time_in>=:first_day AND time_in<=:last_day ";
  PointsQry.CreateVariable( "first_day", otDate, first_day );
  PointsQry.CreateVariable( "last_day", otDate, last_day );

  string::size_type idx;
  while ( (idx = text_sql.find( ":where_airp_sql" )) != string::npos ) {
    text_sql.erase( idx, strlen( ":where_airp_sql" ) );
    text_sql.insert( idx, where_airp_sql );
  };
  while ( (idx = text_sql.find( ":where_airline_sql" )) != string::npos ) {
    text_sql.erase( idx, strlen( ":where_airline_sql" ) );
    if ( !where_airline_sql.empty() ) {
      text_sql.insert( idx, string( " AND " ) + where_airline_sql );
    }
  };
  while ( (idx = text_sql.find( ":or_where_airline_sql" )) != string::npos ) {
    text_sql.erase( idx, strlen( ":or_where_airline_sql" ) );
    if ( !where_airline_sql.empty() ) {
      text_sql.insert( idx, string( " AND ( airline IS NULL OR " ) + where_airline_sql + ")" );
    }
  };
  while ( (idx = text_sql.find( ":where_spp_date_time_out_sql" )) != string::npos ) {
    text_sql.erase( idx, strlen( ":where_spp_date_time_out_sql" ) );
    text_sql.insert( idx, where_spp_date_time_out_sql );
  };
  while ( (idx = text_sql.find( ":where_spp_date_time_in_sql" )) != string::npos ) {
    text_sql.erase( idx, strlen( ":where_spp_date_time_in_sql" ) );
    text_sql.insert( idx, where_spp_date_time_in_sql );
  };
  PointsQry.SQLText = text_sql;
  ProgTrace( TRACE5, "addCondition_N: SQL=\n%s", text_sql.c_str());
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

string internal_ReadData_N( TSOPPTrips &trips, TDateTime first_date, TDateTime next_date,
                            bool arx, TModule module, long int &exec_time, int point_id = NoExists )
{
	string errcity;
	TReqInfo *reqInfo = TReqInfo::Instance();

  if ( ( reqInfo->user.access.airlines.empty() && reqInfo->user.access.airlines_permit ) ||
       ( reqInfo->user.access.airps.empty() && reqInfo->user.access.airps_permit ) )
    return errcity;

  TQuery PointsQry( &OraSession );
  TBaseTable &airps = base_tables.get( "airps" );
  TBaseTable &cities = base_tables.get( "cities" );
  bool pr_addCondition_N = false;
  if ( arx ) {
  	if ( module == tISG )
        PointsQry.SQLText = addCondition( arx_points_ISG_SQL, arx ).c_str();
    else
        PointsQry.SQLText = addCondition( arx_points_SOPP_SQL, arx ).c_str();
  }
  else {
  	if ( module == tISG )
  	  PointsQry.SQLText = addCondition( points_ISG_SQL, arx ).c_str();
    else {
  		if ( point_id == NoExists ) {
	  		addCondition_N( PointsQry, points_SOPP_SQL_N, first_date, next_date, arx );
	  		pr_addCondition_N = true;
      }
      else {
  			PointsQry.SQLText = points_id_SOPP_SQL;
  			PointsQry.CreateVariable( "point_id", otInteger, point_id );
   	  }
    }
  }
  if ( !pr_addCondition_N ) {
    if ( point_id == NoExists ) {
      if ( first_date != NoExists ) {
        if ( 	TReqInfo::Instance()->user.sets.time == ustTimeLocalAirp ) { // локальные времена пульта в first_date, next_date
        	ProgTrace( TRACE5, "ustTimeLocalAirp!!!, first_date=%s, next_date=%s", DateTimeToStr( first_date-1 ).c_str(), DateTimeToStr( next_date+1 ).c_str() );
  // вычитаем сутки, т.к. филтрация идет по UTC, а в случае режима локальных времен может быть переход на
                            // сутки и клиент этот рейс отфильтрует
                            // 20.04.2010 04:00 MEX -> 19.04.2010 22:00 LocalTime
          PointsQry.CreateVariable( "first_date", otDate, first_date-1 ); // UTC +- сутки
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
      }
      else {
        throw Exception( "internal_ReadData: invalid params" );
      }
    }
  }

  TCFG cfg;
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

  TQuery Trfer_outQry( &OraSession );
  TQuery Trfer_inQry( &OraSession );

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
    boost::posix_time::ptime mcsTime = boost::posix_time::microsec_clock::universal_time();
    PointsQry.Execute();
    exec_time = (boost::posix_time::microsec_clock::universal_time() - mcsTime).total_microseconds();
  }
  catch (EOracleError E) {
    if ( arx && E.Code == 376 )
      throw AstraLocale::UserException("MSG.ONE_OF_DB_FILES_UNAVAILABLE.CALL_ADMIN");
    throw;
  }
  if ( PointsQry.Eof ) {
  	if ( !errcity.empty() )
  	  ProgError(  STDLOG, "Invalid city errcity=%s", errcity.c_str() );
  	return errcity;
  }
  TSOPPDests dests;
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
	int col_part_key = -1;
	if ( arx )
		col_part_key = PointsQry.FieldIndex( "part_key" );
  vector<TSOPPTrip> vtrips;
  int fetch_count = 0;
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
            	if ( module == tISG && reqInfo->desk.city != "ЧЛБ" ) {
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
    fetch_count++;
    PointsQry.Next();
  } // end while !PointsQry.Eof
  ProgTrace( TRACE5, "fetch_count=%d", fetch_count );
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
          	if ( module == tISG && reqInfo->desk.city != "ЧЛБ" ) {
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
  // рейсы созданы, перейдем к набору информации по рейсам
  ////////////////////////// crs_displaces ///////////////////////////////
  PerfomTest( 669 );
  ProgTrace( TRACE5, "trips count %zu", trips.size() );

  for ( TSOPPTrips::iterator tr=trips.begin(); tr!=trips.end(); tr++ ) {
    if ( !tr->places_out.empty() ) {
      // добор информации
      if ( module != tISG ) {
        cfg.get( tr->point_id, arx?tr->part_key:ASTRA::NoExists );
        tr->cfg.insert( tr->cfg.begin(), cfg.begin(), cfg.end() );
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
          if (TrferList::trferOutExists( tr->point_id, Trfer_inQry ))
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
      if (TrferList::trferInExists( tr->point_id, tr->trfer_out_point_id, Trfer_outQry))
        tr->TrferType.setFlag( trferIn );
   	}
   	if ( !arx && module == tSOPP && tr->pr_reg ) {
   		TripAlarms( tr->point_id, tr->Alarms );
    }
  }
  PerfomTest( 662 );
	if ( !errcity.empty() ) {
 	  ProgError(  STDLOG, "Invalid city errcity=%s", errcity.c_str() );
  }
  return errcity;
}


string internal_ReadData( TSOPPTrips &trips, TDateTime first_date, TDateTime next_date,
                          bool arx, TModule module, long int &exec_time, int point_id = NoExists )
{
	string errcity;
	TReqInfo *reqInfo = TReqInfo::Instance();

  if ( ( reqInfo->user.access.airlines.empty() && reqInfo->user.access.airlines_permit ) ||
       ( reqInfo->user.access.airps.empty() && reqInfo->user.access.airps_permit ) )
    return errcity;

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
    else {
  		if ( point_id == NoExists ) {
	  		PointsQry.SQLText = addCondition( points_SOPP_SQL, arx ).c_str();
      }
      else {
  			PointsQry.SQLText = points_id_SOPP_SQL;
  			PointsQry.CreateVariable( "point_id", otInteger, point_id );
   	  }
    }

  if ( point_id == NoExists ) {
    if ( first_date != NoExists ) {
      if ( 	TReqInfo::Instance()->user.sets.time == ustTimeLocalAirp ) { // локальные времена пульта в first_date, next_date
      	ProgTrace( TRACE5, "ustTimeLocalAirp!!!, first_date=%s, next_date=%s", DateTimeToStr( first_date-1 ).c_str(), DateTimeToStr( next_date+1 ).c_str() );
// вычитаем сутки, т.к. филтрация идет по UTC, а в случае режима локальных времен может быть переход на
                          // сутки и клиент этот рейс отфильтрует
                          // 20.04.2010 04:00 MEX -> 19.04.2010 22:00 LocalTime
        PointsQry.CreateVariable( "first_date", otDate, first_date-1 ); // UTC +- сутки
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

  TQuery Trfer_outQry( &OraSession );
  TQuery Trfer_inQry( &OraSession );

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
    boost::posix_time::ptime mcsTime = boost::posix_time::microsec_clock::universal_time();
    PointsQry.Execute();
    exec_time = (boost::posix_time::microsec_clock::universal_time() - mcsTime).total_microseconds();
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
	int col_part_key = -1;
	if ( arx )
		col_part_key = PointsQry.FieldIndex( "part_key" );
  vector<TSOPPTrip> vtrips;
  int fetch_count=0;
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
            	if ( module == tISG && reqInfo->desk.city != "ЧЛБ" ) {
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
    fetch_count++;
  } // end while !PointsQry.Eof
  ProgTrace( TRACE5, "fetch_count=%d", fetch_count );
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
          	if ( module == tISG && reqInfo->desk.city != "ЧЛБ" ) {
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
  // рейсы созданы, перейдем к набору информации по рейсам
  ////////////////////////// crs_displaces ///////////////////////////////
  PerfomTest( 669 );
  ProgTrace( TRACE5, "trips count %zu", trips.size() );

  TCFG cfg;

  for ( TSOPPTrips::iterator tr=trips.begin(); tr!=trips.end(); tr++ ) {
    if ( !tr->places_out.empty() ) {
      // добор информации
      if ( module != tISG ) {
        cfg.get( tr->point_id, arx?tr->part_key:ASTRA::NoExists );
        tr->cfg.insert( tr->cfg.begin(), cfg.begin(), cfg.end() );
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
          if (TrferList::trferOutExists( tr->point_id, Trfer_inQry ))
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
      if (TrferList::trferInExists( tr->point_id, tr->trfer_out_point_id, Trfer_outQry))
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
    if ( ( tr->places_in.empty() && tr->places_out.empty() ) ||
         tr->region.empty() ) // такой рейс не отображаем
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
    if ( !tr->cfg.empty() ) {
    	lNode = NewTextChild( tripNode, "classes" );
    	string str;
  	  for ( vector<TCFGItem>::iterator icfg=tr->cfg.begin(); icfg!=tr->cfg.end(); icfg++ ) {
  	  	if ( TReqInfo::Instance()->desk.compatible( LATIN_VERSION ) )
  	  	  SetProp( NewTextChild( lNode, "class", ElemIdToCodeNative( etClass, icfg->cls ) ), "cfg", icfg->cfg );
  	  	else {
          if ( !str.empty() )
            str += " ";
  		    str += ElemIdToCodeNative( etClass, icfg->cls ) + IntToString( icfg->cfg );
        }
  	  }
  	  if ( !TReqInfo::Instance()->desk.compatible( LATIN_VERSION ) )
        NodeSetContent( lNode, str );
    }
    else {
      if ( TReqInfo::Instance()->desk.compatible( LATIN_VERSION ) &&
           SALONS2::isFreeSeating( tr->point_id ) ) {
        lNode = NewTextChild( tripNode, "pr_free_seating", "-" );
      }
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
      	xmlNodePtr an=NewTextChild( alarmsNode, "alarm", EncodeAlarmType(alarm) );
        SetProp( an, "text", TripAlarmString( alarm ) );
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
    if ( ( tr->places_in.empty() && tr->places_out.empty() ) ||
         tr->region.empty() ) // такой рейс не отображаем
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

void IntReadTrips( XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode, long int &exec_time )
{
//  createCentringFile( 13672, "ASTRA", "DMDTST" );
  ProgTrace( TRACE5, "ReadTrips" );
  xmlNodePtr dataNode = NewTextChild( resNode, "data" );
  TModule module;
  
  bool pr_verify_new_select = true;//new select GetNode( "pr_verify_new_select", reqNode );
  ProgTrace( TRACE5, "pr_verify_new_select=%d", pr_verify_new_select );
  
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
    else { // работаем с UTC временами даты СОПП терминала
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
  string errcity;
  if ( pr_verify_new_select ) {
    string errcity = internal_ReadData_N( trips, first_date, next_date, arx, module, exec_time );
  }
  else {
    errcity = internal_ReadData( trips, first_date, next_date, arx, module, exec_time );
  }
  if ( module == tISG )
  	buildISG( trips, errcity, dataNode );
  else
    buildSOPP( trips, errcity, dataNode );
  if ( !errcity.empty() )
    AstraLocale::showErrorMessage( "MSG.CITY.REGION_NOT_DEFINED.NOT_ALL_FLIGHTS_ARE_SHOWN",
    	                             LParams() << LParam("city", ElemIdToCodeNative(etCity,errcity)));
}

void SoppInterface::ReadTrips(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  long int exe_time;
  IntReadTrips( ctxt, reqNode, resNode, exe_time );
}

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

  TrferList::TTrferType trferType;
  if (pr_inbound_tckin)
    trferType=TrferList::tckinInbound;
  else
  {
    if (pr_tlg)
    {
      if (pr_out)
        trferType=TrferList::trferOut;
      else
        trferType=TrferList::trferIn;
    }
    else
      trferType=TrferList::trferCkin;
  };

  TTripInfo flt;
  vector<TrferList::TGrpItem> grps_ckin;
  vector<TrferList::TGrpItem> grps_tlg;
  TrferList::TrferFromDB(trferType, point_id, pr_bag, flt, grps_ckin, grps_tlg);
  TrferList::TrferToXML(trferType, point_id, pr_bag, flt, grps_ckin, grps_tlg, resNode);
  
  get_new_report_form("SOPPTrfer", reqNode, resNode);
  STAT::set_variables(resNode);
};

struct TSegBSMInfo
{
  BSM::TBSMAddrs BSMaddrs;
  map<int/*grp_id*/,BSM::TTlgContent> BSMContentBefore;
};

void DeletePaxGrp( const TAdvTripInfo &fltInfo, int grp_id, bool toLog,
                   TQuery &PaxQry, TQuery &DelQry,
                   map<int/*point_id*/,TSegBSMInfo> &BSMsegs,
                   set<int/*point_id*/> &nextTrferSegs )
{
  int point_id=fltInfo.point_id;

  const char* pax_sql=
    "SELECT pax_id,surname,name,pers_type,reg_no,pr_brd,status "
    "FROM pax_grp, pax "
    "WHERE pax_grp.grp_id=pax.grp_id AND pax_grp.grp_id=:grp_id";
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
    "  DELETE FROM "
    "   (SELECT * "
    "    FROM pax_seats,pax,pax_grp "
    "    WHERE pax_seats.pax_id=pax.pax_id AND "
    "          pax.grp_id=pax_grp.grp_id AND "
    "          pax_grp.grp_id=:grp_id); "
    "  UPDATE pax SET refuse=:refuse,pr_brd=NULL WHERE grp_id=:grp_id; "
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
    DelQry.DeclareVariable("grp_id",otInteger);
  };
  
  //набираем вектор BSMsegs
  if (BSMsegs.find(fltInfo.point_id)==BSMsegs.end())
  {
    BSM::IsSend(fltInfo,BSMsegs[fltInfo.point_id].BSMaddrs);
  };
  
  TSegBSMInfo &BSMseg=BSMsegs[fltInfo.point_id];
  if (!BSMseg.BSMaddrs.empty())
  {
    BSM::TTlgContent BSMContent;
    BSM::LoadContent(grp_id,BSMContent);
    BSMseg.BSMContentBefore[grp_id]=BSMContent;
  };

  //набираем вектор nextTrferSegs
  set<int> ids;
  InboundTrfer::GetNextTrferCheckedFlts(grp_id, idGrp, ids);
  nextTrferSegs.insert(ids.begin(),ids.end());

  bool SyncPaxs=is_sync_paxs(point_id);

  PaxQry.SetVariable("grp_id",grp_id);
  PaxQry.Execute();
  for(;!PaxQry.Eof;PaxQry.Next())
  {
    const string surname=PaxQry.FieldAsString("surname");
    const string name=PaxQry.FieldAsString("name");
    const string pers_type=PaxQry.FieldAsString("pers_type");
    int pax_id=PaxQry.FieldAsInteger("pax_id");
    int reg_no=PaxQry.FieldAsInteger("reg_no");
    bool boarded=!PaxQry.FieldIsNULL("pr_brd") && PaxQry.FieldAsInteger("pr_brd")!=0;
    TPaxStatus status=DecodePaxStatus(PaxQry.FieldAsString("status"));

    if (SyncPaxs)
    {
      update_pax_change(point_id, pax_id, reg_no, "Р");
      if (boarded)
        update_pax_change(point_id, pax_id, reg_no, "П");
    };

    if (toLog)
    {
      std::string lexema_id;
      lexema_id = (status!=psCrew)?"EVT.PASSENGER_CANCEL_CHECKIN":"EVT.CREW_MEMBER_CANCEL_CHECKIN";
      TReqInfo::Instance()->LocaleToLog(lexema_id, LEvntPrms() << PrmSmpl<std::string>("surname", surname)
                                        << PrmSmpl<std::string>("name", (name.empty()?"":" ") + name)
                                        << PrmElem<std::string>("pers_type", etPersType, pers_type)
                                        << PrmSmpl<std::string>("reason", refuseAgentError),
                                        ASTRA::evtPax, point_id, reg_no, grp_id);
    };
  };

  DelQry.SetVariable("grp_id",grp_id);
  DelQry.Execute();
  
  rozysk::sync_pax_grp(grp_id, TReqInfo::Instance()->desk.code, TReqInfo::Instance()->user.descr);

  TQuery Qry(&OraSession);
	Qry.Clear();
	Qry.SQLText=
    "BEGIN "
    "  ckin.check_grp(:grp_id); "
    "END;";
  Qry.CreateVariable("grp_id", otInteger, grp_id);
  Qry.Execute();

};

void DeletePassengers( int point_id, const TDeletePaxFilter &filter,
                       map<int,TAdvTripInfo> &segs )
{
  segs.clear();
  TReqInfo *reqInfo = TReqInfo::Instance();

  TFlights flightsForLock;
  flightsForLock.Get( point_id, ftTranzit );
  flightsForLock.Lock();

  TQuery Qry(&OraSession);
	Qry.Clear();
	Qry.SQLText=
	  "SELECT airline,flt_no,suffix,airp,scd_out, "
	  "       point_id,point_num,first_point,pr_tranzit "
    "FROM points "
    "WHERE point_id=:point_id AND pr_reg<>0 AND pr_del=0";
  Qry.CreateVariable("point_id",otInteger,point_id);
  Qry.Execute();
  if (Qry.Eof) throw AstraLocale::UserException("MSG.FLIGHT.CHANGED.REFRESH_DATA");
  TAdvTripInfo fltInfo(Qry);

  map<int/*point_id*/,TSegBSMInfo> BSMsegs;
  set<int> nextTrferSegs;

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
    "ORDER BY seg_no";
  TCkinQry.DeclareVariable("tckin_id",otInteger);
  TCkinQry.DeclareVariable("seg_no",otInteger);

  ostringstream sql;
  sql << "SELECT pax_grp.grp_id, pax_grp.class, \n"
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
  
  segs.insert(make_pair(point_id, fltInfo));
  for(;!Qry.Eof;Qry.Next())
  {
    if (Qry.FieldIsNULL("class")) continue;  //несопровождаемый багаж не разрегистрируем!
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
  
    //пробегаемся по всем группам рейса
    int grp_id=Qry.FieldAsInteger("grp_id");

    //отвяжем сквозняков от предыдущих сегментов
    if (tckin_id!=NoExists && tckin_seg_no!=NoExists)
    {
      if (SeparateTCkin(grp_id,cssAllPrevCurr,cssNone,NoExists)!=NoExists)
      {
        //разрегистрируем все сквозные сегменты после нашего
        vector<int> point_ids;
        list< pair<int, int> > grp_ids;

        TCkinQry.SetVariable("tckin_id",tckin_id);
        TCkinQry.SetVariable("seg_no",tckin_seg_no);
        TCkinQry.Execute();
        for(;!TCkinQry.Eof;TCkinQry.Next())
        {
          //if (TCkinQry.FieldAsInteger("pr_depend")==0) break;
          //не учитываем связанность последующих сквозных сегментов - тупо разрегистрируем всех

          int tckin_point_id=TCkinQry.FieldAsInteger("point_id");
          int tckin_grp_id=TCkinQry.FieldAsInteger("grp_id");

          if (segs.find(tckin_point_id)==segs.end())
            segs.insert(make_pair(tckin_point_id, TAdvTripInfo(TCkinQry))).first;

          point_ids.push_back(tckin_point_id);
          grp_ids.push_back(make_pair(tckin_point_id, tckin_grp_id));
        };

        TFlights flightsForLock;
        flightsForLock.Get( point_ids, ftTranzit );
      	flightsForLock.Lock();

        for(list< pair<int/*point_id*/, int/*grp_id*/> >::const_iterator g=grp_ids.begin(); g!=grp_ids.end(); ++g)
        {
          map<int,TAdvTripInfo>::const_iterator f=segs.find(g->first);
          if (f==segs.end()) throw Exception("DeletePassengers: f==segs.end()");

          DeletePaxGrp( f->second, g->second, true, PaxQry, DelQry, BSMsegs, nextTrferSegs);
        };
      };
    };
    
    DeletePaxGrp( fltInfo, grp_id, filter.inbound_point_dep!=NoExists, PaxQry, DelQry, BSMsegs, nextTrferSegs);
  };

  //пересчитаем счетчики по всем рейсам, включая сквозные сегменты
  Qry.Clear();
	Qry.SQLText=
	 "BEGIN "
	 "  ckin.recount(:point_id); "
   "END;";
  Qry.DeclareVariable("point_id",otInteger);
  std::vector<int> points_check_wait_alarm;
  std::vector<int> points_tranzit_check_wait_alarm;
  for(map<int,TAdvTripInfo>::const_iterator i=segs.begin();i!=segs.end();++i)
  {
    Qry.SetVariable("point_id",i->first);
    Qry.Execute();
    check_overload_alarm( i->first );
    if ( SALONS2::isTranzitSalons( i->first ) ) {
      if ( find( points_tranzit_check_wait_alarm.begin(),
                 points_tranzit_check_wait_alarm.end(),
                 i->first ) == points_tranzit_check_wait_alarm.end() ) {
        points_tranzit_check_wait_alarm.push_back( i->first );
      }
    }
    else {
      if ( find( points_check_wait_alarm.begin(),
                 points_check_wait_alarm.end(),
                 i->first ) == points_check_wait_alarm.end() ) {
        points_check_wait_alarm.push_back( i->first );
      }
    }
    check_brd_alarm( i->first );
    check_spec_service_alarm( i->first );
    check_TrferExists( i->first );
    check_unattached_trfer_alarm( i->first );
    check_conflict_trfer_alarm( i->first );
    check_apis_alarms( i->first );
  };

  for ( std::vector<int>::iterator i=points_check_wait_alarm.begin();
        i!=points_check_wait_alarm.end(); i++ ) {
    check_waitlist_alarm(*i);
  }
  SALONS2::check_waitlist_alarm_on_tranzit_routes( points_tranzit_check_wait_alarm );
  check_unattached_trfer_alarm(nextTrferSegs);

  if ( filter.inbound_point_dep==NoExists )
  {
    if ( filter.status.empty() )
      reqInfo->LocaleToLog("EVT.PASSENGERS_AND_CREW_CANCEL_CHECKIN", evtPax, point_id);
    else
    {
      if (filter.status==EncodePaxStatus(psCrew))
        reqInfo->LocaleToLog("EVT.CREW_CANCEL_CHECKIN", evtPax, point_id);
      else
        reqInfo->LocaleToLog("EVT.PASSENGERS_WITH_STATUS_CANCEL_CHECKIN",
                             LEvntPrms() << PrmElem<std::string>("status", etGrpStatusType ,filter.status, efmtNameLong),
                             evtPax, point_id);
    };
  };

  //BSM
  for(map<int/*point_id*/,TSegBSMInfo>::const_iterator s=BSMsegs.begin();
                                                       s!=BSMsegs.end(); ++s)
  {
    if (s->second.BSMaddrs.empty()) continue; //для сегмента не заданы адреса отправки
    for(map<int/*grp_id*/,BSM::TTlgContent>::const_iterator i=s->second.BSMContentBefore.begin();
                                                            i!=s->second.BSMContentBefore.end(); ++i)
    {
      BSM::Send(s->first,i->first,i->second,s->second.BSMaddrs);
    };
  };
}

void DeletePassengersAnswer( map<int,TAdvTripInfo> &segs, xmlNodePtr resNode )
{
	TQuery Qry(&OraSession);
	Qry.Clear();
  Qry.SQLText=
    "SELECT pr_etstatus FROM trip_sets WHERE point_id=:point_id ";
  Qry.DeclareVariable("point_id",otInteger);

	xmlNodePtr segsNode=NewTextChild(resNode,"segments");
	for(map<int,TAdvTripInfo>::const_iterator i=segs.begin();i!=segs.end();++i)
  {
    bool pr_etl_only=GetTripSets(tsETLOnly,i->second);
    Qry.SetVariable("point_id",i->first);
    Qry.Execute();
    int pr_etstatus=-1;
    if (!Qry.Eof)
      pr_etstatus=Qry.FieldAsInteger("pr_etstatus");

      //здесь формируем список рейсов, на которых надо сделать смену статуса ЭБ
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
  map<int,TAdvTripInfo> segs;
	DeletePassengers( point_id, filter, segs );
  DeletePassengersAnswer( segs, resNode );
  if (filter.inbound_point_dep==NoExists)
  {
    //это разрегистрация всех пассажиров рейса
    TSOPPTrips trips;
    long int exec_time;
    string errcity = internal_ReadData( trips, NoExists, NoExists, false, tSOPP, exec_time, point_id );
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
    //это разрегистрация пассажиров, прибывающих стыковочным рейсом
    //отправляем обновленные данные
    TTripInfo flt;
    vector<TrferList::TGrpItem> grps_ckin;
    vector<TrferList::TGrpItem> grps_tlg;
    TrferList::TrferFromDB(TrferList::tckinInbound, point_id, true, flt, grps_ckin, grps_tlg);
    TrferList::TrferToXML(TrferList::tckinInbound, point_id, true, flt, grps_ckin, grps_tlg, resNode);
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
                string lexema_id;
                PrmEnum prmenum("names", ",");
                while ( stnode ) {
                    name = NodeAsString( stnode );
                    if ( find( terms.begin(), terms.end(), name ) == terms.end() ) {
                        terms.push_back( name );
                        Qry.SetVariable( "name", name );
                        pr_main = GetNode( "pr_main", stnode );
                        Qry.SetVariable( "pr_main", pr_main );
                        Qry.Execute();
                        if (pr_main) {
                          PrmLexema prmlexema("", "EVT.DESK_MAIN");
                          prmlexema.prms << PrmSmpl<std::string>("", name);
                          prmenum.prms << prmlexema;
                        }
                        else
                          prmenum.prms << PrmSmpl<std::string>("", name);
                        stnode = stnode->next;
                        if (lexema_id.empty() && work_mode == "Р")
                          lexema_id = "EVT.ASSIGNE_DESKS";
                        else if (lexema_id.empty() && work_mode == "П")
                          lexema_id = "EVT.ASSIGNE_BOARDING_GATES";
                    }
                }
                if ( work_mode == "Р" ) {
                  if (lexema_id.empty())
                    TReqInfo::Instance()->LocaleToLog("EVT.DESKS_NOT_ASSIGNED", evtFlt, point_id);
                  else
                    TReqInfo::Instance()->LocaleToLog(lexema_id, LEvntPrms() << prmenum, evtFlt, point_id);
                }
                if ( work_mode == "П" ) {
                  if (lexema_id.empty())
                    TReqInfo::Instance()->LocaleToLog("EVT.BOARDING_GATES_NOT_ASSIGNED", evtFlt, point_id);
                  else
                    TReqInfo::Instance()->LocaleToLog(lexema_id, LEvntPrms() << prmenum, evtFlt, point_id);
                }
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
                    TReqInfo::Instance()->LocaleToLog("EVT.CARGO_MAIL_WEIGHT", LEvntPrms()
                                                   << PrmElem<std::string>("airp", etAirp, Qry.GetVariableAsString( "airp_arv" ))
                                                   << PrmSmpl<int>("cargo_weight", cargo)
                                                   << PrmSmpl<int>("mail_weight", mail),
                                                   evtFlt, point_id);
                    load = load->next;
                }
            }
            if ( max_cNode || trip_loadNode ) { // были изменения в весе
                //проверим максимальную загрузку
                check_overload_alarm( point_id );
            }
        }
        on_change_trip( CALL_POINT, point_id );
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
	  "WHERE pax_grp.grp_id=pax.grp_id AND point_dep=:point_id AND pax_grp.status NOT IN ('E') AND pr_brd=0";
	Qry.CreateVariable( "point_id", otInteger, point_id );
	Qry.Execute();
	NewTextChild( node, "nobrd", Qry.FieldAsInteger( "nobrd" ) );
  Qry.SQLText =
    "SELECT sopp.get_birks(:point_id,:vlang) AS birks FROM dual";
  Qry.CreateVariable( "vlang", otString, TReqInfo::Instance()->desk.lang.empty() );
	Qry.Execute();
	NewTextChild( node, "birks", Qry.FieldAsString( "birks" ) );
}

void GetLuggage( int point_id, xmlNodePtr dataNode )
{
	xmlNodePtr node = NewTextChild( dataNode, "luggage" );
  PersWeightRules r;
  ClassesPersWeight weight;
  r.read( point_id );
	TFlightWeights w;
	w.read( point_id, withBrd, true );
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
		// сообщение в терминале о несоответствии фактической загрузке в терминале реальной в системе
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
  long int exec_time;
  string errcity = internal_ReadData( trips, NoExists, NoExists, false, tSOPP, exec_time, point_id );

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
  TTlgBinding tlgBinding(true);
  TTrferBinding trferBinding;
  tlgBinding.unbind_flt(point_ids);
  trferBinding.unbind_flt(point_ids);

  vector<TTripInfo> flts;
	TSOPPDests vdests;
	string reference;
	internal_ReadDests( move_id, vdests, reference, NoExists);
  // создаем все возможные рейсы из нового маршрута исключая удаленные пункты
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
  tlgBinding.bind_flt_oper(flts);
  trferBinding.bind_flt_oper(flts);
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

bool CheckApis_USA( const std::string &airp )
{
  TBaseTable &baseairps = base_tables.get( "airps" );
  TBaseTable &basecities = base_tables.get( "cities" );
  string country = ((TCitiesRow&)basecities.get_row( "code", ((TAirpsRow&)baseairps.get_row( "code", airp )).city)).country;
  return ( APIS::customsUS().find( country ) != APIS::customsUS().end() );
}

void check_trip_tasks( int move_id )
{
  TSOPPDests dests;
  string reference;
	internal_ReadDests( move_id, dests, reference, ASTRA::NoExists );
	check_trip_tasks( dests );
}

void check_trip_tasks( const TSOPPDests &dests )
{
  tst();
  TQuery Qry(&OraSession);
	Qry.SQLText =
	  "SELECT point_num,first_point,pr_tranzit,airp "
    " FROM points "
    " WHERE point_id=:point_id AND pr_del=0";
  Qry.DeclareVariable( "point_id", otInteger );

  vector<int> points;
  TTripRoute routes;
  vector<int> is_apis_airp;
  for ( TSOPPDests::const_reverse_iterator idest=dests.rbegin();
        idest!=dests.rend(); idest++ ) {
    Qry.SetVariable( "point_id", idest->point_id );
    Qry.Execute();
    if ( Qry.Eof ||
         find( is_apis_airp.begin(), is_apis_airp.end(), idest->point_id ) != is_apis_airp.end() ) {
      tst();
      continue;
    }
    ProgTrace( TRACE5, "CheckApis_USA(%s)=%d", Qry.FieldAsString( "airp" ), CheckApis_USA( Qry.FieldAsString( "airp" ) ) );
    if ( CheckApis_USA( Qry.FieldAsString( "airp" ) ) ) {
      ProgTrace( TRACE5, "CheckApis_USA(%s) return true", Qry.FieldAsString( "airp" ) );
      routes.clear();
      routes.GetRouteBefore( ASTRA::NoExists,
                             idest->point_id,
                             Qry.FieldAsInteger( "point_num" ),
                             Qry.FieldAsInteger( "first_point" ),
                             Qry.FieldAsInteger( "pr_tranzit" ),
                             trtNotCurrent,
                             trtNotCancelled ); // получили все предыдущие пункты транзитного рейса
      TTripRouteItem item;
      item.point_id = idest->point_id;
      item.point_num = Qry.FieldAsInteger( "point_num" );
      item.airp = Qry.FieldAsString( "airp" );
      item.pr_cancel = 0;
      routes.push_back( item );
      for ( TTripRoute::reverse_iterator item=routes.rbegin();
            item!=routes.rend(); item++ ) {
        ProgTrace( TRACE5, "item->point_id=%d", item->point_id );
        for ( ; idest!=dests.rend(); idest++ ) {
          if ( idest->point_id == item->point_id ) {
            tst();
            break;
          }
        }
        if ( idest == dests.rend() ) {
          throw Exception( "check_trip_tasks: idestp == dests.end()" );
        }
        if ( idest->pr_reg == 0 ||
             idest->pr_del != 0 ) {
          ProgTrace( TRACE5, "idest->point_id=%d, idest->pr_reg=%d, idest->pr_del=%d",
                     idest->point_id, idest->pr_reg, idest->pr_del );
          continue;
        }
        ProgTrace( TRACE5, "is_apis_airp.push_back(%d)", item->point_id );
        is_apis_airp.push_back( item->point_id );
      }
    }
  }
  Qry.Clear();
  Qry.SQLText =
    "BEGIN "
    " IF :is_apis=0 THEN "
    "   DELETE trip_tasks WHERE point_id=:point_id AND name=:name;"
    " ELSE "
    "   UPDATE trip_tasks SET next_exec=( SELECT NVL(est_out,scd_out)-:before_minutes/(24*60) FROM points WHERE point_id=:point_id)"
    "    WHERE point_id=:point_id AND name=:name; "
    "    IF SQL%ROWCOUNT=0 THEN "
    "      INSERT INTO trip_tasks(id,point_id,name,last_exec,next_exec) "
    "       SELECT cycle_id__seq.nextval,point_id,:name,NULL, NVL(est_out,scd_out)-:before_minutes/(24*60) "
    "        FROM points WHERE point_id=:point_id; "
    "    END IF;"
    " END IF; "
    "END;";
  Qry.DeclareVariable( "is_apis", otInteger );
  Qry.DeclareVariable( "point_id", otInteger );
  Qry.DeclareVariable( "name", otString );
  Qry.DeclareVariable( "before_minutes", otInteger );
  
  for ( TSOPPDests::const_iterator idest=dests.begin();
        idest!=dests.end(); idest++ ) {
    if ( idest->act_out != ASTRA::NoExists ) {
      continue;
    }
    Qry.SetVariable( "is_apis", find( is_apis_airp.begin(), is_apis_airp.end(),idest->point_id ) != is_apis_airp.end() );
    Qry.SetVariable( "point_id", idest->point_id );
    Qry.SetVariable( "name", BEFORE_TAKEOFF_30_US_CUSTOMS_ARRIVAL );
    Qry.SetVariable( "before_minutes", 30 );
    Qry.Execute();
    Qry.SetVariable( "name", BEFORE_TAKEOFF_60_US_CUSTOMS_ARRIVAL );
    Qry.SetVariable( "before_minutes", 60 );
    Qry.Execute();
    Qry.SetVariable( "name", BEFORE_TAKEOFF_70_US_CUSTOMS_ARRIVAL );
    Qry.SetVariable( "before_minutes", 70 );
    Qry.Execute();
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
  	if ( id->point_num == NoExists || id->pr_del == -1 ) { // вставка или удаление пункта посадки
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
  // проверки
  // если это работник аэропорта, то в маршруте должен быть этот порт,
  // если работник авиакомпании, то авиакомпания
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
    if ( !canDo ) {
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
    	else { // список запрещенных аэропортов
    		string airps;
    		for ( vector<string>::iterator s=reqInfo->user.access.airps.begin(); s!=reqInfo->user.access.airps.end(); s++ ) {
    		  if ( !airps.empty() )
    		    airps += " ";
    		  airps += ElemIdToCodeNative(etAirp,*s);
    		}
            throw AstraLocale::UserException( "MSG.ROUTE.MUST_CONTAIN_ONE_OF_AIRPS_OTHER_THAN", LParams() << LParam("list", airps));
    	}
    }
  }
  try {
    // проверка на отмену + в маршруте участвует всего одна авиакомпания
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

    // отменяем все п.п., т.к. в маршруте всего один не отмененный
    if ( notCancel == 1 ) {
      for( TSOPPDests::iterator id=dests.begin(); id!=dests.end(); id++ ) {
       	if ( !id->pr_del ) {
      		id->pr_del = 1;
      		id->modify = true;
        }
      }
    }
    // проверка упорядоченности времен + дублирование рейса, если move_id == NoExists
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
      	   doubletrip.IsExists( move_id, id->airline, id->flt_no, id->suffix, id->airp, id->scd_in, id->scd_out, point_id ) ) { //??? почему идет сравнение локальных времен в СОПП??? в Сезонке сравнение в UTC!!!
      	existsTrip = true;
      	break;
      }
      if ( id->pr_del != -1 && id != dests.end() && id->est_out != id->scd_out &&
           ( !id->delays.empty() || id->est_out != NoExists ) ) {
        TTripInfo info;
        info.airline = id->airline;
        info.flt_no = id->flt_no;
        info.airp = id->airp;
        if ( GetTripSets( tsCheckMVTDelays, info ) ) { //проверка задержек на совместимость с телеграммами
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
  // задание параметров pr_tranzit, pr_reg, first_point
  TSOPPDests::iterator pid=dests.end();
  int lock_point_id = NoExists;
  bool pr_check_apis_usa=false;
  for( TSOPPDests::iterator id=dests.begin(); id!=dests.end(); id++ ) {
    if ( lock_point_id == NoExists && id->point_id != NoExists ) {
      lock_point_id = id->point_id;
    }
    if ( CheckApis_USA( id->airp ) ) {
      pr_check_apis_usa = true;
    }
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
  /* необходимо сделать проверку на не существование рейса*/
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
    TFlights flights;
    flights.Get( lock_point_id, ftAll );
    flights.Lock();
    Qry.Clear();
    Qry.SQLText =
     "UPDATE move_ref SET reference=:reference WHERE move_id=:move_id ";
    Qry.CreateVariable( "move_id", otInteger, move_id );
    Qry.CreateVariable( "reference", otString, reference );
    Qry.Execute();
  }
  bool ch_dests = false;
  int new_tid;
  bool init_trip_stages;
  //bool set_act_out;
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
  string lexema_id;
  PrmEnum prmenum("flt", "");
  TBaseTable &baseairps = base_tables.get( "airps" );
  vector<int> points_MVTdelays;
  std::vector<int> points_check_wait_alarm;
  std::vector<int> points_tranzit_check_wait_alarm;
  std::vector<int> points_check_diffcomp_alarm;
  bool conditions_check_apis_usa = false;
  for( TSOPPDests::iterator id=dests.begin(); id!=dests.end(); id++ ) {
  	set_pr_del = false;
  	//set_act_out = false;
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
    bool pr_check_wait_list_alarm = ch_point_num;
    bool pr_check_diffcomp_alarm = false;
    TSOPPDest old_dest;
    if ( id->pr_del != -1 ) {
      if (lexema_id.empty())
        if (insert)
          lexema_id = "EVT.FLIGHT.NEW";
        else
          lexema_id = "EVT.FLIGHT.MODIFY_ROUTE";
      else
        prmenum.prms << PrmSmpl<string>("", "-");
        if ( id->flt_no != NoExists )
          prmenum.prms << PrmFlight("", id->airline, id->flt_no, id->suffix) << PrmSmpl<string>("", " ")
                       << PrmElem<std::string>("", etAirp, id->airp);
        else
          prmenum.prms << PrmElem<std::string>("", etAirp, id->airp);
    }

    if ( insert_point ) {
    	ch_craft = false;
    	conditions_check_apis_usa = true;
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
         reqInfo->LocaleToLog("EVT.INPUT_NEW_POINT", LEvntPrms() << PrmFlight("flt", id->airline, id->flt_no, id->suffix)
                              << PrmElem<std::string>("airp", etAirp, id->airp), evtDisp, move_id, id->point_id );
       else
         reqInfo->LocaleToLog("EVT.INPUT_NEW_POINT", LEvntPrms() << PrmSmpl<std::string>("flt", "")
                              << PrmElem<std::string>("airp", etAirp, id->airp),
                              evtDisp, move_id, id->point_id );
       reSetCraft = true;
       reSetWeights = true;
       pr_check_wait_list_alarm = true;
       pr_check_diffcomp_alarm = true;
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

  	  change_stages_out = ( !insert_point && ( id->est_out != old_dest.est_out ||
                                               (id->scd_out != old_dest.scd_out && old_dest.scd_out > NoExists) ) );

  	  if ( !old_dest.pr_reg && id->pr_reg && id->pr_del != -1 ) {
  	    Qry.Clear();
  	    Qry.SQLText = "SELECT COUNT(*) c FROM trip_stages WHERE point_id=:point_id AND rownum<2";
  	    Qry.CreateVariable( "point_id", otInteger, id->point_id );
  	    Qry.Execute();
  	    init_trip_stages = !Qry.FieldAsInteger( "c" );
  	    ProgTrace( TRACE5, "init_trip_stages=%d", init_trip_stages );
  	    conditions_check_apis_usa = true;
  	  }
  	  else
  	  	init_trip_stages = false;

  	  #ifdef NOT_CHANGE_AIRLINE_FLT_NO_SCD
  	  if ( id->pr_del!=-1 && !id->airline.empty() && !old_dest.airline.empty() && id->airline != old_dest.airline ) {
  	  	throw AstraLocale::UserException( "MSG.ROUTE.CANNOT_CHANGE_AIRLINE" );
/*          reqInfo->LocaleToLog("EVT.DISP.CHANGE_AIRLINE", LEvntPrms() << PrmElem<std::string>("old_airline", etAirline, old_dest.airline)
                            << PrmElem<std::string>("airline", etAirline, id->airline) << PrmElem<std::string>("airp", etAirp, id->airp),
                            evtDisp, move_id, id->point_id ); */
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
  	  	pr_check_wait_list_alarm = true;
  	  	pr_check_diffcomp_alarm = true;
  	  }
  	  if ( id->pr_del != -1 && !id->craft.empty() && id->craft != old_dest.craft && !old_dest.craft.empty() ) {
  	  	ch_craft = true;
  	  	if ( !old_dest.craft.empty() ) {
  	  		if ( !id->remark.empty() )
  	  			id->remark += " ";
  	  	  id->remark += "изм. типа ВС с " + old_dest.craft; //!!!locale
  	  	  if ( !id->craft.empty() )
              reqInfo->LocaleToLog("EVT.MODIFY_CRAFT_TYPE", LEvntPrms() << PrmElem<std::string>("craft", etCraft, id->craft)
                                   << PrmElem<std::string>("airp", etAirp, id->airp), evtDisp, move_id, id->point_id );
  	  	}
  	  	else {
            reqInfo->LocaleToLog("EVT.ASSIGNE_CRAFT_TYPE", LEvntPrms() << PrmElem<std::string>("craft", etCraft, id->craft)
                                 << PrmElem<std::string>("airp", etAirp, id->airp), evtDisp, move_id, id->point_id );
  	  	}
  	  	reSetCraft = true;
  	  	reSetWeights = true;
  	  	pr_check_wait_list_alarm = true;
  	  	pr_check_diffcomp_alarm = true;
  	  }
  	  if ( id->pr_del != -1 && id->bort != old_dest.bort ) {
  	  	if ( !old_dest.bort.empty() ) {
  	  		if ( !id->remark.empty() )
  	  			id->remark += " ";
  	  	  id->remark += "изм. борта с " + old_dest.bort; //!!!locale
  	  	  if ( !id->bort.empty() )
              reqInfo->LocaleToLog("EVT.MODIFY_BOARD_TYPE", LEvntPrms() << PrmSmpl<std::string>("bort", id->bort)
                                << PrmElem<std::string>("airp", etAirp, id->airp), evtDisp, move_id, id->point_id);
  	  	}
  	  	else {
            reqInfo->LocaleToLog("EVT.ASSIGNE_BOARD_TYPE", LEvntPrms() << PrmSmpl<std::string>("bort", id->bort)
                                 << PrmElem<std::string>("airp", etAirp, id->airp), evtDisp, move_id, id->point_id );
  	  	}
  	  	reSetCraft = true;
  	  	reSetWeights = true;
  	  	pr_check_wait_list_alarm = true;
  	  	pr_check_diffcomp_alarm = true;
      }
      if ( id->pr_reg != old_dest.pr_reg || id->pr_del != old_dest.pr_del ||
           id->pr_tranzit != old_dest.pr_tranzit ) {
        reSetCraft = true;
        reSetWeights = true;
        pr_check_wait_list_alarm = true;
        pr_check_diffcomp_alarm = true;
        conditions_check_apis_usa = true;
      }
  	  if ( id->pr_del != old_dest.pr_del ) {
  	  	if ( id->pr_del == 1 )
            reqInfo->LocaleToLog("EVT.DISP.CANCEL_POINT", LEvntPrms() << PrmSmpl<std::string>("flt", "")
                                 << PrmElem<std::string>("airp", etAirp, id->airp), evtDisp, move_id, id->point_id);
  	  	else
  	  		if ( id->pr_del == 0 )
                reqInfo->LocaleToLog("EVT.DISP.RETURN_POINT", LEvntPrms() << PrmSmpl<std::string>("flt", "")
                                     << PrmElem<std::string>("airp", etAirp, id->airp), evtDisp, move_id, id->point_id );
	  	  	else
	  	  		if ( id->pr_del == -1 ) {
   	      	  id->point_num = 0-id->point_num-1;
    	      	ProgTrace( TRACE5, "point_num=%d", id->point_num );
          		DelQry.SetVariable( "move_id", move_id );
    	      	DelQry.SetVariable( "point_num", id->point_num );
    		      DelQry.Execute();
              if ( id->flt_no != NoExists )
                  reqInfo->LocaleToLog("EVT.DISP.DELETE_POINT", LEvntPrms() << PrmFlight("flt", id->airline, id->flt_no, id->suffix)
                                       << PrmElem<std::string>("airp", etAirp, id->airp), evtDisp, move_id, id->point_id );
              else
                  reqInfo->LocaleToLog("EVT.DISP.DELETE_POINT", LEvntPrms() << PrmSmpl<std::string>("flt", "")
                                       << PrmElem<std::string>("airp", etAirp, id->airp), evtDisp, move_id, id->point_id );
              pr_check_wait_list_alarm = true;
	  	  		}
         pr_check_wait_list_alarm = true;
         pr_check_diffcomp_alarm = true;
         conditions_check_apis_usa = true;
  	  }
  	  else
  	    if ( !id->pr_del && id->act_out != old_dest.act_out && old_dest.act_out > NoExists ) {
            reqInfo->LocaleToLog("EVT.DISP.MODIFY_TAKEOFF_ACT", LEvntPrms() << PrmDate("time", id->act_out, "hh:nn dd.mm.yy (UTC)")
                                 << PrmElem<std::string>("airp", etAirp, id->airp), evtDisp, move_id, id->point_id );
   		    change_act A;
  		    A.point_id = id->point_id;
  		    A.old_act = old_dest.act_out;
  		    A.act = id->act_out;
  		    A.pr_land = false;
  		    vchangeAct.push_back( A );
  		    conditions_check_apis_usa = true;
  	    }
     if ( (!id->airline.empty() && !old_dest.airline.empty() && id->airline != old_dest.airline) ||
          (!id->airline.empty() && !old_dest.airline.empty() && id->flt_no != old_dest.flt_no) ||
          (!id->airline.empty() && !old_dest.airline.empty() && id->suffix != old_dest.suffix) ) {
         reqInfo->LocaleToLog("EVT.FLIGHT.MODIFY_ATTRIBUTES_FROM", LEvntPrms() << PrmFlight("flt", old_dest.airline, old_dest.flt_no, old_dest.suffix)
                              << PrmFlight("new_flt", id->airline, id->flt_no, id->suffix) << PrmElem<std::string>("airp", etAirp, id->airp),
                              evtDisp, move_id, id->point_id);
       pr_check_wait_list_alarm = true;
       pr_check_diffcomp_alarm = true;
     }
 	   if ( !insert_point && id->pr_del!=-1 && id->scd_out > NoExists && old_dest.scd_out > NoExists && id->scd_out != old_dest.scd_out ) {
           reqInfo->LocaleToLog("EVT.DISP.MODIFY_TAKEOFF_PLAN", LEvntPrms() << PrmDate("time", id->scd_out, "hh:nn dd.mm.yy (UTC)")
                                 << PrmElem<std::string>("airp", etAirp, id->airp), evtDisp, move_id, id->point_id);
  	  	reSetWeights = true;
  	  	conditions_check_apis_usa = true;
  	 }
 	   if ( !insert_point && id->pr_del!=-1 && id->scd_out == NoExists && old_dest.scd_out > NoExists ) {
           reqInfo->LocaleToLog("EVT.DISP.DELETE_TAKEOFF_PLAN", LEvntPrms() << PrmDate("time", old_dest.scd_out, "hh:nn dd.mm.yy (UTC)")
                                << PrmElem<std::string>("airp", etAirp, id->airp), evtDisp, move_id, id->point_id);
  	  	reSetWeights = true;
  	  	conditions_check_apis_usa = true;
  	 }
 	   if ( !insert_point && id->pr_del!=-1 && id->scd_out > NoExists && old_dest.scd_out == NoExists ) {
        reqInfo->LocaleToLog("EVT.DISP.SET_TAKEOFF_PLAN", LEvntPrms() << PrmDate("time", id->scd_out, "hh:nn dd.mm.yy (UTC)")
                          << PrmElem<std::string>("airp", etAirp, id->airp), evtDisp, move_id, id->point_id);
  	  	reSetWeights = true;
  	  	conditions_check_apis_usa = true;
  	 }

    	set_pr_del = ( !old_dest.pr_del && id->pr_del );
    	//ProgTrace( TRACE5, "set_pr_del=%d", set_pr_del );
  	  //set_act_out = ( !id->pr_del && old_dest.act_out == NoExists && id->act_out > NoExists );
      if ( !id->pr_del && old_dest.act_out == NoExists && id->act_out > NoExists ) { //проставление факта вылета
        pr_check_wait_list_alarm = true;
        reqInfo->LocaleToLog("EVT.DISP.SET_TAKEOFF_ACT",  LEvntPrms() << PrmDate("time", id->act_out, "hh:nn dd.mm.yy (UTC)")
                             << PrmElem<std::string>("airp", etAirp, id->airp), evtDisp, move_id, id->point_id );
    		change_act A;
    		A.point_id = id->point_id;
  	  	A.old_act = old_dest.act_out;
  		  A.act = id->act_out;
  		  A.pr_land = false;
  		  vchangeAct.push_back( A );
  		  conditions_check_apis_usa = true;
      }
    	if ( !id->pr_del && old_dest.act_in == NoExists && id->act_in > NoExists ) {
            reqInfo->LocaleToLog("EVT.DISP.SET_LANDING_ACT", LEvntPrms() << PrmDate("time", id->act_in, "hh:nn dd.mm.yy (UTC)")
                                 << PrmElem<std::string>("airp", etAirp, id->airp), evtDisp, move_id, id->point_id );
    		change_act A;
    		A.point_id = id->point_id;
    		A.old_act = old_dest.act_in;
    		A.act = id->act_in;
    		A.pr_land = true;
    		vchangeAct.push_back( A );
    	}
    	if ( !id->pr_del && id->act_in != old_dest.act_in && old_dest.act_in > NoExists ) {
            reqInfo->LocaleToLog("EVT.DISP.MODIFY_LANDING_ACT", LEvntPrms() << PrmDate("time", id->act_in, "hh:nn dd.mm.yy (UTC)")
                                 << PrmElem<std::string>("airp", etAirp, id->airp), evtDisp, move_id, id->point_id);
    		change_act A;
    		A.point_id = id->point_id;
    		A.old_act = old_dest.act_in;
    		A.act = id->act_in;
    		A.pr_land = true;
    		vchangeAct.push_back( A );
    	}
    	if ( id->pr_del != -1 && old_dest.pr_del != -1 && id->pr_tranzit != old_dest.pr_tranzit ) {
        if ( id->pr_tranzit )
          reqInfo->LocaleToLog("EVT.SET_TRANSIT_FLAG", LEvntPrms() << PrmElem<std::string>("airp", etAirp, id->airp),
                            evtDisp, move_id, id->point_id);
        else
          reqInfo->LocaleToLog("EVT.CANCEL_TRANSIT_FLAG", LEvntPrms() << PrmElem<std::string>("airp", etAirp, id->airp),
                            evtDisp, move_id, id->point_id);
        pr_check_wait_list_alarm = true;
        pr_check_diffcomp_alarm = true;
        conditions_check_apis_usa = true;
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
  	
  	if ( pr_check_wait_list_alarm ) {
      ProgTrace( TRACE5, "pr_check_wait_list_alarm set point_id=%d", id->point_id );
  	  if ( SALONS2::isTranzitSalons( id->point_id ) ) {
        points_tranzit_check_wait_alarm.push_back( id->point_id );
  	  }
  	  else {
  	    points_check_wait_alarm.push_back( id->point_id );
      }
  	}
  	
  	if ( pr_check_diffcomp_alarm ) {
      points_check_diffcomp_alarm.push_back( id->point_id );
  	}

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
  	if ( !old_dest.remark.empty() ) {
  		if ( !id->remark.empty() ) {
  		  id->remark += " " + old_dest.remark;
      }
  		else {
  			id->remark = old_dest.remark;
      }
    }
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
      if( !delays.Empty() ) {
        ProgTrace( TRACE5, "points_MVTdelays insert point_id=%d", id->point_id );
        points_MVTdelays.push_back( id->point_id );
      }
  	}
  	if ( init_trip_stages ) {
      set_flight_sets(id->point_id);
  	}
  	else { //!!!возможно изменился признак ignore_auto в trip_stages
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
        conditions_check_apis_usa = true;
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
  	    double f;
  	    if ( !Qry.VariableIsNULL( "vscd" ) ) {
  	      t1 = Qry.GetVariableAsDateTime( "vscd" );
  	      t2 = Qry.GetVariableAsDateTime( "vest" );
  	      t1 = t2-t1;
  	      if ( t1 < 0 ) {
  	      	modf( t1, &f );
  		      if ( f )
                reqInfo->LocaleToLog("EVT.TECHNOLOGY_SCHEDULE_AHEAD", LEvntPrms() << PrmSmpl<int>("val", (int)f)
                                     << PrmDate("time", fabs(t1), "hh:nn") << PrmElem<std::string>("airp", etAirp, id->airp),
                                     evtGraph, id->point_id);
          }
  	      if ( t1 >= 0 ) {
            modf( t1, &f );
  	      	if ( t1 ) {
              if ( f )
                reqInfo->LocaleToLog("EVT.TECHNOLOGY_SCHEDULE_DELAY", LEvntPrms() << PrmSmpl<int>("val", (int)f)
                                     << PrmDate("time", t1, "hh:nn") << PrmElem<std::string>("airp", etAirp, id->airp), evtFlt, id->point_id);
            }
            else
              reqInfo->LocaleToLog("EVT.TECHNOLOGY_SCHEDULE_DELAY_CANCEL", LEvntPrms() <<
                                   PrmElem<std::string>("airp", etAirp, id->airp), evtFlt, id->point_id);
  	      }
        }
      }
    }
/*10.09.13    if ( set_act_out ) {
    	// еще point_num не записан
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
    	reqInfo->MsgToLog( string( "Проставление факт. вылета " ) + DateTimeToStr( id->act_out, "hh:nn dd.mm.yy (UTC)" ) + " порт " + id->airp, evtDisp, move_id, id->point_id );
  		change_act A;
  		A.point_id = id->point_id;
  		A.old_act = old_dest.act_out;
  		A.act = id->act_out;
  		A.pr_land = false;
  		vchangeAct.push_back( A );
 	  }*/
  	if ( set_pr_del ) {
  		ch_dests = true;
  		Qry.Clear();
  		Qry.SQLText =
  		"SELECT COUNT(*) c FROM "
  		"( SELECT 1 FROM pax_grp,points "
  		"   WHERE points.point_id=:point_id AND "
  		"         point_dep=:point_id AND pax_grp.status NOT IN ('E') AND bag_refuse=0 AND rownum<2 "
  		"  UNION "
  		" SELECT 2 FROM pax_grp,points "
  		"   WHERE points.point_id=:point_id AND "
  		"         point_arv=:point_id AND pax_grp.status NOT IN ('E') AND bag_refuse=0 AND rownum<2 ) ";
  		Qry.CreateVariable( "point_id", otInteger, id->point_id );
  		Qry.Execute();
  		if ( Qry.FieldAsInteger( "c" ) ) {
  			if ( id->pr_del == -1 )
  				throw AstraLocale::UserException( "MSG.ROUTE.UNABLE_DEL_AIRP.PAX_EXISTS",
  					                                 LParams() << LParam("airp", ElemIdToCodeNative(etAirp,id->airp)));
  			else
  				throw AstraLocale::UserException( "MSG.ROUTE.UNABLE_CANCEL_AIRP.PAX_EXISTS",
  					                                 LParams() << LParam("airp", ElemIdToCodeNative(etAirp,id->airp)));
      }
  	}
    if ( reSetCraft && id->pr_del == 0 ) {
      ProgTrace( TRACE5, "reSetCraft: point_id=%d", id->point_id );
      setcraft_points.push_back( id->point_id );
    }
    if ( reSetWeights && id->pr_del == 0 ) {
      ProgTrace( TRACE5, "reSetWeights: point_id=%d", id->point_id );
      PersWeightRules newweights;
      persWeights.getRules( id->point_id, newweights );
      PersWeightRules oldweights;
      oldweights.read( id->point_id );
      if ( !oldweights.equal( &newweights ) ) {
        newweights.write( id->point_id );
      }
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
  //exec_stages!!!
  
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
  
  if ( reSetCraft )
    AstraLocale::showErrorMessage( "MSG.DATA_SAVED.CRAFT_CHANGED.NEED_SET_COMPON" );
  else
    AstraLocale::showMessage( "MSG.DATA_SAVED" );
  //новая отвязка телеграмм
  ReBindTlgs( move_id, voldDests );
  //тревога различие компоновок
  for ( std::vector<int>::iterator i=points_check_diffcomp_alarm.begin();
        i!=points_check_diffcomp_alarm.end(); i++ ) {
    SALONS2::check_diffcomp_alarm(*i);
  }
  //тревога ЛО
  for ( std::vector<int>::iterator i=points_check_wait_alarm.begin();
        i!=points_check_wait_alarm.end(); i++ ) {
    check_waitlist_alarm(*i);
  }
  SALONS2::check_waitlist_alarm_on_tranzit_routes( points_tranzit_check_wait_alarm );

  for( vector<change_act>::iterator i=vchangeAct.begin(); i!=vchangeAct.end(); i++ ){
    if ( i->pr_land ) {
      ChangeACT_IN( i->point_id, i->old_act, i->act );  //телеграммы
    }
    else {
      ChangeACT_OUT( i->point_id, i->old_act, i->act ); //телеграммы + stage=Takeoff
    }
  }
  
  //ProgTrace( TRACE5, "ch_dests=%d, insert=%d, change_dests_msg=%s", ch_dests, insert, change_dests_msg.c_str() );
  if ( !ch_dests && !insert )
    lexema_id.clear();
  if ( !lexema_id.empty() )
    reqInfo->LocaleToLog(lexema_id, LEvntPrms() << prmenum, evtDisp, move_id);
    
  vector<TSOPPTrip> trs1, trs2;

  // создаем все возможные рейсы из нового маршрута исключая удаленные пункты
  for( TSOPPDests::iterator i=dests.begin(); i!=dests.end(); i++ ) {
  	if ( i->pr_del == -1 ) continue;
  	TSOPPTrip t = createTrip( move_id, i, dests );
  	ProgTrace( TRACE5, "t.pr_del=%d, t.point_id=%d, t.places_out.size()=%zu, t.suffix_out=%s",
  	                   t.pr_del, t.point_id, t.places_out.size(), t.suffix_out.c_str() );
  	trs1.push_back(t);
  }
  // создаем всевозможные рейсы из старого маршрута исключая удаленные пункты
  for( TSOPPDests::iterator i=voldDests.begin(); i!=voldDests.end(); i++ ) {
  	if ( i->pr_del == -1 ) continue;
  	TSOPPTrip t = createTrip( move_id, i, voldDests );
  	ProgTrace( TRACE5, "t.pr_del=%d, t.point_id=%d, t.places_out.size()=%zu",
                       t.pr_del, t.point_id, t.places_out.size() );
  	trs2.push_back(t);
  }

  // пробег по новым рейсам
  for (vector<TSOPPTrip>::iterator i=trs1.begin(); i!=trs1.end(); i++ ) {
  	vector<TSOPPTrip>::iterator j=trs2.begin();
  	for (; j!=trs2.end(); j++ ) // ищем в старом нужный рейс
  	  if ( i->point_id == j->point_id )
  	  	break;
    if ( j == trs2.end() && i->places_out.size() > 0 ) {
    	TSOPPTrip tr2;
    	BitSet<TSOPPTripChange> FltChange;
    	FltChange.setFlag( tsNew );
 	  	if ( i->pr_del >= 1 ) {
 	  		FltChange.setFlag( tsCancelFltOut ); // отмена вылета
 	  	}
    	ChangeTrip( i->point_id, *i, tr2, FltChange ); // это новый рейс на вылет
    }
    if ( j != trs2.end() ) { // это не новый рейс
    	BitSet<TSOPPTripChange> FltChange;
    	bool pr_f=false;
    	if ( i->places_out.size() && j->places_out.size() ) { // Есть и был вылет
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
    	  		FltChange.setFlag( tsCancelFltOut ); // отмена вылета
    	  		pr_f = true;
    	  	}
    	  	if ( j->pr_del >= 1 ) {
    	  		FltChange.setFlag( tsRestoreFltOut ); // восстановление вылета
    	  		pr_f = true;
    	  	}
    	  }
      }
      if ( i->places_out.size() && !j->places_out.size() ) {
      	FltChange.setFlag( tsAddFltOut ); //стал вылет
      	pr_f = true;
      	if ( i->pr_del >= 1 )
      		FltChange.setFlag( tsCancelFltOut ); // вылет отменен
      }
      if ( !i->places_out.size() && j->places_out.size() ) {
        FltChange.setFlag( tsDelFltOut ); // не стало вылета
        pr_f = true;
      }
      if ( pr_f )
        ChangeTrip( i->point_id, *i, *j, FltChange );
      trs2.erase( j ); // удаляем его из списка
    }
  }
  // пробег по старым рейсам, которых нет в новых
  for(vector<TSOPPTrip>::iterator j=trs2.begin(); j!=trs2.end(); j++ ) {
  	if ( j->places_out.size() && j->pr_del_out == 0 ) { // рейс не был отменен
  		TSOPPTrip tr2;
  		BitSet<TSOPPTripChange> FltChange;
  		FltChange.setFlag( tsDelete );
  	  ChangeTrip( j->point_id, tr2, *j, FltChange );  // рейс на вылет удален
  	}
  }
  // отправка телеграмм задержек
  for ( vector<int>::iterator pdel=points_MVTdelays.begin(); pdel!=points_MVTdelays.end(); pdel++ ) {
      try {
          vector<TypeB::TCreateInfo> createInfo;
          TypeB::TMVTCCreator(*pdel).getInfo(createInfo);
          TelegramInterface::SendTlg(createInfo);
      }
      catch(std::exception &E) {
          ProgError(STDLOG,"internal_WriteDests.SendTlg (point_id=%d): %s",*pdel,E.what());
      };
  }
  if ( pr_check_apis_usa && //в маршруте есть или была нужная страна
       conditions_check_apis_usa ) { //зменились атрибуты - изменилось время вылета,маршрут,признак транзита,признак регистрации,признак отмены/удаления
    try {
      check_trip_tasks( dests );
    }
    catch(std::exception &E) {
      ProgError(STDLOG,"internal_WriteDests.check_trip_tasks (move_id=%d): %s",move_id,E.what());
    };
  }
  for( TSOPPDests::iterator i=dests.begin(); i!=dests.end(); i++ ) {
    on_change_trip( CALL_POINT, i->point_id );
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
			//удаляем все невидимые смиволы
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
        //throw UserException( "Плановое время прибытия в пункте %s не определено однозначно", d.airp.c_str() );
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
        //throw UserException( "Расчетное время прибытия в пункте %s не определено однозначно", d.airp.c_str() );
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
//        throw UserException( "Фактическое время прибытия в пункте %s не определено однозначно", d.airp.c_str() );
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
        //throw UserException( "Плановое время вылета в пункте %s не определено однозначно", d.airp.c_str() );
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
        //throw UserException( "Расчетное время вылета в пункте %s не определено однозначно", d.airp.c_str() );
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
        //throw UserException( "Фактическое время вылета в пункте %s не определено однозначно", d.airp.c_str() );
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
          //throw UserException( "Время задержки в пункте %s не определено однозначно", d.airp.c_str() );
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
  TFlights flights;
  flights.Get( point_id, ftAll );  //весь маршрут
  flights.Lock();

  TQuery Qry(&OraSession);
	Qry.SQLText=
      "SELECT move_id,airline,flt_no,suffix,act_out,point_num,pr_del "
    "FROM points "
    "WHERE point_id=:point_id AND pr_del!=-1 AND act_out IS NOT NULL";
  Qry.CreateVariable("point_id",otInteger,point_id);
  Qry.Execute();
  if (Qry.Eof) throw AstraLocale::UserException("MSG.FLIGHT.CHANGED.REFRESH_DATA");
	int point_num = Qry.FieldAsInteger( "point_num" );
	int move_id = Qry.FieldAsInteger( "move_id" );
    string airline = Qry.FieldAsString( "airline" );
    int flt_no = Qry.FieldAsInteger( "flt_no" );
    string suffix = Qry.FieldAsString( "suffix" );
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
    reqInfo->LocaleToLog("EVT.DISP.DELETE_TAKEOFF_FACT", LEvntPrms() << PrmFlight("flt", airline, flt_no, suffix),
                       evtDisp, move_id, point_id);
	ChangeACT_OUT( point_id, act_out, NoExists );
	SetTripStages_IgnoreAuto( point_id, pr_del != 0 );
  try {
    check_trip_tasks( move_id );
  }
  catch(std::exception &E) {
    ProgError(STDLOG,"DropFlightFact.check_trip_tasks (move_id=%d): %s",move_id,E.what());
  };
  on_change_trip( CALL_POINT, point_id );
	SALONS2::check_waitlist_alarm_on_tranzit_routes( point_id );
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
    str_to = AstraLocale::getLocaleText("Изм. класса") + " " + str_to;
  if ( ch_dest )
    str_to = AstraLocale::getLocaleText("Изм. пункта") + " " + str_to;
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
        LEvntPrms params;
        if ( airp_arv_spp != r->airp_arv_spp ) {
            PrmEnum airp("airp", "");
            airp.prms << PrmElem<string>("", etAirp, airp_arv_spp) << PrmSmpl<string>("", "(")
                         << PrmElem<string>("", etAirp, r->airp_arv_spp) << PrmSmpl<string>("", ")");
            params << airp;
        }
        else
          params << PrmElem<string>("airp", etAirp, airp_arv_spp);
        if ( class_spp != r->class_spp ){
            PrmEnum cls("cls", "");
            cls.prms << PrmElem<string>("", etClass, class_spp) << PrmSmpl<string>("", "(")
                         << PrmElem<string>("", etClass, r->class_spp) << PrmSmpl<string>("", ")");
            params << cls;
        }
        else
          params << PrmElem<string>("cls", etClass, class_spp);
        PrmEnum flt("flt", "");
        flt.prms << PrmFlight("", airline, flt_no, suffix) << PrmSmpl<string>("", "/")
                        << PrmDate("time", scd, "dd.mm");
        if ( airline + IntToString( flt_no ) + suffix + "/" + DateTimeToStr( scd, "dd.mm" ) !=
               r->airline + IntToString( r->flt_no ) + r->suffix + "/" + DateTimeToStr( r->scd, "dd.mm" ) )
          flt.prms << PrmSmpl<string>("", "(") << PrmFlight("", r->airline, r->flt_no, r->suffix)
                       << PrmSmpl<string>("", "/") << PrmDate("time", r->scd, "dd.mm") << PrmSmpl<string>("", ")");
        params << flt;
        params << PrmElem<string>("dep", etAirp, to_airp_dep);
        if ( airp_arv_tlg != r->airp_arv_tlg ) {
            PrmEnum arv("arv", "");
            arv.prms << PrmElem<string>("", etAirp, airp_arv_tlg) << PrmSmpl<string>("", "(")
                         << PrmElem<string>("", etAirp, r->airp_arv_tlg) << PrmSmpl<string>("", ")");
            params << arv;
        }
        else
          params << PrmElem<string>("arv", etAirp, airp_arv_tlg);
        if ( class_tlg != r->class_tlg ){
            PrmEnum clstlg("clstlg", "");
            clstlg.prms << PrmElem<string>("", etClass, class_tlg) << PrmSmpl<string>("", "(")
                         << PrmElem<string>("", etClass, r->class_tlg) << PrmSmpl<string>("", ")");
            params << clstlg;
        }
        else
          params << PrmElem<string>("clstlg", etClass, class_tlg);
        PrmEnum sts("sts", "");
        sts.prms << PrmSmpl<string>("", "'") << PrmElem<string>("", etGrpStatusType, status)
                    << PrmSmpl<string>("", "'");
        if ( status != r->status ){

            sts.prms << PrmSmpl<string>("", "('") << PrmElem<string>("", etGrpStatusType, r->status)
                     << PrmSmpl<string>("", "')");
        }
        params << sts;
        TReqInfo::Instance()->LocaleToLog("EVT.RESEAT_MODIFY", params, evtFlt, point_id);
  	  }
  		crs_displaces.erase( r );
    }
    else {
      LEvntPrms params;
      params << PrmElem<string>("airp", etAirp, airp_arv_spp) << PrmElem<string>("cls", etClass, class_spp)
             << PrmFlight("flt", airline, flt_no, suffix) << PrmDate("time", scd, "dd.mm")
             << PrmElem<string>("dep", etAirp, to_airp_dep) << PrmElem<string>("arv", etAirp, airp_arv_tlg)
             << PrmElem<string>("clstlg", etClass, class_tlg) << PrmElem<string>("sts", etGrpStatusType, status);
      TReqInfo::Instance()->LocaleToLog("EVT.RESEAT_ADD", params, evtFlt, point_id);
    node = node->next;
    }
  }
  for ( vector<tcrs_displ>::iterator r=crs_displaces.begin(); r!=crs_displaces.end(); r++ ) {
    LEvntPrms params;
    params << PrmElem<string>("airp", etAirp, r->airp_arv_spp) << PrmElem<string>("cls", etClass, r->class_spp)
           << PrmFlight("flt", r->airline, r->flt_no, r->suffix) << PrmDate("time", r->scd, "dd.mm")
           << PrmElem<string>("dep", etAirp, r->to_airp_dep) << PrmElem<string>("arv", etAirp, r->airp_arv_tlg)
           << PrmElem<string>("clstlg", etClass, r->class_tlg) << PrmElem<string>("sts", etGrpStatusType, r->status);
    TReqInfo::Instance()->LocaleToLog("EVT.RESEAT_DELETE", params, evtFlt, r->point_id_spp);
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
  // создаем все возможные рейсы из нового маршрута исключая удаленные пункты
  int lock_point_id = NoExists;
  for( TSOPPDests::iterator i=dests_del.begin(); i!=dests_del.end(); i++ ) {
  	if ( i->pr_del == -1 ) continue;
  	if ( lock_point_id == NoExists && i->point_id != NoExists ) {
      lock_point_id = i->point_id;
  	}
  	TSOPPTrip t = createTrip( move_id, i, dests_del );
  	ProgTrace( TRACE5, "t.pr_del=%d, t.point_id=%d, t.places_out.size()=%zu, t.suffix_out=%s",
  	                   t.pr_del, t.point_id, t.places_out.size(), t.suffix_out.c_str() );
  	trs.push_back(t);
  };
  TFlights flights;
  flights.Get( lock_point_id, ftAll );
  flights.Lock();
	TQuery Qry(&OraSession);
  // проверка на предмет того, что во всех пп стоит статус неактивен иначе ругаемся
	Qry.Clear();
	Qry.SQLText = "SELECT COUNT(*) c, point_dep FROM pax_grp WHERE point_dep IN "
	              "( SELECT point_id FROM points WHERE move_id=:move_id ) AND pax_grp.status NOT IN ('E') "
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
    bool empty = true;
    PrmEnum name("name", "/");
    PrmEnum dests("dests", "-");
    PrmEnum prior_name("", "");
    string prior_airline, prior_flt_no, prior_date, str_d;
	while ( !Qry.Eof ) {
      if ( Qry.FieldAsInteger( "pr_reg" ) && !Qry.FieldIsNULL( "scd_out" ) ) {
         TTripStages ts( Qry.FieldAsInteger( "point_id" ) );
         if ( ts.getStage( stCheckIn ) != sNoActive )
           throw AstraLocale::UserException( "MSG.FLIGHT.UNABLE_DEL.STATUS_ACTIVE",
                                             LParams() << LParam("airp", ElemIdToNameLong(etAirp,Qry.FieldAsString("airp"))));
      }
      if (!empty) {
      name.prms << prior_name;
      prior_name.prms.clearPrms();
      empty = true;
      }
      if ( prior_airline.empty() || prior_airline != Qry.FieldAsString( "airline" ) ) {
        prior_airline = Qry.FieldAsString( "airline" );
        prior_name.prms << PrmElem<string>("", etAirline, prior_airline);
        prior_flt_no = Qry.FieldAsString( "flt_no" );
        prior_name.prms << PrmSmpl<string>("", prior_flt_no);
        empty = false;
      }
      if ( prior_flt_no.empty() || prior_flt_no != Qry.FieldAsString( "flt_no" ) ) {
        prior_flt_no = Qry.FieldAsString( "flt_no" );
        prior_name.prms << PrmSmpl<string>("", prior_flt_no);
        empty = false;
      }
      str_d.clear();
      if ( !Qry.FieldIsNULL( "scd_out" ) ) {
      str_d = DateTimeToStr( Qry.FieldAsDateTime( "scd_out" ), "dd.mm" );
      }
      if ( prior_date.empty() || prior_date != str_d ) {
        prior_date = str_d;
        prior_name.prms << PrmSmpl<string>("", " ") << PrmDate("", Qry.FieldAsDateTime( "scd_out" ), "dd.mm");
        empty = false;
      }
      dests.prms << PrmElem<string>("", etAirp, Qry.FieldAsString("airp"));
      Qry.Next();
	}
	
  for(vector<TSOPPTrip>::iterator j=trs.begin(); j!=trs.end(); j++ ) {
  	if ( j->places_out.size() ) { // рейс не был отменен
  		TSOPPTrip tr;
  		BitSet<TSOPPTripChange> FltChange;
  		FltChange.setFlag( tsDelete );
  	  ChangeTrip( j->point_id, tr, *j, FltChange );  // рейс на вылет удален
  	}
  }
  
	Qry.Clear();
	Qry.SQLText = "UPDATE points SET pr_del=-1 WHERE move_id=:move_id";
	Qry.CreateVariable( "move_id", otInteger, move_id );
	Qry.Execute();
  try {
    check_trip_tasks( dests_del );
  }
  catch(std::exception &E) {
    ProgError(STDLOG,"DeleteISGTrips.check_trip_tasks (move_id=%d): %s",move_id,E.what());
  };
  for (TSOPPDests::iterator i=dests_del.begin(); i!=dests_del.end(); i++ ) {
    on_change_trip( CALL_POINT, i->point_id );
  }
  TReqInfo::Instance()->LocaleToLog("EVT.FLIGHT.DELETE", LEvntPrms() << name << dests, evtDisp, move_id);
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
  long int exec_time;
	internal_ReadData( trips, NoExists, NoExists, false, tSPPCEK, exec_time, point_id );
}

void ChangeACT_OUT( int point_id, TDateTime old_act, TDateTime act )
{
  if ( act != NoExists )
  {
    //изменение фактического времени вылета
    try
    {
      vector<TypeB::TCreateInfo> createInfo;
      TypeB::TMVTACreator(point_id).getInfo(createInfo);
      TelegramInterface::SendTlg(createInfo);
    }
    catch(std::exception &E)
    {
      ProgError(STDLOG,"ChangeACT_OUT.SendTlg (point_id=%d): %s",point_id,E.what());
    };
    if ( old_act == NoExists ) { //проставление факта вылета
      try {
         ProgTrace( TRACE5, "exec_stage sTakeoff, point_id=%d", point_id );
         exec_stage( point_id, sTakeoff );
      }
      catch( std::exception &E ) {
        ProgError( STDLOG, "std::exception: %s", E.what() );
      }
      catch( ... ) {
        ProgError( STDLOG, "Unknown error" );
      };
    }
  };
  if ( old_act != NoExists && act == NoExists )
  {
    //отмена вылета
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
    //изменение фактического времени прилета
    try
    {
      //телеграммы на прилет
      TTripRoute route;
      TTripRouteItem prior_airp;
      route.GetPriorAirp(NoExists, point_id, trtNotCancelled, prior_airp);
      if (prior_airp.point_id!=NoExists)
  	  {
        vector<TypeB::TCreateInfo> createInfo;
        TypeB::TMVTBCreator(prior_airp.point_id).getInfo(createInfo);
        TelegramInterface::SendTlg(createInfo);
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

  	    FltChange.isFlag( tsRestoreFltOut )*/ ) { // восстановление (новый)
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

void validateField( const string &surname, const string &fieldname )
{
  for ( string::const_iterator istr=surname.begin(); istr!=surname.end(); istr++ ) {
     if ( !IsDigitIsLetter( *istr ) && *istr != ' ' )
       throw AstraLocale::UserException( "MSG.FIELD_INCLUDE_INVALID_CHARACTER1",
                                         LParams() << LParam( "field_name", AstraLocale::getLocaleText( fieldname ) )
                                                   << LParam( "symbol", string(1,*istr)) );
  }
}

void SoppInterface::WriteCrew(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    LEvntPrms params;
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
  int point_id=NodeAsInteger( "point_id", dataNode );
	Qry.CreateVariable( "point_id", otInteger, point_id );
  string commander = NodeAsString( "commander", dataNode );
  validateField( commander, "КВС" );
 	Qry.CreateVariable( "commander", otString, commander );
	if (GetNode( "cockpit", dataNode )!=NULL && !NodeIsNULL( "cockpit", dataNode )) {
	  Qry.CreateVariable( "cockpit", otInteger, NodeAsInteger( "cockpit", dataNode ) );
      params << PrmSmpl<int>("cockpit", NodeAsInteger("cockpit", dataNode));
    }
	else {
	  Qry.CreateVariable( "cockpit", otInteger, FNull );
      params << PrmLexema("cockpit", "EVT.UNKNOWN");
    }
    if (GetNode( "cabin", dataNode )!=NULL && !NodeIsNULL( "cabin", dataNode )) {
	  Qry.CreateVariable( "cabin", otInteger, NodeAsInteger( "cabin", dataNode ) );
      params << PrmSmpl<int>("cabin", NodeAsInteger("cabin", dataNode));
    }
	else {
	  Qry.CreateVariable( "cabin", otInteger, FNull );
      params << PrmLexema("cabin", "EVT.UNKNOWN");
  }
  params << PrmSmpl<std::string>("commander", NodeAsString("commander", dataNode));
	Qry.Execute();
    TReqInfo::Instance()->LocaleToLog("EVT.SET_CREW", params, evtFlt, point_id );
  check_crew_alarms( point_id );
}

void SoppInterface::ReadDoc(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
	int point_id = NodeAsInteger( "point_id", reqNode );
	TQuery Qry(&OraSession);
	Qry.SQLText =
	  "SELECT loader, pts_agent "
	  "FROM trip_rpt_person WHERE point_id=:point_id";
	Qry.CreateVariable( "point_id", otInteger, point_id );
	Qry.Execute();
	xmlNodePtr dataNode = NewTextChild( NewTextChild( resNode, "data" ), "doc" );
  NewTextChild( dataNode, "loader" );
  NewTextChild( dataNode, "pts_agent" );
	if ( !Qry.Eof )
	{
		ReplaceTextChild( dataNode, "loader", Qry.FieldAsString( "loader" ) );
		ReplaceTextChild( dataNode, "pts_agent", Qry.FieldAsString( "pts_agent" ) );
  };
}

void SoppInterface::WriteDoc(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    LEvntPrms params;
    TQuery Qry(&OraSession);
	Qry.SQLText =
	  "BEGIN "
	  "  UPDATE trip_rpt_person "
	  "  SET loader=SUBSTR(:loader,1,100), "
	  "      pts_agent=SUBSTR(:pts_agent,1,100) "
	  "  WHERE point_id=:point_id; "
	  "  IF SQL%NOTFOUND THEN "
	  "    INSERT INTO trip_rpt_person(point_id, loader, pts_agent) "
	  "    VALUES(:point_id, SUBSTR(:loader,1,100), SUBSTR(:pts_agent,1,100)); "
	  "  END IF;"
	  "END;";
	xmlNodePtr dataNode = NodeAsNode( "data/doc", reqNode );
	Qry.CreateVariable( "point_id", otInteger, NodeAsInteger( "point_id", dataNode ) );
	validateField( NodeAsString( "loader", dataNode ), "Грузчик" );
	Qry.CreateVariable( "loader", otString, NodeAsString( "loader", dataNode ) );
	validateField( NodeAsString( "pts_agent ", dataNode ), "Агент СОПП" );
	Qry.CreateVariable( "pts_agent", otString, NodeAsString( "pts_agent", dataNode ) );
    params << PrmSmpl<std::string>("pts_agent", NodeAsString("pts_agent", dataNode))
              << PrmSmpl<std::string>("loader", NodeAsString("loader", dataNode));
	Qry.Execute();
    TReqInfo::Instance()->LocaleToLog("EVT.PTS_AGENT_LOADER", params, evtFlt,  NodeAsInteger( "point_id", dataNode ) );
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
      if ( string( "Р" ) == Qry.FieldAsString( col_work_mode ) ) {
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
    new_trfer_exists = TrferList::trferCkinExists( point_id, Qry );
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
  convertStrToStations( ckin_desks, "Р", stations );
  convertStrToStations( gates, "П", stations );
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

void SoppInterface::CreateAPIS(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  int point_id=NodeAsInteger( "point_id", reqNode );
  if (!CheckStageACT(point_id, sCloseCheckIn))
    throw UserException("MSG.APIS_CREATION_ONLY_AFTER_CHECKIN_CLOSING");
  if (create_apis_file(point_id, ""))
    AstraLocale::showMessage("MSG.APIS_CREATED");
  else
    AstraLocale::showErrorMessage("MSG.APIS_NOT_CREATED_FOR_FLIGHT");
}

void set_pr_tranzit(int point_id, int point_num, int first_point, bool new_pr_tranzit)
{
  TQuery Qry(&OraSession);
  Qry.Clear();
  if (new_pr_tranzit)
    Qry.SQLText =
      "BEGIN "
      "  UPDATE points SET pr_tranzit=:pr_tranzit WHERE point_id=:point_id AND pr_del>=0; "
      "  UPDATE points SET first_point=:first_point "
      "  WHERE first_point=:point_id AND point_num>:point_num AND pr_del>=0; "
      "END; ";
  else
    Qry.SQLText =
      "BEGIN "
      "  UPDATE points SET pr_tranzit=:pr_tranzit WHERE point_id=:point_id AND pr_del>=0; "
      "  UPDATE points SET first_point=:point_id "
      "  WHERE first_point=:first_point AND point_num>:point_num AND pr_del>=0; "
      "END; ";
  Qry.CreateVariable("pr_tranzit",otInteger,(int)new_pr_tranzit);
  Qry.CreateVariable("point_id",otInteger,point_id);
  Qry.CreateVariable("first_point",otInteger,first_point);
  Qry.CreateVariable("point_num",otInteger,point_num);
  Qry.Execute();
}

void set_trip_sets(const TAdvTripInfo &flt)
{
  /*очистка настроек рейса*/
  TQuery Qry(&OraSession);
  TQuery InsQry(&OraSession);

  Qry.Clear();
  Qry.SQLText=
    "BEGIN "
    "  DELETE FROM trip_bp WHERE point_id=:point_id; "
    "  DELETE FROM trip_bt WHERE point_id=:point_id; "
    "  DELETE FROM trip_hall WHERE point_id=:point_id; "
    "END;";
  Qry.CreateVariable("point_id", otInteger, flt.point_id);
  Qry.Execute();

  Qry.Clear();
  Qry.CreateVariable("airline", otString, flt.airline);
  Qry.CreateVariable("flt_no", otInteger, flt.flt_no);
  Qry.CreateVariable("airp_dep", otString, flt.airp);

  //посадочные талоны
  InsQry.Clear();
  InsQry.SQLText=
    "INSERT INTO trip_bp(point_id, class, bp_type) "
    "VALUES(:point_id, :class, :bp_type) ";
  InsQry.CreateVariable("point_id", otInteger, flt.point_id);
  InsQry.DeclareVariable("class", otString);
  InsQry.DeclareVariable("bp_type", otString);

  Qry.SQLText=
    "SELECT class,bp_type, "
    "       DECODE(airline,NULL,0,8)+ "
    "       DECODE(flt_no,NULL,0,2)+ "
    "       DECODE(airp_dep,NULL,0,4) AS priority "
    "FROM bp_set "
    "WHERE (airline IS NULL OR airline=:airline) AND "
    "      (flt_no IS NULL OR flt_no=:flt_no) AND "
    "      (airp_dep IS NULL OR airp_dep=:airp_dep) "
    "ORDER BY class,priority DESC ";
  Qry.Execute();

  bool pr_first=true;
  string prev_cl;
  for(;!Qry.Eof;Qry.Next())
  {
    string cl=Qry.FieldAsString("class");
    string bp_type=Qry.FieldAsString("bp_type");
    if (pr_first || prev_cl!=cl)
    {
      InsQry.SetVariable("class", cl);
      InsQry.SetVariable("bp_type", bp_type);
      InsQry.Execute();

      ostringstream msg;
      string lexema_id;
      LEvntPrms params;
      params << PrmElem<string>("name", etBPType, bp_type, efmtNameLong);
      if (!cl.empty())
      {
        lexema_id = "EVT.BP_FORM_INSERTED_FOR_CLASS";
        params << PrmElem<string>("cls", etClass, cl, efmtCodeNative);
      }
      else
        lexema_id = "EVT.BP_FORM_INSERTED";
      msg << ".";
      TReqInfo::Instance()->LocaleToLog(lexema_id, params, evtFlt, flt.point_id);
    };
    pr_first=false;
    prev_cl=cl;
  };

  //багажные бирки
  InsQry.Clear();
  InsQry.SQLText=
    "INSERT INTO trip_bt(point_id, tag_type) "
    "VALUES(:point_id, :tag_type) ";
  InsQry.CreateVariable("point_id", otInteger, flt.point_id);
  InsQry.DeclareVariable("tag_type", otString);

  Qry.SQLText=
    "SELECT tag_type, "
    "       DECODE(airline,NULL,0,8)+ "
    "       DECODE(flt_no,NULL,0,2)+ "
    "       DECODE(airp_dep,NULL,0,4) AS priority "
    "FROM bt_set "
    "WHERE (airline IS NULL OR airline=:airline) AND "
    "      (flt_no IS NULL OR flt_no=:flt_no) AND "
    "      (airp_dep IS NULL OR airp_dep=:airp_dep) "
    "ORDER BY priority DESC ";
  Qry.Execute();

  if (!Qry.Eof)
  {
    string tag_type=Qry.FieldAsString("tag_type");
    InsQry.SetVariable("tag_type", tag_type);
    InsQry.Execute();

    TReqInfo::Instance()->LocaleToLog("EVT.BT_FORM_INSERTED", LEvntPrms()
                                      << PrmElem<string>("name", etBTType, tag_type, efmtNameLong),
                                      evtFlt, flt.point_id);
  };

  //залы
  InsQry.Clear();
  InsQry.SQLText=
    "INSERT INTO trip_hall(point_id, type, hall, pr_misc) "
    "VALUES(:point_id, :type, :hall, :pr_misc) ";
  InsQry.CreateVariable("point_id", otInteger, flt.point_id);
  InsQry.DeclareVariable("type", otInteger);
  InsQry.DeclareVariable("hall", otInteger);
  InsQry.DeclareVariable("pr_misc", otInteger);

  Qry.SQLText=
    "SELECT type,hall,pr_misc, "
    "       DECODE(airline,NULL,0,8)+ "
    "       DECODE(flt_no,NULL,0,2)+ "
    "       DECODE(airp_dep,NULL,0,4) AS priority "
    "FROM hall_set "
    "WHERE (airline IS NULL OR airline=:airline) AND "
    "      (flt_no IS NULL OR flt_no=:flt_no) AND "
    "      (airp_dep IS NULL OR airp_dep=:airp_dep) "
    "ORDER BY type,hall,priority DESC ";
  Qry.Execute();

  pr_first=true;
  int prev_type=NoExists;
  int prev_hall=NoExists;
  for(;!Qry.Eof;Qry.Next())
  {
    int type=Qry.FieldAsInteger("type");
    int hall=Qry.FieldIsNULL("hall")?NoExists:Qry.FieldAsInteger("hall");
    bool pr_misc=Qry.FieldAsInteger("pr_misc")!=0;
    if (pr_first || prev_type!=type || prev_hall!=hall)
    {
      InsQry.SetVariable("type", type);
      hall!=NoExists?InsQry.SetVariable("hall", hall):
                     InsQry.SetVariable("hall", FNull);
      InsQry.SetVariable("pr_misc", (int)pr_misc);
      InsQry.Execute();

      if (type==1 || type==2)
      {
        string lexema_id;
        LEvntPrms params;
        params << PrmLexema("action", "EVT.MODE_INSERTED");
        if (type==1)
          lexema_id = (pr_misc?"EVT.TRIP_BRD_AND_REG":"EVT.TRIP_SEPARATE_BRD_AND_REG");
        if (type==2)
          lexema_id = (pr_misc?"EVT.TRIP_EXAM_AND_BRD":"EVT.TRIP_SEPARATE_EXAM_AND_BRD");

        if (hall!=NoExists)
        {
          PrmLexema lexema("hall", "EVT.FOR_HALL");
          lexema.prms << PrmElem<int>("hall", etHall, hall, efmtNameLong);
          params << lexema;
        }
        else
          params << PrmSmpl<string>("hall", "");
        TReqInfo::Instance()->LocaleToLog(lexema_id, params, evtFlt, flt.point_id);
      };
    };
    pr_first=false;
    prev_type=type;
    prev_hall=hall;
  };

  //транзитные настройки
  if (flt.first_point!=NoExists)
  {
    InsQry.Clear();
    InsQry.SQLText="UPDATE trip_sets SET pr_tranz_reg=:pr_reg WHERE point_id=:point_id";
    InsQry.CreateVariable("point_id", otInteger, flt.point_id);
    InsQry.DeclareVariable("pr_reg", otInteger);

    Qry.SQLText=
      "SELECT pr_tranzit,pr_reg, "
      "       DECODE(airline,NULL,0,8)+ "
      "       DECODE(flt_no,NULL,0,2)+ "
      "       DECODE(airp_dep,NULL,0,4) AS priority "
      "FROM tranzit_set "
      "WHERE (airline IS NULL OR airline=:airline) AND "
      "      (flt_no IS NULL OR flt_no=:flt_no) AND "
      "      (airp_dep IS NULL OR airp_dep=:airp_dep) "
      "ORDER BY priority DESC ";
    Qry.Execute();
    if (!Qry.Eof)
    {
      bool pr_tranzit=Qry.FieldAsInteger("pr_tranzit")!=0;
      bool pr_reg=Qry.FieldAsInteger("pr_reg")!=0;
      InsQry.SetVariable("pr_reg", (int)pr_reg);
      InsQry.Execute();

      if (flt.pr_tranzit!=pr_tranzit)
        set_pr_tranzit(flt.point_id, flt.point_num, flt.first_point, pr_tranzit);  //!!!djek функция должна быть более серьезной - взять куски из prepreg.cc

      string lexema_id;
      if (pr_reg && pr_tranzit) lexema_id = "EVT.SET_MODE_WITH_REG_TRANS_FLIGHT";
      else if (!pr_reg && pr_tranzit) lexema_id = "EVT.SET_MODE_WITHOUT_REG_TRANS_FLIGHT";
      else if (pr_reg && !pr_tranzit) lexema_id = "EVT.SET_MODE_WITH_REG_NON_TRANS_FLIGHT";
      else lexema_id = "EVT.SET_MODE_WITHOUT_REG_NON_TRANS_FLIGHT";
      TReqInfo::Instance()->LocaleToLog(lexema_id, evtFlt, flt.point_id);
    };
  };

  //настройки разные
  Qry.SQLText=
    "SELECT type,pr_misc, "
    "       DECODE(airline,NULL,0,8)+ "
    "       DECODE(flt_no,NULL,0,2)+ "
    "       DECODE(airp_dep,NULL,0,4) AS priority "
    "FROM misc_set "
    "WHERE (airline IS NULL OR airline=:airline) AND "
    "      (flt_no IS NULL OR flt_no=:flt_no) AND "
    "      (airp_dep IS NULL OR airp_dep=:airp_dep) "
    "ORDER BY type,priority DESC ";
  Qry.Execute();

  pr_first=true;
  prev_type=NoExists;
  map<TTripSetType, bool> sets;
  for(;!Qry.Eof;Qry.Next())
  {
    int type=Qry.FieldAsInteger("type");
    bool pr_misc=Qry.FieldAsInteger("pr_misc")!=0;
    if (pr_first || prev_type!=type)
    {
      sets.insert(make_pair((TTripSetType)type, pr_misc));
    };
    pr_first=false;
    prev_type=type;
  };
  update_trip_sets(flt.point_id, sets, true);

  //платная регистрация
  InsQry.Clear();
  InsQry.SQLText=
    "INSERT INTO trip_paid_ckin(point_id, pr_permit, prot_timeout) "
    "VALUES(:point_id, :pr_permit, :prot_timeout) ";
  InsQry.CreateVariable("point_id", otInteger, flt.point_id);
  InsQry.DeclareVariable("pr_permit", otInteger);
  InsQry.DeclareVariable("prot_timeout", otInteger);

  Qry.SQLText=
    "SELECT pr_permit,prot_timeout, "
    "       DECODE(airline,NULL,0,8)+ "
    "       DECODE(flt_no,NULL,0,2)+ "
    "       DECODE(airp_dep,NULL,0,4) AS priority "
    "FROM paid_ckin_sets "
    "WHERE (airline IS NULL OR airline=:airline) AND "
    "      (flt_no IS NULL OR flt_no=:flt_no) AND "
    "      (airp_dep IS NULL OR airp_dep=:airp_dep) "
    "ORDER BY priority DESC ";
  Qry.Execute();

  bool pr_permit=false;
  int prot_timeout=NoExists;
  if (!Qry.Eof)
  {
    pr_permit=Qry.FieldAsInteger("pr_permit")!=0;
    prot_timeout=Qry.FieldIsNULL("prot_timeout")?NoExists:Qry.FieldAsInteger("prot_timeout");
  };

  InsQry.SetVariable("pr_permit", (int)pr_permit);
  prot_timeout!=NoExists?InsQry.SetVariable("prot_timeout", prot_timeout):
                         InsQry.SetVariable("prot_timeout", FNull);
  InsQry.Execute();

  if (pr_permit)
  {
    //пишем в лог только в случае платной регистрации
    LEvntPrms params;
    params << PrmLexema("action", (pr_permit?"EVT.CKIN_PERFORMED":"EVT.CKIN_NOT_PERFORMED"))
           << PrmLexema("what", "EVT.TRIP_PAID_CKIN");
    if (pr_permit)
    {
      PrmLexema lexema("params", "EVT.PROT_TIMEOUT");
      if (prot_timeout!=NoExists) {
        PrmLexema timeout("timeout", "EVT.TIMEOUT_VALUE");
        timeout.prms << PrmSmpl<int>("timeout", prot_timeout);
        lexema.prms << timeout;
      }
      else
        lexema.prms << PrmLexema("timeout", "EVT.UNKNOWN");
      params << lexema;
    }
    else
      params << PrmSmpl<string>("params" , "");
    TReqInfo::Instance()->LocaleToLog("EVT.TRIP_CKIN", params, evtFlt, flt.point_id);
  };
};

void update_trip_sets(int point_id, const map<TTripSetType, bool> &sets, bool first_init)
{
  TQuery Qry(&OraSession);

  list<string> fields;
  list<string> msgs;
  for(map<TTripSetType, bool>::const_iterator s=sets.begin(); s!=sets.end(); ++s)
  {
    switch (s->first)
    {
      case tsCheckLoad:
        fields.push_back("pr_check_load=:pr_check_load");
        msgs.push_back(s->second?"EVT.SET_MODE_CHECK_LOAD":
                                 "EVT.SET_MODE_WITHOUT_CHECK_LOAD");
        Qry.CreateVariable("pr_check_load", otInteger, (int)s->second);
        break;
      case tsOverloadReg:
        fields.push_back("pr_overload_reg=:pr_overload_reg");
        msgs.push_back(s->second?"EVT.SET_MODE_OVERLOAD_REG_PERMISSION":
                                 "EVT.SET_MODE_OVERLOAD_REG_PROHIBITION");
        Qry.CreateVariable("pr_overload_reg", otInteger, (int)s->second);
        break;
      case tsExam:
        fields.push_back("pr_exam=:pr_exam");
        msgs.push_back(s->second?"EVT.SET_MODE_EXAM":
                                 "EVT.SET_MODE_WITHOUT_EXAM");
        Qry.CreateVariable("pr_exam", otInteger, (int)s->second);
        break;
      case tsCheckPay:
        fields.push_back("pr_check_pay=:pr_check_pay");
        msgs.push_back(s->second?"EVT.SET_MODE_CHECK_PAY":
                                 "EVT.SET_MODE_WITHOUT_CHECK_PAY");
        Qry.CreateVariable("pr_check_pay", otInteger, (int)s->second);
        break;
      case tsExamCheckPay:
        fields.push_back("pr_exam_check_pay=:pr_exam_check_pay");
        msgs.push_back(s->second?"EVT.SET_MODE_EXAM_CHACK_PAY":
                                 "EVT.SET_MODE_WITHOUT_EXAM_CHACK_PAY");
        Qry.CreateVariable("pr_exam_check_pay", otInteger, (int)s->second);
        break;
      case tsRegWithTkn:
        fields.push_back("pr_reg_with_tkn=:pr_reg_with_tkn");
        msgs.push_back(s->second?"EVT.SET_MODE_REG_WITHOUT_TKN_PROHIBITION":
                                 "EVT.SET_MODE_REG_WITHOUT_TKN_PERMISSION");
        Qry.CreateVariable("pr_reg_with_tkn", otInteger, (int)s->second);
        break;
      case tsRegWithDoc:
        fields.push_back("pr_reg_with_doc=:pr_reg_with_doc");
        msgs.push_back(s->second?"EVT.SET_MODE_REG_WITHOUT_DOC_PROHIBITION":
                                 "EVT.SET_MODE_REG_WITHOUT_DOC_PERMISSION");
        Qry.CreateVariable("pr_reg_with_doc", otInteger, (int)s->second);
        break;
      case tsAutoWeighing:
        fields.push_back("auto_weighing=:auto_weighing");
        msgs.push_back(s->second?"EVT.SET_AUTO_WEIGHING":
                                 "EVT.CANCEL_AUTO_WEIGHING");
        Qry.CreateVariable("auto_weighing", otInteger, (int)s->second);
        break;
      case tsFreeSeating:
        fields.push_back("pr_free_seating=:pr_free_seating");
        if (!first_init || s->second)
          msgs.push_back(s->second?"EVT.SET_FREE_SEATING":
                                   "EVT.CANCEL_FREE_SEATING");
        Qry.CreateVariable("pr_free_seating", otInteger, (int)s->second);
        break;
      case tsAPISControl:
        fields.push_back("apis_control=:apis_control");
        if (!first_init || !s->second)
          msgs.push_back(s->second?"EVT.SET_APIS_DATA_CONTROL":
                                   "EVT.CANCELED_APIS_DATA_CONTROL");
        Qry.CreateVariable("apis_control", otInteger, (int)s->second);
        break;
      case tsAPISManualInput:
        fields.push_back("apis_manual_input=:apis_manual_input");
        if (!first_init || s->second)
          msgs.push_back(s->second?"EVT.ALLOWED_APIS_DATA_MANUAL_INPUT":
                                   "EVT.NOT_ALLOWED_APIS_DATA_MANUAL_INPUT");
        Qry.CreateVariable("apis_manual_input", otInteger, (int)s->second);
        break;
      default:
        break;
    };
  };

  if (fields.empty()) return;

  ostringstream sql;
  sql << "UPDATE trip_sets SET ";
  for(list<string>::const_iterator i=fields.begin(); i!=fields.end(); ++i)
  {
    if (i!=fields.begin()) sql << ", ";
    sql << *i;
  };
  sql << " WHERE point_id=:point_id";

  Qry.SQLText=sql.str().c_str();
  Qry.CreateVariable("point_id", otInteger, point_id);
  Qry.Execute();
  if (Qry.RowsProcessed()>0)
  {
    //запись в журнал операций
    TLogLocale locale;
    locale.ev_type=evtFlt;
    locale.id1=point_id;
    for(list<string>::const_iterator i=msgs.begin(); i!=msgs.end(); ++i)
    {
      locale.lexema_id=*i;
      TReqInfo::Instance()->LocaleToLog(locale);
    };
  };
};

void puttrip_stages(int point_id)
{
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText=
    "SELECT airline, flt_no, suffix, airp, scd_out, points.pr_del, "
    "       act_out, craft, trip_type "
    "FROM points, trip_types "
    "WHERE points.point_id = :point_id AND points.pr_del>=0 AND "
    "      points.trip_type = trip_types.code AND "
    "      trip_types.pr_reg = 1";
  Qry.CreateVariable("point_id", otInteger, point_id);
  Qry.Execute();
  if (Qry.Eof) return;

  TTripInfo flt(Qry);
  TDateTime act_out=Qry.FieldIsNULL("act_out")?NoExists:Qry.FieldAsDateTime("act_out");
  string craft=Qry.FieldAsString("craft");
  string trip_type=Qry.FieldAsString("trip_type");

  TQuery TimesQry(&OraSession);
  TimesQry.Clear();
  TimesQry.SQLText=
    "SELECT TRUNC(:scd_out-time/1440,'MI') AS time, "
    "       DECODE( graph_times.airline, NULL, 0, 8 ) + "
    "       DECODE( graph_times.airp, NULL, 0, 4 ) + "
    "       DECODE( graph_times.craft, NULL, 0, 2 ) + "
    "       DECODE( graph_times.trip_type, NULL, 0, 1 ) AS priority "
    "FROM graph_times "
    "WHERE stage_id = :stage_id AND "
    "      ( graph_times.airline IS NULL OR graph_times.airline = :airline ) AND "
    "      ( graph_times.airp IS NULL OR graph_times.airp = :airp ) AND "
    "      ( graph_times.craft IS NULL OR graph_times.craft = :craft ) AND "
    "      ( graph_times.trip_type IS NULL OR graph_times.trip_type = :trip_type ) "
    "UNION "
    "SELECT TRUNC(:scd_out-time/1440,'MI') AS time, -1 AS priority "
    "FROM graph_stages "
    "WHERE stage_id = :stage_id "
    "ORDER BY 2/*priority*/ DESC ";
  TimesQry.DeclareVariable("stage_id", otInteger);
  TimesQry.CreateVariable("airline", otString, flt.airline);
  TimesQry.CreateVariable("airp", otString, flt.airp);
  TimesQry.CreateVariable("craft", otString, craft);
  TimesQry.CreateVariable("trip_type", otString, trip_type);
  flt.scd_out!=NoExists?TimesQry.CreateVariable("scd_out", otDate, flt.scd_out):
                        TimesQry.CreateVariable("scd_out", otDate, FNull);

  bool ignore_auto=!(act_out==NoExists && flt.pr_del==0);
  TQuery InsQry(&OraSession);
  InsQry.Clear();
  InsQry.SQLText=
    "INSERT INTO trip_stages(point_id, stage_id, scd, est, act, pr_auto, pr_manual, ignore_auto) "
    "VALUES(:point_id, :stage_id, :time, NULL, NULL, :pr_auto, 0, :ignore_auto ) ";
  InsQry.CreateVariable("point_id", otInteger, point_id);
  InsQry.DeclareVariable("stage_id", otInteger);
  InsQry.DeclareVariable("time", otDate);
  InsQry.DeclareVariable("pr_auto", otInteger);
  InsQry.CreateVariable("ignore_auto", otInteger, (int)ignore_auto);

  Qry.Clear();
  Qry.SQLText=
    "SELECT graph_stages.stage_id, "
    "       NVL(stage_names.name,graph_stages.name) name, "
    "       NVL(stage_sets.pr_auto,graph_stages.pr_auto) pr_auto "
    "FROM graph_stages, stage_sets, stage_names "
    "WHERE graph_stages.stage_id > 0 AND graph_stages.stage_id < 99 AND "
    "      stage_sets.airp(+) = :airp AND "
    "      stage_names.airp(+) = :airp AND "
    "      stage_sets.stage_id(+) = graph_stages.stage_id AND "
    "      stage_names.stage_id(+) = graph_stages.stage_id AND "
    "      NOT EXISTS(SELECT stage_id FROM trip_stages WHERE point_id = :point_id AND stage_id = graph_stages.stage_id) "
    "ORDER BY stage_id ";
  Qry.CreateVariable("airp", otString, flt.airp);
  Qry.CreateVariable("point_id", otInteger, point_id);
  Qry.Execute();
  for(;!Qry.Eof;Qry.Next())
  {
    int stage_id=Qry.FieldAsInteger("stage_id");

    TimesQry.SetVariable("stage_id", stage_id);
    TimesQry.Execute();
    if (TimesQry.Eof) continue;

    TDateTime time=TimesQry.FieldIsNULL("time")?NoExists:TimesQry.FieldAsDateTime("time");

    InsQry.SetVariable("stage_id", stage_id);
    time!=NoExists?InsQry.SetVariable("time", time):
                   InsQry.SetVariable("time", FNull);                                 ;
    InsQry.SetVariable("pr_auto", (int)(Qry.FieldAsInteger("pr_auto")!=0));
    InsQry.Execute();

    LEvntPrms params;
    params << PrmStage("name", TStage(stage_id), flt.airp);
    if (time!=NoExists)
      params << PrmDate("time", time, "hh:nn dd.mm.yy (UTC)");
    else
      params << PrmLexema("time", "EVT.UNKNOWN");
    TReqInfo::Instance()->LocaleToLog("EVT.STAGE.PLAN_TIME", params, evtGraph, point_id);
  };

  Qry.Clear();
  Qry.SQLText =
    "BEGIN "
    "  gtimer.sync_trip_final_stages(:point_id); "
    "END;";
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.Execute();
}

void set_flight_sets(int point_id, int f, int c, int y)
{
  //лочим рейс - весь маршрут, т.к. pr_tranzit может поменяться
  TFlights flights;
	 flights.Get( point_id, ftAll );
	 flights.Lock();

  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText=
    "SELECT airline, flt_no, suffix, airp, scd_out, "
    "       point_id, point_num, first_point, pr_tranzit "
    "FROM points "
    "WHERE point_id=:point_id AND pr_del>=0";
  Qry.CreateVariable("point_id", otInteger, point_id);
  Qry.Execute();
  if (Qry.Eof) return;
  TAdvTripInfo flt(Qry);

  Qry.Clear();
  Qry.SQLText=
    "INSERT INTO trip_sets "
    " (point_id,f,c,y,max_commerce,pr_etstatus,pr_stat, "
    "  pr_tranz_reg,pr_check_load,pr_overload_reg,pr_exam,pr_check_pay, "
    "  pr_exam_check_pay,pr_reg_with_tkn,pr_reg_with_doc,crc_comp, "
		"  pr_basel_stat,auto_weighing,pr_free_seating,apis_control,apis_manual_input) "
    "VALUES(:point_id,:f,:c,:y, NULL, 0, 0, NULL, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0) ";
  Qry.CreateVariable("point_id", otInteger, point_id);
  f!=NoExists?Qry.CreateVariable("f", otInteger, f):
              Qry.CreateVariable("f", otInteger, FNull);
  c!=NoExists?Qry.CreateVariable("c", otInteger, c):
              Qry.CreateVariable("c", otInteger, FNull);
  y!=NoExists?Qry.CreateVariable("y", otInteger, y):
              Qry.CreateVariable("y", otInteger, FNull);
  Qry.Execute();

  TQuery InsQry(&OraSession);
  InsQry.Clear();
  InsQry.SQLText=
    "INSERT INTO trip_ckin_client(point_id, client_type, pr_permit, pr_waitlist, pr_tckin, pr_upd_stage, desk_grp_id) "
    "VALUES(:point_id, :client_type, :pr_permit, :pr_waitlist, :pr_tckin, :pr_upd_stage, :desk_grp_id)";
  InsQry.CreateVariable("point_id", otInteger, point_id);
  InsQry.DeclareVariable("client_type", otString);
  InsQry.DeclareVariable("pr_permit", otInteger);
  InsQry.DeclareVariable("pr_waitlist", otInteger);
  InsQry.DeclareVariable("pr_tckin", otInteger);
  InsQry.DeclareVariable("pr_upd_stage", otInteger);
  InsQry.DeclareVariable("desk_grp_id", otInteger);

  Qry.Clear();
  Qry.SQLText=
    "SELECT client_types.code AS client_type, "
    "       ckin_client_sets.desk_grp_id, "
    "       NVL(ckin_client_sets.pr_permit,0) AS pr_permit, "
    "       NVL(ckin_client_sets.pr_waitlist,0) AS pr_waitlist, "
    "       NVL(ckin_client_sets.pr_tckin,0) AS pr_tckin, "
    "       NVL(ckin_client_sets.pr_upd_stage,0) AS pr_upd_stage, "
    "       NVL(ckin_client_sets.priority,0) AS priority "
    "FROM client_types, "
    " (SELECT client_type, "
    "         desk_grp_id, "
    "         pr_permit, "
    "         pr_waitlist, "
    "         pr_tckin, "
    "         pr_upd_stage, "
    "         DECODE(airline,NULL,0,8)+ "
    "         DECODE(flt_no,NULL,0,2)+ "
    "         DECODE(airp_dep,NULL,0,4) AS priority "
    "  FROM ckin_client_sets "
    "  WHERE (airline IS NULL OR airline=:airline) AND "
    "        (flt_no IS NULL OR flt_no=:flt_no) AND "
    "        (airp_dep IS NULL OR airp_dep=:airp_dep)) ckin_client_sets "
    "WHERE client_types.code=ckin_client_sets.client_type(+) AND code IN (:ctWeb, :ctKiosk) " //!!!ctMobile
    "ORDER BY client_types.code,ckin_client_sets.desk_grp_id,priority DESC ";
  Qry.CreateVariable("airline", otString, flt.airline);
  Qry.CreateVariable("flt_no", otInteger, flt.flt_no);
  Qry.CreateVariable("airp_dep", otString, flt.airp);
  Qry.CreateVariable("ctWeb", otString, EncodeClientType(ctWeb));
  //Qry.CreateVariable("ctMobile", otString, EncodeClientType(ctMobile)); //!!!ctMobile
  Qry.CreateVariable("ctKiosk", otString, EncodeClientType(ctKiosk));
  Qry.Execute();
  TClientType prev_client_type=ctTypeNum;
  int prev_desk_grp_id=NoExists;
  for(;!Qry.Eof;Qry.Next())
  {
    TClientType client_type=DecodeClientType(Qry.FieldAsString("client_type"));
    int desk_grp_id=Qry.FieldIsNULL("desk_grp_id")?NoExists:Qry.FieldAsInteger("desk_grp_id");
    bool pr_permit=Qry.FieldAsInteger("pr_permit")!=0;
    bool pr_waitlist=Qry.FieldAsInteger("pr_waitlist")!=0;
    bool pr_tckin=Qry.FieldAsInteger("pr_tckin")!=0;
    bool pr_upd_stage=Qry.FieldAsInteger("pr_upd_stage")!=0;

    if (!((prev_client_type==client_type && prev_desk_grp_id==desk_grp_id) ||
          (client_type==ctKiosk && desk_grp_id==NoExists) ||
          (client_type==ctWeb && desk_grp_id!=NoExists) ||
          (client_type==ctMobile && desk_grp_id!=NoExists)))
    {
      InsQry.SetVariable("client_type", EncodeClientType(client_type));
      InsQry.SetVariable("pr_permit", (int)pr_permit);
      InsQry.SetVariable("pr_waitlist", (int)pr_waitlist);
      InsQry.SetVariable("pr_tckin", (int)pr_tckin);
      InsQry.SetVariable("pr_upd_stage", (int)pr_upd_stage);
      desk_grp_id!=NoExists?InsQry.SetVariable("desk_grp_id", desk_grp_id):
                            InsQry.SetVariable("desk_grp_id", FNull);
      InsQry.Execute();

      LEvntPrms params;
      params << PrmLexema("action", (pr_permit?"EVT.CKIN_ALLOWED":"EVT.CKIN_NOT_ALLOWED"));
      PrmLexema what("what", ((client_type==ctWeb || client_type==ctMobile)?"EVT.TRIP_WEB_CKIN":"EVT.TRIP_KIOSK_CKIN"));     //!!!ctMobile
      if (desk_grp_id!=NoExists) {
        PrmLexema lexema("desk_grp", ((client_type==ctWeb || client_type==ctMobile)?"EVT.FOR_PULT_GRP":"EVT.FOR_DESK_GRP")); //!!!ctMobile
        lexema.prms << PrmElem<int>("desk_grp", etDeskGrp, desk_grp_id, efmtNameLong);
        what.prms << lexema;
      }
      else what.prms << PrmSmpl<string>("desk_grp", "");
      params << what;
      if (pr_permit)
      {
        PrmLexema prms("params", "EVT.PARAMS");
//      "лист ожидания: " << (pr_waitlist?"да":"нет") //пока признак нигде не используется - на будущее
        prms.prms << PrmBool("tckin", pr_tckin) << PrmBool("upd_stage", pr_upd_stage);
        params << prms;
      }
      else params << PrmSmpl<string>("params", "");
      TReqInfo::Instance()->LocaleToLog("EVT.TRIP_CKIN", params, evtFlt, point_id);
    };

    prev_client_type=client_type;
    prev_desk_grp_id=desk_grp_id;
  };
  set_trip_sets(flt);
  puttrip_stages(point_id);
}

