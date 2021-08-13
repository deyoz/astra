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

void createFileParamsSofi( int point_id, string pult, const string &point_addr, map<string,string> &params )
{
  DB::TQuery QryLock(PgOra::getRWSession("SOFI_FILES"), STDLOG);
  QryLock.SQLText =
      "SELECT file_no, time "
      "FROM sofi_files "
      "WHERE point_addr = :point_addr "
      "AND desk = :desk "
      "FOR UPDATE ";
  QryLock.CreateVariable("point_addr", otString, point_addr);
  QryLock.CreateVariable("desk", otString, pult);
  QryLock.Execute();

  int file_no = 0;
  TDateTime now_local = Now();
  if (!QryLock.Eof) {
    TDateTime today;
    modf(now_local, &today);

    file_no = QryLock.FieldAsInteger("file_no");
    TDateTime file_date = QryLock.FieldAsDateTime("time");
    modf(file_date, &file_date);

    if (file_date != today) {
      file_no = 0;
      DB::TQuery QryUpd(PgOra::getRWSession("SOFI_FILES"), STDLOG);
      QryUpd.SQLText =
          "UPDATE sofi_files "
          "SET file_no = :file_no, "
          "time = :now_local "
          "WHERE point_addr = :point_addr "
          "AND desk = :desk ";
      QryUpd.CreateVariable("file_no", otInteger, 1);
      QryUpd.CreateVariable("now_local", otDate, now_local);
      QryUpd.CreateVariable("point_addr", otString, point_addr);
      QryUpd.CreateVariable("desk", otString, pult);
      QryUpd.Execute();
    } else {
      DB::TQuery QryUpd(PgOra::getRWSession("SOFI_FILES"), STDLOG);
      QryUpd.SQLText =
          "UPDATE sofi_files "
          "SET file_no = :file_no "
          "WHERE point_addr = :point_addr "
          "AND desk = :desk ";
      QryUpd.CreateVariable("file_no", otInteger, file_no + 1);
      QryUpd.CreateVariable("point_addr", otString, point_addr);
      QryUpd.CreateVariable("desk", otString, pult);
      QryUpd.Execute();
    }
  } else {
    file_no = 0;
    DB::TQuery QryIns(PgOra::getRWSession("SOFI_FILES"), STDLOG);
    QryIns.SQLText =
        "INSERT INTO sofi_files(file_no, point_addr, desk, time) "
        "VALUES(:file_no, :point_addr, :desk, :now_local) ";
    QryIns.CreateVariable("file_no", otInteger, 1);
    QryIns.CreateVariable("now_local", otDate, now_local);
    QryIns.CreateVariable("point_addr", otString, point_addr);
    QryIns.CreateVariable("desk", otString, pult);
    QryIns.Execute();
  }

  ostringstream res;
  res << setw(0) << DateTimeToStr(NowUTC(), "yymmdd");
  res << pult;
  res << file_no;
  res << setw(0)<<".txt";
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

  // TODO: !!! �஢���� �ଠ� 䠩�� SOFI, �.�. point_id � scd_out ����� ���� �� ��������� !!!
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
	//1 ��� ������������ CHAR(3) ��(���客� ������), ���(����� �����)
	res << setfill(' ') << std::fixed << setw(3) << "���" << dlmt;
	//2 ��㦥���� ���ଠ�� CHAR(X)
	res << "_BAGGAGE_" << dlmt;
	//3 ���� ������ Z, M, CHA, C�� CHAR(3)
  if ( !basic_form_type.empty() && basic_form_type[ 0 ] == 'M' ) // replace lat M to rus M
    basic_form_type[ 0 ] = '�';
  res << setw(3) << basic_form_type << dlmt;
  //4 ����� ������ NUMBER(10)
  string no=QryReceipts.FieldAsString( "no" );
  res << TrimString(no) << dlmt;
  //5 ������ ��� CHAR(3)
  res<<QryReceipts.FieldAsString( "aircode" ) << dlmt;

  string val = CityTZRegion( sale_city );
  TDateTime d = UTCToLocal( QryReceipts.FieldAsDateTime( "issue_date" ), val );
  //6 ��� �த��� CHAR(10) 10.10.2006
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

 res<<buf.str(); //???10.3(2) "rate" - ����� ���, pay_rate - ����� ������
 res<<dlmt; //13

 buf.str("");
 buf<<pay_rate*QryReceipts.FieldAsInteger( "ex_weight" ); //???12.3
 res<<buf.str(); //???10.3(2) "rate" - ����� ���, pay_rate - ����� ������
 res<<dlmt<<setprecision(0);
 res<<pult;
 res<<dlmt; //15
 res<<dlmt; //15
 TFileData fd;
 fd.file_data = res.str();
 createFileParamsSofi( point_id, pult, point_addr, fd.params );
 if ( !fd.file_data.empty() )
	fds.push_back( fd );
 return !fds.empty();
}
