/*
create table aodb_pax( point_id NUMBER(9), pax_id NUMBER(9), record VARCHAR2(2000) );
create table aodb_bag( pax_id NUMBER(9), num NUMBAR(3), record VARCHAR2(2000) );
INSERT INTO file_types(code,name,in_order) VALUES( "AODB", "AODB", 1 );
INSERT INTO file_param_sets
alter table aodb_bag add pr_cabin NUMBER(1) NOT NULL;
*/
#include "aodb.h"

#include <string>
#include <sstream>
#include <iomanip>
#include <vector>
#include <map>
#include <algorithm>
#include <sstream>

#include "exceptions.h"
#include "basic.h"
#include "stl_utils.h"
#include "base_tables.h"
#include "astra_consts.h"
#include "astra_utils.h"
#include "astra_service.h"
#include "misc.h"
#include "astra_misc.h"
#include "stages.h"
#include "tripinfo.h"
#include "salons2.h"
#include "serverlib/helpcpp.h"

#define NICKNAME "DJEK"
#define NICKTRACE DJEK_TRACE
#include "serverlib/slogger.h"

using namespace std;
using namespace EXCEPTIONS;
using namespace BASIC;
using namespace ASTRA;

struct AODB_STRUCT{
	int pax_id;
	int reg_no;
	int num;
	int pr_cabin;
	bool unaccomp;
	bool doit;
	string record;
};

struct AODB_Dest {
	int num; //1
	std::string airp; //3
	int pr_del; //1
};

struct AODB_Term {
	std::string type; //1
	std::string name; //4
	int pr_del; //1
};

const
  unsigned int REC_NO_IDX = 0;
  unsigned int REC_NO_LEN = 6;
  unsigned int FLT_ID_IDX = 6;
  unsigned int FLT_ID_LEN = 10;
  unsigned int TRIP_IDX = 16;
  unsigned int TRIP_LEN = 10;
  unsigned int LITERA_IDX = 26;
  unsigned int LITERA_LEN = 3;
  unsigned int SCD_IDX = 29;
  unsigned int SCD_LEN = 16;
  unsigned int EST_IDX = 45;
  unsigned int EST_LEN = 16;
  unsigned int ACT_IDX = 61;
  unsigned int ACT_LEN = 16;
  unsigned int HALL_IDX = 77;
  unsigned int HALL_LEN = 2;
  unsigned int PARK_OUT_IDX = 79;
  unsigned int PARK_OUT_LEN = 5;
  unsigned int KRM_IDX = 84;
  unsigned int KRM_LEN = 3;
  unsigned int MAX_LOAD_IDX = 87;
  unsigned int MAX_LOAD_LEN = 6;
  unsigned int CRAFT_IDX = 93;
  unsigned int CRAFT_LEN = 10;
  unsigned int BORT_IDX = 103;
  unsigned int BORT_LEN = 10;
  unsigned int CHECKIN_BEG_IDX = 113;
  unsigned int CHECKIN_BEG_LEN = 16;
  unsigned int CHECKIN_END_IDX = 129;
  unsigned int CHECKIN_END_LEN = 16;
  unsigned int BOARDING_BEG_IDX = 145;
  unsigned int BOARDING_BEG_LEN = 16;
  unsigned int BOARDING_END_IDX = 161;
  unsigned int BOARDING_END_LEN = 16;
  unsigned int PR_CANCEL_IDX = 177;
  unsigned int PR_CANCEL_LEN = 1;
  unsigned int PR_DEL_IDX = 178;
  unsigned int PR_DEL_LEN = 1;

struct AODB_Flight {
	int rec_no; //6
	double id;	//10
	std::string airline; //10
	int flt_no;
	std::string suffix;
	std::string litera; //3
	std::string trip_type;
	TDateTime scd; //16
	TDateTime est; //16
	TDateTime act; //16
	std::string hall; //2
	std::string park_out;
	int krm; //3
	int max_load; //6
	std::string craft; //10
	std::string bort; //10
	TDateTime checkin_beg; //16
	TDateTime checkin_end; //16
	TDateTime boarding_beg; //16
	TDateTime boarding_end; //16
	int pr_cancel; //1
	int pr_del; //1
	vector<AODB_Dest> dests;
	vector<AODB_Term> terms;
	string invalid_term;
};

void getRecord( int pax_id, int reg_no, bool pr_unaccomp, const vector<AODB_STRUCT> &aodb_pax,
                const vector<AODB_STRUCT> &aodb_bag,
                string &res_checkin );
void createRecord( int point_id, int pax_id, int reg_no, const string &point_addr, bool pr_unaccomp,
                   const string unaccomp_header,
                   vector<AODB_STRUCT> &aodb_pax, vector<AODB_STRUCT> &aodb_bag,
                   vector<AODB_STRUCT> &prior_aodb_pax, vector<AODB_STRUCT> &prior_aodb_bag,
                   string &res_checkin );

void createFileParamsAODB( int point_id, map<string,string> &params, bool pr_bag )
{
	TQuery FlightQry( &OraSession );
	FlightQry.SQLText = "SELECT airline,flt_no,suffix,scd_out FROM points WHERE point_id=:point_id";
	FlightQry.CreateVariable( "point_id", otInteger, point_id );
	FlightQry.Execute();
	if ( !FlightQry.RowCount() )
		throw Exception( "Flight not found in createFileParams" );
	string region = CityTZRegion( "МОВ" );
	TDateTime scd_out = UTCToLocal( FlightQry.FieldAsDateTime( "scd_out" ), region );
	string p = string( FlightQry.FieldAsString( "airline" ) ) +
	           FlightQry.FieldAsString( "flt_no" ) +
	           FlightQry.FieldAsString( "suffix" ) +
	           string( DateTimeToStr( scd_out, "yymmddhhnn" ) );
	if ( pr_bag )
		p += 'b';
  params[ PARAM_FILE_NAME ] =  p + ".txt";
  params[ NS_PARAM_EVENT_TYPE ] = EncodeEventType( ASTRA::evtPax );
  params[ NS_PARAM_EVENT_ID1 ] = IntToString( point_id );
  params[ PARAM_TYPE ] = VALUE_TYPE_FILE; // FILE
}

void DecodeBagType( int bag_type, int &code, string &name )
{
	switch ( bag_type ) {
	   case 1:
	   case 2: // негабарит
		 	  code = 1;
		 	  name = "НЕГАБ.ГРУЗ";
		 	  break;
		 case 3: // аппаратура
		    code = 2;
		    name = "РАППАРАТУРА";
		    break;
		 case 4: // животные
		 	  code = 3;
		 	  name = "ЖИВОТНЫЕ";
		 	  break;
		 case 5:
		 case 6:
		 case 7: // зелень - деревья, овощи, цветы
		 	  code = 4;
		 	  name = "ЗЕЛЕНЬ";
		 	  break;
		 case 8: // спецсвязь - служебная почта
		 	  code = 6;
		 	  name = "СПЕЦСВЯЗЬ";
		 	  break;
		 default:
		 	  code = 0;
		 	  name = "ОБЫЧНЫЙ";
		 	  break;
		}
}

bool getFlightData( int point_id, const string &point_addr,
                    double &aodb_point_id, string &flight, string &scd_date )
{
	TQuery Qry(&OraSession);
	Qry.SQLText =
	 "SELECT aodb_point_id,airline||flt_no||suffix trip,scd_out "
	 " FROM points, aodb_points, trip_final_stages "
	 " WHERE points.point_id=:point_id AND "
	 "       trip_final_stages.point_id=points.point_id AND "
	 "       trip_final_stages.stage_type=:ckin_stage_type AND "
	 "       trip_final_stages.stage_id BETWEEN :stage1 AND :stage2 AND "
	 "       points.point_id=aodb_points.point_id(+) AND "
	 "       :point_addr=aodb_points.point_addr(+)";
	Qry.CreateVariable( "point_id", otInteger, point_id );
	Qry.CreateVariable( "point_addr", otString, point_addr );
	Qry.CreateVariable( "ckin_stage_type", otInteger, stCheckIn );
	Qry.CreateVariable( "stage1", otInteger, sOpenCheckIn );
	Qry.CreateVariable( "stage2", otInteger, sCloseBoarding );
	Qry.Execute();
	if ( !Qry.RowCount() )
		return false;
	aodb_point_id = Qry.FieldAsFloat( "aodb_point_id" );
	flight = Qry.FieldAsString( "trip" );
	scd_date = DateTimeToStr( UTCToLocal( Qry.FieldAsDateTime( "scd_out" ), CityTZRegion( "МОВ" ) ), "dd.mm.yyyy hh:nn" );
	return true;
}

/*bool createAODBUnaccompBagFile( int point_id, std::map<std::string,std::string> &bag_params, std::string &bag_file_data )
{
	string point_addr = params[PARAM_CANON_NAME];
  double aodb_point_id;
	string flight;
	string region = CityTZRegion( "МОВ" );
	string scd_date;
	vector<AODB_STRUCT> prior_aodb_bag, aodb_bag;
	if ( !getFlightData( point_id, point_addr, aodb_point_id, flight, scd_date ) )
		return false;
	TQuery Qry(&OraSession);
	// достаем старый слепок из БД
	Qry.SQLText =
	 "SELECT grp_id, record FROM aodb_unaccomp "
	 " WHERE point_id=:point_id AND point_addr=:point_addr "
	 " ORDER BY pr_cabin DESC, num ";
	Qry.CreateVariable( "point_id", otInteger, point_id );
	Qry.CreateVariable( "point_addr", otString, point_addr );
	Qry.Execute();
	AODB_STRUCT STRAO;
	STRAO.doit = false;
	STRAO.unaccomp = true;
	while ( !Qry.Eof ) {
		STRAO.pax_id = Qry.FieldAsInteger( "pax_id" );
	  STRAO.num = Qry.FieldAsInteger( "bag_num" );
	  STRAO.record = Qry.FieldAsString( "bag_record" );
	  STRAO.pr_cabin = Qry.FieldAsInteger( "pr_cabin" );
	  prior_aodb_bag.push_back( STRAO );
		Qry.Next();
	}
	// создаем новый слепок на основе данных по регистрации
	TQuery BagQry( &OraSession );
	BagQry.Clear();
	BagQry.SQLText =
		 "SELECT bag2.bag_type,bag2.weight,color,no,"
		 "       bag2.num bag_num "
		 " FROM bag2,bag_tags "
		 " WHERE bag2.grp_id=:grp_id AND "
		 "       bag2.grp_id=bag_tags.grp_id(+) AND "
		 "       bag2.num=bag_tags.bag_num(+) AND "
		 "       bag2.pr_cabin=:pr_cabin "
		 " ORDER BY bag2.num, no";
	BagQry.DeclareVariable( "grp_id", otInteger );
	BagQry.DeclareVariable( "pr_cabin", otInteger );


}
*/

