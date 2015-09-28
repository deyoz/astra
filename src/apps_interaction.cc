#include <sstream>
#include <deque>
#include <boost/algorithm/string.hpp>
#include "apps_interaction.h"
#include "exceptions.h"
#include "basic.h"
#include "alarms.h"
#include "astra_misc.h"
#include "qrys.h"
#include "tlg/tlg.h"
#include <boost/regex.hpp>
#include <ctime>

#define NICKNAME "ANNA"
#define NICKTRACE SYSTEM_TRACE
#include "serverlib/test.h"

using namespace std;
using namespace EXCEPTIONS;
using namespace BASIC;
using namespace ASTRA;

static const std::string ReqTypeCirq = "CIRQ";
static const std::string ReqTypeCicx = "CICX";
static const std::string ReqTypeCimr = "CIMR";

static const std::string AnsTypeCirs = "CIRS";
static const std::string AnsTypeCicc = "CICC";
static const std::string AnsTypeCima = "CIMA";

static const std::pair<std::string, int> FltTypeChk("CHK", 2);
static const std::pair<std::string, int> FltTypeInt("INT", 8);
static const std::pair<std::string, int> FltTypeExo("EXO", 4);
static const std::pair<std::string, int> FltTypeExd("EXD", 4);
static const std::pair<std::string, int> FltTypeInm("INM", 3);

static const std::pair<std::string, int> PaxReqPrq("PRQ", 22);
static const std::pair<std::string, int> PaxReqPcx("PCX", 20);

static const std::pair<std::string, int> PaxAnsPrs("PRS", 27);
static const std::pair<std::string, int> PaxAnsPcc("PCC", 26);

static const std::pair<std::string, int> MftReqMrq("MRQ", 3);
static const std::pair<std::string, int> MftAnsMak("MAK", 4);

static const std::string AnsErrCode = "ERR";

static const std::string APPSFormat = "APPS_FMT";

static const int BasicFormatVer = 21;

static const int MaxCirqPaxNum = 5;
static const int MaxCicxPaxNum = 10;

static const std::string APPSAddr = "APPSC";
static const std::string APPSUser = "ASTRA"; // !!! заполнить

enum { None, Origin, Dest, Both };

static std::string getAPPSUser()
{
  return APPSUser;
}

static std::string getIdent()
{
  TQuery Qry(&OraSession);
  Qry.SQLText = "SELECT apps_msg_id__seq.nextval vid FROM dual";
  Qry.Execute();
  return Qry.FieldAsString("vid");
}

static std::string getCountry( const std::string& airp)
{
  TAirpsRow &airpRow = (TAirpsRow&)base_tables.get("airps").get_row("code",airp);
  TCitiesRow &cityRow = (TCitiesRow&)base_tables.get("cities").get_row("code",airpRow.city);
  return ((TCountriesRow&)base_tables.get("countries").get_row("code",cityRow.country)).code_lat;
}

static bool isAPPSCountry (const std::string& country)
{
  // temporary hardcode
  return ( country == "AE" );
}

static bool checkConflictAlarm( const int pax_id, const std::string& cicx_msg_id, const int pax_seq_no )
{
  TReqPaxData actual;
  if ( actual.getByPaxId(pax_id) || actual.getByCrsPaxId(pax_id) ) {
    TReqPaxData reseived;
    reseived.fromDB( ReqTypeCirq, cicx_msg_id, pax_seq_no );
    return !reseived.equalAttrs( actual );
  }
  else
    throw Exception("Passenger was not found");
}

static std::string requestResStr()
{
  switch(std::time(0) % 10) {
    case 0:
      return string("/0000/T////////");
    case 1:
      return string("/0000/E//6033/INVALID PASSPORT NUMBER FORMAT/6062/TRAVEL DOCUMENT TYPE INVALID/6000/VALIDATION ERROR ? UNKNOWN FIELD/");
    case 2:
      return string("/8502/D////////");
    case 3:
      return string("/8507/U////////");
    case 4:
      return string("/8509/X////////");
    default:
      return (string("/8501/B/") + getIdent() + string("///////"));
  }
}

static std::string cancelResStr()
{
  switch(std::time(0) % 10) {
    case 0:
      return string("/0000/T///////");
    case 1:
      return string("/0000/E//6033/INVALID PASSPORT NUMBER FORMAT////");
    case 2:
      return string("/8506/N///////");
    default:
      return string("/8505/C///////");
  }
}

static void logErrorCondition( const int code, const std::string& text, LEvntPrms& params, const std::string& name )
{
  string id = string( "MSG.APPS_" ) + IntToString(code);
  if ( AstraLocale::getLocaleText( id, AstraLocale::LANG_RU ) != id &&
       AstraLocale::getLocaleText( id, AstraLocale::LANG_EN ) != id ) {
    params << PrmLexema( name, id );
  }
  else
    params << PrmSmpl<string>( name, text );
}

static void sendPax( TReqPaxData& new_pax, const int point_id, const int chkin_point_id, const bool is_cancel )
{
  TReqPaxData actual_pax;
  APPSAction action;
  bool is_exist = actual_pax.fromDB(new_pax.pax_id);
  if(!is_exist && is_cancel) {
    action = NoAction;
  }
  else if(!is_exist && !is_cancel) {
    action = NeedNew;
  }
  else
    action = new_pax.typeOfAction(actual_pax.status, new_pax.equalAttrs(actual_pax), is_cancel, true);

  if (action == NoAction)
    return;
  if ( action == NeedCancel || action == NeedUpdate ) {
    APPSRequest cancel_req(ReqTypeCicx, getAPPSUser(), point_id);
    cancel_req.int_flt.init( FltTypeInt, point_id );
    cancel_req.addPax(actual_pax);
    cancel_req.sendReq();
  }
  if ( action == NeedUpdate || action == NeedNew ) {
    APPSRequest add_req(ReqTypeCirq, getAPPSUser(), point_id);
    if ( chkin_point_id != ASTRA::NoExists )
      add_req.ckin_flt.init( FltTypeChk, chkin_point_id );
    add_req.int_flt.init( FltTypeInt, point_id );
    add_req.addPax(new_pax);
    add_req.sendReq();
  }
}

static void deleteAPPSData( const int pax_id )
{
  TQuery Qry(&OraSession);
  Qry.SQLText = "DELETE FROM apps_data WHERE pax_id = :pax_id";
  Qry.CreateVariable("pax_id", otString, pax_id);
  Qry.Execute();
}

static void deleteAPPSAlarms( const int pax_id )
{
  set_pax_alarm( pax_id, atAPPSNegativeDirective, false );
  set_crs_pax_alarm( pax_id, atAPPSNegativeDirective, false );
  set_pax_alarm( pax_id, atAPPSError, false );
  set_crs_pax_alarm( pax_id, atAPPSError, false );
  set_pax_alarm( pax_id, atAPPSConflict, false );
  set_crs_pax_alarm( pax_id, atAPPSConflict, false );
}

std::string getAPPSAddr()
{
  return APPSAddr;
}

void TSegment::init(const int dep, const int arv, const std::string& port_dep,
          const std::string& port_arv, const TTripInfo& info)
{
  point_dep = dep;
  point_arv = arv;
  airp_dep = port_dep;
  airp_arv = port_arv;
  flt = info;
  country_dep = getCountry(airp_dep);
  country_arv = getCountry(airp_arv);
}

TReqTrans::TReqTrans ( const std::string& trans_type, const std::string& id )
{
  code = trans_type;
  user_id = id;
  if (code == ReqTypeCirq)
    multi_resp = "N";
  ver = BasicFormatVer;
}

void TReqTrans::check_data() const
{
  if (code != ReqTypeCirq && code != ReqTypeCicx && code != ReqTypeCimr)
    throw Exception("Incorrect transacion code");
  if (user_id.empty() || user_id.size() > 6)
    throw Exception("Incorrect User ID");
}

std::string TReqTrans::msg( const std::string& msg_id) const
{
  check_data();
  std::ostringstream msg;
  msg << code << ':' << msg_id << '/' << user_id;
  if (code != ReqTypeCimr)
    msg << '/' << multi_resp << '/' << type;
  msg << '/' << ver;
  return msg.str();
}

TFlightData::TFlightData(const std::pair<std::string, int>& id, const std::string& flt,
                         const std::string& airp_dep, const TDateTime dep,
                         const std::string& airp_arv, const TDateTime arr)
{
  grp_id = id.first;
  flds_count = id.second;
  is_scheduled = "S";
  flt_num = flt;
  port = airp_dep;
  if (dep != NoExists) {
    date = DateTimeToStr( dep, "yyyymmdd" );
    time = DateTimeToStr( dep, "hhnnss" );
  }
  arv_port = airp_arv;
  if (dep != NoExists) {
    arr_date = DateTimeToStr( arr, "yyyymmdd" );
    arr_time = DateTimeToStr( arr, "hhnnss" );
  }
}

