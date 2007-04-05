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
#include "base_tables.h"
#include "astra_utils.h"


using namespace std;
using namespace EXCEPTIONS;
using namespace BASIC;	

/*
NR					C	8			- ����� ३�
ID					N	9			- �����䨪��� ३�
FL_SPP			C	1 
FL_SPP_B 		C	1
FL_ACT			C	1
FL_BAL			C	1
FL_BAL_WRK	C	1
DOP					C	1			- ��⪠ "�������⥫��"
TRANZIT			C	1			- ��⪠ "�࠭����"
TYPE				C	4
BORT				C	9			- ����� ����
KR					N	3			- �᫮ ���ᠦ��᪨� ��ᥫ
MODIF				C	1
PLANDAT			D	8			- �������� ��� �뫥�
FACTDAT			D	8			- 䠪��᪠� ��� �뫥�
TIME				C	4			- �६� �뫥�
STAND				N	2			- ����� ��ﭪ�
SEATLIM			N	3
MAILLIM			N	5
CREW				N	2
STEW				N	2
FEED				N	3
FUEL_TO			N	6
FUEL_USE		N 6
TOW_LIM			N	6
CAP					C	14
COMPANY			C	3			- ������������
OWNER				C	3			- �������� �� (������������)
NR_SM				N	1
WEEK				C	7
OPER_BASIC	N	1
CREW_DOW		N	2
STEW_DOW		N	2
FEED_DOW		N	4
PORT1				C	3			- �㭪� ��ᠤ��
T_MAN1			N	3
T_RB1				N	3
T_RM1				N	2
T_RK1				N	4
T_BAG1			N	5
T_PBAG1			N	4
T_CARGO1		N	6
T_MAIL1			N	5
D_MAN1			N	3			- ����� ���ᠦ���
D_RB1				N	3			- ��� ����訥
D_RM1				N	2			- ��� �����쪨�
D_RK1				N	4			- ��筠� �����
D_BAG1			N	5			- �����
D_PBAG1			N	4			- ����� �����
D_CARGO1		N	6     - ��� ???
D_MAIL1			N	5     - ���� ???
PORT2				C	3			- �㭪� ��ᠤ��
T_MAN2			N	3
T_RB2				N	3
T_RM2				N	2
T_RK2				N	4
T_BAG2			N	5
T_PBAG2			N	4
T_CARGO2		N	6     
T_MAIL2			N	5
D_MAN2			N	3			- ����� ���ᠦ���
D_RB2				N	3			- ��� ����訥
D_RM2				N	2			- ��� �����쪨�
D_RK2				N	4			- ��筠� �����
D_BAG2			N	5			- �����
D_PBAG2			N	4			- ����� �����
D_CARGO2		N	6     - ��� ???
D_MAIL2			N	5     - ���� ???
PORT3				C	3			- �㭪� ��ᠤ��
T_MAN3			N	3
T_RB3				N	3
T_RM3				N	2
T_RK3				N	4
T_BAG3			N	5
T_PBAG3			N	4
T_CARGO3		N	6
T_MAIL3			N	5
D_MAN3			N	3			- ����� ���ᠦ���
D_RB3				N	3			- ��� ����訥
D_RM3				N	2			- ��� �����쪨�
D_RK3				N	4			- ��筠� �����
D_BAG3			N	5			- �����
D_PBAG3			N	4			- ����� �����
D_CARGO3		N	6     - ��� ???
D_MAIL3			N	5     - ���� ???
PORT4				C	3			- �㭪� ��ᠤ��
T_MAN4			N	3
T_RB4				N	3
T_RM4				N	2
T_RK4				N	4
T_BAG4			N	5
T_PBAG4			N	4
T_CARGO4		N	6
T_MAIL4			N	5
D_MAN4			N	3			- ����� ���ᠦ���
D_RB4				N	3			- ��� ����訥
D_RM4				N	2			- ��� �����쪨�
D_RK4				N	4			- ��筠� �����
D_BAG4			N	5			- �����
D_PBAG4			N	4			- ����� �����
D_CARGO4		N	6     - ��� ???
D_MAIL4			N	5     - ���� ???
I_MAN				N	3			- ����� ���ᠦ��� ��ࢮ�� �����
I_RB				N	3			- ��� ����訥 ��ࢮ�� �����
I_RM				N	2			- ��� �����쪨� ��ࢮ�� �����
I_RK				N	4			- ��筠� ����� ��ࢮ�� �����
I_BAG				N	5			- ����� ��ࢮ�� �����
I_PBAG			N	4			- ����� ����� ��ࢮ�� �����
B_MAN				N	3			- ����� ���ᠦ��� ������ �����
B_RB				N	3			- ��� ����訥 ������ �����
B_RM				N	2			- ��� �����쪨� ������ �����
B_RK				N	4			- ��筠� ����� ������ �����
B_BAG				N	5			- ����� ������ �����
B_PBAG			N	4			- ����� ����� ������ �����
K_MAN				N	3			- �᫮ ������� ��ᥫ ����묨 ���ᠦ�ࠬ�
K_RB				N	3			- �᫮ ������� ��ᥫ ����訬� ���쬨
K_RM				N	2			- �᫮ ������� ��ᥫ �����쪨�� ���쬨
K_SL				N	4			- �᫮ ������� ��ᥫ �㦥��� ���ᠦ���
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
	FlightQry.SQLText = "SELECT airline,flt_no,suffix FROM points WHERE point_id=:point_id";
	FlightQry.CreateVariable( "point_id", otInteger, point_id );
	FlightQry.Execute();
	if ( !FlightQry.RowCount() )
		throw Exception( "Flight not found in createFileParams" );
	params[ PARAM_FILE_NAME ] = string( FlightQry.FieldAsString( "airline" ) ) + 
	                            FlightQry.FieldAsString( "flt_no" ) + 
	                            FlightQry.FieldAsString( "suffix" ) + ".dbf";
	params[ PARAM_WORK_DIR ] = FileDirectory;
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
	 pay_bag_weight += p->excess;
	}
	for ( vector<Cargo>::iterator p=lug.vcargo.begin(); p!=lug.vcargo.end(); p++ ) {
		if ( p->point_arv != point_arv )
			continue;
	  cargo_weight += p->cargo;
	  mail_weight += p->mail;
	}
  data.push_back( IntToString( adult ) ); // ����� ���ᠦ���
  data.push_back( IntToString( child ) ); // ��� ����訥
  data.push_back( IntToString( baby ) ); // ��� �����쪨�
  data.push_back( IntToString( rk_weight ) ); // ��筠� �����
  data.push_back( IntToString( bag_weight ) ); // ��筠� �����
	data.push_back( IntToString( pay_bag_weight ) ); // ����� �����    
	data.push_back( IntToString( cargo_weight ) ); // ���    ???	
	data.push_back( IntToString( mail_weight ) ); // ����    ???		
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
	data.push_back( IntToString( adult ) ); // ����� ���ᠦ��� ��ࢮ�� �����
	data.push_back( IntToString( child ) ); // ��� ����訥 ��ࢮ�� �����
	data.push_back( IntToString( baby ) ); // ��� �����쪨� ��ࢮ�� �����
	data.push_back( IntToString( rk_weight ) ); // ��筠� ����� ��ࢮ�� �����
	data.push_back( IntToString( bag_weight ) ); // ����� ��ࢮ�� �����
	data.push_back( IntToString( pay_bag_weight ) ); // ����� ����� ��ࢮ�� �����
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
	data.push_back( IntToString( seatsadult ) ); // �᫮ ������� ��ᥫ ����묨 ���ᠦ�ࠬ�
	data.push_back( IntToString( seatschild ) ); // �᫮ ������� ��ᥫ ����訬� ���쬨
	data.push_back( IntToString( seatsbaby ) ); // �᫮ ������� ��ᥫ �����쪨�� ���쬨
//	data.push_back( "0" ); // �᫮ ������� ��ᥫ �㦥��� ���ᠦ��� !!!
}

bool createCentringFile( int point_id, const string &Sender, const string &Receiver )
{
	Develop_dbf dbf;
	dbf.AddField( "NR", 'C', 8 ); // ����� ३� 3 - Company + 5 flt_no + ??? suffix
	dbf.AddField( "ID", 'N', 9 ); // �����䨪��� ३�
	dbf.AddField( "DOP", 'C', 1 ); // ��⪠ "�������⥫��"
	dbf.AddField( "TRANZIT", 'C', 1 ); // ��⪠ "�࠭����"
	dbf.AddField( "BORT", 'C', 9 ); // ����� ����
	dbf.AddField( "KR", 'N', 3 ); // �᫮ ���ᠦ��᪨� ��ᥫ
	dbf.AddField( "PLANDAT", 'D', 8 ); // �������� ��� �뫥�
	dbf.AddField( "FACTDAT", 'D', 8 ); // 䠪��᪠� ��� �뫥�
	dbf.AddField( "TIME", 'C', 4 ); // �६� �뫥�
	dbf.AddField( "STAND", 'C', 2 ); // ����� ��ﭪ�
	dbf.AddField( "COMPANY", 'C', 3 ); // ������������
	dbf.AddField( "OWNER", 'C', 3 ); // �������� �� (������������)
	dbf.AddField( "PORT1", 'C', 3 ); // �㭪� ��ᠤ��
	dbf.AddField( "D_MAN1", 'N', 3 ); // ����� ���ᠦ���
	dbf.AddField( "D_RB1", 'N', 3 ); // ��� ������
	dbf.AddField( "D_RM1", 'N', 2 ); // ��� �����쪨�
	dbf.AddField( "D_RK1", 'N', 4 ); // ��筠� �����
	dbf.AddField( "D_BAG1", 'N', 5 ); // �����
	dbf.AddField( "D_PBAG1", 'N', 4 ); // ����� �����
	dbf.AddField( "D_CARGO1", 'N', 6 ); // ��� ???
	dbf.AddField( "D_MAIL1", 'N', 5 ); // ���� ???
	dbf.AddField( "PORT2", 'C', 3 ); // �㭪� ��ᠤ��
	dbf.AddField( "D_MAN2", 'N', 3 ); // ����� ���ᠦ���
	dbf.AddField( "D_RB2", 'N', 3 ); // ��� ������
	dbf.AddField( "D_RM2", 'N', 2 ); // ��� �����쪨�
	dbf.AddField( "D_RK2", 'N', 4 ); // ��筠� �����
	dbf.AddField( "D_BAG2", 'N', 5 ); // �����
	dbf.AddField( "D_PBAG2", 'N', 4 ); // ����� �����
	dbf.AddField( "D_CARGO2", 'N', 6 ); // ��� ???
	dbf.AddField( "D_MAIL2", 'N', 5 ); // ���� ???	
	dbf.AddField( "PORT3", 'C', 3 ); // �㭪� ��ᠤ��
	dbf.AddField( "D_MAN3", 'N', 3 ); // ����� ���ᠦ���
	dbf.AddField( "D_RB3", 'N', 3 ); // ��� ������
	dbf.AddField( "D_RM3", 'N', 2 ); // ��� �����쪨�
	dbf.AddField( "D_RK3", 'N', 4 ); // ��筠� �����
	dbf.AddField( "D_BAG3", 'N', 5 ); // �����
	dbf.AddField( "D_PBAG3", 'N', 4 ); // ����� �����
	dbf.AddField( "D_CARGO3", 'N', 6 ); // ��� ???	
	dbf.AddField( "D_MAIL3", 'N', 5 ); // ���� ???		
	dbf.AddField( "PORT4", 'C', 3 ); // �㭪� ��ᠤ��
	dbf.AddField( "D_MAN4", 'N', 3 ); // ����� ���ᠦ���
	dbf.AddField( "D_RB4", 'N', 3 ); // ��� ������
	dbf.AddField( "D_RM4", 'N', 2 ); // ��� �����쪨�
	dbf.AddField( "D_RK4", 'N', 4 ); // ��筠� �����
	dbf.AddField( "D_BAG4", 'N', 5 ); // �����
	dbf.AddField( "D_PBAG4", 'N', 4 ); // ����� �����
	dbf.AddField( "D_CARGO4", 'N', 6 ); // ��� ???	
	dbf.AddField( "D_MAIL4", 'N', 5 ); // ���� ???		
	dbf.AddField( "I_MAN", 'N', 3 ); // ����� ���ᠦ��� ��ࢮ�� �����
	dbf.AddField( "I_RB", 'N', 3 ); // ��� ����訥 ��ࢮ�� �����
	dbf.AddField( "I_RM", 'N', 2 ); // ��� �����쪨� ��ࢮ�� �����
	dbf.AddField( "I_RK", 'N', 4 ); // ��筠� ����� ��ࢮ�� �����
	dbf.AddField( "I_BAG", 'N', 5 ); // ����� ��ࢮ�� �����
	dbf.AddField( "I_PBAG", 'N', 4 ); // ����� ����� ��ࢮ�� �����
	dbf.AddField( "B_MAN", 'N', 3 ); // ����� ���ᠦ��� ������ �����
	dbf.AddField( "B_RB", 'N', 3 ); // ��� ����訥 ������ �����
	dbf.AddField( "B_RM", 'N', 2 ); // ��� �����쪨� ������ �����
	dbf.AddField( "B_RK", 'N', 4 ); // ��筠� ����� ������ �����
	dbf.AddField( "B_BAG", 'N', 5 ); // ����� ������ �����
	dbf.AddField( "B_PBAG", 'N', 4 ); // ����� ����� ������ �����
	dbf.AddField( "K_MAN", 'N', 3 ); // �᫮ ������� ��ᥫ ����묨 ���ᠦ�ࠬ�
	dbf.AddField( "K_RB", 'N', 3 ); // �᫮ ������� ��ᥫ ����訬� ���쬨
	dbf.AddField( "K_RM", 'N', 2 ); // �᫮ ������� ��ᥫ �����쪨�� ���쬨
//???	dbf.AddField( "K_SL", 'N', 4 ); // �᫮ ������� ��ᥫ �㦥��� ���ᠦ���	
	ProgTrace( TRACE5, "dbf.fieldCount=%d", dbf.fieldCount() );
	TQuery Qry( &OraSession );
	Qry.SQLText = 
	 "SELECT points.point_num,points.point_id,pr_tranzit,DECODE(points.pr_tranzit,0,points.point_id,points.first_point) first_point,"
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
  TQuery DestsQry( &OraSession );
  DestsQry.SQLText = 
   "SELECT point_num,point_id,airp FROM points "
   " WHERE first_point=:first_point AND point_num>:point_num and pr_del=0 "
   " ORDER BY point_num ";
  DestsQry.CreateVariable( "first_point", otInteger, Qry.FieldAsInteger( "first_point" ) );
  DestsQry.CreateVariable( "point_num", otInteger, Qry.FieldAsInteger( "point_num" ) );
  DestsQry.Execute();
  tst();
  DBFRow row;
  Luggage lug;
  GetLuggage( point_id, lug, false ); // false - wo pr_brd=1
  TBaseTable &cities = base_tables.get( "cities" );

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
  		row.data.push_back( Qry.FieldAsString( "airline" ) );
  		row.data.push_back( Qry.FieldAsString( "airline" ) ); // owner = company ???		
		}
		// ���室 �� ᫥���騩 �㭪� ��ᠤ��
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
	getTripCountsOnClass( lug, "�", row.data );
	getTripCountsOnClass( lug, "�", row.data );
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
