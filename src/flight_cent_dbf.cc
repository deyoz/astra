#include <string>
#include <map>
#define NICKNAME "DJEK"
#include "exceptions.h"
#include "oralib.h"
#include "slogger.h"
#include "stl_utils.h"
#include "basic.h"
#include "develop_dbf.h"
#include "sopp.h"

using namespace std;
using namespace EXCEPTIONS;
using namespace BASIC;	

/*
NR					C	8			- номер рейса
ID					N	9			- идентификатор рейса
FL_SPP			C	1 
FL_SPP_B 		C	1
FL_ACT			C	1
FL_BAL			C	1
FL_BAL_WRK	C	1
DOP					C	1			- метка "дополнительный"
TRANZIT			C	1			- метка "транзитный"
TYPE				C	4
BORT				C	9			- номер борта
KR					N	3			- число пассажирский кресел
MODIF				C	1
PLANDAT			D	8			- плановая дата вылета
FACTDAT			D	8			- фактическая дата вылета
TIME				C	4			- время вылета
STAND				N	2			- номер стоянки
SEATLIM			N	3
MAILLIM			N	5
CREW				N	2
STEW				N	2
FEED				N	3
FUEL_TO			N	6
FUEL_USE		N 6
TOW_LIM			N	6
CAP					C	14
COMPANY			C	3			- авиакомпания
OWNER				C	3			- владелец ВС (авиакомпания)
NR_SM				N	1
WEEK				C	7
OPER_BASIC	N	1
CREW_DOW		N	2
STEW_DOW		N	2
FEED_DOW		N	4
PORT1				C	3			- пункт посадки
T_MAN1			N	3
T_RB1				N	3
T_RM1				N	2
T_RK1				N	4
T_BAG1			N	5
T_PBAG1			N	4
T_CARGO1		N	6
T_MAIL1			N	5
D_MAN1			N	3			- взрослые пассажиры
D_RB1				N	3			- дети большие
D_RM1				N	2			- дети маленькие
D_RK1				N	4			- ручная кладь
D_BAG1			N	5			- багаж
D_PBAG1			N	4			- платный багаж
D_CARGO1		N	6     - груз ???
D_MAIL1			N	5     - почта ???
PORT2				C	3			- пункт посадки
T_MAN2			N	3
T_RB2				N	3
T_RM2				N	2
T_RK2				N	4
T_BAG2			N	5
T_PBAG2			N	4
T_CARGO2		N	6     
T_MAIL2			N	5
D_MAN2			N	3			- взрослые пассажиры
D_RB2				N	3			- дети большие
D_RM2				N	2			- дети маленькие
D_RK2				N	4			- ручная кладь
D_BAG2			N	5			- багаж
D_PBAG2			N	4			- платный багаж
D_CARGO2		N	6     - груз ???
D_MAIL2			N	5     - почта ???
PORT3				C	3			- пункт посадки
T_MAN3			N	3
T_RB3				N	3
T_RM3				N	2
T_RK3				N	4
T_BAG3			N	5
T_PBAG3			N	4
T_CARGO3		N	6
T_MAIL3			N	5
D_MAN3			N	3			- взрослые пассажиры
D_RB3				N	3			- дети большие
D_RM3				N	2			- дети маленькие
D_RK3				N	4			- ручная кладь
D_BAG3			N	5			- багаж
D_PBAG3			N	4			- платный багаж
D_CARGO3		N	6     - груз ???
D_MAIL3			N	5     - почта ???
PORT4				C	3			- пункт посадки
T_MAN4			N	3
T_RB4				N	3
T_RM4				N	2
T_RK4				N	4
T_BAG4			N	5
T_PBAG4			N	4
T_CARGO4		N	6
T_MAIL4			N	5
D_MAN4			N	3			- взрослые пассажиры
D_RB4				N	3			- дети большие
D_RM4				N	2			- дети маленькие
D_RK4				N	4			- ручная кладь
D_BAG4			N	5			- багаж
D_PBAG4			N	4			- платный багаж
D_CARGO4		N	6     - груз ???
D_MAIL4			N	5     - почта ???
I_MAN				N	3			- взрослые пассажиры первого класса
I_RB				N	3			- дети большие первого класса
I_RM				N	2			- дети маленькие первого класса
I_RK				N	4			- ручная кладь первого класса
I_BAG				N	5			- багаж первого класса
I_PBAG			N	4			- платный багаж первого класса
B_MAN				N	3			- взрослые пассажиры бизнес класса
B_RB				N	3			- дети большие бизнес класса
B_RM				N	2			- дети маленькие бизнес класса
B_RK				N	4			- ручная кладь бизнес класса
B_BAG				N	5			- багаж бизнес класса
B_PBAG			N	4			- платный багаж бизнес класса
K_MAN				N	3			- число занятых кресел взрослыми пассажирами
K_RB				N	3			- число занятых кресел большими детьми
K_RM				N	2			- число занятых кресел маленькими детьми
K_SL				N	4			- число занятых кресел служебные пассажиры
LIM					N	5
SUMMA				N	5
MAX_TOW			N	6
PAS_VZR			N	3
PAS_RB			N	3
PAS_RM			N	3
PAST				N	3
ZAGR_K			N	4
BAG_VS			N	5
BAGPL				N	5
GR_VS				N	5
GRT					N	5
POCT_VS			N	5
POCTT				N	5
HOLD1				N	5
HOLD2				N	5
HOLD3				N	5
HOLD4				N	5
HOLD5				N	5
HOLD6				N	5
HOLD7				N	5
HOLD8				N	5
HOLD9				N	5
HOLD10			N	5
HOLD11			N	5
HOLD12			N	5
HOLD13			N	5
HOLD14			N	5
HOLD15			N	5
HOLD16			N	5
HOLD17			N	5
HOLD18			N	5
HOLD19			N	5
HOLD20			N	5
HOLDUP1			N	5
HOLDUP2			N	5
HOLDUP3			N	5
HOLDUP4			N	5
HOLDUP5			N	5
HOLDUP6			N	5
HOLDUP7			N	5
HOLDUP8			N	5
HOLDUP9			N	5
HOLDUP10		N	5
HOLDUP11		N	5
HOLDUP12		N	5
HOLDUP13		N	5
HOLDUP14		N	5
HOLDUP15		N	5
HOLDUP16		N	5
HOLDUP17		N	5
HOLDUP18		N	5
HOLDUP19		N	5
HOLDUP20		N	5
PAS_ROW			C	1
PASS_1			N	3
PASS_2			N	3
PASS_3			N	3
PASS_4			N	3
PASS_5			N	3
PASS_6			N	3
PASS_7			N	3
PASS_8			N	3
PASS_9			N	3
PASS_10 		N	3
PASS_11 		N	3
PASS_12 		N	3
*/

