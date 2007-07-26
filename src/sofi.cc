#include "sofi.h"

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

void createFileParamsSofi( int receipt_id, map<string,string> &params )
{
	//!!! ���� �����
	TQuery Qry( &OraSession );
	Qry.SQLText = 
	 "BEGIN "
	 " SELECT n INTO :nnn FROM sofi; "
	 " UPDATE sofi SET n=:nnn+1; "
	 " EXCEPTION WHEN NO_DATA_FOUND THEN "
	 "   :nnn := 0; "
	 "   INSERT INTO sofi VALUES( 1 ); "
	 "END; ";
	Qry.CreateVariable( "nnn", otInteger, 0 );
	tst();
	Qry.Execute();
	tst();
	ostringstream res;
	res<<setw(0)<<DateTimeToStr(NowUTC(),"dd.mm.yyyy");
	res<<setfill('0')<<setw(3)<<Qry.GetVariableAsInteger( "nnn" );
	res<<setw(0)<<".txt";
  params[ PARAM_FILE_NAME ] =  res.str();
}

bool createSofiFile( int receipt_id, std::map<std::string,std::string> &params, std::string &file_data )
{
	tst();
  TQuery Qry(&OraSession);
  Qry.SQLText=
    "SELECT TO_CHAR(bag_receipts.no) no,bag_receipts.form_type,bag_receipts.aircode,bag_receipts.issue_date, "
    "      bag_receipts.pax_name,bag_receipts.airp_dep,bag_receipts.airp_arv, "
    "      bag_receipts.airline,bag_receipts.flt_no,bag_receipts.suffix,bag_receipts.grp_id,bag_receipts.ex_weight,  "
    "      bag_receipts.rate,bag_receipts.rate_cur, "
    "      bag_receipts.exch_rate,bag_receipts.exch_pay_rate,bag_receipts.pay_rate_cur, "
    "      bag_receipts.ex_weight,bag_receipts.issue_user_id,  "
    "      desks.sale_point as sale_point, sale_points.city as sale_city,  "
    "      points.scd_out, users2.descr  "
    "FROM bag_receipts, desks, sale_points, pax_grp, points, users2  "
    "WHERE receipt_id= :id AND  "
    "      bag_receipts.service_type IN (1,2) AND "
    "      bag_receipts.issue_desk=desks.code AND  "
    "      desks.sale_point=sale_points.code AND  "
    "      bag_receipts.grp_id=pax_grp.grp_id(+) AND  "
    "      pax_grp.point_dep=points.point_id(+) AND  "
    "      bag_receipts.issue_user_id=users2.user_id";
	Qry.CreateVariable( "id", otInteger, receipt_id );
	Qry.Execute();
	tst();
	if ( Qry.Eof )
		return false;
	const string dlmt = "``|``";
	ostringstream res;
	res<<setfill(' ')<<std::fixed<<setw(3)<<"���";
	res<<dlmt; //1 ��� ������������ CHAR(3) ��(���客� ������), ���(����� �����)
	res<<"_BAGGAGE_";
	res<<dlmt; //2 ��㦥���� ���ଠ�� CHAR(X)
	string t = Qry.FieldAsString( "form_type" );
	if ( t == "M61" )
		res<<setw(3)<<"M";
	else
		if ( t == "Z61" )
			res<<setw(3)<<"Z";
		else
			if ( t == "35" )
				res<<setw(3)<<" "; //???
			else
				res<<setw(3)<<" "; //???
 res<<dlmt; //3 ���� ������ Z, M, CHA, C�� CHAR(3)
 res<<setw(10)<<string(Qry.FieldAsString( "no" )).substr(0,10);
 res<<dlmt; //4 ����� ������ NUMBER(10)
 res<<setw(3)<<Qry.FieldAsString( "aircode" );
 res<<dlmt; //5 ������ ��� CHAR(3)
 string val = CityTZRegion( Qry.FieldAsString( "sale_city" ) );
 TDateTime d = UTCToLocal( Qry.FieldAsDateTime( "issue_date" ), val );
 res<<setw(10)<<DateTimeToStr( d, "dd.mm.yyyy");
 res<<dlmt; //6 ��� �த��� CHAR(10) 10.10.2006
 res<<setw(50)<<Qry.FieldAsString( "pax_name" );
 res<<dlmt; //6
 TAirpsRow *row=(TAirpsRow*)&base_tables.get("airps").get_row("code",Qry.FieldAsString("airp_dep"));
 if ( row->city.empty() )
   res<<setw(3)<<Qry.FieldAsString("airp_dep"); //???4
 else
 	 res<<setw(3)<<row->city; //???4
 res<<dlmt; //7
 row=(TAirpsRow*)&base_tables.get("airps").get_row("code",Qry.FieldAsString("airp_arv"));
 if ( row->city.empty() )
   res<<setw(3)<<Qry.FieldAsString("airp_arv"); //???4
 else
 	 res<<setw(3)<<row->city; 	//???4
 res<<dlmt; //8
 res<<setw(3)<<Qry.FieldAsString( "airline" );
 res<<dlmt; //9
 res<<setw(6)<<string(Qry.FieldAsString( "flt_no" )) + Qry.FieldAsString( "suffix" );
 res<<dlmt; //10
 val = AirpTZRegion( Qry.FieldAsString("airp_dep") );
 d = UTCToLocal( Qry.FieldAsDateTime( "scd_out" ), val );
 res<<setw(10)<<DateTimeToStr( d, "dd.mm.yyyy");
 res<<dlmt; //11
 res<<setprecision(2);
 res<<setw(11)<<Qry.FieldAsInteger( "ex_weight" ); //???9.3(2)
 res<<dlmt; //12

 //����, ������
 //�� ���뢠� �� �뢮� ��� ��᫥ ����⮩ ��� ��� � �㬬�
 //��� �⮣� �ᯮ��� ��楤��� RateToString �� print.cc c fmt_type=1
 //�� ��楤�� ᠬ� �� ��ଠ����
 //
 //��� � ३ᮬ ࢥ��� ��� ���㫨஢����� ���⠭権 ⮫쪮 ⮣��, ����� ��� ३� �室�� � ��娢
 //���⮬� ���� �� �����稢���� �� ������ grp_id=NULL
 //
 //�맮� ������ ��楤��� ������ � PaymentInterface::PrintReceipt
 //�ࠧ� ��᫥ ��ப� "rcpt_id=PutReceipt(rcpt,grp_id);"

 //� ��祣� �� �⫠����� � �� �ᯥ� ��ࠢ��� ���㪠����� �஡����

 //���뢠�� �� ���� �������� ⥫��ࠬ�� ����� ⠪�� � �������
 //��楤��� bool PaymentInterface::GetReceipt(int id, TBagReceipt &rcpt)
 //⮣�� �� ������ ࠡ���� � rcpt (�. ⨯ TBagReceipt)
 //������ ���-����� ���ଠ�� �� �� ࠢ�� ������ ����� �⤥��� ����ᮬ


 string rate_cur=Qry.FieldAsString("rate_cur");
 string pay_rate_cur=Qry.FieldAsString("pay_rate_cur");
 double pay_rate;
 if (rate_cur!=pay_rate_cur)
   pay_rate = Qry.FieldAsFloat( "rate" ) * Qry.FieldAsFloat( "exch_pay_rate" ) /
              Qry.FieldAsInteger( "exch_rate" );
 else
   pay_rate = Qry.FieldAsFloat( "rate" );

 res<<setw(12)<<pay_rate; //???10.3(2) "rate" - ����� ���, pay_rate - ����� ������
 res<<dlmt; //13
 res<<pay_rate*Qry.FieldAsInteger( "ex_weight" ); //???12.3
 res<<dlmt<<setprecision(0);
 res<<setw(20)<<Qry.FieldAsString( "descr" ); //???20 � ����� ���� login
 res<<dlmt; //15
 res<<setw(10)<<Qry.FieldAsString( "sale_point" )<<setw(0);
 //res<<'\t'; //16
 file_data = res.str();
 createFileParamsSofi( receipt_id, params );
 return true;
}


