  #include <stdlib.h>
#include "points.h"
#include "pers_weights.h"
#include "stages.h"
#include "astra_utils.h"
#include "astra_misc.h"
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
#include "season.h" //???
#include "telegram.h"
#include "boost/date_time/local_time/local_time.hpp"
#include <boost/thread/thread.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include "base_tables.h"
#include "docs.h"
#include "stat.h"
#include "salons.h"
#include "seats.h"
#include "sopp.h"
#include "aodb.h"
#include "misc.h"
#include "term_version.h"
#include "trip_tasks.h"

#include "serverlib/perfom.h"

#define NICKNAME "DJEK"
#include "serverlib/test.h"

using namespace std;
using namespace BASIC;
using namespace EXCEPTIONS;
using namespace AstraLocale;
using namespace ASTRA;
using namespace boost::local_time;

const char* points_delays_SQL =
  "SELECT delay_code,time "
  " FROM trip_delays "
  "WHERE point_id=:point_id "
  "ORDER BY delay_num";

const char* points_dest_SQL =
  "SELECT point_num,airp,pr_tranzit,first_point,airline,flt_no,suffix,craft,"
  "       bort,scd_in,est_in,act_in,scd_out,est_out,act_out,trip_type,litera,"
  "       park_in,park_out,remark,pr_reg,pr_del,tid,airp_fmt,airline_fmt,"
  "       suffix_fmt,craft_fmt"
  " FROM points WHERE point_id=:point_id";


void parseFlt( const string &value, string &airline, int &flt_no, string &suffix )
{
  ProgTrace( TRACE5, "parseFlt: value=%s", value.c_str() );
  airline.clear();
  flt_no = NoExists;
  suffix.clear();
  string tmp = value;
  tmp = TrimString( tmp );
	if ( tmp.length() < 3 || tmp.length() > 8 )
		throw Exception( "Ошибка формата номера рейса, значение=%s", value.c_str() );
	int i;
	char c=0;
  char cairline[4];
  char csuffix[2];
  csuffix[ 0 ] = 0; csuffix[ 1 ] = 0;
  long cflt_no;
  if ( IsDigit( tmp[2] ) )
    i = sscanf( tmp.c_str(), "%2[A-ZА-ЯЁ0-9]%5lu%c%c",
                cairline,&cflt_no, &(csuffix[0]), &c );
  else
    i = sscanf( tmp.c_str(), "%3[A-ZА-ЯЁ0-9]%5lu%c%c",
                cairline,  &cflt_no, &(csuffix[0]), &c );
  if ( c != 0 || i < 2 || cflt_no < 0 )
    throw Exception( "Ошибка формата номера рейса, значение=%s", value.c_str() );
  if ( i == 3&& !IsUpperLetter( csuffix[0] ) )
    throw Exception( "Ошибка формата номера рейса, значение=%s", value.c_str() );
  airline = cairline;
  flt_no = cflt_no;
  suffix = csuffix;
}

void TPointsDest::getDestData( TQuery &Qry )
{
  if ( Qry.GetFieldIndex( "point_id" ) >= 0 )
    point_id = Qry.FieldAsInteger( "point_id" );
 	point_num = Qry.FieldAsInteger( "point_num" );
  if ( !Qry.FieldIsNULL( "first_point" ) )
    first_point = Qry.FieldAsInteger( "first_point" );
  else
    first_point = NoExists;
  airp = Qry.FieldAsString( "airp" );
  airp_fmt = (TElemFmt)Qry.FieldAsInteger( "airp_fmt" );
 	airline = Qry.FieldAsString( "airline" );
 	airline_fmt = (TElemFmt)Qry.FieldAsInteger( "airline_fmt" );
  if ( !Qry.FieldIsNULL( "flt_no" ) )
    flt_no = Qry.FieldAsInteger( "flt_no" );
  else
  	flt_no = NoExists;
 	suffix = Qry.FieldAsString( "suffix" );
 	suffix_fmt = (TElemFmt)Qry.FieldAsInteger( "suffix_fmt" );
 	craft = Qry.FieldAsString( "craft" );
 	craft_fmt = (TElemFmt)Qry.FieldAsInteger( "craft_fmt" );
 	bort = Qry.FieldAsString( "bort" );
  if ( !Qry.FieldIsNULL( "scd_in" ) )
	  scd_in = Qry.FieldAsDateTime( "scd_in" );
 	else
 	 	scd_in = NoExists;
 	if ( !Qry.FieldIsNULL( "est_in" ) )
 	  est_in = Qry.FieldAsDateTime( "est_in" );
 	else
 	  est_in = NoExists;
 	if ( !Qry.FieldIsNULL( "act_in" ) )
 	  act_in = Qry.FieldAsDateTime( "act_in" );
 	else
 	  act_in = NoExists;
 	if ( !Qry.FieldIsNULL( "scd_out" ) )
 	  scd_out = Qry.FieldAsDateTime( "scd_out" );
 	else
 	  scd_out = NoExists;
 	if ( !Qry.FieldIsNULL( "est_out" ) )
 	  est_out = Qry.FieldAsDateTime( "est_out" );
 	else
 	  est_out = NoExists;
 	if ( !Qry.FieldIsNULL( "act_out" ) )
 	  act_out = Qry.FieldAsDateTime( "act_out" );
 	else
 	  act_out = NoExists;
	trip_type = Qry.FieldAsString( "trip_type" );
  litera = Qry.FieldAsString( "litera" );
  park_in = Qry.FieldAsString( "park_in" );
  park_out = Qry.FieldAsString( "park_out" );
  pr_tranzit = Qry.FieldAsInteger( "pr_tranzit" );
  pr_reg = Qry.FieldAsInteger( "pr_reg" );
  pr_del = Qry.FieldAsInteger( "pr_del" );
  remark = Qry.FieldAsString( "remark" );
}

void TPointsDest::LoadProps( int vpoint_id, BitSet<TUseDestData> FUseData )
{
  if ( UseData.isFlag( udStages ) ) {
    tst();
    stages.Load( point_id );
  }
  if ( UseData.isFlag( udDelays ) ) {
    tst();
    delays.Load( point_id );
  }
  if ( UseData.isFlag( udCargo ) ) {
    tst();
    cargos.Load( point_id, pr_tranzit, first_point, point_num, pr_del );
  }
  if ( UseData.isFlag( udMaxCommerce ) ) {
    tst();
    max_commerce.Load( point_id );
  }
  if ( UseData.isFlag( udStations ) ) {
    tst();
    stations.Load( point_id );
  }
}

void TPointsDest::Load( int vpoint_id, BitSet<TUseDestData> FUseData )
{
  for ( int i=0; i<(int)udNum; i++ )
    if ( FUseData.isFlag( (TUseDestData)i ) )
      UseData.setFlag( (TUseDestData)i );
  status = tdUpdate;
  point_id = vpoint_id;
  
  TQuery Qry(&OraSession);
  Qry.SQLText = points_dest_SQL;
  Qry.CreateVariable( "point_id", otInteger, vpoint_id );
  Qry.Execute();
  getDestData( Qry );
  LoadProps( vpoint_id, FUseData );
}

