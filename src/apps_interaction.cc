#include <boost/algorithm/string.hpp>
#include <ctime>
#include <deque>
#include <boost/regex.hpp>
#include <sstream>
#include "alarms.h"
#include "apps_interaction.h"
#include "astra_misc.h"
#include "basic.h"
#include "exceptions.h"
#include "points.h"
#include "qrys.h"
#include "tlg/tlg.h"

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
static const std::string  H2HSender = "ASTRA";

static const int BasicFormatVer = 21;

static const int MaxCirqPaxNum = 5;
static const int MaxCicxPaxNum = 10;

enum { None, Origin, Dest, Both };

static std::string getH2HReceiver()
{
  static string VAR;
  if ( VAR.empty() )
    VAR=getTCLParam( "APPS_H2H_ADDR", NULL );
  return VAR;
}

static std::string makeHeader( const std::string& airline )
{
  return string( "V.\rVHLG.WA/E5" + H2HSender + "/I5" + getH2HReceiver() + "/P0001\rVGZ.\rV" + airline + "/MOW/////////RU\r" );
}

static std::string getUserId( const TAirlinesRow &airline )
{
  string user_id;
  if ( !airline.code_lat.empty() && !airline.code_icao_lat.empty() )
   user_id = airline.code_lat + airline.code_icao_lat + "1";
  else user_id = "AST1H";
  return user_id;
}

static int getIdent()
{
  TQuery Qry(&OraSession);
  Qry.SQLText = "SELECT apps_msg_id__seq.nextval vid FROM dual";
  Qry.Execute();
  return Qry.FieldAsInteger("vid");
}

static bool isAPPSCountry( const std::string& country, const std::string& airline )
{
  TQuery AppsSetsQry( &OraSession );
  AppsSetsQry.Clear();
  AppsSetsQry.SQLText=
    "SELECT apps_country "
    "FROM apps_sets "
    "WHERE airline=:airline AND apps_country=:apps_country AND pr_denial=0";
  AppsSetsQry.CreateVariable( "airline", otString, airline );
  AppsSetsQry.CreateVariable( "apps_country", otString, country );

  AppsSetsQry.Execute();
  if (!AppsSetsQry.Eof)
    return true;

  return false;
}

static void sendNewReq( const std::string& text, const int msg_id, const int point_id )
{
  // отправим телеграмму
  sendTlg( APPSAddr, OWN_CANON_NAME(), qpOutApp, 20, text,
          ASTRA::NoExists, ASTRA::NoExists );

//  sendCmd("CMD_APPS_ANSWER_EMUL","H");
  sendCmd("CMD_APPS_HANDLER","H");

  ProgTrace(TRACE5, "New APPS request generated: %s", text.c_str());

  // сохраним информацию о сообщении
  TQuery Qry( &OraSession );
  Qry.SQLText = "INSERT INTO apps_messages(msg_id, msg_text, send_attempts, send_time, point_id) "
                "VALUES (:msg_id, :msg_text, :send_attempts, :send_time, :point_id)";
  Qry.CreateVariable("msg_id", otString, msg_id);
  Qry.CreateVariable("send_time", otDate, BASIC::NowUTC());
  Qry.CreateVariable("msg_text", otString, text);
  Qry.CreateVariable("send_attempts", otInteger, 1);
  Qry.CreateVariable("point_id", otInteger, point_id);
  Qry.Execute();
}

bool checkAPPSSets( const int point_dep, const int point_arv )
{
  bool result = false;
  TAdvTripRoute route;
  route.GetRouteAfter(NoExists, point_dep, trtWithCurrent, trtNotCancelled);

  TQuery AppsSetsQry( &OraSession );
  AppsSetsQry.Clear();
  AppsSetsQry.SQLText=
    "SELECT inbound, outbound "
    "FROM apps_sets "
    "WHERE airline=:airline AND apps_country=:apps_country AND pr_denial=0";
  AppsSetsQry.CreateVariable( "airline", otString, route.front().airline );
  AppsSetsQry.DeclareVariable( "apps_country", otString );
  
  for ( TAdvTripRoute::const_iterator r = route.begin(); r != route.end(); r++ ) {
    AppsSetsQry.SetVariable( "apps_country", getCountryByAirp( r->airp ).code );
    AppsSetsQry.Execute();
    if (!AppsSetsQry.Eof &&
      !( r->point_id == point_dep && !AppsSetsQry.FieldAsInteger("outbound") ) &&
      !( r->point_id == point_arv && !AppsSetsQry.FieldAsInteger("inbound") ) )
        result = true;
    if ( r->point_id == point_arv )
      return result;
  }
  ProgTrace( TRACE5, "Point %d was not found in the flight route !!!", point_arv );
  return false;
}

bool checkAPPSSets( const int point_dep, const std::string& airp_arv)
{
  bool transit;
  return checkAPPSSets( point_dep, airp_arv, transit );
}

bool checkAPPSSets( const int point_dep, const std::string& airp_arv, bool& transit )
{
  bool result = false;
  transit = false;
  TAdvTripRoute route;
  route.GetRouteAfter(NoExists, point_dep, trtWithCurrent, trtNotCancelled);

  if ( route.empty() )
    throw Exception( "Empty route!!!" );

  TQuery AppsSetsQry( &OraSession );
  AppsSetsQry.Clear();
  AppsSetsQry.SQLText=
    "SELECT inbound, outbound "
    "FROM apps_sets "
    "WHERE airline=:airline AND apps_country=:apps_country AND pr_denial=0";
  AppsSetsQry.CreateVariable( "airline", otString, route.front().airline );
  AppsSetsQry.DeclareVariable( "apps_country", otString );

  for ( TAdvTripRoute::const_iterator r = route.begin(); r != route.end(); r++ ) {
    AppsSetsQry.SetVariable( "apps_country", getCountryByAirp( r->airp ).code );
    AppsSetsQry.Execute();
    if (!AppsSetsQry.Eof) {
      if ( r->airp != airp_arv && r->point_id != point_dep )
        transit = true;
      else if ( !( r->airp == airp_arv && !AppsSetsQry.FieldAsInteger("inbound") ) &&
                !( r->point_id == point_dep && !AppsSetsQry.FieldAsInteger("outbound") ) )
        result = true;
    }
    if ( r->airp == airp_arv )
      return result;
  }
  ProgTrace( TRACE5, "Airport %s was not found in the flight route !!!", airp_arv.c_str() );
  return false;
}

bool checkTime( const int point_id )
{
  BASIC::TDateTime start_time = ASTRA::NoExists;
  return checkTime( point_id, start_time );
}

bool checkTime( const int point_id, BASIC::TDateTime& start_time )
{
  start_time = ASTRA::NoExists;
  BASIC::TDateTime now = BASIC::NowUTC();
  TTripInfo trip;
  BASIC::TDateTime scd_out = trip.getByPointId(point_id);
  // The APP System only allows transactions on [- 2 days] TODAY [+ 10 days].
  if ( now - trip.scd_out > 2 )
    return false;
  if ( trip.scd_out - now > 10 ) {
    start_time = scd_out - 10;
  }
  return true;
}

