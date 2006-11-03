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
#include "tripinfo.h"
#include "boost/date_time/local_time/local_time.hpp"

using namespace std;
using namespace BASIC;
using namespace EXCEPTIONS;
using namespace ASTRA;
using namespace boost::local_time;

const char* pointsSQL =
    "SELECT move_id,point_id,point_num,airp,city,first_point,airline,flt_no,suffix,craft,bort,"\
    "       scd_in,est_in,act_in,scd_out,est_out,act_out,trip_type,litera,park_in,park_out,remark,"\
    "       pr_tranzit,pr_reg,points.pr_del pr_del,points.tid tid, "\
    "       tz_regions.region region "\
    " FROM points,airps,cities,tz_regions "
    "WHERE points.airp=airps.code AND airps.city=cities.code AND "\
    "      cities.country=tz_regions.country(+) AND cities.tz=tz_regions.tz(+) AND "\
    "      points.pr_del!=-1 "\
    "ORDER BY points.move_id,point_num,point_id ";
const char* classesSQL =
    "SELECT point_id,class,cfg "\
    " FROM trip_classes,classes "\
    "WHERE trip_classes.class=classes.code "\
    "ORDER BY point_id,priority";
const char* regSQL =
    "SELECT pax_grp.point_dep point_id, SUM(pax.seats) as reg FROM pax_grp, pax "\
    " WHERE pax_grp.grp_id=pax.grp_id AND pax.pr_brd IS NOT NULL "\
    "GROUP BY pax_grp.point_dep "\
    "ORDER BY pax_grp.point_dep ";
const char* resaSQL =
    "SELECT point_dep point_id,SUM(crs_ok) as resa FROM counters2 "\
    " GROUP BY point_dep "\
    "ORDER BY point_dep ";
const char *stagesSQL =
    "SELECT point_id,stage_id,scd,est,act,pr_auto,pr_manual FROM trip_stages "\
    " ORDER BY point_id,stage_id ";
const char* stationsSQL =
    "SELECT point_id,stations.name,stations.work_mode FROM stations,trip_stations "\
    " WHERE stations.desk=trip_stations.desk AND stations.work_mode=trip_stations.work_mode "\
    " ORDER BY point_id,stations.work_mode,stations.name";

struct TDest2 {
  int point_id;
  int point_num;
  string airp;
  string city;
  int first_point;
  string airline;
  int flt_no;
  string suffix;
  string craft;
  string bort;
  TDateTime scd_in;
  TDateTime est_in;
  TDateTime act_in;
  TDateTime scd_out;
  TDateTime est_out;
  TDateTime act_out;
  string triptype;
  string litera;
  string park_in;
  string park_out;
  string remark;
  int pr_tranzit;
  int pr_reg;
  int pr_del;
  int tid;
  string region;
};

struct TSoppStage {
  int stage_id;
  TDateTime scd;
  TDateTime est;
  TDateTime act;
  bool pr_auto;
  bool pr_manual;
};
typedef vector<TSoppStage> tstages;


struct TStation {
  string name;
  string work_mode;
};
typedef vector<TStation> tstations;


struct TTrip {
  int tid;
  int move_id;
  int point_id;

  string airline_in;
  int flt_no_in;
  string suffix_in;
  string craft_in;
  string bort_in;
  TDateTime scd_in;
  TDateTime est_in;
  TDateTime act_in;
  string triptype_in;
  string litera_in;
  string park_in;
  string remark_in;
  int pr_del_in;
  vector<string> places_in;

  string airp;

  string airline_out;
  int flt_no_out;
  string suffix_out;
  string craft_out;
  string bort_out;
  TDateTime scd_out;
  TDateTime est_out;
  TDateTime act_out;
  string triptype_out;
  string litera_out;
  string park_out;
  string remark_out;
  int pr_del_out;
  int pr_reg;
  vector<string> places_out;

  string classes;
  int reg;
  int resa;
  vector<TSoppStage> stages;
  tstations stations;
  string crs_disp_from;
  string crs_disp_to;

  string region;

  TTrip() {
    flt_no_in = NoExists;
    scd_in = NoExists;
    est_in = NoExists;
    act_in = NoExists;
    pr_del_in = -1;

    flt_no_out = NoExists;
    scd_out = NoExists;
    est_out = NoExists;
    act_out = NoExists;
    pr_del_out = -1;
    reg = 0;
    resa = 0;
    pr_reg = 0;
  }
};

typedef vector<TDest2> TDests;
typedef vector<TTrip> TTrips;
typedef map<int,TDests> tmapds;

struct tcrs_displ {
  int point_id;
  string trip;
  string airp_arv;
  string cl;
};

typedef vector<tcrs_displ> TCRS_Displ;

struct TCRS_Displaces {
  map<int,TCRS_Displ> displaces_from;
  map<int,TCRS_Displ> displaces_to;
};

void GetCRS_Displaces( TCRS_Displaces &crsd );
void GetToFrom( int point_id, TCRS_Displaces &crsd, string &str_from, string &str_to );

