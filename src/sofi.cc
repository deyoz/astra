#include "sofi.h"

#include <string>
#include <sstream>
#include <iomanip>
#include <vector>
#include <map>
#include <algorithm>
#include <sstream>

#include "exceptions.h"
#include "date_time.h"
#include "stl_utils.h"
#include "base_tables.h"
#include "astra_utils.h"
//#include "astra_service.h"
#include "file_queue.h"
#define NICKNAME "DJEK"
#define NICKTRACE DJEK_TRACE
#include "serverlib/slogger.h"

using namespace std;
using namespace EXCEPTIONS;
using namespace BASIC::date_time;

void createFileParamsSofi( int point_id, int receipt_id, string pult, const string &point_addr, map<string,string> &params )
{
	TQuery Qry( &OraSession );
	Qry.SQLText =
     "declare "
     "  time date; "
	 "BEGIN "
	 " SELECT file_no, time INTO :file_no, time FROM sofi_files "
     "  where "
     " point_addr = :point_addr and "
     " desk = :desk for update; "
     " if trunc(time) <> trunc(sysdate) then "
     "   :file_no := 0; "
     "   update sofi_files set file_no = 1, time = sysdate where "
     "    point_addr = :point_addr and "
     "    desk = :desk; "
     " else "
	 " UPDATE sofi_files SET file_no=:file_no+1 where point_addr = :point_addr and "
     "    desk = :desk; "
     " end if; "
	 " EXCEPTION WHEN NO_DATA_FOUND THEN "
	 "   :file_no := 0; "
	 "   INSERT INTO sofi_files(file_no, point_addr, desk, time) VALUES( 1, :point_addr, :desk, sysdate ); "
	 "END; ";
	Qry.CreateVariable( "file_no", otInteger, 0 );
	Qry.CreateVariable( "point_addr", otString, point_addr );
	Qry.CreateVariable( "desk", otString, pult );
	tst();
	Qry.Execute();
	ostringstream res;
	res <<setw(0)<<DateTimeToStr(NowUTC(),"yymmdd");
    res << pult;
	res <<Qry.GetVariableAsInteger( "file_no" );
	res <<setw(0)<<".txt";
    ProgTrace(TRACE5, "params.size = %zu", params.size());
    for(map<string,string>::iterator im = params.begin(); im != params.end(); im++) {
        ProgTrace(TRACE5, "params[%s] = %s", im->first.c_str(), im->second.c_str());
    }
  params[ PARAM_FILE_NAME ] =  res.str();
  params[ NS_PARAM_EVENT_TYPE ] = EncodeEventType( ASTRA::evtFlt );
	params[ NS_PARAM_EVENT_ID1 ] = IntToString( point_id );
  params[ PARAM_TYPE ] = VALUE_TYPE_FILE; // FILE
}