bool createAODBCheckInInfoFile( int point_id, bool pr_unaccomp, const std::string &point_addr, TFileDatas &fds )
{
	TFileData fd;
	TDateTime execTask = NowUTC();
  double aodb_point_id;
	string flight;
	string region = CityTZRegion( "МОВ" );
	string scd_date;
	AODB_STRUCT STRAO;
	vector<AODB_STRUCT> prior_aodb_pax, aodb_pax;
	vector<AODB_STRUCT> prior_aodb_bag, aodb_bag;
	if ( !getFlightData( point_id, point_addr, aodb_point_id, flight, scd_date ) )
		return false;
	ostringstream heading;
	if ( aodb_point_id )
	  heading<<setfill(' ')<<std::fixed<<setw(10)<<setprecision(0)<<aodb_point_id;
	else
	  heading<<setfill(' ')<<std::fixed<<setw(10)<<"";
	heading<<setfill(' ')<<std::fixed<<setw(10)<<flight;
	heading<<setw(16)<<scd_date;

	TQuery Qry(&OraSession);
	if ( pr_unaccomp )
	  Qry.SQLText =
	   "SELECT DISTINCT grp_id pax_id, 0 reg_no, NULL record FROM aodb_unaccomp WHERE point_id=:point_id AND point_addr=:point_addr";
	else
	  Qry.SQLText =
	   "SELECT pax_id, reg_no, record FROM aodb_pax WHERE point_id=:point_id AND point_addr=:point_addr";
	Qry.CreateVariable( "point_id", otInteger, point_id );
	Qry.CreateVariable( "point_addr", otString, point_addr );
	Qry.Execute();
	STRAO.doit = false;
	STRAO.unaccomp = pr_unaccomp;
	while ( !Qry.Eof ) {
  	STRAO.pax_id = Qry.FieldAsInteger( "pax_id" );
  	STRAO.reg_no = Qry.FieldAsInteger( "reg_no" );
		STRAO.record = string( Qry.FieldAsString( "record" ) );
		prior_aodb_pax.push_back( STRAO );
		Qry.Next();
	}
	Qry.Clear();
	if ( pr_unaccomp )
	  Qry.SQLText =
	   "SELECT grp_id pax_id,num bag_num,record bag_record,pr_cabin "
	   " FROM aodb_unaccomp "
	   " WHERE point_id=:point_id AND"
	   "       point_addr=:point_addr "
	   " ORDER BY pr_cabin DESC, bag_num ";
	else
	  Qry.SQLText =
	   "SELECT aodb_pax.pax_id,aodb_bag.num bag_num,aodb_bag.record bag_record,aodb_bag.pr_cabin "
	   " FROM aodb_pax,aodb_bag "
	   " WHERE aodb_pax.point_id=:point_id AND"
	   "       aodb_pax.point_addr=:point_addr AND "
	   "       aodb_bag.point_addr=:point_addr AND "
	   "       aodb_pax.pax_id=aodb_bag.pax_id "
	   " ORDER BY pr_cabin DESC, bag_num ";
	Qry.CreateVariable( "point_id", otInteger, point_id );
	Qry.CreateVariable( "point_addr", otString, point_addr );
	Qry.Execute();
	STRAO.doit = false;
	STRAO.unaccomp = pr_unaccomp;
	while ( !Qry.Eof ) {
		STRAO.pax_id = Qry.FieldAsInteger( "pax_id" );
	  STRAO.num = Qry.FieldAsInteger( "bag_num" );
	  STRAO.record = Qry.FieldAsString( "bag_record" );
	  STRAO.pr_cabin = Qry.FieldAsInteger( "pr_cabin" );
	  prior_aodb_bag.push_back( STRAO );
		Qry.Next();
	}
	// теперь создадим похожую картинку по данным рейса из БД
	TQuery BagQry( &OraSession );
	BagQry.Clear();
	BagQry.SQLText =
		 "SELECT bag2.bag_type,bag2.weight,color,no,"
		 "       bag2.num bag_num, bag_tags.bag_num pr_idx "
		 " FROM bag2,bag_tags "
		 " WHERE bag2.grp_id=:grp_id AND "
		 "       bag2.grp_id=bag_tags.grp_id(+) AND "
		 "       bag2.num=bag_tags.bag_num(+) AND "
		 "       bag2.pr_cabin=:pr_cabin "
		 " ORDER BY bag2.num, no";
	BagQry.DeclareVariable( "grp_id", otInteger );
	BagQry.DeclareVariable( "pr_cabin", otInteger );
	Qry.Clear();
	if ( pr_unaccomp )
	  Qry.SQLText =
	   "SELECT grp_id pax_id, grp_id, 0 reg_no FROM pax_grp "
	   " WHERE point_dep=:point_id AND class IS NULL "
	   " ORDER BY grp_id";
  else
  {
	  Qry.SQLText =
	   "SELECT pax.pax_id,pax.reg_no,pax.surname||' '||pax.name name,pax_grp.grp_id,"
	   "       pax_grp.airp_arv,pax_grp.class,pax.refuse,"
	   "       pax.pers_type, "
	   "       salons.get_seat_no(pax.pax_id,pax.seats,pax_grp.status,pax_grp.point_dep,'one',rownum) AS seat_no, "
	   "       pax.seats seats, "
	   "       ckin.get_excess(pax_grp.grp_id,pax.pax_id) excess,"
	   "       ckin.get_rkAmount(pax_grp.grp_id,pax.pax_id,rownum) rkamount,"
	   "       ckin.get_rkWeight(pax_grp.grp_id,pax.pax_id,rownum) rkweight,"
	   "       ckin.get_bagAmount(pax_grp.grp_id,pax.pax_id,rownum) bagamount,"
	   "       ckin.get_bagWeight(pax_grp.grp_id,pax.pax_id,rownum) bagweight,"
	   "       pax.pr_brd,ckin.get_main_pax_id(pax.grp_id) as main_pax_id, "
	   "       pax_grp.status, "
	   "       pax_grp.client_type "
	   " FROM pax_grp, pax "
	   " WHERE pax_grp.grp_id=pax.grp_id AND "
	   "       pax_grp.point_dep=:point_id AND "
	   "       pax.wl_type IS NULL "
	   " ORDER BY pax_grp.grp_id,seats ";
	};
	Qry.CreateVariable( "point_id", otInteger, point_id );
	Qry.Execute();
	TQuery RemQry( &OraSession );
	RemQry.SQLText = "SELECT rem_code FROM pax_rem WHERE pax_id=:pax_id";
	RemQry.DeclareVariable( "pax_id", otInteger );
	TQuery TimeQry( &OraSession );
	TimeQry.SQLText =
	 "SELECT time as mtime,NVL(stations.name,events.station) station FROM events,stations "
	 " WHERE type='ПАС' AND id1=:point_id AND id2=:reg_no AND events.station=stations.desk(+) AND stations.work_mode(+)=:work_mode "
	 " AND screen=:screen "
	 " ORDER BY time DESC,ev_order DESC";
	TimeQry.CreateVariable( "point_id", otInteger, point_id );
	TimeQry.DeclareVariable( "reg_no", otInteger );
	TimeQry.DeclareVariable( "screen", otString );
	TimeQry.DeclareVariable( "work_mode", otString );
	vector<string> baby_names;

	while ( !Qry.Eof ) {
		if ( !pr_unaccomp && Qry.FieldAsInteger( "seats" ) == 0 ) {
			if ( Qry.FieldIsNULL( "refuse" ) )
			  baby_names.push_back( Qry.FieldAsString( "name" ) );
			Qry.Next();
			continue;
		}
		ostringstream record;
		record<<heading.str();
		if ( !pr_unaccomp ) {
		  record<<setw(3)<<Qry.FieldAsInteger( "reg_no");
	  	record<<setw(30)<<string(Qry.FieldAsString( "name" )).substr(0,30);
		  TAirpsRow *row=(TAirpsRow*)&base_tables.get("airps").get_row("code",Qry.FieldAsString("airp_arv"));
		  record<<setw(20)<<row->code.substr(0,20);
		  record<<setw(1);
		  switch ( DecodeClass(Qry.FieldAsString( "class")) ) {
		  	case ASTRA::F:
		  		  record<<0;
		  		  break;
		  	case ASTRA::C:
		  		  record<<1;
		  		  break;
		  	case ASTRA::Y:
		  		  record<<2;
		  		  break;
		  	default:;
		  }
		  record<<setw(1);
		  RemQry.SetVariable( "pax_id", Qry.FieldAsInteger( "pax_id" ) );
	    RemQry.Execute();
		  bool pr_ex = false;
		  while ( !RemQry.Eof && !pr_ex ) {
		  	string rem = RemQry.FieldAsString( "rem_code" );
		  	if ( rem == "VIP" ) {
		  		record<<2;
		  		pr_ex = true;
		  	}
		  	else
		  		if ( rem == "DIPB" ) {
		  			record<<3;
		  			pr_ex=true;
		  		}
		  		else
		  			if ( rem == "SPSV" ) {
		  				record<<4;
		  				pr_ex=true;
		  			}
		  			else
		  				if ( rem == "MEDA" ) {
		  					record<<5;
		  					pr_ex=true;
		  			  }
		  			  else
		  			  	if ( rem == "UMNR" ) {
		  			  		record<<9;
		  			  		pr_ex=true;
		  			  	}

			  RemQry.Next();
		  }
		  if ( !pr_ex )
		  	record<<0;
		  record<<setw(1);
		  bool adult = false;
		  switch ( DecodePerson( Qry.FieldAsString( "pers_type" ) ) ) {
		  	case ASTRA::adult:
		  		  adult = true;
		  		  record<<0;
		  		  break;
		    case ASTRA::baby:
		    	  record<<2;
		    	  break;
			  default:
		  		  record<<1;
	  	}
		  record<<setw(5)<<Qry.FieldAsString( "seat_no" );
		  record<<setw(2)<<Qry.FieldAsInteger( "seats" )-1;
		  record<<setw(4)<<Qry.FieldAsInteger( "excess" );
		  record<<setw(3)<<Qry.FieldAsInteger( "rkamount" );
		  record<<setw(4)<<Qry.FieldAsInteger( "rkweight" );
		  record<<setw(3)<<Qry.FieldAsInteger( "bagamount" );
		  record<<setw(4)<<Qry.FieldAsInteger( "bagweight" );
		  record<<setw(10)<<""; // номер объединенного рейса
		  record<<setw(16)<<""; // дата объединенного рейса
		  record<<setw(3)<<""; // старый рег. номер пассажира
		  if ( Qry.FieldAsInteger( "pr_brd" ) )
		  	record<<setw(1)<<1;
		  else
		  	record<<setw(1)<<0;
		  if ( adult && !baby_names.empty() ) {
		    record<<setw(2)<<1; // РМ количество
		    record<<setw(36)<<baby_names.begin()->substr(0,36); // Имя ребенка
		    baby_names.erase( baby_names.begin() );
	 	  }
		  else {
		    record<<setw(2)<<0; // РМ количество
		    record<<setw(36)<<""; // Имя ребенка
		  }
		  record<<setw(60)<<""; // ДОП. Инфо
		  record<<setw(1)<<0; // международный багаж
//		record<<setw(1)<<0; // трансатлантический багаж :)
      string term;
      TDateTime t;
      // стойка рег. + время рег. + выход на посадку + время прохода на посадку
      TimeQry.SetVariable( "reg_no", Qry.FieldAsInteger( "reg_no" ) );
      TimeQry.SetVariable( "screen", "AIR.EXE" );
      TimeQry.SetVariable( "work_mode", "Р" );
      TimeQry.Execute();
      if ( TimeQry.Eof ) {
      	t = NoExists;
      }
      else {
        if ( psTCheckin == DecodePaxStatus( Qry.FieldAsString( "status" ) ) )
        	term = "99";
        else
        	if ( DecodeClientType( Qry.FieldAsString( "client_type" ) ) == ctWeb )
         		term = "777";
          else
        	  term = TimeQry.FieldAsString( "station" );
      	if ( !term.empty() && term[0] == 'R' )
      		term = term.substr( 1, term.length() - 1 );
      	t = TimeQry.FieldAsDateTime( "mtime" );
      }
      if ( t == NoExists || term.empty() )
        record<<setw(4)<<"";
      else
        record<<setw(4)<<string(term).substr(0,4); // стойка рег.
      if ( t == NoExists )
      	record<<setw(16)<<"";
      else {
      	record<<setw(16)<<DateTimeToStr( UTCToLocal( t, region ), "dd.mm.yyyy hh:nn" );
      }
      TimeQry.SetVariable( "screen", "BRDBUS.EXE" );
      TimeQry.SetVariable( "work_mode", "П" );
      TimeQry.Execute();
      if ( TimeQry.Eof ) {
      	term.clear();
      	t = NoExists;
      }
      else {
      	term = TimeQry.FieldAsString( "station" );
      	if ( !term.empty() && term[0] == 'G' )
      		term = term.substr( 1, term.length() - 1 );
      	t = TimeQry.FieldAsDateTime( "mtime" );
      }
      if ( t == NoExists )
        record<<setw(4)<<"";
      else
        record<<setw(4)<<string(term).substr(0,4); // выход на посадку
      if ( t == NoExists )
      	record<<setw(16)<<"";
      else {
      	record<<setw(16)<<DateTimeToStr( UTCToLocal( t, region ), "dd.mm.yyyy hh:nn" );	 //время прохода на посадку
      }
		  if ( Qry.FieldIsNULL( "refuse" ) )
		  	record<<setw(1)<<0<<";";
		  else
    		record<<setw(1)<<1<<";"; // отказ от полета
    } // end if !pr_unaccomp
	 	STRAO.record = record.str();
	  STRAO.pax_id = Qry.FieldAsInteger( "pax_id" );
	  STRAO.reg_no = Qry.FieldAsInteger( "reg_no" );
		aodb_pax.push_back( STRAO );
		// ручная кладь
		if ( pr_unaccomp || Qry.FieldAsInteger( "main_pax_id" ) == Qry.FieldAsInteger( "pax_id" ) ) {
		  BagQry.SetVariable( "grp_id", Qry.FieldAsInteger( "grp_id" ) );
		  BagQry.SetVariable( "pr_cabin", 1 );
		  BagQry.Execute();

		  int code;
		  string type_name;
		  while ( !BagQry.Eof ) {
		  	ostringstream record_bag;
		  	record_bag<<setfill(' ')<<std::fixed;
		  	DecodeBagType( BagQry.FieldAsInteger( "bag_type" ), code, type_name );
		  	record_bag<<setw(2)<<code<<setw(20)<<type_name.substr(0,20);
	  	  record_bag<<setw(4)<<BagQry.FieldAsInteger( "weight" );
	  		//record<<setw(1)<<0; // снятие
	  		STRAO.pax_id = Qry.FieldAsInteger( "pax_id" );
	  		STRAO.num = BagQry.FieldAsInteger( "bag_num" );
	  		STRAO.record = record_bag.str();
	  		STRAO.pr_cabin = 1;
		  	aodb_bag.push_back( STRAO );
		  	BagQry.Next();
		  }
		  // багаж
		  BagQry.SetVariable( "pr_cabin", 0 );
		  BagQry.Execute();
		  int prior_idx = -1, prior_bag_num = -1;
		  while ( !BagQry.Eof ) {
		  	ostringstream record_bag;
		  	record_bag<<setfill(' ')<<std::fixed;
		  	DecodeBagType( BagQry.FieldAsInteger( "bag_type" ), code, type_name );
		  	record_bag<<setw(2)<<code<<setw(20)<<type_name.substr(0,20);
		  	record_bag<<setw(10)<<setprecision(0)<<BagQry.FieldAsFloat( "no" );
		  	record_bag<<setw(2)<<string(BagQry.FieldAsString( "color" )).substr(0,2);
		  	if ( prior_idx == BagQry.FieldAsInteger( "pr_idx" ) &&
		  		   prior_bag_num == BagQry.FieldAsInteger( "bag_num" ) )
		  		record_bag<<setw(4)<<0;
		  	else
		  		record_bag<<setw(4)<<BagQry.FieldAsInteger( "weight" );
		  	prior_idx = BagQry.FieldAsInteger( "pr_idx" );
		  	prior_bag_num = BagQry.FieldAsInteger( "bag_num" );
	  		STRAO.pax_id = Qry.FieldAsInteger( "pax_id" );
	  		STRAO.num = BagQry.FieldAsInteger( "bag_num" );
	  		STRAO.record = record_bag.str();
	  		STRAO.pr_cabin = 0;
		  	aodb_bag.push_back( STRAO );
		  	BagQry.Next();
		  }
		}
		else {
	  }
		Qry.Next();
	}
	// далее сравнение 2-х слепков, выяснение, что добавилось, что удалилось, что изменилось
	// формирование данных для отпавки + их запись как новый слепок?
	if ( NowUTC() - execTask > 1.0/1440.0 ) {
		ProgTrace( TRACE5, "Attention execute task aodb time > 1 min !!!, time=%s", DateTimeToStr( NowUTC() - execTask, "nn:ss" ).c_str() );
	}

  string res_checkin, prior_res_checkin;
  bool ch_pax;
  // вначале идет проверка на удаленных и измененных пассажиров
	for ( vector<AODB_STRUCT>::iterator p=prior_aodb_pax.begin(); p!=prior_aodb_pax.end(); p++ ) {
 		getRecord( p->pax_id, p->reg_no, pr_unaccomp, aodb_pax, aodb_bag, res_checkin/*, res_bag*/ );
  	getRecord( p->pax_id, p->reg_no, pr_unaccomp, prior_aodb_pax, prior_aodb_bag, prior_res_checkin/*, prior_res_bag*/ );
	  ch_pax = res_checkin != prior_res_checkin;
  	if ( ch_pax ) {
		  createRecord( point_id, p->pax_id, p->reg_no, point_addr, pr_unaccomp,
		                heading.str(),
		                aodb_pax, aodb_bag, prior_aodb_pax, prior_aodb_bag,
		                res_checkin/*, res_bag*/ );
 			if ( ch_pax )
	  		fd.file_data += res_checkin;
	  }
  }
  // потом идет проверка на новых пассажиров
	for ( vector<AODB_STRUCT>::iterator p=aodb_pax.begin(); p!=aodb_pax.end(); p++ ) {
		if ( p->doit )
			continue;
		getRecord( p->pax_id, p->reg_no, pr_unaccomp, aodb_pax, aodb_bag, res_checkin/*, res_bag*/ );
		getRecord( p->pax_id, p->reg_no, pr_unaccomp, prior_aodb_pax, prior_aodb_bag, prior_res_checkin/*, prior_res_bag*/ );
		ch_pax = res_checkin != prior_res_checkin;
		//ch_bag = res_bag != prior_res_bag;
		if ( ch_pax/*|| ch_bag*/ ) {
//		if ( getRecord( p->pax_id, aodb_pax, aodb_bag ) != getRecord( p->pax_id, prior_aodb_pax, prior_aodb_bag ) ) {
//			ProgTrace(TRACE5, "p->doit=%d, pax_id=%d", p->doit, p->pax_id );
			createRecord( point_id, p->pax_id, p->reg_no, point_addr, pr_unaccomp,
			              heading.str(),
			              aodb_pax, aodb_bag, prior_aodb_pax, prior_aodb_bag,
			              res_checkin/*, res_bag*/ );
			if ( ch_pax )
				fd.file_data += res_checkin;
/*			if ( ch_bag ) {
				//ProgTrace( TRACE5, "res_bag=%s, prior_res_bag=%s", res_bag.c_str(), prior_res_bag.c_str() );
				bag_file_data += res_bag;
			}*/
//			ProgTrace(TRACE5, "create record pax_id=%d", p->pax_id );
	  }
  }
  if ( !fd.file_data.empty() ) {
	  createFileParamsAODB( point_id, fd.params, pr_unaccomp );
		fds.push_back( fd );
	}
	return !fds.empty();
}