void TFlightData::init( const std::pair<std::string, int>& id, const int point_id )
{
  TAdvTripRoute route, tmp;
  route.GetRouteBefore(NoExists, point_id, trtWithCurrent, trtNotCancelled);
  tmp.GetRouteAfter(NoExists, point_id, trtNotCurrent, trtNotCancelled);
  route.insert(route.end(), tmp.begin(), tmp.end());

  grp_id = id.first;
  flds_count = id.second;
  is_scheduled = "S";

  TAirlinesRow &airline = (TAirlinesRow&)base_tables.get("airlines").get_row("code", route.front().airline);
  if (airline.code_lat.empty()) throw Exception("airline.code_lat empty (code=%s)",airline.code.c_str());
  string suffix;
  if (!route.front().suffix.empty()) {
    TTripSuffixesRow &suffixRow = (TTripSuffixesRow&)base_tables.get("trip_suffixes").get_row("code", route.front().suffix);
    if (suffixRow.code_lat.empty()) throw Exception("suffixRow.code_lat empty (code=%s)",suffixRow.code.c_str());
    suffix=suffixRow.code_lat;
  }
  ostringstream trip;
  trip << airline.code_lat
       << setw(3) << setfill('0') << route.front().flt_num
       << route.front().suffix;

  flt_num = trip.str();

  TAirpsRow &airp_dep = (TAirpsRow&)base_tables.get("airps").get_row("code", route.front().airp);
  if (airp_dep.code_lat.empty()) throw Exception("airp_dep.code_lat empty (code=%s)",airp_dep.code.c_str());
  port = airp_dep.code_lat;

  if (route.front().scd_out != NoExists) {
    date = DateTimeToStr( route.front().scd_out, "yyyymmdd" );
    time = DateTimeToStr( route.front().scd_out, "hhnnss" );
  }

  TAirpsRow &airp_arv = (TAirpsRow&)base_tables.get("airps").get_row("code", route.back().airp);
  if (airp_arv.code_lat.empty()) throw Exception("airp_arv.code_lat empty (code=%s)",airp_arv.code.c_str());
  arv_port = airp_arv.code_lat;

  if (route.back().scd_in != NoExists) {
    arr_date = DateTimeToStr( route.back().scd_in, "yyyymmdd" );
    arr_time = DateTimeToStr( route.back().scd_in, "hhnnss" );
  }
}

void TFlightData::init(const TFlightData& data)
{
  grp_id = data.grp_id;
  flds_count = data.flds_count;
  is_scheduled = data.is_scheduled;
  flt_num = data.flt_num;
  port = data.port;
  date = data.date;
  time = data.time;
  arv_port = data.arv_port;
  arr_date = data.arr_date;
  arr_time = data.arr_time;
}

void TFlightData::check_data() const
{
  if ( !( ( grp_id == FltTypeChk.first && flds_count == FltTypeChk.second ) || ( grp_id == FltTypeInt.first && flds_count == FltTypeInt.second ) ||
          ( grp_id == FltTypeExo.first && flds_count == FltTypeExo.second ) || ( grp_id == FltTypeExd.first && flds_count == FltTypeExd.second ) ||
          ( grp_id == FltTypeInm.first && flds_count == FltTypeInm.second ) ) )
    throw Exception( "Incorrect group identifier of flight or flight fields count" );
  if ( flt_num.empty() || flt_num.size() > 8 )
    throw Exception(" Incorrect flt_num" );
  if ( port.empty() || port.size() > 5 )
    throw Exception( "Incorrect airport" );
  if ( ( grp_id == FltTypeInt.first && arv_port.empty() ) || arv_port.size() > 5 )
    throw Exception( "Incorrect arrival airport" );
  if ( ( grp_id != FltTypeChk.first && date.empty() ) || date.size() > 8 )
    throw Exception( "Incorrect date format" );
  if ( grp_id != FltTypeChk.first && grp_id != FltTypeInm.first && ( time.empty() || time.size() > 6 ))
    throw Exception( "Incorrect time format" );
  if ( ( grp_id == FltTypeInt.first && arr_date.empty() ) || arr_date.size() > 8 )
    throw Exception( "Incorrect arrival date format" );
  if ( ( grp_id == FltTypeInt.first && arr_time.empty() ) || arr_time.size() > 6 )
    throw Exception( "Incorrect arrival time format" );
}

std::string TFlightData::msg() const
{
  check_data();
  std::ostringstream msg;
  msg << grp_id << '/' << flds_count;
  if (grp_id == FltTypeInt.first)
    msg  << '/' << is_scheduled;
  if (grp_id == FltTypeInt.first || grp_id == FltTypeInm.first)
    msg << '/' << flt_num << '/' << port;
  else
    msg << '/' << port << '/' << flt_num;
  if (grp_id == FltTypeInt.first)
    msg  << '/' << arv_port;
  if (grp_id != FltTypeChk.first)
    msg  << '/' << date;
  if (grp_id != FltTypeChk.first && grp_id != FltTypeInm.first)
    msg  << '/' << time;
  if (grp_id == FltTypeInt.first) {
    msg << '/' << arr_date << '/' << arr_time;
  }
  return msg.str();
}

void TReqPaxData::init( const bool is_crew, const TPaxData& pax_data,
                          const int transfer, const string& override )
{
  pax_id = pax_data.pax_id;
  flds_count = ASTRA::NoExists;
  apps_pax_id = ASTRA::NoExists;
  pax_crew = is_crew?"C":"P";
  nationality = pax_data.doc.nationality;
  issuing_state = pax_data.doc.issue_country;
  passport = pax_data.doc.no.substr(0, 14);
  doc_type =  (pax_data.doc.type == "P" || pax_data.doc.type.empty())?"P":"O";
  if ( pax_data.doc.expiry_date != ASTRA::NoExists )
    expiry_date = DateTimeToStr( pax_data.doc.expiry_date, "yyyymmdd" );
  if( !family_name.empty() ) {
    family_name = transliter(pax_data.doc.surname, 1, 1);
    if (!pax_data.doc.first_name.empty())
      given_names = pax_data.doc.first_name;
    if (!given_names.empty() && !pax_data.doc.second_name.empty())
      given_names = given_names + " " + pax_data.doc.second_name;
    given_names = given_names.substr(0, 24);
    given_names = transliter(given_names, 1, 1);
  }
  else {
    family_name = pax_data.surname;
    family_name = transliter(family_name, 1, 1);
    if(!pax_data.name.empty())
      given_names = (transliter(pax_data.name, 1, 1));
  }
  if ( pax_data.doc.birth_date != ASTRA::NoExists )
    birth_date = DateTimeToStr( pax_data.doc.birth_date, "yyyymmdd" );
  sex = (pax_data.doc.gender == "M" || pax_data.doc.gender == "F")?pax_data.doc.gender:"U";
  trfer_at_origin = (transfer == Origin || transfer == Both)?"Y":"N";
  trfer_at_dest = (transfer == Dest || transfer == Both)?"Y":"N";
  if(!override.empty())
    override_codes = override;
}

void TReqPaxData::init( TQuery &Qry )
{
  pax_id = Qry.FieldAsInteger("pax_id");
  grp_id = PaxReqPrq.first;
  flds_count = PaxReqPrq.second;
  if (!Qry.FieldIsNULL("apps_pax_id"))
    apps_pax_id = Qry.FieldAsInteger("apps_pax_id");
  pax_crew = Qry.FieldAsString("pax_crew");
  nationality = Qry.FieldAsString("nationality");
  issuing_state = Qry.FieldAsString("issuing_state");
  passport = Qry.FieldAsString("passport");
  if (!Qry.FieldIsNULL("check_char"))
    check_char = Qry.FieldAsString("check_char");
  doc_type = Qry.FieldAsString("doc_type");
  expiry_date = Qry.FieldAsString("expiry_date");
  if (!Qry.FieldIsNULL("sup_doc_type"))
    sup_doc_type = Qry.FieldAsString("sup_doc_type");
  if (!Qry.FieldIsNULL("sup_passport"))
    sup_passport = Qry.FieldAsString("sup_passport");
  if (!Qry.FieldIsNULL("sup_check_char"))
    sup_check_char = Qry.FieldAsString("sup_check_char");
  family_name = Qry.FieldAsString("family_name");
  if (!Qry.FieldIsNULL("given_names"))
    given_names = Qry.FieldAsString("given_names");
  birth_date = Qry.FieldAsString("date_of_birth");
  sex = Qry.FieldAsString("sex");
  if (!Qry.FieldIsNULL("birth_country"))
    birth_country = Qry.FieldAsString("birth_country");
  if (!Qry.FieldIsNULL("is_endorsee"))
    endorsee = Qry.FieldAsString("is_endorsee");
  if (!Qry.FieldIsNULL("transfer_at_orgn"))
    trfer_at_origin = Qry.FieldAsString("transfer_at_orgn");
  if (!Qry.FieldIsNULL("transfer_at_dest"))
    trfer_at_dest = Qry.FieldAsString("transfer_at_dest");
  if (!Qry.FieldIsNULL("pnr_source"))
    pnr_source = Qry.FieldAsString("pnr_source");
  if (!Qry.FieldIsNULL("pnr_locator"))
    pnr_locator = Qry.FieldAsString("pnr_locator");
  if (!Qry.FieldIsNULL("status"))
    status = Qry.FieldAsString("status");
}

bool TReqPaxData::fromDB( const int pax_ident )
{
  // попытаемся найти пассажира среди отправленных
  TQuery Qry( &OraSession );
  Qry.SQLText = "SELECT * FROM "
                "(SELECT pax_id, apps_pax_id, status, pax_crew, "
                "nationality, issuing_state, passport, check_char, doc_type, expiry_date, "
                "sup_check_char, sup_doc_type, sup_passport, family_name, given_names, "
                "date_of_birth, sex, birth_country, is_endorsee, transfer_at_orgn, "
                "transfer_at_dest, pnr_source, pnr_locator, send_time "
                "FROM apps_data "
                "WHERE pax_id = :pax_id "
                "ORDER BY send_time DESC) "
                "WHERE rownum = 1";
  Qry.CreateVariable( "pax_id", otInteger, pax_ident );
  Qry.Execute();

  if(Qry.Eof)
    return false;

  init(Qry);
  return true;
}

