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
#include "astra_utils.h"
#include "develop_dbf.h"


using namespace std;
using namespace EXCEPTIONS;
using namespace BASIC;

struct AODB_STRUCT{
	int pax_id;
	int num;
	int rk;
	bool doit;
	string record;
};

string getRecord( int pax_id, const vector<AODB_STRUCT> &aodb_pax, const vector<AODB_STRUCT> &aodb_bag );
string createRecord( int point_id, int pax_id, vector<AODB_STRUCT> &aodb_pax, vector<AODB_STRUCT> &aodb_bag,
                      vector<AODB_STRUCT> &prior_aodb_pax, vector<AODB_STRUCT> &prior_aodb_bag );

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

bool createAODBCheckInInfoFile( int point_id, std::map<std::string,std::string> &params, std::string &file_data )
{
	AODB_STRUCT STRAO;
	vector<AODB_STRUCT> prior_aodb_pax, aodb_pax;
	vector<AODB_STRUCT> prior_aodb_bag, aodb_bag;
	TQuery Qry(&OraSession);
	Qry.SQLText = 
	 "SELECT airline||flt_no||suffix trip,scd_out FROM points WHERE point_id=:point_id FOR UPDATE";
	Qry.CreateVariable( "point_id", otInteger, point_id );
	Qry.Execute();
	if ( !Qry.RowCount() ) {
		ProgError( STDLOG, "Flight not found, point_id=%d", point_id );
		return false;
	}
	string flight = Qry.FieldAsString( "trip" );
	string region = CityTZRegion( "МОВ" );
	string scd_date = DateTimeToStr( UTCToLocal( Qry.FieldAsDateTime( "scd_out" ), region ), "dd.mm.yyyy hh:nn" );	 
	Qry.Clear();	
	Qry.SQLText = 
	 "SELECT pax_id, record FROM aodb_pax WHERE point_id=:point_id";
	Qry.CreateVariable( "point_id", otInteger, point_id );
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
	 "       aodb_pax.pax_id=aodb_bag.pax_id "
	 " ORDER BY pr_cabin DESC, bag_num ";
	Qry.CreateVariable( "point_id", otInteger, point_id );
	Qry.Execute();
	STRAO.doit = false;
	while ( !Qry.Eof ) {
		STRAO.pax_id = Qry.FieldAsInteger( "pax_id" );
	  STRAO.num = Qry.FieldAsInteger( "bag_num" );
	  STRAO.record = Qry.FieldAsString( "bag_record" );
	  STRAO.rk = Qry.FieldAsInteger( "pr_cabin" );
	  prior_aodb_bag.push_back( STRAO );
		Qry.Next();
	}
	// теперь создадим похожую картинку по данным рейса из БД 	 
	TQuery BagQry( &OraSession );
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
	 "       pax.pers_type,pax.seat_no,pax.seats-1 seats,"
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
	TimeQry.SQLText = "SELECT MIN(time) as mtime FROM events WHERE type='ПАС' AND id1=:point_id AND id2=:reg_no ";
	TimeQry.CreateVariable( "point_id", otInteger, point_id );
	TimeQry.DeclareVariable( "reg_no", otInteger );
	while ( !Qry.Eof ) {
		ostringstream record;
		record<<setfill(' ')<<std::fixed<<setw(10)<<flight;
		record<<setw(16)<<scd_date;
		record<<setw(3)<<Qry.FieldAsInteger( "reg_no");
		record<<setw(30)<<Qry.FieldAsString( "name" );
		TAirpsRow *row=(TAirpsRow*)&base_tables.get("airps").get_row("code",Qry.FieldAsString("airp_arv")); 		
		record<<setw(20)<<row->name;
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
		record<<setw(1)<<0; // РМ количество
		record<<setw(36)<<""; // Имя ребенка
		record<<setw(60)<<""; // ДОП. Инфо
		record<<setw(1)<<0; // международный багаж
//		record<<setw(1)<<0; // трансатлантический багаж :)
// стойка рег. + время рег. + выход на посадку + время прохода на посадку
    TimeQry.SetVariable( "reg_no", Qry.FieldAsInteger( "reg_no" ) ); 
    TimeQry.Execute();
    record<<setw(4)<<""; // стойка рег.
    if ( TimeQry.Eof )
    	record<<setw(16)<<"";
    else {
    	record<<setw(16)<<DateTimeToStr( UTCToLocal( TimeQry.FieldAsDateTime( "mtime" ), region ), "dd.mm.yyyy hh:nn" );	 
    }
    record<<setw(4)<<""; // выход на посадку
    record<<setw(16)<<""; // время прохода на посадку
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
		  	record_bag<<setw(2)<<code<<setw(20)<<type_name;
		  	record_bag<<setw(4)<<BagQry.FieldAsInteger( "weight" );
	  		//record<<setw(1)<<0; // снятие
	  		STRAO.pax_id = Qry.FieldAsInteger( "pax_id" );
	  		STRAO.num = BagQry.FieldAsInteger( "bag_num" );
	  		STRAO.record = record_bag.str();
	  		STRAO.rk = 1;
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
		  	record_bag<<setw(2)<<code<<setw(20)<<type_name;
		  	record_bag<<setw(10)<<setprecision(0)<<BagQry.FieldAsFloat( "no" );
		  	record_bag<<setw(2)<<BagQry.FieldAsString( "color" );		  	
		  	if ( pr_first ) {
		  		pr_first = false;
		  		record_bag<<setw(4)<<BagQry.FieldAsInteger( "weight" );
		  	}
		  	else {
		  		record_bag<<setw(4)<<0;		  		
		  	}
		  	record_bag<<setw(10)<<""; // номер контейнера
	  		//record<<setw(1)<<0; // снятие
	  		STRAO.pax_id = Qry.FieldAsInteger( "pax_id" );
	  		STRAO.num = BagQry.FieldAsInteger( "bag_num" );
	  		STRAO.record = record_bag.str();
	  		STRAO.rk = 0;
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
	for ( vector<AODB_STRUCT>::iterator p=aodb_pax.begin(); p!=aodb_pax.end(); p++ ) {
		if ( getRecord( p->pax_id, aodb_pax, aodb_bag ) != getRecord( p->pax_id, prior_aodb_pax, prior_aodb_bag ) ) {
//			ProgTrace(TRACE5, "p->doit=%d, pax_id=%d", p->doit, p->pax_id );
			file_data += createRecord( point_id, p->pax_id, aodb_pax, aodb_bag, prior_aodb_pax, prior_aodb_bag );
//			ProgTrace(TRACE5, "p->doit=%d, pax_id=%d", p->doit, p->pax_id );
	  }
  }
	for ( vector<AODB_STRUCT>::iterator p=prior_aodb_pax.begin(); p!=prior_aodb_pax.end(); p++ ) {
//		ProgTrace(TRACE5, "p->doit=%d, pax_id=%d", p->doit, p->pax_id );
		if ( !p->doit &&
			   getRecord( p->pax_id, aodb_pax, aodb_bag ) != getRecord( p->pax_id, prior_aodb_pax, prior_aodb_bag ) )
			file_data += createRecord( point_id, p->pax_id, aodb_pax, aodb_bag, prior_aodb_pax, prior_aodb_bag );
  } 
   
  if ( !file_data.empty() ) {  	
	  createFileParamsAODB( point_id, params, 0 );
	}
//	ProgTrace( TRACE5, "file_data.empty()=%d", file_data.empty() );
	return !file_data.empty();
}


string getRecord( int pax_id, const vector<AODB_STRUCT> &aodb_pax, const vector<AODB_STRUCT> &aodb_bag )
{
	string res;
	for ( vector<AODB_STRUCT>::const_iterator i=aodb_pax.begin(); i!=aodb_pax.end(); i++ ) {
	  if ( i->pax_id == pax_id ) {
		  res = i->record;
		  for ( vector<AODB_STRUCT>::const_iterator b=aodb_bag.begin(); b!=aodb_bag.end(); b++ ) {
		  	if ( b->pax_id != pax_id )
		  		continue;
		  	res += b->record;
	    }
	    break;
	  }
	}
//	ProgTrace( TRACE5, "getRecord pax_id=%d, return res=%s", pax_id, res.c_str() );
	return res;
}

string createRecord( int point_id, int pax_id, vector<AODB_STRUCT> &aodb_pax, vector<AODB_STRUCT> &aodb_bag,
                      vector<AODB_STRUCT> &prior_aodb_pax, vector<AODB_STRUCT> &prior_aodb_bag )
{
	string res;
	TQuery PQry( &OraSession );
 	PQry.SQLText = "SELECT MAX(rec_no) r FROM aodb_files WHERE point_id=:point_id ";
 	PQry.CreateVariable( "point_id", otInteger, point_id );
 	PQry.Execute();
  ostringstream r;
  r<<setfill(' ')<<std::fixed<<setw(6)<<PQry.FieldAsInteger( "r" ) + 1;	
 	res = r.str();
  	// сохраняем новый слепок  	
 	PQry.Clear();
 	PQry.SQLText = 
 	 "BEGIN "
 	 " DELETE aodb_bag WHERE pax_id=:pax_id; "
 	 " DELETE aodb_pax WHERE pax_id=:pax_id; "
 	 " UPDATE aodb_files SET rec_no=rec_no+1 WHERE point_id=:point_id; "
 	 " IF SQL%NOTFOUND THEN "
 	 "  INSERT INTO aodb_files(point_id,rec_no) VALUES(:point_id, 1); "
 	 " END IF; "
 	 "END; ";
 	PQry.CreateVariable( "point_id", otInteger, point_id );
 	PQry.CreateVariable( "pax_id", otInteger, pax_id );
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
		res += n->record.substr( 0, n->record.length() - 2 );
		res += "1;";
//		ProgTrace( TRACE5, "delete record, record=%s", res.c_str() );
  }
	else  {
		res += d->record;
		PQry.Clear();
    PQry.SQLText = "INSERT INTO aodb_pax(point_id,pax_id,record) VALUES(:point_id,:pax_id,:record)";
    PQry.CreateVariable( "point_id", otInteger, point_id );
    PQry.CreateVariable( "pax_id", otInteger, pax_id );
    PQry.CreateVariable( "record", otString, d->record );		
//    ProgTrace( TRACE5, "insert record point_id=%d, pax_id=%d, record=|%s|", point_id, pax_id, d->record.c_str() );
    PQry.Execute();
  }
  PQry.Clear();
  PQry.SQLText = "INSERT INTO aodb_bag(pax_id,num,pr_cabin,record) VALUES(:pax_id,:num,:pr_cabin,:record)";
  PQry.CreateVariable( "pax_id", otInteger, pax_id );
  PQry.DeclareVariable( "num", otInteger );		
  PQry.DeclareVariable( "pr_cabin", otInteger );		
  PQry.DeclareVariable( "record", otString );		
//  ProgTrace( TRACE5, "aodb_bag.size=%d, prior_aodb_bag.size()=%d", aodb_bag.size(), prior_aodb_bag.size() );
  int num=0;
	for ( int pr_cabin=1; pr_cabin>=0; pr_cabin-- ) {		
		PQry.SetVariable( "pr_cabin", pr_cabin );
	  for ( vector<AODB_STRUCT>::iterator i=aodb_bag.begin(); i!= aodb_bag.end(); i++ ) {
	  	if ( i->rk != pr_cabin || i->pax_id != pax_id )
	  		continue;
	  	PQry.SetVariable( "num", num );
	  	PQry.SetVariable( "record", i->record );
	  	PQry.Execute();
	  	num++;
	  	bool f=false;
	  	for ( vector<AODB_STRUCT>::iterator j=prior_aodb_bag.begin(); j!= prior_aodb_bag.end(); j++ ) {
	  		if ( j->rk == pr_cabin && !j->doit && 
	  			   i->pax_id == pax_id && i->pax_id == j->pax_id && i->record == j->record ) {
	  			f=true;
	  			i->doit=true;
	  			j->doit=true;
	  			res += i->record + "0;";
	  		}
	    }
	    if ( !f ) {
	    	res += i->record + "0;";
	    }
  	}
	  for ( vector<AODB_STRUCT>::iterator i=prior_aodb_bag.begin(); i!= prior_aodb_bag.end(); i++ ) {
	  	bool f=false;
	  	if ( i->doit || i->rk != pr_cabin || i->pax_id != pax_id )
	  		continue;
	  	for ( vector<AODB_STRUCT>::iterator j=aodb_bag.begin(); j!= aodb_bag.end(); j++ ) {
	  		if ( j->rk == pr_cabin && i->pax_id == pax_id && i->pax_id == j->pax_id && i->record == j->record ) {
	  			f=true;
	  			break;
	  		}
	    }
	    if ( !f ) {
	    	res += i->record + "1;";
	    }
  	}
	}
	if ( d != aodb_pax.end() )
		d->doit = true;
	if ( n != prior_aodb_pax.end() )
		n->doit = true;
//	ProgTrace( TRACE5, "createRecord point_id=%d, pax_id=%d, return res=%s", point_id, pax_id, res.c_str() );
	return res;
}