void getRecord( int pax_id, int reg_no, bool pr_unaccomp, const vector<AODB_STRUCT> &aodb_pax, const vector<AODB_STRUCT> &aodb_bag,
                string &res_checkin/*, string &res_bag*/  )
{
	res_checkin.clear();
//	res_bag.clear();
	for ( vector<AODB_STRUCT>::const_iterator i=aodb_pax.begin(); i!=aodb_pax.end(); i++ ) {
	  if ( i->pax_id == pax_id && i->reg_no == reg_no ) {
	  	if ( !pr_unaccomp )
		    res_checkin = i->record;
		  for ( vector<AODB_STRUCT>::const_iterator b=aodb_bag.begin(); b!=aodb_bag.end(); b++ ) {
		  	if ( b->pax_id != pax_id )
		  		continue;
		  	res_checkin += b->record;
//		  	res_bag += b->record;
	    }
	    break;
	  }
	}
//	ProgTrace( TRACE5, "getRecord pax_id=%d, reg_no=%d,return res=%s", pax_id, reg_no, res_checkin.c_str() );
}

void createRecord( int point_id, int pax_id, int reg_no, const string &point_addr, bool pr_unaccomp,
                   const string unaccomp_header,
                   vector<AODB_STRUCT> &aodb_pax, vector<AODB_STRUCT> &aodb_bag,
                   vector<AODB_STRUCT> &prior_aodb_pax, vector<AODB_STRUCT> &prior_aodb_bag,
                   string &res_checkin/*, string &res_bag*/ )
{
	ProgTrace( TRACE5, "point_id=%d, pax_id=%d, reg_no=%d, point_addr=%s", point_id, pax_id, reg_no, point_addr.c_str() );
	res_checkin.clear();
	//res_bag.clear();
	TQuery PQry( &OraSession );
 	PQry.SQLText =
 	 "SELECT NVL(MAX(rec_no_pax),-1) r1, "
 	 "       NVL(MAX(rec_no_bag),-1) r2, "
 	 "       NVL(MAX(rec_no_unaccomp),-1) r3 "
 	 " FROM aodb_points "
 	 " WHERE point_id=:point_id AND point_addr=:point_addr";
 	PQry.CreateVariable( "point_id", otInteger, point_id );
 	PQry.CreateVariable( "point_addr", otString, point_addr );
 	PQry.Execute();
 	int bag_num;
 	if ( pr_unaccomp ) {
 		bag_num = PQry.FieldAsInteger( "r3" ) + 1;
  }
 	else {
 	  bag_num = PQry.FieldAsInteger( "r2" );
    ostringstream r;
    r<<setfill(' ')<<std::fixed<<setw(6)<<PQry.FieldAsInteger( "r1" ) + 1;
 	  res_checkin = r.str();
  }
  	// сохраняем новый слепок
 	PQry.Clear();
 	if ( pr_unaccomp )
 	  PQry.SQLText =
 	   " DELETE aodb_unaccomp WHERE grp_id=:pax_id AND point_addr=:point_addr AND point_id=:point_id ";
 	else
 	  PQry.SQLText =
 	   "BEGIN "
 	   " DELETE aodb_bag WHERE pax_id=:pax_id AND point_addr=:point_addr; "
 	   " DELETE aodb_pax WHERE pax_id=:pax_id AND point_addr=:point_addr; "
 	   " UPDATE aodb_points SET rec_no_pax=NVL(rec_no_pax,-1)+1 WHERE point_id=:point_id AND point_addr=:point_addr; "
     "  IF SQL%NOTFOUND THEN "
     "    INSERT INTO aodb_points(point_id,point_addr,aodb_point_id,record,rec_no_flt,rec_no_pax,rec_no_bag,rec_no_unaccomp) "
     "      VALUES(:point_id,:point_addr,NULL,NULL,-1,0,-1,-1); "
     "  END IF; "
 	   "END; ";
 	PQry.CreateVariable( "point_id", otInteger, point_id );
 	PQry.CreateVariable( "pax_id", otInteger, pax_id );
 	PQry.CreateVariable( "point_addr", otString, point_addr );
 	PQry.Execute();
 	vector<AODB_STRUCT>::iterator d, n;
  d=aodb_pax.end();
  n=prior_aodb_pax.end();
	for ( d=aodb_pax.begin(); d!=aodb_pax.end(); d++ ) // выбираем нужного пассажира
 		if ( d->pax_id == pax_id && d->reg_no == reg_no ) {
 			break;
 		}
  for ( n=prior_aodb_pax.begin(); n!=prior_aodb_pax.end(); n++ )
 		if ( n->pax_id == pax_id && n->reg_no == reg_no ) {
 			break;
 		}
 	if ( !pr_unaccomp ) {
    if ( d == aodb_pax.end() ) { // удаление пассажира
  	  res_checkin += n->record.substr( 0, n->record.length() - 2 );
  	  res_checkin += "1;";
  		ProgTrace( TRACE5, "delete record, record=%s", res_checkin.c_str() );
    }
	  else  {
  	  res_checkin += d->record;
    	PQry.Clear();
       PQry.SQLText =
        "INSERT INTO aodb_pax(point_id,pax_id,reg_no,point_addr,record) "
        " VALUES(:point_id,:pax_id,:reg_no,:point_addr,:record)";
      PQry.CreateVariable( "point_id", otInteger, point_id );
      PQry.CreateVariable( "pax_id", otInteger, pax_id );
      PQry.CreateVariable( "reg_no", otInteger, reg_no );
      PQry.CreateVariable( "point_addr", otString, point_addr );
      PQry.CreateVariable( "record", otString, d->record );
      PQry.Execute();
    }
  }
  PQry.Clear();
  if ( pr_unaccomp )
    PQry.SQLText =
     "INSERT INTO aodb_unaccomp(grp_id,point_id,point_addr,num,pr_cabin,record) "
     " VALUES(:pax_id,:point_id,:point_addr,:num,:pr_cabin,:record)";
  else
    PQry.SQLText =
     "INSERT INTO aodb_bag(pax_id,point_addr,num,pr_cabin,record) "
     " VALUES(:pax_id,:point_addr,:num,:pr_cabin,:record)";
  if ( pr_unaccomp )
  	PQry.CreateVariable( "point_id", otInteger, point_id );
  PQry.CreateVariable( "pax_id", otInteger, pax_id );
  PQry.CreateVariable( "point_addr", otString, point_addr );
  PQry.DeclareVariable( "num", otInteger );
  PQry.DeclareVariable( "pr_cabin", otInteger );
  PQry.DeclareVariable( "record", otString );
  int num=0;
  vector<string> bags;
	vector<string> delbags;
	for ( int pr_cabin=1; pr_cabin>=0; pr_cabin-- ) {
		if ( d != aodb_pax.end() ) {
		  PQry.SetVariable( "pr_cabin", pr_cabin );
	    for ( vector<AODB_STRUCT>::iterator i=aodb_bag.begin(); i!= aodb_bag.end(); i++ ) {
	  	  if ( i->pr_cabin != pr_cabin || i->pax_id != pax_id )
  	  		continue;
	    	PQry.SetVariable( "num", num );
	    	PQry.SetVariable( "record", i->record );
	  	  PQry.Execute();
	  	  num++;
	  	  bool f=false;
	  	  for ( vector<AODB_STRUCT>::iterator j=prior_aodb_bag.begin(); j!= prior_aodb_bag.end(); j++ ) {
  	  		if ( j->pr_cabin == pr_cabin && !j->doit &&
	    			   i->pax_id == pax_id && i->pax_id == j->pax_id && i->record == j->record ) {
	  			  f=true;
	  			  i->doit=true;
	  			  j->doit=true;
   			    bags.push_back( i->record + "0;" );
	  		  }
	      }
	      if ( !f ) {
	    	  bags.push_back( i->record + "0;" );
	      }
  	  }
  	}
  	if ( d != aodb_pax.end() || pr_unaccomp ) {
	    for ( vector<AODB_STRUCT>::iterator i=prior_aodb_bag.begin(); i!= prior_aodb_bag.end(); i++ ) {
	    	bool f=false;
	    	if ( i->doit || i->pr_cabin != pr_cabin || i->pax_id != pax_id )
	  	  	continue;
	  	  for ( vector<AODB_STRUCT>::iterator j=aodb_bag.begin(); j!= aodb_bag.end(); j++ ) {
	  		  if ( j->pr_cabin == pr_cabin && i->pax_id == pax_id && i->pax_id == j->pax_id && i->record == j->record ) {
  	  			f=true;
	    			break;
	    		}
	      }
	      if ( !f ) {
	    	  delbags.push_back(i->record + "1;");
	      }
  	  }
  	}
	} // end for
	  if ( d != aodb_pax.end() )
	  	d->doit = true;
	  if ( n != prior_aodb_pax.end() )
	  	n->doit = true;

	for (vector<string>::iterator si=delbags.begin(); si!=delbags.end(); si++ ) {
		if ( pr_unaccomp ) {
      stringstream r;
      r<<setfill(' ')<<std::fixed<<setw(6)<<bag_num++<<unaccomp_header;
      res_checkin += r.str() + *si + "\n";
    }
    else res_checkin += *si;
	}

	for (vector<string>::iterator si=bags.begin(); si!=bags.end(); si++ ) {
		if ( pr_unaccomp ) {
      stringstream r;
      r<<setfill(' ')<<std::fixed<<setw(6)<<bag_num++<<unaccomp_header;
      res_checkin += r.str() + *si + "\n";
    }
    else res_checkin += *si;
	}

	PQry.Clear();
	if ( pr_unaccomp ) {
	  PQry.SQLText =
 	   "BEGIN "
 	   "UPDATE aodb_points SET rec_no_unaccomp=:rec_no_unaccomp WHERE point_id=:point_id AND point_addr=:point_addr; "
     "  IF SQL%NOTFOUND THEN "
     "    INSERT INTO aodb_points(point_id,point_addr,aodb_point_id,record,rec_no_flt,rec_no_pax,rec_no_bag,rec_no_unaccomp) "
     "      VALUES(:point_id,:point_addr,NULL,NULL,-1,-1,-1,0); "
     "  END IF; "
 	   "END; ";
 	  PQry.CreateVariable( "rec_no_unaccomp", otInteger, bag_num - 1 );
 	}
	else {
	  PQry.SQLText =
 	   "BEGIN "
 	   "UPDATE aodb_points SET rec_no_bag=:rec_no_bag WHERE point_id=:point_id AND point_addr=:point_addr; "
     "  IF SQL%NOTFOUND THEN "
     "    INSERT INTO aodb_points(point_id,point_addr,aodb_point_id,record,rec_no_flt,rec_no_pax,rec_no_bag,rec_no_unaccomp) "
     "      VALUES(:point_id,:point_addr,NULL,NULL,-1,0,:rec_no_bag,-1); "
     "  END IF; "
 	   "END; ";
 	  PQry.CreateVariable( "rec_no_bag", otInteger, bag_num );
 	}
 	PQry.CreateVariable( "point_id", otInteger, point_id );
 	PQry.CreateVariable( "point_addr", otString, point_addr );
 	PQry.Execute();
 	if ( !pr_unaccomp )
	  res_checkin += "\n";
}