bool TReqPaxData::fromDB( const std::string& trans_code, const std::string& msg_id, const int seq_no )
{
  // попытаемся найти пассажира среди отправленных
  TQuery Qry( &OraSession );
  ostringstream sql;
  sql << "SELECT pax_id, apps_pax_id, status, pax_crew, "
         "nationality, issuing_state, passport, check_char, doc_type, expiry_date, "
         "sup_check_char, sup_doc_type, sup_passport, family_name, given_names, "
         "date_of_birth, sex, birth_country, is_endorsee, transfer_at_orgn, "
         "transfer_at_dest, pnr_source, pnr_locator, send_time "
         "FROM apps_data ";
  if ( trans_code == ReqTypeCirq )
    sql << "WHERE cirq_msg_id = :msg_id AND cirq_seq_no = :pax_seq_no";
  else if ( trans_code == ReqTypeCicx )
    sql << "WHERE cicx_msg_id = :msg_id AND cicx_seq_no = :pax_seq_no";
  else
    throw EXCEPTIONS::Exception( std::string( "TReqPaxData::fromDB: unknown transaction code: " + trans_code ) );

  Qry.SQLText = sql.str();
  Qry.CreateVariable( "msg_id", otString, msg_id );
  Qry.CreateVariable( "pax_seq_no", otInteger, seq_no );
  Qry.Execute();

  if(Qry.Eof)
    return false;

  init(Qry);
  return true;
}

bool TReqPaxData::getByPaxId( const int pax_ident )
{
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText="SELECT refuse, surname, name, pax.grp_id, status, airp_dep, airp_arv "
                 "FROM pax_grp, pax "
                 "WHERE pax_id = :pax_id AND pax_grp.grp_id = pax.grp_id";
  Qry.CreateVariable("pax_id", otInteger, pax_ident);
  Qry.Execute();

  if(Qry.Eof)
    return false;

  int transfer = None;
  TCkinRouteItem prior, next;
  int grp_id = Qry.FieldAsInteger("grp_id");
  // проверим входящий трансфер
  if ( TCkinRoute().GetPriorSeg(grp_id, crtIgnoreDependent, prior) && !prior.airp_dep.empty()) {
    string country_dep = getCountry(Qry.FieldAsString("airp_dep"));
    if ( isAPPSCountry( country_dep ) &&
       ( getCountry(prior.airp_dep) != country_dep ) )
      transfer = Origin;
  }
  // проверим исходящий трансфер
  if ( TCkinRoute().GetNextSeg(grp_id, crtIgnoreDependent, next ) && !next.airp_arv.empty() ) {
    string country_arv = getCountry(Qry.FieldAsString("airp_arv"));
    if ( isAPPSCountry( country_arv ) &&
       ( getCountry( next.airp_arv ) != country_arv ) )
      transfer = ( ( transfer == Origin ) ? Both : Dest );
  }
  TPaxData data;
  CheckIn::LoadPaxDoc(pax_ident, data.doc);
  data.pax_id = pax_ident;
  data.is_cancel = !Qry.FieldIsNULL("refuse");
  data.surname = Qry.FieldAsString("surname");
  if(!Qry.FieldIsNULL("name"))
    data.name = Qry.FieldAsString("name");
  TPaxStatus pax_status = DecodePaxStatus(Qry.FieldAsString("status"));
  init( (pax_status==psCrew), data, transfer );
  return true;
}

bool TReqPaxData::getByCrsPaxId( const int pax_ident )
{
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText="SELECT pr_del, surname, name, point_id_spp, airp_arv, crs_pax.pnr_id "
              "FROM crs_pax, crs_pnr, tlg_binding "
              "WHERE pax_id = :pax_id AND crs_pax.pnr_id = crs_pnr.pnr_id AND "
              "      crs_pnr.point_id = tlg_binding.point_id_tlg";
  Qry.CreateVariable("pax_id", otInteger, pax_ident);
  Qry.Execute();

  if (Qry.Eof)
    return false;

  int pnr_id = Qry.FieldAsInteger("pnr_id");
  string airp_arv = Qry.FieldAsString("airp_arv");

  TCachedQuery FltQry("SELECT airline, flt_no, suffix, airp, scd_out FROM points WHERE point_id=:point_id AND pr_del>=0",
                      QParams() << QParam("point_id", otInteger, Qry.FieldAsInteger("point_id_spp")));
  FltQry.get().Execute();

  if (FltQry.get().Eof)
    return false;

  TTripInfo operFlt(FltQry.get());
  TQuery TrferQry(&OraSession);
  int transfer = None;
  map<int, CheckIn::TTransferItem> trfer;
  CheckInInterface::GetOnwardCrsTransfer(pnr_id, TrferQry, operFlt, airp_arv, trfer);
  if (!trfer[1].airp_arv.empty()) {
    // сквозная регистрация
    string country_arv = getCountry(airp_arv);
    if ( isAPPSCountry( country_arv ) &&
       ( getCountry( trfer[1].airp_arv ) != country_arv ) )
      trfer_at_dest = Dest; // исходящий трансфер
  }
  TPaxData data;
  CheckIn::LoadCrsPaxDoc(pax_ident, data.doc);
  data.pax_id = pax_ident;
  data.is_cancel = Qry.FieldAsInteger("pr_del");
  data.surname = Qry.FieldAsString("surname");
  if(!Qry.FieldIsNULL("name"))
    data.name = Qry.FieldAsString("name");
  init( false, data, transfer );
  return true;
}

APPSAction TReqPaxData::typeOfAction(const std::string& status, const bool is_the_same,
                                     const bool is_cancel, const bool is_forced) const
{
  if ( !status.empty() && is_cancel ) {
    deleteAPPSAlarms( pax_id );
    if ( status != "B" ) {
      deleteAPPSData( pax_id );
      return NoAction;
    }
    else
      return NeedCancel;
  }
  if ( !status.empty() && !is_the_same ) {
    if(status == "P")
      return NeedNew;
    else if( status == "X" )
      return NoAction;
    else if( status == "B" )
      return NeedUpdate;
    else
      throw Exception("Unknown status");
  }
  else if ( !status.empty() && is_the_same ) {
    if ( status == "P" && is_forced )
      return NeedNew;
    else
      return NoAction;
  }
  else if ( status.empty() && is_the_same ) {
    return NoAction;
  }
  else {
    // рассинхронизация
    set_pax_alarm( pax_id, atAPPSConflict, true );
    set_crs_pax_alarm( pax_id, atAPPSConflict, true );
    return NoAction;
  }
}

void TReqPaxData::check_data() const
{
  if ( pax_id == ASTRA::NoExists )
    throw Exception( "Empty pax_id" );
  if ( grp_id != PaxReqPrq.first && grp_id != PaxReqPcx.first )
    throw Exception("Incorrect grp_id %s", grp_id.c_str() );
  if ( flds_count != PaxReqPrq.second && flds_count != PaxReqPcx.second )
    throw Exception( "Incorrect flds_count %d", flds_count );
  if ( grp_id == PaxReqPcx.first && apps_pax_id == ASTRA::NoExists )
    throw Exception( "Empty apps_pax_id" );
  if ( pax_crew != "C" && pax_crew != "P" )
    throw Exception( "Incorrect pax_crew %s", pax_crew.c_str() );
/*  if ( nationality.empty() )
    throw Exception( "Empty nationality" );
  if ( grp_id == PaxReqPrq.first && issuing_state.empty() )
    throw Exception( "Empty issuing_state" ); */
  if( passport.size() > 14)
    throw Exception( "Empty passport" );
  if( !doc_type.empty() && doc_type != "P" && doc_type != "O" && doc_type != "N" )
    throw Exception( "Incorrect doc_type: %s", doc_type.c_str() );
/*  if( expiry_date.empty() || expiry_date == "00000000" )
    throw Exception( "Empty expiry_date" ); */
  if( family_name.empty() || family_name.size() < 2 || family_name.size() > 24 )
    throw Exception( "Incorrect family_name: %s", family_name.c_str() );
  if( given_names.size() > 24 )
    throw Exception( "given_names too long: %s", given_names.c_str() );
/*  if( birth_date.empty() )
    throw Exception( "Empty birth_date" ); */
  if( !sex.empty() && sex != "M" && sex != "F" && sex != "U" && sex != "X" )
    throw Exception( "Incorrect sex: %s", sex.c_str() );
  if( !endorsee.empty() && endorsee != "S" )
    throw Exception( "Incorrect endorsee: %s", endorsee.c_str() );
}

std::string TReqPaxData::msg( const int seq_num ) const
{
  check_data();
  std::ostringstream msg;
  msg << grp_id << '/' << flds_count << '/' << seq_num;
  if (grp_id == PaxReqPcx.first)
    msg  << '/' << apps_pax_id;
  msg << '/' << pax_crew << '/' << nationality << '/' << issuing_state << '/' << passport << '/'
      << check_char << '/' << doc_type << '/' << expiry_date << '/' << sup_doc_type << '/'
      << sup_passport << '/' << sup_check_char << '/' << family_name << '/'
      << (given_names.empty()?"-":given_names) << '/' << birth_date << '/' << sex << '/'
      << birth_country << '/' << endorsee << '/' << trfer_at_origin << '/' << trfer_at_dest;
  if (grp_id == PaxReqPrq.first)
    msg  << '/' << override_codes << '/' << pnr_source << '/' << pnr_locator;
  return msg.str();
}

