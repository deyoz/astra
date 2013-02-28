#include "basel_aero.h"

#include <string>
#include <vector>
#include <tcl.h>
#include "base_tables.h"

#include "basic.h"
#include "stl_utils.h"
#include "astra_consts.h"
#include "astra_utils.h"
#include "xml_unit.h"
#include "base_tables.h"
#include "astra_context.h"
#include "jxtlib/xml_stuff.h"
#include "serverlib/logger.h"
#include "http_io.h"

#define NICKNAME "DJEK"
#define NICKTRACE DJEK_TRACE
#include "serverlib/test.h"

using namespace BASIC;
using namespace EXCEPTIONS;



using namespace std;
using namespace EXCEPTIONS;
using namespace BASIC;
using namespace ASTRA;

struct TPaxWanted {
    TDateTime transactionDate;
    string airline;
    string flightNumber; // ���䨪�???
    TDateTime departureDate;
    string rackNumber;
    string seatNumber;
    string firstName;
    string lastName;
    string patronymic;
    string documentNumber;
    string operationType;
    string baggageReceiptsNumber;
    string departureAirport;
    string arrivalAirport;
    string baggageWeight;
    string PNR;
    string visaNumber;
    TDateTime visaDate;
    string visaPlace;
    string visaCountryCode;
    string nationality;
    string gender;
};

bool filter_passenger( const string &airp_dep, const string &airp_arv, const string &nationality )
{
/*1.        ���, 㫥����/�ਫ����� � ��ய���� �� ����ਨ ��������⠭� ��� ����ᨬ��� �� �� �������⢠
  2.        ��� ��������� ��������⠭�, 㫥����/�ਫ����� � ��ய���� �ᥣ� ���, ����� ��室�� �१ ��⥬� Astra .*/
  TBaseTable &basecities = base_tables.get( "cities" );
  TBaseTable &baseairps = base_tables.get( "airps" );
  if ( nationality == "TJK" ||
       ((TCitiesRow&)basecities.get_row( "code", ((TAirpsRow&)baseairps.get_row( "code", airp_dep, true )).city)).country == "��" ||
       ((TCitiesRow&)basecities.get_row( "code", ((TAirpsRow&)baseairps.get_row( "code", airp_arv, true )).city)).country == "��" )
    return true;
  return false;
}

string make_soap_content(const vector<TPaxWanted> &paxs)
{
    ostringstream result;
    result <<
        "<soapenv:Envelope xmlns:soapenv='http://schemas.xmlsoap.org/soap/envelope/' xmlns:sir='http://vtsft.ru/sirenaSearchService/'>\n"
        "   <soapenv:Header/>\n"
        "   <soapenv:Body>\n"
        "      <sir:importASTDateRequest>\n"
        "         <sir:login>test</sir:login>\n";
    for(vector<TPaxWanted>::const_iterator iv = paxs.begin(); iv != paxs.end(); iv++)
        result <<
            "         <sir:policyParameters>\n"
            "            <sir:transactionDate>" << DateTimeToStr(iv->transactionDate, "yyyy-mm-dd") << "</sir:transactionDate>\n"
            "            <sir:transactionTime>" << DateTimeToStr(iv->transactionDate, "yyyymmddhhnnss") << "</sir:transactionTime>\n"
            "            <sir:flightNumber>" << iv->flightNumber << "</sir:flightNumber>\n"
            "            <sir:departureDate>" << (iv->departureDateDate==NoExists?"":TimeToStr(iv->departureDate, "yyyy-mm-dd")) << "</sir:departureDate>\n"
            "            <sir:rackNumber>" << iv->rackNumber << "</sir:rackNumber>\n"
            "            <sir:seatNumber>" << iv->seatNumber << "</sir:seatNumber>\n"
            "            <sir:firstName>" << iv->firstName << "</sir:firstName>\n"
            "            <sir:lastName>" << iv->lastName << "</sir:lastName>\n"
            "            <sir:patronymic>" << iv->patronymic << "</sir:patronymic>\n"
            "            <sir:documentNumber>" << iv->documentNumber << "</sir:documentNumber>\n"
            "            <sir:operationType>" << iv->operationType << "</sir:operationType>\n"
            "            <sir:baggageReceiptsNumber>" << iv->baggageReceiptsNumber << "</sir:baggageReceiptsNumber>\n"
            "            <sir:airline>" << iv->airline << "</sir:airline>\n"
            "            <sir:departureAirport>" << iv->departureAirport << "</sir:departureAirport>\n"
            "            <sir:arrivalAirport>" << iv->arrivalAirport << "</sir:arrivalAirport>\n"
            "            <sir:baggageWeight>" << iv->baggageWeight << "</sir:baggageWeight>\n"
            "            <sir:departureTime>" << (iv->departureDate == NoExists ? "" : DateTimeToStr(iv->departureDate, "yyyymmddhhnnss")) << "</sir:departureTime>\n"
            "            <sir:PNR>" << iv->PNR << "</sir:PNR>\n"
            "            <sir:visaNumber>" << iv->visaNumber << "</sir:visaNumber>\n"
            "            <sir:visaDate>" << (iv->visaDate==NoExists?"":DateTimeToStr(iv->visaDate, "yyyy-mm-dd")) << "</sir:visaDate>\n"
            "            <sir:visaPlace>" << iv->visaPlace << "</sir:visaPlace>\n"
            "            <sir:visaCountryCode>" << iv->visaCountryCode << "</sir:visaCountryCode>\n"
//            "            <sir:nationality>" << iv->nationality << "</sir:nationality>\n"
            "            <sir:sex>" << iv->gender << "</sir:sex>\n"
            "         </sir:policyParameters>\n";
    result <<
        "      </sir:importASTDateRequest>\n"
        "   </soapenv:Body>\n"
        "</soapenv:Envelope>";
    return ConvertCodepage(result.str(), "CP866", "UTF-8");
}