void ParseFlight( const std::string &point_addr, std::string &linestr, AODB_Flight &fl )
{
	TQuery QryTripInfo(&OraSession);
  QryTripInfo.SQLText=
    "SELECT airline,flt_no,suffix,airp,scd_out FROM points "
    "WHERE point_id=:point_id AND pr_del>=0";
	QryTripInfo.DeclareVariable("point_id", otInteger);
  TTripInfo fltInfo;
	int err=0;
try {
	fl.invalid_term.clear();
  fl.rec_no = NoExists;
 	if ( linestr.length() < REC_NO_LEN )
 		throw Exception( "Ошибка формата рейса, длина=%d, значение=%s", linestr.length(), linestr.c_str() );
  err++;
	TReqInfo *reqInfo = TReqInfo::Instance();
  string region = CityTZRegion( "МОВ" );
	TQuery Qry( &OraSession );
	TQuery TIDQry( &OraSession );
	TQuery POINT_IDQry( &OraSession );
	string tmp, tmp1;
	tmp = linestr.substr( REC_NO_IDX, REC_NO_LEN );
	tmp = TrimString( tmp );
	fl.rec_no = NoExists;
	if ( StrToInt( tmp.c_str(), fl.rec_no ) == EOF || fl.rec_no < 0 ||fl.rec_no > 999999 )
		throw Exception( "Ошибка в номере строки, значение=%s", tmp.c_str() );

 	if ( linestr.length() < 180 + 4 )
 		throw Exception( "Ошибка формата рейса, длина=%d, значение=%s", linestr.length(), linestr.c_str() );

	tmp = linestr.substr( FLT_ID_IDX, FLT_ID_LEN );
	tmp = TrimString( tmp );
	if ( StrToFloat( tmp.c_str(), fl.id ) == EOF || fl.id < 0 || fl.id > 9999999999.0 )
		throw Exception( "Ошибка идентификатора рейса, значение=%s", tmp.c_str() );
	tmp = linestr.substr( TRIP_IDX, TRIP_LEN );
	tmp = TrimString( tmp );
	if ( tmp.length() < 3 )
		throw Exception( "Ошибка формата номера рейса, значение=%s", tmp.c_str() );
	int i;
	if ( IsDigit( tmp[ 2 ] ) ) {
	  fl.airline = tmp.substr( 0, 2 );
	  i=2;
	}
	else {
	  fl.airline = tmp.substr( 0, 3 );
	  i=3;
	}
	tmp1.clear();
	for ( ;i<(int)tmp.length(); i++ ) {
		if ( IsDigit( tmp[ i ] ) ) {
			if ( !fl.suffix.empty() ) {
				throw Exception( "Ошибка формата номера рейса, значение=%s", tmp.c_str() );
			}
			tmp1 += tmp[ i ];
		}
		else {
			fl.suffix += tmp[ i ];
		}
	}
	fl.airline = TrimString( fl.airline );
	fl.suffix = TrimString( fl.suffix );
	if ( fl.airline.empty() || tmp1.empty() || fl.suffix.length() > 1 )
		throw Exception( "Ошибка формата номера рейса, значение=%s", tmp.c_str() );
	if ( StrToInt( tmp1.c_str(), fl.flt_no ) == EOF )
		throw Exception( "Ошибка формата номера рейса, значение=%s", tmp.c_str() );
	if ( fl.flt_no > 99999 || fl.flt_no <= 0 )
		throw Exception( "Ошибка формата номера рейса, значение=%s", tmp.c_str() );
	err++;
	TElemFmt fmt;
  try {
   fl.suffix = ElemToElemId( etSuffix, fl.suffix, fmt, false );
  }
  catch( EConvertError &e ) {
  	throw Exception( "Ошибка формата номера рейса, значение=%s", tmp.c_str() );
  }
 	try {
    fl.airline = ElemToElemId( etAirline, fl.airline, fmt, false );
	  if ( fmt == efmtCodeInter || fmt == efmtCodeICAOInter )
		  fl.trip_type = "м";  //!!!vlad а правильно ли так определять тип рейса? не уверен. Проверка при помощи маршрута. Если в маршруте все п.п. принадлежат одной стране то "п" иначе "м"
    else
  	  fl.trip_type = "п";
  }
  catch( EConvertError &e ) {
  	Qry.Clear();
  	Qry.SQLText =
  	 "SELECT airline as code FROM aodb_airlines WHERE aodb_code=:code";
	  Qry.CreateVariable( "code", otString, fl.airline );
	  err++;
	  Qry.Execute();
	  err++;
	  if ( !Qry.RowCount() )
	  	throw Exception( "Неизвестная авиакомпания, значение=%s", fl.airline.c_str() );
	  fl.airline = Qry.FieldAsString( "code" );
	  fl.trip_type = "п"; //???
  }
  err++;
	tmp = linestr.substr( LITERA_IDX, LITERA_LEN );
	fl.litera = TrimString( tmp );
	if ( !fl.litera.empty() ) {
		Qry.Clear();
		Qry.SQLText =
		 "SELECT code,1 FROM trip_liters WHERE code=:code AND pr_del=0"
		 " UNION "
		 "SELECT litera as code,2 FROM aodb_liters WHERE aodb_code=:code "
		 " ORDER BY 2";
		Qry.CreateVariable( "code", otString, fl.litera );
		err++;
		Qry.Execute();
		err++;
		if ( !Qry.RowCount() )
			throw Exception( "Неизвестная литера, значение=%s", fl.litera.c_str() );
		fl.litera = Qry.FieldAsString( "code" );
	}
	err++;
	tmp = linestr.substr( SCD_IDX, SCD_LEN );
	tmp = TrimString( tmp );
  if ( tmp.empty() )
		throw Exception( "Ошибка формата планового времени вылета, значение=%s", tmp.c_str() );
	else
		if ( StrToDateTime( tmp.c_str(), "dd.mm.yyyy hh:nn", fl.scd ) == EOF )
			throw Exception( "Ошибка формата планового времени вылета, значение=%s", tmp.c_str() );
  try {
	  fl.scd = LocalToUTC( fl.scd, region );
	}
	catch( boost::local_time::ambiguous_result ) {
		throw Exception( "Плановое время выполнения рейса определено не однозначно" );
  }
  catch( boost::local_time::time_label_invalid ) {
    throw Exception( "Плановое время выполнения рейса не существует" );
  }
	tmp = linestr.substr( EST_IDX, EST_LEN );
	tmp = TrimString( tmp );
  if ( tmp.empty() )
		fl.est = NoExists;
	else {
		if ( StrToDateTime( tmp.c_str(), "dd.mm.yyyy hh:nn", fl.est ) == EOF )
			throw Exception( "Ошибка формата расчетного времени вылета, значение=%s", tmp.c_str() );
		try {
		  fl.est = LocalToUTC( fl.est, region );
		}
	  catch( boost::local_time::ambiguous_result ) {
		  throw Exception( "Расчетное время выполнения рейса определено не однозначно" );
    }
    catch( boost::local_time::time_label_invalid ) {
      throw Exception( "Расчетное время выполнения рейса не существует" );
    }
	}
	tmp = linestr.substr( ACT_IDX, ACT_LEN );
	tmp = TrimString( tmp );
  if ( tmp.empty() )
		fl.act = NoExists;
	else {
		if ( StrToDateTime( tmp.c_str(), "dd.mm.yyyy hh:nn", fl.act ) == EOF )
			throw Exception( "Ошибка формата фактического времени вылета, значение=%s", tmp.c_str() );
		try {
		  fl.act= LocalToUTC( fl.act, region );
		}
	  catch( boost::local_time::ambiguous_result ) {
	  	throw Exception( "Фактическое время выполнения рейса определено не однозначно" );
    }
    catch( boost::local_time::time_label_invalid ) {
      throw Exception( "Фактическое время выполнения рейса не существует" );
    }
	}
	err++;
	tmp = linestr.substr( HALL_IDX, HALL_LEN );
	fl.hall = TrimString( tmp );
	tmp = linestr.substr( PARK_OUT_IDX, PARK_OUT_LEN );
	fl.park_out = TrimString( tmp );
	fl.park_out = fl.park_out.substr( 0, 3 );

	tmp = linestr.substr( KRM_IDX, KRM_LEN );
	tmp = TrimString( tmp );
	if ( tmp.empty() )
		fl.krm = NoExists;
	else
  	if ( StrToInt( tmp.c_str(), fl.krm ) == EOF )
	  	throw Exception( "Ошибка формата КРМ, значение=%s", tmp.c_str() );
	tmp = linestr.substr( MAX_LOAD_IDX, MAX_LOAD_LEN );
	tmp = TrimString( tmp );
	if ( tmp.empty() )
		fl.max_load = NoExists;
	else
  	if ( StrToInt( tmp.c_str(), fl.max_load ) == EOF || fl.max_load < 0 || fl.max_load > 999999 )
	  	throw Exception( "Ошибка формата МКЗ, значение=%s", tmp.c_str() );
	tmp = linestr.substr( CRAFT_IDX, CRAFT_LEN );
	fl.craft = TrimString( tmp );
	bool pr_craft_error = true;
	err++;
	if ( !fl.craft.empty() ) {
 	  try {
      fl.craft = ElemCtxtToElemId( ecDisp, etCraft, fl.craft, fmt, false );
    }
    catch( EConvertError &e ) {
  	  Qry.Clear();
    	Qry.SQLText =
	     "SELECT code, 1 FROM crafts WHERE ( name=:code OR name_lat=:code ) AND pr_del=0 "
	     " UNION "
	     "SELECT craft as code, 2 FROM aodb_crafts WHERE aodb_code=:code";
	    Qry.CreateVariable( "code", otString, fl.craft );
	    err++;
	    Qry.Execute();
	    err++;
	    pr_craft_error = !Qry.RowCount();
	    if ( !pr_craft_error )
	      fl.craft = Qry.FieldAsString( "code" );
    }
	}
	err++;
  tmp = linestr.substr( BORT_IDX, BORT_LEN );
  fl.bort = TrimString( tmp );
	tmp = linestr.substr( CHECKIN_BEG_IDX, CHECKIN_BEG_LEN );
	tmp = TrimString( tmp );
  if ( tmp.empty() )
		fl.checkin_beg = NoExists;
	else {
		if ( StrToDateTime( tmp.c_str(), "dd.mm.yyyy hh:nn", fl.checkin_beg ) == EOF )
			throw Exception( "Ошибка формата начала регистрации, значение=%s", tmp.c_str() );
		try {
		  fl.checkin_beg = LocalToUTC( fl.checkin_beg, region );
		}
	  catch( boost::local_time::ambiguous_result ) {
	  	fl.checkin_beg = LocalToUTC( fl.checkin_beg + 1, region ) - 1;
    }
    catch( boost::local_time::time_label_invalid ) {
      throw Exception( "Начало регистрации рейса не существует" );
    }
	}
	tmp = linestr.substr( CHECKIN_END_IDX, CHECKIN_END_LEN );
	tmp = TrimString( tmp );
  if ( tmp.empty() )
		fl.checkin_end = NoExists;
	else {
		if ( StrToDateTime( tmp.c_str(), "dd.mm.yyyy hh:nn", fl.checkin_end ) == EOF )
			throw Exception( "Ошибка формата окончания регистрации, значение=%s", tmp.c_str() );
		try {
		  fl.checkin_end = LocalToUTC( fl.checkin_end, region );
		}
	  catch( boost::local_time::ambiguous_result ) {
	  	fl.checkin_end = LocalToUTC( fl.checkin_end + 1, region ) - 1;
    }
    catch( boost::local_time::time_label_invalid ) {
      throw Exception( "Окончание регистрации рейса не существует" );
    }
	}
	tmp = linestr.substr( BOARDING_BEG_IDX, BOARDING_BEG_LEN );
	tmp = TrimString( tmp );
	err++;
  if ( tmp.empty() )
		fl.boarding_beg = NoExists;
	else {
		if ( StrToDateTime( tmp.c_str(), "dd.mm.yyyy hh:nn", fl.boarding_beg ) == EOF )
			throw Exception( "Ошибка формата начала посадки, значение=%s", tmp.c_str() );
		try {
		  fl.boarding_beg = LocalToUTC( fl.boarding_beg, region );
		}
	  catch( boost::local_time::ambiguous_result ) {
	  	fl.boarding_beg = LocalToUTC( fl.boarding_beg + 1, region ) - 1;
    }
    catch( boost::local_time::time_label_invalid ) {
      throw Exception( "Начало посадки рейса не существует" );
    }
	}
	err++;
	tmp = linestr.substr( BOARDING_END_IDX, BOARDING_END_LEN );
	tmp = TrimString( tmp );
  if ( tmp.empty() )
		fl.boarding_end = NoExists;
	else {
		if ( StrToDateTime( tmp.c_str(), "dd.mm.yyyy hh:nn", fl.boarding_end ) == EOF )
			throw Exception( "Ошибка формата окончания посадки, значение=%s", tmp.c_str() );
		try {
		  fl.boarding_end = LocalToUTC( fl.boarding_end, region );
		}
	  catch( boost::local_time::ambiguous_result ) {
	  	fl.boarding_end = LocalToUTC( fl.boarding_end + 1, region ) - 1;
    }
    catch( boost::local_time::time_label_invalid ) {
      throw Exception( "Окончание посадки рейса не существует" );
    }
	}
	err++;
	tmp = linestr.substr( PR_CANCEL_IDX, PR_CANCEL_LEN );
	tmp = TrimString( tmp );
	if ( tmp.empty() )
		fl.pr_cancel = 0;
	else
  	if ( StrToInt( tmp.c_str(), fl.pr_cancel ) == EOF )
	  	throw Exception( "Ошибка формата признака отмены, значение=%s", tmp.c_str() );
	tmp = linestr.substr( PR_DEL_IDX, PR_DEL_LEN );
	tmp = TrimString( tmp );
	if ( tmp.empty() )
		fl.pr_del = NoExists;
	else
  	if ( StrToInt( tmp.c_str(), fl.pr_del ) == EOF )
	  	throw Exception( "Ошибка формата признака удаления, значение=%s", tmp.c_str() );
	int len = linestr.length();
	i = PR_DEL_IDX + PR_DEL_LEN;
	tmp = linestr.substr( i, 1 );
	if ( tmp[ 0 ] != ';' )
		throw Exception( "Ошибка формата маршрута. Ожидался символ ';', встретился символ	%c", linestr[ i ] );
	i++;
	bool dest_mode = true;
	err++;
	AODB_Dest dest;
	AODB_Term term;
	while ( i < len ) {
		tmp = linestr.substr( i, 1 );
		tmp = TrimString( tmp );
		if ( dest_mode ) {
    	if ( StrToInt( tmp.c_str(), dest.num ) == EOF || dest.num < 0 || dest.num > 9 )
    		if ( fl.dests.empty() )
	    	  throw Exception( "Ошибка формата номера пункта посадки, значение=%s", tmp.c_str() );
	      else
	    	  dest_mode = false;
	    else {
	    	i++;
	    	tmp = linestr.substr( i, 3 );
	    	dest.airp = TrimString( tmp );
	    	if ( dest.airp.empty() )
	    		throw Exception( "Ошибка формата кода аэропорта, значение=%s", dest.airp.c_str() );
    	  try {
         dest.airp = ElemCtxtToElemId( ecDisp, etAirp, dest.airp, fmt, false );
        }
        catch( EConvertError &e ) {
	    	  Qry.Clear();
	    	  Qry.SQLText =
	    	   "SELECT code, 1 FROM airps WHERE ( name=:code OR name_lat=:code ) AND pr_del=0 "
	    	   " UNION "
	    	   "SELECT airp as code, 2 FROM aodb_airps WHERE aodb_code=:code "
	    	   " ORDER BY 2 ";
	      	Qry.CreateVariable( "code", otString, dest.airp );
	      	err++;
	      	Qry.Execute();
	      	err++;
	      	if ( !Qry.RowCount() )
	      		throw Exception( "Неизвестный код аэропорта, значение=%s", dest.airp.c_str() );
	      	dest.airp = Qry.FieldAsString( "code" );
        }
	    	i += 3;
	    	tmp = linestr.substr( i, 1 );
	    	tmp = TrimString( tmp );
	    	if ( tmp.empty() || StrToInt( tmp.c_str(), dest.pr_del ) == EOF || dest.pr_del < 0 || dest.pr_del > 1 )
	    	  throw Exception( "Ошибка формата признака удаления пункта, значение=%s", tmp.c_str() );
	    	fl.dests.push_back( dest );
	    	i++;
	    	tmp = linestr.substr( i, 1 );
	      if ( tmp[ 0 ] != ';' )
		      throw Exception( "Ошибка формата маршрута. Ожидался символ ';', встретился символ	%c", linestr[ i ] );
		    i++;
	    }
		}
		err++;
		if ( !dest_mode ) {
			int old_i = i;
			try {
			  if ( tmp != "П" && tmp != "Р" )
			  	throw Exception( "Ошибка формата типа стойки, значение=%s", tmp.c_str() );
			  term.type = tmp;
			  i++;
    	  tmp = linestr.substr( i, 4 );
	   	  term.name = TrimString( tmp );
			  if ( term.name.empty() )
			  	throw Exception( "Ошибка формата номера стойки, значение=%s", term.name.c_str() );
			  string term_name;
			  if ( term.type == "П" )
		  		term_name = "G" + term.name;
		  	else
		  		term_name = "R" + term.name;
	  		Qry.Clear();
	  		Qry.SQLText = "SELECT desk FROM stations WHERE airp=:airp AND work_mode=:work_mode AND name=:code";
	  		Qry.CreateVariable( "airp", otString, "ВНК" );
	  		Qry.CreateVariable( "work_mode", otString, term.type );
	  		Qry.CreateVariable( "code", otString, term_name );
	  		err++;
	  		Qry.Execute();
	  		err++;
	  		if ( !Qry.RowCount() ) {
    			if ( term.type == "П" )
	    			term_name = "G0" + term.name;
		    	else
		  	  	term_name = "R0" + term.name;
			    Qry.SetVariable( "code", term_name );
			    err++;
			    Qry.Execute();
			    err++;
			    if ( !Qry.RowCount() )
				    throw Exception( "Неизвестная стойка, значение=%s", term.name.c_str() );
		  	}
		  	term.name = Qry.FieldAsString( "desk" );
				i += 4;
       	tmp = linestr.substr( i, 1 );
	   	  tmp = TrimString( tmp );
	      if ( tmp.empty() || StrToInt( tmp.c_str(), term.pr_del ) == EOF || term.pr_del < 0 || term.pr_del > 1 )
	        throw Exception( "Ошибка формата признака удаления стойки, значение=%s", tmp.c_str() );
	      fl.terms.push_back( term );
		  }
		  catch( Exception &e ) {
		  	i = old_i + 1 + 4;
		  	if ( fl.invalid_term.empty() )
		  		fl.invalid_term = e.what();
		  }
	    i++;
	    tmp = linestr.substr( i, 1 );
	   	if ( tmp[ 0 ] != ';' )
    		throw Exception( "Ошибка формата маршрута. Ожидался символ ';', встретился символ	%c (2)", linestr[ i ] );
      i++;
		}
	}
	err++;
	bool overload_alarm = false;
	// запись в БД
	TQuery QrySet(&OraSession);
	QrySet.SQLText =
     "BEGIN "
     " sopp.set_flight_sets(:point_id,:use_seances);"
     " UPDATE trip_sets SET max_commerce=:max_commerce WHERE point_id=:point_id;"
     "END;";
	QrySet.DeclareVariable( "point_id", otInteger );
	QrySet.CreateVariable( "use_seances", otInteger, (int)USE_SEANCES() );
	QrySet.DeclareVariable( "max_commerce", otInteger );


	Qry.Clear();
	Qry.SQLText =
	 "SELECT move_id,point_id,craft,bort,scd_out,est_out,act_out,litera,park_out,pr_del "
	 " FROM points WHERE airline=:airline AND flt_no=:flt_no AND "
	 "                   ( suffix IS NULL AND :suffix IS NULL OR suffix=:suffix ) AND "
	 "                   airp=:airp AND "
	 "  scd_out >= TRUNC(:scd_out) AND scd_out < TRUNC(:scd_out) + 1";
	Qry.CreateVariable( "airline", otString, fl.airline );
	Qry.CreateVariable( "flt_no", otInteger, fl.flt_no );
	if ( fl.suffix.empty() )
	  Qry.CreateVariable( "suffix", otString, FNull );
	else
		Qry.CreateVariable( "suffix", otString, fl.suffix );
	Qry.CreateVariable( "airp", otString, "ВНК" );
  Qry.CreateVariable( "airline", otString, fl.airline );
	Qry.CreateVariable( "scd_out", otDate, fl.scd );
	err++;
	Qry.Execute();
	err++;
ProgTrace( TRACE5, "airline=%s, flt_no=%d, suffix=%s, scd_out=%s, insert=%d", fl.airline.c_str(), fl.flt_no,
	           fl.suffix.c_str(), DateTimeToStr( fl.scd ).c_str(), Qry.Eof );
	int move_id, new_tid, point_id;
	bool pr_insert = Qry.Eof;
	if ( pr_insert ) {
		if ( fl.craft.empty() )
			throw Exception( "Не задан тип ВС" );
		else
			if ( pr_craft_error )
				throw Exception( "Неизвестный тип ВС, значение=%s", fl.craft.c_str() );
	}
	else
		if ( pr_craft_error )
			fl.craft.clear(); // очищаем значение типа ВС - это не должно попасть в БД
 	TIDQry.SQLText = "SELECT tid__seq.nextval n FROM dual ";
	POINT_IDQry.SQLText = "SELECT point_id.nextval point_id FROM dual";
	if ( pr_insert ) { // insert
    Qry.Clear();
    Qry.SQLText =
     "BEGIN "\
     " SELECT move_id.nextval INTO :move_id from dual; "\
     " INSERT INTO move_ref(move_id,reference)  SELECT :move_id, :reference FROM dual; "\
     "END;";
    Qry.DeclareVariable( "move_id", otInteger );
    Qry.CreateVariable( "reference", otString, FNull );
    err++;
    Qry.Execute();
    err++;
    move_id = Qry.GetVariableAsInteger( "move_id" );
    err++;
    TIDQry.Execute();
    err++;
    new_tid = TIDQry.FieldAsInteger( "n" );
    err++;
    POINT_IDQry.Execute();
    err++;
    point_id = POINT_IDQry.FieldAsInteger( "point_id" );
    string lmes = "Ввод нового рейса ";
    lmes +=  fl.airline + IntToString( fl.flt_no ) + fl.suffix + ", маршрут ";
    for ( vector<AODB_Dest>::iterator it=fl.dests.begin(); it!=fl.dests.end(); it++ ) {
    	if ( it != fl.dests.begin() )
    		lmes += "-";
      lmes += it->airp;
    }
    err++;
    reqInfo->MsgToLog( lmes, evtDisp, move_id, point_id );
    err++;
    reqInfo->MsgToLog( string( "Ввод нового пункта " ) + "ВНК", evtDisp, move_id, point_id );
    err++;
    Qry.Clear();
    Qry.SQLText =
     "INSERT INTO points(move_id,point_id,point_num,airp,pr_tranzit,first_point,airline,flt_no,suffix,"\
     "                   craft,bort,scd_in,est_in,act_in,scd_out,est_out,act_out,trip_type,litera,"\
     "                   park_in,park_out,pr_del,tid,remark,pr_reg,airline_fmt,airp_fmt,craft_fmt,suffix_fmt) "\
     " VALUES(:move_id,:point_id,:point_num,:airp,:pr_tranzit,:first_point,:airline,:flt_no,:suffix,"\
     "        :craft,:bort,:scd_in,:est_in,:act_in,:scd_out,:est_out,:act_out,:trip_type,:litera,"\
     "        :park_in,:park_out,:pr_del,:tid,:remark,:pr_reg,0,0,0,0)";
    Qry.CreateVariable( "move_id", otInteger, move_id );
    Qry.CreateVariable( "point_id", otInteger, point_id );
    Qry.CreateVariable( "point_num", otInteger, 0 );
    Qry.CreateVariable( "airp", otString, "ВНК" );
    Qry.CreateVariable( "pr_tranzit", otInteger, 0 );
    Qry.CreateVariable( "first_point", otInteger, FNull );
    Qry.CreateVariable( "airline", otString, fl.airline );
    Qry.CreateVariable( "flt_no", otInteger, fl.flt_no );
    if ( fl.suffix.empty() )
      Qry.CreateVariable( "suffix", otString, FNull );
    else
    	Qry.CreateVariable( "suffix", otString, fl.suffix );
    Qry.CreateVariable( "craft", otString, fl.craft );
    Qry.CreateVariable( "bort", otString, fl.bort );
    Qry.CreateVariable( "scd_in", otDate, FNull );
    Qry.CreateVariable( "est_in", otDate, FNull );
    Qry.CreateVariable( "act_in", otDate, FNull );
    Qry.CreateVariable( "scd_out", otDate, fl.scd );
    if ( fl.est > NoExists )
      Qry.CreateVariable( "est_out", otDate, fl.est );
    else
    	Qry.CreateVariable( "est_out", otDate, FNull );
    if ( fl.act > NoExists )
      Qry.CreateVariable( "act_out", otDate, fl.act );
    else
    	Qry.CreateVariable( "act_out", otDate, FNull );
    Qry.CreateVariable( "trip_type", otString, fl.trip_type );
    Qry.CreateVariable( "litera", otString, fl.litera );
    Qry.CreateVariable( "park_in", otString, FNull );
    Qry.CreateVariable( "park_out", otString, fl.park_out );
/*    if ( fl.pr_del )
    	Qry.CreateVariable( "pr_del", otInteger, -1 );
    else*/
   	if ( fl.pr_cancel )
    	Qry.CreateVariable( "pr_del", otInteger, 1 );
    else
    	Qry.CreateVariable( "pr_del", otInteger, 0 );
    Qry.CreateVariable( "tid", otInteger, new_tid );
    Qry.CreateVariable( "remark", otString, FNull );
    Qry.CreateVariable( "pr_reg", otInteger, 1 );
    err++;
    Qry.Execute();
    err++;
    int num = 0;
    for ( vector<AODB_Dest>::iterator it=fl.dests.begin(); it!=fl.dests.end(); it++ ) {
    	num++;
    	err++;
      POINT_IDQry.Execute();
      err++;
      Qry.SetVariable( "point_id", POINT_IDQry.FieldAsInteger( "point_id" ) );
      Qry.SetVariable( "point_num", num );
      Qry.SetVariable( "airp", it->airp );
      Qry.SetVariable( "pr_tranzit", 0 );
      Qry.SetVariable( "first_point", point_id );
      if ( it == fl.dests.end() - 1 ) {
      	Qry.SetVariable( "airline", FNull );
        Qry.SetVariable( "flt_no", FNull );
        Qry.SetVariable( "suffix", FNull );
        Qry.SetVariable( "craft", FNull );
        Qry.SetVariable( "bort", FNull );
        Qry.SetVariable( "park_out", FNull );
        Qry.SetVariable( "trip_type", FNull );
        Qry.SetVariable( "litera", FNull );
        Qry.SetVariable( "pr_reg", 0 );
      }
      Qry.SetVariable( "scd_out", FNull );
      Qry.SetVariable( "est_out", FNull );
      Qry.SetVariable( "act_out", FNull );
      Qry.SetVariable( "park_out", FNull );
      if ( it->pr_del )
      	Qry.SetVariable( "pr_del", -1 );
      else
    		Qry.SetVariable( "pr_del", 0 );
    	err++;
      TIDQry.Execute();
      err++;
      Qry.SetVariable( "tid", TIDQry.FieldAsInteger( "n" ) );
      err++;
      Qry.Execute();
      err++;
      reqInfo->MsgToLog( string( "Ввод нового пункта " ) + it->airp, evtDisp, move_id, POINT_IDQry.FieldAsInteger( "point_id" ) );
    }
    // создаем времена технологического графика только для пункта вылета из ВНК и далее по маршруту
    QrySet.SetVariable( "point_id", point_id );
    QrySet.SetVariable( "max_commerce", fl.max_load );
		err++;
		QrySet.Execute();
		err++;
	}
	else { // update
		bool change_comp=false;
		string remark;
	  AODB_Flight old_fl;
		point_id = Qry.FieldAsInteger( "point_id" );
		move_id = Qry.FieldAsInteger( "move_id" );
		old_fl.craft = Qry.FieldAsString( "craft" );
		old_fl.bort = Qry.FieldAsString( "bort" );
		if ( Qry.FieldIsNULL( "scd_out" ) )
			old_fl.scd = NoExists;
		else
		  old_fl.scd = Qry.FieldAsDateTime( "scd_out" );
		if ( Qry.FieldIsNULL( "est_out" ) )
			old_fl.est = NoExists;
		else
		  old_fl.est = Qry.FieldAsDateTime( "est_out" );
		if ( Qry.FieldIsNULL( "act_out" ) )
			old_fl.act = NoExists;
		else
		  old_fl.act = Qry.FieldAsDateTime( "act_out" );
		old_fl.litera = Qry.FieldAsString( "litera" );
		old_fl.park_out = Qry.FieldAsString( "park_out" );
		old_fl.pr_del = ( Qry.FieldAsInteger( "pr_del" ) == -1 );
		old_fl.pr_cancel = ( Qry.FieldAsInteger( "pr_del" ) == 1 );
		Qry.Clear();
    Qry.SQLText =
     "BEGIN "
     " UPDATE move_ref SET move_id=move_id WHERE move_id=:move_id; "
     " UPDATE points SET move_id=move_id WHERE move_id=:move_id; "
     "END;";
    Qry.CreateVariable( "move_id", otInteger, move_id );
    err++;
    Qry.Execute(); // лочим
    err++;
    Qry.Clear();
    Qry.SQLText =
     "UPDATE points "
     " SET craft=NVL(craft,:craft),bort=NVL(bort,:bort),est_out=:est_out,act_out=:act_out,litera=:litera, "
     "     park_out=:park_out "
     " WHERE point_id=:point_id";
    Qry.CreateVariable( "point_id", otInteger, point_id );
    Qry.CreateVariable( "craft", otString, fl.craft );
 	  if ( fl.craft != old_fl.craft ) {
	  	if ( !old_fl.craft.empty() ) {
/* 	  	  remark += " изм. типа ВС с " + old_fl.craft;
 	  	  if ( !fl.craft.empty() )
 	  	    reqInfo->MsgToLog( string( "Изменение типа ВС на " ) + fl.craft + " порт ВНК" , evtDisp, move_id, point_id );*/
 	  	}
 	  	else {
 	  		reqInfo->MsgToLog( string( "Назначение ВС " ) + fl.craft + " порт ВНК" , evtDisp, move_id, point_id );
 	  		change_comp = true;
 	  	}
 	  }
 	  Qry.CreateVariable( "bort", otString, fl.bort );
 	  if ( fl.bort != old_fl.bort ) {
 	  	if ( !old_fl.bort.empty() ) {
/* 	  	  remark += " изм. борта с " + old_fl.bort;
 	  	  if ( !fl.bort.empty() )
 	  	    reqInfo->MsgToLog( string( "Изменение борта на " ) + fl.bort + " порт ВНК", evtDisp, move_id, point_id );*/
 	  	}
 	  	else {
 	  		reqInfo->MsgToLog( string( "Назначение борта " ) + fl.bort + " порт ВНК", evtDisp, move_id, point_id );
 	  		change_comp = true;
 	  	}
 	  }
 	  string tl;
 	  if ( fl.est > NoExists )
 	    Qry.CreateVariable( "est_out", otDate, fl.est );
 	  else
 	  	Qry.CreateVariable( "est_out", otDate, FNull );
 	  if ( fl.act > NoExists )
 	  	Qry.CreateVariable( "act_out", otDate, fl.act );
 	  else
 	    Qry.CreateVariable( "act_out", otDate, FNull );
 	  Qry.CreateVariable( "litera", otString, fl.litera );
 	  Qry.CreateVariable( "park_out", otString, fl.park_out );
 	  if ( fl.est != old_fl.est ) {
 	  	if ( fl.est > NoExists )
 	  	  tl += string("Расч. время ") + DateTimeToStr( fl.est, "dd hh:nn" );
 	  	else
 	  		tl += "Удаление расч. времени";
 	  }
 	  err++;
 	  if ( fl.act != old_fl.act ) {
 	  	if ( !tl.empty() )
 	  		tl += ",";
        if ( fl.act > NoExists ) {
            try {
            	exec_stage( point_id, sTakeoff );
            }
            catch( Exception &E ) {
                ProgError( STDLOG, "AODB exec_stage: Takeoff. Exception: %s", E.what() );
            }
            catch( std::exception &E ) {
                ProgError( STDLOG, "AODB exec_stage: Takeoff. std::exception: %s", E.what() );
            }
            catch( ... ) {
                ProgError( STDLOG, "AODB exec_stage: Unknown error" );
            };
            tl += string("Проставление факт. времени вылета ") + DateTimeToStr( fl.act, "hh:nn dd.mm.yy" ) + string(" (UTC)");
        } else
 	  		tl += "Отмена факта вылета";
 	  }
 	  if ( fl.litera != old_fl.litera ) {
 	  	if ( !tl.empty() )
 	  		tl += ",";
 	  	tl += string("Литера ") + fl.litera;
 	  }
 	  if ( fl.park_out != old_fl.park_out ) {
 	  	if ( !tl.empty() )
 	  		tl += ",";
 	  	if ( fl.park_out.empty() )
 	  		tl += "Стоянка удалена";
 	  	else
  	    tl += "Стоянка " + fl.park_out;
 	  }
/* 	  if ( fl.pr_cancel != old_fl.pr_cancel ) {
 	  	if ( !tl.empty() )
 	  		tl += ",";
 	  	if ( fl.pr_cancel )
 	  	  tl += "Отмена рейса ";
 	  	else
 	  		tl += "Удаление отмены рейса";
 	  }*/
 	  if ( !tl.empty() ) {
 	  	reqInfo->MsgToLog( tl, evtDisp, move_id, point_id );
 	  }
/* 	  if ( fl.pr_del )
 	  	 Qry.CreateVariable( "pr_del", otInteger, -1 );
 	  else
 	  	if ( fl.pr_cancel )
 	  		Qry.CreateVariable( "pr_del", otInteger, 1 );
 	  	else
 	  		Qry.CreateVariable( "pr_del", otInteger, 0 );
 	  if ( fl.pr_del != old_fl.pr_del ) {
 	  	if ( fl.pr_del )
 	  		;*//*reqInfo->MsgToLog( string( "Удаление рейса ВНК" ), evtDisp, move_id, point_id );*/
/* 	  	else
 	  		reqInfo->MsgToLog( string( "Добавление рейса ВНК" ), evtDisp, move_id, point_id );
 	  }
 	  if ( fl.pr_cancel != old_fl.pr_cancel ) {
 	  	if ( fl.pr_cancel )
 	  		reqInfo->MsgToLog( string( "Отмена рейса ВНК" ), evtDisp, move_id, point_id );
 	  	else
 	  		reqInfo->MsgToLog( string( "Удаление отмены рейса ВНК" ), evtDisp, move_id, point_id );
 	  }*/
 	  err++;
 	  Qry.Execute();
 	  err++;
 	  if ( change_comp )
 	  	SALONS::AutoSetCraft( point_id, fl.craft, -1 );
 	  // теперь работа с пунктами посадки
/*    int num = 0;
    int point_num = 0;*/
    vector<AODB_Dest> old_dests;
    Qry.Clear();
    Qry.SQLText = "SELECT point_num,airp,pr_del FROM points WHERE move_id=:move_id ORDER BY point_num";
    Qry.CreateVariable( "move_id", otInteger, move_id );
    err++;
    Qry.Execute();
    err++;
    while ( !Qry.Eof ) {
    	AODB_Dest d;
    	d.num = Qry.FieldAsInteger( "point_num" );
    	d.airp = Qry.FieldAsString( "airp" );
    	d.pr_del = ( Qry.FieldAsInteger( "pr_del" ) != 0 );
    	Qry.Next();
    }
/*    for ( vector<AODB_Dest>::iterator it=fl.dests.begin(),
    	    vecto; it!=fl.dests.end(); it++ ) {

    	if ( it->pr_del || it->pr_cancel ) {
    		Qry.Clear();
  		  Qry.SQLText =
  	   	 "SELECT COUNT(*) c FROM pax_grp,points "\
  		   " WHERE points.point_id=:point_id AND "\
  		   "       point_dep=:point_id AND bag_refuse=0 ";
  		  Qry.CreateVariable( "point_id", otInteger, point_id );
  		Qry.Execute();
  		if ( Qry.FieldAsInteger( "c" ) )
  			if ( id->pr_del == -1 )
  				throw UserException( string( "Нельзя удалить аэропорт " ) + id->airp + ". " + "Есть зарегистрированные пассажиры." );
  			else
  				throw UserException( string( "Нельзя отменить аэропорт " ) + id->airp + ". " + "Есть зарегистрированные пассажиры." );
    	}
    	num++;
    } 	    	 	  	                     */
    overload_alarm = Get_AODB_overload_alarm( point_id, fl.max_load );
    Qry.Clear();
    Qry.SQLText =
     "UPDATE trip_sets SET max_commerce=:max_commerce WHERE point_id=:point_id";
    Qry.CreateVariable( "point_id", otInteger, point_id );
    Qry.CreateVariable( "max_commerce", otInteger, fl.max_load );
    err++;
    Qry.Execute();
    err++;
  	QryTripInfo.SetVariable( "point_id", point_id );
    QryTripInfo.Execute();
    if ( !QryTripInfo.Eof ) {
    	fltInfo.Init(QryTripInfo);
    	Set_overload_alarm( point_id, Get_overload_alarm( point_id, fltInfo ) );
    }
	}

  Qry.Clear();
	Qry.SQLText =
	 "BEGIN "
	 " UPDATE aodb_points "
	 " SET aodb_point_id=:aodb_point_id "
	 " WHERE point_id=:point_id AND point_addr=:point_addr; "
	 " IF SQL%NOTFOUND THEN "
	 "  INSERT INTO aodb_points(aodb_point_id,point_addr,point_id,record,rec_no_pax,rec_no_bag,rec_no_flt,rec_no_unaccomp,overload_alarm) "
	 "    VALUES(:aodb_point_id,:point_addr,:point_id,NULL,-1,-1,-1,-1,0);"
	 " END IF; "
	 "END;";
	Qry.CreateVariable( "point_id", otInteger, point_id );
	Qry.CreateVariable( "point_addr", otString, point_addr );
	Qry.CreateVariable( "aodb_point_id", otFloat, fl.id );
	err++;
	Qry.Execute();
	err++;
  Set_AODB_overload_alarm( point_id, overload_alarm );
	// обновление времен технологического графика
  Qry.Clear();
	Qry.SQLText = "UPDATE trip_stages SET est=NVL(:scd,est) WHERE point_id=:point_id AND stage_id=:stage_id";
	Qry.CreateVariable( "point_id", otInteger, point_id );
	Qry.CreateVariable( "stage_id", otInteger, sOpenCheckIn );
	Qry.CreateVariable( "scd", otDate, fl.checkin_beg );
	err++;
	Qry.Execute();
	err++;
	Qry.SetVariable( "stage_id", sCloseCheckIn );
	Qry.SetVariable( "scd", fl.checkin_end );
	err++;
	Qry.Execute();
	err++;
	Qry.SetVariable( "stage_id", sOpenBoarding );
	Qry.SetVariable( "scd", fl.boarding_beg );
	err++;
	Qry.Execute();
	err++;
	Qry.SetVariable( "stage_id", sCloseBoarding );
	Qry.SetVariable( "scd", fl.boarding_end );
	err++;
	Qry.Execute();
	err++;
	// обновление стоек регистрации и выходов на покадку
	Qry.Clear();
	Qry.SQLText =
	 "BEGIN "
	 " :pr_change := 0; "
	 " IF :pr_del != 0 THEN "
	 "   DELETE trip_stations	WHERE point_id=:point_id AND desk=:desk AND work_mode=:work_mode; "
	 "   :pr_change := 1; "
	 " ELSE "
	 "  UPDATE trip_stations SET desk=desk WHERE point_id=:point_id AND desk=:desk AND work_mode=:work_mode; "
 	 "  IF SQL%NOTFOUND THEN "
 	 "   INSERT INTO trip_stations(point_id,desk,work_mode,pr_main) "
	 "    VALUES(:point_id,:desk,:work_mode,0); "
	 "   :pr_change := 1; "
 	 "  END IF; "
	 " END IF; "
	 "END;";
	Qry.CreateVariable( "point_id", otInteger, point_id );
	Qry.DeclareVariable( "pr_del", otInteger );
	Qry.DeclareVariable( "desk", otString );
	Qry.DeclareVariable( "work_mode", otString );
	Qry.DeclareVariable( "pr_change", otInteger );
//rogTrace( TRACE5, "fl.terms.size()=%d, point_id=%d", fl.terms.size(), point_id );
  string reg, reg_del, brd, brd_del;
  bool pr_change_reg = false, pr_change_brd = false;
	for ( vector<AODB_Term>::iterator it=fl.terms.begin(); it!=fl.terms.end(); it++ ) {
		Qry.SetVariable( "desk", it->name );
		Qry.SetVariable( "work_mode", it->type );
		Qry.SetVariable( "pr_del", it->pr_del );
//ProgTrace( TRACE5, "desk=%s, work_mode=%s, pr_del=%d", it->name.c_str(), it->type.c_str(), it->pr_del );
    err++;
		Qry.Execute();
		err++;
		if ( it->type == "Р" )
			pr_change_reg = pr_change_reg || Qry.GetVariableAsInteger( "pr_change" );
		else
			pr_change_brd = pr_change_brd || Qry.GetVariableAsInteger( "pr_change" );
		if ( it->pr_del )
		  if ( it->type == "Р" )
		  	reg_del = reg_del + " " + it->name;
		  else
		  	brd_del = brd_del + " " + it->name;
		else
		  if ( it->type == "Р" )
		  	reg = reg + " " + it->name;
		  else
		  	brd = brd + " " + it->name;
	}
	if ( pr_change_reg ) {
		if ( !reg_del.empty() )
	    reqInfo->MsgToLog( string( "Удаление стоек регистрации" ) + reg_del, evtDisp, move_id, point_id );
		if ( !reg.empty() )
	    reqInfo->MsgToLog( string( "Назначение стоек регистрации" ) + reg, evtDisp, move_id, point_id );
	}
	if ( pr_change_brd ) {
		if ( !brd_del.empty() )
		  reqInfo->MsgToLog( string( "Удаление выходов на посадку" ) + brd_del, evtDisp, move_id, point_id );
		if ( !brd.empty() )
		  reqInfo->MsgToLog( string( "Назначение выходов на посадку" ) + brd, evtDisp, move_id, point_id );
	}
}
catch(...){
	ProgError( STDLOG, "AODB error=%d", err );
	throw;
}
}


