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
#include "http_io.h"
#include "passenger.h"
#include "jxtlib/xml_stuff.h"
#include "serverlib/logger.h"

#define NICKNAME "VLAD"
#define NICKTRACE SYSTEM_TRACE
#include "serverlib/test.h"

using namespace std;
using namespace EXCEPTIONS;
using namespace BASIC;
using namespace ASTRA;

const std::string ROZYSK_MAGISTRAL = "ROZYSK_MAGISTRAL";
const std::string ROZYSK_MAGISTRAL_24 = "ROZYSK_MAGISTRAL_24";
const std::string ROZYSK_SIRENA = "ROZYSK_SIRENA";

namespace rozysk
{

struct TRow {
    //дополнительно
    TDateTime time;
    string term;
    //рейс
    string airline;
    int flt_no;
    string suffix;
    TDateTime takeoff;
    string airp_dep;
    //пассажир
    string airp_arv;
    string surname;
    string name;
    string seat_no;
    int bag_weight;
    string tags;
    string pnr;
    string operation;
    //документ
    string doc_no;
    string doc_nationality;
    string doc_gender;
    //виза
    string visa_no;
    string visa_issue_place;
    TDateTime visa_issue_date;
    string visa_applic_country;
    TRow()
    {
      time=NoExists;
      flt_no=NoExists;
      takeoff=NoExists;
      bag_weight=NoExists;
      visa_issue_date=NoExists;
    };
    TRow& fltFromDB(TQuery &Qry);
    TRow& paxFromDB(TQuery &Qry);
    TRow& setDoc(const CheckIn::TPaxDocItem &doc);
    TRow& setVisa(const CheckIn::TPaxDocoItem &doc);
    const TRow& toDB(TQuery &Qry) const;
};

TRow& TRow::fltFromDB(TQuery &Qry)
{
  airline=Qry.FieldAsString("airline");
  flt_no=Qry.FieldIsNULL("flt_no")?NoExists:Qry.FieldAsInteger("flt_no");
  suffix=Qry.FieldAsString("suffix");
  takeoff=Qry.FieldIsNULL("takeoff")?NoExists:Qry.FieldAsDateTime("takeoff");
  airp_dep=Qry.FieldAsString("airp_dep");
  if (takeoff!=NoExists && Qry.FieldAsInteger("is_utc")!=0)
    takeoff=UTCToLocal(takeoff, AirpTZRegion(airp_dep));
  return *this;
};

TRow& TRow::paxFromDB(TQuery &Qry)
{
  airp_arv=Qry.FieldAsString("airp_arv");
  surname=Qry.FieldAsString("surname");
  name=Qry.FieldAsString("name");
  seat_no=Qry.FieldAsString("seat_no");
  bag_weight=Qry.FieldIsNULL("bag_weight")?NoExists:Qry.FieldAsInteger("bag_weight");
  tags=Qry.FieldAsString("tags");
  pnr=Qry.FieldAsString("pnr");
  operation=Qry.FieldAsString("operation");
  return *this;
};

TRow& TRow::setDoc(const CheckIn::TPaxDocItem &doc)
{
  doc_no=doc.no;
  doc_nationality=doc.nationality;
  doc_gender=doc.gender;
  return *this;
};

TRow& TRow::setVisa(const CheckIn::TPaxDocoItem &visa)
{
  if (visa.type=="V")
  {
    visa_no=visa.no;
    visa_issue_place=visa.issue_place;
    visa_issue_date=visa.issue_date;
    visa_applic_country=visa.applic_country;
  }
  else
  {
    visa_no.clear();
    visa_issue_place.clear();
    visa_issue_date=NoExists;
    visa_applic_country.clear();
  };
  return *this;
};

const TRow& TRow::toDB(TQuery &Qry) const
{
  const char* sql=
    "INSERT INTO rozysk "
    "  (time,term,airline,flt_no,suffix,takeoff,airp_dep, "
    "   airp_arv,surname,name,seat_no,bag_weight,tags,pnr,operation, "
    "   doc_no,doc_nationality,doc_gender, "
    "   visa_no,visa_issue_place,visa_issue_date,visa_applic_country) "
    "VALUES "
    "  (:time,:term,:airline,:flt_no,:suffix,:takeoff,:airp_dep, "
    "   :airp_arv,:surname,:name,:seat_no,:bag_weight,:tags,:pnr,:operation, "
    "   :doc_no,:doc_nationality,:doc_gender, "
    "   :visa_no,:visa_issue_place,:visa_issue_date,:visa_applic_country) ";
  if (strcmp(Qry.SQLText.SQLText(),sql)!=0)
  {
    Qry.Clear();
    Qry.SQLText=sql;
    Qry.DeclareVariable("time", otDate);
    Qry.DeclareVariable("term", otString);
    Qry.DeclareVariable("airline", otString);
    Qry.DeclareVariable("flt_no", otInteger);
    Qry.DeclareVariable("suffix", otString);
    Qry.DeclareVariable("takeoff", otDate);
    Qry.DeclareVariable("airp_dep", otString);
    Qry.DeclareVariable("airp_arv", otString);
    Qry.DeclareVariable("surname", otString);
    Qry.DeclareVariable("name", otString);
    Qry.DeclareVariable("seat_no", otString);
    Qry.DeclareVariable("bag_weight", otInteger);
    Qry.DeclareVariable("tags", otString);
    Qry.DeclareVariable("pnr", otString);
    Qry.DeclareVariable("operation", otString);
    Qry.DeclareVariable("doc_no", otString);
    Qry.DeclareVariable("doc_nationality", otString);
    Qry.DeclareVariable("doc_gender", otString);
    Qry.DeclareVariable("visa_no", otString);
    Qry.DeclareVariable("visa_issue_place", otString);
    Qry.DeclareVariable("visa_issue_date", otDate);
    Qry.DeclareVariable("visa_applic_country", otString);
  };
  time!=NoExists?Qry.SetVariable("time", time):
                 Qry.SetVariable("time", FNull);
  Qry.SetVariable("term", term);
  Qry.SetVariable("airline", airline);
  flt_no!=NoExists?Qry.SetVariable("flt_no", flt_no):
                   Qry.SetVariable("flt_no", FNull);
  Qry.SetVariable("suffix", suffix);
  takeoff!=NoExists?Qry.SetVariable("takeoff", takeoff):
                    Qry.SetVariable("takeoff", FNull);
  Qry.SetVariable("airp_dep", airp_dep);
  Qry.SetVariable("airp_arv", airp_arv);
  Qry.SetVariable("surname", surname);
  Qry.SetVariable("name", name);
  Qry.SetVariable("seat_no", seat_no);
  bag_weight!=NoExists?Qry.SetVariable("bag_weight", bag_weight):
                       Qry.SetVariable("bag_weight", FNull);
  Qry.SetVariable("tags", tags.substr(0,250));
  Qry.SetVariable("pnr", pnr);
  Qry.SetVariable("operation", operation);
  Qry.SetVariable("doc_no", doc_no);
  Qry.SetVariable("doc_nationality", doc_nationality);
  Qry.SetVariable("doc_gender", doc_gender);
  Qry.SetVariable("visa_no", visa_no);
  Qry.SetVariable("visa_issue_place", visa_issue_place);
  visa_issue_date!=NoExists?Qry.SetVariable("visa_issue_date", visa_issue_date):
                            Qry.SetVariable("visa_issue_date", FNull);
  Qry.SetVariable("visa_applic_country", visa_applic_country);
  Qry.Execute();
  return *this;
};

void check_flight(TQuery &Qry, int point_id)
{
  if (Qry.Eof) throw Exception("flight not found (point_id=%d)", point_id);
  if (Qry.FieldIsNULL("airline")) throw Exception("empty airline (point_id=%d)", point_id);
  if (Qry.FieldIsNULL("flt_no")) throw Exception("empty flt_no (point_id=%d)", point_id);
  if (Qry.FieldIsNULL("takeoff")) throw Exception("empty takeoff (point_id=%d)", point_id);
  if (Qry.FieldIsNULL("airp_dep")) throw Exception("empty airp_dep (point_id=%d)", point_id);
};

void get_flight(int point_id, TRow &r)
{
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText =
    "SELECT airline, flt_no, suffix, "
    "       NVL(scd_out,NVL(est_out,act_out)) AS takeoff, 1 AS is_utc, "
    "       airp AS airp_dep "
    "FROM points WHERE point_id=:point_id";
  Qry.CreateVariable("point_id", otInteger, point_id);
  Qry.Execute();
  check_flight(Qry, point_id);
  r.fltFromDB(Qry);
};

void get_crs_flight(int point_id, TRow &r)
{
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText =
    "SELECT airline, flt_no, suffix, scd AS takeoff, 0 AS is_utc, airp_dep "
    "FROM tlg_trips WHERE point_id=:point_id";
  Qry.CreateVariable("point_id", otInteger, point_id);
  Qry.Execute();
  check_flight(Qry, point_id);
  r.fltFromDB(Qry);
};

const char* pax_sql=
  "SELECT "
  "  pax_grp.point_dep AS point_id, pax_grp.airp_arv, "
  "  pax.pax_id, pax.surname, pax.name, "
  "  salons.get_seat_no(pax.pax_id,pax.seats,pax_grp.status,pax_grp.point_dep,'one',rownum) AS seat_no, "
  "  NVL(ckin.get_bagWeight2(pax_grp.grp_id,pax.pax_id,pax.bag_pool_num,rownum),0)+ "
  "  NVL(ckin.get_rkWeight2(pax_grp.grp_id,pax.pax_id,pax.bag_pool_num,rownum),0) AS bag_weight, "
  "  ckin.get_birks2(pax_grp.grp_id,pax.pax_id,pax.bag_pool_num) AS tags, "
  "  ckin.get_pax_pnr_addr(pax.pax_id) AS pnr, "
  "  DECODE(pax.refuse,NULL,DECODE(pax.pr_brd,0,DECODE(pax.pr_exam,0,'K1','K2'),'K3'),'K0') AS operation "
  "FROM pax_grp, pax "
  "WHERE pax_grp.grp_id=pax.grp_id";
  
const char* crs_pax_sql=
  "SELECT "
  "  crs_pnr.point_id, crs_pnr.airp_arv, "
  "  crs_pax.pax_id, crs_pax.surname, crs_pax.name, "
  "  salons.get_crs_seat_no(seat_xname,seat_yname,crs_pax.seats,crs_pnr.point_id,'one',rownum) AS seat_no, "
  "  NULL AS bag_weight, NULL AS tags, "
  "  ckin.get_pnr_addr(crs_pnr.pnr_id) AS pnr, "
  "  'K0' AS operation "
  "FROM crs_pnr,crs_pax,pax "
  "WHERE crs_pnr.pnr_id=crs_pax.pnr_id AND crs_pax.pr_del=0 AND "
  "      crs_pax.pax_id=pax.pax_id(+) AND pax.pax_id IS NULL";

void check_pax(TQuery &Qry, int pax_id)
{
  if (Qry.Eof) throw Exception("passenger not found (pax_id=%d)", pax_id);
  if (Qry.FieldIsNULL("airp_arv")) throw Exception("empty airp_arv (pax_id=%d)", pax_id);
  if (Qry.FieldIsNULL("surname")) throw Exception("empty surname (pax_id=%d)", pax_id);
  if (Qry.FieldIsNULL("operation")) throw Exception("empty operation(pax_id=%d)", pax_id);
};

void sync_pax_internal(int id, const string &term, bool is_grp_id)
{
  TQuery Qry(&OraSession);
  Qry.Clear();
  ostringstream sql;
  if (is_grp_id)
    sql << pax_sql << " AND pax_grp.grp_id=:grp_id";
  else
    sql << pax_sql << " AND pax.pax_id=:pax_id";
  Qry.SQLText=sql.str().c_str();
  if (is_grp_id)
    Qry.CreateVariable("grp_id", otInteger, id);
  else
    Qry.CreateVariable("pax_id", otInteger, id);
  Qry.Execute();
  if (Qry.Eof)
  {
    if (is_grp_id)
      throw Exception("group not found (grp_id=%d)", id);
    else
      throw Exception("passenger not found (pax_id=%d)", id);
  };
  TQuery InsQry(&OraSession);
  TQuery PaxDocQry(&OraSession);
  TQuery PaxDocoQry(&OraSession);
  TRow row;
  //дополнительно
  row.time=NowUTC();
  row.term=term;
  //рейс
  int point_id=Qry.FieldAsInteger("point_id");
  get_flight(point_id, row);

  for(;!Qry.Eof;Qry.Next())
  {
    int pax_id=Qry.FieldAsInteger("pax_id");
    //пассажир
    check_pax(Qry, pax_id);
    row.paxFromDB(Qry);
    //документ
    CheckIn::TPaxDocItem doc;
    LoadPaxDoc(pax_id, doc, PaxDocQry);
    row.setDoc(doc);
    //виза
    CheckIn::TPaxDocoItem doco;
    LoadPaxDoco(pax_id, doco, PaxDocoQry);
    row.setVisa(doco);

    row.toDB(InsQry);
  };
};

void sync_pax(int pax_id, const string &term)
{
  try
  {
    sync_pax_internal(pax_id, term, false);
  }
  catch(Exception &e)
  {
    ProgError(STDLOG, "rozysk::sync_pax: %s", e.what());
  }
  catch(std::exception &e)
  {
    ProgError(STDLOG, "rozysk::sync_pax: %s", e.what());
  };
};

void sync_pax_grp(int grp_id, const string &term)
{
  try
  {
    sync_pax_internal(grp_id, term, true);
  }
  catch(Exception &e)
  {
    ProgError(STDLOG, "rozysk::sync_pax_grp: %s", e.what());
  }
  catch(std::exception &e)
  {
    ProgError(STDLOG, "rozysk::sync_pax_grp: %s", e.what());
  };
};

void sync_crs_pax_internal(int id, const string &term, bool is_pnr_id)
{
  TQuery Qry(&OraSession);
  Qry.Clear();
  ostringstream sql;
  if (is_pnr_id)
    sql << crs_pax_sql << " AND crs_pnr.pnr_id=:pnr_id";
  else
    sql << crs_pax_sql << " AND crs_pax.pax_id=:pax_id";
  Qry.SQLText=sql.str().c_str();
  if (is_pnr_id)
    Qry.CreateVariable("pnr_id", otInteger, id);
  else
    Qry.CreateVariable("pax_id", otInteger, id);
  Qry.Execute();
  if (Qry.Eof) return;

  TQuery InsQry(&OraSession);
  TQuery PaxDocQry(&OraSession);
  TQuery PaxDocQry2(&OraSession);
  TQuery PaxDocoQry(&OraSession);
  TRow row;
  //дополнительно
  row.time=NowUTC();
  row.term=term;
  //рейс
  int point_id=Qry.FieldAsInteger("point_id");
  get_crs_flight(point_id, row);

  for(;!Qry.Eof;Qry.Next())
  {
    int pax_id=Qry.FieldAsInteger("pax_id");
    //пассажир
    check_pax(Qry, pax_id);
    row.paxFromDB(Qry);
    //документ
    CheckIn::TPaxDocItem doc;
    LoadCrsPaxDoc(pax_id, doc, PaxDocQry, PaxDocQry2);
    row.setDoc(doc);
    //виза
    CheckIn::TPaxDocoItem doco;
    LoadCrsPaxVisa(pax_id, doco, PaxDocoQry);
    row.setVisa(doco);

    row.toDB(InsQry);
  };
};

void sync_crs_pax(int pax_id, const string &term)
{
  try
  {
    sync_crs_pax_internal(pax_id, term, false);
  }
  catch(Exception &e)
  {
    ProgError(STDLOG, "rozysk::sync_crs_pax: %s", e.what());
  }
  catch(std::exception &e)
  {
    ProgError(STDLOG, "rozysk::sync_crs_pax: %s", e.what());
  };
};

void sync_crs_pnr(int pnr_id, const string &term)
{
  try
  {
    sync_crs_pax_internal(pnr_id, term, true);
  }
  catch(Exception &e)
  {
    ProgError(STDLOG, "rozysk::sync_crs_pnr: %s", e.what());
  }
  catch(std::exception &e)
  {
    ProgError(STDLOG, "rozysk::sync_crs_pnr: %s", e.what());
  };
};

struct TPax {
    TDateTime transactionDate;
    string airline;
    string flightNumber; // суффикс???
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
/*
  1.        Всех, улетающих/прилетающих в аэропорты на территории Таджикистана вне зависимости от их подданства
  2.        Всех подданных Таджикистана, улетающих/прилетающих в аэропорты всего мира, которые проходят через систему Astra .
*/
  TBaseTable &basecities = base_tables.get( "cities" );
  TBaseTable &baseairps = base_tables.get( "airps" );
  if ( nationality == "TJK" ||
       ((TCitiesRow&)basecities.get_row( "code", ((TAirpsRow&)baseairps.get_row( "code", airp_dep, true )).city)).country == "ТД" ||
       ((TCitiesRow&)basecities.get_row( "code", ((TAirpsRow&)baseairps.get_row( "code", airp_arv, true )).city)).country == "ТД" )
    return true;
  return false;
}

string make_soap_content(const vector<TPax> &paxs)
{
    ostringstream result;
    result <<
        "<soapenv:Envelope xmlns:soapenv='http://schemas.xmlsoap.org/soap/envelope/' xmlns:sir='http://vtsft.ru/sirenaSearchService/'>\n"
        "   <soapenv:Header/>\n"
        "   <soapenv:Body>\n"
        "      <sir:importASTDateRequest>\n"
        "         <sir:login>test</sir:login>\n";
    for(vector<TPax>::const_iterator iv = paxs.begin(); iv != paxs.end(); iv++)
        result <<
            "         <sir:policyParameters>\n"
            "            <sir:transactionDate>" << DateTimeToStr(iv->transactionDate, "yyyy-mm-dd") << "</sir:transactionDate>\n"
            "            <sir:transactionTime>" << DateTimeToStr(iv->transactionDate, "yyyymmddhhnnss") << "</sir:transactionTime>\n"
            "            <sir:flightNumber>" << iv->flightNumber << "</sir:flightNumber>\n"
            "            <sir:departureDate>" << (iv->departureDate==NoExists?"":DateTimeToStr(iv->departureDate, "yyyy-mm-dd")) << "</sir:departureDate>\n"
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
    //ProgTrace( TRACE5, "make_soap_content: return %s", result.str().c_str() );
    return ConvertCodepage(result.str(), "CP866", "UTF-8");
}

} //namespace rozysk

