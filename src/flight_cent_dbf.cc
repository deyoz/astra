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
//#include "base_tables.h"
#include "astra_utils.h"


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
TYPE				C	4			- тип ВС
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
const string PARAM_WORK_DIR = "WorkDir";
const string PARAM_FILE_NAME = "FileName";

void createFileParams( int point_id, map<string,string> &params )
{
	params.clear();
	TQuery FlightQry( &OraSession );
	FlightQry.SQLText = "SELECT airline,flt_no,suffix, gtimer.get_stage(:point_id, 1) as st FROM points WHERE point_id=:point_id";
	FlightQry.CreateVariable( "point_id", otInteger, point_id );
	FlightQry.Execute();
	if ( !FlightQry.RowCount() )
		throw Exception( "Flight not found in createFileParams" );
	ProgTrace( TRACE5, "stage=%d", FlightQry.FieldAsInteger( "st" ) );
	if ( FlightQry.FieldAsInteger( "st" ) != 20 ) //!!!
    params[ PARAM_FILE_NAME ] = string( FlightQry.FieldAsString( "airline" ) ) +
	                              FlightQry.FieldAsString( "flt_no" ) +
	                              FlightQry.FieldAsString( "suffix" ) + "_0.dbf";
  else			
	  params[ PARAM_FILE_NAME ] = string( FlightQry.FieldAsString( "airline" ) ) +
	                              FlightQry.FieldAsString( "flt_no" ) +
	                              FlightQry.FieldAsString( "suffix" ) + ".dbf";
	params[ PARAM_WORK_DIR ] = FileDirectory;
}

void getTripCountsOnDest( int point_arv, Luggage &lug, vector<std::string> &data )
{
	//SELECT * from unccomp_bag //!!!
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
	 pay_bag_weight += p->excess;
	}
	for ( vector<Cargo>::iterator p=lug.vcargo.begin(); p!=lug.vcargo.end(); p++ ) {
		if ( p->point_arv != point_arv )
			continue;
	  cargo_weight += p->cargo;
	  mail_weight += p->mail;
	}
	data.push_back( "0" ); //T_MAN1
	data.push_back( "0" ); //T_RB1
	data.push_back( "0" ); //T_RM1
	data.push_back( "0" ); //T_RK1
	data.push_back( "0" ); //T_BAG1
	data.push_back( "0" ); //T_PBAG1
	data.push_back( "0" ); //T_CARGO1
	data.push_back( "0" ); //T_MAIL1
  data.push_back( IntToString( adult ) ); // взрослые пассажиры
  data.push_back( IntToString( child ) ); // дети большие
  data.push_back( IntToString( baby ) ); // дети маленькие
  data.push_back( IntToString( rk_weight ) ); // ручная кладь
  data.push_back( IntToString( bag_weight ) ); // ручная кладь
	data.push_back( IntToString( pay_bag_weight ) ); // платный багаж
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
	  pay_bag_weight += p->excess;
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
	data.push_back( "0" ); //K_SL число занятых кресел служебные пассажиры !!!
}

