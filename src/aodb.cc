/*
create table aodb_files( point_id NUMBER(9), rec_no NUMBER(10) );
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
#define NICKNAME "DJEK"
#define NICKTRACE DJEK_TRACE
#include "slogger.h"
#include "base_tables.h"
#include "astra_consts.h"
#include "astra_utils.h"
#include "develop_dbf.h"
#include "helpcpp.h"
#include "misc.h"
#include "stages.h"


using namespace std;
using namespace EXCEPTIONS;
using namespace BASIC;
using namespace ASTRA;

struct AODB_STRUCT{
	int pax_id;
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
	std::string term; //2
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
};

void getRecord( int pax_id, const vector<AODB_STRUCT> &aodb_pax, const vector<AODB_STRUCT> &aodb_bag,
                string &res_checkin );
void createRecord( int point_id, int pax_id, const string &point_addr,
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
	string p = string( FlightQry.FieldAsString( "airline" ) ) +
	           FlightQry.FieldAsString( "flt_no" ) +
	           FlightQry.FieldAsString( "suffix" ) +
	           string( DateTimeToStr( FlightQry.FieldAsDateTime( "scd_out" ), "yymmddhhnn" ) );
	if ( pr_bag )
		p += 'b';
  params[ PARAM_FILE_NAME ] =  p + ".txt";
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

bool createAODBCheckInInfoFile( int point_id, 
                                std::map<std::string,std::string> &params, std::string &file_data/*,
                                std::map<std::string,std::string> &bag_params, std::string &bag_file_data*/ )
{
	string point_addr = params[PARAM_CANON_NAME];
	AODB_STRUCT STRAO;
	vector<AODB_STRUCT> prior_aodb_pax, aodb_pax;
	vector<AODB_STRUCT> prior_aodb_bag, aodb_bag;
	TQuery Qry(&OraSession);
	Qry.SQLText = 
	 "SELECT aodb_point_id,airline||flt_no||suffix trip,scd_out FROM points, aodb_points "
	 " WHERE points.point_id=:point_id AND points.point_id=aodb_points.point_id AND "
	 "       aodb_points.point_addr=:point_addr FOR UPDATE";
	Qry.CreateVariable( "point_id", otInteger, point_id );
	Qry.CreateVariable( "point_addr", otString, params[PARAM_CANON_NAME] );	
	Qry.Execute();
	if ( !Qry.RowCount() ) {
		ProgError( STDLOG, "Flight not found, point_id=%d", point_id );
		return false;
	}
	float aodb_point_id = Qry.FieldAsFloat( "aodb_point_id" );
	string flight = Qry.FieldAsString( "trip" );
	string region = CityTZRegion( "МОВ" );
	string scd_date = DateTimeToStr( UTCToLocal( Qry.FieldAsDateTime( "scd_out" ), region ), "dd.mm.yyyy hh:nn" );	 
	Qry.Clear();	
	Qry.SQLText = 
	 "SELECT pax_id, record FROM aodb_pax WHERE point_id=:point_id AND point_addr=:point_addr";
	Qry.CreateVariable( "point_id", otInteger, point_id );
	Qry.CreateVariable( "point_addr", otString, point_addr );
	Qry.Execute();
	STRAO.doit = false;
	while ( !Qry.Eof ) {
  	STRAO.pax_id = Qry.FieldAsInteger( "pax_id" );
		STRAO.record = string( Qry.FieldAsString( "record" ) );
		prior_aodb_pax.push_back( STRAO );	
		Qry.Next();
	}
	Qry.Clear();	 
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
	STRAO.unaccomp = false;
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
	BagQry.SQLText =
	 "SELECT weight,tags FROM unaccomp_bag WHERE point_dep=:point_id "
	 " ORDER BY id";
	BagQry.CreateVariable( "point_id", otInteger, point_id );
	BagQry.Execute();
	while ( !BagQry.Eof ) {
		
		BagQry.Next();
	}
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
	Qry.Clear();
	Qry.SQLText =	
	 "SELECT pax.pax_id,pax.reg_no,pax.surname||' '||pax.name name,pax_grp.grp_id,"
	 "       pax_grp.airp_arv,pax_grp.class,pax.refuse,"
	 "       pax.pers_type,pax.seat_no,DECODE(pax.seats,0,0,pax.seats-1) seats,"
	 "       ckin.get_excess(pax_grp.grp_id,pax.pax_id) excess,"
	 "       ckin.get_rkAmount(pax_grp.grp_id,pax.pax_id,rownum) rkamount,"
	 "       ckin.get_rkWeight(pax_grp.grp_id,pax.pax_id,rownum) rkweight,"
	 "       ckin.get_bagAmount(pax_grp.grp_id,pax.pax_id,rownum) bagamount,"
	 "       ckin.get_bagWeight(pax_grp.grp_id,pax.pax_id,rownum) bagweight,"
	 "       pax.pr_brd,ckin.get_main_pax_id(pax.grp_id) as main_pax_id "
	 " FROM pax_grp, pax "
	 " WHERE pax_grp.grp_id=pax.grp_id AND "
	 "       pax_grp.point_dep=:point_id";
	Qry.CreateVariable( "point_id", otInteger, point_id );
	Qry.Execute();	
	TQuery RemQry( &OraSession );
	RemQry.SQLText = "SELECT rem_code FROM pax_rem WHERE pax_id=:pax_id";
	RemQry.DeclareVariable( "pax_id", otInteger );		
	TQuery TimeQry( &OraSession );
	TimeQry.SQLText = 
	 "SELECT time as mtime,NVL(stations.name,events.station) station FROM events,stations "
	 " WHERE type='ПАС' AND id1=:point_id AND id2=:reg_no AND events.station=stations.desk(+) AND stations.work_mode=:work_mode "
	 " AND screen=:screen "
	 " ORDER BY time DESC,ev_order DESC";
	TimeQry.CreateVariable( "point_id", otInteger, point_id );
	TimeQry.DeclareVariable( "reg_no", otInteger );
	TimeQry.DeclareVariable( "screen", otString );
	TimeQry.DeclareVariable( "work_mode", otString );
	while ( !Qry.Eof ) {
		ostringstream record;
		record<<setfill(' ')<<std::fixed<<setw(10)<<aodb_point_id;
		record<<setfill(' ')<<std::fixed<<setw(10)<<flight;
		record<<setw(16)<<scd_date;
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
			RemQry.Next();
		}
		if ( !pr_ex )
			record<<0;
		record<<setw(1);
		switch ( DecodePerson( Qry.FieldAsString( "pers_type" ) ) ) {
			case ASTRA::adult:
				  record<<0;
				  break;
		  case ASTRA::baby:
		  	  record<<2;
		  	  break;
			default:
				  record<<1;
		}
		record<<setw(5)<<Qry.FieldAsString( "seat_no" );
		record<<setw(2)<<Qry.FieldAsInteger( "seats" );
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
		record<<setw(2)<<0; // РМ количество
		record<<setw(36)<<""; // Имя ребенка
		record<<setw(60)<<""; // ДОП. Инфо
		record<<setw(1)<<0; // международный багаж
//		record<<setw(1)<<0; // трансатлантический багаж :)
// стойка рег. + время рег. + выход на посадку + время прохода на посадку
    TimeQry.SetVariable( "reg_no", Qry.FieldAsInteger( "reg_no" ) ); 
    TimeQry.SetVariable( "screen", "AIR.EXE" );
    TimeQry.SetVariable( "work_mode", "Р" );
    TimeQry.Execute();
    string term;
    TDateTime t;
    if ( TimeQry.Eof ) {
    	t = NoExists;
    }
    else {
    	term = TimeQry.FieldAsString( "station" );
    	if ( !term.empty() && term[0] == 'R' )
    		term = term.substr( 1, term.length() - 1 );
    	t = TimeQry.FieldAsDateTime( "mtime" );
    }    	
    if ( t == NoExists )
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
		STRAO.record = record.str();
		STRAO.pax_id = Qry.FieldAsInteger( "pax_id" );
		aodb_pax.push_back( STRAO );
//!!!		record<<setw(1)<<";"; // конец записи по пассажиру
		// ручная кладь
//		ProgTrace( TRACE5, "main_pax_id=%d, pax_id=%d", Qry.FieldAsInteger( "main_pax_id" ), Qry.FieldAsInteger( "pax_id" ) );
		if ( Qry.FieldAsInteger( "main_pax_id" ) == Qry.FieldAsInteger( "pax_id" ) ) {
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
//		  	ProgTrace( TRACE5, "record rk=%s", record_bag.str().c_str() );
		  	BagQry.Next();
		  }		  
//!!!		  record<<setw(1)<<";";
		  // багаж
		  BagQry.SetVariable( "pr_cabin", 0 );
		  BagQry.Execute();
		  int pr_first=true;
		  while ( !BagQry.Eof ) {
		  	ostringstream record_bag;
		  	record_bag<<setfill(' ')<<std::fixed;
		  	DecodeBagType( BagQry.FieldAsInteger( "bag_type" ), code, type_name );
		  	record_bag<<setw(2)<<code<<setw(20)<<type_name.substr(0,20);
		  	record_bag<<setw(10)<<setprecision(0)<<BagQry.FieldAsFloat( "no" );
		  	record_bag<<setw(2)<<string(BagQry.FieldAsString( "color" )).substr(0,2);		  	
		 // 	if ( pr_first ) {
		  		pr_first = false;
		  		record_bag<<setw(4)<<BagQry.FieldAsInteger( "weight" );
/*		  	}
		  	else {
		  		record_bag<<setw(4)<<0;		  		
		  	}*/
		  	//record_bag<<setw(10)<<""; // номер контейнера
	  		//record<<setw(1)<<0; // снятие
	  		STRAO.pax_id = Qry.FieldAsInteger( "pax_id" );
	  		STRAO.num = BagQry.FieldAsInteger( "bag_num" );
	  		STRAO.record = record_bag.str();
	  		STRAO.pr_cabin = 0;
		  	aodb_bag.push_back( STRAO );
//		  	ProgTrace( TRACE5, "record bag=%s", record_bag.str().c_str() );
		  	BagQry.Next();
		  }		  
//		  record<<setw(1)<<";";
		}
		else {
//			record<<setw(1)<<";"<<";"; // нет ручной клади и багажа
	  }
		Qry.Next();
	}
	// далее сравнение 2-х слепков, выяснение, что добавилось, что удалилось, что изменилось
	// формирование данных для отпавки + их запись как новый слепок?	