void TReqPaxData::savePaxData( const std::string& code, const std::string& msg_id, const int seq_num ) const
{
  TQuery Qry(&OraSession);

  if( code == ReqTypeCicx ) {
    Qry.SQLText = "UPDATE apps_data SET cicx_msg_id = :cicx_msg_id, cicx_seq_no = :cicx_seq_no "
                  "WHERE apps_pax_id = :apps_pax_id";
    Qry.CreateVariable("cicx_msg_id", otString, msg_id);
    Qry.CreateVariable("cicx_seq_no", otInteger, seq_num);
    Qry.CreateVariable("apps_pax_id", otInteger, apps_pax_id);
    Qry.Execute();

    return;
  }

  Qry.SQLText = "INSERT INTO apps_data (pax_id, cirq_msg_id, cirq_seq_no, pax_crew, nationality, "
                "                       issuing_state, passport, check_char, doc_type, expiry_date, "
                "                       sup_check_char, sup_doc_type, sup_passport, family_name, given_names, "
                "                       date_of_birth, sex, birth_country, is_endorsee, transfer_at_orgn, "
                "                       transfer_at_dest, pnr_source, pnr_locator, send_time) "
                "VALUES (:pax_id, :cirq_msg_id, :cirq_seq_no, :pax_crew, :nationality, :issuing_state, "
                "        :passport, :check_char, :doc_type, :expiry_date, :sup_check_char, :sup_doc_type, "
                "        :sup_passport, :family_name, :given_names, :date_of_birth, :sex, :birth_country, "
                "        :is_endorsee, :transfer_at_orgn, :transfer_at_dest, :pnr_source, :pnr_locator, :send_time)";

  Qry.CreateVariable("cirq_msg_id", otString, msg_id);
  Qry.CreateVariable("pax_id", otInteger, pax_id);
  Qry.CreateVariable("cirq_seq_no", otInteger, seq_num);
  Qry.CreateVariable("pax_crew", otString, pax_crew);
  Qry.CreateVariable("nationality", otString, nationality);
  Qry.CreateVariable("issuing_state", otString, issuing_state);
  Qry.CreateVariable("passport", otString, passport);
  Qry.CreateVariable("check_char", otString, check_char);
  Qry.CreateVariable("doc_type", otString, doc_type);
  Qry.CreateVariable("expiry_date", otString, expiry_date);
  Qry.CreateVariable("sup_check_char", otString, sup_check_char);
  Qry.CreateVariable("sup_doc_type", otString, sup_doc_type);
  Qry.CreateVariable("sup_passport", otString, sup_passport);
  Qry.CreateVariable("family_name", otString, family_name);
  Qry.CreateVariable("given_names", otString, given_names);
  Qry.CreateVariable("date_of_birth", otString, birth_date);
  Qry.CreateVariable("sex", otString, sex);
  Qry.CreateVariable("birth_country", otString, birth_country);
  Qry.CreateVariable("is_endorsee", otString, endorsee);
  Qry.CreateVariable("transfer_at_orgn", otString, trfer_at_origin);
  Qry.CreateVariable("transfer_at_dest", otString, trfer_at_dest);
  Qry.CreateVariable("pnr_source", otString, pnr_source);
  Qry.CreateVariable("pnr_locator", otString, pnr_locator);
  Qry.CreateVariable("send_time", otDate, BASIC::NowUTC());
  Qry.Execute();
}

void TReqMft::init(const std::string& state, const std::string& pax_req, const std::string& crew_req)
{
  grp_id = MftReqMrq.first;
  flds_count = MftReqMrq.second;
  country = state;
  mft_pax = pax_req;
  mft_crew = crew_req;
}

void TReqMft::check_data() const
{
  if (grp_id != MftReqMrq.first)
    throw Exception("Incorrect grp_id");
  if (flds_count != MftReqMrq.second)
    throw Exception("Incorrect flds_count");
  if (country.empty())
    throw Exception("Empty country");
  if (mft_pax != "C")
    throw Exception("Incorrect mft_pax");
  if (mft_crew != "C")
    throw Exception("Incorrect mft_crew");
}

std::string TReqMft::msg() const
{
  check_data();
  std::ostringstream msg;
  msg << grp_id << '/' << flds_count << '/' << country << '/' << mft_pax << '/' << mft_crew;
  return msg.str();
}

void APPSRequest::sendReq() const
{
  std::ostringstream msg;
  string msg_id;

  if ( trans.code == ReqTypeCimr ) {

    msg_id = getIdent();
    msg << trans.msg(msg_id) << '/';
    msg << inm_flt.msg() << '/' << mft_req.msg() << '/';

    sendMsg( msg.str(), msg_id );

    return;
  }

  if (passengers.empty()) {
    return;
  }

  // заполним информацию о перелете
  std::ostringstream flt_info;
  if (trans.code == ReqTypeCirq && !ckin_flt.empty())
    flt_info << ckin_flt.msg() << '/';
  if (int_flt.empty())
    throw Exception("Internation flight is empty!");
  flt_info << int_flt.msg() << '/';
  if (trans.code == ReqTypeCirq && !exp_flt.empty()) {
    for (std::vector<TFlightData>::const_iterator it = exp_flt.begin(); it != exp_flt.end(); it++) {
      flt_info << it->msg() << '/';
    }
  }

  int seq_num = 0;
  for (std::vector<TReqPaxData>::const_iterator it = passengers.begin(); it != passengers.end(); it++) {
    if (seq_num == 0) {
      msg_id = getIdent();
      msg.str("");
      msg << trans.msg( msg_id ) << '/' << flt_info.str();
    }
    msg << it->msg( ++seq_num ) << '/';
    it->savePaxData(trans.code, msg_id, seq_num);
    if ( seq_num == ( (trans.code== ReqTypeCirq)?MaxCirqPaxNum:MaxCicxPaxNum ) ) {
      sendMsg( msg.str(), msg_id );
      seq_num = 0;
    }
  }
  if (seq_num != 0)
    sendMsg( msg.str(), msg_id );
}

void APPSRequest::sendMsg( const std::string& text, const std::string& msg_id ) const
{
  // отправим телеграмму
  sendTlg( getAPPSAddr().c_str(), OWN_CANON_NAME(), qpOutApp, 20, text,
          ASTRA::NoExists, ASTRA::NoExists );

  sendCmd("CMD_APPS_ANSWER_EMUL","H");
  sendCmd("CMD_APPS_HANDLER","H");

  ProgTrace(TRACE5, "New APPS request generated: %s", text.c_str());

  TQuery Qry(&OraSession);
  Qry.SQLText = "INSERT INTO apps_messages(msg_id, msg_text, send_attempts, send_time, point_id) "
                "VALUES (:msg_id, :msg_text, :send_attempts, :send_time, :point_id)";
  Qry.CreateVariable("msg_id", otString, msg_id);
  Qry.CreateVariable("send_time", otDate, BASIC::NowUTC());
  Qry.CreateVariable("msg_text", otString, text);
  Qry.CreateVariable("send_attempts", otInteger, 1);
  Qry.CreateVariable("point_id", otInteger, point_id);
  Qry.Execute();
}

void APPSRequest::addPax(const TReqPaxData& pax_data)
{
  passengers.push_back(pax_data);
  if (trans.code == ReqTypeCirq) {
    passengers.back().grp_id = PaxReqPrq.first;
    passengers.back().flds_count = PaxReqPrq.second;
  }
  else if (trans.code == ReqTypeCicx) {
    passengers.back().grp_id = PaxReqPcx.first;
    passengers.back().flds_count = PaxReqPcx.second;
  }
  else
    throw Exception("Incorrect format of APPS Request");
}

std::string TAnsTrans::toString() const
{
  std::string res = "code: " + code + "\n" + "msg_ident: " + msg_ident + "\n";
  return res;
}

void TAnsPaxData::getPaxId( const TAnsTrans& trans )
{
  ostringstream sql;
  sql << "SELECT pax_id FROM apps_data ";
  if ( trans.code == AnsTypeCirs )
    sql << "WHERE cirq_msg_id = :msg_id AND cirq_seq_no = :pax_seq_no";
  else
    sql << "WHERE cicx_msg_id = :msg_id AND cicx_seq_no = :pax_seq_no";
  TQuery Qry(&OraSession);
  Qry.SQLText = sql.str();
  Qry.CreateVariable("msg_id", otString, trans.msg_ident);
  Qry.CreateVariable("pax_seq_no", otInteger, seq_num);
  Qry.Execute();

  if(Qry.Eof)
    throw Exception("Passenger does not exist in apps_data");
  pax_id = Qry.FieldAsInteger("pax_id");
}

std::string TAnsPaxData::toString() const
{
  std::ostringstream res;
  res << "pax_id: " << pax_id  << std::endl
      << "grp_id: " << grp_id << std::endl
      << "flds_count: " << flds_count << std::endl
      << "seq_num: " << seq_num << std::endl
      << "country: " << country << std::endl
      << "pax_crew: " << pax_crew << std::endl
      << "nationality: " << nationality << std::endl;
  if(!issuing_state.empty()) res << "issuing_state: " << issuing_state << std::endl;
  if(!passport.empty()) res << "passport: " << passport << std::endl;
  if(!check_char.empty()) res << "check_char: " << check_char << std::endl;
  if(!doc_type.empty()) res << "doc_type: " << doc_type << std::endl;
  if(!expiry_date.empty()) res << "expiry_date: " << expiry_date << std::endl;
  if(!sup_doc_type.empty()) res << "sup_doc_type: " << sup_doc_type << std::endl;
  if(!sup_passport.empty()) res << "sup_passport: " << sup_passport << std::endl;
  if(!sup_check_char.empty()) res << "sup_check_char: " << sup_check_char << std::endl;
  if(!family_name.empty()) res << "family_name: " << family_name << std::endl;
  if(!given_names.empty()) res << "given_names: " << given_names << std::endl;
  if(!birth_date.empty()) res << "birth_date: " << birth_date << std::endl;
  if(!sex.empty()) res << "sex: " << sex << std::endl;
  if(!birth_country.empty()) res << "birth_country: " << birth_country << std::endl;
  if(!endorsee.empty()) res << "endorsee: " << endorsee << std::endl;
  res << "code: " << code << std::endl << "status: " << status << std::endl;
  if(apps_pax_id != ASTRA::NoExists) res << "apps_pax_id: " << apps_pax_id << std::endl;
  if(error_code1 != ASTRA::NoExists) res << "error_code1: " << error_code1 << std::endl;
  if(!error_text1.empty()) res << "error_text1: " << error_text1 << std::endl;
  if(error_code2 != ASTRA::NoExists) res << "error_code2: " << error_code2 << std::endl;
  if(!error_text2.empty()) res << "error_text2: " << error_text2 << std::endl;
  if(error_code3 != ASTRA::NoExists) res << "error_code3: " << error_code3 << std::endl;
  if(!error_text3.empty()) res << "error_text3: " << error_text3 << std::endl;
  return res.str();
}

