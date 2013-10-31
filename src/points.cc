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
  tst();
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


  if ( UseData.isFlag( udStages ) )
    stages.Load( point_id );
  if ( UseData.isFlag( udDelays ) ) {
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
      if ( !craft.empty() && vdest.craft.empty() )
        events.setFlag( dmSetCraft );
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
       !events.isFlag( dmChangeCraft ) ) {
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
  if ( isSetOtherTime( status, est_out, vdest.est_out ) )
    events.setFlag( dmSetESTOUT );
  if ( isChangeTime( status, est_out, vdest.est_out ) )
    events.setFlag( dmChangeESTOUT );
  if ( isDeleteTime( status, est_out, vdest.est_out ) )
    events.setFlag( dmDeleteESTOUT );
  //est in
  if ( isSetOtherTime( status, est_in, vdest.est_in ) )
    events.setFlag( dmSetESTIN );
  if ( isChangeTime( status, est_in, vdest.est_in ) )
    events.setFlag( dmChangeESTIN );
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
  if ( status == tdInsert || point_num != vdest.point_num )
    events.setFlag( dmPoint_Num );
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
    string msg;
    if ( status == tdInsert )
      msg = "Ввод нового пункта ";
    if ( status == tdDelete )
      msg = "Удаление пункта ";
    if ( events.isFlag( dmSetCancel ) )
      msg = "Отмена пункта ";
    if ( events.isFlag( dmSetUnCancel ) )
      msg = "Возврат пункта ";
    if ( flt_no != NoExists )
      reqInfo->MsgToLog( msg + airline + IntToString(flt_no) + suffix + " " + airp, evtDisp, move_id, point_id );
    else
      reqInfo->MsgToLog( msg + airp, evtDisp, move_id, point_id );
  }
  if ( events.isFlag( dmChangeAirline ) ||
       events.isFlag( dmChangeFltNo ) ||
       events.isFlag( dmChangeSuffix ) ) {
    if ( dest.flt_no != NoExists ) {
      reqInfo->MsgToLog( string( "Изменение атрибутов рейса с " ) + dest.airline + IntToString(dest.flt_no) + dest.suffix +
                         " на " + airline + IntToString(flt_no) + suffix + " порт " + airp, evtDisp, move_id, point_id );
    }
    else {
      reqInfo->MsgToLog( string( "Изменение атрибутов рейса на ") + airline + IntToString(flt_no) + suffix + " порт " + airp, evtDisp, move_id, point_id );
    }
  }
  if ( events.isFlag( dmSetSCDOUT ) )
    reqInfo->MsgToLog( string( "Задано плановое время вылета " ) + DateTimeToStr( scd_out, "hh:nn dd.mm.yy (UTC)" ) + " порт " + airp, evtDisp, move_id, point_id );
  if ( events.isFlag( dmChangeSCDOUT ) )
    reqInfo->MsgToLog( string( "Изменено плановое время вылета " ) + DateTimeToStr( scd_out, "hh:nn dd.mm.yy (UTC)" ) + " порт " + airp, evtDisp, move_id, point_id );
  if ( events.isFlag( dmDeleteSCDOUT ) )
    reqInfo->MsgToLog( string( "Удалено плановое время вылета " ) + DateTimeToStr( dest.scd_out, "hh:nn dd.mm.yy (UTC)" ) + " порт " + airp, evtDisp, move_id, point_id );
  if ( events.isFlag( dmSetESTOUT ) )
    reqInfo->MsgToLog( string( "Задано расчетное время вылета " ) + DateTimeToStr( est_out, "hh:nn dd.mm.yy (UTC)" ) + " порт " + airp, evtDisp, move_id, point_id );
  if ( events.isFlag( dmChangeESTOUT ) )
    reqInfo->MsgToLog( string( "Изменено расчетное время вылета " ) + DateTimeToStr( est_out, "hh:nn dd.mm.yy (UTC)" ) + " порт " + airp, evtDisp, move_id, point_id );
  if ( events.isFlag( dmDeleteESTOUT ) )
    reqInfo->MsgToLog( string( "Удалено расчетное время вылета " ) + DateTimeToStr( dest.est_out, "hh:nn dd.mm.yy (UTC)" ) + " порт " + airp, evtDisp, move_id, point_id );
  if ( events.isFlag( dmSetACTOUT ) )
    reqInfo->MsgToLog( string( "Задано фактическое время вылета " ) + DateTimeToStr( act_out, "hh:nn dd.mm.yy (UTC)" ) + " порт " + airp, evtDisp, move_id, point_id );
  if ( events.isFlag( dmChangeACTOUT ) )
    reqInfo->MsgToLog( string( "Изменено фактическое время вылета " ) + DateTimeToStr( act_out, "hh:nn dd.mm.yy (UTC)" ) + " порт " + airp, evtDisp, move_id, point_id );
  if ( events.isFlag( dmDeleteACTOUT ) )
    reqInfo->MsgToLog( string( "Удалено фактическое время вылета " ) + DateTimeToStr( dest.act_out, "hh:nn dd.mm.yy (UTC)" ) + " порт " + airp, evtDisp, move_id, point_id );
    
  if ( events.isFlag( dmSetSCDIN ) )
    reqInfo->MsgToLog( string( "Задано плановое время прилета " ) + DateTimeToStr( scd_in, "hh:nn dd.mm.yy (UTC)" ) + " порт " + airp, evtDisp, move_id, point_id );
  if ( events.isFlag( dmChangeSCDIN ) )
    reqInfo->MsgToLog( string( "Изменено плановое время прилета " ) + DateTimeToStr( scd_in, "hh:nn dd.mm.yy (UTC)" ) + " порт " + airp, evtDisp, move_id, point_id );
  if ( events.isFlag( dmDeleteSCDIN ) )
    reqInfo->MsgToLog( string( "Удалено плановое время прилета " ) + DateTimeToStr( dest.scd_in, "hh:nn dd.mm.yy (UTC)" ) + " порт " + airp, evtDisp, move_id, point_id );
  if ( events.isFlag( dmSetESTIN ) )
    reqInfo->MsgToLog( string( "Задано расчетное время прилета " ) + DateTimeToStr( est_in, "hh:nn dd.mm.yy (UTC)" ) + " порт " + airp, evtDisp, move_id, point_id );
  if ( events.isFlag( dmChangeESTIN ) )
    reqInfo->MsgToLog( string( "Изменено расчетное время прилета " ) + DateTimeToStr( est_in, "hh:nn dd.mm.yy (UTC)" ) + " порт " + airp, evtDisp, move_id, point_id );
  if ( events.isFlag( dmDeleteESTIN ) )
    reqInfo->MsgToLog( string( "Удалено расчетное время прилета " ) + DateTimeToStr( dest.est_in, "hh:nn dd.mm.yy (UTC)" ) + " порт " + airp, evtDisp, move_id, point_id );
  if ( events.isFlag( dmSetACTIN ) )
    reqInfo->MsgToLog( string( "Задано фактическое время прилета " ) + DateTimeToStr( act_in, "hh:nn dd.mm.yy (UTC)" ) + " порт " + airp, evtDisp, move_id, point_id );
  if ( events.isFlag( dmChangeACTIN ) )
    reqInfo->MsgToLog( string( "Изменено фактическое время прилета " ) + DateTimeToStr( act_in, "hh:nn dd.mm.yy (UTC)" ) + " порт " + airp, evtDisp, move_id, point_id );
  if ( events.isFlag( dmDeleteACTIN ) )
    reqInfo->MsgToLog( string( "Удалено фактическое время прилета " ) + DateTimeToStr( dest.act_in, "hh:nn dd.mm.yy (UTC)" ) + " порт " + airp, evtDisp, move_id, point_id );

  if ( events.isFlag( dmChangeStageESTTime ) ) {
    TDateTime diff = stage_est - stage_scd;
  	double f;
  	string msg;
    if ( diff < 0 ) {
  	  modf( diff, &f );
  		msg = "Опережение выполнения технологического графика на ";
  		if ( f )
    	  msg += IntToString( (int)f ) + " ";
    	msg += DateTimeToStr( fabs(diff), "hh:nn" );
  	}
  	if ( diff >= 0 ) {
  	  modf( diff, &f );
  	  if ( diff ) {
  		  msg = "Задержка выполнения технологического графика на ";
  		  if ( f )
  		   	msg += IntToString( (int)f ) + " ";
  		  msg += DateTimeToStr( diff, "hh:nn" );
  		}
  		else
  		  msg = "Отмена задержка выполнения технологического графика";
    }
	  reqInfo->MsgToLog( msg + " порт " + airp, evtFlt, point_id );
  }

  if ( events.isFlag( dmChangeTripType ) )
    reqInfo->MsgToLog( string( "Изменение типа рейса с '" ) + dest.trip_type + "' на '" + trip_type + "' порт " + airp, evtDisp, move_id, point_id );
  if ( events.isFlag( dmChangeLitera ) )
    reqInfo->MsgToLog( string( "Изменение литеры рейса с '" ) + dest.litera + "' на '" + litera + "' порт " + airp, evtDisp, move_id, point_id );
  if ( events.isFlag( dmReg ) )
    reqInfo->MsgToLog( string( "Изменение признака регистрации с '" ) + IntToString(dest.pr_reg) + "' на '" + IntToString(pr_reg) + "' порт " + airp, evtDisp, move_id, point_id );
  if ( events.isFlag( dmChangeCraft ) )
    reqInfo->MsgToLog( string( "Изменение типа ВС на " ) + craft + " порт " + airp, evtDisp, move_id, point_id );
  if ( events.isFlag( dmSetCraft ) )
		reqInfo->MsgToLog( string( "Назначение типа ВС " ) + craft + " порт " + airp, evtDisp, move_id, point_id );
  if ( events.isFlag( dmChangeBort ) )
    reqInfo->MsgToLog( string( "Изменение борта на " ) + bort + " порт " + airp, evtDisp, move_id, point_id );
  if ( events.isFlag( dmSetBort ) )
    reqInfo->MsgToLog( string( "Назначение борта " ) + bort + " порт " + airp, evtDisp, move_id, point_id );
  if ( status != tdInsert &&
       ( events.isFlag( dmSetACTOUT ) ||
         events.isFlag( dmDeleteACTOUT ) ||
         events.isFlag( dmSetCancel ) ||
         events.isFlag( dmSetUnCancel ) ||
         events.isFlag( dmSetDelete ) ) ) {
    SetTripStages_IgnoreAuto( point_id, act_out != NoExists || pr_del != 0 );
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
  if ( dest.events.emptyFlags() )  //нет изменений
    return;
  ProgTrace( TRACE5, "WriteDest" );
  TQuery Qry(&OraSession);
  Qry.SQLText =
    "SELECT cycle_tid__seq.nextval n FROM dual ";
 	Qry.Execute();
 	dest.tid = Qry.FieldAsInteger( "n" );
 	if ( dest.status == tdInsert ) {
 	  tst();
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
  ProgTrace( TRACE5, "desk.airp=%s, point_id=%d, point_num=%d",
             dest.airp.c_str(), dest.point_id, dest.point_num );
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
	ProgTrace( TRACE5, "dest.bort=%s", dest.bort.c_str() );
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
  else
  	Qry.CreateVariable( "est_out", otDate, dest.est_out );
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
    TReqInfo::Instance()->MsgToLog( dest.airline+IntToString(dest.flt_no)+dest.suffix +
                                    " " +dest.airp + ": Была вызвана процедура назначения шагов тех графика на рейс ",
                                    evtDisp, move_id, dest.point_id );
  	Qry.Clear();
  	Qry.SQLText =
      "BEGIN "
      " sopp.set_flight_sets(:point_id,:use_seances);"
      "END;";
 		Qry.CreateVariable( "point_id", otInteger, dest.point_id );
 		Qry.CreateVariable( "use_seances", otInteger, (int)USE_SEANCES() );
 		Qry.Execute();
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
}

string DecodeEvents( TTripEvents event )
{
  string res;
  switch ( (int)event ) {
    case teNewLand:
      res = "teNewLand: Добавление прилет";
      break;
    case teNewTakeoff:
      res = "teNewTakeoff: Добавление вылет";
      break;
    case teDeleteLand:
      res = "teDeleteLand: Удаление прилета";
      break;
    case teDeleteTakeoff:
      res = "teDeleteTakeoff: Удаление вылета";
      break;
    case teSetCancelLand:
      res = "teSetCancelLand: Отмена прилета";
      break;
    case teSetCancelTakeoff:
      res = "teSetCancelTakeoff: Отмена вылета";
      break;
    case teSetUnCancelLand:
      res = "teSetUnCancelLand: Возврат прилета";
      break;
    case teSetUnCancelTakeoff:
      res = "teSetUnCancelTakeoff: Возврат вылета";
      break;
    case teSetSCDIN:
      res = "teSetSCDIN: Ввод планового времени прилета";
      break;
    case teChangeSCDIN:
      res = "teChangeSCDIN: Изменение планового времени прилета";
      break;
    case teDeleteSCDIN:
      res = "teDeleteSCDIN: Удаление планового времени прилета";
      break;
    case teSetESTIN:
      res = "teSetESTIN: Ввод расчетного времени прилета";
      break;
    case teChangeESTIN:
      res = "teChangeESTIN: Изменение расчетного времени прилета";
      break;
    case teDeleteESTIN:
      res = "teDeleteESTIN: Удаление расчетного времени прилета";
      break;
    case teSetACTIN:
      res = "teSetACTIN: Ввод фактического времени прилета";
      break;
    case teChangeACTIN:
      res = "teChangeACTIN: Изменение фактического времени прилета";
      break;
    case teDeleteACTIN:
      res = "teDeleteACTIN: Удаление фактического времени прилета";
      break;
    case teChangeParkIn:
      res = "teChangeParkIn: Изменение стоянки на прилет";
      break;
    case teChangeCraftLand:
      res = "teChangeCraftLand: Изменение типа ВС на прилет";
      break;
    case teSetCraftLand:
      res = "teSetCraftLand: Ввод типа ВС на прилет";
      break;
    case teChangeBortLand:
      res = "teChangeBortLand: Изменение борта ВС на прилет";
      break;
    case teSetBortLand:
      res = "teSetBortLand: Ввод борта ВС на прилет";
      break;
    case teChangeTripTypeLand:
      res = "teChangeTripTypeLand: Изменение типа рейса на прилет";
      break;
    case teChangeLiteraLand:
      res = "teChangeTripTypeLand: Изменение литеры рейса на прилет";
      break;
    case teChangeFlightAttrLand:
      res = "teChangeFlightAttrLand: Изменение аттрибутов рейса на прилет";
      break;
    case teInitStages:
      res = "teInitStages: Необходима инициализация шагов тех. графика";
      break;
    case teInitComps:
      res = "teInitComps: Необходим автоматический поиск компоновки";
      break;
    case teChangeStageESTTime:
      res = "teChangeStageESTTime: Изменения в расчетных временах тех. графика";
      break;
    case teSetSCDOUT:
      res = "teSetSCDOUT: Ввод планового времени вылета";
      break;
    case teChangeSCDOUT:
      res = "teChangeSCDOUT: Изменение планового времени вылета";
      break;
    case teDeleteSCDOUT:
      res = "teDeleteSCDOUT: Удаление планового времени вылета";
      break;
    case teSetESTOUT:
      res = "teSetESTOUT: Ввод расчетного времени вылета";
      break;
    case teChangeESTOUT:
      res = "teChangeESTOUT: Изменение расчетного времени вылета";
      break;
    case teDeleteESTOUT:
      res = "teDeleteESTOUT: Удаление расчетного времени вылета";
      break;
    case teSetACTOUT:
      res = "teSetACTOUT: Ввод фактического времени вылета";
      break;
    case teChangeACTOUT:
      res = "teChangeACTOUT: Изменение фактического времени вылета";
      break;
    case teDeleteACTOUT:
      res = "teDeleteACTOUT: Удаление фактического времени вылета";
      break;
    case teChangeCraftTakeoff:
      res = "teChangeCraftTakeoff: Изменение типа ВС на вылет";
      break;
    case teSetCraftTakeoff:
      res = "teSetCraftTakeoff: Ввод типа ВС на вылет";
      break;
    case teChangeBortTakeoff:
      res = "teChangeBortTakeoff: Изменение борта ВС на вылет";
      break;
    case teSetBortTakeoff:
      res = "teSetBortTakeoff: Ввод борта ВС на вылет";
      break;
    case teChangeLiteraTakeoff:
      res = "teChangeLiteraTakeoff: Изменение литеры рейс на вылет";
      break;
    case teChangeTripTypeTakeoff:
      res = "teChangeTripTypeTakeoff: Изменение типа рейс на вылет";
      break;
    case teChangeParkOut:
      res = "teChangeParkOut: Изменение стоянки рейс на вылет";
      break;
    case teChangeFlightAttrTakeoff:
      res = "teChangeFlightAttrTakeoff: Изменение аттрибутов рейс на вылет";
      break;
    case teChangeDelaysTakeoff:
      res = "teChangeDelaysTakeoff: Изменение задержки рейса на вылет";
      break;
    case teTranzitTakeoff:
      res = "teTranzitTakeoff: Изменение признака транзита рейса на вылет";
      break;
    case teRegTakeoff:
      res = "teRegTakeoff: Изменение признака регистрации рейса в порту на вылет";
      break;
    case teFirst_PointTakeoff:
      res = "teFirst_PointTakeoff: Изменение признака 'first_point' рейса на вылет";
      break;
    case teChangeRemarkTakeoff:
      res = "teChangeRemarkTakeoff: Изменение ремарки рейса на вылет";
      break;
    case tePoint_NumTakeoff:
      res = "teFirst_PointTakeoff: Изменение признака 'point_num' рейса на вылет";
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
      ProgTrace( TRACE5, "point_id=%d, event=%s", this->key.point_id, DecodeEvents( (TTripEvents)i ).c_str() );
      string msg = this->key.airline;
      if ( this->key.flt_no != NoExists )
        msg += IntToString(this->key.flt_no);
      msg += this->key.suffix + " " + this->key.airp + ": " + DecodeEvents( (TTripEvents)i );
      TReqInfo::Instance()->MsgToLog( msg, evtDisp, move_id, this->key.point_id );
    }
  }
  
  
  if ( this->events.isFlag( teInitComps ) ) {
    TReqInfo::Instance()->MsgToLog( "Была вызвана процедура автоматического назначения компоновки на рейс", evtDisp, move_id, this->key.point_id );
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
        TReqInfo::Instance()->MsgToLog( "Была вызвана процедура формирования телеграммы MVTB", evtDisp, move_id, this->key.point_id );
      };
    }
    catch(std::exception &E) {
      ProgError(STDLOG,"SetACTIN||ChangeACTIN.SendTlg (point_id=%d): %s",this->key.point_id,E.what());
    };
  }
  if ( this->events.isFlag( teSetACTOUT ) ) {
    try {
      exec_stage( this->key.point_id, sTakeoff );
      TReqInfo::Instance()->MsgToLog( "Была вызвана процедура выполнения шага тех графика 'Вылет'", evtDisp, move_id, this->key.point_id );
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
      TReqInfo::Instance()->MsgToLog( "Была вызвана процедура формирования телеграммы MVTA", evtDisp, move_id, this->key.point_id );
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
      TReqInfo::Instance()->MsgToLog( "Была вызвана процедура удаления признака эл. билета", evtDisp, move_id, this->key.point_id );
    }
    catch(std::exception &E) {
      ProgError(STDLOG,"teDeleteACTOUT.ETStatus (point_id=%d): %s",this->key.point_id,E.what());
    };
  }
  if ( this->events.isFlag( teSetSCDOUT ) ||
       this->events.isFlag( teChangeSCDOUT ) ||
       this->events.isFlag( teDeleteSCDOUT ) ||
       this->events.isFlag( teChangeCraftTakeoff ) ||
       this->events.isFlag( teSetCraftTakeoff ) ||
       this->events.isFlag( teChangeBortTakeoff ) ||
       this->events.isFlag( teSetBortTakeoff ) ||
       this->events.isFlag( teChangeFlightAttrTakeoff ) ) {
     TPersWeights persWeights; // некрасиво, т.к. каждый раз начитка
     PersWeightRules weights;
     persWeights.getRules( this->key.point_id, weights );
     weights.write( this->key.point_id );
  }
       
  //отвязка PNL-телеграмм
  if ( this->events.isFlag( teNeedUnBindTlgs ) ) {
    vector<int> point_ids;
    point_ids.push_back( this->key.point_id );
    ProgTrace( TRACE5, "teNeedUnBindTlgs: key->point_id=%d", this->key.point_id );
    try {
    //!!!
    }
    catch(std::exception &E) {
      ProgError(STDLOG,"UnBindTlg: point_id=%d, %s",this->key.point_id,E.what());
    };
    tst();
    if ( this->key.flt_no != NoExists )
      TReqInfo::Instance()->MsgToLog( this->key.airline+IntToString(this->key.flt_no)+this->key.suffix+
                                      " "+this->key.airp+": Была вызвана процедура отвязки PNL", evtDisp, move_id, this->key.point_id );
    else
      TReqInfo::Instance()->MsgToLog( " "+this->key.airp+": Была вызвана процедура отвязки PNL", evtDisp, move_id, this->key.point_id );
    tst();
  }
  tst();
  if ( this->events.isFlag( teNeedBindTlgs ) ) {
    vector<TTripInfo> flts;
    TTripInfo tripInfo;
    tripInfo.airline = this->key.airline;
    tripInfo.flt_no = this->key.flt_no;
    tripInfo.suffix = this->key.suffix;
    tripInfo.airp = this->key.airp;
    tripInfo.scd_out = this->key.scd_out;
    flts.push_back( tripInfo );
    ProgTrace( TRACE5, "teNeedBindTlgs: key->point_id=%d, airp=%s, trip=%s",
               this->key.point_id,
               tripInfo.airp.c_str(),
               string(tripInfo.airline + IntToString(tripInfo.flt_no) + tripInfo.suffix + DateTimeToStr( tripInfo.scd_out, "hh:nn dd.mm.yy (UTC)" )).c_str() );
    try {
    //!!!
    }
    catch(std::exception &E) {
      ProgError(STDLOG,"BindTlg: point_id=%d, %s",this->key.point_id,E.what());
    };
    if ( tripInfo.flt_no != NoExists )
      TReqInfo::Instance()->MsgToLog( tripInfo.airline+IntToString(tripInfo.flt_no)+tripInfo.suffix+
                                      " "+tripInfo.airp+": Была вызвана процедура привязки PNL", evtDisp, move_id, this->key.point_id );
    else
      TReqInfo::Instance()->MsgToLog( " "+tripInfo.airp+": Была вызвана процедура привязки PNL", evtDisp, move_id, this->key.point_id );
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
  tst();
  if ( this->events.isFlag( teChangeCargos ) ) {
    this->key.cargos.Save( this->key.point_id, this->dests );
  }
  if ( this->events.isFlag( teChangeMaxCommerce ) ) {
    tst();
    this->key.max_commerce.Save( this->key.point_id );
  }
  if ( this->events.isFlag( teChangeCargos ) || this->events.isFlag( teChangeMaxCommerce ) ) {
    check_overload_alarm( this->key.point_id );
  }
  if ( this->events.isFlag( teChangeStations ) ) {
    this->key.stations.Save( this->key.point_id );
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
      ProgTrace( TRACE5, "point_dep=%d, point_arv=%d", i->point_id, j->point_id );
      Qry.Execute();
      if ( !Qry.Eof ) {
        tst();
        return true;
      }
    }
  }
  return false;
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
  //проверка на то, что пункт можно отменить или удалить
  map<int,TPointsDest> olddests;
  for( vector<TPointsDest>::iterator id=dests.items.begin(); id!=dests.items.end(); id++ ) {
    ProgTrace( TRACE5, "id->airp=%s, id->pr_del=%d", id->airp.c_str(), id->pr_del );
    if ( id->status != tdInsert ) {
      olddests[ id->point_id ].Load( id->point_id, id->UseData );
    }
    id->getEvents( olddests[ id->point_id ] );
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
    ProgTrace( TRACE5, "id->airp=%s, id->pr_del=%d", id->airp.c_str(), id->pr_del );
    if ( id->events.isFlag( dmTranzit ) && !id->pr_tranzit ) {
      if ( existsTranzitPassengers( id->point_id ) )
        throw AstraLocale::UserException( "MSG.ROUTE.UNABLE_CHANGE_PR_TRANZIT",
                                          LParams() << LParam("airp", ElemIdToCodeNative(etAirp,id->airp)));
    }
    ProgTrace( TRACE5, "id->airp=%s, id->pr_del=%d", id->airp.c_str(), id->pr_del );
    id->setRemark( olddests[ id->point_id ] );
    ProgTrace( TRACE5, "id->airp=%s, id->pr_del=%d", id->airp.c_str(), id->pr_del );
  }
  //сохранение маршрута
  tst();
  for( vector<TPointsDest>::iterator id=dests.items.begin(); id!=dests.items.end(); id++ ) {
    ProgTrace( TRACE5, "id->point_id=%d,point_id->num=%d, id->airp=%s, id->bort=%s",
                id->point_id, id->point_num, id->airp.c_str(), id->bort.c_str() );
    WriteDest( *id );
  }
  string msg;
  for( vector<TPointsDest>::iterator id=dests.items.begin(); id!=dests.items.end(); id++ ) {
    id->DoEvents( move_id, olddests[ id->point_id ] );
    if ( events.isFlag( pePointNum ) && id->status != tdDelete ) {
      if ( msg.empty() )
        if ( status == peInsert )
          msg = "Ввод нового рейса: ";
        else
          msg = "Изменение маршрута рейса: ";
      else
        msg += "-";
      if ( id->flt_no != NoExists )
        msg += id->airline + IntToString(id->flt_no) + id->suffix + " " + id->airp;
      else
        msg += id->airp;
    }
  }
  if ( !msg.empty() )
    TReqInfo::Instance()->MsgToLog( msg, evtDisp, move_id );

  //получение событий по рейсам, которые есть или были в маршруте
  vector<PointsKeyTrip<TPointsDest> > keytrips1, keytrips2;
  // создаем все возможные рейсы из нового маршрута исключая удаленные пункты
  getKeyTrips( dests.items, keytrips1 );
  std::vector<TPointsDest> priorDests;
  for ( map<int,TPointsDest>::iterator i=olddests.begin(); i!=olddests.end(); i++ ) {
    if ( i->second.point_id == NoExists )
      continue;
    ProgTrace( TRACE5, "priorDests.push_back point_id=%d, i->first=%d", i->second.point_id, i->first );
    priorDests.push_back( i->second );
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
  ProgTrace( TRACE5, "TPoints::isDouble: move_id=%d, airline=%s, flt_no=%d, suffix=%s, airp=%s, scd_in=%s, scd_out=%s",
             move_id, airline.c_str(), flt_no, suffix.c_str(), airp.c_str(),
             DateTimeToStr( scd_in, "hh:nn dd.mm.yy" ).c_str(),
             DateTimeToStr( scd_out, "hh:nn dd.mm.yy" ).c_str() );
  TElemFmt fmt;
  airp = ElemToElemId( etAirp, airp, fmt );
  suffix = ElemToElemId( etSuffix, suffix, fmt );
  airline = ElemToElemId( etAirline, airline, fmt );
  double local_scd_in,local_scd_out,d1;
  TBaseTable &baseairps = base_tables.get( "airps" );
  string region = CityTZRegion( ((TAirpsRow&)baseairps.get_row( "code", airp, true )).city );
  if ( scd_in >NoExists ) {
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
    "SELECT scd_in, scd_out, move_id, point_id FROM points "
    " WHERE airline=:airline AND flt_no=:flt_no AND NVL(suffix,' ')=NVL(:suffix,' ') AND "
    "       move_id!=:move_id AND airp=:airp AND pr_del!=-1 AND "
    "       ( scd_in BETWEEN :scd_in-2 AND :scd_in+2 OR "
    "         scd_out BETWEEN :scd_out-2 AND :scd_out+2 )";
  Qry.CreateVariable( "move_id", otInteger, move_id );
  Qry.CreateVariable( "airp", otString, airp );
  Qry.CreateVariable( "airline", otString,airline );
  Qry.CreateVariable( "flt_no", otInteger, flt_no );
  Qry.CreateVariable( "suffix", otString, suffix );
	if ( scd_in > NoExists )
	  Qry.CreateVariable( "scd_in", otDate, scd_in );
	else
		Qry.CreateVariable( "scd_in", otDate, FNull );
	if ( scd_out > NoExists )
		Qry.CreateVariable( "scd_out", otDate, scd_out );
	else
		Qry.CreateVariable( "scd_out", otDate, FNull );
	Qry.Execute();
  while ( !Qry.Eof ) {
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
      TReqInfo::Instance()->MsgToLog(
          	string( "Направление " ) + icargo->airp_arv + ": " +
            "груз " + IntToString( icargo->cargo ) + " кг., " +
            "почта " + IntToString( icargo->mail ) + " кг.", evtFlt, point_id );
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
      TReqInfo::Instance()->MsgToLog(
          	string( "Направление " ) + jcargo->airp_arv + ": " +
            "груз " + IntToString( 0 ) + " кг., " +
            "почта " + IntToString( 0 ) + " кг.", evtFlt, point_id );
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
 	  TReqInfo::Instance()->MsgToLog( string( "Макс. коммерческая загрузка: " ) + "не задано", evtFlt, point_id );
 	else
    TReqInfo::Instance()->MsgToLog( string( "Макс. коммерческая загрузка: " ) + IntToString( value ) + "кг.", evtFlt, point_id );
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
      string tolog;
      for ( tstations::iterator istation=stations.begin(); istation!=stations.end(); istation++ ) {
        if ( istation->work_mode != work_mode )
          continue;
        if ( find( terms.begin(), terms.end(), istation->name ) == terms.end() ) {
          terms.push_back( istation->name );
        	Qry.SetVariable( "name", istation->name );
       		Qry.SetVariable( "pr_main", istation->pr_main );
          Qry.Execute();
          if ( !tolog.empty() )
          	tolog += ", ";
          tolog += istation->name;
          if ( istation->pr_main )
          	tolog += " (главная)";
        }
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
   	  check_DesksGates( point_id );
    }
  }
}
//////////////////////////////TPointDests//////////////////////////////////////
void TPointDests::Load( int move_id )
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
    dest.stages.Load( dest.point_id );
    items.push_back( dest );
    Qry.Next();
  }
}