void sync_sirena_rozysk( TDateTime utcdate )
{
  ProgTrace(TRACE5,"sync_sirena_rozysk started");
  vector<rozysk::TPax> paxs;

  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText=
      "SELECT addr, port, http_resource, action, last_create "
      "FROM http_sets "
      "WHERE code=:code AND pr_denial=0 FOR UPDATE";
  Qry.CreateVariable("code", otString, ROZYSK_SIRENA);
  Qry.Execute();
  if (Qry.Eof) return;
  TDateTime last_time=NowUTC() - 1.0/1440.0; //отматываем минуту, так как данные последних секунд могут быть еще не закоммичены в rozysk
  TDateTime first_time = Qry.FieldIsNULL("last_create") ? last_time - 10.0/1440.0 :
                                                          Qry.FieldAsDateTime("last_create");

  if (first_time>=last_time) return;
  HTTPRequestInfo request;
  request.addr=Qry.FieldAsString("addr");
  request.port=Qry.FieldAsInteger("port");
  request.resource=Qry.FieldAsString("http_resource");
  request.action=Qry.FieldAsString("action");


  Qry.Clear();
  Qry.SQLText =
      "SELECT time, term, "
      "       airline, flt_no, suffix, takeoff, airp_dep, "
      "       airp_arv, surname, "
      "       LTRIM(RTRIM(SUBSTR(name||' ',1,INSTR(name||' ',' ')))) AS name, "
      "       LTRIM(RTRIM(SUBSTR(name||' ',INSTR(name||' ',' ')+1))) AS patronymic, "
      "       seat_no, bag_weight, tags, pnr, operation, "
      "       doc_no, doc_nationality, NVL(doc_gender,'N') AS doc_gender, "
      "       visa_no, visa_issue_place, visa_issue_date, visa_applic_country "
      "FROM rozysk "
      "WHERE time>=:first_time AND time<:last_time "
      "ORDER BY time";
  Qry.CreateVariable( "first_time", otDate, first_time );
  Qry.CreateVariable( "last_time", otDate, last_time );
  Qry.Execute();
  if (!Qry.Eof)
  {
    int idx_time = Qry.FieldIndex( "time" );
    int idx_term = Qry.FieldIndex( "term" );

    int idx_airline = Qry.FieldIndex( "airline" );
    int idx_flt_no = Qry.FieldIndex( "flt_no" );
    int idx_suffix = Qry.FieldIndex( "suffix" );
    int idx_takeoff = Qry.FieldIndex( "takeoff" );
    int idx_airp_dep = Qry.FieldIndex( "airp_dep" );

    int idx_airp_arv = Qry.FieldIndex( "airp_arv" );
    int idx_surname = Qry.FieldIndex( "surname" );
    int idx_name = Qry.FieldIndex( "name" );
    int idx_patronymic = Qry.FieldIndex( "patronymic" );
    int idx_seat_no = Qry.FieldIndex( "seat_no" );
    int idx_bag_weight = Qry.FieldIndex( "bag_weight" );
    int idx_tags = Qry.FieldIndex( "tags" );
    int idx_pnr = Qry.FieldIndex( "pnr" );
    int idx_operation = Qry.FieldIndex( "operation" );

    int idx_doc_no = Qry.FieldIndex( "doc_no" );
    int idx_doc_nationality = Qry.FieldIndex( "doc_nationality" );
    int idx_doc_gender = Qry.FieldIndex( "doc_gender" );
    int idx_visa_no = Qry.FieldIndex( "visa_no" );
    int idx_visa_issue_place = Qry.FieldIndex( "visa_issue_place" );
    int idx_visa_issue_date = Qry.FieldIndex( "visa_issue_date" );
    int idx_visa_applic_country = Qry.FieldIndex( "visa_applic_country" );

    for ( ;!Qry.Eof; Qry.Next() )
    {
      if ( !rozysk::filter_passenger( Qry.FieldAsString( idx_airp_dep ),
                                      Qry.FieldAsString( idx_airp_arv ),
                                      Qry.FieldAsString( idx_doc_nationality ) ) )
        continue;
      rozysk::TPax pax;
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
      pax.documentNumber = Qry.FieldAsString( idx_doc_no );
      pax.operationType = Qry.FieldAsString( idx_operation );
      pax.baggageReceiptsNumber = Qry.FieldAsString( idx_tags );
      pax.departureAirport = Qry.FieldAsString( idx_airp_dep );
      pax.arrivalAirport = Qry.FieldAsString( idx_airp_arv );
      pax.baggageWeight = Qry.FieldAsString( idx_bag_weight );
      pax.PNR = Qry.FieldAsString( idx_pnr );
      pax.visaNumber = Qry.FieldAsString( idx_visa_no );
      if ( Qry.FieldIsNULL( idx_visa_issue_date ) )
        pax.visaDate = ASTRA::NoExists;
      else
        pax.visaDate = Qry.FieldAsDateTime( idx_visa_issue_date );
      pax.visaPlace = Qry.FieldAsString( idx_visa_issue_place );
      pax.visaCountryCode = Qry.FieldAsString( idx_visa_applic_country );
      pax.gender = Qry.FieldAsString( idx_doc_gender );
      paxs.push_back( pax );
    }
  };
  Qry.Clear();
  Qry.SQLText =
    "UPDATE http_sets SET last_create=:last_time WHERE code=:code";
  Qry.CreateVariable("code", otString, ROZYSK_SIRENA);
  Qry.CreateVariable("last_time", otDate, last_time);
  Qry.Execute();

  ProgTrace( TRACE5, "pax.size()=%zu", paxs.size() );
  if(not paxs.empty()) {
    request.content=make_soap_content(paxs);
    sirena_rozysk_send(request);
    ProgTrace(TRACE5, "sirena_rozysk_send completed");
  }
}