void TAnsPaxData::logAPPSPaxStatus( const std::string& trans_code, const int point_id ) const
{
  string lexema_id;
  if (status == "B")
    lexema_id = "MSG.PASSENGER.APPS_STATUS_B";
  else if (status == "D")
    lexema_id = "MSG.PASSENGER.APPS_STATUS_D";
  else if (status == "X")
    lexema_id = "MSG.PASSENGER.APPS_STATUS_X";
  else if (status == "U")
    lexema_id = "MSG.PASSENGER.APPS_STATUS_U";
  else if (status == "I")
    lexema_id = "MSG.PASSENGER.APPS_STATUS_I";
  else if (status == "T")
    lexema_id = "MSG.PASSENGER.APPS_STATUS_T";
  else if (status == "E")
    lexema_id = "MSG.PASSENGER.APPS_STATUS_E";
  else if (status == "C")
    lexema_id = "MSG.PASSENGER.APPS_STATUS_C";
  else if (status == "N")
    lexema_id = "MSG.PASSENGER.APPS_STATUS_N";

  LEvntPrms params;
  PrmLexema lexema("result", lexema_id);
  if (status == "E") {
    if ( error_code1 == ASTRA::NoExists || error_text1.empty() )
      throw Exception("logAPPSPaxStatus: error_code1 or error_text1 is empty");
    PrmEnum errors("condition", ". ");
    logErrorCondition(error_code1, error_text1, errors.prms, "condition1");
    if ( error_code1 != ASTRA::NoExists )
      logErrorCondition(error_code2, error_text2, errors.prms, "condition2");
    if ( error_code1 != ASTRA::NoExists )
      logErrorCondition(error_code3, error_text3, errors.prms, "condition3");
    lexema.prms << errors;
  }
  params << lexema;

  if ( !country.empty() ) {
    TElemFmt fmt;
    string apps_country = ElemToElemId(etCountry,country,fmt);
    if ( fmt == efmtUnknown )
      throw EConvertError("Unknown format");
    PrmLexema lexema("country", "MSG.APPS_COUNTRY");
    lexema.prms << PrmElem<string>("country", etCountry, apps_country);
    params << lexema;
  }
  else
    params << PrmSmpl<string>("country", "");

  params << PrmSmpl<string>("passengers", family_name);
  TReqInfo::Instance()->LocaleToLog( ((trans_code == AnsTypeCirs)?"MSG.APPS_BRD_RESP":"MSG.APPS_CNL_RESP"),
                                     params, evtPax, point_id );
}

std::string TAnsError::toString() const
{
  if(grp_id.empty())
    return std::string("");
  std::ostringstream res;
  res << "flds_count: " << flds_count << std::endl;
  for (std::vector<TError>::const_iterator it = errors.begin(); it != errors.end(); it++)
    res << "country: " << it->country << std::endl << "error_code: " << it->error_code << std::endl <<
           "error_text: " << it->error_text << std::endl;
  return res.str();
}

std::string TAnsMft::toString() const
{
  if(grp_id.empty())
    return std::string("");
  std::ostringstream res;
  res << "flds_count: " << flds_count << std::endl << "country: " << country << std::endl <<
         "resp_code: " << resp_code << std::endl;
  if (error_code != ASTRA::NoExists)
    res << "error_code: " << error_code << std::endl << "error_text: " << error_text << std::endl;
  return res.str();
}

bool APPSAnswer::init(const std::string& source)
{
  if(source.empty())
    throw Exception("Answer is empty");

  std::vector<std::string> temp;
  trans.code = source.substr(0, 4);
  string answer = source.substr(5, source.size() - 6); // отрезаем код транзакции и замыкающий '/' (XXXX:text_to_parse/)
  boost::split(temp, answer, boost::is_any_of("/"));

  std::vector<std::string>::iterator it = temp.begin();
  if (trans.code != AnsTypeCirs && trans.code != AnsTypeCicc && trans.code != AnsTypeCima)
    throw Exception( std::string( "Unknown transaction code: " + trans.code ) );
  trans.msg_ident = *(it++);

  if(!getMsgInfo(trans.msg_ident))
    return false;

  while(it < temp.end() && *it != AnsErrCode) {
    if ((trans.code == AnsTypeCirs && *it == PaxAnsPrs.first)
        || (trans.code == AnsTypeCicc && *it == PaxAnsPcc.first)) {
      TAnsPaxData pax;
      pax.grp_id = *(it++);
      pax.flds_count = ToInt(*(it++));
      pax.seq_num = ToInt(*(it++));
      pax.country = *(it++);
      pax.pax_crew = (*(it++)).front();
      pax.nationality = *(it++);
      pax.issuing_state = *(it++);
      pax.passport = *(it++);
      pax.check_char = (*(it++)).front();
      pax.doc_type = (*(it++)).front();
      pax.expiry_date = *(it++);
      pax.sup_doc_type = (*(it++));
      pax.sup_passport = *(it++);
      pax.sup_check_char = (*(it++)).front();
      pax.family_name = *(it++);
      if (*it != "-")
        pax.given_names = *(it++);
      else it++;
      pax.birth_date = *(it++);
      pax.sex = (*(it++)).front();
      pax.birth_country = *(it++);
      pax.endorsee = (*(it++)).front();
      pax.code = ToInt(*(it++));
      pax.status = (*(it++)).front();
      if (trans.code == AnsTypeCirs)
        pax.apps_pax_id = ToInt(*(it++));
      pax.error_code1 = ToInt(*(it++));
      pax.error_text1 = *(it++);
      pax.error_code2 = ToInt(*(it++));
      pax.error_text2= *(it++);
      pax.error_code3 = ToInt(*(it++));
      pax.error_text3 = *(it++);
      pax.getPaxId(trans);
      passengers.push_back(pax);
    }
    else if (trans.code == AnsTypeCima && *it == MftAnsMak.first) {
      mft_grp.grp_id = *(it++);
      mft_grp.flds_count = ToInt(*(it++));
      mft_grp.country = *(it++);
      mft_grp.resp_code = ToInt(*(it++));
      mft_grp.error_code = ToInt(*(it++));
      mft_grp.error_text = *(it++);
    }
    else
      throw Exception( std::string( "Unknown trans.code: " ) + trans.code + " or grp_id" + *it );
  }

  if(it >= temp.end()) return true;

  err_grp.grp_id = *(it++);
  err_grp.flds_count = ToInt(*(it++));
  while(it < temp.end()) {
    TError error;
    error.country = *(it++);
    error.error_code = ToInt(*(it++));
    error.error_text = *(it++);
    err_grp.errors.push_back(error);
  }

  if ( trans.code == AnsTypeCima )
    return true;

  // найдем всех пассажиров из сообщения
  ostringstream sql;
  sql << "SELECT pax_id, family_name FROM apps_data ";
  if ( trans.code == AnsTypeCirs )
    sql << "WHERE cirq_msg_id = :msg_id ";
  else
    sql << "WHERE cicx_msg_id = :msg_id ";
  TQuery Qry(&OraSession);
  Qry.SQLText = sql.str();

  Qry.CreateVariable("msg_id", otString, trans.msg_ident);
  Qry.Execute();

  if(Qry.Eof)
    throw Exception("Passengers were not found");

  while(!Qry.Eof) {
      TAnsPaxData pax;
      pax.family_name = Qry.FieldAsString("family_name");
      pax.pax_id = Qry.FieldAsInteger("pax_id");
      passengers.push_back(pax);
      Qry.Next();
  }
  return true;
}