/*
 create table aodb_spp (
 name varchar2(50) not null,
 receiver varchar2(6) not null,
 rec_no number(9),
 value varchar2(4000),
 error varchar2(4000) );

 create table aodb_airlines (
  code varchar2(5) not null,
  airline varchar2(3) not null );

 create table aodb_airps (
  code varchar2(3) not null,
  airp varchar2(3) not null );


 create table aodb_crafts (
  code varchar2(10) not null,
  craft varchar2(3) not null );

 create table aodb_liters (
  code varchar2(3) not null,
  litera varchar2(3) not null );
 */

void ParseAndSaveSPP( const std::string &filename, const std::string &canon_name, const std::string airline,
	                    std::string &fd, const string &convert_aodb )
{
	TQuery QryLog( &OraSession );
	QryLog.SQLText =
	 "INSERT INTO aodb_events(filename,point_addr,airline,rec_no,record,msg,time,type) "
	 " SELECT :filename,:point_addr,:airline,:rec_no,:record,:msg,system.UTCSYSDATE,:type FROM dual ";
 	QryLog.CreateVariable( "filename", otString, filename );
	QryLog.CreateVariable( "point_addr", otString, canon_name );
	QryLog.CreateVariable( "airline", otString, airline );
	QryLog.DeclareVariable( "rec_no", otInteger );
	QryLog.DeclareVariable( "record", otString );
	QryLog.DeclareVariable( "msg", otString );
	QryLog.DeclareVariable( "type", otString );
	string errs;
	string linestr;
	char c_n = 13, c_a = 10;
	int max_rec_no = -1;
  while ( !fd.empty() ) {
  	std::string::size_type i = fd.find( c_n );
  	if ( i == string::npos ) {
  		linestr = fd;
  		fd.clear();
  	}
  	else {
  	  linestr = fd.substr( 0, i );
  	  if ( i < fd.length() - 1 && fd[ i + 1 ] == c_a ) {
  		  i++;
  		}
  	  fd.erase( 0, i + 1 );
  	}
    AODB_Flight fl;
    try {
    	fl.rec_no = NoExists;
    	if ( !convert_aodb.empty() ) {
    		try {
    		  linestr = ConvertCodepage( linestr, convert_aodb, "CP866" );
    		}
	      catch( EConvertError &E ) {
	      	string l;
	      	try {
	      		l = ConvertCodepage( linestr.substr( REC_NO_IDX, REC_NO_LEN ), convert_aodb, "CP866" );
	      	}
	        catch( EConvertError &E ) {
	        	throw Exception( string("Ошибка перекодировки рейса") );
	        }
	      	throw Exception( string("Ошибка перекодировки рейса, строка ") + l );
	      }
      }
      ParseFlight( canon_name, linestr, fl );
      if ( !fl.invalid_term.empty() )
      	throw Exception( fl.invalid_term );
      QryLog.SetVariable( "rec_no", fl.rec_no );
      if ( linestr.empty() )
      	QryLog.SetVariable( "record", "empty line!" );
      else
        QryLog.SetVariable( "record", linestr );
    	QryLog.SetVariable( "msg", "ok" );
    	QryLog.SetVariable( "type", EncodeEventType( ASTRA::evtFlt ) );
      QryLog.Execute();
    }
    catch( Exception &e ) {
      if ( fl.rec_no == NoExists )
      	QryLog.SetVariable( "rec_no", -1 );
      else
        QryLog.SetVariable( "rec_no", fl.rec_no );
      if ( linestr.empty() )
      	QryLog.SetVariable( "record", "empty line!" );
      else
        QryLog.SetVariable( "record", linestr );
    	QryLog.SetVariable( "msg", e.what() );
    	QryLog.SetVariable( "type", EncodeEventType( ASTRA::evtProgError ) );
      QryLog.Execute();
      if ( !errs.empty() )
      	errs += c_n/* + c_a*/;
      if ( fl.rec_no == NoExists )
        errs += string( "Ошибка разбора строки: " ) + string(e.what()).substr(0,120);
      else
      	errs += string( "Ошибка разбора строки " ) + IntToString( fl.rec_no ) + " : " + string(e.what()).substr(0,120);
    }
    if ( fl.rec_no > NoExists )
      max_rec_no = fl.rec_no;
  }
	TQuery Qry( &OraSession );
	Qry.SQLText = "UPDATE aodb_spp_files SET rec_no=:rec_no WHERE filename=:filename AND point_addr=:point_addr AND airline=:airline";
	Qry.CreateVariable( "rec_no", otInteger, max_rec_no );
	Qry.CreateVariable( "filename", otString, filename );
	Qry.CreateVariable( "point_addr", otString, canon_name );
	Qry.CreateVariable( "airline", otString, airline );
	Qry.Execute();
	if ( !errs.empty() )
	 showProgError( errs );
}