void TPoints::Verify( bool ignoreException, LexemaData &lexemaData )
{
  TReqInfo *reqInfo = TReqInfo::Instance();
  if ( dests.items.size() < 2 )
  	throw AstraLocale::UserException( "MSG.CHECK_FLIGHT.ROUTE_LEAST_TWO_POINTS" );
  bool pr_last;
  if ( reqInfo->user.user_type != utSupport ) {
    bool pr_permit = reqInfo->user.user_type == utAirline;
    for( vector<TPointsDest>::iterator id=dests.items.begin(); id!=dests.items.end(); id++ ) {
    	if ( id->pr_del == -1 )
    		continue;
      if ( reqInfo->CheckAirp( id->airp ) ) {
        pr_permit = true;
      }
      pr_last = true;
      for ( vector<TPointsDest>::iterator ir=id + 1; ir!=dests.items.end(); ir++ ) {
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
    if ( !pr_permit ) {
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
  bool pr_other_airline = false;
  bool doubleTrip = false;
  try {
    // проверка на отмену + в маршруте участвует всего одна авиакомпания
    string old_airline;
    for( vector<TPointsDest>::iterator id=dests.items.begin(); id!=dests.items.end(); id++ ) {
      if ( id->pr_del == 0 ) {
      	if ( old_airline.empty() )
      		old_airline = id->airline;
      	if ( !id->airline.empty() && old_airline != id->airline )
      		pr_other_airline = true;
      }
    }

    // проверка упорядоченности времен + дублирование рейса, если move_id == NoExists
    TDateTime oldtime, curtime = NoExists;
    bool pr_time=false;
    for( vector<TPointsDest>::iterator id=dests.items.begin(); id!=dests.items.end(); id++ ) {
  	  if ( id->pr_del != -1 &&(id->scd_in > NoExists || id->scd_out > NoExists) )
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
        for ( vector<TPointsDest>::iterator xd=id+1; xd!=dests.items.end(); xd++ ) {
        	if ( xd->pr_del )
        		continue;
          throw AstraLocale::UserException( "MSG.CRAFT.NOT_SET" );
        }
      }
      if ( !doubleTrip &&
      	   id != dests.items.end() - 1 &&
      	   isDouble( move_id, id->airline, id->flt_no, id->suffix, id->airp,
                     id->scd_in, id->scd_out ) ) {
      	doubleTrip = true;
      	break;
      }
      if ( id->pr_del != -1 && id != dests.items.end() &&
           ( !id->delays.Empty() || id->est_out != NoExists ) ) {
        TTripInfo info;
        info.airline = id->airline;
        info.flt_no = id->flt_no;
        info.airp = id->airp;
        if ( GetTripSets( tsCheckMVTDelays, info ) ) { //проверка задержек на совместимость с телеграммами
          if ( id->delays.Empty() )
            throw AstraLocale::UserException( "MSG.MVTDELAY.NOT_SET" );
          TDateTime prior_delay_time  = id->scd_out;
          std::vector<TPointsDestDelay> vdelays;
          id->delays.Get( vdelays );
          for ( vector<TPointsDestDelay>::iterator q=vdelays.begin(); q!=vdelays.end(); q++ ) {
            if ( !check_delay_code( q->code ) )
              throw AstraLocale::UserException( "MSG.MVTDELAY.INVALID_CODE" );
            if ( check_delay_value( q->time - prior_delay_time ) )
              throw AstraLocale::UserException( "MSG.MVTDELAY.INVALID_TIME" );
            prior_delay_time = q->time;
          }
        }
      }
    } // end for
    if ( !pr_time )
    	throw AstraLocale::UserException( "MSG.ROUTE.IN_OUT_TIMES_NOT_SPECIFIED" );
  }
  catch( AstraLocale::UserException &e ) {
  	if ( !ignoreException ) {
  	  lexemaData = e.getLexemaData();
  	  return;
    }
  }

  if ( pr_other_airline )
    throw AstraLocale::UserException( "MSG.CHECK_FLIGHT.ROUTE_CANNOT_BELONG_TO_DIFFERENT_AIRLINES" );

  if ( doubleTrip )
    throw AstraLocale::UserException( "MSG.FLIGHT.DUPLICATE.ALREADY_EXISTS" );
    
}

inline bool isSetSCDTime( TStatus status, BASIC::TDateTime time, BASIC::TDateTime priortime )
{
  return ( status != tdDelete && status != tdInsert &&
           priortime == NoExists && time != NoExists );
}
inline bool isChangeTime( TStatus status, BASIC::TDateTime time, BASIC::TDateTime priortime )
{
  return ( status != tdDelete && status != tdInsert &&
           priortime != NoExists && time != NoExists && time != priortime );
}
inline bool isDeleteTime( TStatus status, BASIC::TDateTime time, BASIC::TDateTime priortime )
{
  return ( status != tdDelete && status != tdInsert &&
           priortime != NoExists && time == NoExists );
}
inline bool isSetOtherTime( TStatus status, BASIC::TDateTime time, BASIC::TDateTime priortime )
{
  return ( status != tdDelete &&
          ((status == tdInsert && time != NoExists) ||
           (status != tdInsert && priortime == NoExists && time != NoExists)) );
}

void TPointsDest::getEvents( const TPointsDest &vdest )
{
  ProgTrace( TRACE5, "TPointsDest::getEvents: point_id=%d", vdest.point_id );

  events.clearFlags();
  //создание шагов технологического графика
  if ( status != tdDelete && pr_reg &&
       ( status == tdInsert || stages.Empty() ) ) {
    events.setFlag( dmInitStages );
  }
  if ( UseData.isFlag( udNoCalcESTTimeStage ) && pr_reg &&
       ( status == tdInsert || !stages.equal( vdest.stages ) ) ) {
      ProgTrace( TRACE5, "getEvents: dmChangeStages" );
      events.setFlag( dmChangeStages );
  }
  //поиск подходящей компоновки
  if ( status != tdDelete && status != tdInsert ) {
    if ( ElemIdToElemCtxt(ecDisp,etCraft,craft,craft_fmt) != ElemIdToElemCtxt(ecDisp,etCraft,vdest.craft,vdest.craft_fmt) ) { // изменение типа ВС
      ProgTrace( TRACE5, "newcraft=%s ,oldcraft=%s", craft.c_str(), vdest.craft.c_str() );
      if ( !craft.empty() && vdest.craft.empty() ) {
        events.setFlag( dmSetCraft );
        tst();
      }
      else
        events.setFlag( dmChangeCraft );
      if ( !craft.empty() && pr_reg && !events.isFlag( dmInitStages ) ) // есть регистрация и не будет выполнения шагов тех. графика
        events.setFlag( dmInitComps );
    }
  }
  if ( status != tdDelete && status != tdInsert &&
       ElemIdToElemCtxt(ecDisp,etAirline,airline,airline_fmt)+
       IntToString(flt_no)+
       ElemIdToElemCtxt(ecDisp,etSuffix,suffix,suffix_fmt) !=
       ElemIdToElemCtxt(ecDisp,etAirline,vdest.airline,vdest.airline_fmt)+
       IntToString(vdest.flt_no)+
       ElemIdToElemCtxt(ecDisp,etSuffix,vdest.suffix,vdest.suffix_fmt) ) { //изменение рейса
    if ( ElemIdToElemCtxt(ecDisp,etAirline,airline,airline_fmt) != ElemIdToElemCtxt(ecDisp,etAirline,vdest.airline,vdest.airline_fmt) ) {
      tst();
      events.setFlag( dmChangeAirline );
    }
    if ( flt_no != vdest.flt_no )
      events.setFlag( dmChangeFltNo );
    if ( ElemIdToElemCtxt(ecDisp,etSuffix,suffix,suffix_fmt) != ElemIdToElemCtxt(ecDisp,etSuffix,vdest.suffix,vdest.suffix_fmt) ) {
      events.setFlag( dmChangeSuffix );
    }
    if ( pr_reg && !events.isFlag( dmInitStages ) ) // и не будет выполнения шагов тех. графика
      events.setFlag( dmInitComps );
  }
  if ( status != tdDelete && status != tdInsert &&
       ElemIdToElemCtxt(ecDisp,etAirp,airp,airp_fmt) != ElemIdToElemCtxt(ecDisp,etAirp,vdest.airp,vdest.airp_fmt) ) {
    tst();
    events.setFlag( dmChangeAirp );
  }
  
  if ( status != tdDelete && status != tdInsert &&
       !events.isFlag( dmChangeAirline ) ) {
    airline = vdest.airline;
    airline_fmt = vdest.airline_fmt;
  }
  if ( status != tdDelete && status != tdInsert &&
       !events.isFlag( dmChangeSuffix ) ) {
    suffix = vdest.suffix;
    suffix_fmt = vdest.suffix_fmt;
  }
  if ( status != tdDelete && status != tdInsert &&
       !events.isFlag( dmChangeAirp ) ) {
    airp = vdest.airp;
    airp_fmt = vdest.airp_fmt;
  }
  if ( status != tdDelete && status != tdInsert &&
       !events.isFlag( dmChangeCraft ) && !events.isFlag( dmSetCraft ) ) {
    craft = vdest.craft;
    craft_fmt = vdest.craft_fmt;
  }

  if ( status != tdDelete && status != tdInsert &&
       bort != vdest.bort ) {  // изменение борта
    if ( !bort.empty() && pr_reg && !events.isFlag( dmInitStages ) )
      events.setFlag( dmInitComps );
    if ( !bort.empty() && vdest.bort.empty() )
      events.setFlag( dmSetBort );
    else
      events.setFlag( dmChangeBort );
  }
  if ( status != tdDelete && status != tdInsert &&
       pr_del != vdest.pr_del ) {
    if ( pr_del == 1 )
      events.setFlag( dmSetCancel );
    else
      if ( pr_del == 0 && status != tdInsert )
        events.setFlag( dmSetUnCancel );

  }
  if ( status == tdDelete )
    events.setFlag( dmSetDelete );
  if ( status == tdInsert && pr_del == 1 )
    events.setFlag( dmSetCancel );
  //scd out
  if ( isSetSCDTime( status, scd_out, vdest.scd_out ) )
    events.setFlag( dmSetSCDOUT );
  if ( isChangeTime( status, scd_out, vdest.scd_out ) )
    events.setFlag( dmChangeSCDOUT );
  if ( isDeleteTime( status, scd_out, vdest.scd_out ) )
    events.setFlag( dmDeleteSCDOUT );
  //scd in
  if ( isSetSCDTime( status, scd_in, vdest.scd_in ) )
    events.setFlag( dmSetSCDIN );
  if ( isChangeTime( status, scd_in, vdest.scd_in ) )
    events.setFlag( dmChangeSCDIN );
  if ( isDeleteTime( status, scd_in, vdest.scd_in ) )
    events.setFlag( dmDeleteSCDIN );
  //est out
  if ( isSetOtherTime( status, est_out, vdest.est_out ) ) {
    events.setFlag( dmSetESTOUT );
  }
  if ( isChangeTime( status, est_out, vdest.est_out ) ) {
    events.setFlag( dmChangeESTOUT );
  }
  if ( isDeleteTime( status, est_out, vdest.est_out ) )
    events.setFlag( dmDeleteESTOUT );
  //est in
  if ( isSetOtherTime( status, est_in, vdest.est_in ) )
    events.setFlag( dmSetESTIN );
  if ( isChangeTime( status, est_in, vdest.est_in ) ) {
    events.setFlag( dmChangeESTIN );
  }
  if ( isDeleteTime( status, est_in, vdest.est_in ) )
    events.setFlag( dmDeleteESTIN );
  //act out
  if ( isSetOtherTime( status, act_out, vdest.act_out ) )
    events.setFlag( dmSetACTOUT );
  if ( isChangeTime( status, act_out, vdest.act_out ) )
    events.setFlag( dmChangeACTOUT );
  if ( isDeleteTime( status, act_out, vdest.act_out ) )
    events.setFlag( dmDeleteACTOUT );
  //act in
  if ( isSetOtherTime( status, act_in, vdest.act_in ) )
    events.setFlag( dmSetACTIN );
  if ( isChangeTime( status, act_in, vdest.act_in ) )
    events.setFlag( dmChangeACTIN );
  if ( isDeleteTime( status, act_in, vdest.act_in ) )
    events.setFlag( dmDeleteACTIN );
  if ( status != tdDelete && status != tdInsert ) {
    if ( trip_type != vdest.trip_type ) {
      events.setFlag( dmChangeTripType );
    }
    if ( litera != vdest.litera )
      events.setFlag( dmChangeLitera );
    if ( park_in != vdest.park_in ) {
      ProgTrace( TRACE5, "point_id=%d, park_in=%s, vdest.park_in=%s", point_id, park_in.c_str(), vdest.park_in.c_str() );
      events.setFlag( dmChangeParkIn );
    }
    if ( park_out != vdest.park_out )
      events.setFlag( dmChangeParkOut );
    if ( pr_tranzit != vdest.pr_tranzit )
      events.setFlag( dmTranzit );
    if ( pr_reg != vdest.pr_reg )
      events.setFlag( dmReg );
    if ( first_point != vdest.first_point )
      events.setFlag( dmFirst_Point );
  }
  if ( status != tdDelete && UseData.isFlag( udDelays ) ) {
    if ( !delays.equal( vdest.delays ) )
      events.setFlag( dmChangeDelays );
  }
  
	if ( !UseData.isFlag( udNoCalcESTTimeStage ) && status != tdDelete && pr_reg ) {
  	TDateTime t1 = NoExists, t2 = NoExists;
  	if ( status != tdInsert ) {
    	if ( vdest.est_out > NoExists )
      	t1 = vdest.est_out;
      else
     		t1 = vdest.scd_out;
    }
    if ( est_out > NoExists )
     	t2 = est_out;
    else
    	t2 = scd_out;
    if (  status == tdInsert && est_out > NoExists && scd_out > NoExists ) {
    	t1 = scd_out;
    	t2 = est_out;
    }
    if ( t1 > NoExists && t2 > NoExists && t1 != t2 ) {
      stage_scd = t1;
      stage_est = t2;
      events.setFlag( dmChangeStageESTTime );
    }
  }
  if ( status == tdInsert || point_num != vdest.point_num ) {
    events.setFlag( dmPoint_Num );
  }
}

void TPointsDest::setRemark( const TPointsDest &dest )
{
  remark = dest.remark;
  if ( events.isFlag( dmChangeCraft ) ) {
    if ( !remark.empty() )
      remark += " ";
    remark += "изм. типа ВС с " + dest.craft; //!!locale
    events.setFlag( dmChangeRemark );
  }
  if ( events.isFlag( dmChangeBort ) ) {
    if ( !remark.empty() )
      remark += " ";
    remark += "изм. борта с " + dest.bort; //!!locale
    events.setFlag( dmChangeRemark );
  }
  if ( events.isFlag( dmChangeRemark ) )
    remark = remark.substr( 0, 250 );
}

void TPointsDest::DoEvents( int move_id, const TPointsDest &dest )
{
  if ( events.emptyFlags() )  //нет изменений
    return;
  TReqInfo* reqInfo = TReqInfo::Instance();
  if ( status == tdInsert || status == tdDelete ||
       events.isFlag( dmSetCancel ) || events.isFlag( dmSetUnCancel ) ) {
    std::string lexema_id;
    if ( status == tdInsert )
      lexema_id = "EVT.INPUT_NEW_POINT";
    else if ( status == tdDelete )
      lexema_id = "EVT.DISP.DELETE_POINT";
    else if ( events.isFlag( dmSetCancel ) )
      lexema_id = "EVT.DISP.CANCEL_POINT";
    else if ( events.isFlag( dmSetUnCancel ) )
      lexema_id = "EVT.DISP.RETURN_POINT";
    if ( flt_no != NoExists )
      reqInfo->LocaleToLog(lexema_id, LEvntPrms() << PrmFlight("flt", airline, flt_no, suffix)
                           << PrmElem<std::string>("airp", etAirp, airp), evtDisp, move_id, point_id );
    else
      reqInfo->LocaleToLog(lexema_id, LEvntPrms() << PrmSmpl<std::string>("flt", "")
                           << PrmElem<std::string>("airp", etAirp, airp), evtDisp, move_id, point_id );
  }
  if ( events.isFlag( dmChangeAirline ) ||
       events.isFlag( dmChangeFltNo ) ||
       events.isFlag( dmChangeSuffix ) ) {
    if ( dest.flt_no != NoExists )
        reqInfo->LocaleToLog("EVT.FLIGHT.MODIFY_ATTRIBUTES_FROM", LEvntPrms() << PrmFlight("flt", dest.airline, dest.flt_no, dest.suffix)
                              << PrmFlight("new_flt", airline, flt_no, suffix) << PrmElem<std::string>("airp", etAirp, airp), evtDisp, move_id, point_id );
    else
        reqInfo->LocaleToLog("EVT.FLIGHT.MODIFY_ATTRIBUTES", LEvntPrms() << PrmFlight("flt", airline, flt_no, suffix)
                              << PrmElem<std::string>("airp", etAirp, airp), evtDisp, move_id, point_id );
  }
  if ( events.isFlag( dmSetSCDOUT ) )
    reqInfo->LocaleToLog("EVT.DISP.SET_TAKEOFF_PLAN", LEvntPrms() << PrmDate("time", scd_out, "hh:nn dd.mm.yy (UTC)")
                          << PrmElem<std::string>("airp", etAirp, airp), evtDisp, move_id, point_id );
  if ( events.isFlag( dmChangeSCDOUT ) )
    reqInfo->LocaleToLog("EVT.DISP.MODIFY_TAKEOFF_PLAN", LEvntPrms() << PrmDate("time", scd_out, "hh:nn dd.mm.yy (UTC)")
                          << PrmElem<std::string>("airp", etAirp, airp), evtDisp, move_id, point_id );
  if ( events.isFlag( dmDeleteSCDOUT ) )
    reqInfo->LocaleToLog("EVT.DISP.DELETE_TAKEOFF_PLAN", LEvntPrms() << PrmDate("time", dest.scd_out, "hh:nn dd.mm.yy (UTC)")
                          << PrmElem<std::string>("airp", etAirp, airp), evtDisp, move_id, point_id );
  if ( events.isFlag( dmSetESTOUT ) )
    reqInfo->LocaleToLog("EVT.DISP.SET_TAKEOFF_EST", LEvntPrms() << PrmDate("time", est_out, "hh:nn dd.mm.yy (UTC)")
                          << PrmElem<std::string>("airp", etAirp, airp), evtDisp, move_id, point_id );
  if ( events.isFlag( dmChangeESTOUT ) )
    reqInfo->LocaleToLog("EVT.DISP.MODIFY_TAKEOFF_EST", LEvntPrms() << PrmDate("time", est_out, "hh:nn dd.mm.yy (UTC)")
                          << PrmElem<std::string>("airp", etAirp, airp), evtDisp, move_id, point_id );
  if ( events.isFlag( dmDeleteESTOUT ) )
    reqInfo->LocaleToLog("EVT.DISP.DELETE_TAKEOFF_EST", LEvntPrms() << PrmDate("time", dest.est_out, "hh:nn dd.mm.yy (UTC)")
                          << PrmElem<std::string>("airp", etAirp, airp), evtDisp, move_id, point_id );
  if ( events.isFlag( dmSetACTOUT ) )
    reqInfo->LocaleToLog("EVT.DISP.SET_TAKEOFF_ACT", LEvntPrms() << PrmDate("time", act_out, "hh:nn dd.mm.yy (UTC)")
                          << PrmElem<std::string>("airp", etAirp, airp), evtDisp, move_id, point_id );
  if ( events.isFlag( dmChangeACTOUT ) )
    reqInfo->LocaleToLog("EVT.DISP.MODIFY_TAKEOFF_ACT", LEvntPrms() << PrmDate("time", act_out, "hh:nn dd.mm.yy (UTC)")
                          << PrmElem<std::string>("airp", etAirp, airp), evtDisp, move_id, point_id );
  if ( events.isFlag( dmDeleteACTOUT ) )
    reqInfo->LocaleToLog("EVT.DISP.DELETE_TAKEOFF_ACT", LEvntPrms() << PrmDate("time", dest.act_out, "hh:nn dd.mm.yy (UTC)")
                          << PrmElem<std::string>("airp", etAirp, airp), evtDisp, move_id, point_id );
  if ( events.isFlag( dmSetSCDIN ) )
    reqInfo->LocaleToLog("EVT.DISP.SET_LANDING_PLAN", LEvntPrms() << PrmDate("time", scd_in, "hh:nn dd.mm.yy (UTC)")
                          << PrmElem<std::string>("airp", etAirp, airp), evtDisp, move_id, point_id );
  if ( events.isFlag( dmChangeSCDIN ) )
    reqInfo->LocaleToLog("EVT.DISP.MODIFY_LANDING_PLAN", LEvntPrms() << PrmDate("time", scd_in, "hh:nn dd.mm.yy (UTC)")
                          << PrmElem<std::string>("airp", etAirp, airp), evtDisp, move_id, point_id );
  if ( events.isFlag( dmDeleteSCDIN ) )
    reqInfo->LocaleToLog("EVT.DISP.DELETE_LANDING_PLAN", LEvntPrms() << PrmDate("time", dest.scd_in, "hh:nn dd.mm.yy (UTC)")
                          << PrmElem<std::string>("airp", etAirp, airp), evtDisp, move_id, point_id );
  if ( events.isFlag( dmSetESTIN ) )
    reqInfo->LocaleToLog("EVT.DISP.SET_LANDING_EST", LEvntPrms() << PrmDate("time", est_in, "hh:nn dd.mm.yy (UTC)")
                          << PrmElem<std::string>("airp", etAirp, airp), evtDisp, move_id, point_id );
  if ( events.isFlag( dmChangeESTIN ) )
    reqInfo->LocaleToLog("EVT.DISP.MODIFY_LANDING_EST", LEvntPrms() << PrmDate("time", est_in, "hh:nn dd.mm.yy (UTC)")
                          << PrmElem<std::string>("airp", etAirp, airp), evtDisp, move_id, point_id );
  if ( events.isFlag( dmDeleteESTIN ) )
    reqInfo->LocaleToLog("EVT.DISP.DELETE_LANDING_EST", LEvntPrms() << PrmDate("time", dest.est_in, "hh:nn dd.mm.yy (UTC)")
                          << PrmElem<std::string>("airp", etAirp, airp), evtDisp, move_id, point_id );
  if ( events.isFlag( dmSetACTIN ) )
    reqInfo->LocaleToLog("EVT.DISP.SET_LANDING_ACT", LEvntPrms() << PrmDate("time", act_in, "hh:nn dd.mm.yy (UTC)")
                          << PrmElem<std::string>("airp", etAirp, airp), evtDisp, move_id, point_id );
  if ( events.isFlag( dmChangeACTIN ) )
    reqInfo->LocaleToLog("EVT.DISP.MODIFY_LANDING_ACT", LEvntPrms() << PrmDate("time", act_in, "hh:nn dd.mm.yy (UTC)")
                          << PrmElem<std::string>("airp", etAirp, airp), evtDisp, move_id, point_id );
  if ( events.isFlag( dmDeleteACTIN ) )
    reqInfo->LocaleToLog("EVT.DISP.DELETE_LANDING_ACT", LEvntPrms() << PrmDate("time", dest.act_in, "hh:nn dd.mm.yy (UTC)")
                          << PrmElem<std::string>("airp", etAirp, airp), evtDisp, move_id, point_id );

  if ( events.isFlag( dmChangeStageESTTime ) ) {
    TDateTime diff = stage_est - stage_scd;
  	double f;
    if ( diff < 0 ) {
  	  modf( diff, &f );
  		if ( f )
        reqInfo->LocaleToLog("EVT.TECHNOLOGY_SCHEDULE_AHEAD", LEvntPrms() << PrmSmpl<int>("val", (int)f)
                             << PrmDate("time", fabs(diff), "hh:nn") << PrmElem<std::string>("airp", etAirp, airp), evtFlt, point_id);
  	}
  	if ( diff >= 0 ) {
  	  modf( diff, &f );
  	  if ( diff ) {
  		  if ( f )
          reqInfo->LocaleToLog("EVT.TECHNOLOGY_SCHEDULE_DELAY", LEvntPrms() << PrmSmpl<int>("val", (int)f)
                               << PrmDate("time", diff, "hh:nn") << PrmElem<std::string>("airp", etAirp, airp), evtFlt, point_id);
  		}
  		else
          reqInfo->LocaleToLog("EVT.TECHNOLOGY_SCHEDULE_DELAY_CANCEL", LEvntPrms() <<
                               PrmElem<std::string>("airp", etAirp, airp), evtFlt, point_id);
    }
  }

  if ( events.isFlag( dmChangeTripType ) )
    reqInfo->LocaleToLog("EVT.MODIFY_FLIGHT_TYPE", LEvntPrms() << PrmElem<std::string>("type", etTripType, dest.trip_type, efmtNameLong)
                         << PrmElem<std::string>("new_type", etTripType, trip_type, efmtNameLong)
                         << PrmElem<std::string>("airp", etAirp, airp), evtDisp, move_id, point_id );
  if ( events.isFlag( dmChangeLitera ) )
    reqInfo->LocaleToLog("EVT.MODIFY_FLIGHT_LITERA", LEvntPrms() << PrmElem<std::string>("litera", etTripLiter, dest.litera)
                         << PrmElem<std::string>("new_litera", etTripLiter, litera)
                         << PrmElem<std::string>("airp", etAirp, airp), evtDisp, move_id, point_id );
  if ( events.isFlag( dmReg ) )
    reqInfo->LocaleToLog("EVT.MODIFY_REG_FLAG", LEvntPrms() << PrmBool("pr_reg", dest.pr_reg) << PrmBool("new_pr_reg", pr_reg)
                         << PrmElem<std::string>("airp", etAirp, airp), evtDisp, move_id, point_id );
  if ( events.isFlag( dmChangeCraft ) )
    reqInfo->LocaleToLog("EVT.MODIFY_CRAFT_TYPE", LEvntPrms() << PrmElem<std::string>("craft", etCraft, craft)
                         << PrmElem<std::string>("airp", etAirp, airp), evtDisp, move_id, point_id );
  if ( events.isFlag( dmSetCraft ) )
    reqInfo->LocaleToLog("EVT.ASSIGNE_CRAFT_TYPE", LEvntPrms() << PrmElem<std::string>("craft", etCraft, craft)
                         << PrmElem<std::string>("airp", etAirp, airp), evtDisp, move_id, point_id );
  if ( events.isFlag( dmChangeBort ) )
    reqInfo->LocaleToLog("EVT.MODIFY_BOARD_TYPE", LEvntPrms() << PrmSmpl<std::string>("bort", bort)
                         << PrmElem<std::string>("airp", etAirp, airp), evtDisp, move_id, point_id );
  if ( events.isFlag( dmSetBort ) )
    reqInfo->LocaleToLog("EVT.ASSIGNE_BOARD_TYPE", LEvntPrms() << PrmSmpl<std::string>("bort", bort)
                         << PrmElem<std::string>("airp", etAirp, airp), evtDisp, move_id, point_id );
  if ( status != tdInsert &&
       ( events.isFlag( dmSetACTOUT ) ||
         events.isFlag( dmDeleteACTOUT ) ||
         events.isFlag( dmSetCancel ) ||
         events.isFlag( dmSetUnCancel ) ||
         events.isFlag( dmSetDelete ) ) ) {
    SetTripStages_IgnoreAuto( point_id, act_out != NoExists || pr_del != 0 );
  }
  on_change_trip( CALL_POINT, point_id );
  if ( events.isFlag( dmChangeDelays ) ) {
    try {
          vector<TypeB::TCreateInfo> createInfo;
          TypeB::TMVTCCreator(point_id).getInfo(createInfo);
          TelegramInterface::SendTlg(createInfo);
    }
    catch(std::exception &E) {
        ProgError(STDLOG,"TPointsDest::DoEvents: SendTlg (point_id=%d): %s",point_id,E.what());
    };
  }
}

////////////////////////////////////////////////////

int TPoints::getPoint_id()
{
  TQuery Qry(&OraSession);
  Qry.SQLText =
    "SELECT point_id.nextval point_id FROM dual";
  Qry.Execute();
  return Qry.FieldAsInteger( "point_id" );
}

void TPoints::WriteDest( TPointsDest &dest )
{
  if ( dest.events.emptyFlags() &&
       ( dest.status != tdInsert && dest.status != tdDelete &&
         !dest.events.isFlag( dmSetCancel ) && !dest.events.isFlag( dmSetUnCancel ) ) ) { //нет изменений
    ProgTrace( TRACE5, "WriteDest: no change, point_id=%d", dest.point_id );
    return;
  }
  ProgTrace( TRACE5, "WriteDest" );
  TQuery Qry(&OraSession);
  Qry.SQLText =
    "SELECT cycle_tid__seq.nextval n FROM dual ";
 	Qry.Execute();
 	dest.tid = Qry.FieldAsInteger( "n" );
 	if ( dest.status == tdInsert ) {
    ProgTrace( TRACE5, "insert" );
    Qry.Clear();
    Qry.SQLText =
      "BEGIN "
      " UPDATE points SET point_num=point_num+1 WHERE point_num>=:point_num AND move_id=:move_id; "
      " INSERT INTO points(move_id,point_id,point_num,airp,airp_fmt,pr_tranzit,first_point,"
      "                    airline,airline_fmt,flt_no,suffix,suffix_fmt,craft,craft_fmt,"
      "                    bort,scd_in,est_in,act_in,scd_out,est_out,act_out,trip_type,litera,"
      "                    park_in,park_out,pr_del,tid,remark,pr_reg) "
      " VALUES(:move_id,:point_id,:point_num,:airp,:airp_fmt,:pr_tranzit,:first_point,"
      "        :airline,:airline_fmt,:flt_no,:suffix,:suffix_fmt,:craft,:craft_fmt,"
      "        :bort,:scd_in,:est_in,:act_in,:scd_out,:est_out,:act_out,:trip_type,:litera,"
      "        :park_in,:park_out,:pr_del,:tid,:remark,:pr_reg); "
      "END;";
  }
  else {
    ProgTrace( TRACE5, "update, point_id=%d, est_out=%s", dest.point_id, DateTimeToStr( dest.est_out ).c_str() );
  	Qry.Clear();
  	Qry.SQLText =
      "BEGIN "
      " IF :point_num<0 THEN "
      "   UPDATE points SET point_num=point_num-1 WHERE point_num<=:point_num AND move_id=:move_id AND pr_del=-1; "
      " END IF;"
      " UPDATE points "
      "  SET point_num=:point_num,airp=:airp,airp_fmt=:airp_fmt,pr_tranzit=:pr_tranzit,"\
      "      first_point=:first_point,airline=:airline,airline_fmt=:airline_fmt,flt_no=:flt_no,"
      "      suffix=:suffix,suffix_fmt=:suffix_fmt,craft=:craft,craft_fmt=:craft_fmt,"\
      "      bort=:bort,scd_in=:scd_in,est_in=:est_in,act_in=:act_in,"\
      "      scd_out=:scd_out,est_out=:est_out,act_out=:act_out,trip_type=:trip_type,"\
      "      litera=:litera,park_in=:park_in,park_out=:park_out,pr_del=:pr_del,tid=:tid,"\
      "      remark=:remark,pr_reg=:pr_reg "
      "  WHERE point_id=:point_id AND move_id=:move_id; "
      "END; ";
  }
  Qry.CreateVariable( "move_id", otInteger, move_id );
  Qry.CreateVariable( "point_id", otInteger, dest.point_id );
 	Qry.CreateVariable( "point_num", otInteger, dest.point_num );
//  ProgTrace( TRACE5, "move_id=%d, desk.airp=%s, point_id=%d, point_num=%d",
//             move_id, dest.airp.c_str(), dest.point_id, dest.point_num );
  Qry.CreateVariable( "airp", otString, dest.airp );
  Qry.CreateVariable( "airp_fmt", otInteger, (int)dest.airp_fmt );
  Qry.CreateVariable( "pr_tranzit", otInteger, dest.pr_tranzit );
  if ( dest.first_point == NoExists )
  	Qry.CreateVariable( "first_point", otInteger, FNull );
  else
    Qry.CreateVariable( "first_point", otInteger, dest.first_point );
  if ( dest.airline.empty() ) {
  	Qry.CreateVariable( "airline", otString, FNull );
  	Qry.CreateVariable( "airline_fmt", otInteger, FNull );
  }
  else {
    Qry.CreateVariable( "airline", otString, dest.airline );
    Qry.CreateVariable( "airline_fmt", otInteger, (int)dest.airline_fmt );
  }
  if ( dest.flt_no == NoExists )
  	Qry.CreateVariable( "flt_no", otInteger, FNull );
  else
  	Qry.CreateVariable( "flt_no", otInteger, dest.flt_no );
  if ( dest.suffix.empty() ) {
  	Qry.CreateVariable( "suffix", otString, FNull );
  	Qry.CreateVariable( "suffix_fmt", otInteger, FNull );
  }
  else {
  	Qry.CreateVariable( "suffix", otString, dest.suffix );
  	Qry.CreateVariable( "suffix_fmt", otInteger, (int)dest.suffix_fmt );
  }
  if ( dest.craft.empty() ) {
  	Qry.CreateVariable( "craft", otString, FNull );
  	Qry.CreateVariable( "craft_fmt", otInteger, FNull );
  }
  else {
  	Qry.CreateVariable( "craft", otString, dest.craft );
  	Qry.CreateVariable( "craft_fmt", otInteger, (int)dest.craft_fmt );
  }
  Qry.CreateVariable( "bort", otString, dest.bort );
  ProgTrace( TRACE5, "dest.craft=%s, dest.bort=%s", dest.craft.c_str(), dest.bort.c_str() );
  if ( dest.scd_in == NoExists )
  	Qry.CreateVariable( "scd_in", otDate, FNull );
  else
  	Qry.CreateVariable( "scd_in", otDate, dest.scd_in );
  if ( dest.est_in == NoExists )
  	Qry.CreateVariable( "est_in", otDate, FNull );
  else
  	Qry.CreateVariable( "est_in", otDate, dest.est_in );
  if ( dest.act_in == NoExists )
  	Qry.CreateVariable( "act_in", otDate, FNull );
  else
  	Qry.CreateVariable( "act_in", otDate, dest.act_in );
  if ( dest.scd_out == NoExists )
  	Qry.CreateVariable( "scd_out", otDate, FNull );
  else
  	Qry.CreateVariable( "scd_out", otDate, dest.scd_out );
  if ( dest.est_out == NoExists )
  	Qry.CreateVariable( "est_out", otDate, FNull );
  else {
  	Qry.CreateVariable( "est_out", otDate, dest.est_out );
//    ProgTrace( TRACE5, "dest.est_out=%f", dest.est_out );
  }
  if ( dest.act_out == NoExists )
  	Qry.CreateVariable( "act_out", otDate, FNull );
  else
  	Qry.CreateVariable( "act_out", otDate, dest.act_out );
  if ( dest.trip_type.empty() )
  	Qry.CreateVariable( "trip_type", otString, FNull );
  else
  	Qry.CreateVariable( "trip_type", otString, dest.trip_type );
  if ( dest.litera.empty() )
  	Qry.CreateVariable( "litera", otString, FNull );
  else
  	Qry.CreateVariable( "litera", otString, dest.litera );
  if ( dest.park_in.empty() )
  	Qry.CreateVariable( "park_in", otString, FNull );
  else
  	Qry.CreateVariable( "park_in", otString, dest.park_in );
  if ( dest.park_out.empty() )
  	Qry.CreateVariable( "park_out", otString, FNull );
  else
  	Qry.CreateVariable( "park_out", otString, dest.park_out );
  Qry.CreateVariable( "pr_del", otInteger, dest.pr_del );
  Qry.CreateVariable( "tid", otInteger, dest.tid );
  Qry.CreateVariable( "remark", otString, dest.remark );
  Qry.CreateVariable( "pr_reg", otInteger, dest.pr_reg );
  Qry.Execute();
  if ( dest.events.isFlag( dmChangeDelays ) ) {
     dest.delays.Save( dest.point_id );
  }
  if ( dest.events.isFlag( dmInitStages ) ) {
    ProgTrace( TRACE5, "dmInitStages, point_id=%d", dest.point_id );
    TReqInfo::Instance()->LocaleToLog("EVT.TECHNOLOGY_STEPS_ASSIGNEMENT", LEvntPrms()
                                      << PrmFlight("flt", dest.airline, dest.flt_no, dest.suffix)
                                      << PrmElem<std::string>("airp", etAirp, dest.airp),
                                      evtDisp, move_id, dest.point_id );
    set_flight_sets(dest.point_id);
/* 		TMapTripStages NewStages;
 		TTripStages::LoadStages( dest.point_id, NewStages );
 		for (TMapTripStages::iterator istage=NewStages.begin(); istage!=NewStages.end(); istage++ ) {
 		  TMapTripStages::iterator jstage = dest.stages.find( istage->first );
      if ( jstage != dest.stages.end() ) {
        istage->second.scd = jstage->second.scd;
        istage->second.est = jstage->second.est;
        istage->second.act = jstage->second.act;
      }
 		}
 		dest.stages = NewStages;*/
  }
  if ( dest.events.isFlag( dmChangeStages ) ) {
    dest.stages.Save( dest.point_id );
  }
  tst();
	if ( dest.events.isFlag( dmChangeStageESTTime ) ) {
   	ProgTrace( TRACE5, "trip_stages delay=%s", DateTimeToStr(dest.stage_est-dest.stage_scd).c_str() );
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
    Qry.CreateVariable( "point_id", otInteger, dest.point_id );
  	Qry.CreateVariable( "vscd", otDate, dest.stage_scd );
  	Qry.CreateVariable( "vest", otDate, dest.stage_est );
  	Qry.Execute();
  	if ( Qry.VariableIsNULL( "vscd" ) ) {
  	  dest.events.clearFlag( dmChangeStageESTTime );
  	  dest.stage_scd = ASTRA::NoExists;
  	  dest.stage_est = ASTRA::NoExists;
  	}
  	else {
  	  dest.stage_scd = Qry.GetVariableAsDateTime( "vscd" );
      dest.stage_est = Qry.GetVariableAsDateTime( "vest" );
  	}
  }
  tst();
}

template <class A, class B>
void getKeyTrips( const std::vector<A> &dests, std::vector<B> &trips )
{
  trips.clear();
  for ( typename std::vector<A>::const_iterator i=dests.begin(); i!=dests.end(); i++ ) {
    if ( i->pr_del == -1 )
      continue;
    B trip( *i );
    int first_point;
    if ( i->pr_tranzit )
      first_point = i->first_point;
    else
      first_point = i->point_id;
    for ( typename std::vector<A>::const_iterator j=dests.begin(); j!=dests.end(); j++ ) {
      if ( j->pr_del == -1 )
        continue;
      if ( (j->point_num <= i->point_num &&
            ( i->first_point == j->first_point || i->first_point == j->point_id )) ||
           (j->point_num > i->point_num && j->first_point == first_point) ) {
        if ( i->pr_del == 1 || i->pr_del == j->pr_del ) {
          trip.push_back( *j );
          ProgTrace( TRACE5, "getKeyTrips: push dest point_id=%d", j->point_id );
        }
      }
    }
    ProgTrace( TRACE5, "getKeyTrips: push trip.point_id=%d, dest.size()=%zu", trip.key.point_id, trip.dests.size() );
    trips.push_back( trip );
  }
}

typedef vector<TPointsDest>::iterator TPointsDestIter;

inline void getPriorNextDest( vector<TPointsDest> &dests,
                              int point_id,
                              TPointsDestIter &prior,
                              TPointsDestIter &next )
{
  prior = dests.end();
  next = dests.end();
  bool pr_find_own_dest=false;
  //находим пред. и след. пункт посадки
  for ( TPointsDestIter i=dests.begin(); i!=dests.end(); i++ ) {
    if ( i->point_id == point_id )
      pr_find_own_dest = true;
    if ( !pr_find_own_dest )
      prior = i;
    else
      if ( i->point_id != point_id )
        next = i;
  }
}

template <typename T>
void PointsKeyTrip<T>::getEvents( KeyTrip<T> &trip )
{
  ProgTrace( TRACE5, "PointsKeyTrip:getEvents, this->point_id=%d, trip.point_id=%d",
             this->key.point_id, trip.key.point_id );
  this->events.clearFlags();

  TPointsDestIter prior, oldprior, next, oldnext;
  getPriorNextDest( this->dests, this->key.point_id, prior, next );
  getPriorNextDest( trip.dests, trip.key.point_id, oldprior, oldnext );

  if ( this->key.status != tdUpdate ||
       this->key.events.isFlag( dmPoint_Num ) ||
       this->key.events.isFlag( dmChangeAirline ) ||
       this->key.events.isFlag( dmChangeFltNo ) ||
       this->key.events.isFlag( dmChangeSuffix ) ||
       this->key.events.isFlag( dmChangeAirp ) ||
       this->key.events.isFlag( dmSetCancel ) ||
       this->key.events.isFlag( dmSetUnCancel ) ||
       this->key.events.isFlag( dmSetDelete ) ||
       this->key.events.isFlag( dmSetSCDOUT ) ||
       this->key.events.isFlag( dmChangeSCDOUT ) ||
       this->key.events.isFlag( dmDeleteSCDOUT ) ) {
      this->events.setFlag( teNeedUnBindTlgs );
      if ( next != this->dests.end() &&
           this->key.scd_out != NoExists &&
           !this->key.airline.empty() &&
           this->key.flt_no != NoExists ) { // есть вылет
        this->events.setFlag( teNeedBindTlgs );
      }
  }

  if ( trip.key.point_id == NoExists && this->key.status != tdDelete ) { //новый рейс
    // рейс новый
    if ( prior != this->dests.end() ) // новый на прилет
      this->events.setFlag( teNewLand );
    if ( next != this->dests.end() ) // новый на вылет
      this->events.setFlag( teNewTakeoff );
  }
  if ( this->key.status == trip.key.status && trip.key.status == tdDelete ) {
    // рейс удален
    if ( prior != this->dests.end() ) // удален на прилет
      this->events.setFlag( teDeleteLand );
    if ( next != this->dests.end() ) // удален на вылет
      this->events.setFlag( teDeleteTakeoff );
    return;
  }
  if ( this->key.point_id == trip.key.point_id ) { // рейс был
    if ( prior == this->dests.end() && oldprior != trip.dests.end() )
      this->events.setFlag( teDeleteLand );
    if ( prior != this->dests.end() && oldprior == trip.dests.end() )
      this->events.setFlag( teNewLand );
    if ( next == this->dests.end() && oldnext != trip.dests.end() )
      this->events.setFlag( teDeleteTakeoff );
    if ( next != this->dests.end() && oldnext == trip.dests.end() )
      this->events.setFlag( teNewTakeoff );
      
    if ( this->key.events.isFlag( dmChangeParkIn ) )
       ProgTrace( TRACE5, "point_id=%d, prior != this->dests.items.end()=%d", this->key.point_id, prior != this->dests.end() );

      
    if ( prior != this->dests.end() ) { // есть прилет
      if ( this->key.events.isFlag( dmSetSCDIN ) )
        this->events.setFlag( teSetSCDIN );
      if ( this->key.events.isFlag( dmChangeSCDIN ) )
        this->events.setFlag( teChangeSCDIN );
      if ( this->key.events.isFlag( dmDeleteSCDIN ) )
        this->events.setFlag( teDeleteSCDIN );
      if ( this->key.events.isFlag( dmSetESTIN ) )
        this->events.setFlag( teSetESTIN );
      if ( this->key.events.isFlag( dmChangeESTIN ) )
        this->events.setFlag( teChangeESTIN );
      if ( this->key.events.isFlag( dmDeleteESTIN ) )
        this->events.setFlag( teDeleteESTIN );
      if ( this->key.events.isFlag( dmSetACTIN ) )
        this->events.setFlag( teSetACTIN );
      if ( this->key.events.isFlag( dmChangeACTIN ) )
        this->events.setFlag( teChangeACTIN );
      if ( this->key.events.isFlag( dmDeleteACTIN ) )
        this->events.setFlag( teDeleteACTIN );
      if ( this->key.events.isFlag( dmChangeParkIn ) ) {
        tst();
        this->events.setFlag( teChangeParkIn );
      }
      if ( prior->events.isFlag( dmChangeCraft ) )
        this->events.setFlag( teChangeCraftLand );
      if ( prior->events.isFlag( dmSetCraft ) )
        this->events.setFlag( teSetCraftLand );
      if ( prior->events.isFlag( dmChangeBort ) )
        this->events.setFlag( teChangeBortLand );
      if ( prior->events.isFlag( dmSetBort ) )
        this->events.setFlag( teSetBortLand );
      if ( prior->events.isFlag( dmChangeTripType ) )
        this->events.setFlag( teChangeTripTypeLand );
      if ( prior->events.isFlag( dmChangeLitera ) )
        this->events.setFlag( teChangeLiteraLand );
      if ( prior->events.isFlag( dmChangeAirline ) ||
           prior->events.isFlag( dmChangeFltNo ) ||
           prior->events.isFlag( dmChangeSuffix ) )
        events.setFlag( teChangeFlightAttrLand );
    }
    if ( next != this->dests.end() ) { // есть вылет
      if ( this->key.events.isFlag( dmChangeCraft ) )
        this->events.setFlag( teChangeCraftTakeoff );
      if ( this->key.events.isFlag( dmSetCraft ) )
        this->events.setFlag( teSetCraftTakeoff );
      if ( this->key.events.isFlag( dmChangeBort ) )
        this->events.setFlag( teChangeBortTakeoff );
      if ( this->key.events.isFlag( dmSetBort ) )
        this->events.setFlag( teSetBortTakeoff );
      if ( this->key.events.isFlag( dmChangeTripType ) )
        this->events.setFlag( teChangeTripTypeTakeoff );
      if ( this->key.events.isFlag( dmChangeLitera ) )
        this->events.setFlag( teChangeLiteraTakeoff );
      if ( this->key.events.isFlag( dmChangeParkOut ) )
        this->events.setFlag( teChangeParkOut );
      if ( this->key.events.isFlag( dmChangeAirline ) ||
           this->key.events.isFlag( dmChangeFltNo ) ||
           this->key.events.isFlag( dmChangeSuffix ) )
        this->events.setFlag( teChangeFlightAttrTakeoff );
      if ( this->key.events.isFlag( dmChangeDelays ) )
        this->events.setFlag( teChangeDelaysTakeoff );
      if ( this->key.events.isFlag( dmTranzit ) )
        this->events.setFlag( teTranzitTakeoff );
      if ( this->key.events.isFlag( dmReg ) )
        this->events.setFlag( teRegTakeoff );
      if ( this->key.events.isFlag( dmFirst_Point ) )
        this->events.setFlag( teFirst_PointTakeoff );
      if ( this->key.events.isFlag( dmChangeRemark ) )
        this->events.setFlag( teChangeRemarkTakeoff );
      if ( this->key.events.isFlag( dmPoint_Num ) )
        this->events.setFlag( tePoint_NumTakeoff );
      if ( this->key.events.isFlag( dmSetSCDOUT ) )
        this->events.setFlag( teSetSCDOUT );
      if ( this->key.events.isFlag( dmChangeSCDOUT ) )
        this->events.setFlag( teChangeSCDOUT );
      if ( this->key.events.isFlag( dmDeleteSCDOUT ) )
        this->events.setFlag( teDeleteSCDOUT );
      if ( this->key.events.isFlag( dmSetESTOUT ) )
        this->events.setFlag( teSetESTOUT );
      if ( this->key.events.isFlag( dmChangeESTOUT ) )
        this->events.setFlag( teChangeESTOUT );
      if ( this->key.events.isFlag( dmDeleteESTOUT ) )
        this->events.setFlag( teDeleteESTOUT );
      if ( this->key.events.isFlag( dmSetACTOUT ) )
        this->events.setFlag( teSetACTOUT );
      if ( this->key.events.isFlag( dmChangeACTOUT ) )
        this->events.setFlag( teChangeACTOUT );
      if ( this->key.events.isFlag( dmDeleteACTOUT ) )
        this->events.setFlag( teDeleteACTOUT );
    }
  }
  if ( this->key.events.isFlag( dmInitStages ) )
    this->events.setFlag( teInitStages );
  if ( this->key.events.isFlag( dmInitComps ) )
    this->events.setFlag( teInitComps );
  if ( this->key.events.isFlag( dmChangeStageESTTime ) )
    this->events.setFlag( teChangeStageESTTime );
  if ( prior != this->dests.end() && prior->events.isFlag( dmSetCancel ) )
 	  this->events.setFlag( teSetCancelLand ); // отмена прилета
/*  if ( prior != this->dests.items.end() && prior->events.isFlag( dmSetUnCancel ) )
 	  this->events.setFlag( teSetUnCancelLand ); // возврат прилета*/
  if ( next != this->dests.end() && this->key.events.isFlag( dmSetCancel ) )
 	  this->events.setFlag( teSetCancelTakeoff ); // отмена вылета
  if ( next != this->dests.end() && this->key.events.isFlag( dmSetUnCancel ) )
 	  this->events.setFlag( teSetUnCancelTakeoff ); // возврат вылета
  if ( next != this->dests.end() && next->events.isFlag( dmSetCancel ) )
    this->events.setFlag( teSetCancelTakeoff );
/*  if ( next != this->dests.items.end() && next->events.isFlag( dmSetUnCancel ) )
    this->events.setFlag( teSetUnCancelTakeoff );*/
    
  ProgTrace( TRACE5, "this->key.status == tdInsert=%d, this->key.UseData.isFlag( udCargo )=%d",
             this->key.status == tdInsert, this->key.UseData.isFlag( udCargo ) );
  if ( next != this->dests.end() && this->key.status != tdDelete && this->key.pr_reg && /*&& this->key.pr_del == 0*/
       this->key.UseData.isFlag( udCargo ) ) {
    if ( this->key.status == tdInsert ||
         !this->key.cargos.equal( trip.key.cargos ) ) {
      ProgTrace( TRACE5, "getEvents: teChangeCargos" );
      this->events.setFlag( teChangeCargos );
    }
  }
  if ( next != this->dests.end() && this->key.status != tdDelete && this->key.pr_reg &&/*&& this->key.pr_del == 0*/
       this->key.UseData.isFlag( udMaxCommerce ) ) {
    if ( this->key.status == tdInsert ||
         !this->key.max_commerce.equal( trip.key.max_commerce ) ) {
      ProgTrace( TRACE5, "getEvents: teChangeMaxCommerce" );
      this->events.setFlag( teChangeMaxCommerce );
    }
  }
  if ( next != this->dests.end() && this->key.status != tdDelete && this->key.pr_reg &&
       this->key.events.isFlag( dmChangeStages ) ) {
    tst();
    this->events.setFlag( teChangeStages );
  }
  if ( next != this->dests.end() && this->key.status != tdDelete && this->key.pr_reg &&
       this->key.UseData.isFlag( udStations ) ) {
    tst();
    if ( this->key.status == tdInsert ||
         !this->key.stations.equal( trip.key.stations ) ) {
      ProgTrace( TRACE5, "getEvents: teChangeStations" );
      this->events.setFlag( teChangeStations );
    }
  }

  if ( this->key.status == tdInsert ||
       this->key.status == tdDelete ||
       this->events.isFlag( teNewLand ) ||
       this->events.isFlag( teNewTakeoff ) ||
       this->events.isFlag( teDeleteLand ) ||
       this->events.isFlag( teDeleteTakeoff ) ||
       this->events.isFlag( teSetCancelLand ) ||
       this->events.isFlag( teSetCancelTakeoff ) ||
       this->events.isFlag( teSetUnCancelLand ) ||
       this->events.isFlag( teSetUnCancelTakeoff ) ||
       this->events.isFlag( teRegTakeoff ) ||
       this->events.isFlag( teTranzitTakeoff ) ||
       this->events.isFlag( teFirst_PointTakeoff ) ||
       this->events.isFlag( teSetSCDOUT  ) ||
       this->events.isFlag( teChangeSCDOUT  ) ||
       this->events.isFlag( teDeleteSCDOUT  ) ||
       this->events.isFlag( teSetACTOUT  ) ||
       this->events.isFlag( teChangeACTOUT  ) ||
       this->events.isFlag( teDeleteACTOUT  ) ||
       this->events.isFlag( teChangeFlightAttrTakeoff  ) ||
       this->events.isFlag( teSetESTOUT  ) ||
       this->events.isFlag( teChangeESTOUT  ) ||
       this->events.isFlag( teDeleteESTOUT  ) ||
       this->events.isFlag( teChangeFlightAttrLand  ) ||
       this->events.isFlag( teChangeStageESTTime  ) ) {
    for ( vector<TPointsDest>::iterator id=this->dests.begin(); id!=this->dests.end(); id++ ) {
      if ( CheckApis_USA( id->airp ) ) {
        this->events.setFlag( teNeedApisUSA );
        break;
      }
    }
  }
}

string DecodeEvents( TTripEvents event )
{
  string res;
  switch ( (int)event ) {
    case teNewLand:
      res = "EVT.NEW_LAND";
      break;
    case teNewTakeoff:
      res = "EVT.NEW_TAKEOFF";
      break;
    case teDeleteLand:
      res = "EVT.DELETE_LAND";
      break;
    case teDeleteTakeoff:
      res = "EVT.DELETE_TAKEOFF";
      break;
    case teSetCancelLand:
      res = "EVT.CANCEL_LAND";
      break;
    case teSetCancelTakeoff:
      res = "EVT.CANCEL_TAKEOFF";
      break;
    case teSetUnCancelLand:
      res = "EVT.SET_UNCANCEL_LAND";
      break;
    case teSetUnCancelTakeoff:
      res = "EVT.SET_UNCANCEL_TAKEOFF";
      break;
    case teSetSCDIN:
      res = "EVT.SET_SCD_IN";
      break;
    case teChangeSCDIN:
      res = "EVT.CHANGE_SCD_IN";
      break;
    case teDeleteSCDIN:
      res = "EVT.DELETE_SCD_IN";
      break;
    case teSetESTIN:
      res = "EVT.SET_EST_IN";
      break;
    case teChangeESTIN:
      res = "EVT.CHANGE_EST_IN";
      break;
    case teDeleteESTIN:
      res = "EVT.DELETE_EST_IN";
      break;
    case teSetACTIN:
      res = "EVT.SET_ACT_IN";
      break;
    case teChangeACTIN:
      res = "EVT.CHANGE_ACT_IN";
      break;
    case teDeleteACTIN:
      res = "EVT.DELETE_ACT_IN";
      break;
    case teChangeParkIn:
      res = "EVT.CHANGE_PARK_IN";
      break;
    case teChangeCraftLand:
      res = "EVT.CHANGE_CRAFT_LAND";
      break;
    case teSetCraftLand:
      res = "EVT.SET_CRAFT_LAND";
      break;
    case teChangeBortLand:
      res = "EVT.CHANGE_BORT_LAND";
      break;
    case teSetBortLand:
      res = "EVT.SET_BORT_LAND";
      break;
    case teChangeTripTypeLand:
      res = "EVT.CHANGE_TRIP_TYPE_LAND";
      break;
    case teChangeLiteraLand:
      res = "EVT.CHANGE_LITERA_LAND";
      break;
    case teChangeFlightAttrLand:
      res = "EVT.CHANGE_FLIGHT_ATTR_LAND";
      break;
    case teInitStages:
      res = "EVT.INIT_STAGESа";
      break;
    case teInitComps:
      res = "EVT.INIT_COMPS";
      break;
    case teChangeStageESTTime:
      res = "EVT.CHANGE_STAGE_EST_TIME";
      break;
    case teSetSCDOUT:
      res = "EVT.SET_SCD_OUT";
      break;
    case teChangeSCDOUT:
      res = "EVT.CHANGE_SCD_OUT";
      break;
    case teDeleteSCDOUT:
      res = "EVT.DELETE_SCD_OUT";
      break;
    case teSetESTOUT:
      res = "EVT.SET_EST_OUT";
      break;
    case teChangeESTOUT:
      res = "EVT.CHANGE_EST_OUT";
      break;
    case teDeleteESTOUT:
      res = "EVT.DELETE_EST_OUT";
      break;
    case teSetACTOUT:
      res = "EVT.SET_ACT_OUT";
      break;
    case teChangeACTOUT:
      res = "EVT.CHANGE_ACT_OUT";
      break;
    case teDeleteACTOUT:
      res = "EVT.DELETE_ACT_OUT";
      break;
    case teChangeCraftTakeoff:
      res = "EVT.CHANGE_CRAFT_TAKEOFF";
      break;
    case teSetCraftTakeoff:
      res = "EVT.SET_CRAFT_TAKEOFF";
      break;
    case teChangeBortTakeoff:
      res = "EVT.CHANGE_BORT_TAKEOFF";
      break;
    case teSetBortTakeoff:
      res = "EVT.SET_BORT_TAKEOFF";
      break;
    case teChangeLiteraTakeoff:
      res = "EVT.CHANGE_LITERA_TAKEOFF";
      break;
    case teChangeTripTypeTakeoff:
      res = "EVT.CHANGE_TRIP_TYPE_TAKEOFF";
      break;
    case teChangeParkOut:
      res = "EVT.CHANGE_PARK_OUT";
      break;
    case teChangeFlightAttrTakeoff:
      res = "EVT.CHANGE_FLIGHT_ATTR_TAKEOFF";
      break;
    case teChangeDelaysTakeoff:
      res = "EVT.CHANGE_DELAYS_TAKEOFF";
      break;
    case teTranzitTakeoff:
      res = "EVT.TRANZIT_TAKEOFF";
      break;
    case teRegTakeoff:
      res = "EVT.REG_TAKEOFF";
      break;
    case teFirst_PointTakeoff:
      res = "EVT.FIRST_POINT_TAKEOFF";
      break;
    case teChangeRemarkTakeoff:
      res = "EVT.CHANGE_REMARK_TAKEOFF";
      break;
    case tePoint_NumTakeoff:
      res = "EVT.POINT_NUM_TAKEOFF";
      break;
  }
  return res;
}

template <typename T>
void PointsKeyTrip<T>::DoEvents( int move_id )
{
  ProgTrace( TRACE5, "PointsKeyTrip:DoEvents" );
  TQuery Qry(&OraSession);
  for ( int i=(int)teNewLand; i<=(int)tePoint_NumTakeoff; i++ ) {
    if ( this->events.isFlag( (TTripEvents)i ) ) {
      LEvntPrms params;
      params << PrmElem<string>("airl", etAirline, this->key.airline);
      if ( this->key.flt_no != NoExists )
        params << PrmSmpl<int>("flt_no", this->key.flt_no);
      else
        params << PrmSmpl<string>("flt_no", "");
      params << PrmElem<string>("suffix", etSuffix, this->key.airline)
                << PrmElem<string>("airp", etAirp, this->key.airp);
      //TReqInfo::Instance()->LocaleToLog( DecodeEvents( (TTripEvents)i ), params, evtDisp, move_id, this->key.point_id );
    }
  }
  
  
  if ( this->events.isFlag( teInitComps ) ) {
    TReqInfo::Instance()->LocaleToLog( "EVT.SALONS.ASSIGNE_LAYOUT", evtDisp, move_id, this->key.point_id );
    SALONS2::TFindSetCraft res = SALONS2::AutoSetCraft( this->key.point_id );
    if ( res != SALONS2::rsComp_Found && res != SALONS2::rsComp_NoChanges ) {
      if ( this->key.pr_reg &&
           ( this->key.events.isFlag( dmChangeAirline ) ||
             this->key.events.isFlag( dmChangeFltNo ) ||
             this->key.events.isFlag( dmChangeSuffix ) ||
             this->events.isFlag( teChangeCraftTakeoff ) ||
             this->events.isFlag( teChangeBortTakeoff ) ) )
        this->events.setFlag( teNeedChangeComps );
    }
  }
  if ( this->events.isFlag( teNewLand ) ||
       this->events.isFlag( teNewTakeoff ) ||
       this->events.isFlag( teDeleteLand ) ||
       this->events.isFlag( teDeleteTakeoff ) ||
       this->events.isFlag( teSetCancelLand ) ||
       this->events.isFlag( teSetCancelTakeoff ) ||
       this->events.isFlag( teSetUnCancelLand ) ||
       this->events.isFlag( teSetUnCancelTakeoff ) ||
       this->events.isFlag( teTranzitTakeoff ) ||
       this->events.isFlag( teRegTakeoff ) ||
       this->events.isFlag( teChangeFlightAttrTakeoff ) ||
       this->events.isFlag( teChangeCraftTakeoff ) ||
       this->events.isFlag( teSetCraftTakeoff ) ||
       this->events.isFlag( teChangeBortTakeoff ) ||
       this->events.isFlag( teSetBortTakeoff ) ) {
    SALONS2::check_diffcomp_alarm( this->key.point_id );
    if ( SALONS2::isTranzitSalons( this->key.point_id ) ) {
      SALONS2:: check_waitlist_alarm_on_tranzit_routes( this->key.point_id );
    }
    else {
      check_waitlist_alarm( this->key.point_id );
    }
  }
  if ( this->events.isFlag( teSetACTIN ) ||
       this->events.isFlag( teChangeACTIN ) ) {
    //изменение фактического времени прилета
    try {
      //телеграммы на прилет
      TTripRoute route;
      TTripRouteItem prior_airp;
      route.GetPriorAirp(NoExists, this->key.point_id, trtNotCancelled, prior_airp);
      if (prior_airp.point_id!=NoExists)
  	  {
        //проверить автоформирование MVTB!
        vector<TypeB::TCreateInfo> createInfo;
        TypeB::TMVTBCreator(prior_airp.point_id).getInfo(createInfo);
        TelegramInterface::SendTlg(createInfo);
        TReqInfo::Instance()->LocaleToLog( "EVT.TLG.MVTB_CREATION", evtDisp, move_id, this->key.point_id );
      };
    }
    catch(std::exception &E) {
      ProgError(STDLOG,"SetACTIN||ChangeACTIN.SendTlg (point_id=%d): %s",this->key.point_id,E.what());
    };
  }
  if ( this->events.isFlag( teSetACTOUT ) ) {
    try {
      exec_stage( this->key.point_id, sTakeoff );
      TReqInfo::Instance()->LocaleToLog( "EVT.TECHNOLOGY_STEP_TAKEOFF", evtDisp, move_id, this->key.point_id );
    }
    catch( std::exception &E ) {
      ProgError( STDLOG, "SetACTOUT.exec.stages(point_id=%d, sTakeoff): %s", this->key.point_id, E.what() );
    }
    catch( ... ) {
      ProgError( STDLOG, "Unknown error" );
    };
  }

  if ( this->events.isFlag( teSetACTOUT ) ||
       this->events.isFlag( teChangeACTOUT ) ) {
    //изменение фактического времени вылета
    try {
      //проверить автоформирование MVTA!
      vector<TypeB::TCreateInfo> createInfo;
      TypeB::TMVTACreator(this->key.point_id).getInfo(createInfo);
      TelegramInterface::SendTlg(createInfo);
      TReqInfo::Instance()->LocaleToLog( "EVT.TLG.MVTA_CREATION", evtDisp, move_id, this->key.point_id );
    }
    catch(std::exception &E) {
      ProgError(STDLOG,"teSetACTOUT||ChangeACTOUT.SendTlg (point_id=%d): %s",this->key.point_id,E.what());
    };
  }
  if ( this->events.isFlag( teDeleteACTOUT ) ) {
    //отмена вылета
    try {
      Qry.Clear();
  	  Qry.SQLText =
  	    "UPDATE trip_sets SET pr_etstatus=0,et_final_attempt=0 WHERE point_id=:point_id";
      Qry.CreateVariable("point_id",otInteger,this->key.point_id);
      Qry.Execute();
      TReqInfo::Instance()->LocaleToLog( "EVT.ETICKET.DELETE_FLAG", evtDisp, move_id, this->key.point_id );
    }
    catch(std::exception &E) {
      ProgError(STDLOG,"teDeleteACTOUT.ETStatus (point_id=%d): %s",this->key.point_id,E.what());
    };
  }
  if ( this->events.isFlag( teSetUnCancelTakeoff ) ||
       this->events.isFlag( teSetSCDOUT ) ||
       this->events.isFlag( teChangeSCDOUT ) ||
       this->events.isFlag( teChangeCraftTakeoff ) ||
       this->events.isFlag( teSetCraftTakeoff ) ||
       this->events.isFlag( teChangeBortTakeoff ) ||
       this->events.isFlag( teSetBortTakeoff ) ||
       this->events.isFlag( teChangeFlightAttrTakeoff ) ) {
     TPersWeights persWeights; // некрасиво, т.к. каждый раз начитка
     PersWeightRules weights;
     persWeights.getRules( this->key.point_id, weights );
     PersWeightRules oldweights;
     oldweights.read( this->key.point_id );
     if ( !oldweights.equal( &weights ) ) {
       weights.write( this->key.point_id );
     }
  }
       
/*    if ( tripInfo.flt_no != NoExists )
      TReqInfo::Instance()->LocaleToLog("EVT.BINED_PNL", LEvntPrms()
                                        << PrmFlight("flt", tripInfo.airline, tripInfo.flt_no, tripInfo.suffix)
                                        << PrmElem<std::string>("airp", etAirp, tripInfo.airp), evtDisp, move_id, this->key.point_id);

    else
      TReqInfo::Instance()->LocaleToLog("EVT.BINED_PNL", LEvntPrms() << PrmSmpl<std::string>("flt", "")
                                        << PrmElem<std::string>("airp", etAirp, tripInfo.airp), evtDisp, move_id, this->key.point_id);*/
   if ( this->events.isFlag( teNewLand ) ||
        this->events.isFlag( teNewTakeoff ) ||
        this->events.isFlag( teDeleteLand ) ||
        this->events.isFlag( teDeleteTakeoff ) ||
        this->events.isFlag( teSetSCDIN ) ||
        this->events.isFlag( teSetSCDOUT ) ||
        this->events.isFlag( teChangeSCDOUT ) ||
        this->events.isFlag( teDeleteSCDOUT ) ||
        this->events.isFlag( teChangeSCDIN ) ||
        this->events.isFlag( teDeleteSCDIN ) ||
        this->events.isFlag( teDeleteSCDOUT ) ||
        this->events.isFlag( teDeleteSCDIN ) ||
        this->events.isFlag( teChangeFlightAttrTakeoff ) ||
        this->events.isFlag( teChangeFlightAttrLand ) ) {
    if ( this->key.scd_out != ASTRA::NoExists ) {
      try {
        string region = AirpTZRegion( this->key.airp, true );
        TDateTime locale_scd_out = UTCToLocal( this->key.scd_out, region );
        bindingAODBFlt( this->key.airline, this->key.flt_no, this->key.suffix,
                        locale_scd_out, this->key.airp );
      }
      catch(std::exception &E) {
        ProgError(STDLOG,"BindAODBFlt: point_id=%d, %s",this->key.point_id,E.what());
      };
    }
  }

  if ( this->events.isFlag( teChangeCargos ) ) {
    tst();
    this->key.cargos.Save( this->key.point_id, this->dests );
  }
  if ( this->events.isFlag( teChangeMaxCommerce ) ) {
    tst();
    this->key.max_commerce.Save( this->key.point_id );
  }
  if ( this->events.isFlag( teChangeCargos ) || this->events.isFlag( teChangeMaxCommerce ) ) {
    tst();
    check_overload_alarm( this->key.point_id );
  }
  if ( this->events.isFlag( teChangeStations ) ) {
    tst();
    this->key.stations.Save( this->key.point_id );
  }

  if ( this->events.isFlag( teNeedApisUSA ) ) {
    try {
      check_trip_tasks( move_id );
    }
    catch(std::exception &E) {
      ProgError(STDLOG,"internal_WriteDests.check_trip_tasks (move_id=%d): %s",move_id,E.what());
    };
  }
  tst();
}

bool existsTranzitPassengers( int point_id ) // определение того, можно ли сделать из транзитного рейса не транзитный
{
  TQuery Qry(&OraSession);
  Qry.SQLText =
    "SELECT grp_id FROM pax_grp "
    " WHERE point_dep=:point_dep AND point_arv=:point_arv AND bag_refuse=0 AND rownum<2 ";
  Qry.DeclareVariable( "point_dep", otInteger );
  Qry.DeclareVariable( "point_arv", otInteger );
  TTripRoute routes_before, routes_after;
  routes_before.GetRouteBefore( ASTRA::NoExists, point_id, trtNotCurrent, trtNotCancelled );
  routes_after.GetRouteAfter( ASTRA::NoExists, point_id, trtNotCurrent, trtNotCancelled );
  for ( vector<TTripRouteItem>::iterator i=routes_before.begin(); i!=routes_before.end(); i++ ) {
    Qry.SetVariable( "point_dep", i->point_id );
    for ( vector<TTripRouteItem>::iterator j=routes_after.begin(); j!=routes_after.end(); j++ ) {
      Qry.SetVariable( "point_arv", j->point_id );
      Qry.Execute();
      ProgTrace( TRACE5, "point_dep=%d, point_arv=%d, Qry.Eof=%d", i->point_id, j->point_id, Qry.Eof );
      if ( !Qry.Eof ) {
        return true;
      }
    }
  }
  return false;
}

void ReBindTlgs( int move_id, const std::vector<int> &oldPointsId )
{
  TTlgBinding tlgBinding(true);
  TTrferBinding trferBinding;
  tlgBinding.unbind_flt(oldPointsId);
  trferBinding.unbind_flt(oldPointsId);

  vector<TTripInfo> flts;
	TPointDests vdests;

  BitSet<TUseDestData> FUseData;
  FUseData.clearFlags();
  vdests.Load( move_id, FUseData );
  // создаем все возможные рейсы из нового маршрута исключая удаленные пункты
  for( std::vector<TPointsDest>::iterator i=vdests.items.begin(); i!=vdests.items.end(); i++ ) {
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

void TPoints::Save( bool isShowMsg )
{
 //  #warning points.cc. usa apis
  events.clearFlags();
  if ( move_id == NoExists )
    events.setFlag( peInsert );
	vector<TPointsDest>::iterator last_dest = dests.items.end() - 1;
  for( vector<TPointsDest>::iterator id=dests.items.begin(); id!=dests.items.end(); id++ ) {
  	if ( id->point_id == NoExists || id->pr_del == -1 ) { // вставка или удаление пункта посадки
  	  events.setFlag( pePointNum );
  	  ProgTrace( TRACE5, "events: pePointNum" );
  		break;
  	}
  }
  if ( events.isFlag( pePointNum ) ) {
    int point_diff = 0, prior_point_num = -1;
    for( vector<TPointsDest>::iterator id=dests.items.begin(); id!=dests.items.end(); id++ ) {
      if ( id->pr_del == -1 ) {
        id->point_num = 0-id->point_num-1;
        continue;
      }
      if ( id->point_id == NoExists ) {
        id->point_num = prior_point_num + 1;
        prior_point_num = id->point_num;
        point_diff++;
      }
      else
 	      id->point_num += point_diff;
      last_dest = id;
      prior_point_num = id->point_num;
      ProgTrace( TRACE5, "id->point_id=%d, id->point_num=%d, id->key=%s", id->point_id, id->point_num, id->key.c_str() );
    }
  }
  tst();
  // задание параметров pr_tranzit, pr_reg, first_point, prep_reg.cc:508 - проверка на возможность разбития рейса, если есть транзитные пассажиры
  bool pr_begin = true;
  int first_point;
  int notCancelCount = dests.items.size();
  vector<TPointsDest>::iterator pid=dests.items.end();
  vector<TPointsDest>::iterator ilast = dests.items.end();
  for( vector<TPointsDest>::iterator id=dests.items.begin(); id!=dests.items.end(); id++ ) {
  	if ( id->pr_del == -1 ) {
  	  id->status = tdDelete;
      continue;
    }
    else
      ilast = id;
    if ( id->point_id == ASTRA::NoExists ) {
      id->status = tdInsert;
      id->point_id = getPoint_id();
    }
    else
      id->status = tdUpdate;
    if ( id->pr_del == 1 ) {
      notCancelCount--;
    }
  	if( pid == dests.items.end() || id + 1 == dests.items.end() )
  		id->pr_tranzit = 0;
  	else
  	 if ( id->status == tdInsert ||events.isFlag( pePointNum ))
       id->pr_tranzit = ( pid->airline + IntToString( pid->flt_no ) + pid->suffix /*+ p->triptype ???*/ ==
                          id->airline + IntToString( id->flt_no ) + id->suffix /*+ id->triptype*/ );
    //ProgTrace( TRACE5, "id->pr_tranzit=%d, pid->suffix=|%s|, id->suffix=|%s|", id->pr_tranzit, pid->suffix.c_str(), id->suffix.c_str() );

    id->pr_reg = ( id->scd_out > NoExists &&
                   ((TTripTypesRow&)base_tables.get("trip_types").get_row( "code", id->trip_type, true )).pr_reg!=0 &&
                   id != last_dest );
    if ( id == last_dest ) { // последний пункт
      id->trip_type.clear();
      id->bort.clear();
      id->craft.clear();
      id->litera.clear();
    }
 		if ( pr_begin ) {
 		  pr_begin = false;
      first_point = id->point_id;
      id->first_point = NoExists;
    }
    else {
      id->first_point = first_point;
      if ( !id->pr_tranzit )
        first_point = id->point_id;
    }
  	pid = id;
  }
  
  // отменяем все п.п., т.к. в маршруте всего один не отмененный
  if ( notCancelCount == 1 ) {
    for( vector<TPointsDest>::iterator id=dests.items.begin(); id!=dests.items.end(); id++ ) {
     	if ( !id->pr_del ) {
    		id->pr_del = 1;
      }
    }
  }
  if ( ilast != dests.items.end() ) { // удаляем все ненужные значения полей
    ilast->airline.clear();
    ilast->flt_no = NoExists;
    ilast->suffix.clear();
    ilast->craft.clear();
    ilast->bort.clear();
    ilast->trip_type.clear();
    ilast->litera.clear();
    ilast->park_out.clear();
  }
  TQuery Qry(&OraSession);
  //лочим рейс
  if ( events.isFlag( peInsert ) ) {
  /* необходимо сделать проверку на не существование рейса*/
    Qry.SQLText =
     "BEGIN "\
     " SELECT move_id.nextval INTO :move_id from dual; "\
     " INSERT INTO move_ref(move_id,reference)  SELECT :move_id, :reference FROM dual; "\
     "END;";
    Qry.DeclareVariable( "move_id", otInteger );
    Qry.CreateVariable( "reference", otString, ref );
    Qry.Execute();
    move_id = Qry.GetVariableAsInteger( "move_id" );
  }
  else {
    for( vector<TPointsDest>::iterator id=dests.items.begin(); id!=dests.items.end(); id++ ) {
      if ( id->status != tdInsert ) {
        TFlights flights;
		    flights.Get( id->point_id, ftAll );
		    flights.Lock();
        break;
      }
    }
    Qry.Clear();
    Qry.SQLText =
      " UPDATE move_ref SET reference=:reference WHERE move_id=:move_id ";
    Qry.CreateVariable( "move_id", otInteger, move_id );
    Qry.CreateVariable( "reference", otString, ref );
    Qry.Execute();
  }
  tst();
  //проверка на то, что пункт можно отменить или удалить
  map<int,TPointsDest> olddests;
  for( vector<TPointsDest>::iterator id=dests.items.begin(); id!=dests.items.end(); id++ ) {
    ProgTrace( TRACE5, "id->airp=%s, id->pr_del=%d, id->craft=%s", id->airp.c_str(), id->pr_del, id->craft.c_str() );
    if ( id->status != tdInsert ) {
      ProgTrace( TRACE5, "load: point_id=%d", id->point_id );
      olddests[ id->point_id ].Load( id->point_id, id->UseData );
    }
    ProgTrace( TRACE5, "id->airp=%s, id->pr_del=%d, id->craft=%s", id->airp.c_str(), id->pr_del, id->craft.c_str() );
    id->getEvents( olddests[ id->point_id ] );
    ProgTrace( TRACE5, "id->airp=%s, id->pr_del=%d, id->craft=%s", id->airp.c_str(), id->pr_del, id->craft.c_str() );
/*    if ( id->status != tdInsert ) { // работая в лат. терминале - возвращаем лат. коды, но при этом не должны коды портиться у других
      if ( !olddests[ id->point_id ].airline.empty() && !id->events.isFlag( dmChangeAirline ) )
        id->airline_fmt = olddests[ id->point_id ].airline_fmt;
      if ( !olddests[ id->point_id ].suffix.empty() && !id->events.isFlag( dmChangeSuffix ) )
        id->suffix_fmt = olddests[ id->point_id ].suffix_fmt;
      if ( !olddests[ id->point_id ].airp.empty() && !id->events.isFlag( dmChangeAirp ) )
        id->airp_fmt = olddests[ id->point_id ].airp_fmt;
      if ( !olddests[ id->point_id ].craft.empty() && !id->events.isFlag( dmChangeCraft ) )
        id->craft_fmt = olddests[ id->point_id ].craft_fmt;
    }*/
    if ( id->events.isFlag( dmSetDelete ) ||
         id->events.isFlag( dmSetCancel ) ) {
      tst();
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
  			if ( id->events.isFlag( dmSetDelete) ) {
  		 	  throw AstraLocale::UserException( "MSG.ROUTE.UNABLE_DEL_AIRP.PAX_EXISTS",
  		                                      LParams() << LParam("airp", ElemIdToCodeNative(etAirp,id->airp)));
        }
  		  else {
  		    throw AstraLocale::UserException( "MSG.ROUTE.UNABLE_CANCEL_AIRP.PAX_EXISTS",
  			                                    LParams() << LParam("airp", ElemIdToCodeNative(etAirp,id->airp)));
        }
      }
    }
    ProgTrace( TRACE5, "id->airp=%s, id->pr_del=%d, id->craft=%s", id->airp.c_str(), id->pr_del, id->craft.c_str() );
    if ( id->events.isFlag( dmTranzit ) && !id->pr_tranzit ) {
      if ( existsTranzitPassengers( id->point_id ) )
        throw AstraLocale::UserException( "MSG.ROUTE.UNABLE_CHANGE_PR_TRANZIT",
                                          LParams() << LParam("airp", ElemIdToCodeNative(etAirp,id->airp)));
    }
    ProgTrace( TRACE5, "id->airp=%s, id->pr_del=%d", id->airp.c_str(), id->pr_del );
    id->setRemark( olddests[ id->point_id ] );
    ProgTrace( TRACE5, "id->airp=%s, id->pr_del=%d, id->remark=%s", id->airp.c_str(), id->pr_del, id->remark.c_str() );
  }
  //сохранение маршрута
  tst();
  for( vector<TPointsDest>::iterator id=dests.items.begin(); id!=dests.items.end(); id++ ) {
    ProgTrace( TRACE5, "id->point_id=%d,point_id->num=%d, id->airp=%s, id->bort=%s, id->craft=%s, id->est_out=%f",
                id->point_id, id->point_num, id->airp.c_str(), id->bort.c_str(), id->craft.c_str(), id->est_out );
    WriteDest( *id );
  }
  string lexema_id;
  PrmEnum prmenum("flt", "-");
  for( vector<TPointsDest>::iterator id=dests.items.begin(); id!=dests.items.end(); id++ ) {
    id->DoEvents( move_id, olddests[ id->point_id ] );
    if ( events.isFlag( pePointNum ) && id->status != tdDelete ) {
      if ( lexema_id.empty() ) {
        if ( status == peInsert )
          lexema_id = "EVT.FLIGHT.NEW";
        else
          lexema_id = "EVT.FLIGHT.MODIFY_ROUTE";
      }
      if ( id->flt_no != NoExists )
        prmenum.prms << PrmFlight("", id->airline, id->flt_no, id->suffix)
                     << PrmElem<std::string>("", etAirp, id->airp);
      else
        prmenum.prms << PrmElem<std::string>("", etAirp, id->airp);
    }
  }
  if ( !lexema_id.empty() )
    TReqInfo::Instance()->LocaleToLog(lexema_id, LEvntPrms() << prmenum, evtDisp, move_id);

  //получение событий по рейсам, которые есть или были в маршруте
  vector<PointsKeyTrip<TPointsDest> > keytrips1, keytrips2;
  // создаем все возможные рейсы из нового маршрута исключая удаленные пункты
  getKeyTrips( dests.items, keytrips1 );
  std::vector<TPointsDest> priorDests;
  std::vector<int> priorPointIds;

  for ( map<int,TPointsDest>::iterator i=olddests.begin(); i!=olddests.end(); i++ ) {
    if ( i->second.point_id == NoExists )
      continue;
    ProgTrace( TRACE5, "priorDests.push_back point_id=%d, i->first=%d", i->second.point_id, i->first );
    priorDests.push_back( i->second );
    priorPointIds.push_back( i->second.point_id );
  }
  // создаем всевозможные рейсы из старого маршрута исключая удаленные пункты
  getKeyTrips( priorDests, keytrips2 );
  TPointsDest dest;
  dest.point_id = ASTRA::NoExists;
  // пробег по новым рейсам
  for ( vector<PointsKeyTrip<TPointsDest> >::iterator i=keytrips1.begin(); i!=keytrips1.end(); i++ ) {
    vector<PointsKeyTrip<TPointsDest> >::iterator j = keytrips2.begin();
  	for ( ; j!=keytrips2.end(); j++ ) { // ищем в старом нужный рейс
  	  if ( i->key.point_id == j->key.point_id ) {
        tst();
        i->getEvents( *j );
  	  	break;
      }
    }
    if ( j == keytrips2.end() ) { //не нашли старый рейс => новый
      PointsKeyTrip<TPointsDest> del_trip( dest );
      tst();
      i->getEvents( del_trip );
      continue;
    }
    keytrips2.erase( j );
  }
  tst();
  // пробег по старым рейсам рейсам
  for ( vector<PointsKeyTrip<TPointsDest> >::iterator i=keytrips2.begin(); i!=keytrips2.end(); i++ ) {
    i->key.status = tdDelete;
    tst();
    i->getEvents( *i );
    keytrips1.push_back( *i );
  }
  //пробег по рейсам и выполнение событий
  bool pr_need_changecomps;
  for ( vector<PointsKeyTrip<TPointsDest> >::iterator i=keytrips1.begin(); i!=keytrips1.end(); i++ ) {
    i->DoEvents( move_id );
    if ( i->isNeedChangeComps() )
      pr_need_changecomps = true;
  }
  //перепривязка телеграмм
  ReBindTlgs( move_id, priorPointIds );

  if ( isShowMsg ) {
    if ( pr_need_changecomps )
      AstraLocale::showErrorMessage( "MSG.DATA_SAVED.NEED_SET_COMPON" );
    else
      AstraLocale::showMessage( "MSG.DATA_SAVED" );
  }
}

bool TPoints::isDouble( int move_id, std::string airline, int flt_no,
	                      std::string suffix, std::string airp,
	                      BASIC::TDateTime scd_in, BASIC::TDateTime scd_out,
                        int &findMove_id, int &point_id )
{
  findMove_id = NoExists;
  point_id = NoExists;
  ProgTrace( TRACE5, "TPoints::isDouble: move_id=%d, airline=%s, flt_no=%d, suffix=%s, airp=%s, scd_in=%s(%f), scd_out=%s(%f)",
             move_id, airline.c_str(), flt_no, suffix.c_str(), airp.c_str(),
             DateTimeToStr( scd_in, "hh:nn dd.mm.yy" ).c_str(), scd_in,
             DateTimeToStr( scd_out, "hh:nn dd.mm.yy" ).c_str(), scd_out );
  TElemFmt fmt;
  airp = ElemToElemId( etAirp, airp, fmt );
  suffix = ElemToElemId( etSuffix, suffix, fmt );
  airline = ElemToElemId( etAirline, airline, fmt );
  double local_scd_in,local_scd_out,d1;
  TBaseTable &baseairps = base_tables.get( "airps" );
  string region = CityTZRegion( ((TAirpsRow&)baseairps.get_row( "code", airp, true )).city );
  if ( scd_in > NoExists ) {
    d1 = UTCToLocal( scd_in, region );
    modf( d1, &local_scd_in );
  }
  else local_scd_in = NoExists;
  if ( scd_out > NoExists ) {
    d1 = UTCToLocal( scd_out, region );
    modf( d1, &local_scd_out );
  }
  else local_scd_out = NoExists;

  TQuery Qry(&OraSession);
  Qry.SQLText =
    "SELECT airline, flt_no, suffix, scd_in, scd_out, move_id, point_id, point_num FROM points "
    "  WHERE move_id!=:move_id AND airp=:airp AND pr_del!=-1 AND "
    "       ( scd_in BETWEEN :scd_in-2 AND :scd_in+2 OR "
    "         scd_out BETWEEN :scd_out-2 AND :scd_out+2 ) "
    "ORDER BY move_id,point_num ";
/*    "SELECT scd_in, scd_out, move_id, point_id FROM points "
    " WHERE airline=:airline AND flt_no=:flt_no AND NVL(suffix,' ')=NVL(:suffix,' ') AND "
    "       move_id!=:move_id AND airp=:airp AND pr_del!=-1 AND "
    "       ( scd_in BETWEEN :scd_in-2 AND :scd_in+2 OR "
    "         scd_out BETWEEN :scd_out-2 AND :scd_out+2 )";*/
  Qry.CreateVariable( "move_id", otInteger, move_id );
  Qry.CreateVariable( "airp", otString, airp );
/*  Qry.CreateVariable( "airline", otString,airline );
  Qry.CreateVariable( "flt_no", otInteger, flt_no );
  Qry.CreateVariable( "suffix", otString, suffix );*/
	if ( scd_in > NoExists ) {
    double d;
    modf( scd_in, &d );
	  Qry.CreateVariable( "scd_in", otDate, d );
  }
	else
		Qry.CreateVariable( "scd_in", otDate, FNull );
	if ( scd_out > NoExists ) {
    double d;
    modf( scd_out, &d );
		Qry.CreateVariable( "scd_out", otDate, d );
  }
	else
		Qry.CreateVariable( "scd_out", otDate, FNull );
	Qry.Execute();
  string prior_airline, prior_suffix;
  int prior_flt_no;
  while ( !Qry.Eof ) {
    if ( !Qry.FieldIsNULL( "airline" ) ) {
      prior_airline = Qry.FieldAsString( "airline" );
      prior_suffix = Qry.FieldAsString( "suffix" );
      if ( Qry.FieldIsNULL( "flt_no" ) ) {
        prior_flt_no = NoExists;
      }
      else {
        prior_flt_no = Qry.FieldAsInteger( "flt_no" );
      }
    }
    else {
      prior_airline.clear();
      prior_suffix.clear();
      prior_flt_no = NoExists;
      TTripRoute route;
      if ( route.GetRouteBefore( NoExists, Qry.FieldAsInteger( "point_id" ), trtNotCurrent, trtWithCancelled ) && !route.empty() ) {
        TQuery QryFlt(&OraSession);
        QryFlt.SQLText = "SELECT airline,flt_no,suffix FROM points WHERE point_id=:point_id";
        QryFlt.CreateVariable( "point_id", otInteger, route.back().point_id );
        QryFlt.Execute();
        if ( !QryFlt.Eof ) {
          prior_airline = QryFlt.FieldAsString( "airline" );
          prior_suffix = QryFlt.FieldAsString( "suffix" );
          if ( QryFlt.FieldIsNULL( "flt_no" ) ) {
            prior_flt_no = NoExists;
          }
          else {
            prior_flt_no = QryFlt.FieldAsInteger( "flt_no" );
          }
        }
      }
    }
    if ( airline != prior_airline ||
         suffix != prior_suffix ||
         flt_no != prior_flt_no ) {
      Qry.Next();
      continue;
    }
    tst();
  	if ( !Qry.FieldIsNULL( "scd_in" ) && local_scd_in > NoExists ) {
      modf( (double)UTCToLocal( Qry.FieldAsDateTime( "scd_in" ), region ), &d1 );
      if ( d1 == local_scd_in ) {
        findMove_id = Qry.FieldAsInteger( "move_id" );
        point_id = Qry.FieldAsInteger( "point_id" );
        ProgTrace( TRACE5, "isDouble: scd_in, move_id=%d, findMovde_id=%d, point_id=%d",
                   move_id, findMove_id, point_id );
   			return true;
      }
    }
    if ( !Qry.FieldIsNULL( "scd_out" ) && local_scd_out > NoExists ) {
      modf( (double)UTCToLocal( Qry.FieldAsDateTime( "scd_out" ), region ), &d1 );
      if ( d1 == local_scd_out ) {
        findMove_id = Qry.FieldAsInteger( "move_id" );
        point_id = Qry.FieldAsInteger( "point_id" );
        ProgTrace( TRACE5, "isDouble: scd_in, move_id=%d, findMovde_id=%d, point_id=%d",
                   move_id, findMove_id, point_id );
   			return true;
      }
    }
    Qry.Next();
  }
  tst();
	return false;
}

bool TPoints::isDouble( int move_id, std::string airline, int flt_no,
	                      std::string suffix, std::string airp,
	                      BASIC::TDateTime scd_in, BASIC::TDateTime scd_out )
{
  int findMove_id, point_id;
  return isDouble( move_id, airline, flt_no, suffix, airp, scd_in, scd_out, findMove_id, point_id );
}

void ConvertSOPPToPOINTS( int move_id, const TSOPPDests &dests, string reference, TPoints &points )
{
  points.move_id = move_id;
  points.ref = reference;
  points.dests.items.clear();
  for( TSOPPDests::const_iterator i=dests.begin(); i!=dests.end(); i++ ) {
    TPointsDest dest;
    dest.UseData.setFlag( udDelays );
    dest.point_id = i->point_id;
    dest.point_num = i->point_num;
    dest.airp = i->airp;
    dest.pr_tranzit = i->pr_tranzit;
    dest.first_point = i->first_point;
    dest.airline = i->airline;
    dest.flt_no = i->flt_no;
    dest.suffix = i->suffix;
    dest.craft = i->craft;
    dest.bort = i->bort;
    dest.scd_in = i->scd_in;
    dest.est_in = i->est_in;
    dest.act_in = i->act_in;
    dest.scd_out = i->scd_out;
    dest.est_out = i->est_out;
    dest.act_out = i->act_out;
    dest.trip_type = i->triptype;
    dest.litera = i->litera;
    dest.park_in = i->park_in;
    dest.park_out = i->park_out;
    dest.remark = i->remark;
    dest.pr_reg = i->pr_reg;
    dest.pr_del = i->pr_del;
    dest.tid = i->tid;
    dest.airline_fmt = i->airline_fmt;
    dest.airp_fmt = i->airp_fmt;
    dest.suffix_fmt = i->suffix_fmt;
    dest.craft_fmt = i->craft_fmt;
    for ( vector<TSOPPDelay>::const_iterator d=i->delays.begin(); d!=i->delays.end(); d++ ) {
      TPointsDestDelay dd;
      dd.code = d->code;
      dd.time = d->time;
      dest.delays.Add( dd );
    }
    points.dests.items.push_back( dest );
  }
}

void WriteDests( TPoints &points, bool ignoreException,
                 XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode )
{
  ProgTrace( TRACE5, "WriteDests" );
  AstraLocale::LexemaData lexemaData;
  points.Verify( ignoreException, lexemaData );
  ProgTrace( TRACE5, "WriteDests after points.Verify" );
  if ( !ignoreException && !lexemaData.lexema_id.empty() ) {
    tst();
  	NewTextChild( NewTextChild( resNode, "data" ), "notvalid" );
  	AstraLocale::showErrorMessage( "MSG.ERR_MSG.REPEAT_F9_SAVE", LParams() << LParam("msg", getLocaleText(lexemaData)));
  }
  else {
    tst();
    points.Save( true );
    NewTextChild( reqNode, "move_id", points.move_id );
//    INT_ReadDests( ctxt, reqNode, resNode );
  }
}

bool findFlt( const std::string &airline, const int &flt_no, const std::string &suffix,
              const BASIC::TDateTime &local_scd_out, const std::string &airp, const int &withDeleted,
              TFndFlts &flts )
{
  flts.clear();
  TQuery Qry(&OraSession);
  string sql =
	 "SELECT point_id, move_id, airp, scd_out, pr_del "
	 " FROM points "
   "WHERE airline=:airline AND flt_no=:flt_no AND "
	 "     ( suffix IS NULL AND :suffix IS NULL OR suffix=:suffix ) AND "
	 "     airp=:airp AND "
	 "     scd_out>=:scd_out AND scd_out<:scd_out+1";
  if ( !withDeleted )
    sql += " AND pr_del != -1";
  Qry.SQLText = sql;
	Qry.CreateVariable( "airline", otString, airline );
	Qry.CreateVariable( "flt_no", otInteger, flt_no );
	if ( suffix.empty() )
	  Qry.CreateVariable( "suffix", otString, FNull );
	else
		Qry.CreateVariable( "suffix", otString, suffix );
  string region = AirpTZRegion( airp, true );
	TDateTime  utc_scd_out =  LocalToUTC( local_scd_out, region );
	modf( utc_scd_out, &utc_scd_out );
	Qry.CreateVariable( "scd_out", otDate, utc_scd_out );
	Qry.CreateVariable( "airp", otString, airp );
	Qry.Execute();
	TDateTime lso;
	modf( local_scd_out, &lso );
	//может выбраться 2 рейса
	while ( !Qry.Eof ) {
    tst();
    TDateTime local_scd_out1 = UTCToLocal( Qry.FieldAsDateTime( "scd_out" ), region );
    modf( local_scd_out1, &local_scd_out1 );
    ProgTrace( TRACE5, "local_scd_out1=%f, local_scd_out=%f", local_scd_out1, lso );
    if ( local_scd_out1 == lso ) {
      tst();
      TFndFlt flt;
      flt.move_id = Qry.FieldAsInteger( "move_id" );
      flt.point_id = Qry.FieldAsInteger( "point_id" );
      flt.pr_del = Qry.FieldAsInteger( "pr_del" );
      flts.push_back( flt );
    }
    Qry.Next();
	}
	return !flts.empty();
}

/////////////////////Cargos/////////////////////////////////////////////////////
void TFlightCargos::Load( int point_id, bool pr_tranzit, int first_point, int point_num, int pr_cancel )
{
  TQuery Qry(&OraSession);
  Qry.SQLText =
    "SELECT cargo,mail,a.airp airp_arv,a.airp_fmt airp_arv_fmt, a.point_id point_arv, a.point_num "
    " FROM trip_load, "
    "( SELECT point_id, point_num, airp, airp_fmt FROM points "
    "   WHERE first_point=:first_point AND point_num>:point_num AND pr_del=:pr_cancel ) a, "
    "( SELECT MIN(point_num) as point_num FROM points "
    "   WHERE first_point=:first_point AND point_num>:point_num AND pr_del=:pr_cancel "
    "  GROUP BY airp ) b "
    "WHERE a.point_num=b.point_num AND trip_load.point_dep(+)=:point_id AND "
    "      trip_load.point_arv(+)=a.point_id "
    "ORDER BY a.point_num ";
  Qry.CreateVariable( "point_id", otInteger, point_id );
  if ( !pr_tranzit )
    Qry.CreateVariable( "first_point", otInteger, point_id );
  else
    Qry.CreateVariable( "first_point", otInteger, first_point );
  Qry.CreateVariable( "point_num", otInteger, point_num );
  Qry.CreateVariable( "pr_cancel", otInteger, pr_cancel );
  Qry.Execute();
  while ( !Qry.Eof ) {
    TPointsDestCargo cargo;
    cargo.cargo = Qry.FieldAsInteger( "cargo" );
    cargo.mail = Qry.FieldAsInteger( "mail" );
    cargo.point_arv = Qry.FieldAsInteger( "point_arv" );
    cargo.key = Qry.FieldAsString( "point_arv" );
    cargo.airp_arv =  Qry.FieldAsString( "airp_arv" );
    cargo.airp_arv_fmt = (TElemFmt)Qry.FieldAsInteger( "airp_arv_fmt" );
    Add( cargo );
    Qry.Next();
  }
}

void TFlightCargos::Save( int point_id, const vector<TPointsDest> &dests )
{
  ProgTrace( TRACE5, "TFlightCargos::Save" );
  vector<TPointsDest>::const_iterator owndest = dests.end();
  vector<TPointsDest>::const_iterator idest = dests.end();
  for ( idest=dests.begin(); idest!=dests.end(); idest++ ) {
    if ( idest->point_id == point_id ) {
      owndest = idest;
      ProgTrace( TRACE5, "owndest->point_id=%d, calcPoint_id=%d", owndest->point_id, calcPoint_id() );
      idest++;
      break;
    }
  }
  if ( owndest == dests.end() )
    throw Exception( "TFlightCargos::Save: point_id=%d, own airp not found in dests" );
  for ( vector<TPointsDestCargo>::iterator icargo=cargos.begin(); icargo!=cargos.end(); icargo++ ) {
    vector<TPointsDest>::const_iterator jdest=idest;
    for ( ; jdest!=dests.end(); jdest++ ) {
      if ( calcPoint_id() && ( icargo->key.empty() || jdest->key.empty() ) ) {
        throw Exception( "TFlightCargos::Save: calcPoint_id=true, but key is empty" );
      }
      if ( (calcPoint_id() && icargo->key == jdest->key) ||
           (!calcPoint_id() && icargo->point_arv == jdest->point_id) ) {
        icargo->point_arv = jdest->point_id;
        icargo->airp_arv = jdest->airp;
        break;
      }
    }
    if ( jdest == dests.end() )
      throw Exception( "TFlightCargos::Save: calcPoint_id=true, but cargo.key=%s not found in dests", icargo->key.c_str() );
    ProgTrace( TRACE5, "icargo->point_arv=%d", icargo->point_arv );
  }
  tst();
  TFlightCargos oldcargos;
  oldcargos.Load( point_id, owndest->pr_tranzit, owndest->first_point, owndest->point_num, owndest->pr_del );
  tst();
  TQuery Qry(&OraSession);
  Qry.SQLText =
    "BEGIN "
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
  ProgTrace( TRACE5, "oldcargos.cargos.size()=%zu", oldcargos.cargos.size() );
  for ( vector<TPointsDestCargo>::iterator icargo=cargos.begin(); icargo!=cargos.end(); icargo++ ) {
    vector<TPointsDestCargo>::iterator jcargo=oldcargos.cargos.begin();
    for ( ; jcargo!=oldcargos.cargos.end(); jcargo++ ) {
      if ( icargo->point_arv == jcargo->point_arv ) {
        icargo->airp_arv_fmt = jcargo->airp_arv_fmt; // для equal
        jcargo->key = icargo->key;
        tst();
        break;
      }
    }
    ProgTrace( TRACE5, "icargo->point_arv=%d, icargo->airp_arv=%s, icargo->cargo=%d, icargo->mail=%d",
               icargo->point_arv, icargo->airp_arv.c_str(), icargo->cargo, icargo->mail );
    if ( jcargo == oldcargos.cargos.end() || !icargo->equal( *jcargo ) ) {
      Qry.SetVariable( "point_arv", icargo->point_arv );
      Qry.SetVariable( "airp_arv", icargo->airp_arv );
      Qry.SetVariable( "cargo", icargo->cargo );
      Qry.SetVariable( "mail", icargo->mail );
      Qry.Execute();
      TReqInfo::Instance()->LocaleToLog("EVT.CARGO_MAIL_WEIGHT", LEvntPrms()
                                     << PrmElem<std::string>("airp", etAirp, icargo->airp_arv)
                                     << PrmSmpl<int>("cargo_weight", icargo->cargo)
                                     << PrmSmpl<int>("mail_weight", icargo->mail),
                                     evtFlt, point_id);
    }
    if ( jcargo != oldcargos.cargos.end() )
      oldcargos.cargos.erase( jcargo );
  }
  tst();
  Qry.Clear();
  Qry.SQLText =
    "DELETE trip_load WHERE point_dep=:point_dep AND point_arv=:point_arv";
  Qry.CreateVariable( "point_dep", otInteger, point_id );
  Qry.DeclareVariable( "point_arv", otInteger );
  //удаление
  for ( vector<TPointsDestCargo>::iterator jcargo=oldcargos.cargos.begin(); jcargo!=oldcargos.cargos.end(); jcargo++ ) {
    Qry.SetVariable( "point_arv", jcargo->point_arv );
    Qry.Execute();
    TReqInfo::Instance()->LocaleToLog("EVT.CARGO_MAIL_WEIGHT", LEvntPrms()
                                   << PrmElem<std::string>("airp", etAirp, jcargo->airp_arv)
                                   << PrmSmpl<int>("cargo_weight", 0)
                                   << PrmSmpl<int>("mail_weight", 0),
                                   evtFlt, point_id);
  }
  tst();
}
//////////////////////////////////////TFlightMaxCommerce////////////////////////
void TFlightMaxCommerce::Load( int point_id )
{
  TQuery Qry(&OraSession);
  Qry.SQLText =
    "SELECT max_commerce FROM trip_sets WHERE point_id=:point_id";
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.Execute();
  if ( Qry.Eof || Qry.FieldIsNULL( "max_commerce" ) )
    value = ASTRA::NoExists;
  else
    value = Qry.FieldAsInteger( "max_commerce" );
}

void TFlightMaxCommerce::Save( int point_id )
{
  tst();
  TFlightMaxCommerce old;
  old.Load( point_id );
  ProgTrace( TRACE5, "value=%d", value );
  if ( old.value == value )
    return;
  bool pr_overload_alarm = Get_AODB_overload_alarm( point_id, value );
  TQuery Qry(&OraSession);
  Qry.SQLText =
  	"UPDATE trip_sets SET max_commerce=:max_commerce "
    " WHERE point_id=:point_id";
  Qry.CreateVariable( "point_id", otInteger, point_id );
  if ( value == NoExists )
    Qry.CreateVariable( "max_commerce", otInteger, FNull );
  else
    Qry.CreateVariable( "max_commerce", otInteger, value );
 	Qry.Execute();
 	if ( value == NoExists )
      TReqInfo::Instance()->LocaleToLog("EVT.MAX_COMMERCE_LOAD_UNKNOWN", evtFlt, point_id);
 	else
      TReqInfo::Instance()->LocaleToLog("EVT.MAX_COMMERCE_LOAD", LEvntPrms()
                                        << PrmSmpl<int>("weight", value), evtFlt, point_id);
  Set_AODB_overload_alarm( point_id, pr_overload_alarm );
}
////////////////////////////////////TFlightDelays///////////////////////////////

void TFlightDelays::Load( int point_id )
{
   TQuery Qry(&OraSession);
   Qry.SQLText = points_delays_SQL;
   Qry.CreateVariable( "point_id", otInteger, point_id );
   Qry.Execute();
   while ( !Qry.Eof ) {
     tst();
 		 TPointsDestDelay delay;
 		 delay.code = Qry.FieldAsString( "delay_code" );
 		 delay.time = Qry.FieldAsDateTime( "time" );
 		 delays.push_back( delay );
 		 Qry.Next();
   }
}

void TFlightDelays::Save( int point_id )
{
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText = "DELETE trip_delays WHERE point_id=:point_id";
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.Execute();
  if ( !delays.empty() ) {
  	Qry.Clear();
  	Qry.SQLText =
  	 "INSERT INTO trip_delays(point_id,delay_num,delay_code,time) "\
  	 " VALUES(:point_id,:delay_num,:delay_code,:time) ";
  	Qry.CreateVariable( "point_id", otInteger, point_id );
  	Qry.DeclareVariable( "delay_num", otInteger );
  	Qry.DeclareVariable( "delay_code", otString );
  	Qry.DeclareVariable( "time", otDate );
  	int r=0;
  	for ( vector<TPointsDestDelay>::iterator q=delays.begin(); q!=delays.end(); q++ ) {
  		Qry.SetVariable( "delay_num", r );
  		Qry.SetVariable( "delay_code", q->code );
  		Qry.SetVariable( "time", q->time );
  		Qry.Execute();
  		r++;
  	}
  }
}

////////////////////////////////////TFlightStages///////////////////////////////
void TFlightStages::Load( int point_id )
{
  TTripStages::LoadStages( point_id, stages );
}

void TFlightStages::Save( int point_id )
{
  ProgTrace( TRACE5, "TFlightStages::Save, point_id=%d", point_id );
  TFlightStages old;
  old.Load( point_id );
  if ( old.Empty() )
    throw Exception( "TFlightStages::Save stages not init, point_id=%d", point_id );
  TMapTripStages forSaveStages;
  for ( TMapTripStages::iterator istage=old.stages.begin(); istage!=old.stages.end(); istage++ ) {
    stages[ istage->first ].scd = istage->second.scd;
    stages[ istage->first ].old_est = istage->second.est;
    stages[ istage->first ].old_act = istage->second.act;
    stages[ istage->first ].pr_auto = istage->second.pr_auto;
    if ( stages[ istage->first ].old_est != stages[ istage->first ].est ||
         stages[ istage->first ].old_act != stages[ istage->first ].act ) {
      forSaveStages[ istage->first ] = stages[ istage->first ];
    }
  }
  TTripStages::WriteStagesUTC( point_id, forSaveStages );
  on_change_trip( CALL_POINT, point_id );
}

/////////////////////////////////////TFlightStations////////////////////////////
void TFlightStations::Load( int point_id )
{
  get_DesksGates( point_id, stations );
};

void TFlightStations::Save( int point_id )
{
  ProgTrace( TRACE5, "TFlightStations::Save, point_id=%d", point_id );
  TFlightStations old;
  old.Load( point_id );
  TQuery DelQry(&OraSession);
  DelQry.SQLText =
    "DELETE trip_stations WHERE point_id=:point_id AND work_mode=:work_mode";
  DelQry.CreateVariable( "point_id", otInteger, point_id );
  DelQry.DeclareVariable( "work_mode", otString );
  TQuery Qry(&OraSession);
  Qry.SQLText =
    "INSERT INTO trip_stations(point_id,desk,work_mode,pr_main) "
    " SELECT :point_id,desk,:work_mode,:pr_main FROM stations,points "
    "  WHERE points.point_id=:point_id AND stations.airp=points.airp AND name=:name";
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.DeclareVariable( "name", otString );
  Qry.DeclareVariable( "pr_main", otInteger );
  Qry.DeclareVariable( "work_mode", otString );

  string work_mode;
  for ( int i=0; i<=1; i++ ) {
    switch ( i ) {
      case 0:
        work_mode = "Р";
        break;
      case 1:
        work_mode = "П";
        break;
    }
    if ( !equal( old, work_mode ) ) {
      DelQry.SetVariable( "work_mode", work_mode );
      DelQry.Execute();
      Qry.SetVariable( "work_mode", work_mode );
      vector<string> terms;
      string lexema_id;
      PrmEnum prmenum("names", ",");
      for ( tstations::iterator istation=stations.begin(); istation!=stations.end(); istation++ ) {
        if ( istation->work_mode != work_mode )
          continue;
        if ( find( terms.begin(), terms.end(), istation->name ) == terms.end() ) {
          terms.push_back( istation->name );
        	Qry.SetVariable( "name", istation->name );
       		Qry.SetVariable( "pr_main", istation->pr_main );
          Qry.Execute();
          if ( istation->pr_main ) {
            PrmLexema prmlexema("", "EVT.DESK_MAIN");
            prmlexema.prms << PrmSmpl<std::string>("", istation->name);
            prmenum.prms << prmlexema;
          }
          else
            prmenum.prms << PrmSmpl<std::string>("", istation->name);

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
   	  check_DesksGates( point_id );
    }
  }
}
//////////////////////////////TPointDests//////////////////////////////////////
void TPointDests::Load( int move_id, BitSet<TUseDestData> FUseData )
{
  items.clear();
  TQuery Qry(&OraSession);
  Qry.SQLText =
    "SELECT point_id,point_num,airp,pr_tranzit,first_point,airline,flt_no,suffix,craft,"
    "       bort,scd_in,est_in,act_in,scd_out,est_out,act_out,trip_type,litera,"
    "       park_in,park_out,remark,pr_reg,pr_del,tid,airp_fmt,airline_fmt,"
    "       suffix_fmt,craft_fmt"
    " FROM points WHERE move_id=:move_id AND pr_del != -1 "
    "ORDER By point_num";
  Qry.CreateVariable( "move_id", otInteger, move_id );
  Qry.Execute();
  while ( !Qry.Eof ) {
    TPointsDest dest;
    dest.getDestData( Qry );
    dest.UseData = FUseData;
    dest.LoadProps( dest.point_id, FUseData );
    items.push_back( dest );
    Qry.Next();
  }
}


void TPointDests::sychDests( TPointDests &new_dests, bool pr_change_dests, sychDestsType sychType )
{
  ProgTrace( TRACE5, "TPointDests::sychDests: items.size()=%zu, new_dests.items.size()=%zu",
             items.size(), new_dests.items.size() );
  std::vector<TPointsDest>::iterator prior_find_dest = new_dests.items.begin();
  for ( std::vector<TPointsDest>::iterator i=items.begin(); i!=items.end(); i++ ) {
    std::vector<TPointsDest>::iterator j=prior_find_dest;
    for ( ; j!=new_dests.items.end(); j++ ) {
        ProgTrace( TRACE5, "i->point_id=%d, i->airline=%s, i->flt_no=%d, i->airp=%s, j->point_id=%d, j->airline=%s, j->flt_no=%d, j->airp=%s",
                   i->point_id, i->airline.c_str(), i->flt_no, i->airp.c_str(), j->point_id, j->airline.c_str(), j->flt_no, j->airp.c_str() );
      if ( j->point_id == ASTRA::NoExists &&
           i->airline == j->airline &&
           i->flt_no == j->flt_no &&
           i->suffix == j->suffix &&
           i->airp == j->airp ) {
        ProgTrace( TRACE5, "i->point_id=%d, i->scd_out=%f, j->scd_out=%f, i->scd_in=%f, j->scd_in=%f",
                   i->point_id, i->scd_out, j->scd_out, i->scd_in, j->scd_in );
        bool pr_compare = false;
        switch ( sychType ) {
          case dtSomeLocalSCD: //сравнение дат надо делать в локальных временах
          {string region = AirpTZRegion( j->airp, true );
            TDateTime locale_scd_out1 = i->scd_out, locale_scd_out2 = j->scd_out,
                      locale_scd_in1 = i->scd_in, locale_scd_in2 = j->scd_in;
            if ( locale_scd_out1 != NoExists )
              locale_scd_out1 = UTCToLocal( locale_scd_out1, region );
            if ( locale_scd_out2 != NoExists )
              locale_scd_out2 = UTCToLocal( locale_scd_out2, region );
            if ( locale_scd_in1 != NoExists )
              locale_scd_in1 = UTCToLocal( locale_scd_in1, region );
            if ( locale_scd_in2 != NoExists )
              locale_scd_in2 = UTCToLocal( locale_scd_in2, region );
            double d1, d2, d3, d4;
            modf( locale_scd_out1, &d1 );
            modf( locale_scd_out2, &d2 );
            modf( locale_scd_in1, &d3 );
            modf( locale_scd_in2, &d4 );
            pr_compare = ( ( d1 == d2 || d2 == ASTRA::NoExists ) &&
                           ( d3 == d4 || d4 == ASTRA::NoExists ) );}
            break;
          case dtAllLocalSCD: //сравнение дат надо делать в локальных временах
          {string region = AirpTZRegion( j->airp, true );
            TDateTime locale_scd_out1 = i->scd_out, locale_scd_out2 = j->scd_out,
                      locale_scd_in1 = i->scd_in, locale_scd_in2 = j->scd_in;
            if ( locale_scd_out1 != NoExists )
              locale_scd_out1 = UTCToLocal( locale_scd_out1, region );
            if ( locale_scd_out2 != NoExists )
              locale_scd_out2 = UTCToLocal( locale_scd_out2, region );
            if ( locale_scd_in1 != NoExists )
              locale_scd_in1 = UTCToLocal( locale_scd_in1, region );
            if ( locale_scd_in2 != NoExists )
              locale_scd_in2 = UTCToLocal( locale_scd_in2, region );
            double d1, d2, d3, d4;
            modf( locale_scd_out1, &d1 );
            modf( locale_scd_out2, &d2 );
            modf( locale_scd_in1, &d3 );
            modf( locale_scd_in2, &d4 );
            pr_compare = ( d1 == d2 && d3 == d4 );}
            break;
          case dtAllSCD:
            pr_compare = ( i->scd_in == j->scd_in && i->scd_out == j->scd_out );
            break;
        }
        if ( pr_compare ) {
          break;
        }
      }
    }
    if ( j == new_dests.items.end() ) {
      tst();
      if ( pr_change_dests ) {
        i->pr_del = -1;
        ProgTrace( TRACE5, "delete i->point_id=%d", i->point_id );
      }
    }
    else {
      prior_find_dest = j + 1;
      j->point_id = i->point_id;
      ProgTrace( TRACE5, "i->point_id=%d", i->point_id );
    }
  }
  if ( !pr_change_dests )
    return;
  tst();
  prior_find_dest = items.begin();
  for ( std::vector<TPointsDest>::iterator i=new_dests.items.begin(); i!=new_dests.items.end(); i++ ) {
    if ( i->point_id == NoExists ) {
      prior_find_dest = items.insert( prior_find_dest, *i );
      ProgTrace( TRACE5, "insert i->key=%s", i->key.c_str() );
      prior_find_dest++;
    }
    else
     for ( std::vector<TPointsDest>::iterator j=prior_find_dest; j!=items.end(); j++ ) {
       if ( j->pr_del == -1 )
         continue;
       if ( j->point_id == i->point_id ) {
         prior_find_dest = j + 1;
         break;
       }
     }
  }
  ProgTrace( TRACE5, "items.size()=%zu", items.size() );
}


void FlightPoints::Get( int vpoint_dep )
{
  clear();
	TQuery Qry(&OraSession);
	Qry.SQLText =
	  "SELECT point_num,first_point,pr_tranzit,airp,pr_del "
    " FROM points "
    " WHERE point_id=:point_id AND pr_del!=-1";
  Qry.CreateVariable( "point_id", otInteger, vpoint_dep );
  Qry.Execute();
  if ( Qry.Eof )
    return;
  TTripRouteItem routeItem;
  routeItem.point_id = vpoint_dep;
  routeItem.point_num = Qry.FieldAsInteger( "point_num" );
  routeItem.airp = Qry.FieldAsString( "airp" );
  routeItem.pr_cancel = Qry.FieldAsInteger( "pr_del" );
  push_back( routeItem );
  
  int first_point = Qry.FieldIsNULL("first_point")?NoExists:Qry.FieldAsInteger("first_point");
  bool pr_tranzit = Qry.FieldAsInteger( "pr_tranzit" ) != 0;
  TTripRoute routes;
  if ( pr_tranzit ) {
    routes.GetRouteBefore( ASTRA::NoExists,
                           routeItem.point_id,
                           routeItem.point_num,
                           first_point,
                           pr_tranzit,
                           trtNotCurrent,
                           trtWithCancelled );
    if ( !routes.empty() ) {
      tst();
      insert( begin(), routes.begin(), routes.end() );
    }
  }
  routes.clear();
  routes.GetRouteAfter( ASTRA::NoExists,
                        routeItem.point_id,
                        routeItem.point_num,
                        first_point,
                        pr_tranzit,
                        trtNotCurrent,
                        trtWithCancelled );
  if ( !routes.empty() ) {
    insert( end(), routes.begin(), routes.end() );
  }
  if ( !empty() ) {
    point_dep = begin()->point_id;
    point_arv = rbegin()->point_id;
  }
  ProgTrace( TRACE5, "FlightPoints::Get(%d): point_dep=%d, point_arv=%d",
             vpoint_dep, point_dep, point_arv );
};


void TFlights::Get( const std::vector<int> &points, TFlightType flightType )
{
  std::vector<int> pnts;
  if ( flightType == ftAll ) {
    TQuery Qry( &OraSession );
    Qry.SQLText =
    "SELECT p2.point_id FROM points p1, points p2 "
    " WHERE p1.point_id=:point_id AND "
    "       p2.move_id=p1.move_id AND "
    "       p2.pr_del !=-1";
    Qry.DeclareVariable( "point_id", otInteger );
    for ( std::vector<int>::const_iterator ipoint=points.begin();
          ipoint!=points.end(); ipoint++ ) {
      Qry.SetVariable( "point_id", *ipoint );
      ProgTrace( TRACE5, "TFlights::Get: SELECT point_id=%d", *ipoint );
      Qry.Execute();
      for ( ; !Qry.Eof; Qry.Next() ) {
        pnts.push_back( Qry.FieldAsInteger( "point_id" ) );
      }
    }
  }
  else {
    pnts = points;
  }
  sort(pnts.begin(),pnts.end());
  for ( std::vector<int>::iterator ipoint=pnts.begin(); //пробег по point_id
        ipoint!=pnts.end(); ipoint++ ) {
    FlightPoints fp;
    fp.Get( *ipoint ); //получаем рейс по point_id
    if ( fp.empty() ) { //пусто
      continue;
    }
    for( TFlights::iterator iflight=begin();
         iflight!=end(); iflight++ ) {
      if ( fp.point_dep == iflight->point_dep &&
           fp.point_arv == iflight->point_arv ) { //уже есть такой рейс
        fp.clear();
        break;
      }
    }
    if ( fp.empty() ) {
      continue;
    }
    push_back( fp ); //добавляем рейс
    for ( FlightPoints::iterator ipoint=fp.begin();
          ipoint!=fp.end(); ipoint++ ) {
      ProgTrace( TRACE5, "TFlights::Get: PUT point_id=%d", ipoint->point_id );
    }
  }
}

void TFlights::Lock()
{
  set<int,std::less<int> > points;
  TQuery Qry( &OraSession );
  Qry.SQLText =
    "SELECT point_id FROM points WHERE point_id=:point_id FOR UPDATE";
  Qry.DeclareVariable( "point_id", otInteger );
  for( TFlights::iterator iflight=begin();
       iflight!=end(); iflight++ ) {
    for ( FlightPoints::iterator ipoint=iflight->begin();
          ipoint!=iflight->end(); ipoint++ ) {
      if ( points.find( ipoint->point_id ) != points.end() ) {
        continue;
      }
      points.insert( ipoint->point_id );
    }
  }
  for ( set<int>::iterator ipoint=points.begin(); ipoint!=points.end(); ipoint++ ) {
    Qry.SetVariable( "point_id", *ipoint );
    Qry.Execute();
    ProgTrace( TRACE5, "TLockedFlights::Lock(%d)", *ipoint );
  }
}