void APPSAnswer::processReply() const
{
  // выключим тревогу "Нет связи с APPS"
  set_alarm( point_id, atAPPSOutage, false );

  // Сообщение не обработано системой. Разберем ошибки.
  if(!err_grp.errors.empty()) {
    processError();
    return;
  }

  TQuery Qry(&OraSession);

  // Сообщение обработано системой. Каждому пассажиру присвоен статус.
  if (trans.code == AnsTypeCirs) {

    map<int, pair<string, int>> result; // map<seq_num, pair<status, apps_pax_id>>
    for ( vector<TAnsPaxData>::const_iterator it = passengers.begin(); it < passengers.end(); it++ ) {
      // запишем в журнал операций присланный статус
      it->logAPPSPaxStatus(trans.code, point_id);
      // включим тревоги
      if (it->status == "D" || it->status == "X") {
        set_pax_alarm( it->pax_id, atAPPSNegativeDirective, true );
        set_crs_pax_alarm( it->pax_id, atAPPSNegativeDirective, true );
      }
      else if (it->status != "D" && it->status != "X" && it->status != "B") {
        set_pax_alarm( it->pax_id, atAPPSError, true );
        set_crs_pax_alarm( it->pax_id, atAPPSError, true );
      }

      /* Каждая страна участник APPS присылает свой статус пассажира, т.е.
       * один пассажир может иметь несколько статусов. Получим по каждому пассажиру
       * обобщенный статус для записи в apps_data. Если все "B" -> "B", если хотя бы
       * один "X" -> "X", если хотябы один "U","I","T" или "E" -> "P" (Problem) */
      if ( result[it->seq_num].first.empty() ||
           ( result[it->seq_num].first == "B" && it->status != "B" ) )
        result[it->seq_num].first = (it->status == "B" || it->status == "X")?it->status:"P";
      else if (result[it->seq_num].first == "P" && it->status == "X")
        result[it->seq_num].first = "X";
      if(it->apps_pax_id != ASTRA::NoExists && result[it->seq_num].second == 0)
        result[it->seq_num].second = it->apps_pax_id;
    }

    // сохраним полученные статусы
    Qry.Clear();
    Qry.SQLText = "BEGIN "
                  "UPDATE apps_data SET status = :status, apps_pax_id = :apps_pax_id "
                  "WHERE cirq_msg_id = :cirq_msg_id and cirq_seq_no = :cirq_seq_no "
                  "RETURNING pax_id INTO :pax_id; "
                  "END;";
    Qry.CreateVariable("cirq_msg_id", otString, trans.msg_ident);
    Qry.DeclareVariable("cirq_seq_no", otInteger);
    Qry.DeclareVariable("status", otString);
    Qry.DeclareVariable("apps_pax_id", otInteger);
    Qry.CreateVariable("pax_id", otInteger, FNull);

    for(map<int, pair<string, int>>::const_iterator it = result.begin(); it != result.end(); it++ ) {
      Qry.SetVariable("cirq_seq_no", it->first);
      Qry.SetVariable("status", it->second.first);
      if (it->second.second != 0)
        Qry.SetVariable("apps_pax_id", it->second.second);
      else
        Qry.SetVariable("apps_pax_id", FNull);
      Qry.Execute();

      // погасим тревоги
      int pax_id = Qry.GetVariableAsInteger("pax_id");
      if (it->second.first == "B") {
        set_pax_alarm( pax_id, atAPPSNegativeDirective, false );
        set_crs_pax_alarm( pax_id, atAPPSNegativeDirective, false );
        set_pax_alarm( pax_id, atAPPSError, false );
        set_crs_pax_alarm( pax_id, atAPPSError, false );
      }
      // проверим, нужно ли гасить тревогу "рассинхронизация"
      if ( !checkConflictAlarm( pax_id, trans.msg_ident, it->first ) ) {
          set_pax_alarm( pax_id, atAPPSConflict, false );
          set_crs_pax_alarm( pax_id, atAPPSConflict, false );
      }
    }
  }
  else if (trans.code == AnsTypeCicc) {
    map<int, bool> result; // map<seq_num, result> (true - pax отменен или не найден, false - ошибка)
    for ( vector<TAnsPaxData>::const_iterator it = passengers.begin(); it < passengers.end(); it++ ) {
      it->logAPPSPaxStatus(trans.code, point_id);
      if( result.count(it->seq_num) == 0 )
        result[it->seq_num] = (it->status == "C" || it->status == "N")?true:false;
      else if( it->status != "C" && it->status != "N")
        result[it->seq_num] = false;
    }
    Qry.Clear();
    Qry.SQLText = "BEGIN "
                  "SELECT send_time, pax_id INTO :send_time, :pax_id FROM apps_data "
                  "WHERE cicx_seq_no = :cicx_seq_no AND cicx_msg_id = :cicx_msg_id; "
                  "DELETE FROM apps_data WHERE send_time <= :send_time AND pax_id =:pax_id; "
                  "END;";
    Qry.CreateVariable("cicx_msg_id", otString, trans.msg_ident);
    Qry.DeclareVariable("cicx_seq_no", otInteger);
    Qry.CreateVariable( "send_time", otDate, FNull );
    Qry.CreateVariable( "pax_id", otInteger, FNull );

    for(map<int, bool>::const_iterator it = result.begin(); it != result.end(); it++) {
      if(it->second) {
        Qry.SetVariable("cicx_seq_no", it->first);
        Qry.Execute();
      }
    }
  }
  else if (trans.code == AnsTypeCima) {

    TElemFmt fmt;
    string apps_country = ElemToElemId(etCountry,mft_grp.country,fmt);
    if ( fmt == efmtUnknown )
      throw EConvertError("Unknown format");

    PrmLexema result("result", (mft_grp.resp_code == 8700)?"MSG.APPS_FLT_CLS_PROC":"MSG.APPS_FLT_CLS_REJ");
    if (mft_grp.resp_code != 8700) {
      if ( mft_grp.error_code == ASTRA::NoExists || mft_grp.error_text.empty() )
        throw Exception("processReply: mft_grp.error_code or mft_grp.error_text is empty");
      logErrorCondition(mft_grp.error_code, mft_grp.error_text, result.prms, "reason");
    }

    TReqInfo::Instance()->LocaleToLog( "MSG.APPS_CLS_RESP", LEvntPrms() << result <<
                                       PrmSmpl<string>("country", apps_country), evtFlt, point_id );
  }
  else
    throw Exception("Unknown code of transaction");

  deleteMsg(trans.msg_ident);
}

std::string APPSAnswer::toString() const
{
  std::ostringstream res;
  res << trans.toString();
  for (std::vector<TAnsPaxData>::const_iterator it = passengers.begin(); it != passengers.end(); it++)
    res << it->toString();
  res << mft_grp.toString() << err_grp.toString();
  return res.str();
}

bool APPSAnswer::getMsgInfo(const std::string& msg_id)
{
  TQuery Qry(&OraSession);
  Qry.SQLText="SELECT send_attempts, msg_text, point_id "
              "FROM apps_messages "
              "WHERE msg_id = :msg_id";
  Qry.CreateVariable("msg_id", otString, msg_id);
  Qry.Execute();

  if(Qry.Eof)
    return false;
  point_id = Qry.FieldAsInteger("point_id");
  msg_text = Qry.FieldAsString("msg_text");
  send_attempts = Qry.FieldAsInteger("send_attempts");
  return true;
}

void APPSAnswer::processError() const
{
  TReqInfo *reqInfo = TReqInfo::Instance();
  bool need_resend = false;
  string paxs = getPaxsStr();
  string lexema_id;
  TEventType evt_type;

  if (trans.code == AnsTypeCirs) {
    lexema_id = "MSG.APPS_BRD_RESP";
    evt_type = evtPax;
  }
  else if (trans.code == AnsTypeCicc) {
    lexema_id = "MSG.APPS_CNL_RESP";
    evt_type = evtPax;
  }
  else if (trans.code == AnsTypeCima) {
    lexema_id = "MSG.APPS_CLS_RESP";
    evt_type = evtFlt;
  }
  else
    throw Exception( std::string( "Unknown transaction code: " + trans.code ) );

  for(std::vector<TError>::const_iterator it = err_grp.errors.begin(); it < err_grp.errors.end(); it ++) {
    // запишем в журнал ошибку
    LEvntPrms params;
    if ( !it->country.empty() ) {
      TElemFmt fmt;
      string apps_country = ElemToElemId(etCountry,it->country,fmt);
      if ( fmt == efmtUnknown )
        throw EConvertError("Unknown format");
      PrmLexema lexema("country", "MSG.APPS_COUNTRY");
      lexema.prms << PrmElem<string>("country", etCountry, apps_country);
      params << lexema;
    }
    else
      params << PrmSmpl<string>("country", "");

    if (!paxs.empty())
      params << PrmSmpl<string>("passengers", paxs);

    logErrorCondition( it->error_code, it->error_text, params, "result" );

    reqInfo->LocaleToLog( lexema_id, params, evt_type, point_id );

    if ( it->error_code == 6102 || it->error_code == 6900 || it->error_code == 6979 || it->error_code == 5057) {
      // ... Try again later
      need_resend = true;
    }
  }

  // Определим, нужно ли отправлять повторно
  if ( need_resend ) {
    LEvntPrms params;
    if (!paxs.empty()) {
      PrmLexema paxs_param("passengers", "MSG.APPS_PASSENGERS");
      paxs_param.prms << PrmSmpl<string>("passengers", paxs);
      params << paxs_param;
    }
    else params << PrmSmpl<string>("passengers", "");
    if( send_attempts < MaxSendAttempts ) {
      // Посылаем еще раз
      reSendMsg(send_attempts, msg_text, trans.msg_ident);
      reqInfo->LocaleToLog( "MSG.APPS_REQ_RESENT", params, evt_type, point_id );
      return;
    }
    // Не посылаем. Превышено максимальное число попыток
    reqInfo->LocaleToLog( "MSG.APPS_REQ_NOT_RESENT", params, evt_type, point_id );
  }

  // не отправили повторно
  deleteMsg(trans.msg_ident);

  if(trans.code != AnsTypeCirs)
    return;

  for ( vector<TAnsPaxData>::const_iterator it = passengers.begin(); it < passengers.end(); it++ ) {
    set_pax_alarm( it->pax_id, atAPPSConflict, true ); // рассинхронизация
    set_crs_pax_alarm( it->pax_id, atAPPSConflict, true ); // рассинхронизация
  }

  // удаляем apps_data cirq_msg_id
  TQuery Qry(&OraSession);
  Qry.SQLText = "DELETE FROM apps_data WHERE cirq_msg_id = :cirq_msg_id ";
  Qry.CreateVariable("cirq_msg_id", otString, trans.msg_ident);
  Qry.Execute();
}

std::string APPSAnswer::getPaxsStr() const
{
  ostringstream paxs;
  for ( vector<TAnsPaxData>::const_iterator it = passengers.begin(); it < passengers.end(); it++ ) {
    if (it != passengers.begin())
      paxs << ", ";
    paxs << it->family_name;
  }
  return paxs.str();
}

