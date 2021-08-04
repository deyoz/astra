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
#include "astra_main.h"
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
  DB::TQuery QryReceipts(PgOra::getRWSession("BAG_RECEIPTS"), STDLOG);
  QryReceipts.SQLText=
    "SELECT "
    + std::string(PgOra::supportsPg("BAG_RECEIPTS")
                  ? "TO_CHAR(no) AS no, "
                  : "CAST(no AS varchar) AS no, ") +
    "form_type, aircode, issue_date, pax_name, tickets, airp_dep, airp_arv, "
    "airline, flt_no, suffix, grp_id, ex_weight, rate, rate_cur, exch_rate, "
    "exch_pay_rate, pay_rate_cur, ex_weight, issue_user_id, issue_desk "
    "FROM bag_receipts  "
    "WHERE receipt_id= :id "
    "AND service_type IN (1,2)";
  QryReceipts.CreateVariable( "id", otInteger, receipt_id );
  QryReceipts.Execute();
  if (QryReceipts.Eof) {
    return false;
  }

  const std::string form_type_code = QryReceipts.FieldAsString("form_type");
  const int issue_user_id = QryReceipts.FieldAsInteger("issue_user_id");
  const std::string issue_desk = QryReceipts.FieldAsString("issue_desk");
  const int grp_id = QryReceipts.FieldAsInteger("grp_id");

  DB::TQuery QryFormTypes(PgOra::getROSession("FORM_TYPES"), STDLOG);
  QryFormTypes.SQLText=
    "SELECT form_types.basic_type AS basic_form_type,form_types.validator "
    "FROM form_types "
    "WHERE form_types.code = :form_type ";
  QryFormTypes.CreateVariable("form_type", otString, form_type_code);
  QryFormTypes.Execute();
  if (QryFormTypes.Eof) {
    return false;
  }
  std::string basic_form_type = QryFormTypes.FieldAsString("basic_form_type");
  const std::string validator = QryFormTypes.FieldAsString("validator");

  DB::TQuery QryUsers(PgOra::getRWSession("USERS2"), STDLOG);
  QryUsers.SQLText=
    "SELECT 1 FROM users2 "
    "WHERE user_id = :user_id ";
  QryUsers.CreateVariable("user_id", otInteger, issue_user_id);
  QryUsers.Execute();
  if (QryUsers.Eof) {
    return false;
  }

  DB::TQuery QryDesks(PgOra::getRWSession({"DESKS", "DESK_GRP"}), STDLOG);
  QryDesks.SQLText=
      "SELECT desks.code pult, desk_grp.city as sale_city  "
      "FROM desks, desk_grp "
      "WHERE desks.code = :issue_desk "
      "AND desks.grp_id = desk_grp.grp_id ";
  QryDesks.CreateVariable("issue_desk", otString, issue_desk);
  QryDesks.Execute();
  if (QryDesks.Eof) {
    return false;
  }
  const std::string pult = QryDesks.FieldAsString("pult");
  const std::string sale_city = QryDesks.FieldAsString("sale_city");

  // TODO: !!! Проверить формат файла SOFI, т.к. point_id и scd_out могут быть не заполнены !!!
  int point_id = 0;
  TTripInfo flt;
  if (flt.getByGrpId(grp_id)) {
    point_id = flt.point_id;
  }
  const TDateTime scd_out = flt.scd_out;

  for(map<string,string>::iterator im = inparams.begin(); im != inparams.end(); im++) {
 	  if ( im->first == SOFI_AGENCY_PARAMS ) {
      DB::TQuery QryAgency(PgOra::getROSession({"SALE_DESKS", "SALE_POINTS"}), STDLOG);
      QryAgency.SQLText =
          "SELECT sale_points.agency "
          "FROM sale_desks, sale_points "
          "WHERE sale_desks.code=:code "
          "AND sale_desks.validator=:validator "
          "AND sale_points.code=sale_desks.sale_point "
          "AND sale_points.validator=sale_desks.validator";
      QryAgency.CreateVariable("code", otString, issue_desk);
      QryAgency.CreateVariable("validator", otString, validator);
      QryAgency.Execute();
      if (QryAgency.Eof ||
          im->second != QryAgency.FieldAsString("agency")) return false;
 	  	break;
 	  }
 	}
	const string dlmt = "``|``";
	ostringstream res;
	//1 ВИД ДЕЯТЕЛЬНОСТИ CHAR(3) СП(Страховые полисы), КПБ(платный багаж)
	res << setfill(' ') << std::fixed << setw(3) << "КПБ" << dlmt;
	//2 Служебная информация CHAR(X)
	res << "_BAGGAGE_" << dlmt;
	//3 Серия бланка Z, M, CHA, CБА CHAR(3)
  if ( !basic_form_type.empty() && basic_form_type[ 0 ] == 'M' ) // replace lat M to rus M
    basic_form_type[ 0 ] = 'М';
  res << setw(3) << basic_form_type << dlmt;
  //4 Номер бланка NUMBER(10)
  string no=QryReceipts.FieldAsString( "no" );
  res << TrimString(no) << dlmt;
  //5 Расчетный код CHAR(3)
  res<<QryReceipts.FieldAsString( "aircode" ) << dlmt;

  string val = CityTZRegion( sale_city );
  TDateTime d = UTCToLocal( QryReceipts.FieldAsDateTime( "issue_date" ), val );
  //6 Дата продажи CHAR(10) 10.10.2006
  res << DateTimeToStr( d, "dd.mm.yyyy") << dlmt;

 // surname & name
 {
     string pax_name = QryReceipts.FieldAsString( "pax_name" );
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

 string tickets=QryReceipts.FieldAsString( "tickets" );
 res << TrimString(tickets) << dlmt;

 TAirpsRow *row=(TAirpsRow*)&base_tables.get("airps").get_row("code",QryReceipts.FieldAsString("airp_dep"));
 if ( row->city.empty() )
   res<<QryReceipts.FieldAsString("airp_dep"); //???4
 else
 	 res<<row->city; //???4
 res<<dlmt; //7
 row=(TAirpsRow*)&base_tables.get("airps").get_row("code",QryReceipts.FieldAsString("airp_arv"));
 if ( row->city.empty() )
   res<<QryReceipts.FieldAsString("airp_arv"); //???4
 else
 	 res<<row->city; 	//???4
 res<<dlmt; //8
 res<<QryReceipts.FieldAsString( "airline" );
 res<<dlmt; //9
 res << setw(3) << setfill('0') << QryReceipts.FieldAsInteger( "flt_no" )
     << QryReceipts.FieldAsString( "suffix" );
 res<<dlmt; //10
 val = AirpTZRegion( QryReceipts.FieldAsString("airp_dep") );
 d = UTCToLocal( scd_out, val );
 res<<DateTimeToStr( d, "dd.mm.yyyy");
 res<<dlmt; //11
 res << setprecision(2);
 res<<QryReceipts.FieldAsInteger( "ex_weight" );
 res<<dlmt; //12

 //короче, баклан
 //не забывай про вывод цифр после запятой для тарифа и суммы
 //для этого используй процедуру RateToString из print.cc c fmt_type=1
 //эта процедура сама все сформатирует
 //
 //связь с рейсом рвется для аннулированных квитанций только тогда, когда этот рейс уходит в архив
 //поэтому пока не заморачивайся по поводу grp_id=NULL
 //
 //вызов данной процедуры помести в PaymentInterface::PrintReceipt
 //сразу после строки "rcpt_id=PutReceipt(rcpt,grp_id);"

 //я ничего не отлаживал и не успел исправить вышеуказанные проблемы

 //считывать из базы конкретную телеграмму можно также с помощью
 //процедуры bool PaymentInterface::GetReceipt(int id, TBagReceipt &rcpt)
 //тогда ты можешь работать с rcpt (см. тип TBagReceipt)
 //однако кое-какую информацию ты все равно должен взять отдельным запросом


 string rate_cur=QryReceipts.FieldAsString("rate_cur");
 string pay_rate_cur=QryReceipts.FieldAsString("pay_rate_cur");
 double pay_rate;
 if (rate_cur!=pay_rate_cur)
   pay_rate = QryReceipts.FieldAsFloat( "rate" ) * QryReceipts.FieldAsFloat( "exch_pay_rate" ) /
              QryReceipts.FieldAsInteger( "exch_rate" );
 else
   pay_rate = QryReceipts.FieldAsFloat( "rate" );

 ostringstream buf;
 buf << std::fixed << setprecision(2) << pay_rate;

 res<<buf.str(); //???10.3(2) "rate" - валюта тарифа, pay_rate - валюта оплаты
 res<<dlmt; //13

 buf.str("");
 buf<<pay_rate*QryReceipts.FieldAsInteger( "ex_weight" ); //???12.3
 res<<buf.str(); //???10.3(2) "rate" - валюта тарифа, pay_rate - валюта оплаты
 res<<dlmt<<setprecision(0);
 res<<pult;
 res<<dlmt; //15
 res<<dlmt; //15
 TFileData fd;
 fd.file_data = res.str();
 createFileParamsSofi( point_id, receipt_id, pult, point_addr, fd.params );
 if ( !fd.file_data.empty() )
	fds.push_back( fd );
 return !fds.empty();
}