bool BuildAODBTimes( int point_id, const std::string &point_addr, TFileDatas &fds )
{
	TFileData fd;
	TQuery Qry( &OraSession );
	Qry.SQLText =
	 "SELECT aodb_point_id,airline||flt_no||suffix trip,scd_out,aodb_points.overload_alarm, "
	 "       rec_no_flt,record "
	 " FROM points, aodb_points "
	 " WHERE points.point_id=:point_id AND "
	 "       points.point_id=aodb_points.point_id(+) AND "
	 "       :point_addr=aodb_points.point_addr(+) ";
/*	 "       ( gtimer.get_stage(points.point_id,1) >= :stage2 OR record IS NOT NULL )";
	Qry.CreateVariable( "stage2", otInteger, sPrepCheckIn );*/
	Qry.CreateVariable( "point_id", otInteger, point_id );
	Qry.CreateVariable( "point_addr", otString, point_addr );
	Qry.Execute();
	if ( Qry.Eof )
		return false;
  string flight = Qry.FieldAsString( "trip" );
	string region = CityTZRegion( "МОВ" );
	TDateTime scd_out = UTCToLocal( Qry.FieldAsDateTime( "scd_out" ), region );
	double aodb_point_id = Qry.FieldAsFloat( "aodb_point_id" );
	int rec_no;
	string old_res;
	if ( Qry.Eof || Qry.FieldIsNULL( "rec_no_flt" ) )
		rec_no = 0;
	else {
	  rec_no = Qry.FieldAsInteger( "rec_no_flt" ) + 1;
	  old_res = Qry.FieldAsString( "record" );
	}
	int checkin = 0, boarding = 0;
	TMapTripStages stages;
	TTripStages::LoadStages( point_id, stages );
	ostringstream record;
	record<<setfill(' ');
	if ( stages[ sOpenCheckIn ].act > NoExists )
		record<<setw(16)<<DateTimeToStr( UTCToLocal( stages[ sOpenCheckIn ].act, region ), "dd.mm.yyyy hh:nn" );
	else
		record<<setw(16)<<" ";
	if ( stages[ sCloseCheckIn ].act > NoExists )
		record<<setw(16)<<DateTimeToStr( UTCToLocal( stages[ sCloseCheckIn ].act, region ), "dd.mm.yyyy hh:nn" );
	else
		record<<setw(16)<<" ";
	if ( stages[ sOpenBoarding ].act > NoExists )
		record<<setw(16)<<DateTimeToStr( UTCToLocal( stages[ sOpenBoarding ].act, region ), "dd.mm.yyyy hh:nn" );
	else
		record<<setw(16)<<" ";
	if ( stages[ sCloseBoarding ].act > NoExists )
		record<<setw(16)<<DateTimeToStr( UTCToLocal( stages[ sCloseBoarding ].act, region ), "dd.mm.yyyy hh:nn" );
	else
		record<<setw(16)<<" ";
  checkin = ( stages[ sOpenCheckIn ].act > NoExists && stages[ sCloseCheckIn ].act == NoExists );
	boarding = ( stages[ sOpenBoarding ].act > NoExists && stages[ sCloseBoarding ].act == NoExists );
	record<<setw(1)<<checkin<<setw(1)<<boarding<<setw(1)<<Qry.FieldAsInteger( "overload_alarm" );
	TQuery StationsQry( &OraSession );

	StationsQry.Clear();
	StationsQry.SQLText =
	 "SELECT name, start_time FROM trip_stations, stations "
	 " WHERE point_id=:point_id AND trip_stations.work_mode='Р' AND "
	 "       trip_stations.desk=stations.desk AND trip_stations.work_mode=stations.work_mode";
	StationsQry.CreateVariable( "point_id", otInteger, point_id );
	StationsQry.Execute();
	while ( !StationsQry.Eof ) {
    string term = StationsQry.FieldAsString( "name" );
    if ( !term.empty() && term[0] == 'R' )
    	term = term.substr( 1, term.length() - 1 );
		record<<";"<<"Р"<<setw(4)<<term.substr(0,4)<<setw(1)<<(int)!StationsQry.FieldIsNULL( "start_time" );
		StationsQry.Next();
	}
	if ( Qry.Eof || record.str() != string( Qry.FieldAsString( "record" ) ) ) {
		ostringstream r;
		if ( aodb_point_id )
		  r<<std::fixed<<setw(6)<<rec_no<<setw(10)<<setprecision(0)<<aodb_point_id;
		else
			r<<std::fixed<<setw(6)<<rec_no<<setw(10)<<"";
		r<<setw(10)<<flight.substr(0,10)<<setw(16)<<DateTimeToStr( scd_out, "dd.mm.yyyy hh:nn" )<<record.str();
		Qry.Clear();
  	Qry.SQLText =
  	 "BEGIN "
   	 " UPDATE aodb_points SET record=:record,rec_no_flt=NVL(rec_no_flt,-1)+1 "
   	 "  WHERE point_id=:point_id AND point_addr=:point_addr; "
   	 "  IF SQL%NOTFOUND THEN "
   	 "    INSERT INTO aodb_points(point_id,point_addr,aodb_point_id,record,rec_no_flt,rec_no_pax,rec_no_bag,rec_no_unaccomp) "
   	 "      VALUES(:point_id,:point_addr,NULL,:record,0,-1,-1,-1); "
   	 "  END IF; "
   	 "END;";
   	Qry.CreateVariable( "point_id", otInteger, point_id );
   	Qry.CreateVariable( "point_addr", otString, point_addr );
   	Qry.CreateVariable( "record", otString, record.str() );
 	  Qry.Execute();
 	  fd.file_data = r.str() + "\n";
	}
  if ( !fd.file_data.empty() ) {
	  string p = flight + DateTimeToStr( scd_out, "yymmddhhnn" );
    fd.params[ PARAM_FILE_NAME ] =  p + "reg.txt";
	  fd.params[ NS_PARAM_EVENT_TYPE ] = EncodeEventType( ASTRA::evtFlt );
	  fd.params[ NS_PARAM_EVENT_ID1 ] = IntToString( point_id );
    fd.params[ PARAM_TYPE ] = VALUE_TYPE_FILE; // FILE
    fds.push_back( fd );
	}
	return !fds.empty();
}