bool isNeedAPPSReq(const int point_dep, const int point_arv)
{
  set<string> countries;

  TTripRoute route;
  route.GetRouteAfter(NoExists, point_dep, trtWithCurrent, trtNotCancelled);

  for ( TTripRoute::const_iterator r = route.begin(); r != route.end() && r->point_id <= point_arv; r++ )
    countries.insert( getCountry( r->airp ) );

  return isNeedAPPSReq(countries);
}

bool isNeedAPPSReq( const int point_dep, const std::string& airp_arv )
{
  set<string> countries;

  TTripRoute route;
  route.GetRouteAfter(NoExists, point_dep, trtWithCurrent, trtNotCancelled);

  for ( TTripRoute::const_iterator r = route.begin(); r != route.end(); r++ ) {
    countries.insert( getCountry( r->airp ) );
    if ( r->airp == airp_arv )
      break;
  }

  return isNeedAPPSReq(countries);
}

bool isNeedAPPSReq(const std::set<std::string>& countries)
{
  if(countries.size() < 2)
    return false; // не отправляем для местных рейсов

  for (set<string>::const_iterator it = countries.begin(); it != countries.end(); it++)
    if(isAPPSCountry(*it)) return true;

  return false;
}

bool needFltCloseout(const set<string>& countries, set<string>& countries_need_req)
{
  if(countries.size() < 2)
    return false; // не отправляем для местных рейсов

  for (set<string>::const_iterator it = countries.begin(); it != countries.end(); it++)
    if(isAPPSCountry(*it)) countries_need_req.insert(*it);
  /* TODO: Не все APPS country требуют flt closeout.
   * В будущем нужно будет хранить в базе список APPS стран и список типов
   * требуемых манифестов */

  return !countries_need_req.empty();
}

void composeAPPSReq( const TSegmentsList& segs, const int point_dep, const TPaxDocList& paxs,
                     const bool is_crew, const string& override_type )
{
  ProgTrace(TRACE5, "composeAPPSReq: point_dep: %d", point_dep);
  TSegmentsList::const_iterator s = segs.begin();
  for (; s < segs.end() && s->point_dep != point_dep; s++);
  if (s == segs.end())
    throw Exception("point_id was not found in map segs");

  //определим, нужно ли отправлять APPS request
  if (!isNeedAPPSReq(s->point_dep, s->point_arv) || !checkTime( s->point_dep ))
    return;

  APPSRequest add_req(ReqTypeCirq, getAPPSUser(), point_dep);
  APPSRequest cancel_req(ReqTypeCicx, getAPPSUser(), point_dep);

  // здесь заполним все, то касается перелета
  // International flight
  TFlightData flt;
  flt.init(FltTypeInt, point_dep);
  add_req.int_flt.init(flt);
  cancel_req.int_flt.init(flt);

  int transfer = None;
  if( segs.size() > 1 ) {

    /* TODO заполнить expected flight - a flight at which a passenger will be
       cleared by Customs and Immigration for movement into or out of a country. */

    // выясним, нужно ли заполнять Check-in Flight
    if ( s != segs.begin() ) {
      add_req.ckin_flt.init( FltTypeChk, segs.begin()->point_dep );
      if ( isAPPSCountry( s->country_dep ) && (s - 1)->country_dep != s->country_dep ) {
        transfer = Origin;
      }
    }
    if ( (s + 1) != segs.end() && isAPPSCountry( s->country_arv ) &&
          s->country_arv != (s + 1)->country_arv ) {
      transfer=( ( transfer == Origin ) ? Both : Dest );
    }
  }
  // информация о пассажирах
  for( TPaxDocList::const_iterator it = paxs.begin(); it < paxs.end(); it++ ) {
    TReqPaxData new_pax;
    new_pax.init( is_crew, *it, transfer, override_type );
    TReqPaxData actual_pax;
    APPSAction action;
    bool is_exist = actual_pax.fromDB(it->pax_id);
    if(!is_exist && it->is_cancel) {
      action = NoAction;
    }
    else if(!is_exist && !it->is_cancel)
      action = NeedNew;
    else
      action = new_pax.typeOfAction(actual_pax.status, new_pax.equalAttrs(actual_pax), it->is_cancel, false);
    if (action == NoAction)
      continue;
    else if ( action == NeedNew ) {
      add_req.addPax(new_pax);
    }
    else if ( action == NeedUpdate ) {
      cancel_req.addPax(actual_pax);
      add_req.addPax(new_pax);
    }
    else if ( action == NeedCancel ) {
      cancel_req.addPax(actual_pax);
    }
    else
      throw Exception("Unknown type of action");
  }
  cancel_req.sendReq();
  add_req.sendReq();
}

void processPax(const int pax_id, const std::string& override_type )
{
  ProgTrace(TRACE5, "processPax: %d", pax_id);
  TQuery Qry(&OraSession);
  Qry.SQLText="SELECT surname, name, pax.grp_id, status, point_dep, "
                 "       airp_dep, airp_arv, refuse "
                 "FROM pax_grp, pax "
                 "WHERE pax_id = :pax_id AND pax_grp.grp_id = pax.grp_id";
  Qry.CreateVariable("pax_id", otInteger, pax_id);
  Qry.Execute();

  if(Qry.Eof) {
    processCrsPax(pax_id, override_type);
    return;
  }

  int point_id = Qry.FieldAsInteger("point_dep");
  string airp_arv = Qry.FieldAsString("airp_arv");

  //определим, нужно ли отправлять APPS request
  if (!isNeedAPPSReq(point_id, airp_arv))
    throw AstraLocale::UserException("Passenger does not need to be processed by APPS");

  // Определим, пришло ли время.
  if ( !checkTime( point_id ) )
    throw AstraLocale::UserException ( "Time window [- 2 days] TODAY [+ 10 days] was violated" );

  TCkinRoute tckin_route;
  int grp_id = Qry.FieldAsInteger("grp_id");
  tckin_route.GetRouteBefore( grp_id, crtNotCurrent, crtIgnoreDependent );

  int chkin_point_id = tckin_route.front().point_dep;

  int transfer = None;
  TCkinRouteItem prior, next;
  // проверим входящий трансфер
  prior = tckin_route.back();
  if ( !prior.airp_dep.empty() ) {
    string country_dep = getCountry(Qry.FieldAsString("airp_dep"));
    if ( isAPPSCountry( country_dep ) &&
       ( getCountry(prior.airp_dep) != country_dep ) )
      transfer = Origin;
  }
  // проверим исходящий трансфер
  TCkinRoute().GetNextSeg(grp_id, crtIgnoreDependent, next );
  if ( !next.airp_arv.empty() ) {
    string country_arv = getCountry(airp_arv);
    if ( isAPPSCountry( country_arv ) &&
       ( getCountry( next.airp_arv ) != country_arv ) )
      transfer = ( ( transfer == Origin ) ? Both : Dest );
  }
  TPaxData data;
  CheckIn::LoadPaxDoc(pax_id, data.doc);
  data.pax_id = pax_id;
  data.is_cancel = !Qry.FieldIsNULL("refuse");
  data.surname = Qry.FieldAsString("surname");
  if(!Qry.FieldIsNULL("name"))
    data.name = Qry.FieldAsString("name");
  TPaxStatus pax_status = DecodePaxStatus(Qry.FieldAsString("status"));
  TReqPaxData new_pax;
  new_pax.init( (pax_status==psCrew), data, transfer, override_type );
  sendPax( new_pax, point_id, chkin_point_id, data.is_cancel );
}

void processCrsPax( const int pax_id, const std::string& override_type )
{
  ProgTrace(TRACE5, "processCrsPax: %d", pax_id);
  TQuery Qry(&OraSession);
  Qry.SQLText="SELECT surname, name, point_id_spp, airp_arv, crs_pax.pnr_id, pr_del "
              "FROM crs_pax, crs_pnr, tlg_binding "
              "WHERE pax_id = :pax_id AND crs_pax.pnr_id = crs_pnr.pnr_id AND "
              "      crs_pnr.point_id = tlg_binding.point_id_tlg";
  Qry.CreateVariable("pax_id", otInteger, pax_id);
  Qry.Execute();

  if (Qry.Eof)
    throw AstraLocale::UserException("Passenger was not found");

  int point_id = Qry.FieldAsInteger("point_id_spp");
  string airp_arv = Qry.FieldAsString("airp_arv");

  //определим, нужно ли отправлять APPS request
  if (!isNeedAPPSReq(point_id, airp_arv))
    throw AstraLocale::UserException("Passenger does not need to be processed by APPS");

  // Определим, пришло ли время.
  if ( !checkTime( point_id ) )
    throw AstraLocale::UserException( "Time window [- 2 days] TODAY [+ 10 days] was violated" );

  TCachedQuery FltQry("SELECT airline, flt_no, suffix, airp, scd_out FROM points WHERE point_id=:point_id AND pr_del>=0",
                      QParams() << QParam("point_id", otInteger, point_id));
  FltQry.get().Execute();

  if (FltQry.get().Eof)
    throw AstraLocale::UserException("Flight was not found");

  TTripInfo operFlt(FltQry.get());
  TQuery TrferQry(&OraSession);
  map<int, CheckIn::TTransferItem> trfer;
  CheckInInterface::GetOnwardCrsTransfer(Qry.FieldAsInteger("pnr_id"), TrferQry, operFlt, airp_arv, trfer);

  int transfer = None;
  if (!trfer[1].airp_arv.empty()) {
    // сквозная регистрация
    string country_arv = getCountry(airp_arv);
    if ( isAPPSCountry( country_arv ) &&
       ( getCountry( trfer[1].airp_arv ) != country_arv ) )
      transfer = Dest; // исходящий трансфер
  }
  TPaxData data;
  CheckIn::LoadCrsPaxDoc(pax_id, data.doc);
  data.pax_id = pax_id;
  data.is_cancel = Qry.FieldAsInteger("pr_del");
  data.surname = Qry.FieldAsString("surname");
  if(!Qry.FieldIsNULL("name"))
    data.name = Qry.FieldAsString("name");
  TReqPaxData new_pax;
  new_pax.init( false, data, transfer, override_type );
  sendPax( new_pax, point_id, ASTRA::NoExists, data.is_cancel );
}