//	ProgTrace( TRACE5, "aodb_pax.size()=%d, prior_aodb_pax.size()=%d",aodb_pax.size(), prior_aodb_pax.size() );
  string res_checkin, /*res_bag,*/ prior_res_checkin/*, prior_res_bag*/;
  bool ch_pax/*, ch_bag*/;
	for ( vector<AODB_STRUCT>::iterator p=aodb_pax.begin(); p!=aodb_pax.end(); p++ ) {
		getRecord( p->pax_id, aodb_pax, aodb_bag, res_checkin/*, res_bag*/ );
		getRecord( p->pax_id, prior_aodb_pax, prior_aodb_bag, prior_res_checkin/*, prior_res_bag*/ );		
		ch_pax = res_checkin != prior_res_checkin;
		//ch_bag = res_bag != prior_res_bag;
		if ( ch_pax/*|| ch_bag*/ ) {
//		if ( getRecord( p->pax_id, aodb_pax, aodb_bag ) != getRecord( p->pax_id, prior_aodb_pax, prior_aodb_bag ) ) {
//			ProgTrace(TRACE5, "p->doit=%d, pax_id=%d", p->doit, p->pax_id );
			createRecord( point_id, p->pax_id, point_addr, aodb_pax, aodb_bag, prior_aodb_pax, prior_aodb_bag,
			              res_checkin/*, res_bag*/ );
			if ( ch_pax )
				file_data += res_checkin;
/*			if ( ch_bag ) {
				//ProgTrace( TRACE5, "res_bag=%s, prior_res_bag=%s", res_bag.c_str(), prior_res_bag.c_str() );				
				bag_file_data += res_bag;        
			}*/
			ProgTrace(TRACE5, "pax_id=%d", p->pax_id );
	  }
  }
	for ( vector<AODB_STRUCT>::iterator p=prior_aodb_pax.begin(); p!=prior_aodb_pax.end(); p++ ) {
//		ProgTrace(TRACE5, "p->doit=%d, pax_id=%d", p->doit, p->pax_id );
		if ( !p->doit ) {
  		getRecord( p->pax_id, aodb_pax, aodb_bag, res_checkin/*, res_bag*/ );
	  	getRecord( p->pax_id, prior_aodb_pax, prior_aodb_bag, prior_res_checkin/*, prior_res_bag*/ );			
		  ch_pax = res_checkin != prior_res_checkin;
		  //ch_bag = res_bag != prior_res_bag;	  	
	  	if ( ch_pax /*|| ch_bag*/ ) { 
//			   getRecord( p->pax_id, aodb_pax, aodb_bag ) != getRecord( p->pax_id, prior_aodb_pax, prior_aodb_bag ) )
			  createRecord( point_id, p->pax_id, point_addr, aodb_pax, aodb_bag, prior_aodb_pax, prior_aodb_bag,
			                res_checkin/*, res_bag*/ );
   			if ( ch_pax )
		  		file_data += res_checkin;
/*		  	if ( ch_bag ) {
//			  	ProgTrace( TRACE5, "res_bag=%s, prior_res_bag=%s", res_bag.c_str(), prior_res_bag.c_str() );				
				  bag_file_data += res_bag;
				}*/
		  }  
		}
  } 
   
  if ( !file_data.empty() ) {  	
	  createFileParamsAODB( point_id, params, 0 );
	}