TTrip createTrip( int move_id, TDests::iterator &id, TDests &dests )
{
  TTrip trip;
  trip.move_id = move_id;
  trip.point_id = id->point_id;
  trip.tid = id->tid;
  int first_point;
  if ( id->pr_tranzit )
    first_point = id->first_point;
  else
    first_point = id->point_id;
  TDests::iterator pd = dests.end();
  for ( TDests::iterator fd=dests.begin(); fd!=dests.end(); fd++ ) {
  	if ( fd->point_num < id->point_num ) {
  		if ( fd->first_point == first_point || fd->point_id == first_point) {
  			if ( id->pr_del == 1 || id->pr_del == fd->pr_del ) {
          trip.places_in.push_back( fd->airp );
          pd = fd;
        }  			
  		}
    }
    else
      if ( fd->point_num > id->point_num && fd->first_point == first_point )
      	if ( id->pr_del == 1 || id->pr_del == fd->pr_del ) {
          trip.places_out.push_back( fd->airp );
        }
  }
  if ( !trip.places_in.empty() ) { // trip is landing
    trip.airline_in = pd->airline;
    trip.flt_no_in = pd->flt_no;
    trip.suffix_in = pd->suffix;
    trip.craft_in = pd->craft;
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

  if ( !trip.places_out.empty() ) { // trip is takeoffing
    trip.airline_out = id->airline;
    trip.flt_no_out = id->flt_no;
    trip.suffix_out = id->suffix;
    trip.craft_out = id->craft;
    trip.bort_out = id->bort;
    trip.scd_out = id->scd_out;
    trip.est_out = id->est_out;
    trip.act_out = id->act_out;
    trip.triptype_out = id->triptype;
    trip.litera_out = id->litera;
    trip.park_out = id->park_out;
    trip.remark_out = id->remark;
    trip.pr_del_out = id->pr_del;
    trip.pr_reg = id->pr_reg;
  }
  trip.region = id->region;
  return trip;
}

void SoppInterface::ReadTrips(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  ProgTrace( TRACE5, "ReadTrips" );
  TReqInfo *reqInfo = TReqInfo::Instance();
  xmlNodePtr dataNode = NewTextChild( resNode, "data" );
  if ( GetNode( "CorrectStages", reqNode ) ) {
  	tst();
//    TStagesRules::Instance()->Build( dataNode );
  }
  TQuery PointsQry( &OraSession );
  PointsQry.SQLText = pointsSQL;

  TQuery ClassesQry( &OraSession );
  ClassesQry.SQLText = classesSQL;
  TQuery RegQry( &OraSession );
  RegQry.SQLText = regSQL;
  TQuery ResaQry( &OraSession );
  ResaQry.SQLText = resaSQL;
  TQuery StagesQry( &OraSession );
  StagesQry.SQLText = stagesSQL;
  TQuery StationsQry( &OraSession );
  StationsQry.SQLText = stationsSQL;

  PointsQry.Execute();
  ClassesQry.Execute();
  RegQry.Execute();
  ResaQry.Execute();
  StagesQry.Execute();
  StationsQry.Execute();
  TDests dests;
  TTrips trips;
  
  int move_id = NoExists;
  bool canUseAirline, canUseAirp; 
  while ( !PointsQry.Eof ) {
    if ( move_id != PointsQry.FieldAsInteger( "move_id" ) ) {
      if ( move_id > NoExists && canUseAirline && canUseAirp ) {
        //create trips
//        ProgTrace( TRACE5, "create trips with move_id=%d", move_id );
        for( TDests::iterator id=dests.begin(); id!=dests.end(); id++ ) {
          if ( reqInfo->user.user_type != utAirport ||
               find( reqInfo->user.access.airlines.begin(),
                     reqInfo->user.access.airlines.end(),
                     id->airline
                    ) != reqInfo->user.access.airlines.end() ) {
            trips.push_back( createTrip( move_id, id, dests ) );
          }
        }
      }
      move_id = PointsQry.FieldAsInteger( "move_id" );
      dests.clear();
      if ( reqInfo->user.user_type == utSupport ) {
        canUseAirline = reqInfo->user.access.airlines.empty();
        canUseAirp = reqInfo->user.access.airps.empty();
      }
      else {
       canUseAirline = ( reqInfo->user.user_type == utAirport && reqInfo->user.access.airlines.empty() );
       canUseAirp = ( reqInfo->user.user_type == utAirline && reqInfo->user.access.airps.empty() );
      }
    }
    TDest2 d;
    d.point_id = PointsQry.FieldAsInteger( "point_id" );
    d.point_num = PointsQry.FieldAsInteger( "point_num" );

    d.airp = PointsQry.FieldAsString( "airp" );
    d.city = PointsQry.FieldAsString( "city" );

    if ( PointsQry.FieldIsNULL( "first_point" ) )
      d.first_point = NoExists;
    else
      d.first_point = PointsQry.FieldAsInteger( "first_point" );
    d.airline = PointsQry.FieldAsString( "airline" );
    if ( PointsQry.FieldIsNULL( "flt_no" ) )
      d.flt_no = NoExists;
    else
      d.flt_no = PointsQry.FieldAsInteger( "flt_no" );
    d.suffix = PointsQry.FieldAsString( "suffix" );
    d.craft = PointsQry.FieldAsString( "craft" );
    d.bort = PointsQry.FieldAsString( "bort" );
    if ( PointsQry.FieldIsNULL( "scd_in" ) )
      d.scd_in = NoExists;
    else
      d.scd_in = PointsQry.FieldAsDateTime( "scd_in" );
    if ( PointsQry.FieldIsNULL( "est_in" ) )
      d.est_in = NoExists;
    else
      d.est_in = PointsQry.FieldAsDateTime( "est_in" );
    if ( PointsQry.FieldIsNULL( "act_in" ) )
      d.act_in = NoExists;
    else
      d.act_in = PointsQry.FieldAsDateTime( "act_in" );
    if ( PointsQry.FieldIsNULL( "scd_out" ) )
      d.scd_out = NoExists;
    else
      d.scd_out = PointsQry.FieldAsDateTime( "scd_out" );
    if ( PointsQry.FieldIsNULL( "est_out" ) )
      d.est_out = NoExists;
    else
      d.est_out = PointsQry.FieldAsDateTime( "est_out" );
    if ( PointsQry.FieldIsNULL( "act_out" ) )
      d.act_out = NoExists;
    else
      d.act_out = PointsQry.FieldAsDateTime( "act_out" );
    d.triptype = PointsQry.FieldAsString( "trip_type" );
    d.litera = PointsQry.FieldAsString( "litera" );
    d.park_in = PointsQry.FieldAsString( "park_in" );
    d.park_out = PointsQry.FieldAsString( "park_out" );
    d.remark = PointsQry.FieldAsString( "remark" );
    d.pr_tranzit = PointsQry.FieldAsInteger( "pr_tranzit" );
    d.pr_reg = PointsQry.FieldAsInteger( "pr_reg" );
    d.pr_del = PointsQry.FieldAsInteger( "pr_del" );
    d.tid = PointsQry.FieldAsInteger( "tid" );
    d.region = PointsQry.FieldAsString( "region" );
    if ( !canUseAirp && 
         find( reqInfo->user.access.airps.begin(),
               reqInfo->user.access.airps.end(),
               d.airp
             ) != reqInfo->user.access.airps.end() ) { 
      canUseAirp = true;
    }
    if ( !canUseAirline &&
         find( reqInfo->user.access.airlines.begin(),
               reqInfo->user.access.airlines.end(),
               d.airline
             ) != reqInfo->user.access.airlines.end() ) {
      canUseAirline = true;
    }
    dests.push_back( d );
    PointsQry.Next();
  } // end while !PointsQry.Eof
  if ( move_id > NoExists && canUseAirline && canUseAirp ) {
        //create trips
    tst();
    for( TDests::iterator id=dests.begin(); id!=dests.end(); id++ ) {
      if ( reqInfo->user.user_type != utAirport ||
           find( reqInfo->user.access.airlines.begin(),
                 reqInfo->user.access.airlines.end(),
                 id->airline
                ) != reqInfo->user.access.airlines.end() ) {
        trips.push_back( createTrip( move_id, id, dests ) );
      }
    }
  }
  tst();

  xmlNodePtr tripsNode = NULL;

  map<int,string> classesmap;
  map<int,int> regmap;
  map<int,int> resamap;
  map<int,tstages> stagesmap;
  map<int,tstations> stationsmap;
  bool find;
  int p_id;
  tst();
  // рейсы созданы, перейдем к набору информации по рейсам
  ////////////////////////// crs_displaces ///////////////////////////////
  TCRS_Displaces crsd;
  GetCRS_Displaces( crsd ); //пересадки
  tst();

  for ( TTrips::iterator tr=trips.begin(); tr!=trips.end(); tr++ ) {
    if ( !tr->places_out.empty() ) {
      // добор информации
      find = false;
      while ( !ClassesQry.Eof && tr->point_id >= ClassesQry.FieldAsInteger( "point_id" ) ) {
        if ( tr->point_id == ClassesQry.FieldAsInteger( "point_id" ) ) {
          find = true;
          if ( !tr->classes.empty() )
            tr->classes += " ";
          tr->classes = string(ClassesQry.FieldAsString( "class" )) + string(ClassesQry.FieldAsString( "cfg" ));
        }
        else {
          p_id = ClassesQry.FieldAsInteger( "point_id" );
          string &str_classes = classesmap[ p_id ];
          if ( !str_classes.empty() )
            str_classes += " ";
          str_classes = string(ClassesQry.FieldAsString( "class" )) + string(ClassesQry.FieldAsString( "cfg" ));
          classesmap[ p_id ] = str_classes;
        }
        ClassesQry.Next();
      }
      if ( !find && classesmap.find( tr->point_id ) != classesmap.end() )
        tr->classes = classesmap[ tr->point_id ];
      ///////////////////////////// reg /////////////////////////
      find = false;
      while ( !RegQry.Eof && tr->point_id >= RegQry.FieldAsInteger( "point_id" ) ) {
        if ( tr->point_id == RegQry.FieldAsInteger( "point_id" ) ) {
          find = true;
          tr->reg = RegQry.FieldAsInteger( "reg" );
      	  break;
        }
        else {
          p_id = RegQry.FieldAsInteger( "point_id" );
          regmap[ p_id ] = RegQry.FieldAsInteger( "reg" );
        }
        RegQry.Next();
      }
      if ( !find && regmap.find( tr->point_id ) != regmap.end( ) )
          tr->reg = regmap[ tr->point_id ];
      ///////////////////////// resa ///////////////////////////
      find = false;
      while ( !ResaQry.Eof && tr->point_id >= ResaQry.FieldAsInteger( "point_id" ) ) {
        if ( tr->point_id == ResaQry.FieldAsInteger( "point_id" ) ) {
          find = true;
          tr->resa = ResaQry.FieldAsInteger( "resa" );
          break;
        }
        else {
          p_id = ResaQry.FieldAsInteger( "point_id" );
          resamap[ p_id ] = ResaQry.FieldAsInteger( "resa" );
        }
        ResaQry.Next();
      }
      if ( !find && resamap.find( tr->point_id ) != resamap.end() )
        tr->resa = resamap[ tr->point_id ];
      ////////////////////// stages ///////////////////////////////
      find = false;
      while ( !StagesQry.Eof && tr->point_id >= StagesQry.FieldAsInteger( "point_id" ) ) {
        TSoppStage stage;
        stage.stage_id = StagesQry.FieldAsInteger( "stage_id" );
        if ( StagesQry.FieldIsNULL( "scd" ) )
          stage.scd = NoExists;
        else
          stage.scd = StagesQry.FieldAsDateTime( "scd" );
        if ( StagesQry.FieldIsNULL( "est" ) )
          stage.est = NoExists;
        else
          stage.est = StagesQry.FieldAsDateTime( "est" );
        if ( StagesQry.FieldIsNULL( "act" ) )
          stage.act = NoExists;
        else
          stage.act = StagesQry.FieldAsDateTime( "act" );
        stage.pr_manual = StagesQry.FieldAsInteger( "pr_manual" );
        stage.pr_auto = StagesQry.FieldAsInteger( "pr_auto" );

        if ( tr->point_id == StagesQry.FieldAsInteger( "point_id" ) ) {
          find = true;
          tr->stages.push_back( stage );
        }
        else {
          p_id = StagesQry.FieldAsInteger( "point_id" );
          stagesmap[ p_id ].push_back( stage );
        }
        StagesQry.Next();
      }
      if ( !find && stagesmap.find( tr->point_id ) != stagesmap.end() )
        tr->stages = stagesmap[ tr->point_id ];
      ////////////////////////// stations //////////////////////////////
      find = false;
      tst();
      while ( !StationsQry.Eof && tr->point_id >= StationsQry.FieldAsInteger( "point_id" ) ) {
        TStation station;
        station.name = StationsQry.FieldAsString( "name" );
        station.work_mode = StationsQry.FieldAsString( "work_mode" );
        if ( tr->point_id == StationsQry.FieldAsInteger( "point_id" ) ) {
          find = true;
          tr->stations.push_back( station );
        }
        else {
          p_id = StationsQry.FieldAsInteger( "point_id" );
          stationsmap[ p_id ].push_back( station );
        }
        StationsQry.Next();
      }
      tst();
      if ( !find && stationsmap.find( tr->point_id ) != stationsmap.end() )
        tr->stations = stationsmap[ tr->point_id ];
      GetToFrom( tr->point_id, crsd, tr->crs_disp_from, tr->crs_disp_to );
    } // end if (!place_out.empty())
    else
      if ( tr->places_in.empty() ) // такой рейс не отображаем
        continue;
    if ( !tripsNode )
      tripsNode = NewTextChild( dataNode, "trips" );
    xmlNodePtr tripNode = NewTextChild( tripsNode, "trip" );
    NewTextChild( tripNode, "move_id", tr->move_id );
    NewTextChild( tripNode, "point_id", tr->point_id );
    if ( !tr->airline_in.empty() )
      NewTextChild( tripNode, "airline_in", tr->airline_in );
    if ( tr->flt_no_in > NoExists )
      NewTextChild( tripNode, "flt_no_in", tr->flt_no_in );
    if ( !tr->suffix_in.empty() )
      NewTextChild( tripNode, "suffix_in", tr->suffix_in );
    if ( tr->craft_in != tr->craft_out && !tr->craft_in.empty() )
      NewTextChild( tripNode, "craft_in", tr->craft_in );
    if ( tr->bort_in != tr->bort_out  && !tr->bort_in.empty() )
      NewTextChild( tripNode, "bort_in", tr->bort_in );
    if ( tr->scd_in > NoExists )
      NewTextChild( tripNode, "scd_in", DateTimeToStr( UTCToClient( tr->scd_in, tr->region ) ) );
    if ( tr->est_in > NoExists )
      NewTextChild( tripNode, "est_in", DateTimeToStr( UTCToClient( tr->est_in, tr->region ) ) );
    if ( tr->act_in > NoExists )
      NewTextChild( tripNode, "act_in", DateTimeToStr( UTCToClient( tr->act_in, tr->region ) ) );
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
    for ( vector<string>::iterator sairp=tr->places_in.begin(); sairp!=tr->places_in.end(); sairp++ ) {
      if ( !lNode )
        lNode = NewTextChild( tripNode, "places_in" );
      NewTextChild( lNode, "airp", *sairp );
    }
    

    NewTextChild( tripNode, "airp", tr->airp );

    if ( !tr->airline_out.empty() )
      NewTextChild( tripNode, "airline_out", tr->airline_out );
    if ( tr->flt_no_out > NoExists )
      NewTextChild( tripNode, "flt_no_out", tr->flt_no_out );
    if ( !tr->suffix_out.empty() )
      NewTextChild( tripNode, "suffix_out", tr->suffix_out );
    if ( !tr->craft_out.empty() )
      NewTextChild( tripNode, "craft_out", tr->craft_out );
    if ( !tr->bort_out.empty() )
      NewTextChild( tripNode, "bort_out", tr->bort_out );
    if ( tr->scd_out > NoExists ) {
//    	ProgTrace( TRACE5, "tr->scd_out=%f, region=%s, point_id=%d",tr->scd_out, tr->region.c_str(), tr->point_id );
      NewTextChild( tripNode, "scd_out", DateTimeToStr( UTCToClient( tr->scd_out, tr->region ) ) );
    }
    if ( tr->est_out > NoExists )
      NewTextChild( tripNode, "est_out", DateTimeToStr( UTCToClient( tr->est_out, tr->region ) ) );
    if ( tr->act_out > NoExists )
      NewTextChild( tripNode, "act_out", DateTimeToStr( UTCToClient( tr->act_out, tr->region ) ) );
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
    NewTextChild( tripNode, "pr_reg", tr->pr_reg );
    lNode = NULL;
    for ( vector<string>::iterator sairp=tr->places_out.begin(); sairp!=tr->places_out.end(); sairp++ ) {
      if ( !lNode )
        lNode = NewTextChild( tripNode, "places_out" );
      NewTextChild( lNode, "airp", *sairp );
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
    lNode = NULL;
    for ( tstages::iterator st=tr->stages.begin(); st!=tr->stages.end(); st++ ) {
      if ( !lNode )
        lNode = NewTextChild( tripNode, "stages" );
      xmlNodePtr stageNode = NewTextChild( lNode, "stage" );
      NewTextChild( stageNode, "stage_id", st->stage_id );
      if ( st->scd > NoExists )
        NewTextChild( stageNode, "scd", DateTimeToStr( UTCToClient( st->scd, tr->region ) ) );
      if ( st->est > NoExists )
        NewTextChild( stageNode, "est", DateTimeToStr( UTCToClient( st->est, tr->region ) ) );
      if ( st->act > NoExists )
        NewTextChild( stageNode, "act", DateTimeToStr( UTCToClient( st->act, tr->region ) ) );
      NewTextChild( stageNode, "pr_auto", st->pr_auto );
      NewTextChild( stageNode, "pr_manual", st->pr_manual );
    }
    lNode = NULL;
    for ( tstations::iterator st=tr->stations.begin(); st!=tr->stations.end(); st++ ) {
      if ( !lNode )
        lNode = NewTextChild( tripNode, "stations" );
      xmlNodePtr stationNode = NewTextChild( lNode, "station" );
      NewTextChild( stationNode, "name", st->name );
      NewTextChild( stationNode, "work_mode", st->work_mode );
    }
  } // end for trip
}

//!!! только на вылет
void GetCRS_Displaces( TCRS_Displaces &crsd )
{
  crsd.displaces_from.clear();
  crsd.displaces_to.clear();
  TQuery Qry(&OraSession);
  Qry.SQLText =
    "SELECT point_from,"\
    "       p1.airline||p1.flt_no||p1.suffix trip_from,p1.scd_out scd_from,"\
    "       system.AIRPTZREGION( p1.airp ) region_from, "\
    "       airp_arv_from,class_from,point_to,"\
    "       p2.airline||p2.flt_no||p2.suffix trip_to,p2.scd_out scd_to,"\
    "       system.AIRPTZREGION( p2.airp ) region_to, "\
    "       airp_arv_to,class_to "\
    " FROM points p1, points p2, crs_displace "\
    "WHERE p1.point_id=crs_displace.point_from AND "\
    "      p2.point_id=crs_displace.point_to";
  Qry.Execute();
  double sd,d1;
  modf( NowUTC(), &sd );
  while ( !Qry.Eof ) {
    tcrs_displ dis;
    dis.point_id = Qry.FieldAsInteger( "point_from" );
    dis.trip = Qry.FieldAsString( "trip_from" );
    modf( UTCToClient( Qry.FieldAsDateTime( "scd_from" ), Qry.FieldAsString( "region_from" ) ), &d1 );
    if ( sd != d1 )
      dis.trip += DateTimeToStr( d1, "(dd)" );
    dis.airp_arv = Qry.FieldAsString( "airp_arv_from" );
    dis.cl = Qry.FieldAsString( "class_from" );
    crsd.displaces_from[ Qry.FieldAsInteger( "point_to" ) ].push_back( dis );
    dis.point_id = Qry.FieldAsInteger( "point_to" );
    dis.trip = Qry.FieldAsString( "trip_to" );
    modf( UTCToClient( Qry.FieldAsDateTime( "scd_to" ), Qry.FieldAsString( "region_to" ) ), &d1 );
    if ( sd != d1 )
      dis.trip += DateTimeToStr( d1, "(dd)" );
    dis.airp_arv = Qry.FieldAsString( "airp_arv_to" );
    dis.cl = Qry.FieldAsString( "class_to" );
    crsd.displaces_to[ Qry.FieldAsInteger( "point_from" ) ].push_back( dis );
    Qry.Next();
  }
}

void GetToFrom( int point_id, TCRS_Displaces &crsd, string &str_from, string &str_to )
{
  TCRS_Displ &v_to = crsd.displaces_to[ point_id ];
  bool ch_class = false;
  bool ch_dest = false;
  for ( TCRS_Displ::iterator to=v_to.begin(); to!=v_to.end(); to++ ) {
  if ( point_id == to->point_id ) { //в сам себя, тогда
    TCRS_Displ &v_from = crsd.displaces_from[ point_id ];
    for ( TCRS_Displ::iterator from=v_from.begin(); from!=v_from.end(); from++ ) {
      if ( to->cl != from->cl )
        ch_class = true;
      if ( to->airp_arv != from->airp_arv )
        ch_dest = true;
    }
  }
  else {
    if ( str_to.find( to->trip ) != string::npos ) {
      if ( !str_to.empty() )
        str_to += " ";
        str_to += to->trip;
      }
    }
  }
  TCRS_Displ &v_from = crsd.displaces_from[ point_id ];
  for ( TCRS_Displ::iterator from=v_from.begin(); from!=v_from.end(); from++ ) {
    if ( point_id == from->point_id )
      continue;
    if ( str_from.find( from->trip ) != string::npos ) {
      if ( !str_from.empty() )
        str_from += " ";
      str_from += from->trip;
    }
  }
  if ( ch_class )
    str_from = "Изм. класса " + str_from;
  if ( ch_dest )
    str_from = "Изм. пункта " + str_from;
}

void SoppInterface::GetTransfer(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode,
                                bool pr_bag)
{
  TQuery Qry(&OraSession);
  bool pr_out=NodeAsInteger("pr_out",reqNode)!=0;
  int point_id=NodeAsInteger("point_id",reqNode);
  if (pr_out)



    Qry.SQLText=
      "SELECT company AS airline,flt_no,suffix, "
      "       TO_CHAR(NVL(act,NVL(est,scd)),'DD')|| "
      "       DECODE(TRUNC(NVL(act,NVL(est,scd))),TRUNC(scd),'', "
      "              TO_CHAR(scd,'(DD)')) AS scd "
      "FROM trips "
      "WHERE trip_id=:point_id AND status>=0";
  else
    Qry.SQLText=
      "SELECT company AS airline,flt_no,suffix, "
      "       TO_CHAR(NVL(act,NVL(est,scd)),'DD')|| "
      "       DECODE(TRUNC(NVL(act,NVL(est,scd))),TRUNC(scd),'', "
      "              TO_CHAR(scd,'(DD)')) AS scd "
      "FROM trips_in "
      "WHERE trip_id=:point_id AND status>=0";
  Qry.CreateVariable("point_id",otInteger,point_id);
  Qry.Execute();
  if (Qry.Eof) throw UserException("__cб -_ - c¤_-");
  ostringstream trip;
  trip << Qry.FieldAsString("airline")
       << Qry.FieldAsInteger("flt_no")
       << Qry.FieldAsString("suffix") << "/"
       << Qry.FieldAsString("scd");


  NewTextChild(resNode,"trip",trip.str());

  Qry.Clear();
  if (pr_out)
    Qry.SQLText=
      "SELECT tlg_trips.point_id,tlg_trips.airline,tlg_trips.flt_no,tlg_trips.suffix, "
      "       tlg_trips.scd,tlg_trips.airp_dep AS airp, "
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
      "       tlg_trips.scd,tlg_trips.airp_arv AS airp, "
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

  TQuery PaxQry(&OraSession);
  PaxQry.SQLText="SELECT surname,name FROM trfer_pax WHERE grp_id=:grp_id ORDER BY surname,name";
  PaxQry.DeclareVariable("grp_id",otInteger);

  TQuery TagQry(&OraSession);
  TagQry.SQLText=
    "SELECT TRUNC(no/1000) AS pack, "
    "       MOD(no,1000) AS no "
    "FROM trfer_tags WHERE grp_id=:grp_id ORDER BY trfer_tags.no";
  TagQry.DeclareVariable("grp_id",otFloat);

  Qry.Execute();
  xmlNodePtr trferNode=NewTextChild(resNode,"transfer");
  xmlNodePtr grpNode,paxNode,node,node2;
  int grp_id;
  char subcl[2];
  point_id=-1;
  *subcl=0;
  for(;!Qry.Eof;Qry.Next())
  {
    if (point_id==-1 ||
        point_id!=Qry.FieldAsInteger("point_id") ||
        strcmp(subcl,Qry.FieldAsString("subcl"))!=0)
    {
      node=NewTextChild(trferNode,"trfer_flt");
      ostringstream trip;
      trip << Qry.FieldAsString("airline")
           << Qry.FieldAsInteger("flt_no")
           << Qry.FieldAsString("suffix") << "/"
           << DateTimeToStr(Qry.FieldAsDateTime("scd"),"dd");

      NewTextChild(node,"trip",trip.str());
      NewTextChild(node,"airp",Qry.FieldAsString("airp"));
      NewTextChild(node,"subcl",Qry.FieldAsString("subcl"));
      grpNode=NewTextChild(node,"grps");
      point_id=Qry.FieldAsInteger("point_id");
      strcpy(subcl,Qry.FieldAsString("subcl"));
    };

    node=NewTextChild(grpNode,"grp");
    if (!Qry.FieldIsNULL("seats"))
      NewTextChild(node,"seats",Qry.FieldAsInteger("seats"));
    else
      NewTextChild(node,"seats");
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
    if (!PaxQry.Eof)
    {
      paxNode=NewTextChild(node,"passengers");
      for(;!PaxQry.Eof;PaxQry.Next())
      {
        node2=NewTextChild(paxNode,"pax");
        NewTextChild(node2,"surname",PaxQry.FieldAsString("surname"));
        NewTextChild(node2,"name",PaxQry.FieldAsString("name"),"");
      };
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
	TQuery Qry(&OraSession);
	Qry.SQLText = 
	 "BEGIN "\
   " DECLARE "\
   " CURSOR cur IS "\
   "   SELECT grp_id FROM pax_grp WHERE point_dep=:point_id; "\
   " curRow      cur%ROWTYPE; "\
   " vpoint_id    points.point_id%TYPE; "\
   " BEGIN "\
   "  SELECT point_id INTO vpoint_id FROM points WHERE point_id=:point_id FOR UPDATE; "\
   "  UPDATE trip_comp_elems SET pr_free=1 WHERE point_id=:point_id; "\
   "  FOR curRow IN cur LOOP "\
   "    UPDATE pax SET refuse='А',pr_brd=NULL,seat_no=NULL WHERE grp_id=curRow.grp_id; "\
   "    mvd.sync_pax_grp(curRow.grp_id,:term); "\
   "    ckin.check_grp(curRow.grp_id); "\
   "  END LOOP; "\
   "  ckin.recount(:point_id); "\
   " EXCEPTION "\
   "  WHEN NO_DATA_FOUND THEN "\
   "   IF vpoint_id IS NOT NULL THEN RAISE; END IF; "\
   " END;"\
   "END;";
  int point_id = NodeAsInteger( "point_id", reqNode );
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.CreateVariable( "term", otString, TReqInfo::Instance()->desk.code );
  Qry.Execute();
  TReqInfo::Instance()->MsgToLog( "Все пассажиры разрегистрированы", evtPax, point_id );
  showMessage( "Все пассажиры разрегистрированы" );
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
    	  Qry.SQLText = "INSERT INTO trip_stations(point_id,desk,work_mode) "\
    	                " SELECT :point_id,desk,:work_mode FROM stations,points "\
    	                "  WHERE points.point_id=:point_id AND stations.airp=points.airp AND name=:name";
    	  Qry.CreateVariable( "point_id", otInteger, point_id );
    	  Qry.DeclareVariable( "name", otString );
    	  Qry.CreateVariable( "work_mode", otString, work_mode );
    	  tst();
    	  stnode = ddddNode->children; //tag name
    	  string tolog;
    	  string name;
      	while ( stnode ) {
      		name = NodeAsString( stnode );
      		Qry.SetVariable( "name", name );
      		Qry.Execute();
      		if ( !tolog.empty() )
      				tolog += ", ";
      			tolog += name;
    		  stnode = stnode->next;
      	}
      	if ( work_mode == "Р" ) {
      	  if ( tolog.empty() )
      		  tolog = "Не назначены стойки регистрации";
      	  else
      	  	tolog = "Назначены стойки регистрации: " + tolog;
      	}
      	if ( work_mode == "П" ) {
        	if ( tolog.empty() )
      		  tolog = "Не назначены выходы на посадку";
      	  else
      	  	tolog = "Назначены выходы на посадку: " + tolog;
      	}
      	TReqInfo::Instance()->MsgToLog( tolog, evtFlt, point_id );				 
				ddddNode = ddddNode->next;
			}
		}
  	xmlNodePtr stagesNode = GetNode( "tripstages", node );
    if ( stagesNode ) {
    	tst();
  	  TMapTripStages stages;
  	  TTripStages::ParseStages( stagesNode, stages );
  	  TTripStages::WriteStages( point_id, stages );		
  	}
  	xmlNodePtr luggageNode = GetNode( "luggage", node );
  	if ( luggageNode ) {
  		tst();
  		xmlNodePtr max_cNode = GetNode( "max_commerce", luggageNode );
  		if ( max_cNode ) {
 		    Qry.Clear();
  	    Qry.SQLText = "UPDATE trip_sets SET max_commerce=:max_commerce WHERE point_id=:point_id";
  	    Qry.CreateVariable( "point_id", otInteger, point_id );
  	    int max_commerce = NodeAsInteger( max_cNode );
  	    Qry.CreateVariable( "max_commerce", otInteger, max_commerce );
  	    Qry.Execute();
  	    TReqInfo::Instance()->MsgToLog( string( "Макс. коммерческая загрузка: " ) + IntToString( max_commerce ) + "кг.", evtFlt, point_id );	
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
          	string( "Направление " ) + airp_arv + ": " +
            "груз " + IntToString( cargo ) + " кг., " +
            "почта " + IntToString( mail ) + " кг.", evtFlt, point_id );  				
  			  load = load->next;
  			}
  		}
  	}
		node = node->next;
	}
	showMessage( "Данные успешно сохранены" );
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
	tst();
	xmlNodePtr node = NewTextChild( dataNode, "birks" );
	NewTextChild( node, "nobrd", Qry.FieldAsInteger( "nobrd" ) );
	NewTextChild( node, "birks", Qry.FieldAsString( "birks" ) );
}