void APPSFlightCloseout( const int point_id )
{
  if ( !checkTime( point_id ) )
    return;

  TFlightData flt;
  flt.init( FltTypeInt, point_id );

  APPSRequest cancel_pax( ReqTypeCicx, getAPPSUser(), point_id );
  cancel_pax.int_flt.init(flt);

  APPSRequest cancel_crew( ReqTypeCicx, getAPPSUser(), point_id );
  cancel_crew.int_flt.init( flt );

  TTripRoute route;
  route.GetRouteAfter( NoExists, point_id, trtWithCurrent, trtNotCancelled );
  TTripRoute::const_iterator r=route.begin();
  set<string> countries;
  countries.insert(getCountry(r->airp));
  int point_dep = r->point_id;
  for(r++; r!=route.end(); r++) {
    // определим, нужно ли отправлять данные
    countries.insert(getCountry(r->airp));
    if(!isNeedAPPSReq(countries)) {
      continue;
    }

    /* Определим, есть ли пассажиры не прошедшие посадку,
     * информация о которых была отправлена в SITA.
     * Для таких пассажиров нужно послать отмену */
    TQuery Qry(&OraSession);
    Qry.Clear();
    Qry.SQLText="SELECT cirq_msg_id, cirq_seq_no, pax_grp.status "
                "FROM pax_grp, pax, apps_data "
                "WHERE pax_grp.grp_id=pax.grp_id AND apps_data.pax_id = pax.pax_id AND "
                "      pax_grp.point_dep=:point_dep AND pax_grp.point_arv=:point_arv AND "
                "      (pax.name IS NULL OR pax.name<>'CBBG') AND "
                "      pr_brd=0 AND apps_data.status = 'B'";
    Qry.CreateVariable("point_dep", otInteger, point_dep);
    Qry.CreateVariable("point_arv", otInteger, r->point_id);
    Qry.Execute();

    for(; !Qry.Eof; Qry.Next()) {
      bool is_crew = (DecodePaxStatus(Qry.FieldAsString("status")) == psCrew);
      TReqPaxData pax;
      pax.fromDB(ReqTypeCirq, Qry.FieldAsString("cirq_msg_id"), Qry.FieldAsInteger("cirq_seq_no"));
      if(pax.status != "B") {
        continue; // CICX request has already been send
      }
      if (is_crew)
        cancel_crew.addPax(pax);
      else
        cancel_pax.addPax(pax);
    }
  }
  cancel_crew.sendReq();
  cancel_pax.sendReq();

  set<string> countries_need_req;
  needFltCloseout( countries, countries_need_req );
  APPSRequest close_flt( ReqTypeCimr, getAPPSUser(), point_id );
  close_flt.inm_flt.init( flt );
  for( set<string>::const_iterator it = countries_need_req.begin(); it != countries_need_req.end(); it++ ) {
    close_flt.mft_req.init( *it );
    close_flt.sendReq();
  }
}

bool IsAPPSAnswText(const std::string& tlg_body)
{
  string trans_type = tlg_body.substr( 0, AnsTypeCirs.size());
  string delim = tlg_body.substr( AnsTypeCirs.size(), 1);
  return ( ( trans_type == AnsTypeCirs || trans_type == AnsTypeCicc ||
             trans_type == AnsTypeCima ) && delim == ":");
}

std::string emulateAnswer( const std::string& request )
{
  ostringstream answer;
  string code = request.substr(0, ReqTypeCirq.size());

  boost::regex pattern("(?<=:).*?(?=/)");
  boost::smatch result;
  if (!boost::regex_search(request, result, pattern))
    throw EXCEPTIONS::Exception( "emulateAnswer: msg_id was not found" );
  string msg_id = result[0];

  string ans_type;
  if(code == ReqTypeCirq) {
    ans_type = AnsTypeCirs;
  }
  else if (code == ReqTypeCicx) {
    ans_type = AnsTypeCicc;
  }
  else if (code == ReqTypeCimr) {
    ans_type = AnsTypeCima;
  }
  else
    throw EXCEPTIONS::Exception( std::string( "emulateAnswer: unknown transaction code: " + code ) );

  answer << ans_type << ":" << msg_id << "/";

  int val = std::time(0) % 10;
  if (val == 0)
    return ""; // no answer
  if (val == 1)
    return answer.str() + string("ERR/3/AE/6999/AP ERROR: PL-SQL FAILED/");
  if (val == 2)
    return answer.str() + string("ERR/3//5057/NO RESPONSE FROM ETA-APP SYSTEM. PLEASE TRY AGAIN LATER.");

  if ( code == ReqTypeCirq || code == ReqTypeCicx) {

    std::pair<std::string, int> ans_info = (code == ReqTypeCirq)?PaxAnsPrs:PaxAnsPcc;
    std::pair<std::string, int> req_info = (code == ReqTypeCirq)?PaxReqPrq:PaxReqPcx;
    string num_cut = (code == ReqTypeCirq)?"5":"2";

    string str = string("(?<=") + req_info.first + string("/") +
                 IntToString(req_info.second) + string("/).*?((?=(/[^/]*){" +
                 num_cut + "}/") + req_info.first +
                 string(")|(?=(/[^/]*){" + num_cut + "}/$))");
    boost::regex regex(str);

    const int subs[] = {0};
    boost::sregex_token_iterator i(request.begin(), request.end(), regex, subs);
    boost::sregex_token_iterator j;
    if (i == j)
      throw EXCEPTIONS::Exception( "emulateAnswer: passengers were not found" );
    while(i != j) {
      string pax(*i++);

      string seq_no = pax.substr( 0, pax.find( "/" ) );
      size_t pos = (code == ReqTypeCirq)?0:pax.find("/");
      string pax_info = pax.substr( pax.find( "/", pos + 1 ) + 1 );
      answer << ans_info.first << "/" << ans_info.second << "/" << seq_no << "/AE/" << pax_info;
      if (ans_type == AnsTypeCirs)
        answer << requestResStr();
      else
        answer << cancelResStr();
    }
  }
  else {
    answer << MftAnsMak.first << "/" << MftAnsMak.second << "/AE/";
    if ( val == 3 || val == 4 )
      answer << "8701/5593/INTERNATIONAL FLIGHT NOT IN AIRLINE SCHEDULES/";
    else
      answer << "8700///";
  }
  return answer.str();
}

void reSendMsg( const int send_attempts, const std::string& msg_text, const std::string& msg_id )
{
  sendTlg( getAPPSAddr().c_str(), OWN_CANON_NAME(), qpOutApp, 20, msg_text,
          ASTRA::NoExists, ASTRA::NoExists );

  sendCmd("CMD_APPS_ANSWER_EMUL","H");
  sendCmd("CMD_APPS_HANDLER","H");

  TQuery Qry(&OraSession);
  Qry.SQLText = "UPDATE apps_messages "
                "SET send_attempts = :send_attempts, send_time = :send_time "
                "WHERE msg_id = :msg_id ";
  Qry.CreateVariable("msg_id", otString, msg_id);
  Qry.CreateVariable("send_attempts", otInteger, send_attempts + 1);
  Qry.CreateVariable("send_time", otDate, BASIC::NowUTC());
  Qry.Execute();
  ProgTrace(TRACE5, "Message id=%s was re-sent. Send attempts: %d", msg_id.c_str(), send_attempts);
}

void sendAPPSInfo( const int point_id, const std::string& task_name, const std::string& params )
{
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText ="SELECT pax_id "
               "FROM crs_pax, crs_pnr, tlg_binding "
               "WHERE need_apps <> 0 AND point_id_spp = :point_id_spp AND "
               "      crs_pax.pnr_id = crs_pnr.pnr_id AND "
               "      crs_pnr.point_id = tlg_binding.point_id_tlg";
  Qry.CreateVariable("point_id_spp", otInteger, point_id);
  Qry.Execute();

  for( ; !Qry.Eof; Qry.Next() ) {
    int pax_id = Qry.FieldAsInteger( "pax_id" );
    processCrsPax( pax_id, "" );
    TCachedQuery Qry( "UPDATE crs_pax SET need_apps=0 WHERE pax_id=:pax_id",
                      QParams() << QParam( "pax_id", otInteger, pax_id ) );
    Qry.get().Execute();
  }
}

void deleteMsg( const std::string& msg_id )
{
  TQuery Qry(&OraSession);
  Qry.SQLText = "DELETE FROM apps_messages "
                "WHERE msg_id = :msg_id ";
  Qry.CreateVariable("msg_id", otString, msg_id);
  Qry.Execute();
}

bool checkTime( const int point_id )
{
  // The APP System only allows transactions on [- 2 days] TODAY [+ 10 days].
  BASIC::TDateTime now = BASIC::NowUTC();
  TAdvTripRoute route;
  route.GetRouteAfter(ASTRA::NoExists, point_id, trtWithCurrent, trtNotCancelled);
  if (now - route.front().scd_out > 2 && route.front().scd_out - now > 10)
    return false;
  return true;
}