/*	if ( !bag_file_data.empty() ) {
		createFileParamsAODB( point_id, bag_params, 1 );
	}*/
//	ProgTrace( TRACE5, "file_data.empty()=%d", file_data.empty() );
/*  reso = codec.encode( bag_file_data );
  bag_file_data = reso;*/
	return !file_data.empty()/* || !bag_file_data.empty()*/;
}


void getRecord( int pax_id, const vector<AODB_STRUCT> &aodb_pax, const vector<AODB_STRUCT> &aodb_bag,
                string &res_checkin/*, string &res_bag*/  )
{
	res_checkin.clear();
//	res_bag.clear();
	for ( vector<AODB_STRUCT>::const_iterator i=aodb_pax.begin(); i!=aodb_pax.end(); i++ ) {
	  if ( i->pax_id == pax_id ) {
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
//	ProgTrace( TRACE5, "getRecord pax_id=%d, return res=%s", pax_id, res.c_str() );
}

void createRecord( int point_id, int pax_id, const string &point_addr,
                   vector<AODB_STRUCT> &aodb_pax, vector<AODB_STRUCT> &aodb_bag,
                   vector<AODB_STRUCT> &prior_aodb_pax, vector<AODB_STRUCT> &prior_aodb_bag,
                   string &res_checkin/*, string &res_bag*/ )
{
//	ProgTrace( TRACE5, "point_id=%d, pax_id=%d, point_addr=%s", point_id, pax_id, point_addr.c_str() );
	res_checkin.clear();
	//res_bag.clear();
	TQuery PQry( &OraSession );	
 	PQry.SQLText = 
 	 "SELECT MAX(rec_no_pax) r1, MAX(rec_no_bag) r2 FROM aodb_files "
 	 " WHERE point_id=:point_id AND point_addr=:point_addr";
 	PQry.CreateVariable( "point_id", otInteger, point_id );
 	PQry.CreateVariable( "point_addr", otString, point_addr );
 	PQry.Execute();
 	int bag_num = PQry.FieldAsInteger( "r2" );
 	{
  ostringstream r;
  r<<setfill(' ')<<std::fixed<<setw(6)<<PQry.FieldAsInteger( "r1" ) + 1;	
 	res_checkin = r.str();
  }
  	// сохраняем новый слепок  	
 	PQry.Clear();
 	PQry.SQLText = 
 	 "BEGIN "
 	 " DELETE aodb_bag WHERE pax_id=:pax_id AND point_addr=:point_addr; "
 	 " DELETE aodb_pax WHERE pax_id=:pax_id AND point_addr=:point_addr; "
 	 " UPDATE aodb_files SET rec_no_pax=rec_no_pax+1 WHERE point_id=:point_id AND point_addr=:point_addr; "
 	 " IF SQL%NOTFOUND THEN "
 	 "  INSERT INTO aodb_files(point_id,point_addr,rec_no_pax,rec_no_bag) VALUES(:point_id,:point_addr,1,0); "
 	 " END IF; "
 	 "END; ";
 	PQry.CreateVariable( "point_id", otInteger, point_id );
 	PQry.CreateVariable( "pax_id", otInteger, pax_id );
 	PQry.CreateVariable( "point_addr", otString, point_addr );
 	PQry.Execute(); 	  	 
	vector<AODB_STRUCT>::iterator d=aodb_pax.end();
	vector<AODB_STRUCT>::iterator n=prior_aodb_pax.end();
	for ( d=aodb_pax.begin(); d!=aodb_pax.end(); d++ ) 
		if ( d->pax_id == pax_id ) {
			break;
		}
	for ( n=prior_aodb_pax.begin(); n!=prior_aodb_pax.end(); n++ ) 
		if ( n->pax_id == pax_id ) {
			break;
		}
		
	if ( d == aodb_pax.end() ) { // удаление
		res_checkin += n->record.substr( 0, n->record.length() - 2 );
		res_checkin += "1;";
//		ProgTrace( TRACE5, "delete record, record=%s", res.c_str() );
  }
	else  {
		res_checkin += d->record;
		PQry.Clear();
    PQry.SQLText = 
     "INSERT INTO aodb_pax(point_id,pax_id,point_addr,record) VALUES(:point_id,:pax_id,:point_addr,:record)";
    PQry.CreateVariable( "point_id", otInteger, point_id );
    PQry.CreateVariable( "pax_id", otInteger, pax_id );
    PQry.CreateVariable( "point_addr", otString, point_addr );
    PQry.CreateVariable( "record", otString, d->record );		
    PQry.Execute();
  }
  PQry.Clear();
  PQry.SQLText = "INSERT INTO aodb_bag(pax_id,point_addr,num,pr_cabin,record) VALUES(:pax_id,:point_addr,:num,:pr_cabin,:record)";
  PQry.CreateVariable( "pax_id", otInteger, pax_id );
  PQry.CreateVariable( "point_addr", otString, point_addr );
  PQry.DeclareVariable( "num", otInteger );		
  PQry.DeclareVariable( "pr_cabin", otInteger );		
  PQry.DeclareVariable( "record", otString );		
  int num=0;
	for ( int pr_cabin=1; pr_cabin>=0; pr_cabin-- ) {		
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
	  			res_checkin += i->record + "0;";
	  		}
	    }
	    if ( !f ) {
	    	res_checkin += i->record + "0;";
	    }
  	}
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
	    	res_checkin += i->record + "1;";
	    }
  	}
	}
	if ( d != aodb_pax.end() )
		d->doit = true;
	if ( n != prior_aodb_pax.end() )
		n->doit = true;
	PQry.Clear();
	PQry.SQLText = 
 	 "BEGIN "
 	 " UPDATE aodb_files SET rec_no_bag=:rec_no_bag WHERE point_id=:point_id AND point_addr=:point_addr; "
 	 " IF SQL%NOTFOUND THEN "
 	 "  INSERT INTO aodb_files(point_id,point_addr,rec_no_pax,rec_no_bag) VALUES(:point_id,:point_addr,0,:rec_no_bag); "
 	 " END IF; "
 	 "END; ";	
 	PQry.CreateVariable( "point_id", otInteger, point_id );
 	PQry.CreateVariable( "point_addr", otString, point_addr );
 	PQry.CreateVariable( "rec_no_bag", otInteger, bag_num );
 	PQry.Execute();
	res_checkin += "\n";	
}