static void deleteAPPSData( const int pax_id )
{
  TQuery Qry(&OraSession);
  Qry.SQLText = "DELETE FROM apps_pax_data WHERE pax_id = :pax_id";
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

void TTransData::init( const bool pre_ckin, const std::string& trans_code, const std::string& id )
{
  code = trans_code;
  msg_id = getIdent();
  user_id = id;
  type = pre_ckin;
  ver = BasicFormatVer;
}

void TTransData::check_data() const
{
  if (code != ReqTypeCirq && code != ReqTypeCicx && code != ReqTypeCimr)
    throw Exception("Incorrect transacion code");
  if (ver != BasicFormatVer)
    throw Exception("Incorrect version");
  if (user_id.empty() || user_id.size() > 6)
    throw Exception("Incorrect User ID");
}

std::string TTransData::msg() const
{
  check_data();
  std::ostringstream msg;
  msg << code << ':' << msg_id << '/' << user_id;
  if (code != ReqTypeCimr)
    msg << "/N/" << ( type?"P":"" );
  msg << '/' << ver;
  return msg.str();
}

void TFlightData::init( const int id, const std::string& flt_type )
{
  if ( id == ASTRA::NoExists )
    return;
  point_id = id;
  type = flt_type;
  TAdvTripRoute route, tmp;
  route.GetRouteBefore(NoExists, point_id, trtWithCurrent, trtNotCancelled);
  tmp.GetRouteAfter(NoExists, point_id, trtNotCurrent, trtNotCancelled);
  route.insert(route.end(), tmp.begin(), tmp.end());

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

  if( type != FltTypeChk.first )
    date = UTCToLocal( route.front().scd_out, CityTZRegion(airp_dep.city) );

  if( type == FltTypeInt.first ) {
  TAirpsRow &airp_arv = (TAirpsRow&)base_tables.get("airps").get_row("code", route.back().airp);
  if (airp_arv.code_lat.empty()) throw Exception("airp_arv.code_lat empty (code=%s)",airp_arv.code.c_str());
  arv_port = airp_arv.code_lat;

  arv_date = UTCToLocal( route.back().scd_in, CityTZRegion( airp_arv.city ) );
  }
}

void TFlightData::init( const int id, const std::string& flt_type, const std::string&  num, const std::string& airp, const std::string& arv_airp,
           BASIC::TDateTime dep, BASIC::TDateTime arv )
{
  point_id = id;
  type = flt_type;
  flt_num = num;
  port = airp;
  arv_port = arv_airp;
  date = dep;
  arv_date = arv;
}

void TFlightData::check_data() const
{
  if( type != FltTypeChk.first && type != FltTypeInm.first && type != FltTypeInt.first )
    throw Exception( "Incorrect type of flight: %s", type.c_str() );
  if ( flt_num.empty() || flt_num.size() > 8 )
    throw Exception(" Incorrect flt_num: %s", flt_num.c_str() );
  if ( port.empty() || port.size() > 5 )
    throw Exception( "Incorrect airport: %s", port.c_str() );
  if ( type != FltTypeChk.first && date == ASTRA::NoExists )
    throw Exception( "Date empty" );
  if ( type == FltTypeInt.first ) {
    if ( arv_port.empty() || arv_port.size() > 5 )
      throw Exception( "Incorrect arrival airport: %s", arv_port.c_str() );
    if ( arv_date == ASTRA::NoExists )
      throw Exception( "Arrival date empty" );
  }
}

std::string TFlightData::msg() const
{
  check_data();
  std::ostringstream msg;
  msg << type << '/';
  if ( type == FltTypeChk.first ) {
    msg << FltTypeChk.second << '/' << port << '/' << flt_num;
    return msg.str();
  }
  else if ( type == FltTypeInt.first )
    msg << FltTypeInt.second << "/S";
  else msg << FltTypeInm.second;
  msg << '/' << flt_num << '/' << port;
  if (type == FltTypeInt.first)
    msg  << '/' << arv_port;
  msg  << '/' << DateTimeToStr( date, "yyyymmdd" );
  if (type == FltTypeInt.first) {
    msg << '/' << DateTimeToStr( date, "hhnnss" ) << '/'
        << DateTimeToStr( arv_date, "yyyymmdd" ) << '/'
        << DateTimeToStr( arv_date, "hhnnss" );
  }
  return msg.str();
}

void TPaxData::init( const int pax_ident, const std::string& surname, const std::string name,
                     const bool is_crew, const int transfer, const std::string& override )
{
  pax_id = pax_ident;
  CheckIn::TPaxDocItem doc;
  CheckIn::LoadPaxDoc(pax_ident, doc);
  pax_crew = is_crew?"C":"P";
  nationality = doc.nationality;
  issuing_state = doc.issue_country;
  passport = doc.no.substr(0, 14);
  doc_type = (doc.type == "P" || doc.type.empty())?"P":"O";
  if ( doc.expiry_date != ASTRA::NoExists )
    expiry_date = DateTimeToStr( doc.expiry_date, "yyyymmdd" );
  if( !doc.surname.empty() ) {
    family_name = transliter(doc.surname, 1, 1);
    if (!doc.first_name.empty())
      given_names = doc.first_name;
    if (!given_names.empty() && !doc.second_name.empty())
      given_names = given_names + " " + doc.second_name;
    given_names = given_names.substr(0, 24);
    given_names = transliter(given_names, 1, 1);
  }
  else {
    family_name = surname;
    family_name = transliter(family_name, 1, 1);
    if(!name.empty())
      given_names = (transliter(name, 1, 1));
  }
  if ( doc.birth_date != ASTRA::NoExists )
    birth_date = DateTimeToStr( doc.birth_date, "yyyymmdd" );
  sex = (doc.gender == "M" || doc.gender == "F")?doc.gender:"U";
  trfer_at_origin = (transfer == Origin || transfer == Both)?"Y":"N";
  trfer_at_dest = (transfer == Dest || transfer == Both)?"Y":"N";
  if(!override.empty())
    override_codes = override;
}

void TPaxData::init( TQuery &Qry )
{
  pax_id = Qry.FieldAsInteger("pax_id");
  if (!Qry.FieldIsNULL("apps_pax_id"))
    apps_pax_id = Qry.FieldAsString("apps_pax_id");
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

void TPaxData::check_data() const
{
  if ( pax_id == ASTRA::NoExists )
    throw Exception( "Empty pax_id" );
  if ( pax_crew != "C" && pax_crew != "P" )
    throw Exception( "Incorrect pax_crew %s", pax_crew.c_str() );
  if( passport.size() > 14)
    throw Exception( "Passport number too long: %s", passport.c_str() );
  if( !doc_type.empty() && doc_type != "P" && doc_type != "O" && doc_type != "N" )
    throw Exception( "Incorrect doc_type: %s", doc_type.c_str() );
  if( family_name.empty() || family_name.size() < 2 || family_name.size() > 24 )
    throw Exception( "Incorrect family_name: %s", family_name.c_str() );
  if( given_names.size() > 24 )
    throw Exception( "given_names too long: %s", given_names.c_str() );
  if( !sex.empty() && sex != "M" && sex != "F" && sex != "U" && sex != "X" )
    throw Exception( "Incorrect gender: %s", sex.c_str() );
  if( !endorsee.empty() && endorsee != "S" )
    throw Exception( "Incorrect endorsee: %s", endorsee.c_str() );
}

std::string TPaxData::msg() const
{
  check_data();
  std::ostringstream msg;
  if ( apps_pax_id.empty() ) {
    msg << PaxReqPrq.first << '/' << PaxReqPrq.second << "/1";
  }
  else {
    msg << PaxReqPcx.first << '/' << PaxReqPcx.second << "/1/" << apps_pax_id;
  }
  msg << '/' << pax_crew << '/' << nationality << '/' << issuing_state << '/' << passport << '/'
      << check_char << '/' << doc_type << '/' << expiry_date << '/' << sup_doc_type << '/'
      << sup_passport << '/' << sup_check_char << '/' << family_name << '/'
      << (given_names.empty()?"-":given_names) << '/' << birth_date << '/' << sex << '/'
      << birth_country << '/' << endorsee << '/' << trfer_at_origin << '/' << trfer_at_dest;
  if ( apps_pax_id.empty() )
    msg  << '/' << override_codes << '/' << pnr_source << '/' << pnr_locator;
  return msg.str();
}

bool TPaxRequest::getByPaxId( const int pax_id, const std::string& override_type )
{
  ProgTrace( TRACE5, "TPaxRequest::getByPaxId: %d", pax_id );
  TQuery Qry( &OraSession );
  Qry.SQLText="SELECT surname, name, pax.grp_id, status, point_dep, refuse, airp_dep, airp_arv "
              "FROM pax_grp, pax "
              "WHERE pax_id = :pax_id AND pax_grp.grp_id = pax.grp_id";
  Qry.CreateVariable( "pax_id", otInteger, pax_id );
  Qry.Execute();

  if(Qry.Eof)
    return false;

  int point_dep = Qry.FieldAsInteger("point_dep");
  string airp_arv = Qry.FieldAsString("airp_arv");

  TTripInfo info;
  info.getByPointId( point_dep );

  TAirlinesRow &airline = (TAirlinesRow&)base_tables.get("airlines").get_row("code", info.airline);
  if (airline.code_lat.empty())
    throw Exception("airline.code_lat empty (code=%s)",airline.code.c_str());
  header = makeHeader( airline.code_lat );
  trans.init( false, (Qry.FieldIsNULL("refuse"))?ReqTypeCirq:ReqTypeCicx, getUserId( airline ) );
  int_flt.init( point_dep, FltTypeInt.first );

  /* Проверим транзит. В случае транзита через страну-участницу APPS, выставим
     флаг "transfer at destination". Это противоречит тому, что написано в
     спецификации, однако при проведении сертификации SITA потребовала именно
     так обрабатывать транзитные рейсы. */
  bool transit;
  checkAPPSSets( point_dep, airp_arv, transit );

  int transfer = transit?Dest:None;

  TCkinRoute tckin_route;
  int grp_id = Qry.FieldAsInteger("grp_id");

  // проверим исходящий трансфер
  TCkinRouteItem next;
  TCkinRoute().GetNextSeg(grp_id, crtIgnoreDependent, next );
  if ( !next.airp_arv.empty() ) {
    string country_arv = getCountryByAirp( airp_arv ).code_lat;
    if ( isAPPSCountry( country_arv, airline.code_lat ) &&
       ( getCountryByAirp( next.airp_arv ).code_lat != country_arv ) )
      transfer = Dest;
  }
  tckin_route.GetRouteBefore( grp_id, crtNotCurrent, crtIgnoreDependent );
  if ( !tckin_route.empty() ) {
    ckin_flt.init( tckin_route.front().point_dep, FltTypeChk.first );
    TCkinRouteItem prior;
    // проверим входящий трансфер
    prior = tckin_route.back();
    if ( !prior.airp_dep.empty() ) {
      string country_dep = getCountryByAirp(Qry.FieldAsString("airp_dep")).code_lat;
      if ( isAPPSCountry( country_dep, airline.code_lat ) &&
         ( getCountryByAirp(prior.airp_dep).code_lat != country_dep ) )
        transfer = ( ( transfer == Dest ) ? Both : Origin );
    }
  }
  // заполним информацию о пассажире
  string name = (!Qry.FieldIsNULL("name"))?Qry.FieldAsString("name"):"";
  TPaxStatus pax_status = DecodePaxStatus(Qry.FieldAsString("status"));
  pax.init( pax_id, Qry.FieldAsString("surname"), name, (pax_status==psCrew), transfer, override_type );
  return true;
}

bool TPaxRequest::getByCrsPaxId( const int pax_id, const std::string& override_type )
{
  ProgTrace(TRACE5, "TPaxRequest::getByCrsPaxId: %d", pax_id);
  TQuery Qry(&OraSession);
  Qry.SQLText="SELECT surname, name, point_id_spp, crs_pax.pnr_id, pr_del, airp_arv "
              "FROM crs_pax, crs_pnr, tlg_binding "
              "WHERE pax_id = :pax_id AND crs_pax.pnr_id = crs_pnr.pnr_id AND "
              "      crs_pnr.point_id = tlg_binding.point_id_tlg";
  Qry.CreateVariable("pax_id", otInteger, pax_id);
  Qry.Execute();

  if ( Qry.Eof )
    throw AstraLocale::UserException("MSG.PASSENGER.NOT_FOUND");

  int point_id = Qry.FieldAsInteger("point_id_spp");
  string airp_arv = Qry.FieldAsString("airp_arv");
  TTripInfo info;
  info.getByPointId( point_id );
  TAirlinesRow &airline = (TAirlinesRow&)base_tables.get("airlines").get_row("code", info.airline);
  if (airline.code_lat.empty())
    throw Exception("airline.code_lat empty (code=%s)",airline.code.c_str());
  header = makeHeader( airline.code_lat );
  trans.init( true, Qry.FieldAsInteger("pr_del")?ReqTypeCicx:ReqTypeCirq, getUserId( airline ) );
  int_flt.init( point_id, FltTypeInt.first );

  TQuery TrferQry( &OraSession );
  map<int, CheckIn::TTransferItem> trfer;
  CheckInInterface::GetOnwardCrsTransfer(Qry.FieldAsInteger("pnr_id"), TrferQry, info, airp_arv, trfer);

  /* Проверим транзит. В случае транзита через страну-участницу APPS, выставим
     флаг "transfer at destination". Это противоречит тому, что написано в
     спецификации, однако при проведении сертификации SITA потребовала именно
     так обрабатывать транзитные рейсы. */
  bool transit;
  checkAPPSSets( point_id, airp_arv, transit );

  int transfer = transit?Dest:None;

  if ( !trfer.empty() && !trfer[1].airp_arv.empty() ) {
    // сквозная регистрация
    string country_arv = getCountryByAirp(airp_arv).code_lat;
    if ( isAPPSCountry( country_arv, airline.code_lat ) &&
       ( getCountryByAirp( trfer[1].airp_arv ).code_lat != country_arv ) )
      transfer = Dest; // исходящий трансфер
  }
  // заполним информацию о пассажире
  string name = (!Qry.FieldIsNULL("name"))?Qry.FieldAsString("name"):"";
  pax.init( pax_id, Qry.FieldAsString("surname"), name, false, transfer, override_type );
  return true;
}

bool TPaxRequest::fromDBByPaxId( const int pax_id )
{
  // попытаемся найти пассажира среди отправленных
  TQuery Qry( &OraSession );
  Qry.SQLText = "SELECT * FROM "
                "(SELECT pax_id, apps_pax_id, status, pax_crew, "
                "        nationality, issuing_state, passport, check_char, doc_type, expiry_date, "
                "        sup_check_char, sup_doc_type, sup_passport, family_name, given_names, "
                "        date_of_birth, sex, birth_country, is_endorsee, transfer_at_orgn, "
                "        transfer_at_dest, pnr_source, pnr_locator, send_time, pre_ckin, "
                "        flt_num, dep_port, dep_date, arv_port, arv_date, ckin_flt_num, ckin_port, "
                "        point_id, ckin_point_id "
                "FROM apps_pax_data "
                "WHERE pax_id = :pax_id "
                "ORDER BY send_time DESC) "
                "WHERE rownum = 1";
  Qry.CreateVariable( "pax_id", otInteger, pax_id );
  Qry.Execute();

  if(Qry.Eof)
    return false;

  int point_id = Qry.FieldAsInteger("point_id");
  TTripInfo info;
  info.getByPointId( point_id );
  TAirlinesRow &airline = (TAirlinesRow&)base_tables.get("airlines").get_row("code", info.airline);
  if ( airline.code_lat.empty() )
    throw Exception("airline.code_lat empty (code=%s)",airline.code.c_str());
  header = makeHeader( airline.code_lat );
  trans.init( Qry.FieldAsInteger("pre_ckin"), ReqTypeCicx, getUserId( airline ) );
  int_flt.init( point_id, FltTypeInt.first, Qry.FieldAsString("flt_num"), Qry.FieldAsString("dep_port"),
                Qry.FieldAsString("arv_port"), Qry.FieldAsDateTime("dep_date"), Qry.FieldAsDateTime("arv_date") );
  if ( !Qry.FieldIsNULL("ckin_point_id") )
    ckin_flt.init( Qry.FieldAsInteger("ckin_point_id"), FltTypeChk.first, Qry.FieldAsString("ckin_flt_num"),
                   Qry.FieldAsString("ckin_port"), "", ASTRA::NoExists, ASTRA::NoExists );
  pax.init( Qry );
  return true;
}

bool TPaxRequest::fromDBByMsgId( const int msg_id )
{
  // попытаемся найти пассажира среди отправленных
  TQuery Qry( &OraSession );
  Qry.SQLText = "SELECT pax_id, apps_pax_id, status, pax_crew, "
                "       nationality, issuing_state, passport, check_char, doc_type, expiry_date, "
                "       sup_check_char, sup_doc_type, sup_passport, family_name, given_names, "
                "       date_of_birth, sex, birth_country, is_endorsee, transfer_at_orgn, "
                "       transfer_at_dest, pnr_source, pnr_locator, send_time, pre_ckin, "
                "       flt_num, dep_port, dep_date, arv_port, arv_date, ckin_flt_num, ckin_port, "
                "       point_id, ckin_point_id "
                "FROM apps_pax_data "
                "WHERE cirq_msg_id = :msg_id";

  Qry.CreateVariable( "msg_id", otInteger, msg_id );
  Qry.Execute();

  if(Qry.Eof)
    return false;

  int point_id = Qry.FieldAsInteger("point_id");
  TTripInfo info;
  info.getByPointId( point_id );
  TAirlinesRow &airline = (TAirlinesRow&)base_tables.get("airlines").get_row("code", info.airline);
  if (airline.code_lat.empty())
    throw Exception("airline.code_lat empty (code=%s)",airline.code.c_str());
  header = makeHeader( airline.code_lat );
  trans.init( Qry.FieldAsInteger("pre_ckin"), ReqTypeCicx, getUserId( airline ) );
  int_flt.init( point_id, FltTypeInt.first, Qry.FieldAsString("flt_num"), Qry.FieldAsString("dep_port"),
                Qry.FieldAsString("arv_port"), Qry.FieldAsDateTime("dep_date"), Qry.FieldAsDateTime("arv_date") );
  if ( !Qry.FieldIsNULL("ckin_point_id") )
    ckin_flt.init( Qry.FieldAsInteger("ckin_point_id"), FltTypeChk.first, Qry.FieldAsString("ckin_flt_num"),
                   Qry.FieldAsString("ckin_port"), "", ASTRA::NoExists, ASTRA::NoExists );
  pax.init( Qry );
  return true;
}

std::string TPaxRequest::msg() const
{
  std::ostringstream msg;
  msg << trans.msg() << "/" << int_flt.msg() << "/";
  if ( trans.code == ReqTypeCirq && ckin_flt.point_id != ASTRA::NoExists )
    msg << ckin_flt.msg() << "/";
  msg << pax.msg() << "/";
  return string(header + "\x02" + msg.str() + "\x03");
}

void TPaxRequest::saveData() const
{
  TQuery Qry(&OraSession);

  if( trans.code == ReqTypeCicx ) {
    Qry.SQLText = "UPDATE apps_pax_data SET cicx_msg_id = :cicx_msg_id "
                  "WHERE apps_pax_id = :apps_pax_id";
    Qry.CreateVariable("cicx_msg_id", otInteger, trans.msg_id);
    Qry.CreateVariable("apps_pax_id", otString, pax.apps_pax_id);
    Qry.Execute();

    return;
  }

  Qry.SQLText = "INSERT INTO apps_pax_data (pax_id, cirq_msg_id, pax_crew, nationality, issuing_state, "
                "                       passport, check_char, doc_type, expiry_date, sup_check_char, "
                "                       sup_doc_type, sup_passport, family_name, given_names, "
                "                       date_of_birth, sex, birth_country, is_endorsee, transfer_at_orgn, "
                "                       transfer_at_dest, pnr_source, pnr_locator, send_time, pre_ckin, "
                "                       flt_num, dep_port, dep_date, arv_port, arv_date, ckin_flt_num, ckin_port, "
                "                       point_id, ckin_point_id ) "
                "VALUES (:pax_id, :cirq_msg_id, :pax_crew, :nationality, :issuing_state, :passport, "
                "        :check_char, :doc_type, :expiry_date, :sup_check_char, :sup_doc_type, :sup_passport, "
                "        :family_name, :given_names, :date_of_birth, :sex, :birth_country, :is_endorsee, "
                "        :transfer_at_orgn, :transfer_at_dest, :pnr_source, :pnr_locator, :send_time, :pre_ckin, "
                "        :flt_num, :dep_port, :dep_date, :arv_port, :arv_date, :ckin_flt_num, :ckin_port, "
                "        :point_id, :ckin_point_id )";

  Qry.CreateVariable("cirq_msg_id", otInteger, trans.msg_id);
  Qry.CreateVariable("pax_id", otInteger, pax.pax_id);
  Qry.CreateVariable("pax_crew", otString, pax.pax_crew);
  Qry.CreateVariable("nationality", otString, pax.nationality);
  Qry.CreateVariable("issuing_state", otString, pax.issuing_state);
  Qry.CreateVariable("passport", otString,pax. passport);
  Qry.CreateVariable("check_char", otString, pax.check_char);
  Qry.CreateVariable("doc_type", otString, pax.doc_type);
  Qry.CreateVariable("expiry_date", otString, pax.expiry_date);
  Qry.CreateVariable("sup_check_char", otString, pax.sup_check_char);
  Qry.CreateVariable("sup_doc_type", otString, pax.sup_doc_type);
  Qry.CreateVariable("sup_passport", otString, pax.sup_passport);
  Qry.CreateVariable("family_name", otString, pax.family_name);
  Qry.CreateVariable("given_names", otString, pax.given_names);
  Qry.CreateVariable("date_of_birth", otString, pax.birth_date);
  Qry.CreateVariable("sex", otString, pax.sex);
  Qry.CreateVariable("birth_country", otString, pax.birth_country);
  Qry.CreateVariable("is_endorsee", otString, pax.endorsee);
  Qry.CreateVariable("transfer_at_orgn", otString, pax.trfer_at_origin);
  Qry.CreateVariable("transfer_at_dest", otString, pax.trfer_at_dest);
  Qry.CreateVariable("pnr_source", otString, pax.pnr_source);
  Qry.CreateVariable("pnr_locator", otString, pax.pnr_locator);
  Qry.CreateVariable("send_time", otDate, BASIC::NowUTC());
  Qry.CreateVariable("pre_ckin", otInteger, trans.type);
  Qry.CreateVariable("flt_num", otString, int_flt.flt_num);
  Qry.CreateVariable("dep_port", otString, int_flt.port);
  Qry.CreateVariable("dep_date", otDate, int_flt.date);
  Qry.CreateVariable("arv_port", otString, int_flt.arv_port);
  Qry.CreateVariable("arv_date", otDate, int_flt.arv_date);
  Qry.CreateVariable("ckin_flt_num", otString, ckin_flt.flt_num);
  Qry.CreateVariable("ckin_port", otString, ckin_flt.port);
  Qry.CreateVariable("point_id", otInteger, int_flt.point_id);
  if( ckin_flt.point_id != ASTRA::NoExists )
    Qry.CreateVariable("ckin_point_id", otInteger, ckin_flt.point_id);
  else
    Qry.CreateVariable("ckin_point_id", otInteger, FNull);
  Qry.Execute();
}

APPSAction TPaxRequest::typeOfAction( const bool is_exists, const std::string& status,
                                      const bool is_the_same, const bool is_forced) const
{
  bool is_cancel = (trans.code == ReqTypeCicx);
  if( !is_exists && is_cancel )
    return NoAction;
  if(!is_exists && !is_cancel)
    return NeedNew;
  if ( !status.empty() && is_cancel ) {
    deleteAPPSAlarms( pax.pax_id );
    if ( status != "B" ) {
      deleteAPPSData( pax.pax_id );
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
    set_pax_alarm( pax.pax_id, atAPPSConflict, true );
    set_crs_pax_alarm( pax.pax_id, atAPPSConflict, true );
    return NoAction;
  }
}

void TPaxRequest::sendReq() const
{
  saveData();
  sendNewReq( msg(), trans.msg_id, int_flt.point_id );
}

void TMftData::check_data() const
{
  if (country.empty())
    throw Exception("Empty country");
  if (mft_pax != "C")
    throw Exception("Incorrect mft_pax");
  if (mft_crew != "C")
    throw Exception("Incorrect mft_crew");
}

std::string TMftData::msg() const
{
  check_data();
  std::ostringstream msg;
  msg << MftReqMrq.first << '/' << MftReqMrq.second << '/'
      << country << '/' << mft_pax << '/' << mft_crew;
  return msg.str();
}

void TManifestRequest::init( const int point_id, const std::string& country ) {
  TTripInfo info;
  info.getByPointId( point_id );
  TAirlinesRow &airline = (TAirlinesRow&)base_tables.get("airlines").get_row("code", info.airline);
  if (airline.code_lat.empty())
    throw Exception("airline.code_lat empty (code=%s)",airline.code.c_str());
  header = makeHeader( airline.code_lat );
  trans.init( false, ReqTypeCimr, getUserId( airline ) );
  int_flt.init( point_id, FltTypeInm.first );
  mft_req.init( country );
}

std::string TManifestRequest::msg() const
{
  string msg = trans.msg() + "/" + int_flt.msg() + "/" + mft_req.msg() + "/";
  return string(header + "\x02" + msg + "\x03");
}

void TManifestRequest::sendReq() const
{
  sendNewReq( msg(), trans.msg_id, int_flt.point_id );
}

// APPS ANSWER ================================================================================================

static int getInt( const std::string& val )
{
  if ( val.empty() ) return ASTRA::NoExists;
  return ToInt( val );
}

void TAnsPaxData::init( std::string source )
{
  vector<string> tmp;
  boost::split( tmp, source, boost::is_any_of( "/" ) );
  vector<string>::const_iterator it = tmp.begin();
  string grp_id = *(it++);
  int flt_count = getInt( *(it++) );
  int seq_num = getInt( *(it++) );
  if ( grp_id != PaxAnsPrs.first && grp_id != PaxAnsPcc.first )
    throw Exception( "Incorrect grp_id: %s", grp_id.c_str() );
  if ( ( grp_id == PaxAnsPrs.first && flt_count != PaxAnsPrs.second ) ||
       ( grp_id == PaxAnsPcc.first && flt_count != PaxAnsPcc.second ) )
    throw Exception( "Incorrect flt_count: %d", flt_count );
  if( seq_num != 1 )
    throw Exception( "Incorrect seq_num: %d", seq_num );
  country = *(it++);
  vector<string>::const_reverse_iterator rit = tmp.rbegin();
  error_text3 = *(rit++);
  error_code3 = getInt(*(rit++));
  error_text2= *(rit++);
  error_code2 = getInt(*(rit++));
  error_text1 = *(rit++);
  error_code1 = getInt(*(rit++));
  if ( grp_id == PaxAnsPrs.first )
    apps_pax_id = *(rit++);
  status = *((*(rit++)).begin());
  code = getInt(*(rit++));
}

std::string TAnsPaxData::toString() const
{
  std::ostringstream res;
  res << "country: " << country << std::endl << "code: " << code
      << std::endl << "status: " << status << std::endl;
  if(!apps_pax_id.empty()) res << "apps_pax_id: " << apps_pax_id << std::endl;
  if(error_code1 != ASTRA::NoExists) res << "error_code1: " << error_code1 << std::endl;
  if(!error_text1.empty()) res << "error_text1: " << error_text1 << std::endl;
  if(error_code2 != ASTRA::NoExists) res << "error_code2: " << error_code2 << std::endl;
  if(!error_text2.empty()) res << "error_text2: " << error_text2 << std::endl;
  if(error_code3 != ASTRA::NoExists) res << "error_code3: " << error_code3 << std::endl;
  if(!error_text3.empty()) res << "error_text3: " << error_text3 << std::endl;
  return res.str();
}

void TAPPSAns::getLogParams( LEvntPrms& params, const std::string& country, const int status_code,
                const int error_code, const std::string& error_text ) const
{
  params << PrmLexema( "req_type", "MSG.APPS_" + code );

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

  if ( status_code != ASTRA::NoExists ) {
    // нужно как-то различать Timeout и Error condition. Для обоих status_code 0000
    if ( status_code == 0 )
      params << PrmLexema( "result", ( error_code != ASTRA::NoExists )?"MSG.APPS_STATUS_1111":"MSG.APPS_STATUS_2222" );
    else
      params << PrmLexema( "result", "MSG.APPS_STATUS_" + IntToString(status_code) );
  }
  else
    params << PrmSmpl<string>( "result", "" );

  if ( error_code != ASTRA::NoExists ) {
    string id = string( "MSG.APPS_" ) + IntToString(error_code);
    if ( AstraLocale::getLocaleText( id, AstraLocale::LANG_RU ) != id &&
         AstraLocale::getLocaleText( id, AstraLocale::LANG_EN ) != id ) {
      params << PrmLexema( "error", id );
    }
    else
      params << PrmSmpl<string>( "error", error_text );
  }
  else
    params << PrmSmpl<string>( "error", "" );
}

bool TAPPSAns::init( const std::string& trans_type, const std::string& source )
{
  vector<string> tmp;
  boost::split( tmp, source, boost::is_any_of( "/" ) );
  vector<string>::const_iterator it = tmp.begin();
  code = trans_type;
  msg_id = getInt( *(it++) );

  TQuery Qry(&OraSession);
  Qry.SQLText="SELECT send_attempts, msg_text, point_id "
              "FROM apps_messages "
              "WHERE msg_id = :msg_id";
  Qry.CreateVariable("msg_id", otInteger, msg_id);
  Qry.Execute();

  if(Qry.Eof)
    return false;

  point_id = Qry.FieldAsInteger("point_id");
  msg_text = Qry.FieldAsString("msg_text");
  send_attempts = Qry.FieldAsInteger("send_attempts");

  if( *(it++) == AnsErrCode ) {
    size_t fld_count = getInt( *(it++) );
    if ( fld_count != ( tmp.size() - 3) )
      throw Exception( "Incorrect fld_count: %d", fld_count );
    while(it < tmp.end()) {
      TError error;
      error.country = *(it++);
      error.error_code = getInt(*(it++));
      error.error_text = *(it++);
      errors.push_back(error);
    }
  }
  return true;
}

std::string TAPPSAns::toString() const
{
  std::ostringstream res;
  res << "code: " << code << std::endl << "msg_id: " << msg_id << std::endl
      << "point_id: " << point_id << std::endl << "msg_text: " << msg_text << std::endl
      << "send_attempts: " << send_attempts << std::endl;
  for ( vector<TError>::const_iterator it = errors.begin(); it < errors.end(); it++ ) {
    res << "country: " << it->country << std::endl << "error_code: " << it->error_code << std::endl
        << "error_text: " << it->error_text << std::endl;
  }
  return res.str();
}

void TAPPSAns::beforeProcessAnswer() const
{
  TFlights flightsForLock;
  flightsForLock.Get( point_id, ftTranzit );
  flightsForLock.Lock();

  // выключим тревогу "Нет связи с APPS"
  set_alarm( point_id, atAPPSOutage, false );
}

bool TAPPSAns::CheckIfNeedResend() const
{
  bool need_resend = false;
  for(std::vector<TError>::const_iterator it = errors.begin(); it < errors.end(); it ++) {
    if ( it->error_code == 6102 || it->error_code == 6900 || it->error_code == 6979 || it->error_code == 5057) {
      // ... Try again later
      need_resend = true;
    }
  }
  // Если нужно отправить повторно, не удаляем сообщение из apps_messages, чтобы resend_tlg его нашел
  if ( !need_resend )
    deleteMsg( msg_id );
  return need_resend;
}

bool TPaxReqAnswer::init( const std::string& code, const std::string& source )
{
  if( !TAPPSAns::init( code, source ) )
    return false;

  // попытаемся найти пассажира среди отправленных
  TQuery Qry( &OraSession );
  ostringstream sql;
  sql << "SELECT pax_id, family_name "
         "FROM apps_pax_data ";
  if ( code == AnsTypeCirs )
    sql << "WHERE cirq_msg_id = :msg_id";
  else
    sql << "WHERE cicx_msg_id = :msg_id";
  Qry.SQLText = sql.str();
  Qry.CreateVariable( "msg_id", otInteger, msg_id );
  Qry.Execute();

  if(Qry.Eof)
    return false;

  pax_id = Qry.FieldAsInteger( "pax_id" );
  family_name = Qry.FieldAsString( "family_name" );

  string delim = (code == AnsTypeCirs)?PaxAnsPrs.first:PaxAnsPcc.first;
  delim = string("/") + delim + string("/");
  std::size_t pos1 = source.find( delim );
  while ( pos1 != string::npos ) {
    std::size_t pos2 = source.find( delim, pos1 + delim.size() );
    TAnsPaxData data;
    data.init( source.substr( pos1 + 1, pos2 - pos1 ) );
    passengers.push_back( data );
    pos1 = pos2;
  }
  return true;
}

std::string TPaxReqAnswer::toString() const
{
  std::ostringstream res;
  res << TAPPSAns::toString()
      << "pax_id: " << pax_id << std::endl << "family_name: " << family_name << std::endl;
  for (std::vector<TAnsPaxData>::const_iterator it = passengers.begin(); it != passengers.end(); it++)
    res << it->toString();
  return res.str();
}

void TPaxReqAnswer::processErrors() const
{
  if( errors.empty() )
    return;

  // Сообщение не обработано системой. Разберем ошибки.
  for(std::vector<TError>::const_iterator it = errors.begin(); it < errors.end(); it ++)
    logAnswer( it->country, ASTRA::NoExists, it->error_code, it->error_text );

  if( /*!CheckIfNeedResend() &&*/ code == AnsTypeCirs ) {
    set_pax_alarm( pax_id, atAPPSConflict, true ); // рассинхронизация
    set_crs_pax_alarm( pax_id, atAPPSConflict, true ); // рассинхронизация
    // удаляем apps_pax_data cirq_msg_id
    TQuery Qry( &OraSession );
    Qry.SQLText = "DELETE FROM apps_pax_data WHERE cirq_msg_id = :cirq_msg_id ";
    Qry.CreateVariable( "cirq_msg_id", otInteger, msg_id );
    Qry.Execute();
  }
  deleteMsg( msg_id );
}

void TPaxReqAnswer::processAnswer() const
{
  if ( passengers.empty() )
    return;

  // Сообщение обработано системой. Каждому пассажиру присвоен статус.
  bool result = true;
  string status = "B";
  string apps_pax_id;
  for ( vector<TAnsPaxData>::const_iterator it = passengers.begin(); it < passengers.end(); it++ ) {
    // запишем в журнал операций присланный статус
    logAnswer( it->country, it->code, it->error_code1, it->error_text1 );
    if ( it->error_code2 != ASTRA::NoExists ) {
      logAnswer( it->country, it->code, it->error_code2, it->error_text2 );
    }
    if ( it->error_code3 != ASTRA::NoExists )
      logAnswer( it->country, it->code, it->error_code3, it->error_text3 );

    if ( code == AnsTypeCicc ) {
      // Ответ на отмену
      // Определим статус пассажира. true - отменен или не найден, false - ошибка.
      if( it->status == "E" || it->status == "T")
        result = false;
      continue;
    }

    // Ответ на запрос статуса
    if ( it != passengers.begin() ) {
      if ( apps_pax_id != it->apps_pax_id )
        throw Exception( "apps_pax_id is not the same" );
    }
    else
       apps_pax_id = it->apps_pax_id;
    // включим тревоги
    if (it->status == "D" || it->status == "X") {
      set_pax_alarm( pax_id, atAPPSNegativeDirective, true );
      set_crs_pax_alarm( pax_id, atAPPSNegativeDirective, true );
    }
    else if (it->status == "U" || it->status == "I" || it->status == "T" || it->status == "E") {
      set_pax_alarm( pax_id, atAPPSError, true );
      set_crs_pax_alarm( pax_id, atAPPSError, true );
    }
    /* Каждая страна участник APPS присылает свой статус пассажира, т.е.
     * один пассажир может иметь несколько статусов. Получим по каждому пассажиру
     * обобщенный статус для записи в apps_pax_data. Если все "B" -> "B", если хотя бы
     * один "X" -> "X", если хотябы один "U","I","T" или "E" -> "P" (Problem) */
    if ( status == "B" && it->status != "B" )
      status = (it->status == "B" || it->status == "X")?it->status:"P";
    else if (status == "P" && it->status == "X")
      status = "X";
  }
  TQuery Qry(&OraSession);
  if ( code == AnsTypeCicc ) {
    if( result ) {
    // Пассажир отменен. Удалим его из apps_pax_data
    Qry.Clear();
    Qry.SQLText = "BEGIN "
                  "SELECT send_time, pax_id INTO :send_time, :pax_id FROM apps_pax_data "
                  "WHERE cicx_msg_id = :cicx_msg_id; "
                  "DELETE FROM apps_pax_data WHERE send_time <= :send_time AND pax_id =:pax_id; "
                  "END;";
    Qry.CreateVariable( "cicx_msg_id", otInteger, msg_id );
    Qry.CreateVariable( "send_time", otDate, FNull );
    Qry.CreateVariable( "pax_id", otInteger, FNull );
    Qry.Execute();
    }
    deleteMsg( msg_id );
    return;
  }

  // сохраним полученный статус
  Qry.Clear();
  Qry.SQLText = "UPDATE apps_pax_data SET status = :status, apps_pax_id = :apps_pax_id "
                "WHERE cirq_msg_id = :cirq_msg_id";
  Qry.CreateVariable("cirq_msg_id", otInteger, msg_id );
  Qry.CreateVariable("status", otString, status);
  Qry.CreateVariable("apps_pax_id", otString, apps_pax_id);
  Qry.Execute();

  // погасим тревоги
  if ( status == "B" ) {
    set_pax_alarm( pax_id, atAPPSNegativeDirective, false );
    set_crs_pax_alarm( pax_id, atAPPSNegativeDirective, false );
    set_pax_alarm( pax_id, atAPPSError, false );
    set_crs_pax_alarm( pax_id, atAPPSError, false );
  }
  // проверим, нужно ли гасить тревогу "рассинхронизация"
  TPaxRequest * actual = new TPaxRequest();
  TPaxRequest * reseived = new TPaxRequest();
  actual->init( pax_id );
  reseived->fromDBByMsgId( msg_id );
  if ( *reseived != *actual ) {
    set_pax_alarm( pax_id, atAPPSConflict, false );
    set_crs_pax_alarm( pax_id, atAPPSConflict, false );
  }
  if ( actual ) delete actual;
  if ( reseived ) delete reseived;
  deleteMsg( msg_id );
}

void TPaxReqAnswer::logAnswer( const std::string& country, const int status_code,
                const int error_code, const std::string& error_text ) const
{
  LEvntPrms params;
  getLogParams( params, country, status_code, error_code, error_text );
  PrmLexema lexema( "passenger", "MSG.APPS_PASSENGER" );
  lexema.prms << PrmSmpl<string>( "passenger", family_name );
  params << lexema;

  TQuery Qry(&OraSession);
  Qry.SQLText = "SELECT pax.*, NULL As seat_no FROM pax WHERE pax_id=:pax_id";
  Qry.CreateVariable("pax_id", otInteger, pax_id);
  Qry.Execute();
  CheckIn::TSimplePaxItem pax;
  if (!Qry.Eof)
    pax.fromDB(Qry);
  TReqInfo::Instance()->LocaleToLog( "MSG.APPS_RESP", params, evtPax, point_id, pax.reg_no );
}

void TMftAnswer::logAnswer( const std::string& country, const int status_code,
                const int error_code, const std::string& error_text ) const
{
  LEvntPrms params;
  getLogParams( params, country, status_code, error_code, error_text );
  params << PrmSmpl<string>("passenger", "");
  TReqInfo::Instance()->LocaleToLog( "MSG.APPS_RESP", params, evtFlt, point_id );
}

bool TMftAnswer::init( const std::string& code, const std::string& source )
{
  if( !TAPPSAns::init( code, source) )
    return false;

  size_t pos = source.find(MftAnsMak.first);

  if ( pos != string::npos ) {
    vector<string> tmp;
    string text = source.substr( pos );
    boost::split( tmp, text, boost::is_any_of( "/" ) );
    vector<string>::const_iterator it = tmp.begin() + 1; // пропустим grp_id
    int flt_count = getInt( *(it++) );
    if ( flt_count != MftAnsMak.second )
      throw Exception( "Incorrect flt_count: %d", flt_count );

    country = *(it++);
    resp_code = getInt(*(it++));
    error_code = getInt(*(it++));
    error_text = *(it++);
  }
  return true;
}

void TMftAnswer::processErrors() const
{
  if(errors.empty())
    return;

  // Сообщение не обработано системой. Разберем ошибки.
  for(std::vector<TError>::const_iterator it = errors.begin(); it < errors.end(); it ++)
    logAnswer( it->country, ASTRA::NoExists, it->error_code, it->error_text );
  // CheckIfNeedResend();
  deleteMsg( msg_id );
}

void TMftAnswer::processAnswer() const
{
  if( resp_code == ASTRA::NoExists  )
    return;

  // Сообщение обработано системой. Залогируем ответ и удалим сообщение из apps_messages.
  logAnswer( country, resp_code, error_code, error_text );
  deleteMsg( msg_id );
}

std::string TMftAnswer::toString() const
{
  std::ostringstream res;
  res << TAPPSAns::toString()
      << "country: " << country << std::endl << "resp_code: " << resp_code << std::endl
      << "error_code: " << error_code << std::endl << "error_text: " << error_text << std::endl;
  return res.str();
}

std::string getAnsText( const std::string& tlg )
{
  string::size_type pos1 = tlg.find("\x02");
  string::size_type pos2 = tlg.find("\x03");
  if ( pos1 == std::string::npos )
    throw Exception( "Wrong answer format" );
  return tlg.substr( pos1 + 1, pos2 - pos1 - 1 );
}

bool processReply( const std::string& source )
{
  if(source.empty())
    throw Exception("Answer is empty");

  string code = source.substr(0, 4);
  string answer = source.substr(5, source.size() - 6); // отрезаем код транзакции и замыкающий '/' (XXXX:text_to_parse/)
  TAPPSAns * res = NULL;
  if ( code == AnsTypeCirs || code == AnsTypeCicc )
    res = new TPaxReqAnswer();
  else if ( code == AnsTypeCima ) {
    res = new TMftAnswer();
  }
  else
    throw Exception( std::string( "Unknown transaction code: " + code ) );

  if (!res->init( code, answer ))
    return false;
  ProgTrace( TRACE5, "Result: %s", res->toString().c_str() );
  res->beforeProcessAnswer();
  res->processErrors();
  res->processAnswer();
  if ( res ) delete res;
  return true;
}

void processPax( const int pax_id, const std::string& override_type, const bool is_forced )
{
  ProgTrace( TRACE5, "processPax: %d", pax_id );
  TPaxRequest * new_data = new TPaxRequest();
  new_data->init( pax_id, override_type );
  TPaxRequest * actual_data = new TPaxRequest();
  bool is_exists = actual_data->fromDBByPaxId( pax_id );
  APPSAction action = new_data->typeOfAction( is_exists, actual_data->getStatus(),
                                             ( *new_data == *actual_data ), is_forced );
  if (action == NoAction)
    return;
  if ( action == NeedCancel || action == NeedUpdate )
    actual_data->sendReq();
  if ( action == NeedUpdate || action == NeedNew )
    new_data->sendReq();
  if ( new_data ) delete new_data;
  if ( actual_data ) delete actual_data;
}

std::set<std::string> needFltCloseout( const std::set<std::string>& countries, const std::string airline )
{
  set<string> countries_need_req;

  if(countries.size() > 1) {
    // не отправляем для местных рейсов

    TQuery AppsSetsQry( &OraSession );
    AppsSetsQry.Clear();
    AppsSetsQry.SQLText=
      "SELECT flt_closeout "
      "FROM apps_sets "
      "WHERE airline=:airline AND apps_country=:apps_country AND pr_denial=0";
    AppsSetsQry.CreateVariable( "airline", otString, airline );
    AppsSetsQry.DeclareVariable( "apps_country", otString );

    for (set<string>::const_iterator it = countries.begin(); it != countries.end(); it++) {
      AppsSetsQry.SetVariable( "apps_country", *it );
      AppsSetsQry.Execute();
      if (!AppsSetsQry.Eof && AppsSetsQry.FieldAsInteger("flt_closeout"))
        countries_need_req.insert(*it);
    }
  }
  return countries_need_req;
}

void APPSFlightCloseout( const int point_id )
{
  ProgTrace(TRACE5, "APPSFlightCloseout: point_id = %d", point_id);
  if ( !checkTime( point_id ) )
    return;

  TAdvTripRoute route;
  route.GetRouteAfter( NoExists, point_id, trtWithCurrent, trtNotCancelled );
  TAdvTripRoute::const_iterator r=route.begin();
  set<string> countries;
  countries.insert( getCountryByAirp(r->airp).code_lat );
  for(r++; r!=route.end(); r++) {
    // определим, нужно ли отправлять данные
    if( !checkAPPSSets( point_id, r->point_id ) )
      continue;
    countries.insert( getCountryByAirp(r->airp).code_lat );

    /* Определим, есть ли пассажиры не прошедшие посадку,
     * информация о которых была отправлена в SITA.
     * Для таких пассажиров нужно послать отмену */
    TQuery Qry(&OraSession);
    Qry.Clear();
    Qry.SQLText="SELECT cirq_msg_id, pax_grp.status "
                "FROM pax_grp, pax, apps_pax_data "
                "WHERE pax_grp.grp_id=pax.grp_id AND apps_pax_data.pax_id = pax.pax_id AND "
                "      pax_grp.point_dep=:point_dep AND pax_grp.point_arv=:point_arv AND "
                "      (pax.name IS NULL OR pax.name<>'CBBG') AND "
                "      pr_brd=0 AND apps_pax_data.status = 'B'";
    Qry.CreateVariable("point_dep", otInteger, point_id);
    Qry.CreateVariable("point_arv", otInteger, r->point_id);
    Qry.Execute();

    for( ; !Qry.Eof; Qry.Next() ) {
      TPaxRequest * pax = new TPaxRequest();
      pax->fromDBByMsgId( Qry.FieldAsInteger("cirq_msg_id") );
      if( pax->getStatus() != "B" )
        continue; // CICX request has already been send
      pax->sendReq();
      if ( pax ) delete pax;
    }
  }
  set<string> countries_need_req = needFltCloseout( countries, route.front().airline );
  for( set<string>::const_iterator it = countries_need_req.begin(); it != countries_need_req.end(); it++ ) {
    TManifestRequest close_flt;
    close_flt.init( point_id, *it );
    close_flt.sendReq();
  }
}

bool IsAPPSAnswText(const std::string& tlg_body)
{
  return ( ( tlg_body.find( AnsTypeCirs + ":" ) != string::npos ) ||
           ( tlg_body.find( AnsTypeCicc + ":" ) != string::npos ) ||
           ( tlg_body.find( AnsTypeCima + ":" ) != string::npos ) );
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
      return (string("/8501/B/") + IntToString( getIdent() ) + string("///////"));
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

std::string emulateAnswer( const std::string& request )
{
  ostringstream answer;
  string code = request.substr(0, ReqTypeCirq.size());

  boost::regex pattern("(?<=:).*?(?=/)");
  boost::smatch result;
  if (!boost::regex_search(request, result, pattern))
    throw EXCEPTIONS::Exception( "emulateAnswer: msg_id oot found" );
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

      string seq_num = pax.substr( 0, pax.find( "/" ) );
      size_t pos = (code == ReqTypeCirq)?0:pax.find("/");
      string pax_info = pax.substr( pax.find( "/", pos + 1 ) + 1 );
      answer << ans_info.first << "/" << ans_info.second << "/" << seq_num << "/AE/" << pax_info;
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

void reSendMsg( const int send_attempts, const std::string& msg_text, const int msg_id )
{
  sendTlg( APPSAddr, OWN_CANON_NAME(), qpOutApp, 20, msg_text,
          ASTRA::NoExists, ASTRA::NoExists );

//  sendCmd("CMD_APPS_ANSWER_EMUL","H");
  sendCmd("CMD_APPS_HANDLER","H");

  TQuery Qry(&OraSession);
  Qry.SQLText = "UPDATE apps_messages "
                "SET send_attempts = :send_attempts, send_time = :send_time "
                "WHERE msg_id = :msg_id ";
  Qry.CreateVariable("msg_id", otInteger, msg_id);
  Qry.CreateVariable("send_attempts", otInteger, send_attempts + 1);
  Qry.CreateVariable("send_time", otDate, BASIC::NowUTC());
  Qry.Execute();
  ProgTrace(TRACE5, "Message id=%d was re-sent. Send attempts: %d", msg_id, send_attempts);
}

void deleteMsg( const int msg_id )
{
  TQuery Qry(&OraSession);
  Qry.SQLText = "DELETE FROM apps_messages "
                "WHERE msg_id = :msg_id ";
  Qry.CreateVariable("msg_id", otInteger, msg_id);
  Qry.Execute();
}

CheckIn::TPaxRemItem getAPPSRem( const int pax_id )
{
  TPaxRequest * apps_pax = new TPaxRequest();
  apps_pax->fromDBByPaxId( pax_id );
  string status = apps_pax->getStatus();
  CheckIn::TPaxRemItem rem;
  if( status == "B" )
    rem.code = "SBIA";
  else if( status == "P" )
    rem.code = "SPIA";
  else if( status == "X" )
    rem.code = "SXIA";
  if ( !rem.code.empty() ) {
    TCkinRemTypesRow &ckin_rem = (TCkinRemTypesRow&)base_tables.get("CKIN_REM_TYPES").get_row("code",rem.code);
    rem.text = ( TReqInfo::Instance()->desk.lang == AstraLocale::LANG_RU ) ? ckin_rem.name : ckin_rem.name_lat;
  }
  if ( apps_pax ) delete apps_pax;
  return rem;
}

bool isAPPSRem( const std::string& rem )
{
  return ( rem == "RSIA" || rem == "SPIA" || rem == "SBIA" || rem == "SXIA" ||
           rem == "ATH" || rem == "GTH" || rem == "AAE" || rem == "GAE" );
}

static void sendAPPSInfo( const int point_id, const int point_id_tlg )
{
  ProgTrace( TRACE5, "sendAPPSInfo: point_id %d, point_id_tlg: %d", point_id, point_id_tlg );
  TQuery Qry( &OraSession );
  Qry.Clear();
  Qry.SQLText =
    "SELECT pax_id, airp_arv FROM crs_pnr, crs_pax "
    "WHERE crs_pnr.pnr_id=crs_pax.pnr_id AND point_id=:point_id_tlg";
  Qry.CreateVariable( "point_id_tlg", otInteger, point_id_tlg );
  Qry.Execute();

  TFlights flightsForLock;
  flightsForLock.Get( point_id, ftTranzit );
  flightsForLock.Lock();

  for ( ; !Qry.Eof; Qry.Next() )
    if ( checkAPPSSets( point_id, Qry.FieldAsString( "airp_arv" ) ) )
      processPax( Qry.FieldAsInteger( "pax_id" ) );
}

void sendAllAPPSInfo( const int point_id, const std::string& task_name, const std::string& params )
{
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText =
    "SELECT point_id_tlg "
    "FROM tlg_binding, points "
    "WHERE tlg_binding.point_id_spp=points.point_id AND "
    "      point_id_spp=:point_id_spp AND "
    "      points.pr_del=0 AND points.pr_reg<>0";
  Qry.CreateVariable("point_id_spp", otInteger, point_id);
  Qry.Execute();

  for( ; !Qry.Eof; Qry.Next() )
    sendAPPSInfo( point_id, Qry.FieldAsInteger("point_id_tlg") );
}

void sendNewAPPSInfo( const int point_id, const std::string& task_name, const std::string& params )
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

  TFlights flightsForLock;
  flightsForLock.Get( point_id, ftTranzit );
  flightsForLock.Lock();

  for( ; !Qry.Eof; Qry.Next() ) {
    int pax_id = Qry.FieldAsInteger( "pax_id" );
    processPax( pax_id );

    TCachedQuery Qry( "UPDATE crs_pax SET need_apps=0 WHERE pax_id=:pax_id",
                      QParams() << QParam( "pax_id", otInteger, pax_id ) );
    Qry.get().Execute();
  }
}