const string FileDirectory = "C:\\Program Files\\kupol\\base"; 

void createFileParams( int point_id, map<string,string> &params )
{
	params.clear();
	TQuery FlightQry( &OraSession );
	FlightQry.SQLText = "SELECT airline,flt_no,suffix FROM points WHERE point_id=:point_id";
	FlightQry.CreateVariable( "point_id", otInteger, point_id );
	FlightQry.Execute();
	if ( !FlightQry.RowCount() )
		throw Exception( "Flight not found in createFileParams" );
	params[ "FileName" ] = string( FlightQry.FieldAsString( "airline" ) ) + 
	                       FlightQry.FieldAsString( "flt_no" ) + 
	                       FlightQry.FieldAsString( "suffix" ) + ".dbf";
	params[ "WorkDir" ] = FileDirectory;
}

void getTripCountsOnDest( int point_arv, Luggage &lug, vector<std::string> &data )
{
	int adult = 0;
	int child = 0;
	int baby = 0;
	int rk_weight = 0;
	int bag_weight = 0;
	int pay_bag_weight = 0;
	int cargo_weight = 0;
	int mail_weight = 0;
	for ( vector<PaxLoad>::iterator p=lug.vpaxload.begin(); p!=lug.vpaxload.end(); p++ ) {
	 if ( p->point_arv != point_arv )
	   continue;	  	
	 adult += p->adult;
	 child += p->child;
	 baby += p->baby;
	 rk_weight += p->rk_weight;
	 bag_weight += p->bag_weight;
	}
	for ( vector<Cargo>::iterator p=lug.vcargo.begin(); p!=lug.vcargo.end(); p++ ) {
		if ( p->point_arv != point_arv )
			continue;
	  cargo_weight += p->cargo;
	  mail_weight += p->mail;
	}
  data.push_back( IntToString( adult ) ); // взрослые пассажиры
  data.push_back( IntToString( child ) ); // дети большие
  data.push_back( IntToString( baby ) ); // дети маленькие
  data.push_back( IntToString( rk_weight ) ); // ручная кладь
  data.push_back( IntToString( bag_weight ) ); // ручная кладь
	data.push_back( IntToString( pay_bag_weight ) ); // платный багаж    !!!
	data.push_back( IntToString( cargo_weight ) ); // груз    ???	
	data.push_back( IntToString( mail_weight ) ); // почта    ???		
}