void ParseFlight( const std::string &point_addr, std::string &linestr, AODB_Flight &fl )
{
  fl.rec_no = NoExists;	
 	if ( linestr.length() < 6 )
 		throw Exception( "invalid flight record format, length=%d, value=%s", linestr.length(), linestr.c_str() );	
 		
	TReqInfo *reqInfo = TReqInfo::Instance();
  string region = CityTZRegion( "МОВ" );	  	 	  	
	TQuery Qry( &OraSession );	
	TQuery TIDQry( &OraSession );	
	TQuery POINT_IDQry( &OraSession );	
	string tmp, tmp1;		
	tmp = linestr.substr( 0, 6 );
	tmp = TrimString( tmp );
	fl.rec_no = NoExists;
	if ( StrToInt( tmp.c_str(), fl.rec_no ) == EOF || fl.rec_no < 0 ||fl.rec_no > 999999 )
		throw Exception( "Invalid rec_no, value=%s", tmp.c_str() );
		
 	if ( linestr.length() < 175 + 4 )
 		throw Exception( "invalid flight record format, length=%d, value=%s", linestr.length(), linestr.c_str() );
		
	tmp = linestr.substr( 6, 10 );
	tmp = TrimString( tmp );
	if ( StrToFloat( tmp.c_str(), fl.id ) == EOF || fl.id < 0 || fl.id > 9999999999.0 )
		throw Exception( "Invalid id, value=%s", tmp.c_str() );
	tmp = linestr.substr( 16, 10 );
	tmp = TrimString( tmp );
	if ( tmp.length() < 3 )
		throw Exception( "Invalid trip, value=%s", tmp.c_str() );
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
				throw Exception( "Invalid trip, value=%s", tmp.c_str() );
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
		throw Exception( "Invalid trip, value=%s", tmp.c_str() );
	if ( StrToInt( tmp1.c_str(), fl.flt_no ) == EOF )
		throw Exception( "Invalid trip, value=%s", tmp.c_str() );
	if ( fl.flt_no > 99999 || fl.flt_no <= 0 )
		throw Exception( "Invalid trip, value=%s", tmp.c_str() );
	Qry.Clear();
	Qry.SQLText = 
	 "SELECT code,1 as f FROM airlines WHERE code=:code AND pr_del=0 "
	 " UNION "
	 "SELECT code,2 as f FROM airlines WHERE code_lat=:code AND pr_del=0 "
	 " UNION "
	 "SELECT airline as code,3 FROM aodb_airlines WHERE aodb_code=:code"
	 " ORDER BY 2";
	Qry.CreateVariable( "code", otString, fl.airline );
	Qry.Execute();
	if ( !Qry.RowCount() )
		throw Exception( "Invalid airline, value=%s", fl.airline.c_str() ); 
	fl.airline = Qry.FieldAsString( "code" );
	if ( Qry.FieldAsInteger( "f" ) == 2 )
		fl.trip_type = "м";
  else
  	fl.trip_type = "п";
	tmp = linestr.substr( 26, 3 );
	fl.litera = TrimString( tmp );
	if ( !fl.litera.empty() ) {
		Qry.Clear();
		Qry.SQLText = 
		 "SELECT code,1 FROM trip_liters WHER code=:code AND pr_del=0"
		 " UNION "
		 "SELECT litera as code,2 FROM aodb_liters WHERE aodb_code=:code "
		 " ORDER BY 2";
		Qry.CreateVariable( "code", otString, fl.litera );
		Qry.Execute();
		if ( !Qry.RowCount() )
			throw Exception( "Invalid litera, value=%s", fl.litera.c_str() );
		fl.litera = Qry.FieldAsString( "code" );
	}
	tmp = linestr.substr( 29, 16 );
	tmp = TrimString( tmp );
  if ( tmp.empty() )
		throw Exception( "Invalid scd, value=|%s|", tmp.c_str() );
	else
		if ( StrToDateTime( tmp.c_str(), "dd.mm.yyyy hh:nn", fl.scd ) == EOF )
			throw Exception( "Invalid scd, value=%s", tmp.c_str() );	
	fl.scd = LocalToUTC( fl.scd, region );
	tmp = linestr.substr( 45, 16 );
	tmp = TrimString( tmp );
  if ( tmp.empty() )
		fl.est = NoExists;
	else {
		if ( StrToDateTime( tmp.c_str(), "dd.mm.yyyy hh:nn", fl.est ) == EOF )
			throw Exception( "Invalid est, value=%s", tmp.c_str() );	
		fl.est = LocalToUTC( fl.est, region );
	}
	tmp = linestr.substr( 61, 16 );
	tmp = TrimString( tmp );
  if ( tmp.empty() )
		fl.act = NoExists;
	else {
		if ( StrToDateTime( tmp.c_str(), "dd.mm.yyyy hh:nn", fl.act ) == EOF )
			throw Exception( "Invalid act, value=%s", tmp.c_str() );				
		fl.act= LocalToUTC( fl.act, region );
	}
	tmp = linestr.substr( 77, 2 );
	fl.term = TrimString( tmp );	
	tmp = linestr.substr( 79, 3 );	
	tmp = TrimString( tmp );
	if ( tmp.empty() )
		fl.krm = NoExists;
	else
  	if ( StrToInt( tmp.c_str(), fl.krm ) == EOF )
	  	throw Exception( "Invalid krm, value=%s", tmp.c_str() );
	tmp = linestr.substr( 82, 6 );
	tmp = TrimString( tmp );
	if ( tmp.empty() )
		fl.max_load = NoExists;
	else
  	if ( StrToInt( tmp.c_str(), fl.max_load ) == EOF || fl.max_load < 0 || fl.max_load > 999999 )
	  	throw Exception( "Invalid max_load, value=%s", tmp.c_str() );
	tmp = linestr.substr( 88, 10 );
	fl.craft = TrimString( tmp );
	if ( fl.craft.empty() )
		throw Exception( "Invalid craft, value=|%s|", tmp.c_str() );
	Qry.Clear();
	Qry.SQLText = 
	 "SELECT code, 1 FROM crafts WHERE ( code=:code OR code_lat=:code OR name=:code OR name_lat=:code ) AND pr_del=0 "
	 " UNION "
	 "SELECT craft as code, 2 FROM aodb_crafts WHERE aodb_code=:code";
	Qry.CreateVariable( "code", otString, fl.craft );
	Qry.Execute();
	if ( !Qry.RowCount() )
		throw Exception( "Invalid craft, value=%s", fl.craft.c_str() );
	fl.craft = Qry.FieldAsString( "code" );
  tmp = linestr.substr( 98, 10 );  		
  fl.bort = TrimString( tmp );
	tmp = linestr.substr( 108, 16 );
	tmp = TrimString( tmp );
  if ( tmp.empty() )
		fl.checkin_beg = NoExists;
	else {
		if ( StrToDateTime( tmp.c_str(), "dd.mm.yyyy hh:nn", fl.checkin_beg ) == EOF )
			throw Exception( "Invalid checkin_beg, value=%s", tmp.c_str() );				
		fl.checkin_beg = LocalToUTC( fl.checkin_beg, region );
	}
	tmp = linestr.substr( 124, 16 );
	tmp = TrimString( tmp );
  if ( tmp.empty() )
		fl.checkin_end = NoExists;
	else {
		if ( StrToDateTime( tmp.c_str(), "dd.mm.yyyy hh:nn", fl.checkin_end ) == EOF )
			throw Exception( "Invalid checkin_end, value=%s", tmp.c_str() );				
		fl.checkin_end = LocalToUTC( fl.checkin_end, region );
	}
	tmp = linestr.substr( 140, 16 );
	tmp = TrimString( tmp );
  if ( tmp.empty() )
		fl.boarding_beg = NoExists;
	else {
		if ( StrToDateTime( tmp.c_str(), "dd.mm.yyyy hh:nn", fl.boarding_beg ) == EOF )
			throw Exception( "Invalid boarding_beg, value=%s", tmp.c_str() );				
		fl.boarding_beg = LocalToUTC( fl.boarding_beg, region );
	}
	tmp = linestr.substr( 156, 16 );
	tmp = TrimString( tmp );
  if ( tmp.empty() )
		fl.boarding_end = NoExists;
	else {
		if ( StrToDateTime( tmp.c_str(), "dd.mm.yyyy hh:nn", fl.boarding_end ) == EOF )
			throw Exception( "Invalid boarding_beg, value=%s", tmp.c_str() );				  
		fl.boarding_end = LocalToUTC( fl.boarding_end, region );
	}
	tmp = linestr.substr( 172, 1 );
	tmp = TrimString( tmp );			
	if ( tmp.empty() )
		fl.pr_cancel = NoExists;
	else
  	if ( StrToInt( tmp.c_str(), fl.pr_cancel ) == EOF )
	  	throw Exception( "Invalid pr_cancel, value=%s", tmp.c_str() );		
	tmp = linestr.substr( 173, 1 );
	tmp = TrimString( tmp );			
	if ( tmp.empty() )
		fl.pr_del = NoExists;
	else
  	if ( StrToInt( tmp.c_str(), fl.pr_del ) == EOF )
	  	throw Exception( "Invalid pr_del, value=%s", tmp.c_str() );			  	
	int len = linestr.length();
	i = 174;
	tmp = linestr.substr( i, 1 );
	if ( tmp[ 0 ] != ';' )
		throw Exception( "Invalid format can ';', but	%c", linestr[ i ] );
	i++;
	bool dest_mode = true;
	AODB_Dest dest;
	AODB_Term term;
	while ( i < len ) {		
		tmp = linestr.substr( i, 1 );
		tmp = TrimString( tmp );
		if ( dest_mode ) {
    	if ( StrToInt( tmp.c_str(), dest.num ) == EOF || dest.num < 0 || dest.num > 9 )
    		if ( fl.dests.empty() )
	    	  throw Exception( "Invalid dest.num, value=%s", tmp.c_str() );					
	      else
	    	  dest_mode = false;
	    else {
	    	i++;
	    	tmp = linestr.substr( i, 3 );
	    	dest.airp = TrimString( tmp );
	    	if ( dest.airp.empty() )
	    		throw Exception( "Invalid dest.airp, value=|%s|", dest.airp.c_str() );
	    	Qry.Clear();
	    	Qry.SQLText = 
	    	 "SELECT code, 1 FROM airps WHERE ( code=:code OR code_lat=:code OR name=:code OR name_lat=:code ) AND pr_del=0 "
	    	 " UNION "
	    	 "SELECT airp as code, 2 FROM aodb_airps WHERE aodb_code=:code "
	    	 " ORDER BY 2 ";
	    	Qry.CreateVariable( "code", otString, dest.airp );
	    	Qry.Execute();
	    	if ( !Qry.RowCount() )
	    		throw Exception( "Invalid dest.airp, value=%s", dest.airp.c_str() );
	    	dest.airp = Qry.FieldAsString( "code" );
	    	i += 3;
	    	tmp = linestr.substr( i, 1 );
	    	tmp = TrimString( tmp );
	    	if ( tmp.empty() || StrToInt( tmp.c_str(), dest.pr_del ) == EOF || dest.pr_del < 0 || dest.pr_del > 1 )
	    	  throw Exception( "Invalid dest.pr_del, value=|%s|", tmp.c_str() );					
	    	fl.dests.push_back( dest );
	    	i++;
	    	tmp = linestr.substr( i, 1 );
	      if ( tmp[ 0 ] != ';' )
		      throw Exception( "Invalid format can ';', but	%c", linestr[ i ] );	    	
		    i++;
	    }
		}
		if ( !dest_mode ) {
			if ( tmp != "П" && tmp != "Р" )
				throw Exception( "Invalid term type, value=%s", tmp.c_str() );					
			term.type = tmp;
			i++;
    	tmp = linestr.substr( i, 4 );
	   	term.name = TrimString( tmp );
			if ( term.name.empty() )
				throw Exception( "Invalid term name, value=|%s|", term.name.c_str() );					
			//!!!!
			string term_name;
			if ( term.type == "П" )
				term_name = "G" + term.name;
			else
				term_name = "R" + term.name;
			Qry.Clear();
//	ProgTrace( TRACE5, "term.name=%s", term_name.c_str() );
			Qry.SQLText = "SELECT desk FROM stations WHERE airp=:airp AND work_mode=:work_mode AND name=:code";
			Qry.CreateVariable( "airp", otString, "ВНК" );
			Qry.CreateVariable( "work_mode", otString, term.type );
			Qry.CreateVariable( "code", otString, term_name );
			Qry.Execute();
			if ( !Qry.RowCount() ) {
  			if ( term.type == "П" )
	  			term_name = "G0" + term.name;
		  	else
			  	term_name = "R0" + term.name;				
			  Qry.SetVariable( "code", term_name );
			  Qry.Execute();
			  if ( !Qry.RowCount() )
				  throw Exception( "Invalid term name, value=%s", term.name.c_str() );
			}
			term.name = Qry.FieldAsString( "desk" );
			i += 4;
     	tmp = linestr.substr( i, 1 );
	   	tmp = TrimString( tmp );
	    if ( tmp.empty() || StrToInt( tmp.c_str(), term.pr_del ) == EOF || term.pr_del < 0 || term.pr_del > 1 )
	      throw Exception( "Invalid term.pr_del, value=|%s|", tmp.c_str() );								
	    fl.terms.push_back( term );
	    i++;
	    tmp = linestr.substr( i, 1 );
	   	if ( tmp[ 0 ] != ';' )
    		throw Exception( "Invalid format can ';', but	%c", linestr[ i ] );
      i++;
		}				
	}
	// запись в БД
	tst();
	Qry.Clear();
	Qry.SQLText = 
	 "SELECT point_id,craft,bort,scd_out,est_out,act_out,litera,park_out,pr_del "
	 " FROM points WHERE airline=:airline AND flt_no=:flt_no AND "
	 "                   ( suffix IS NULL AND :suffix IS NULL OR suffix=:suffix ) AND "
	 "                   airp=:airp AND TRUNC(scd_out)=TRUNC(:scd_out) ";
	Qry.CreateVariable( "airline", otString, fl.airline );
	Qry.CreateVariable( "flt_no", otInteger, fl.flt_no );
	if ( fl.suffix.empty() )
	  Qry.CreateVariable( "suffix", otInteger, FNull );
	else
		Qry.CreateVariable( "suffix", otInteger, fl.suffix );
	Qry.CreateVariable( "airp", otString, "ВНК" );
  Qry.CreateVariable( "airline", otString, fl.airline );
	Qry.CreateVariable( "scd_out", otDate, fl.scd );
	tst();
	Qry.Execute();
	tst();
/*rogTrace( TRACE5, "airline=%s, flt_no=%d, suffix=%s, scd_out=%s, insert=%d", fl.airline.c_str(), fl.flt_no,
	           fl.suffix.c_str(), DateTimeToStr( fl.scd ).c_str(), Qry.Eof );*/
	int move_id, new_tid, point_id;
	bool pr_insert = Qry.Eof;
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
    Qry.Execute();
    move_id = Qry.GetVariableAsInteger( "move_id" );
    reqInfo->MsgToLog( "Вводнового рейса ", evtDisp, move_id );		
    reqInfo->MsgToLog( string( "Ввод нового пункта " ) + "ВНК", evtDisp, move_id );
    TIDQry.Execute();
    new_tid = TIDQry.FieldAsInteger( "n" );
    POINT_IDQry.Execute();
    point_id = POINT_IDQry.FieldAsInteger( "point_id" );    
    Qry.Clear();
    Qry.SQLText =
     "INSERT INTO points(move_id,point_id,point_num,airp,pr_tranzit,first_point,airline,flt_no,suffix,"\
     "                   craft,bort,scd_in,est_in,act_in,scd_out,est_out,act_out,trip_type,litera,"\
     "                   park_in,park_out,pr_del,tid,remark,pr_reg) "\
     " VALUES(:move_id,:point_id,:point_num,:airp,:pr_tranzit,:first_point,:airline,:flt_no,:suffix,"\
     "        :craft,:bort,:scd_in,:est_in,:act_in,:scd_out,:est_out,:act_out,:trip_type,:litera,"\
     "        :park_in,:park_out,:pr_del,:tid,:remark,:pr_reg)";
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
    Qry.CreateVariable( "park_out", otString, fl.term );
    if ( fl.pr_del )
    	Qry.CreateVariable( "pr_del", otInteger, -1 );
    else
    	if ( fl.pr_cancel )
    		Qry.CreateVariable( "pr_del", otInteger, 1 );
    	else
    		Qry.CreateVariable( "pr_del", otInteger, 0 );
    Qry.CreateVariable( "tid", otInteger, new_tid );
    Qry.CreateVariable( "remark", otString, FNull );
    Qry.CreateVariable( "pr_reg", otInteger, 1 );
    Qry.Execute();
    int num = 0;
    for ( vector<AODB_Dest>::iterator it=fl.dests.begin(); it!=fl.dests.end(); it++ ) {
    	num++;
      POINT_IDQry.Execute();
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
      TIDQry.Execute();
      Qry.SetVariable( "tid", TIDQry.FieldAsInteger( "n" ) );            
      Qry.Execute();
      reqInfo->MsgToLog( string( "Ввод нового пункта " ) + it->airp, evtDisp, move_id );
    }
    // создаем времена технологического графика только для пункта вылета из ВНК???
 		Qry.Clear();
 		Qry.SQLText =
     "BEGIN "
     " INSERT INTO trip_sets(point_id,f,c,y,max_commerce,pr_etstatus,pr_stat, "
     "    pr_tranz_reg,pr_check_load,pr_overload_reg,pr_exam,pr_check_pay,pr_trfer_reg) "
     "  VALUES(:point_id,0,0,0, :max_commerce, 0, 0, "
     "    NULL, 0, 1, 0, 0, 0); "
     " ckin.set_trip_sets(:point_id); "
     " gtimer.puttrip_stages(:point_id); "
     "END;";
		Qry.CreateVariable( "point_id", otInteger, point_id );
		Qry.CreateVariable( "max_commerce", otInteger, fl.max_load );
		Qry.Execute();		
		Qry.Clear();
		Qry.SQLText = 
		 "INSERT INTO aodb_points(aodb_point_id,point_addr,point_id) VALUES(:aodb_point_id,:point_addr,:point_id)";
		Qry.CreateVariable( "point_id", otInteger, point_id );
		Qry.CreateVariable( "point_addr", otString, point_addr );
		Qry.CreateVariable( "aodb_point_id", otFloat, fl.id );
		Qry.Execute();
	}
	else { // update
		//reqInfo->MsgToLog( "Обновление рейса ", evtDisp, move_id );		
		string remark;
	  AODB_Flight old_fl;		
		point_id = Qry.FieldAsInteger( "point_id" );
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
		old_fl.term = Qry.FieldAsString( "park_out" );
		old_fl.pr_del = ( Qry.FieldAsInteger( "pr_del" ) == -1 );
		old_fl.pr_cancel = ( Qry.FieldAsInteger( "pr_del" ) == 1 );
		Qry.Clear();
    Qry.SQLText =
     "BEGIN "\
     " UPDATE points SET move_id=move_id WHERE move_id=:move_id; "\
     " UPDATE move_ref SET move_id=move_id WHERE move_id=:move_id; "\
     "END;";
    Qry.CreateVariable( "move_id", otInteger, move_id );
    Qry.Execute(); // лочим
    Qry.Clear();
    Qry.SQLText = 
     "UPDATE points "
     " SET craft=:craft,bort=:bort,est_out=:est_out,act_out=:act_out,litera=:litera, "
     "     park_out=:park_out,pr_del=:pr_del "
     " WHERE point_id=:point_id";
    Qry.CreateVariable( "point_id", otInteger, point_id );
    Qry.CreateVariable( "craft", otString, fl.craft );    
 	  if ( fl.craft != old_fl.craft ) {
	  	if ( !old_fl.craft.empty() ) {
 	  	  remark += " изм. типа ВС с " + old_fl.craft;
 	  	  if ( !fl.craft.empty() )
 	  	    reqInfo->MsgToLog( string( "Изменение типа ВС на " ) + fl.craft + " порт ВНК" , evtDisp, move_id, point_id );
 	  	}
 	  	else {  	  		
 	  		reqInfo->MsgToLog( string( "Назначение ВС " ) + fl.craft + " порт ВНК" , evtDisp, move_id, point_id );
 	  	}
 	  }
 	  Qry.CreateVariable( "bort", otString, fl.bort );
 	  if ( fl.bort != old_fl.bort ) {
 	  	if ( !old_fl.bort.empty() ) {
 	  	  remark += " изм. борта с " + old_fl.bort;
 	  	  if ( !fl.bort.empty() )
 	  	    reqInfo->MsgToLog( string( "Изменение борта на " ) + fl.bort + " порт ВНК", evtDisp, move_id, point_id );
 	  	}
 	  	else {
 	  		reqInfo->MsgToLog( string( "Назначение борта " ) + fl.bort + " порт ВНК", evtDisp, move_id, point_id );
 	  	}
 	  }
 	  if ( fl.est > NoExists )
 	    Qry.CreateVariable( "est_out", otDate, fl.est );
 	  else
 	  	Qry.CreateVariable( "est_out", otDate, FNull );
 	  if ( fl.act > NoExists )
 	  	Qry.CreateVariable( "act_out", otDate, fl.act );
 	  else
 	    Qry.CreateVariable( "act_out", otDate, FNull );
 	  Qry.CreateVariable( "litera", otString, fl.litera );
 	  Qry.CreateVariable( "park_out", otString, fl.term );
 	  if ( fl.pr_del )
 	  	 Qry.CreateVariable( "pr_del", otInteger, -1 );
 	  else
 	  	if ( fl.pr_cancel )
 	  		Qry.CreateVariable( "pr_del", otInteger, 1 );
 	  	else
 	  		Qry.CreateVariable( "pr_del", otInteger, 0 );
 	  if ( fl.pr_del != old_fl.pr_del ) {
 	  	if ( fl.pr_del )
 	  		reqInfo->MsgToLog( string( "Удаление рейса ВНК" ), evtDisp, move_id, point_id );
 	  	else
 	  		reqInfo->MsgToLog( string( "Добавление рейса ВНК" ), evtDisp, move_id, point_id );
 	  }
 	  if ( fl.pr_cancel != old_fl.pr_cancel ) {
 	  	if ( fl.pr_cancel )
 	  		reqInfo->MsgToLog( string( "Отмена рейса ВНК" ), evtDisp, move_id, point_id );
 	  	else
 	  		reqInfo->MsgToLog( string( "Возврат рейса ВНК" ), evtDisp, move_id, point_id ); 	  	
 	  }
 	  Qry.Execute();
 	  // теперь работа с пунктами посадки
/*    int num = 0;
    int point_num = 0;*/
    vector<AODB_Dest> old_dests;
    Qry.Clear();
    Qry.SQLText = "SELECT point_num,airp,pr_del FROM points WHERE move_id=:move_id ORDER BY point_num";
    Qry.CreateVariable( "move_id", otInteger, move_id );
    Qry.Execute();
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
  		   "       point_dep=:point_id AND pr_refuse=0 ";
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
    Qry.Clear();
    Qry.SQLText = "UPDATE trip_sets SET max_commerce=:max_commerce WHERE point_id=:point_id";
    Qry.CreateVariable( "point_id", otInteger, point_id );
    Qry.CreateVariable( "max_commerce", otInteger, fl.max_load );
    Qry.Execute();
	}
	// обновление времен технологического графика
  Qry.Clear();
	Qry.SQLText = "UPDATE trip_stages SET est=NVL(:scd,est) WHERE point_id=:point_id AND stage_id=:stage_id";
	Qry.CreateVariable( "point_id", otInteger, point_id );
	Qry.CreateVariable( "stage_id", otInteger, sOpenCheckIn );
	Qry.CreateVariable( "scd", otDate, fl.checkin_beg );
	Qry.Execute();
	Qry.SetVariable( "stage_id", sCloseCheckIn );
	Qry.SetVariable( "scd", fl.checkin_end );
	Qry.Execute();
	Qry.SetVariable( "stage_id", sOpenBoarding );
	Qry.SetVariable( "scd", fl.boarding_beg );
	Qry.Execute();
	Qry.SetVariable( "stage_id", sCloseBoarding );
	Qry.SetVariable( "scd", fl.boarding_end );
	Qry.Execute();	
	// обновление стоек регистрации и выходов на покадку
	Qry.Clear();
	Qry.SQLText = 
	 "BEGIN "
	 " IF :pr_del != 0 THEN "
	 "   DELETE trip_stations	WHERE point_id=:point_id AND desk=:desk AND work_mode=:work_mode; "
	 " ELSE "
	 "  UPDATE trip_stations SET desk=desk WHERE point_id=:point_id AND desk=:desk AND work_mode=:work_mode; "
 	 "  IF SQL%NOTFOUND THEN "
 	 "   INSERT INTO trip_stations(point_id,desk,work_mode,pr_main) "
	 "    VALUES(:point_id,:desk,:work_mode,0); "
 	 "  END IF; "
	 " END IF; "
	 "END;";
	Qry.CreateVariable( "point_id", otInteger, point_id );
	Qry.DeclareVariable( "pr_del", otInteger );
	Qry.DeclareVariable( "desk", otString );
	Qry.DeclareVariable( "work_mode", otString );