void TPointDests::sychDests( TPointDests &new_dests, bool pr_change_dests, bool pr_compare_date )
{
  ProgTrace( TRACE5, "TPointDests::sychDests: items.size()=%zu, new_dests.items.size()=%zu",
             items.size(), new_dests.items.size() );
  std::vector<TPointsDest>::iterator prior_find_dest = new_dests.items.begin();
  for ( std::vector<TPointsDest>::iterator i=items.begin(); i!=items.end(); i++ ) {
    std::vector<TPointsDest>::iterator j=prior_find_dest;
    for ( ; j!=new_dests.items.end(); j++ ) {
      if ( i->airline == j->airline &&
           i->flt_no == j->flt_no &&
           i->suffix == j->suffix &&
           i->airp == j->airp ) {
        ProgTrace( TRACE5, "i->point_id=%d, i->scd_out=%f, j->scd_out=%f, i->scd_in=%f, j->scd_in=%f",
                   i->point_id, i->scd_out, j->scd_out, i->scd_in, j->scd_in );
        if ( pr_compare_date ) { //сравнение дат надо делать в локальных временах
          string region = AirpTZRegion( j->airp, true );
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
          if ( d1 == d2 &&
               d3 == d4 ) {
            break;
          }
        }
        else {
          if ( i->scd_in == j->scd_in &&
               i->scd_out == j->scd_out ) {
            break;
          }
        }
      }
    }
    if ( j == new_dests.items.end() ) {
      if ( pr_change_dests ) {
        i->pr_del = -1;
        tst();
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
      insert( end(), routes.begin(), routes.end() );
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