void GetLuggage( int point_id, xmlNodePtr dataNode )
{
	xmlNodePtr node = NewTextChild( dataNode, "luggage" );
	TQuery Qry(&OraSession);
	Qry.SQLText =
	 "SELECT "\
   " SUM(DECODE(pr_cabin,0,weight,0)) AS bag_weight, "\
   " SUM(DECODE(pr_cabin,1,weight,0)) AS rk_weight "\
   " FROM bag2, "\
   "   (SELECT DISTINCT pax_grp.grp_id FROM pax_grp,pax "\
   "     WHERE pax_grp.grp_id=pax.grp_id AND point_dep=:point_id AND pr_brd=1) pax_grp "\
   "WHERE bag2.grp_id=pax_grp.grp_id ";
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.Execute();
  tst();
  int bag_weight = Qry.FieldAsInteger( "bag_weight" );
  int rk_weight = Qry.FieldAsInteger( "rk_weight" );
  Qry.Clear();
  Qry.SQLText =
   "SELECT "\
   " SUM(DECODE(pers_type,'ВЗ',1,0)) AS adult, "\
   " SUM(DECODE(pers_type,'РБ',1,0)) AS child, "\
   " SUM(DECODE(pers_type,'РМ',1,0)) AS baby "\
   " FROM pax_grp,pax "\
   "WHERE pax_grp.grp_id=pax.grp_id AND point_dep=:point_id AND pr_brd=1";
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.Execute();
  tst();
  int adult = Qry.FieldAsInteger( "adult" );
  int child = Qry.FieldAsInteger( "child" );
  int baby = Qry.FieldAsInteger( "baby" );
  Qry.Clear();
  Qry.SQLText = 
   "SELECT NVL(SUM(weight),0) AS dosbag_weight "\
   " FROM unaccomp_bag "\
   "WHERE point_dep=:point_id";
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.Execute();
  tst();
  int dosbag_weight = Qry.FieldAsInteger( "dosbag_weight" );

  Qry.Clear();  
  Qry.SQLText = 
   "SELECT tz_regions.region region,act_out,points.pr_del pr_del,max_commerce,pr_tranzit,first_point,point_num "\
   " FROM points,trip_sets,airps,cities,tz_regions "
    "WHERE points.point_id=:point_id AND trip_sets.point_id=:point_id AND "\
    "      points.airp=airps.code AND airps.city=cities.code AND "\
    "      cities.country=tz_regions.country(+) AND cities.tz=tz_regions.tz(+)";
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.Execute();
  tst();
  NewTextChild( node, "max_commerce", Qry.FieldAsInteger( "max_commerce" ) );
  tst();
  int pr_edit = !Qry.FieldIsNULL( "act_out" ) || Qry.FieldAsInteger( "pr_del" ) != 0;
  NewTextChild( node, "pr_edit", pr_edit );
  string region = Qry.FieldAsString( "region" );
  int pr_tranzit = Qry.FieldAsInteger( "pr_tranzit" );
  int first_point = Qry.FieldAsInteger( "first_point" );
  int point_num = Qry.FieldAsInteger( "point_num" );

  // определяем лето ли сейчас
  int summer = is_dst( NowUTC(), region );
  Qry.Clear();
  Qry.SQLText = 
   "SELECT "\
   " code, DECODE(:summer,1,weight_sum,weight_win) as weight "\
   " FROM pers_types"; 
  Qry.CreateVariable( "summer", otInteger, summer );
	Qry.Execute();
	tst();
	xmlNodePtr wm = NewTextChild( node, "weightman" );
	while ( !Qry.Eof ) {
		xmlNodePtr weightNode = NewTextChild( wm, "weight" );
		NewTextChild( weightNode, "code", Qry.FieldAsString( "code" ) );
		NewTextChild( weightNode, "weight", Qry.FieldAsInteger( "weight" ) );
		Qry.Next();
	}
	NewTextChild( node, "bag_weight", bag_weight + dosbag_weight );
	NewTextChild( node, "rk_weight", rk_weight );
	NewTextChild( node, "adult", adult );
	NewTextChild( node, "child", child );
	NewTextChild( node, "baby", baby );
	
	
	if ( !pr_tranzit )
    first_point = point_id;
	Qry.Clear();
	Qry.SQLText =
	 "SELECT cargo,mail,a.airp airp_arv,a.point_id point_arv "\
	 " FROM trip_load, "\
	 "( SELECT point_id, point_num, airp FROM points "
	 "   WHERE first_point=:first_point AND point_num>:point_num AND pr_del=0 ) a, "\
	 "( SELECT MIN(point_num) as point_num FROM points "\
	 "   WHERE first_point=:first_point AND point_num>:point_num AND pr_del=0 "\
	 "  GROUP BY airp ) b "\
	 "WHERE a.point_num=b.point_num AND trip_load.point_dep(+)=:point_id AND "\
	 "      trip_load.point_arv(+)=a.point_id ";
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.CreateVariable( "first_point", otInteger, first_point );
  Qry.CreateVariable( "point_num", otInteger, point_num );
  Qry.Execute();	
  tst();
  xmlNodePtr loadNode = NewTextChild( node, "trip_load" );
  while ( !Qry.Eof ) {
  	xmlNodePtr fn = NewTextChild( loadNode, "load" );
  	NewTextChild( fn, "cargo", Qry.FieldAsInteger( "cargo" ) );
  	NewTextChild( fn, "mail", Qry.FieldAsInteger( "mail" ) );
  	NewTextChild( fn, "airp_arv", Qry.FieldAsString( "airp_arv" ) );
  	NewTextChild( fn, "point_arv", Qry.FieldAsInteger( "point_arv" ) );
  	Qry.Next();
  }
}

void SoppInterface::ReadTripInfo(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
	int point_id = NodeAsInteger( "point_id", reqNode );
	xmlNodePtr dataNode = NewTextChild( resNode, "data" );
	ProgTrace( TRACE5, "point_id=%d", point_id );
  if ( GetNode( "tripcounters", reqNode ) )
    readTripCounters( point_id, dataNode );
  if ( GetNode( "birks", reqNode ) ) {
  	tst();
  	GetBirks( point_id, dataNode );
  }
  tst();
  if ( GetNode( "luggage", reqNode ) ) {
  	tst();
  	GetLuggage( point_id, dataNode );
  }
tst();
  

 

}
