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
	TQuery Qry( &OraSession );
	Qry.SQLText = "SELECT 1 as nnn FROM dual"; //!!! надо считать
	Qry.Execute();
	ostringstream res;
	res<<setw(0)<<DateTimeToStr(NowUTC(),"dd.mm.yyyy");
	res<<setfill('0')<<setw(3)<<Qry.FieldAsInteger( "nnn" );
	res<<setw(0)<<".txt";
  params[ PARAM_FILE_NAME ] =  res.str();
}

bool createSofiFile( int receipt_id, std::map<std::string,std::string> &params, std::string &file_data )
{
	tst();
  TQuery Qry(&OraSession);
  Qry.SQLText=
    "SELECT TO_CHAR(bag_receipts.no) no,bag_receipts.form_type,bag_receipts.aircode,bag_receipts.issue_date,"
    "       bag_receipts.pax,bag_receipts.airp_dep,bag_receipts.airp_arv,"
    "       bag_receipts.airline,bag_receipts.flt_no,bag_receipts.suffix,bag_receipts.grp_id,bag_receipts.ex_weight, "
    "       bag_receipts.rate,bag_receipts.pay_rate,bag_receipts.ex_weight,bag_receipts.issue_user_id, "
    "       desks.sale_point as sale_point, sale_points.city as sale_city, "
    "       points.scd_out, users2.descr "
    " FROM bag_receipts, desks, sale_points, pax_grp, points, users2 "
    " WHERE receipt_id=:id AND "
    "       bag_receipts.issue_desk=desks.code AND "
    "       desks.sale_point=sale_points.code AND "
    "       bag_receipts.grp_id=pax_grp.grp_id AND "
    "       pax_grp.point_dep=points.point_id AND "
    "       bag_receipts.issue_user_id=users2.user_id ";
	Qry.CreateVariable( "id", otInteger, receipt_id );
	Qry.Execute();
	tst();
	if ( Qry.Eof )
		return false;
	tst();
	ostringstream res;
	res<<setfill(' ')<<std::fixed<<setw(3)<<"КПБ";
	res<<'\t'; //1
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
 res<<'\t'; //2
 res<<setw(10)<<Qry.FieldAsString( "no" );
 res<<'\t'; //3
 res<<setw(3)<<Qry.FieldAsString( "aircode" );
 res<<'\t'; //4
 string val = CityTZRegion( Qry.FieldAsString( "sale_city" ) );
 TDateTime d = UTCToLocal( Qry.FieldAsDateTime( "issue_date" ), val );
 res<<setw(10)<<DateTimeToStr( d, "dd.mm.yyyy");
 res<<'\t'; //5
 res<<setw(50)<<Qry.FieldAsString( "pax" );
 res<<'\t'; //6
 TAirpsRow *row=(TAirpsRow*)&base_tables.get("airps").get_row("code",Qry.FieldAsString("airp_dep")); 
 if ( row->city.empty() )
   res<<setw(3)<<Qry.FieldAsString("airp_dep"); //???4
 else
 	 res<<setw(3)<<row->city; //???4
 res<<'\t'; //7
 row=(TAirpsRow*)&base_tables.get("airps").get_row("code",Qry.FieldAsString("airp_arv")); 
 if ( row->city.empty() )
   res<<setw(3)<<Qry.FieldAsString("airp_arv"); //???4
 else
 	 res<<setw(3)<<row->city; 	//???4
 res<<'\t'; //8
 res<<setw(3)<<Qry.FieldAsString( "airline" );
 res<<'\t'; //9
 res<<setw(6)<<string(Qry.FieldAsString( "flt_no" )) + Qry.FieldAsString( "suffix" );
 res<<'\t'; //10
 val = AirpTZRegion( Qry.FieldAsString("airp_dep") );
 d = UTCToLocal( Qry.FieldAsDateTime( "scd_out" ), val );
 res<<setw(10)<<DateTimeToStr( d, "dd.mm.yyyy");
 res<<'\t'; //11
 res<<setprecision(2); 
 res<<setw(11)<<Qry.FieldAsInteger( "ex_weight" ); //???9.3(2)
 res<<'\t'; //12
 res<<setw(12)<<Qry.FieldAsFloat( "pay_rate" ); //???10.3(2) "rate" - валюта тарифа, pay_rate - валюта оплаты
 res<<'\t'; //13
 res<<Qry.FieldAsFloat( "pay_rate" )*Qry.FieldAsFloat( "ex_weight" ); //???12.3
 res<<'\t'<<setprecision(0);
 res<<setw(20)<<Qry.FieldAsString( "descr" ); //???20 а может надо login
 res<<'\t'; //15
 res<<setw(10)<<Qry.FieldAsString( "sale_point" )<<setw(0); 
 //res<<'\t'; //16
 file_data = res.str();
 createFileParamsSofi( receipt_id, params ); 
 return true;
}