bool createAODBFiles( int point_id, const std::string &point_addr, TFileDatas &fds )
{
	createAODBCheckInInfoFile( point_id, false, point_addr, fds );
	createAODBCheckInInfoFile( point_id, true, point_addr, fds );
	BuildAODBTimes( point_id, point_addr, fds );
	return !fds.empty();
}
/*
 возвращает true только в случае, когда есть перегрузка и нет изменения в max_commerce
*/
bool Get_AODB_overload_alarm( int point_id, int max_commerce )
{
	TQuery Qry( &OraSession );
	Qry.SQLText =
	  "SELECT max_commerce, aodb_points.overload_alarm FROM aodb_points, trip_sets "
	  " WHERE aodb_points.point_id=:point_id AND aodb_points.point_id=trip_sets.point_id";
	Qry.CreateVariable( "point_id", otInteger, point_id );
	Qry.Execute();
	return ( !Qry.Eof &&
	         Qry.FieldAsInteger( "max_commerce" ) == max_commerce &&
	         Qry.FieldAsInteger( "overload_alarm" ) );
}

void Set_AODB_overload_alarm( int point_id, bool overload_alarm )
{
	TQuery Qry( &OraSession );
	Qry.SQLText =
	  "UPDATE aodb_points SET overload_alarm=:overload_alarm WHERE point_id=:point_id";
	Qry.CreateVariable( "overload_alarm", otInteger, overload_alarm );
	Qry.CreateVariable( "point_id", otInteger, point_id );
	Qry.Execute();
}

void VerifyParseFlight( )
{
	std::string linestr =
"     1    184071     ЮТ437   01.09.2008 21:5001.09.2008 21:5001.09.2008 22:04 2   85134 11474    ТУ154М     8568101.09.2008 20:2001.09.2008 21:1001.09.2008 21:1001.09.2008 21:3000;1СКЧ0;Р  130;Р  140;Р  190;Р  200;Р  210;Р  220;Р  230;Р  240;Р  250;Р  260;Р  270;Р  350;Р  360;Р  370;Р 5010;Р 5020;Р 5030;Р 5040;Р 5050;Р 5060;П   40;";
  AODB_Flight fl;
  string point_addr = "DJEK";
  ProgTrace( TRACE5, "linestr=%s", linestr.c_str() );
  ParseFlight( point_addr, linestr, fl );
  tst();
}