void getTripCountsOnClass( Luggage &lug, std::string cl, vector<std::string> &data )
{
	int adult = 0;
	int child = 0;
	int baby = 0;
	int bag_weight = 0;
	int rk_weight = 0;
	int pay_bag_weight = 0;
	for ( vector<PaxLoad>::iterator p=lug.vpaxload.begin(); p!=lug.vpaxload.end(); p++ ) {
		if ( p->cl != cl )
			continue;
	  adult += p->adult;
	  child += p->child;
	  baby += p->baby;
	  bag_weight += p->bag_weight;
	  rk_weight += p->rk_weight;	  
	}
	data.push_back( IntToString( adult ) ); // взрослые пассажиры первого класса
	data.push_back( IntToString( child ) ); // дети большие первого класса
	data.push_back( IntToString( baby ) ); // дети маленькие первого класса
	data.push_back( IntToString( rk_weight ) ); // ручная кладь первого класса
	data.push_back( IntToString( bag_weight ) ); // багаж первого класса
	data.push_back( IntToString( pay_bag_weight ) ); // платный багаж первого класса
}

void getTripSeats( Luggage &lug, vector<std::string> &data )
{
	int seatsadult = 0;
	int seatschild = 0;
	int seatsbaby = 0;
	for ( vector<PaxLoad>::iterator p=lug.vpaxload.begin(); p!=lug.vpaxload.end(); p++ ) {
	  seatsadult += p->seatsadult;
	  seatschild += p->seatschild;
	  seatsbaby += p->seatsbaby;
	}	
	data.push_back( IntToString( seatsadult ) ); // число занятых кресел взрослыми пассажирами
	data.push_back( IntToString( seatschild ) ); // число занятых кресел большими детьми
	data.push_back( IntToString( seatsbaby ) ); // число занятых кресел маленькими детьми
	data.push_back( "0" ); // число занятых кресел служебные пассажиры !!!
}