#define ENDL "\r\n"

void create_mvd_file(TDateTime first_time, TDateTime last_time,
                     const char* airp, const char* tz_region, const char* file_name)
{
  ofstream f;
  f.open(file_name);
  if (!f.is_open()) throw Exception("Can't open file '%s'",file_name);
  try
  {
    TQuery Qry(&OraSession);
    Qry.Clear();
    Qry.SQLText=
      "SELECT time, "
      "       airline,flt_no,suffix, "
      "       takeoff, "
      "       SUBSTR(term,1,6) AS term, "
      "       seat_no, "
      "       SUBSTR(surname,1,20) AS surname, "
      "       SUBSTR(LTRIM(RTRIM(SUBSTR(name||' ',1,INSTR(name||' ',' ')))),1,20) AS name, "
      "       SUBSTR(LTRIM(RTRIM(SUBSTR(name||' ',INSTR(name||' ',' ')+1))),1,20) AS patronymic, "
      "       SUBSTR(doc_no,1,20) AS doc_no, "
      "       operation, "
      "       tags, "
      "       airp_dep, "
      "       airp_arv, "
      "       bag_weight, "
      "       SUBSTR(pnr,1,12) AS pnr "
      "FROM rozysk "
      "WHERE time>=:first_time AND time<:last_time AND (airp_dep=:airp OR :airp IS NULL) "
      "ORDER BY time";
    Qry.CreateVariable("first_time",otDate,first_time);
    Qry.CreateVariable("last_time",otDate,last_time);
    Qry.CreateVariable("airp",otString,airp);
    Qry.Execute();

    if (tz_region!=NULL&&*tz_region!=0)
    {
      first_time = UTCToLocal(first_time, tz_region);
      last_time = UTCToLocal(last_time, tz_region);
    };

    f << DateTimeToStr(first_time,"ddmmyyhhnn") << '-'
      << DateTimeToStr(last_time-1.0/1440,"ddmmyyhhnn") << ENDL;
    for(;!Qry.Eof;Qry.Next())
    {
      string tz_region = AirpTZRegion(Qry.FieldAsString("airp_dep"));

      TDateTime time_local=UTCToLocal(Qry.FieldAsDateTime("time"),tz_region);
      TDateTime takeoff_local;
      if (!Qry.FieldIsNULL("term"))
        takeoff_local=UTCToLocal(Qry.FieldAsDateTime("takeoff"),tz_region);
      else
        takeoff_local=Qry.FieldAsDateTime("takeoff");

      f << DateTimeToStr(time_local,"dd.mm.yyyy") << '|'
        << DateTimeToStr(time_local,"hh:nn") << '|'
        << setw(3) << setfill('0') << Qry.FieldAsInteger("flt_no") << Qry.FieldAsString("suffix") << '|'
        << DateTimeToStr(takeoff_local,"dd.mm.yyyy") << '|'
        << Qry.FieldAsString("term") << '|'
        << Qry.FieldAsString("seat_no") << '|'
        << Qry.FieldAsString("surname") << '|'
        << Qry.FieldAsString("name") << '|'
        << Qry.FieldAsString("patronymic") << '|'
        << Qry.FieldAsString("doc_no") << '|'
        << Qry.FieldAsString("operation") << '|'
        << Qry.FieldAsString("tags") << '|'
        << Qry.FieldAsString("airline") << '|' << '|'
        << Qry.FieldAsString("airp_dep") << '|'
        << Qry.FieldAsString("airp_arv") << '|'
        << Qry.FieldAsString("bag_weight") << '|' << '|'
        << DateTimeToStr(takeoff_local,"hh:nn") << '|'
        << Qry.FieldAsString("pnr") << '|' << ENDL;
    };
    f.close();
  }
  catch(...)
  {
    try { f.close(); } catch( ... ) { };
    try
    {
      //в случае ошибки запишем пустой файл
      f.open(file_name);
      if (f.is_open()) f.close();
    }
    catch( ... ) { };
    throw;
  };
};