void sync_sirena_wanted( TDateTime utcdate )
{
  ProgTrace(TRACE5,"sync_sirena_wanted started");
  vector<TPaxWanted> paxs;
  paxs.clear();
  string prior_val;
  TDateTime prior_time = ASTRA::NoExists;
  if ( AstraContext::GetContext( "sync_sirena_wanted", 0, prior_val ) != ASTRA::NoExists ) {
    if ( StrToDateTime( prior_val.c_str(), "dd.mm.yyyy hh:nn", prior_time ) == EOF )
      throw Exception( "get_pax_wanted: invalid context 'sync_sirena_wanted' %s", prior_val.c_str() );
  }
  TQuery Qry(&OraSession);
  Qry.SQLText =
      "SELECT time, "
      "       airline,flt_no,suffix, "
      "       takeoff, "
      "       SUBSTR(term,1,6) AS term, "
      "       seat_no, "
      "       SUBSTR(surname,1,20) AS surname, "
      "       SUBSTR(LTRIM(RTRIM(SUBSTR(name||' ',1,INSTR(name||' ',' ')))),1,20) AS name, "
      "       SUBSTR(LTRIM(RTRIM(SUBSTR(name||' ',INSTR(name||' ',' ')+1))),1,20) AS patronymic, "
      "       SUBSTR(document,1,20) AS document, "
      "       operation, "
      "       tags, "
      "       airp_dep, "
      "       airp_arv, "
      "       bag_weight, "
      "       SUBSTR(pnr,1,12) AS pnr, "
      "       visano,issue_date,issue_place,"
      "       nationality,NVL(gender,'N') gender,applic_country "
      "FROM rozysk "
      "WHERE time>=:time "
      "ORDER BY time";
  if ( prior_time == ASTRA::NoExists )
    prior_time =  NowUTC() - 10.0/1440.0;
  Qry.CreateVariable( "time", otDate, prior_time );
  Qry.Execute();
  prior_time = ASTRA::NoExists;
  int idx_time = Qry.FieldIndex( "time" );
  int idx_airline = Qry.FieldIndex( "airline" );
  int idx_flt_no = Qry.FieldIndex( "flt_no" );
  int idx_suffix = Qry.FieldIndex( "suffix" );
  int idx_takeoff = Qry.FieldIndex( "takeoff" );
  int idx_term = Qry.FieldIndex( "term" );
  int idx_surname = Qry.FieldIndex( "surname" );
  int idx_name = Qry.FieldIndex( "name" );
  int idx_patronymic = Qry.FieldIndex( "patronymic" );
  int idx_document = Qry.FieldIndex( "document" );
  int idx_airp_dep = Qry.FieldIndex( "airp_dep" );
  int idx_airp_arv = Qry.FieldIndex( "airp_arv" );
  int idx_seat_no = Qry.FieldIndex( "seat_no" );
  int idx_bag_weight = Qry.FieldIndex( "bag_weight" );
  int idx_tags = Qry.FieldIndex( "tags" );
  int idx_pnr = Qry.FieldIndex( "pnr" );
  int idx_operation = Qry.FieldIndex( "operation" );
  int idx_visano = Qry.FieldIndex( "visano" );
  int idx_issue_date = Qry.FieldIndex( "issue_date" );
  int idx_issue_place = Qry.FieldIndex( "issue_place" );
  int idx_nationality = Qry.FieldIndex( "nationality" );
  int idx_gender = Qry.FieldIndex( "gender" );
  int idx_applic_country = Qry.FieldIndex( "applic_country" );

  for ( ;!Qry.Eof; Qry.Next() ) {
    if ( prior_time < Qry.FieldAsDateTime( idx_time ) )
      prior_time = Qry.FieldAsDateTime( idx_time );
    if ( !filter_passenger( Qry.FieldAsString( idx_airp_dep ),
                            Qry.FieldAsString( idx_airp_arv ),
                            Qry.FieldAsString( idx_nationality ) ) )
      continue;
    TPaxWanted pax;
    pax.transactionDate = Qry.FieldAsDateTime( idx_time );
    pax.airline = Qry.FieldAsString( idx_airline );
    pax.flightNumber = string(Qry.FieldAsString( idx_flt_no )) + Qry.FieldAsString( idx_suffix );
    if ( Qry.FieldIsNULL( idx_takeoff ) )
      pax.departureDate = ASTRA::NoExists;
    else
      pax.departureDate = Qry.FieldAsDateTime( idx_takeoff );
    pax.rackNumber = Qry.FieldAsString( idx_term );
    pax.seatNumber = Qry.FieldAsString( idx_seat_no );
    pax.firstName = Qry.FieldAsString( idx_surname );
    pax.lastName = Qry.FieldAsString( idx_name );
    pax.patronymic = Qry.FieldAsString( idx_patronymic );
    pax.documentNumber = Qry.FieldAsString( idx_document );
    pax.operationType = Qry.FieldAsString( idx_operation );
    pax.baggageReceiptsNumber = Qry.FieldAsString( idx_tags );
    pax.departureAirport = Qry.FieldAsString( idx_airp_dep );
    pax.arrivalAirport = Qry.FieldAsString( idx_airp_arv );
    pax.baggageWeight = Qry.FieldAsString( idx_bag_weight );
    pax.PNR = Qry.FieldAsString( idx_pnr );
    pax.visaNumber = Qry.FieldAsString( idx_visano );
    if ( Qry.FieldIsNULL( idx_issue_date ) )
      pax.visaDate = ASTRA::NoExists;
    else
      pax.visaDate = Qry.FieldAsDateTime( idx_issue_date );
    pax.visaPlace = Qry.FieldAsString( idx_issue_place );
    pax.visaCountryCode = Qry.FieldAsString( idx_applic_country );
    pax.gender = Qry.FieldAsString( idx_gender );
    paxs.push_back( pax );
  }
  ProgTrace( TRACE5, "pax.size()=%d", paxs.size() );
  sirena_wanted_send(make_soap_content(paxs));
  ProgTrace(TRACE5, "sirena_wanted_send completed");
  
  if ( prior_time != ASTRA::NoExists ) {
    AstraContext::ClearContext( "sync_sirena_wanted", 0 );
    AstraContext::SetContext( "sync_sirena_wanted", 0, DateTimeToStr( prior_time, "dd.mm.yyyy hh:nn" ) );
  }
}