bool createCentringFile( int point_id, const string &Sender, const string &Receiver )
{
	Develop_dbf dbf;
	dbf.AddField( "NR", 'C', 8 ); // номер рейса 3 - Company + 5 flt_no + ??? suffix
	dbf.AddField( "ID", 'N', 9 ); // идентификатор рейса
	dbf.AddField( "DOP", 'C', 1 ); // метка "дополнительный"
	dbf.AddField( "TRANZIT", 'C', 1 ); // метка "транзитный"
	dbf.AddField( "BORT", 'C', 9 ); // номер борта
	dbf.AddField( "KR", 'N', 3 ); // число пассажирский кресел
	dbf.AddField( "PLANDAT", 'D', 8 ); // плановая дата вылета
	dbf.AddField( "FACTDAT", 'D', 8 ); // фактическая дата вылета
	dbf.AddField( "TIME", 'C', 4 ); // время вылета
	dbf.AddField( "STAND", 'N', 2 ); // номер стоянки
	dbf.AddField( "COMPANY", 'C', 3 ); // авиакомпания
	dbf.AddField( "OWNER", 'C', 3 ); // владелец ВС (авиакомпания)
	dbf.AddField( "PORT1", 'C', 3 ); // пункт посадки
	dbf.AddField( "D_MAN1", 'N', 3 ); // взрослые пассажиры
	dbf.AddField( "D_RB1", 'N', 3 ); // дети большые
	dbf.AddField( "D_RM1", 'N', 2 ); // дети маленькие
	dbf.AddField( "D_RK1", 'N', 4 ); // ручная кладь
	dbf.AddField( "D_BAG1", 'N', 5 ); // багаж
	dbf.AddField( "D_PBAG1", 'N', 4 ); // платный багаж
	dbf.AddField( "D_CARGO1", 'N', 6 ); // груз ???
	dbf.AddField( "D_MAIL1", 'N', 5 ); // почта ???
	dbf.AddField( "PORT2", 'C', 3 ); // пункт посадки
	dbf.AddField( "D_MAN2", 'N', 3 ); // взрослые пассажиры
	dbf.AddField( "D_RB2", 'N', 3 ); // дети большые
	dbf.AddField( "D_RM2", 'N', 2 ); // дети маленькие
	dbf.AddField( "D_RK2", 'N', 4 ); // ручная кладь
	dbf.AddField( "D_BAG2", 'N', 5 ); // багаж
	dbf.AddField( "D_PBAG2", 'N', 4 ); // платный багаж
	dbf.AddField( "D_CARGO2", 'N', 6 ); // груз ???
	dbf.AddField( "D_MAIL2", 'N', 5 ); // почта ???	
	dbf.AddField( "PORT3", 'C', 3 ); // пункт посадки
	dbf.AddField( "D_MAN3", 'N', 3 ); // взрослые пассажиры
	dbf.AddField( "D_RB3", 'N', 3 ); // дети большые
	dbf.AddField( "D_RM3", 'N', 2 ); // дети маленькие
	dbf.AddField( "D_RK3", 'N', 4 ); // ручная кладь
	dbf.AddField( "D_BAG3", 'N', 5 ); // багаж
	dbf.AddField( "D_PBAG3", 'N', 4 ); // платный багаж
	dbf.AddField( "D_CARGO3", 'N', 6 ); // груз ???	
	dbf.AddField( "D_MAIL3", 'N', 5 ); // почта ???		
	dbf.AddField( "PORT4", 'C', 3 ); // пункт посадки
	dbf.AddField( "D_MAN4", 'N', 3 ); // взрослые пассажиры
	dbf.AddField( "D_RB4", 'N', 3 ); // дети большые
	dbf.AddField( "D_RM4", 'N', 2 ); // дети маленькие
	dbf.AddField( "D_RK4", 'N', 4 ); // ручная кладь
	dbf.AddField( "D_BAG4", 'N', 5 ); // багаж
	dbf.AddField( "D_PBAG4", 'N', 4 ); // платный багаж
	dbf.AddField( "D_CARGO4", 'N', 6 ); // груз ???	
	dbf.AddField( "D_MAIL4", 'N', 5 ); // почта ???		
	dbf.AddField( "I_MAN", 'N', 3 ); // взрослые пассажиры первого класса
	dbf.AddField( "I_RB", 'N', 3 ); // дети большие первого класса
	dbf.AddField( "I_RM", 'N', 2 ); // дети маленькие первого класса
	dbf.AddField( "I_RK", 'N', 4 ); // ручная кладь первого класса
	dbf.AddField( "I_BAG", 'N', 5 ); // багаж первого класса
	dbf.AddField( "I_PBAG", 'N', 4 ); // платный багаж первого класса
	dbf.AddField( "B_MAN", 'N', 3 ); // взрослые пассажиры бизнес класса
	dbf.AddField( "B_RB", 'N', 3 ); // дети большие бизнес класса
	dbf.AddField( "B_RM", 'N', 2 ); // дети маленькие бизнес класса
	dbf.AddField( "B_RK", 'N', 4 ); // ручная кладь бизнес класса
	dbf.AddField( "B_BAG", 'N', 5 ); // багаж бизнес класса
	dbf.AddField( "B_PBAG", 'N', 4 ); // платный багаж бизнес класса
	dbf.AddField( "K_MAN", 'N', 3 ); // число занятых кресел взрослыми пассажирами
	dbf.AddField( "K_RB", 'N', 3 ); // число занятых кресел большими детьми
	dbf.AddField( "K_RM", 'N', 2 ); // число занятых кресел маленькими детьми
	dbf.AddField( "K_SL", 'N', 4 ); // число занятых кресел служебные пассажиры	
	ProgTrace( TRACE5, "dbf.fieldCount=%d", dbf.fieldCount() );
	TQuery Qry( &OraSession );
	Qry.SQLText = 
	 "SELECT points.point_num,points.point_id,pr_tranzit,first_point,"
	 "       airline,flt_no,suffix,bort,scd_out,act_out,"
	 "       trip_type,SUBSTR( park_out, 1, 2 ) park_out,pr_reg,airp,pr_del, "
	 "       a.cfg "
	 "       FROM points, trip_sets, "
	 "( SELECT SUM(cfg) as cfg FROM trip_classes WHERE point_id=:point_id ) a "
	 " WHERE points.point_id=:point_id AND points.point_id=trip_sets.point_id(+) ";
	Qry.CreateVariable( "point_id", otInteger, point_id );
	tst();
	Qry.Execute();
  tst();
  if ( !Qry.RowCount() )
  	return false;
  tst();
  DBFRow row;
  Luggage lug;
  GetLuggage( point_id, lug, false, true ); // false - wo pr_brd=1, pr_refuse=0

  int p_id = -1;
	for ( int count=1; count<=4; count++ ) {
		if ( count == 1 ) {
			p_id = point_id;
      row.pr_del = Qry.FieldAsInteger( "pr_del" );
 		  row.data.push_back( string( Qry.FieldAsString( "airline" ) ) + Qry.FieldAsString( "flt_no" ) );
		  row.data.push_back( Qry.FieldAsString( "point_id" ) );
		  row.data.push_back( Qry.FieldAsString( "suffix" ) );
		  if ( Qry.FieldIsNULL( "pr_tranzit" ) )
	  		row.data.push_back( "" );
	  	else	
	  	  row.data.push_back( "*" ); //???
  		row.data.push_back( Qry.FieldAsString( "bort" ) );
  		row.data.push_back( IntToString( Qry.FieldAsInteger( "cfg" ) ) ); //kr
  		if ( Qry.FieldIsNULL( "scd_out" ) )
  			return false;
  		row.data.push_back( DateTimeToStr( Qry.FieldAsDateTime( "scd_out" ), "yyyymmdd" ) );
  		if ( !Qry.FieldIsNULL( "act_out" ) ) {
  			row.data.push_back( DateTimeToStr( Qry.FieldAsDateTime( "act_out" ), "yyyymmdd" ) );
  			row.data.push_back( DateTimeToStr( Qry.FieldAsDateTime( "act_out" ), "hhnn" ) );
  	  }
  		else {
  		  row.data.push_back( "" );
  		  row.data.push_back( DateTimeToStr( Qry.FieldAsDateTime( "scd_out" ), "hhnn" ) );
  		}
  		row.data.push_back( Qry.FieldAsString( "park_out" ) );
  		row.data.push_back( Qry.FieldAsString( "airline" ) );
  		row.data.push_back( Qry.FieldAsString( "airline" ) ); // owner = company ???		
		}
		// переход на следующий пункт посадки
		p_id = -1;				
		if ( p_id < 0 )
			row.data.push_back( "" );
		else
  		row.data.push_back( Qry.FieldAsString( "airp" ) );
  		//13
	  getTripCountsOnDest( p_id, lug, row.data );
	}
	getTripCountsOnClass( lug, "П", row.data );
	getTripCountsOnClass( lug, "Б", row.data );
	getTripSeats( lug, row.data );
	
	ProgTrace( TRACE5, "row.column.count=%d", (int)row.data.size() );

	dbf.AddRow( row );
	tst();
	dbf.Build( );
	tst();
	if ( !dbf.isEmpty() ) {
		tst();
		Qry.Clear();
		Qry.SQLText = "SELECT id__seq.nextval id, system.UTCSYSDATE now FROM dual ";
		tst();
		Qry.Execute();
		tst();
		int id = Qry.FieldAsInteger( "id" );
		TDateTime d = Qry.FieldAsDateTime( "now" );
		Qry.Clear();
		Qry.SQLText = 
		  "INSERT INTO files(id,sender,receiver,type,error,time,data) "
		  " VALUES(:id,:sender,:receiver,:type,:error,:time,:data) ";
		Qry.CreateVariable( "id", otInteger, id );
		Qry.CreateVariable( "sender", otString, Sender );
		Qry.CreateVariable( "receiver", otString, Receiver );
		Qry.CreateVariable( "type", otString, "FILE" );
		Qry.CreateVariable( "error", otString, FNull );
		Qry.CreateVariable( "time", otDate, d );
		string res = dbf.Result( );
		Qry.CreateLongVariable( "data", otLongRaw, (void*)res.data(), (int)res.size() );
		ProgTrace( TRACE5, "create centring file size=%d", (int)res.size() );
		Qry.Execute();	
		Qry.Clear();
		Qry.SQLText =
		  "INSERT INTO file_params(id,name,value) "
		  " VALUES(:id,:name,:value) ";
		Qry.CreateVariable( "id", otInteger, id );
		Qry.DeclareVariable( "name", otString );
		Qry.DeclareVariable( "value", otString );
		map<string,string> params;
		createFileParams( point_id, params );
		for (map<string,string>::iterator p=params.begin(); p!=params.end(); p++ ) {
			Qry.SetVariable( "name", p->first );
			Qry.SetVariable( "value", p->second );
      Qry.Execute();
		}
		Qry.Clear();
		Qry.SQLText = 
	   "INSERT INTO file_queue(id,sender,receiver,type,status,time) "
	   " SELECT id,sender,receiver,type,'PUT',system.UTCSYSDATE FROM files "
	   "  WHERE id=:id ";
	  Qry.CreateVariable( "id", otInteger, id );
	  Qry.Execute();
	}
	tst();
	return !dbf.isEmpty();
}