void sync_mvd(void)
{
  int Hour,Min,Sec;
  TDateTime now=NowUTC();
  DecodeTime(now,Hour,Min,Sec);
  if (Min%15!=0) return;

  TQuery Qry(&OraSession);
  Qry.SQLText =
    "UPDATE file_sets SET last_create=:now WHERE code=:code AND "
    "                 (airp=:airp OR airp IS NULL AND :airp IS NULL)";
  Qry.DeclareVariable("code",otString);
  Qry.DeclareVariable("airp",otString);
  Qry.DeclareVariable("now",otDate);

  TQuery FilesQry(&OraSession);
  FilesQry.Clear();
  FilesQry.SQLText=
    "SELECT name,dir,last_create,airp "
    "FROM file_sets "
    "WHERE code=:code AND pr_denial=0";
  FilesQry.DeclareVariable("code",otString);

  for(int pass=0;pass<=1;pass++)
  {
    modf(now,&now);
    if (pass==0)
    {
      TDateTime now_time;
      EncodeTime(Hour,Min,0,now_time);
      now+=now_time;
      FilesQry.SetVariable("code",ROZYSK_MAGISTRAL);
    }
    else
      FilesQry.SetVariable("code",ROZYSK_MAGISTRAL_24);

    Qry.SetVariable("code",FilesQry.GetVariableAsString("code"));
    Qry.SetVariable("now",now);

    FilesQry.Execute();
    for(;!FilesQry.Eof;FilesQry.Next())
    {
      if (!FilesQry.FieldIsNULL("last_create")&&
         (now-FilesQry.FieldAsDateTime("last_create"))<1.0/1440) continue;

      TDateTime local;
      string tz_region;
      if (!FilesQry.FieldIsNULL("airp"))
      {
        tz_region=AirpTZRegion(FilesQry.FieldAsString("airp"));
        local=UTCToLocal(now,tz_region);
      }
      else
      {
        tz_region.clear();
        local=now;
      };
      ostringstream file_name;
      file_name << FilesQry.FieldAsString("dir")
                << DateTimeToStr(local,FilesQry.FieldAsString("name"));

      if (FilesQry.FieldIsNULL("last_create"))
        create_mvd_file(now-1,now,
                        FilesQry.FieldAsString("airp"),
                        tz_region.c_str(),
                        file_name.str().c_str());
      else
        create_mvd_file(FilesQry.FieldAsDateTime("last_create"),now,
                        FilesQry.FieldAsString("airp"),
                        tz_region.c_str(),
                        file_name.str().c_str());

      Qry.SetVariable("airp",FilesQry.FieldAsString("airp"));
      Qry.Execute();
      OraSession.Commit();
    };
  };
};