//rogTrace( TRACE5, "fl.terms.size()=%d, point_id=%d", fl.terms.size(), point_id );
	for ( vector<AODB_Term>::iterator it=fl.terms.begin(); it!=fl.terms.end(); it++ ) {
		Qry.SetVariable( "desk", it->name );
		Qry.SetVariable( "work_mode", it->type );
		Qry.SetVariable( "pr_del", it->pr_del );
//ProgTrace( TRACE5, "desk=%s, work_mode=%s, pr_del=%d", it->name.c_str(), it->type.c_str(), it->pr_del );
		Qry.Execute();
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
 
void ParseAndSaveSPP( const std::string &filename, const std::string &canon_name, std::string &fd )
{
	TQuery QryLog( &OraSession );
	QryLog.SQLText =
	 " BEGIN "
 	 " UPDATE aodb_spp_error SET record=:record, msg=:msg "
 	 "  WHERE filename=:filename AND point_addr=:point_addr AND rec_no=:rec_no; "
 	 " IF SQL%NOTFOUND THEN "
	 "  INSERT INTO aodb_spp_error(filename,point_addr,rec_no,record,msg) VALUES(:filename,:point_addr,:rec_no,:record,:msg); "
 	 " END IF; "
 	 "END;";
	QryLog.CreateVariable( "filename", otString, filename );
	QryLog.CreateVariable( "point_addr", otString, canon_name );
	QryLog.DeclareVariable( "rec_no", otInteger );
	QryLog.DeclareVariable( "record", otString );
	QryLog.DeclareVariable( "msg", otString );
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
      ParseFlight( canon_name, linestr, fl );
    }
    catch( Exception &e ) {    	    	
      if ( fl.rec_no == NoExists )
      	QryLog.SetVariable( "rec_no", -1 );
      else
        QryLog.SetVariable( "rec_no", fl.rec_no );
      QryLog.SetVariable( "record", linestr );
    	QryLog.SetVariable( "msg", e.what() );
      QryLog.Execute();          
    }
    if ( fl.rec_no > NoExists )
      max_rec_no = fl.rec_no;     
  }
	TQuery Qry( &OraSession );
	Qry.SQLText = "UPDATE aodb_spp_files SET rec_no=:rec_no WHERE filename=:filename AND point_addr=:point_addr";	
	Qry.CreateVariable( "rec_no", otInteger, max_rec_no );
	Qry.CreateVariable( "filename", otString, filename );
	Qry.CreateVariable( "point_addr", otString, canon_name );
	Qry.Execute();
}