bool createSofiFile( int receipt_id, std::map<std::string,std::string> &inparams,
                     const std::string &point_addr, TFileDatas &fds )
{
	ProgTrace( TRACE5, "inparams.size()=%zu", inparams.size() );
  TQuery Qry(&OraSession);
  Qry.SQLText=
    "SELECT TO_CHAR(bag_receipts.no) no,bag_receipts.form_type,bag_receipts.aircode,bag_receipts.issue_date, "
    "      bag_receipts.pax_name,bag_receipts.tickets,bag_receipts.airp_dep,bag_receipts.airp_arv, "
    "      bag_receipts.airline,bag_receipts.flt_no,bag_receipts.suffix,bag_receipts.grp_id,bag_receipts.ex_weight,  "
    "      bag_receipts.rate,bag_receipts.rate_cur, "
    "      bag_receipts.exch_rate,bag_receipts.exch_pay_rate,bag_receipts.pay_rate_cur, "
    "      bag_receipts.ex_weight,bag_receipts.issue_user_id, bag_receipts.issue_desk, "
    "      desks.code pult, desk_grp.city as sale_city,  "
    "      points.scd_out, users2.descr, points.point_id point_id,  "
    "      form_types.basic_type AS basic_form_type,form_types.validator "
    "FROM bag_receipts, form_types, desks, desk_grp, pax_grp, points, users2  "
    "WHERE bag_receipts.form_type=form_types.code AND "
    "      receipt_id= :id AND  "
    "      bag_receipts.service_type IN (1,2) AND "
    "      bag_receipts.issue_desk=desks.code AND  "
    "      desks.grp_id=desk_grp.grp_id AND  "
    "      bag_receipts.grp_id=pax_grp.grp_id(+) AND  "
    "      pax_grp.point_dep=points.point_id(+) AND  "
    "      bag_receipts.issue_user_id=users2.user_id";
	Qry.CreateVariable( "id", otInteger, receipt_id );
	Qry.Execute();
	if ( Qry.Eof )
		return false;
	int point_id = Qry.FieldAsInteger( "point_id" );

  for(map<string,string>::iterator im = inparams.begin(); im != inparams.end(); im++) {
 	  if ( im->first == SOFI_AGENCY_PARAMS ) {
 	    TQuery AgencyQry(&OraSession);
      AgencyQry.Clear();
      AgencyQry.SQLText=
       "SELECT sale_points.agency FROM sale_desks, sale_points "
       "WHERE sale_desks.code=:code AND sale_desks.validator=:validator AND "
       " sale_points.code=sale_desks.sale_point AND sale_points.validator=sale_desks.validator";
      AgencyQry.CreateVariable( "code", otString, Qry.FieldAsString( "issue_desk" ) );
      AgencyQry.CreateVariable( "validator", otString, Qry.FieldAsString("validator") );
 	    AgencyQry.Execute();
 	    if (AgencyQry.Eof ||
 	        im->second != AgencyQry.FieldAsString("agency")) return false;
 	  	break;
 	  }
 	}
	const string dlmt = "``|``";
	ostringstream res;
	//1 ??? ???????????? CHAR(3) ??(????????? ??????), ???(??????? ?????)
	res << setfill(' ') << std::fixed << setw(3) << "???" << dlmt;
	//2 ????????? ?????????? CHAR(X)
	res << "_BAGGAGE_" << dlmt;
	//3 ????? ?????? Z, M, CHA, C?? CHAR(3)
	string form_type = Qry.FieldAsString( "basic_form_type" );
	if ( !form_type.empty() && form_type[ 0 ] == 'M' ) // replace lat M to rus M
		form_type[ 0 ] = '?';
  res << setw(3) << form_type << dlmt;
  //4 ????? ?????? NUMBER(10)
  string no=Qry.FieldAsString( "no" );
  res << TrimString(no) << dlmt;
  //5 ????????? ??? CHAR(3)
  res<<Qry.FieldAsString( "aircode" ) << dlmt;

  string val = CityTZRegion( Qry.FieldAsString( "sale_city" ) );
  TDateTime d = UTCToLocal( Qry.FieldAsDateTime( "issue_date" ), val );
  //6 ???? ??????? CHAR(10) 10.10.2006
  res << DateTimeToStr( d, "dd.mm.yyyy") << dlmt;

 // surname & name
 {
     string pax_name = Qry.FieldAsString( "pax_name" );
     TrimString(pax_name);
     string surname;
     string::size_type pos = pax_name.find(" ");
     if(pos != string::npos) {
         surname = pax_name.substr(0, pos);
         pax_name = pax_name.substr(pos + 1);
     }
     res<<TrimString(surname).substr(0, 50);
     res<<dlmt;
     res<<TrimString(pax_name).substr(0, 50);
     res<<dlmt; //6
 }

 string tickets=Qry.FieldAsString( "tickets" );
 res << TrimString(tickets) << dlmt;

 TAirpsRow *row=(TAirpsRow*)&base_tables.get("airps").get_row("code",Qry.FieldAsString("airp_dep"));
 if ( row->city.empty() )
   res<<Qry.FieldAsString("airp_dep"); //???4
 else
 	 res<<row->city; //???4
 res<<dlmt; //7
 row=(TAirpsRow*)&base_tables.get("airps").get_row("code",Qry.FieldAsString("airp_arv"));
 if ( row->city.empty() )
   res<<Qry.FieldAsString("airp_arv"); //???4
 else
 	 res<<row->city; 	//???4
 res<<dlmt; //8
 res<<Qry.FieldAsString( "airline" );
 res<<dlmt; //9
 res << setw(3) << setfill('0') << Qry.FieldAsInteger( "flt_no" )
     << Qry.FieldAsString( "suffix" );
 res<<dlmt; //10
 val = AirpTZRegion( Qry.FieldAsString("airp_dep") );
 d = UTCToLocal( Qry.FieldAsDateTime( "scd_out" ), val );
 res<<DateTimeToStr( d, "dd.mm.yyyy");
 res<<dlmt; //11
 res << setprecision(2);
 res<<Qry.FieldAsInteger( "ex_weight" );
 res<<dlmt; //12

 //??????, ??????
 //?? ??????? ??? ????? ???? ????? ??????? ??? ?????? ? ?????
 //??? ????? ????????? ????????? RateToString ?? print.cc c fmt_type=1
 //??? ????????? ???? ??? ????????????
 //
 //????? ? ?????? ?????? ??? ?????????????? ????????? ?????? ?????, ????? ???? ???? ?????? ? ?????
 //??????? ???? ?? ????????????? ?? ?????? grp_id=NULL
 //
 //????? ?????? ????????? ??????? ? PaymentInterface::PrintReceipt
 //????? ????? ?????? "rcpt_id=PutReceipt(rcpt,grp_id);"

 //? ?????? ?? ????????? ? ?? ????? ????????? ????????????? ????????

 //????????? ?? ???? ?????????? ?????????? ????? ????? ? ???????
 //????????? bool PaymentInterface::GetReceipt(int id, TBagReceipt &rcpt)
 //????? ?? ?????? ???????? ? rcpt (??. ??? TBagReceipt)
 //?????? ???-????? ?????????? ?? ??? ????? ?????? ????? ????????? ????????


 string rate_cur=Qry.FieldAsString("rate_cur");
 string pay_rate_cur=Qry.FieldAsString("pay_rate_cur");
 double pay_rate;
 if (rate_cur!=pay_rate_cur)
   pay_rate = Qry.FieldAsFloat( "rate" ) * Qry.FieldAsFloat( "exch_pay_rate" ) /
              Qry.FieldAsInteger( "exch_rate" );
 else
   pay_rate = Qry.FieldAsFloat( "rate" );

 ostringstream buf;
 buf << std::fixed << setprecision(2) << pay_rate;

 res<<buf.str(); //???10.3(2) "rate" - ?????? ??????, pay_rate - ?????? ??????
 res<<dlmt; //13

 buf.str("");
 buf<<pay_rate*Qry.FieldAsInteger( "ex_weight" ); //???12.3
 res<<buf.str(); //???10.3(2) "rate" - ?????? ??????, pay_rate - ?????? ??????
 res<<dlmt<<setprecision(0);
 res<<Qry.FieldAsString( "pult" );
 res<<dlmt; //15
 res<<dlmt; //15
 TFileData fd;
 fd.file_data = res.str();
 createFileParamsSofi( point_id, receipt_id, Qry.FieldAsString( "pult" ), point_addr, fd.params );
 if ( !fd.file_data.empty() )
	fds.push_back( fd );
 return !fds.empty();
}