bool createCentringFile( int point_id, const string &Sender, const string &Receiver )
{
	Develop_dbf dbf;
	dbf.AddField( "NR", 'C', 8 ); //1 номер рейса 3 - Company + 5 flt_no + ??? suffix
	dbf.AddField( "ID", 'N', 9 ); //2 идентификатор рейса
	dbf.AddField( "FL_SPP", 'C', 1 ); //3 FL_SPP
	dbf.AddField( "FL_SPP_B", 'C', 1 ); //4
	dbf.AddField( "FL_ACT", 'C', 1 ); //5
	dbf.AddField( "FL_BAL", 'C', 1 ); //6
	dbf.AddField( "FL_BAL_WRK", 'C', 1 ); //7
	dbf.AddField( "DOP", 'C', 1 ); //8 метка "дополнительный"
	dbf.AddField( "TRANZIT", 'C', 1 ); //9 метка "транзитный"
	dbf.AddField( "TYPE", 'C', 4 ); //10 тип ВС
	dbf.AddField( "BORT", 'C', 9 ); //11 номер борта
	dbf.AddField( "KR", 'N', 3 ); //12 число пассажирский кресел
	dbf.AddField( "MODIF", 'C', 1 ); //13
	dbf.AddField( "PLANDAT", 'D', 8 ); //14 плановая дата вылета
	dbf.AddField( "FACTDAT", 'D', 8 ); //15 фактическая дата вылета
	dbf.AddField( "TIME", 'C', 4 ); //16 время вылета
	dbf.AddField( "STAND", 'C', 2 ); //17 номер стоянки
	dbf.AddField( "SEATLIM", 'N', 3 ); //18
	dbf.AddField( "MAILLIM", 'N', 5 ); //19
	dbf.AddField( "CREW", 'N', 2 ); //20
	dbf.AddField( "STEW", 'N', 2 ); //21
	dbf.AddField( "FEED", 'N', 3 ); //22
	dbf.AddField( "FUEL_TO", 'N', 6 ); //23
	dbf.AddField( "FUEL_USE", 'N', 6 ); //24
	dbf.AddField( "TOW_LIM", 'N', 6 ); //25
	dbf.AddField( "CAP", 'C', 14 ); //26
	dbf.AddField( "COMPANY", 'C', 3 ); //27 авиакомпания
	dbf.AddField( "OWNER", 'C', 3 ); //28 владелец ВС (авиакомпания)
	dbf.AddField( "NR_SM", 'N', 1 ); //29
	dbf.AddField( "WEEK", 'C', 7 ); //30
	dbf.AddField( "OPER_BASIC", 'N', 1 ); //31
	dbf.AddField( "CREW_DOW", 'N', 2 ); //32
	dbf.AddField( "STEW_DOW", 'N', 2 ); //33
	dbf.AddField( "FEED_DOW", 'N', 4 ); //34
	dbf.AddField( "PORT1", 'C', 3 ); //35 пункт посадки
	dbf.AddField( "T_MAN1", 'N', 3 ); //36
	dbf.AddField( "T_RB1", 'N', 3 ); //37
	dbf.AddField( "T_RM1", 'N', 2 ); //38
	dbf.AddField( "T_RK1", 'N', 4 ); //39
	dbf.AddField( "T_BAG1", 'N', 5 ); //40
	dbf.AddField( "T_PBAG1", 'N', 4 ); //41
	dbf.AddField( "T_CARGO1", 'N', 6 ); //42
	dbf.AddField( "T_MAIL1", 'N', 5 ); //43
	dbf.AddField( "D_MAN1", 'N', 3 ); //44 взрослые пассажиры
	dbf.AddField( "D_RB1", 'N', 3 ); //45 дети большые
	dbf.AddField( "D_RM1", 'N', 2 ); //46 дети маленькие
	dbf.AddField( "D_RK1", 'N', 4 ); //47 ручная кладь
	dbf.AddField( "D_BAG1", 'N', 5 ); //48 багаж
	dbf.AddField( "D_PBAG1", 'N', 4 ); //49 платный багаж
	dbf.AddField( "D_CARGO1", 'N', 6 ); //50 груз ???
	dbf.AddField( "D_MAIL1", 'N', 5 ); //51 почта ???
	dbf.AddField( "PORT2", 'C', 3 ); //52 пункт посадки
	dbf.AddField( "T_MAN2", 'N', 3 ); //53
	dbf.AddField( "T_RB2", 'N', 3 ); //54
	dbf.AddField( "T_RM2", 'N', 2 ); //55
	dbf.AddField( "T_RK2", 'N', 4 ); //56
	dbf.AddField( "T_BAG2", 'N', 5 ); //57
	dbf.AddField( "T_PBAG2", 'N', 4 ); //58
	dbf.AddField( "T_CARGO2", 'N', 6 ); //59
	dbf.AddField( "T_MAIL2", 'N', 5 ); //60
	dbf.AddField( "D_MAN2", 'N', 3 ); //61 взрослые пассажиры
	dbf.AddField( "D_RB2", 'N', 3 ); //62 дети большые
	dbf.AddField( "D_RM2", 'N', 2 ); //63 дети маленькие
	dbf.AddField( "D_RK2", 'N', 4 ); //64 ручная кладь
	dbf.AddField( "D_BAG2", 'N', 5 ); //65 багаж
	dbf.AddField( "D_PBAG2", 'N', 4 ); //66 платный багаж
	dbf.AddField( "D_CARGO2", 'N', 6 ); //67 груз ???
	dbf.AddField( "D_MAIL2", 'N', 5 ); //68 почта ???
	dbf.AddField( "PORT3", 'C', 3 ); //69 пункт посадки
	dbf.AddField( "T_MAN3", 'N', 3 ); //70
	dbf.AddField( "T_RB3", 'N', 3 ); //71
	dbf.AddField( "T_RM3", 'N', 2 ); //72
	dbf.AddField( "T_RK3", 'N', 4 ); //73
	dbf.AddField( "T_BAG3", 'N', 5 ); //74
	dbf.AddField( "T_PBAG3", 'N', 4 ); //75
	dbf.AddField( "T_CARGO3", 'N', 6 ); //76
	dbf.AddField( "T_MAIL3", 'N', 5 ); //77
	dbf.AddField( "D_MAN3", 'N', 3 ); //78 взрослые пассажиры
	dbf.AddField( "D_RB3", 'N', 3 ); //79 дети большые
	dbf.AddField( "D_RM3", 'N', 2 ); //80 дети маленькие
	dbf.AddField( "D_RK3", 'N', 4 ); //81 ручная кладь
	dbf.AddField( "D_BAG3", 'N', 5 ); //82 багаж
	dbf.AddField( "D_PBAG3", 'N', 4 ); //83 платный багаж
	dbf.AddField( "D_CARGO3", 'N', 6 ); //84 груз ???
	dbf.AddField( "D_MAIL3", 'N', 5 ); //85 почта ???
	dbf.AddField( "PORT4", 'C', 3 ); //86 пункт посадки
	dbf.AddField( "T_MAN4", 'N', 3 ); //87
	dbf.AddField( "T_RB4", 'N', 3 ); //88
	dbf.AddField( "T_RM4", 'N', 2 ); //89
	dbf.AddField( "T_RK4", 'N', 4 ); //90
	dbf.AddField( "T_BAG4", 'N', 5 ); //91
	dbf.AddField( "T_PBAG4", 'N', 4 ); //92
	dbf.AddField( "T_CARGO4", 'N', 6 ); //93
	dbf.AddField( "T_MAIL4", 'N', 5 ); //94
	dbf.AddField( "D_MAN4", 'N', 3 ); //95 взрослые пассажиры
	dbf.AddField( "D_RB4", 'N', 3 ); //96 дети большые
	dbf.AddField( "D_RM4", 'N', 2 ); //97 дети маленькие
	dbf.AddField( "D_RK4", 'N', 4 ); //98 ручная кладь
	dbf.AddField( "D_BAG4", 'N', 5 ); //99 багаж
	dbf.AddField( "D_PBAG4", 'N', 4 ); //100 платный багаж
	dbf.AddField( "D_CARGO4", 'N', 6 ); //101 груз ???
	dbf.AddField( "D_MAIL4", 'N', 5 ); //102 почта ???
	dbf.AddField( "I_MAN", 'N', 3 ); //103 взрослые пассажиры первого класса
	dbf.AddField( "I_RB", 'N', 3 ); //104 дети большие первого класса
	dbf.AddField( "I_RM", 'N', 2 ); //105 дети маленькие первого класса
	dbf.AddField( "I_RK", 'N', 4 ); //106 ручная кладь первого класса
	dbf.AddField( "I_BAG", 'N', 5 ); //107 багаж первого класса
	dbf.AddField( "I_PBAG", 'N', 4 ); //108 платный багаж первого класса
	dbf.AddField( "B_MAN", 'N', 3 ); //109 взрослые пассажиры бизнес класса
	dbf.AddField( "B_RB", 'N', 3 ); //110 дети большие бизнес класса
	dbf.AddField( "B_RM", 'N', 2 ); //111 дети маленькие бизнес класса
	dbf.AddField( "B_RK", 'N', 4 ); //112 ручная кладь бизнес класса
	dbf.AddField( "B_BAG", 'N', 5 ); //113 багаж бизнес класса
	dbf.AddField( "B_PBAG", 'N', 4 ); //114 платный багаж бизнес класса
	dbf.AddField( "K_MAN", 'N', 3 ); //115 число занятых кресел взрослыми пассажирами
	dbf.AddField( "K_RB", 'N', 3 ); //116 число занятых кресел большими детьми
	dbf.AddField( "K_RM", 'N', 2 ); //117 число занятых кресел маленькими детьми
	dbf.AddField( "K_SL", 'N', 4 ); //118 число занятых кресел служебные пассажиры НЕ ЗАПОЛНЯЕМ!!!
	dbf.AddField( "LIM", 'N', 5 ); //119
	dbf.AddField( "SUMMA", 'N', 5 ); //120
	dbf.AddField( "MAX_TOW", 'N', 6 ); //121
	dbf.AddField( "PAS_VZR", 'N', 3 ); //122
	dbf.AddField( "PAS_RB", 'N', 3 ); //123
	dbf.AddField( "PAS_RM", 'N', 3 ); //124
	dbf.AddField( "PAST", 'N', 3 ); //125
	dbf.AddField( "ZAGR_K", 'N', 4 ); //126
	dbf.AddField( "BAG_VS", 'N', 5 ); //127
	dbf.AddField( "BAGPL", 'N', 5 ); //128
	dbf.AddField( "GR_VS", 'N', 5 ); //129
	dbf.AddField( "GRT", 'N', 5 ); //130
	dbf.AddField( "POCT_VS", 'N', 5 ); //131
	dbf.AddField( "POCTT", 'N', 5 ); //132
	dbf.AddField( "HOLD1", 'N', 5 ); //133
	dbf.AddField( "HOLD2", 'N', 5 ); //134
	dbf.AddField( "HOLD3", 'N', 5 ); //135
	dbf.AddField( "HOLD4", 'N', 5 ); //136
	dbf.AddField( "HOLD5", 'N', 5 ); //137
	dbf.AddField( "HOLD6", 'N', 5 ); //138
	dbf.AddField( "HOLD7", 'N', 5 ); //139
	dbf.AddField( "HOLD8", 'N', 5 ); //140	 
	dbf.AddField( "HOLD9", 'N', 5 ); //141
	dbf.AddField( "HOLD10", 'N', 5 ); //142
  dbf.AddField( "HOLD11", 'N', 5 ); //143
  dbf.AddField( "HOLD12", 'N', 5 ); //144
  dbf.AddField( "HOLD13", 'N', 5 ); //145
  dbf.AddField( "HOLD14", 'N', 5 ); //146
  dbf.AddField( "HOLD15", 'N', 5 ); //147
  dbf.AddField( "HOLD16", 'N', 5 ); //148
  dbf.AddField( "HOLD17", 'N', 5 ); //149
  dbf.AddField( "HOLD18", 'N', 5 ); //150
  dbf.AddField( "HOLD19", 'N', 5 ); //151
  dbf.AddField( "HOLD20", 'N', 5 ); //152
  dbf.AddField( "HOLDUP1", 'N', 5 ); //153
  dbf.AddField( "HOLDUP2", 'N', 5 ); //154
  dbf.AddField( "HOLDUP3", 'N', 5 ); //155
  dbf.AddField( "HOLDUP4", 'N', 5 ); //156
  dbf.AddField( "HOLDUP5", 'N', 5 ); //157
  dbf.AddField( "HOLDUP6", 'N', 5 ); //158
  dbf.AddField( "HOLDUP7", 'N', 5 ); //159
  dbf.AddField( "HOLDUP8", 'N', 5 ); //160
  dbf.AddField( "HOLDUP9", 'N', 5 ); //161
  dbf.AddField( "HOLDUP10", 'N', 5 ); //162
  dbf.AddField( "HOLDUP11", 'N', 5 ); //163
  dbf.AddField( "HOLDUP12", 'N', 5 ); //164
  dbf.AddField( "HOLDUP13", 'N', 5 ); //165
  dbf.AddField( "HOLDUP14", 'N', 5 ); //166
  dbf.AddField( "HOLDUP15", 'N', 5 ); //167
  dbf.AddField( "HOLDUP16", 'N', 5 ); //168
  dbf.AddField( "HOLDUP17", 'N', 5 ); //169
  dbf.AddField( "HOLDUP18", 'N', 5 ); //170
  dbf.AddField( "HOLDUP19", 'N', 5 ); //171
  dbf.AddField( "HOLDUP20", 'N', 5 ); //172
  dbf.AddField( "PAS_ROW", 'C', 1 ); //173
  dbf.AddField( "PASS_1", 'N', 3 ); //174
  dbf.AddField( "PASS_2", 'N', 3 ); //175
  dbf.AddField( "PASS_3", 'N', 3 ); //176
  dbf.AddField( "PASS_4", 'N', 3 ); //177
  dbf.AddField( "PASS_5", 'N', 3 ); //178
  dbf.AddField( "PASS_6", 'N', 3 ); //179
  dbf.AddField( "PASS_7", 'N', 3 ); //180
  dbf.AddField( "PASS_8", 'N', 3 ); //181
  dbf.AddField( "PASS_9", 'N', 3 ); //182
  dbf.AddField( "PASS_10", 'N', 3 ); //183
  dbf.AddField( "PASS_11", 'N', 3 ); //184
  dbf.AddField( "PASS_12", 'N', 3 ); //185
	ProgTrace( TRACE5, "dbf.fieldCount=%d", dbf.fieldCount() );
	TQuery Qry( &OraSession );
	Qry.SQLText =
	 "SELECT points.point_num,points.point_id,pr_tranzit,DECODE(points.pr_tranzit,0,points.point_id,points.first_point) first_point,"
	 "       airline,flt_no,suffix,bort,scd_out,act_out,"
	 "       craft,trip_type,SUBSTR( park_out, 1, 2 ) park_out,pr_reg,airp,pr_del, "
	 "       a.cfg "
	 "       FROM points, trip_sets, "
	 "( SELECT SUM(cfg) as cfg FROM trip_classes WHERE point_id=:point_id ) a "
	 " WHERE points.point_id=:point_id AND points.point_id=trip_sets.point_id(+) ";
	Qry.CreateVariable( "point_id", otInteger, point_id );
	Qry.Execute();
  if ( !Qry.RowCount() )
  	return false;
  TQuery DestsQry( &OraSession );
  DestsQry.SQLText =
   "SELECT point_num,point_id,airp FROM points "
   " WHERE first_point=:first_point AND point_num>:point_num and pr_del=0 "
   " ORDER BY point_num ";
  DestsQry.CreateVariable( "first_point", otInteger, Qry.FieldAsInteger( "first_point" ) );
  DestsQry.CreateVariable( "point_num", otInteger, Qry.FieldAsInteger( "point_num" ) );
  DestsQry.Execute();
  DBFRow row;
  Luggage lug;
  GetLuggage( point_id, lug, false ); // false - wo pr_brd=1

  int p_id = -1;
  for ( int count=1; count<=4; count++ ) {
		if ( count == 1 ) {
			p_id = point_id;
      row.pr_del = Qry.FieldAsInteger( "pr_del" );
 		  row.data.push_back( string( Qry.FieldAsString( "airline" ) ) + Qry.FieldAsString( "flt_no" ) );
		  row.data.push_back( Qry.FieldAsString( "point_id" ) );
		  row.data.push_back( "" ); //FL_SPP
		  row.data.push_back( "" ); //FL_SPP_B
		  row.data.push_back( "" ); //FL_ACT
		  row.data.push_back( "" ); //FL_BAL
		  row.data.push_back( "" ); //FL_BAL_WRK
		  row.data.push_back( Qry.FieldAsString( "suffix" ) );
		  if ( Qry.FieldIsNULL( "pr_tranzit" ) )
	  		row.data.push_back( "" );
	  	else
	  	  row.data.push_back( "*" ); //???
	  	row.data.push_back( Qry.FieldAsString( "craft" ) ); // TYPE тип ВС
  		row.data.push_back( Qry.FieldAsString( "bort" ) );
  		row.data.push_back( IntToString( Qry.FieldAsInteger( "cfg" ) ) ); //kr
  		row.data.push_back( "" ); //MODIF
  		if ( Qry.FieldIsNULL( "scd_out" ) )
  			return false;
      string region = AirpTZRegion( Qry.FieldAsString( "airp" ) );
      TDateTime d = UTCToLocal( Qry.FieldAsDateTime( "scd_out" ), region );

  		row.data.push_back( DateTimeToStr( d, "yyyymmdd" ) );
  		if ( !Qry.FieldIsNULL( "act_out" ) ) {
  			d = UTCToLocal( Qry.FieldAsDateTime( "act_out" ), region );
  			row.data.push_back( DateTimeToStr( d, "yyyymmdd" ) );
  			row.data.push_back( DateTimeToStr( d, "hhnn" ) );
  	  }
  		else {
  		  row.data.push_back( "" );
  		  row.data.push_back( DateTimeToStr( d, "hhnn" ) );
  		}
  		row.data.push_back( Qry.FieldAsString( "park_out" ) );
  		row.data.push_back( "0" ); //SEATLIM
  		row.data.push_back( "0" ); //MAILLIM
  		row.data.push_back( "0" ); //CREW
  		row.data.push_back( "0" ); //STEW
  		row.data.push_back( "0" ); //FEED
  		row.data.push_back( "0" ); //FUEL_TO
  		row.data.push_back( "0" ); //FUEL_USE
  		row.data.push_back( "0" ); //TOW_LIM
  		row.data.push_back( " " ); //CAP
  		row.data.push_back( Qry.FieldAsString( "airline" ) );
  		row.data.push_back( Qry.FieldAsString( "airline" ) ); // owner = company ???
  		row.data.push_back( "0" ); //NR_SM
  		row.data.push_back( " " ); //WEEK
  		row.data.push_back( "0" ); //OPER_BASIC
  		row.data.push_back( "0" ); //CREW_DOW
  		row.data.push_back( "0" ); //STEW_DOW
  		row.data.push_back( "0" ); //FEED_DOW
		}
		// переход на следующий пункт посадки
		if ( DestsQry.Eof )
			p_id = -1;
		else {
		  p_id = DestsQry.FieldAsInteger( "point_id" );
		}
		if ( p_id < 0 )
			row.data.push_back( "" );
		else
  		row.data.push_back( DestsQry.FieldAsString( "airp" ) );
  		//13
	  getTripCountsOnDest( p_id, lug, row.data );
	  if ( !DestsQry.Eof )
	    DestsQry.Next();
	}
	getTripCountsOnClass( lug, "П", row.data );
	getTripCountsOnClass( lug, "Б", row.data );
	getTripSeats( lug, row.data );
	row.data.push_back( "0" ); //LIM
	row.data.push_back( "0" ); //SUMMA
	row.data.push_back( "0" ); //MAX_TOW
	row.data.push_back( "0" ); //PAS_VZR
	row.data.push_back( "0" ); //PAS_RB
	row.data.push_back( "0" ); //PAS_RM
	row.data.push_back( "0" ); //PAST
	row.data.push_back( "0" ); //ZAGR_K
	row.data.push_back( "0" ); //BAG_VS
	row.data.push_back( "0" ); //BAGPL
	row.data.push_back( "0" ); //GR_VS
	row.data.push_back( "0" ); //GRT
	row.data.push_back( "0" ); //POCT_VS
	row.data.push_back( "0" ); //POCTT
	row.data.push_back( "0" ); //HOLD1
	row.data.push_back( "0" ); //HOLD2
	row.data.push_back( "0" ); //HOLD3
	row.data.push_back( "0" ); //HOLD4
	row.data.push_back( "0" ); //HOLD5
	row.data.push_back( "0" ); //HOLD6
	row.data.push_back( "0" ); //HOLD7
	row.data.push_back( "0" ); //HOLD8
	row.data.push_back( "0" ); //HOLD9
	row.data.push_back( "0" ); //HOLD10
	row.data.push_back( "0" ); //HOLD11
	row.data.push_back( "0" ); //HOLD12
	row.data.push_back( "0" ); //HOLD13
	row.data.push_back( "0" ); //HOLD14
	row.data.push_back( "0" ); //HOLD15
	row.data.push_back( "0" ); //HOLD16
	row.data.push_back( "0" ); //HOLD17
	row.data.push_back( "0" ); //HOLD18
	row.data.push_back( "0" ); //HOLD19
	row.data.push_back( "0" ); //HOLD20
	row.data.push_back( "0" ); //HOLDUP1
  row.data.push_back( "0" ); //HOLDUP2
  row.data.push_back( "0" ); //HOLDUP3
  row.data.push_back( "0" ); //HOLDUP4
  row.data.push_back( "0" ); //HOLDUP5
  row.data.push_back( "0" ); //HOLDUP6
  row.data.push_back( "0" ); //HOLDUP7
  row.data.push_back( "0" ); //HOLDUP8
  row.data.push_back( "0" ); //HOLDUP9
  row.data.push_back( "0" ); //HOLDUP10
  row.data.push_back( "0" ); //HOLDUP11
  row.data.push_back( "0" ); //HOLDUP12
  row.data.push_back( "0" ); //HOLDUP13
  row.data.push_back( "0" ); //HOLDUP14
  row.data.push_back( "0" ); //HOLDUP15
  row.data.push_back( "0" ); //HOLDUP16
  row.data.push_back( "0" ); //HOLDUP17
  row.data.push_back( "0" ); //HOLDUP18
  row.data.push_back( "0" ); //HOLDUP19
  row.data.push_back( "0" ); //HOLDUP20
  row.data.push_back( " " ); //PAS_ROW
  row.data.push_back( "0" ); //PASS_1
  row.data.push_back( "0" ); //PASS_2
  row.data.push_back( "0" ); //PASS_3
  row.data.push_back( "0" ); //PASS_4
  row.data.push_back( "0" ); //PASS_5
  row.data.push_back( "0" ); //PASS_6
  row.data.push_back( "0" ); //PASS_7
  row.data.push_back( "0" ); //PASS_8
  row.data.push_back( "0" ); //PASS_9
  row.data.push_back( "0" ); //PASS_10
  row.data.push_back( "0" ); //PASS_11
  row.data.push_back( "0" ); //PASS_12
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
