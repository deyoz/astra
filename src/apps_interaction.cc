#include <boost/algorithm/string.hpp>
#include <ctime>
#include <deque>
#include <boost/regex.hpp>
#include <sstream>
#include "alarms.h"
#include "apps_interaction.h"
#include "astra_misc.h"
#include "exceptions.h"
#include "points.h"
#include "qrys.h"
#include "trip_tasks.h"
#include "tlg/tlg.h"
#include "tlg/paxlst_request.h"
#include <boost/scoped_ptr.hpp>
#include "apis_utils.h"

#include "apis_edi_file.h"
#include "apis_creator.h"

#include <serverlib/str_utils.h>
#include <serverlib/tcl_utils.h>
#include <edilib/edi_user_func.h>
#include <edilib/edi_except.h>

#define NICKNAME "ANNA"
#define NICKTRACE SYSTEM_TRACE
#include <serverlib/slogger.h>

using namespace std;
using namespace EXCEPTIONS;
using namespace BASIC::date_time;
using namespace ASTRA;

using namespace edilib;

// реализованы версии сообщений 21 и 26

// static const std::string ReqTypeCirq = "CIRQ";
// static const std::string ReqTypeCicx = "CICX";
// static const std::string ReqTypeCimr = "CIMR";

// static const std::string AnsTypeCirs = "CIRS";
// static const std::string AnsTypeCicc = "CICC";
// static const std::string AnsTypeCima = "CIMA";

// static const std::pair<std::string, int> FltTypeChk("CHK", 2);
// static const std::pair<std::string, int> FltTypeInt("INT", 8);
// static const std::pair<std::string, int> FltTypeExo("EXO", 4);
// static const std::pair<std::string, int> FltTypeExd("EXD", 4);
// static const std::pair<std::string, int> FltTypeInm("INM", 3);

// static const std::pair<std::string, int> PaxReqPrq("PRQ", 22);
// static const std::pair<std::string, int> PaxReqPcx("PCX", 20);

// static const std::pair<std::string, int> PaxAnsPrs("PRS", 27);
// static const std::pair<std::string, int> PaxAnsPcc("PCC", 26);

// static const std::pair<std::string, int> MftReqMrq("MRQ", 3);
// static const std::pair<std::string, int> MftAnsMak("MAK", 4);

// static const std::string AnsErrCode = "ERR";

// static const std::string APPSFormat = "APPS_FMT";

class AppsPaxNotFoundException : public std::exception {} ;

// версия спецификации документа
enum ESpecVer { SPEC_6_76, SPEC_6_83 };

ESpecVer GetSpecVer(int msg_ver)
{
  if (msg_ver == APPS_VERSION_21)
    return SPEC_6_76;
  else
    return SPEC_6_83;
}

const int pax_seq_num = 1;

static const int MaxCirqPaxNum = 5;
static const int MaxCicxPaxNum = 10;

enum { None, Origin, Dest, Both };

int FieldCount( string data_group, int version )
{
  // независимые от версии
  if (data_group == "CHK") return 2;
  if (data_group == "INT") return 8;
  if (data_group == "INM") return 3;
  if (data_group == "MRQ") return 3;
  if (data_group == "PCC") return 26;
  if (data_group == "EXO") return 4;
  if (data_group == "EXD") return 4;
  if (data_group == "MAK") return 4;
  // зависимые от версии
  if (version == APPS_VERSION_21)
  {
    if (data_group == "PRQ") return 22;
    if (data_group == "PCX") return 20;
    if (data_group == "PRS") return 27;
    if (data_group == "PAD") return 0;
  }
  if (version >= APPS_VERSION_26) // FIXME временное решение для Китая - было "==" а стало ">="
  {
    if (data_group == "PRQ") return 34;
    if (data_group == "PCX") return 21; // NOTE в спецификации нет версии 26 для этой группы данных
    if (data_group == "PRS") return 29;
    if (data_group == "PAD") return 13;
  }
  throw Exception("unsupported field count, version: %d, data group: %s", version, data_group.c_str());
  return 0;
}

//-----------------------------------------------------------------------------------

class EdiSessionHandler
{
  _edi_mes_head_* mhead = nullptr;
  EDI_MSG_TYPE* pEType = nullptr;
  EdiSessWrData* EdiSess = nullptr;
public:
  EdiSessionHandler()
  {
    mhead = new edi_mes_head;
    memset ( mhead,0, sizeof ( edi_mes_head ) );
    mhead->msg_type = PAXLST;

    pEType =  GetEdiMsgTypeStrByType_ ( GetEdiTemplateMessages(), mhead->msg_type ) ;
    if ( !pEType )
    {
        LogError ( STDLOG ) << "No types defined for type " << mhead->msg_type;
        throw edilib::EdiExcept ( std::string ( "No types defined for type " ) +
                                    boost::lexical_cast<std::string> ( mhead->msg_type ) );
    }

    mhead->msg_type_req = pEType->query_type;
    mhead->answer_type  = pEType->answer_type;
    mhead->msg_type_str = pEType;
    strcpy ( mhead->code,       pEType->code );
    strcpy ( mhead->unh_number, "11085B94E1F8FA" );

    EdiSess = new AstraEdiSessWR("MOVGRG",
                                 mhead,
                                 Ticketing::RemoteSystemContext::IapiSystemContext::read());
  }
  ~EdiSessionHandler()
  {
    EdiSess->ediSession()->CommitEdiSession();
    delete EdiSess;
    delete mhead;
  }
};

//-----------------------------------------------------------------------------------

TAppsSets::TAppsSets(const std::string& airline, const std::string& country)
  : _airline(airline), _country(country)
{
}

void TAppsSets::init_qry(TQuery& AppsSetsQry, const std::string& select_string)
{
  AppsSetsQry.Clear();
  AppsSetsQry.SQLText =
  string("SELECT ") + select_string + string(" ") +
  string("FROM apps_sets ") +
  string("WHERE airline=:airline AND apps_country=:apps_country AND pr_denial=0");
  AppsSetsQry.CreateVariable( "airline", otString, _airline );
  AppsSetsQry.CreateVariable( "apps_country", otString, _country );
}

bool TAppsSets::get_country()
{
  TQuery AppsSetsQry( &OraSession );
  init_qry(AppsSetsQry, "apps_country");
  AppsSetsQry.Execute();
  if (AppsSetsQry.Eof)
    return false;
  else
    return true;
}

bool TAppsSets::get_inbound_outbound(int& inbound, int& outbound)
{
  TQuery AppsSetsQry( &OraSession );
  init_qry(AppsSetsQry, "inbound, outbound");
  AppsSetsQry.Execute();
  if (AppsSetsQry.Eof)
    return false;
  else
  {
    inbound = AppsSetsQry.FieldAsInteger("inbound");
    outbound = AppsSetsQry.FieldAsInteger("outbound");
    return true;
  }
}

bool TAppsSets::get_flt_closeout(int& flt_closeout)
{
  TQuery AppsSetsQry( &OraSession );
  init_qry(AppsSetsQry, "flt_closeout");
  AppsSetsQry.Execute();
  if (AppsSetsQry.Eof)
    return false;
  else
  {
    flt_closeout = AppsSetsQry.FieldAsInteger("flt_closeout");
    return true;
  }
}

std::string TAppsSets::get_format()
{
  TQuery AppsSetsQry( &OraSession );
  init_qry(AppsSetsQry, "format");
  AppsSetsQry.Execute();
  if (AppsSetsQry.Eof || AppsSetsQry.FieldIsNULL("format"))
    return APPS_FORMAT_21;
  else
    return AppsSetsQry.FieldAsString("format");
}

int TAppsSets::get_version()
{
  string fmt = get_format();
  if (fmt == APPS_FORMAT_21) return APPS_VERSION_21;
  if (fmt == APPS_FORMAT_26) return APPS_VERSION_26;
  if (fmt == APPS_FORMAT_CHINA) return APPS_VERSION_CHINA;
  throw Exception("cannot get version, format %s", fmt.c_str());
  return 0;
}

int GetVersion(string airline, string country)
{
  TAppsSets sets(airline, country);
  return sets.get_version();
}

static std::string getH2HReceiver()
{
  static string VAR;
  if ( VAR.empty() )
    VAR=getTCLParam( "APPS_H2H_ADDR", NULL );
  return VAR;
}

static std::string getH2HTpr(int msgId)
{
    const size_t TprLen = 4;

    std::string res = StrUtils::ToBase36Lpad(msgId, TprLen);
    if(res.length() > TprLen)
        res = res.substr(res.length() - TprLen, TprLen);
    return res;
}

static std::string makeHeader( const std::string& airline, int msgId, const int version )
{
  if (version == APPS_VERSION_CHINA)
    return string( string("V.\rVHLG.WA/E11HCNIAPIQ") + "/I11HCNIAPIR" + "/P" + getH2HTpr(msgId) + "\rVGZ.\rV" + airline + "/MOW/////////RU\r" );
  else
    return string( "V.\rVHLG.WA/E5" + string(OWN_CANON_NAME()) + "/I5" + getH2HReceiver() + "/P" + getH2HTpr(msgId) + "\rVGZ.\rV" + airline + "/MOW/////////RU\r" );
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
  TAppsSets sets(airline, country);
  return sets.get_country();
}

static void saveAppsMessage(const std::string& text,
                            const int msg_id,
                            const int point_id,
                            int version)
{
    TQuery Qry( &OraSession );
    Qry.SQLText = "INSERT INTO apps_messages(msg_id, msg_text, send_attempts, send_time, point_id, version) "
                  "VALUES (:msg_id, :msg_text, :send_attempts, :send_time, :point_id, :version)";
    Qry.CreateVariable("msg_id", otString, msg_id);
    Qry.CreateVariable("send_time", otDate, NowUTC());
    Qry.CreateVariable("msg_text", otString, text);
    Qry.CreateVariable("send_attempts", otInteger, 1);
    Qry.CreateVariable("point_id", otInteger, point_id);
    Qry.CreateVariable("version", otInteger, version);
    Qry.Execute();

    ProgTrace(TRACE5, "New APPS request generated: %s", text.c_str());
}

static void sendNewReq( const std::string& text, const int msg_id, const int point_id, const int version )
{
    sendTlg(getAPPSRotName(), OWN_CANON_NAME(), qpOutApp, 20, text,
            ASTRA::NoExists, ASTRA::NoExists);
    sendCmd("CMD_APPS_HANDLER","H");
    saveAppsMessage(text, msg_id, point_id, version);
}

static void sendNewReq( const TPaxRequest& paxReq, const int msg_id, const int point_id, int version )
{
    // отправим телеграмму
    std::string text;
    using namespace edifact;
    if (version == APPS_VERSION_CHINA)
    {
        PaxlstRequest ediReq(PaxlstReqParams("", paxReq.toPaxlst()));
        ediReq.sendTlg();
        text = ediReq.tlgOut()->text();
    }
    else
        sendTlg( getAPPSRotName(), OWN_CANON_NAME(), qpOutApp, 20, text,
                 ASTRA::NoExists, ASTRA::NoExists );

    sendCmd("CMD_APPS_HANDLER","H");

    saveAppsMessage(text, msg_id, point_id, version);
}

const char* getAPPSRotName()
{
  static string VAR;
  if ( VAR.empty() )
    VAR=getTCLParam( "APPS_ROT_NAME", NULL );
  return VAR.c_str();
}

const std::string getIAPIRemEdiAddr()
{
    static const std::string remEdiAddr = readStringFromTcl("IAPI_REM_EDI_ADDR", "NIAC");
    return remEdiAddr;
}

const std::string getIAPIOurEdiAddr()
{
    static const std::string ourEdiAddr = readStringFromTcl("IAPI_OUR_EDI_ADDR", "NORDWIND");
    return ourEdiAddr;
}

const std::string getIAPIEdiProfileName()
{
    static const std::string ediProfileName = readStringFromTcl("IAPI_EDI_PROFILE_NAME", "IAPI");
    return ediProfileName;
}

bool checkAPPSSets( const int point_dep, const int point_arv )
{
  bool result = false;
  TAdvTripRoute route;
  route.GetRouteAfter(NoExists, point_dep, trtWithCurrent, trtNotCancelled);

  for ( TAdvTripRoute::const_iterator r = route.begin(); r != route.end(); r++ )
  {
    TAppsSets sets(route.front().airline, getCountryByAirp( r->airp ).code);
    int inbound, outbound;
    bool not_eof = sets.get_inbound_outbound(inbound, outbound);
    if (not_eof &&
      !( r->point_id == point_dep && !outbound ) &&
      !( r->point_id == point_arv && !inbound ) )
      result = true;
    if ( r->point_id == point_arv )
      return result;
  }
  ProgTrace( TRACE5, "Point %d was not found in the flight route !!!", point_arv );
  return false;
}

bool checkAPPSSets( const int point_dep, const std::string& airp_arv, set<string>* pFormats )
{
  bool transit;
  return checkAPPSSets( point_dep, airp_arv, transit, pFormats );
}

bool checkAPPSSets( const int point_dep, const std::string& airp_arv, bool& transit, set<string>* pFormats )
{
  bool result = false;
  transit = false;
  TAdvTripRoute route;
  route.GetRouteAfter(NoExists, point_dep, trtWithCurrent, trtNotCancelled);

  if ( route.empty() )
  {
    //нормальныя ситуация, когда маршрут полностью отменен
    ProgTrace( TRACE5, "Empty route!!! (point_dep=%d)", point_dep );
    return false;
  }

  for ( TAdvTripRoute::const_iterator r = route.begin(); r != route.end(); r++ )
  {
    TAppsSets sets(route.front().airline, getCountryByAirp( r->airp ).code);
    int inbound, outbound;
    bool not_eof = sets.get_inbound_outbound(inbound, outbound);
    if (not_eof)
    {
      if ( r->airp != airp_arv && r->point_id != point_dep )
        transit = true;
      else if ( !( r->airp == airp_arv && !inbound ) &&
                !( r->point_id == point_dep && !outbound ) )
      {
        if (pFormats!=nullptr) pFormats->insert(sets.get_format());
        result = true;
      }
    }
    if ( r->airp == airp_arv )
      return result;
  }
  ProgTrace( TRACE5, "Airport %s was not found in the flight route !!! (point_dep=%d)", airp_arv.c_str(), point_dep );
  return false;
}

bool checkTime( const int point_id )
{
  TDateTime start_time = ASTRA::NoExists;
  return checkTime( point_id, start_time );
}

bool checkTime( const int point_id, TDateTime& start_time )
{
  start_time = ASTRA::NoExists;
  TDateTime now = NowUTC();
  TTripInfo trip;
  if (not trip.getByPointId(point_id))
  {
    ProgTrace(TRACE5, "getByPointId returned false, point_id=%d", point_id);
    return false;
  }
  // The APP System only allows transactions on [- 2 days] TODAY [+ 10 days].
  if ( ( now - trip.scd_out ) > 2 )
    return false;
  if ( ( trip.scd_out - now ) > 10 ) {
    start_time = trip.scd_out - 10;
    return false;
  }
  return true;
}

void deleteAPPSData( const int pax_id )
{
  TQuery Qry(&OraSession);
  Qry.SQLText = "DELETE FROM apps_pax_data WHERE pax_id = :pax_id";
  Qry.CreateVariable("pax_id", otString, pax_id);
  Qry.Execute();
}

static void syncAPPSAlarms(const int pax_id, const int point_id_spp)
{
  if (point_id_spp!=ASTRA::NoExists)
    TTripAlarmHook::set(Alarm::APPSProblem, point_id_spp);
  else
  {
    if (pax_id!=ASTRA::NoExists)
    {
      TPaxAlarmHook::set(Alarm::APPSProblem, pax_id);
      TCrsPaxAlarmHook::set(Alarm::APPSProblem, pax_id);
    }
  }
}

static void addAPPSAlarm( const int pax_id,
                          const std::initializer_list<Alarm::Enum>& alarms,
                          const int point_id_spp=ASTRA::NoExists )
{
  if (!addAlarmByPaxId(pax_id, alarms, {paxCheckIn, paxPnl})) return; //ничего не изменилось
  syncAPPSAlarms(pax_id, point_id_spp);
}

static void deleteAPPSAlarm( const int pax_id,
                             const std::initializer_list<Alarm::Enum>& alarms,
                             const int point_id_spp=ASTRA::NoExists )
{
  if (!deleteAlarmByPaxId(pax_id, alarms, {paxCheckIn, paxPnl})) return; //ничего не изменилось
  syncAPPSAlarms(pax_id, point_id_spp);
}

void deleteAPPSAlarms( const int pax_id, const int point_id_spp )
{
  if (!deleteAlarmByPaxId(pax_id,
                          {Alarm::APPSNegativeDirective,
                           Alarm::APPSError,
                           Alarm::APPSConflict},
                          {paxCheckIn, paxPnl})) return; //ничего не изменилось

  if (point_id_spp!=ASTRA::NoExists)
    syncAPPSAlarms(pax_id, point_id_spp);
  else
    LogError(STDLOG) << __func__ << ": point_id_spp==ASTRA::NoExists";
}

void TTransData::init(const bool pre_ckin, const std::string& trans_code,
                      const TAirlinesRow& airline, int ver)
{
  code = trans_code;
  msg_id = getIdent();
  user_id = getUserId(airline);
  type = pre_ckin;
  version = ver;
  header = makeHeader(airline.code_lat, msg_id, version);
}

void TTransData::check_data() const
{
  if (code != "CIRQ" && code != "CICX" && code != "CIMR")
    throw Exception("Incorrect transacion code");
  if (user_id.empty() || user_id.size() > 6)
    throw Exception("Incorrect User ID");
}

std::string TTransData::msg() const // TODO уточнить зависимость от версии
{
  check_data();
  std::ostringstream msg;
  /* 1 */ msg << code << ':';
  /* 2 */ msg << msg_id << '/';
  /* 3 */ msg << user_id << '/';
  if (code == "CIRQ" || code == "CICX")
  {
    /* 4 */ msg << "N" << '/';
    /* 5 */ msg << ( type ? "P" : "" ) << '/';
    /* 6 */ msg << version;
  }
  else if (code == "CIMR")
  {
    /* 4 */ msg << version;
  }
  return msg.str();
}

void TFlightData::init( const int id, const std::string& flt_type, int ver )
{
  ProgTrace( TRACE5, "TFlightData::init: %d", id );
  if ( id == ASTRA::NoExists )
    return;
  point_id = id;
  type = flt_type;
  TAdvTripRoute route, tmp;
  route.GetRouteBefore(NoExists, point_id, trtWithCurrent, trtNotCancelled);
  tmp.GetRouteAfter(NoExists, point_id, trtNotCurrent, trtNotCancelled);
  route.insert(route.end(), tmp.begin(), tmp.end());

  if( route.empty() )
    throw Exception( "Empty route, point_id %d", point_id );

  const TAirlinesRow &airline = (const TAirlinesRow&)base_tables.get("airlines").get_row("code", route.front().airline);
  if (airline.code_lat.empty()) throw Exception("airline.code_lat empty (code=%s)",airline.code.c_str());
  string suffix;
  if (!route.front().suffix.empty()) {
    const TTripSuffixesRow &suffixRow = (const TTripSuffixesRow&)base_tables.get("trip_suffixes").get_row("code", route.front().suffix);
    if (suffixRow.code_lat.empty()) throw Exception("suffixRow.code_lat empty (code=%s)",suffixRow.code.c_str());
    suffix=suffixRow.code_lat;
  }
  ostringstream trip;
  trip << airline.code_lat
       << setw(3) << setfill('0') << route.front().flt_num
       << route.front().suffix;

  flt_num = trip.str();

  const TAirpsRow &airp_dep = (const TAirpsRow&)base_tables.get("airps").get_row("code", route.front().airp);
  if (airp_dep.code_lat.empty()) throw Exception("airp_dep.code_lat empty (code=%s)",airp_dep.code.c_str());
  port = airp_dep.code_lat;

  if( type != "CHK" && route.front().scd_out != ASTRA::NoExists )
    date = UTCToLocal( route.front().scd_out, CityTZRegion(airp_dep.city) );

  if( type == "INT" ) {
  const TAirpsRow &airp_arv = (const TAirpsRow&)base_tables.get("airps").get_row("code", route.back().airp);
  if (airp_arv.code_lat.empty()) throw Exception("airp_arv.code_lat empty (code=%s)",airp_arv.code.c_str());
  arv_port = airp_arv.code_lat;

  if( route.back().scd_in != ASTRA::NoExists )
    arv_date = UTCToLocal( route.back().scd_in, CityTZRegion( airp_arv.city ) );
  }
  version = ver;
}

void TFlightData::init( const int id, const std::string& flt_type, const std::string&  num, const std::string& airp, const std::string& arv_airp,
           TDateTime dep, TDateTime arv, int ver )
{
  point_id = id;
  type = flt_type;
  flt_num = num;
  port = airp;
  arv_port = arv_airp;
  date = dep;
  arv_date = arv;
  version = ver;
}

void TFlightData::check_data() const
{
  if( type != "CHK" && type != "INM" && type != "INT" )
    throw Exception( "Incorrect type of flight: %s", type.c_str() );
  if ( flt_num.empty() || flt_num.size() > 8 )
    throw Exception(" Incorrect flt_num: %s", flt_num.c_str() );
  if ( port.empty() || port.size() > 5 )
    throw Exception( "Incorrect airport: %s", port.c_str() );
  if ( type != "CHK" && date == ASTRA::NoExists )
    throw Exception( "Date empty" );
  if ( type == "INT" ) {
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
  /* 1  */ msg << type << '/';
  /* 2  */ msg << FieldCount(type, version) << '/';
  if (type == "CHK")
  {
    /* 3  */ msg << port << '/';
    /* 4  */ msg << flt_num;
  }
  else if ( type == "INT" )
  {
    /* 3  */ msg << "S" << '/';
    /* 4  */ msg << flt_num << '/';
    /* 5  */ msg << port << '/';
    /* 6  */ msg << arv_port << '/';
    /* 7  */ msg << DateTimeToStr( date, "yyyymmdd" ) << '/';
    /* 8  */ msg << DateTimeToStr( date, "hhnnss" ) << '/';
    /* 9  */ msg << DateTimeToStr( arv_date, "yyyymmdd" ) << '/';
    /* 10 */ msg << DateTimeToStr( arv_date, "hhnnss" );
  }
  else if ( type == "INM" )
  {
    /* 3  */ msg << flt_num << '/';
    /* 4  */ msg << port << '/';
    /* 5  */ msg << DateTimeToStr( date, "yyyymmdd" );
  }
  return msg.str();
}

void TPaxData::init( const int pax_ident, const std::string& surname, const std::string name,
                     const bool is_crew, const int transfer, const std::string& override_type,
                     const int reg_no, const ASTRA::TTrickyGender::Enum tricky_gender, int ver )
{
  pax_id = pax_ident;
  CheckIn::TPaxDocItem doc;
  if ( !CheckIn::LoadPaxDoc( pax_ident, doc ) )
    CheckIn::LoadCrsPaxDoc( pax_ident, doc );
  pax_crew = is_crew?"C":"P";
  nationality = doc.nationality;
  issuing_state = doc.issue_country;
  passport = doc.no.substr(0, 14);
  switch (GetSpecVer(ver))
  {
  case SPEC_6_76:
    doc_type = (doc.type == "P" || doc.type.empty())?"P":"O";
    break;
  case SPEC_6_83:
    if (doc.type == "P" || doc.type == "I")
      doc_type = doc.type;
    else if (doc.type.empty())
      doc_type = "P";
    else
      doc_type = "O";
    break;
  default:
    throw Exception( "Unknown specification version" );
    break;
  }
  doc_subtype = doc.subtype;
  if ( doc.expiry_date != ASTRA::NoExists )
    expiry_date = DateTimeToStr( doc.expiry_date, "yyyymmdd" );
  if( !doc.surname.empty() ) {
    family_name = transliter(doc.surname, 1, 1);
    if (!doc.first_name.empty())
      given_names = doc.first_name;
    if (!given_names.empty() && !doc.second_name.empty())
      given_names = given_names + " " + doc.second_name;
    given_names = given_names.substr(0, 40);
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
  if(!override_type.empty())
    override_codes = override_type;
  // passenger reference
  int seq_num = (reg_no == ASTRA::NoExists) ? 0 : reg_no;
  int pass_desc;
  switch (tricky_gender)
  {
    case ASTRA::TTrickyGender::Male:    pass_desc = 1; break;
    case ASTRA::TTrickyGender::Female:  pass_desc = 2; break;
    case ASTRA::TTrickyGender::Child:   pass_desc = 3; break;
    case ASTRA::TTrickyGender::Infant:  pass_desc = 4; break;
    default: pass_desc = 8; break;
  }
  ostringstream ref;
  ref << setfill('0') << setw(4) << seq_num << pass_desc;
  reference = ref.str();
  version = ver;
}

void TPaxData::init( TQuery &Qry, int ver )
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
  doc_subtype = Qry.FieldAsString("doc_subtype");
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
  if (!Qry.FieldIsNULL("pass_ref"))
    reference = Qry.FieldAsString("pass_ref");
  version = ver;
}

void TPaxData::check_data() const
{
  if ( pax_id == ASTRA::NoExists )
    throw Exception( "Empty pax_id" );
  if ( pax_crew != "C" && pax_crew != "P" )
    throw Exception( "Incorrect pax_crew %s", pax_crew.c_str() );
  if( passport.size() > 14)
    throw Exception( "Passport number too long: %s", passport.c_str() );
  if( !doc_type.empty() && doc_type != "P" && doc_type != "O" && doc_type != "N" && doc_type != "I" )
    throw Exception( "Incorrect doc_type: %s", doc_type.c_str() );
  if( family_name.empty() || family_name.size() > 40 )
    throw Exception( "Incorrect family_name: %s", family_name.c_str() );
  if( given_names.size() > 40 )
    throw Exception( "given_names too long: %s", given_names.c_str() );
  if( !sex.empty() && sex != "M" && sex != "F" && sex != "U" && sex != "X" )
    throw Exception( "Incorrect gender: %s", sex.c_str() );
  if( !endorsee.empty() && endorsee != "S" )
    throw Exception( "Incorrect endorsee: %s", endorsee.c_str() );
  if (reference.size() > 5)
    throw Exception( "reference too long: %s", reference.c_str() );
}

std::string TPaxData::msg() const
{
  check_data();
  std::ostringstream msg;
  string data_group;
  if (apps_pax_id.empty())
  {
    /* PRQ */
    data_group = "PRQ";
    /* 1  */ msg << data_group << '/';
    /* 2  */ msg << FieldCount(data_group, version) << '/';
    /* 3  */ msg << pax_seq_num << '/';
    /* 4  */ msg << pax_crew << '/';
    /* 5  */ msg << nationality << '/';
    /* 6  */ msg << issuing_state << '/';
    /* 7  */ msg << passport << '/';
    /* 8  */ msg << check_char << '/';
    /* 9  */ msg << doc_type << '/';
    if (version >= APPS_VERSION_26)
    {
      /* 10 */ msg << doc_subtype << '/';
    }
    /* 11 */ msg << expiry_date << '/';
    /* 12 */ // reserved for version 27
    /* 13 */ msg << sup_doc_type << '/';
    /* 14 */ msg << sup_passport << '/';
    /* 15 */ msg << sup_check_char << '/';
    /* 16 */ msg << family_name << '/';
    /* 17 */ msg << (given_names.empty() ? "-" : given_names) << '/';
    /* 18 */ msg << birth_date << '/';
    /* 19 */ msg << sex << '/';
    /* 20 */ msg << birth_country << '/';
    /* 21 */ msg << endorsee << '/';
    /* 22 */ msg << trfer_at_origin << '/';
    /* 23 */ msg << trfer_at_dest << '/';
    /* 24 */ msg << override_codes << '/';
    /* 25 */ msg << pnr_source << '/';
    /* 26 */ msg << pnr_locator;
    if (version >= APPS_VERSION_26)
    {
      msg << '/';
      /* 27 */ msg << '/';
      /* 28 */ msg << '/';
      /* 29 */ msg << '/';
      /* 30 */ msg << reference;
    }
    if (version >= APPS_VERSION_26)
    {
      msg << '/';
      /* 31 */ msg << '/';
      /* 32 */ msg << '/';
      /* 33 */ msg << '/';
      /* 34 */ msg << '/';
      /* 35 */ msg << '/';
      /* 36 */ msg << '/';
      /* 37 */ msg << "";
    }
  }
  else
  {
    /* PCX */
    data_group = "PCX";
    /* 1  */ msg << data_group << '/';
    /* 2  */ msg << FieldCount(data_group, version) << '/';
    /* 3  */ msg << pax_seq_num << '/';
    /* 4  */ msg << apps_pax_id << '/';
    /* 5  */ msg << pax_crew << '/';
    /* 6  */ msg << nationality << '/';
    /* 7  */ msg << issuing_state << '/';
    /* 8  */ msg << passport << '/';
    /* 9  */ msg << check_char << '/';
    /* 10 */ msg << doc_type << '/';
    /* 11 */ msg << expiry_date << '/';
    /* 12 */ msg << sup_doc_type << '/';
    /* 13 */ msg << sup_passport << '/';
    /* 14 */ msg << sup_check_char << '/';
    /* 15 */ msg << family_name << '/';
    /* 16 */ msg << (given_names.empty() ? "-" : given_names) << '/';
    /* 17 */ msg << birth_date << '/';
    /* 18 */ msg << sex << '/';
    /* 19 */ msg << birth_country << '/';
    /* 20 */ msg << endorsee << '/';
    /* 21 */ msg << trfer_at_origin << '/';
    /* 22 */ msg << trfer_at_dest;
    if (version >= APPS_VERSION_26)
    {
      msg << '/';
      /* 23 */ msg << reference;
    }
  }
  return msg.str();
}

string IssuePlaceToCountry(const string& issue_place)
{
  if (issue_place.empty())
    return issue_place;
  string country = SubstrAfterLastSpace(issue_place);
  TElemFmt elem_fmt;
  string country_id = ElemToPaxDocCountryId(upperc(country), elem_fmt);
  if (elem_fmt != efmtUnknown)
    return country_id;
  else
    throw Exception("IssuePlaceToCountry failed: issue_place=\"%s\"", issue_place.c_str());
}

void TPaxAddData::init( const int pax_id, const int ver )
{
  ProgTrace(TRACE5, "TPaxAddData::init: %d", pax_id);
  version = ver;

  CheckIn::TPaxDocoItem doco;
  if ( !CheckIn::LoadPaxDoco( pax_id, doco ) )
    CheckIn::LoadCrsPaxVisa( pax_id, doco );

  CheckIn::TPaxDocaItem docaD;
  if ( !CheckIn::LoadPaxDoca( pax_id, CheckIn::docaDestination, docaD ) )
  {
    CheckIn::TDocaMap doca_map;
    CheckIn::LoadCrsPaxDoca( pax_id, doca_map );
    docaD = doca_map[apiDocaD];
  }

  if (!doco.applic_country.empty())
  {
    string country_code = ((const TPaxDocCountriesRow&)base_tables.get("pax_doc_countries").get_row("code", doco.applic_country)).country;
    country_for_data = country_code.empty() ? "" : ((const TCountriesRow&)base_tables.get("countries").get_row("code", country_code)).code_lat;
  }
  doco_type = doco.type;
  doco_no = doco.no.substr(0, 20); // в БД doco.no VARCHAR2(25 BYTE)
  country_issuance = IssuePlaceToCountry(doco.issue_place);

  if (doco.expiry_date != ASTRA::NoExists)
    doco_expiry_date = DateTimeToStr( doco.expiry_date, "yyyymmdd" );

  num_street = docaD.address;
  city = docaD.city;
  state = docaD.region.substr(0, 20); // уточнить
  postal_code = docaD.postal_code;

  redress_number = "";
  traveller_number = "";
}

void TPaxAddData::init( TQuery &Qry, int ver )
{
  country_for_data = Qry.FieldAsString("country_for_data");
  doco_type = Qry.FieldAsString("doco_type");
  doco_no = Qry.FieldAsString("doco_no");
  country_issuance = IssuePlaceToCountry(Qry.FieldAsString("country_issuance"));
  doco_expiry_date = Qry.FieldAsString("doco_expiry_date");
  num_street = Qry.FieldAsString("num_street");
  city = Qry.FieldAsString("city");
  state = Qry.FieldAsString("state");
  postal_code = Qry.FieldAsString("postal_code");
  redress_number = Qry.FieldAsString("redress_number");
  traveller_number = Qry.FieldAsString("traveller_number");
  version = ver;
}

void TPaxAddData::check_data() const
{
  if (country_for_data.size() > 2)
    throw Exception( "country_for_data too long: %s", country_for_data.c_str() );
  if (doco_type.size() > 2)
    throw Exception( "doco_type too long: %s", doco_type.c_str() );
  if (doco_no.size() > 20)
    throw Exception( "doco_no too long: %s", doco_no.c_str() );
  if (country_issuance.size() > 3)
    throw Exception( "country_issuance too long: %s", country_issuance.c_str() );
  if (doco_expiry_date.size() > 8)
    throw Exception( "doco_expiry_date too long: %s", doco_expiry_date.c_str() );
  if (num_street.size() > 60)
    throw Exception( "num_street too long: %s", num_street.c_str() );
  if (city.size() > 60)
    throw Exception( "city too long: %s", city.c_str() );
  if (state.size() > 20)
    throw Exception( "state too long: %s", state.c_str() );
  if (postal_code.size() > 20)
    throw Exception( "postal_code too long: %s", postal_code.c_str() );
  if (redress_number.size() > 13)
    throw Exception( "redress_number too long: %s", redress_number.c_str() );
  if (traveller_number.size() > 25)
    throw Exception( "traveller_number too long: %s", traveller_number.c_str() );
}

std::string TPaxAddData::msg() const
{
  check_data();
  std::ostringstream msg;
  /* 1  */ msg << "PAD" << '/';
  /* 2  */ msg << FieldCount("PAD", version) << '/';
  /* 3  */ msg << pax_seq_num << '/'; // Passenger Sequence Number
  /* 4  */ msg << country_for_data << '/';
  /* 5  */ msg << doco_type << '/';
  /* 6  */ msg << '/'; // subtype пока не используется
  /* 7  */ msg << doco_no << '/';
  /* 8  */ msg << country_issuance << '/';
  /* 9  */ msg << doco_expiry_date << '/';
  /* 10 */ // reserved for version 27
  /* 11 */ msg << num_street << '/';
  /* 12 */ msg << city << '/';
  /* 13 */ msg << state << '/';
  /* 14 */ msg << postal_code << '/';
  /* 15 */ msg << redress_number << '/';
  /* 16 */ msg << traveller_number;
  return msg.str();
}

void TPaxRequest::init(const int pax_id, Timing::Points &timing, const std::string& override_type )
{
  if ( !getByPaxId( pax_id, timing, override_type ) )
    getByCrsPaxId( pax_id, timing, override_type );
}

bool TPaxRequest::getByPaxId( const int pax_id, Timing::Points& timing, const std::string& override_type )
{
  ProgTrace( TRACE5, "TPaxRequest::getByPaxId: %d", pax_id );
  timing.start("getByPaxId");
  timing.start("getByPaxId Qry.Execute");
  TQuery Qry( &OraSession );
  Qry.SQLText="SELECT surname, name, pax.grp_id, status, point_dep, refuse, airp_dep, airp_arv "
              ", pers_type, is_female, reg_no "
              "FROM pax_grp, pax "
              "WHERE pax_id = :pax_id AND pax_grp.grp_id = pax.grp_id";
  Qry.CreateVariable( "pax_id", otInteger, pax_id );
  Qry.Execute();
  timing.finish("getByPaxId Qry.Execute");

  if(Qry.Eof)
  {
    timing.finish("getByPaxId");
    return false;
  }

  int point_dep = Qry.FieldAsInteger("point_dep");
  string airp_arv = Qry.FieldAsString("airp_arv");

  timing.start("getByPaxId getByPointId");
  TTripInfo info;
  if (not info.getByPointId( point_dep ))
  {
    ProgTrace(TRACE5, "getByPointId returned false, point_id=%d, pax_id=%d", point_dep, pax_id);
    timing.finish("getByPaxId getByPointId");
    timing.finish("getByPaxId");
    return false;
  }
  timing.finish("getByPaxId getByPointId");

  const TAirlinesRow &airline = (const TAirlinesRow&)base_tables.get("airlines").get_row("code", info.airline);
  if (airline.code_lat.empty())
  {
    timing.finish("getByPaxId");
    throw Exception("airline.code_lat empty (code=%s)",airline.code.c_str());
  }
  timing.start("getByPaxId version, header");
  version = GetVersion(airline.code, getCountryByAirp( airp_arv ).code);
  timing.finish("getByPaxId version, header");
  timing.start("getByPaxId trans.init");
  trans.init( false, (Qry.FieldIsNULL("refuse"))?"CIRQ":"CICX", airline, version );
  timing.finish("getByPaxId trans.init");
  timing.start("getByPaxId int_flt.init");
  int_flt.init( point_dep, "INT", version );
  timing.finish("getByPaxId int_flt.init");

  /* Проверим транзит. В случае транзита через страну-участницу APPS, выставим
     флаг "transfer at destination". Это противоречит тому, что написано в
     спецификации, однако при проведении сертификации SITA потребовала именно
     так обрабатывать транзитные рейсы. */
  timing.start("getByPaxId transfer");
  bool transit;
  checkAPPSSets( point_dep, airp_arv, transit );

  int transfer = transit?Dest:None;

  TCkinRoute tckin_route;
  int grp_id = Qry.FieldAsInteger("grp_id");

  // проверим исходящий трансфер
  TCkinRouteItem next;
  TCkinRoute().GetNextSeg(grp_id, crtIgnoreDependent, next );
  if ( !next.airp_arv.empty() )
  {
    string country_arv = getCountryByAirp( airp_arv ).code;
    if ( isAPPSCountry( country_arv, airline.code ) &&
       ( getCountryByAirp( next.airp_arv ).code != country_arv ) )
      transfer = Dest;
  }
  tckin_route.GetRouteBefore( grp_id, crtNotCurrent, crtIgnoreDependent );
  if ( !tckin_route.empty() )
  {
    timing.start("getByPaxId ckin_flt.init");
    ckin_flt.init( tckin_route.front().point_dep, "CHK", version );
    timing.finish("getByPaxId ckin_flt.init");
    TCkinRouteItem prior;
    // проверим входящий трансфер
    prior = tckin_route.back();
    if ( !prior.airp_dep.empty() )
    {
      string country_dep = getCountryByAirp(Qry.FieldAsString("airp_dep")).code;
      if ( isAPPSCountry( country_dep, airline.code ) &&
         ( getCountryByAirp(prior.airp_dep).code != country_dep ) )
        transfer = ( ( transfer == Dest ) ? Both : Origin );
    }
  }
  timing.finish("getByPaxId transfer");
  // заполним информацию о пассажире
  timing.start("getByPaxId name, status, gender");
  string name = (!Qry.FieldIsNULL("name"))?Qry.FieldAsString("name"):"";
  TPaxStatus pax_status = DecodePaxStatus(Qry.FieldAsString("status"));
  ASTRA::TTrickyGender::Enum tricky_gender = CheckIn::TSimplePaxItem::getTrickyGender(
    DecodePerson(Qry.FieldAsString("pers_type")), CheckIn::TSimplePaxItem::genderFromDB(Qry) );
  timing.finish("getByPaxId name, status, gender");
  timing.start("getByPaxId pax.init");
  pax.init( pax_id, Qry.FieldAsString("surname"), name, (pax_status==psCrew), transfer, override_type,
    Qry.FieldAsInteger("reg_no"), tricky_gender, version );
  timing.finish("getByPaxId pax.init");
  if(version >= APPS_VERSION_26)
  {
    timing.start("getByPaxId pax_add.init");
    pax_add.init(pax_id, version);
    timing.finish("getByPaxId pax_add.init");
  }
  timing.finish("getByPaxId");
  return true;
}

bool TPaxRequest::getByCrsPaxId( const int pax_id, Timing::Points& timing, const std::string& override_type )
{
  timing.start("getByCrsPaxId");
  ProgTrace(TRACE5, "TPaxRequest::getByCrsPaxId: %d", pax_id);
  timing.start("getByCrsPaxId Qry.Execute");
  TQuery Qry(&OraSession);
  Qry.SQLText="SELECT surname, name, point_id_spp, crs_pax.pnr_id, pr_del, airp_arv "
              ", pers_type "
              "FROM crs_pax, crs_pnr, tlg_binding "
              "WHERE pax_id = :pax_id AND crs_pax.pnr_id = crs_pnr.pnr_id AND "
              "      crs_pnr.point_id = tlg_binding.point_id_tlg";
  Qry.CreateVariable("pax_id", otInteger, pax_id);
  Qry.Execute();
  timing.finish("getByCrsPaxId Qry.Execute");

  if ( Qry.Eof )
  {
    timing.finish("getByCrsPaxId");
    throw AppsPaxNotFoundException();
  }

  int point_id = Qry.FieldAsInteger("point_id_spp");
  string airp_arv = Qry.FieldAsString("airp_arv");
  timing.start("getByCrsPaxId getByPointId");
  TTripInfo info;
  if (not info.getByPointId( point_id ))
  {
    timing.finish("getByCrsPaxId getByPointId");
    timing.finish("getByCrsPaxId");
    throw Exception("getByPointId returned false, point_id=%d, pax_id=%d (getByCrsPaxId)", point_id, pax_id);
  }
  timing.finish("getByCrsPaxId getByPointId");

  const TAirlinesRow &airline = (const TAirlinesRow&)base_tables.get("airlines").get_row("code", info.airline);
  if (airline.code_lat.empty())
  {
    timing.finish("getByCrsPaxId");
    throw Exception("airline.code_lat empty (code=%s)",airline.code.c_str());
  }
  timing.start("getByCrsPaxId version, header");
  version = GetVersion(airline.code, getCountryByAirp( airp_arv ).code);
  timing.finish("getByCrsPaxId version, header");
  timing.start("getByCrsPaxId trans.init");
  trans.init( true, Qry.FieldAsInteger("pr_del")?"CICX":"CIRQ", airline, version );
  timing.finish("getByCrsPaxId trans.init");
  timing.start("getByCrsPaxId int_flt.init");
  int_flt.init( point_id, "INT", version );
  timing.finish("getByCrsPaxId int_flt.init");

  timing.start("getByCrsPaxId transfer");
  map<int, CheckIn::TTransferItem> trfer;
  CheckInInterface::GetOnwardCrsTransfer(Qry.FieldAsInteger("pnr_id"), true, info, airp_arv, trfer);

  /* Проверим транзит. В случае транзита через страну-участницу APPS, выставим
     флаг "transfer at destination". Это противоречит тому, что написано в
     спецификации, однако при проведении сертификации SITA потребовала именно
     так обрабатывать транзитные рейсы. */
  bool transit;
  checkAPPSSets( point_id, airp_arv, transit );

  int transfer = transit?Dest:None;

  if ( !trfer.empty() && !trfer[1].airp_arv.empty() )
  {
    // сквозная регистрация
    string country_arv = getCountryByAirp(airp_arv).code;
    if ( isAPPSCountry( country_arv, airline.code ) &&
       ( getCountryByAirp( trfer[1].airp_arv ).code != country_arv ) )
      transfer = Dest; // исходящий трансфер
  }
  timing.finish("getByCrsPaxId transfer");
  // заполним информацию о пассажире
  timing.start("getByCrsPaxId name, gender");
  string name = (!Qry.FieldIsNULL("name"))?Qry.FieldAsString("name"):"";
  ASTRA::TTrickyGender::Enum tricky_gender = CheckIn::TSimplePaxItem::getTrickyGender(
    DecodePerson(Qry.FieldAsString("pers_type")), TGender::Unknown );
  timing.finish("getByCrsPaxId name, gender");
  timing.start("getByCrsPaxId pax.init");
  pax.init( pax_id, Qry.FieldAsString("surname"), name, false, transfer, override_type,
    ASTRA::NoExists, tricky_gender, version );
  timing.finish("getByCrsPaxId pax.init");
  if(version >= APPS_VERSION_26)
  {
    timing.start("getByCrsPaxId pax_add.init");
    pax_add.init(pax_id, version);
    timing.finish("getByCrsPaxId pax_add.init");
  }
  timing.finish("getByCrsPaxId");
  return true;
}

bool TPaxRequest::fromDBByPaxId( const int pax_id )
{
  ProgTrace(TRACE5, "TPaxRequest::fromDBByPaxId: %d", pax_id);
  // попытаемся найти пассажира среди отправленных
  TQuery Qry( &OraSession );
  Qry.SQLText = "SELECT * FROM "
                "(SELECT pax_id, apps_pax_id, status, pax_crew, "
                "        nationality, issuing_state, passport, check_char, doc_type, doc_subtype, expiry_date, "
                "        sup_check_char, sup_doc_type, sup_passport, family_name, given_names, "
                "        date_of_birth, sex, birth_country, is_endorsee, transfer_at_orgn, "
                "        transfer_at_dest, pnr_source, pnr_locator, send_time, pre_ckin, "
                "        flt_num, dep_port, dep_date, arv_port, arv_date, ckin_flt_num, ckin_port, "
                "        point_id, ckin_point_id, pass_ref, version, "
                " country_for_data, doco_type, doco_no, country_issuance, doco_expiry_date, "
                " num_street, city, state, postal_code, redress_number, traveller_number "
                "FROM apps_pax_data "
                "WHERE pax_id = :pax_id "
                "ORDER BY send_time DESC) "
                "WHERE rownum = 1"; // TODO
  Qry.CreateVariable( "pax_id", otInteger, pax_id );
  Qry.Execute();

  if(Qry.Eof)
    return false;

  int point_id = Qry.FieldAsInteger("point_id");
  version = Qry.FieldIsNULL("version")? APPS_VERSION_21: Qry.FieldAsInteger("version");
  TTripInfo info;
  if (not info.getByPointId( point_id ))
  {
    ProgTrace(TRACE5, "getByPointId returned false, point_id=%d, pax_id=%d", point_id, pax_id);
    return false; // или удалить пакса?
  }
  const TAirlinesRow &airline = (const TAirlinesRow&)base_tables.get("airlines").get_row("code", info.airline);
  if ( airline.code_lat.empty() )
    throw Exception("airline.code_lat empty (code=%s)",airline.code.c_str());
  trans.init( Qry.FieldAsInteger("pre_ckin"), "CICX", airline, version );
  int_flt.init( point_id, "INT", Qry.FieldAsString("flt_num"), Qry.FieldAsString("dep_port"),
                Qry.FieldAsString("arv_port"), Qry.FieldAsDateTime("dep_date"), Qry.FieldAsDateTime("arv_date"), version );
  if ( !Qry.FieldIsNULL("ckin_point_id") )
    ckin_flt.init( Qry.FieldAsInteger("ckin_point_id"), "CHK", Qry.FieldAsString("ckin_flt_num"),
                   Qry.FieldAsString("ckin_port"), "", ASTRA::NoExists, ASTRA::NoExists, version );
  pax.init( Qry, version );
  if(version >= APPS_VERSION_26)
    pax_add.init(Qry, version);
  return true;
}

bool TPaxRequest::fromDBByMsgId( const int msg_id )
{
  ProgTrace(TRACE5, "TPaxRequest::fromDBByMsgId: %d", msg_id);
  // попытаемся найти пассажира среди отправленных
  TQuery Qry( &OraSession );
  Qry.SQLText = "SELECT pax_id, apps_pax_id, status, pax_crew, "
                "       nationality, issuing_state, passport, check_char, doc_type, doc_subtype, expiry_date, "
                "       sup_check_char, sup_doc_type, sup_passport, family_name, given_names, "
                "       date_of_birth, sex, birth_country, is_endorsee, transfer_at_orgn, "
                "       transfer_at_dest, pnr_source, pnr_locator, send_time, pre_ckin, "
                "       flt_num, dep_port, dep_date, arv_port, arv_date, ckin_flt_num, ckin_port, "
                "       point_id, ckin_point_id, pass_ref, version, "
                " country_for_data, doco_type, doco_no, country_issuance, doco_expiry_date, "
                " num_street, city, state, postal_code, redress_number, traveller_number "
                "FROM apps_pax_data "
                "WHERE cirq_msg_id = :msg_id";

  Qry.CreateVariable( "msg_id", otInteger, msg_id );
  Qry.Execute();

  if(Qry.Eof)
    return false;

  int point_id = Qry.FieldAsInteger("point_id");
  version = Qry.FieldIsNULL("version")? APPS_VERSION_21: Qry.FieldAsInteger("version");
  TTripInfo info;
  if (not info.getByPointId( point_id ))
  {
    ProgTrace(TRACE5, "getByPointId returned false, point_id=%d, msg_id=%d", point_id, msg_id);
    return false;
  }
  const TAirlinesRow &airline = (const TAirlinesRow&)base_tables.get("airlines").get_row("code", info.airline);
  if (airline.code_lat.empty())
    throw Exception("airline.code_lat empty (code=%s)",airline.code.c_str());
  trans.init( Qry.FieldAsInteger("pre_ckin"), "CICX", airline, version );
  int_flt.init( point_id, "INT", Qry.FieldAsString("flt_num"), Qry.FieldAsString("dep_port"),
                Qry.FieldAsString("arv_port"), Qry.FieldAsDateTime("dep_date"), Qry.FieldAsDateTime("arv_date"), version );
  if ( !Qry.FieldIsNULL("ckin_point_id") )
    ckin_flt.init( Qry.FieldAsInteger("ckin_point_id"), "CHK", Qry.FieldAsString("ckin_flt_num"),
                   Qry.FieldAsString("ckin_port"), "", ASTRA::NoExists, ASTRA::NoExists, version );
  pax.init( Qry, version );
  if(version >= APPS_VERSION_26)
    pax_add.init(Qry, version);
  return true;
}

std::string TPaxRequest::msg() const
{
  std::ostringstream msg;
  if (version == APPS_VERSION_CHINA)
  {
    msg << trans.header << msg_china_iapi();
    return msg.str();
  }
  else
  {
    msg << trans.msg() << "/";
    if ( trans.code == "CIRQ" && ckin_flt.point_id != ASTRA::NoExists )
    {
      msg << ckin_flt.msg() << "/";
    }
    msg << int_flt.msg() << "/";
    msg << pax.msg() << "/";
    if ( trans.code == "CIRQ" && version >= APPS_VERSION_26 && !pax_add.country_for_data.empty() )
    {
      msg << pax_add.msg() << "/";
    }
    return string(trans.header + "\x02" + msg.str() + "\x03");
  }
}

//-----------------------------------------------------------------------------------

string get_airp_code_lat(const string& airp_code)
{
  return ((const TAirpsRow&)base_tables.get("airps").get_row("code",airp_code)).code_lat;
}

TDateTime get_scd_local(const TDateTime& scd_utc, string airp_code)
{
  return UTCToLocal(scd_utc, AirpTZRegion(airp_code));
}

class DepArr
{
  bool valid_ = false;
public:
  string airp125;
  TDateTime time189 = NoExists;
  string airp87;
  TDateTime time232 = NoExists;
  string airpCBP;
  bool valid() { return valid_; }
  void validate()
  {
    time189 = get_scd_local(time189, airp125);
    time232 = get_scd_local(time232, airp87);
    airp125 = get_airp_code_lat(airp125);
    airp87 = get_airp_code_lat(airp87);
    airpCBP = get_airp_code_lat(airpCBP);
    valid_ = true;
  }
};

DepArr get_dep_arr(const TAdvTripRoute& route, string country)
{
  struct FindCountry
  {
    const string country_code;
    FindCountry(const string& code) : country_code(code) {}
    bool operator()(const TAdvTripRouteItem& item) const
    { return getCountryByAirp(item.airp).code == country_code; }
  };

  // в настоящее время PAXLST Астры предоставляет только 2 поля: 125 и 87
  // поэтому более сложные случаи пока не рассматриваем
  DepArr da;
  const FindCountry fc(country);
  if (find_if_not(route.cbegin(), route.cend(), fc) == route.cend()) /* других нет - неожиданность */
    return da;
  if (find_if(route.crbegin(), route.crend(), fc) == route.crbegin()) /* inbound - последним */
  {
    auto last_before_arr = find_if_not(route.crbegin(), route.crend(), fc);
    auto first_after_arr = find_if(last_before_arr.base()-1, route.cend(), fc);
    da.airp125 = last_before_arr->airp;
    da.time189 = last_before_arr->scd_out;
    da.airp87 = first_after_arr->airp;
    da.time232 = first_after_arr->scd_in;
    da.airpCBP = da.airp87;
    da.validate();
  }
  else if (find_if(route.cbegin(), route.cend(), fc) == route.cbegin()) /* outbound - первым */
  {
    auto first_after_dep = find_if_not(route.cbegin(), route.cend(), fc);
    auto last_before_dep = find_if(make_reverse_iterator(first_after_dep), route.crend(), fc);
    da.airp125 = last_before_dep->airp;
    da.time189 = last_before_dep->scd_out;
    da.airp87 = first_after_dep->airp;
    da.time232 = first_after_dep->scd_in;
    da.airpCBP = da.airp125;
    da.validate();
  }
  else if (find_if(route.cbegin(), route.cend(), fc) != route.cend()) /* где-то в середине - рассмотрим как inbound */
  {
    auto first_after_arr = find_if(route.cbegin(), route.cend(), fc);
    auto last_before_arr = find_if_not(make_reverse_iterator(first_after_arr), route.crend(), fc);
    da.airp125 = last_before_arr->airp;
    da.time189 = last_before_arr->scd_out;
    da.airp87 = first_after_arr->airp;
    da.time232 = first_after_arr->scd_in;
    da.airpCBP = da.airp87;
    da.validate();
  }
  /* не найдено - неожиданность */
  return da;
}

//-----------------------------------------------------------------------------------

std::string TPaxRequest::msg_china_iapi() const
{
    Paxlst::PaxlstInfo paxlstInfo = toPaxlst();
    string result = paxlstInfo.toEdiString();
    LogTrace(TRACE5) << __func__ << ": " << result;
    return result;
}

Paxlst::PaxlstInfo TPaxRequest::toPaxlst() const
{
    const string country_china = "ЦН";
    CheckIn::TSimplePaxItem pax_item;
    CheckIn::TSimplePaxGrpItem grp_item;
    if (not pax_item.getByPaxId(pax.pax_id))
      throw Exception("%s getByPaxId failed, pax_id %d", __func__, pax.pax_id);
    if (not grp_item.getByGrpId(pax_item.grp_id))
      throw Exception("%s getByGrpId failed, grp_id %d", __func__, pax_item.grp_id);
    string pax_airp_dep = (static_cast<const TAirpsRow&>(base_tables.get("airps").get_row("code",grp_item.airp_dep))).code_lat;
    string pax_airp_arv = (static_cast<const TAirpsRow&>(base_tables.get("airps").get_row("code",grp_item.airp_arv))).code_lat;

    int point_id = grp_item.point_dep;
    TAdvTripRoute route, tmp;
    route.GetRouteBefore(NoExists, point_id, trtWithCurrent, trtNotCancelled);
    tmp.GetRouteAfter(NoExists, point_id, trtNotCurrent, trtNotCancelled);
    route.insert(route.end(), tmp.begin(), tmp.end());
    if(route.empty())
      throw Exception("%s empty route, point_id %d", __func__, point_id);

    DepArr da = get_dep_arr(route, country_china);
    if (not da.valid())
      throw Exception("%s cannot get departure or arrival", __func__);

    const string DOC_ID = ""; // empty docId for Clear Passenger Request
    Paxlst::PaxlstInfo paxlstInfo(Paxlst::PaxlstInfo::FlightPassengerManifest, DOC_ID);
    paxlstInfo.settings().setRespAgnCode("ZZZ");
    paxlstInfo.settings().setAppRef("IAPI");
    paxlstInfo.settings().setMesRelNum("05B");
    paxlstInfo.settings().setMesAssCode("IATA");
    paxlstInfo.settings().setViewUNGandUNE(true); // иначе ошибка
    paxlstInfo.settings().set_unh_number("11085B94E1F8FA");

    paxlstInfo.setSenderName("NORDWIND");
    paxlstInfo.setSenderCarrierCode("ZZ");
    paxlstInfo.setRecipientName("NIAC");
    paxlstInfo.setRecipientCarrierCode("ZZ");
    paxlstInfo.setIataCode("");

    // 5.6 RFF: Reference
    paxlstInfo.settings().set_view_RFF_TN(true);
    stringstream rff_tn;
    rff_tn << trans.msg_id;
    paxlstInfo.settings().set_RFF_TN(rff_tn.str());

    // 5.7 NAD: Name and Address ? Reporting Party-GR.1
  //  paxlstInfo.setPartyName("TESTPARTYNAME");

    // 5.8 COM: Communication Contact-GR.1
  //  paxlstInfo.setPhone("11111111");
  //  paxlstInfo.setFax("22222222");

    // 5.9 TDT: Details of Transport-GR.2
    paxlstInfo.setFlight(int_flt.flt_num);

    // 5.10 LOC: Place/Location Identification ? Flight Itinerary-GR.3 DEPARTURE
    paxlstInfo.setDepPort(da.airp125);
    // 5.11 DTM: Date/Time/Period ? Flight Time-GR.3 DEPARTURE
    paxlstInfo.setDepDateTime(da.time189);
    // 5.10 LOC: Place/Location Identification ? Flight Itinerary-GR.3 ARRIVAL
    paxlstInfo.setArrPort(da.airp87);
    // 5.11 DTM: Date/Time/Period ? Flight Time-GR.3 ARRIVAL
    paxlstInfo.setArrDateTime(da.time232);

    Paxlst::PassengerInfo paxInfo;

    // 5.12 NAD: Name and Address ? Traveler-GR.4
    paxInfo.setSurname(pax.family_name);
    paxInfo.setFirstName(pax.given_names.empty()?"FNU":pax.given_names);
    paxInfo.setSecondName("");

    // 5.13 ATT: Attribute-GR.4
    paxInfo.setSex(pax.sex);

    // 5.14 DTM: Date/Time/Period ? Date of Birth-GR.4
    TDateTime birth_date;
    StrToDateTime(pax.birth_date.c_str(), "yyyymmdd", birth_date);
    paxInfo.setBirthDate(birth_date);

    // 5.15 GEI: Processing Information-GR.4
    paxInfo.setProcInfo("173"); // FIXME TESTING

    // 5.16 LOC: Place/Location Identification ? Residence/Itinerary -GR4
    paxInfo.setCBPPort(da.airpCBP);
    paxInfo.setDepPort(pax_airp_dep);
    paxInfo.setArrPort(pax_airp_arv);

    // 5.17 NAT: Nationality-GR4
    paxInfo.setNationality(pax.nationality);

    // 5.18 RFF: Reference-GR.4
    paxInfo.setReservNum(pax.pnr_locator.empty()?"XXX":pax.pnr_locator);
    paxInfo.setPaxRef(IntToString(pax.pax_id));
    paxInfo.setTicketNumber(pax_item.tkn.no_str("C"));

    // 5.19 DOC: Document/Message Details-GR.5
    // 5.20 DTM: Date/Time/Period - Traveler Document Expiration/issue-GR.5
    // 5.21 LOC: Place/Location Identification - Travel Document Issuing Country-GR.5
    // travel document
    // 5.19
    if ("P" == pax.doc_type or "T" == pax.doc_type)
      paxInfo.setDocType(pax.doc_type);
    paxInfo.setDocNumber(pax.passport);
    // 5.20
    TDateTime doc_expiry_date;
    StrToDateTime(pax.expiry_date.c_str(), "yyyymmdd", doc_expiry_date);
    paxInfo.setDocExpirateDate(doc_expiry_date);
    // 5.21
    paxInfo.setDocCountry(pax.issuing_state);
    // visa
    // 5.19
    if ("V" == pax_add.doco_type)
      paxInfo.setDocoType(pax_add.doco_type);
    paxInfo.setDocoNumber(pax_add.doco_no);
    // 5.20
    TDateTime doco_expiry_date;
    StrToDateTime(pax_add.doco_expiry_date.c_str(), "yyyymmdd", doco_expiry_date);
    paxInfo.setDocoExpirateDate(doco_expiry_date);
    // 5.21
    paxInfo.setDocoCountry(pax_add.country_issuance);

    paxlstInfo.addPassenger(paxInfo);

    return paxlstInfo;
}

//-----------------------------------------------------------------------------------

void TPaxRequest::saveData() const
{
  TQuery Qry(&OraSession);

  if( trans.code == "CICX" )
  {
    Qry.SQLText = "UPDATE apps_pax_data SET cicx_msg_id = :cicx_msg_id "
                  "WHERE apps_pax_id = :apps_pax_id";
    Qry.CreateVariable("cicx_msg_id", otInteger, trans.msg_id);
    Qry.CreateVariable("apps_pax_id", otString, pax.apps_pax_id);
    Qry.Execute();

    return;
  }

  Qry.SQLText = "INSERT INTO apps_pax_data (pax_id, cirq_msg_id, pax_crew, nationality, issuing_state, "
                "                       passport, check_char, doc_type, doc_subtype, expiry_date, sup_check_char, "
                "                       sup_doc_type, sup_passport, family_name, given_names, "
                "                       date_of_birth, sex, birth_country, is_endorsee, transfer_at_orgn, "
                "                       transfer_at_dest, pnr_source, pnr_locator, send_time, pre_ckin, "
                "                       flt_num, dep_port, dep_date, arv_port, arv_date, ckin_flt_num, ckin_port, "
                "                       point_id, ckin_point_id, pass_ref, version, "
                "     country_for_data, doco_type, doco_no, country_issuance, doco_expiry_date, "
                "     num_street, city, state, postal_code, redress_number, traveller_number ) "
                "VALUES (:pax_id, :cirq_msg_id, :pax_crew, :nationality, :issuing_state, :passport, "
                "        :check_char, :doc_type, :doc_subtype, :expiry_date, :sup_check_char, :sup_doc_type, :sup_passport, "
                "        :family_name, :given_names, :date_of_birth, :sex, :birth_country, :is_endorsee, "
                "        :transfer_at_orgn, :transfer_at_dest, :pnr_source, :pnr_locator, :send_time, :pre_ckin, "
                "        :flt_num, :dep_port, :dep_date, :arv_port, :arv_date, :ckin_flt_num, :ckin_port, "
                "        :point_id, :ckin_point_id, :pass_ref, :version, "
                "     :country_for_data, :doco_type, :doco_no, :country_issuance, :doco_expiry_date, "
                "     :num_street, :city, :state, :postal_code, :redress_number, :traveller_number )";

  Qry.CreateVariable("cirq_msg_id", otInteger, trans.msg_id);
  Qry.CreateVariable("pax_id", otInteger, pax.pax_id);
  Qry.CreateVariable("pax_crew", otString, pax.pax_crew);
  Qry.CreateVariable("nationality", otString, pax.nationality);
  Qry.CreateVariable("issuing_state", otString, pax.issuing_state);
  Qry.CreateVariable("passport", otString, pax.passport);
  Qry.CreateVariable("check_char", otString, pax.check_char);
  Qry.CreateVariable("doc_type", otString, pax.doc_type);
  Qry.CreateVariable("doc_subtype", otString, pax.doc_subtype);
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
  Qry.CreateVariable("send_time", otDate, NowUTC());
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
  Qry.CreateVariable("pass_ref", otString, pax.reference);
  Qry.CreateVariable("version", otInteger, version);
  Qry.CreateVariable("country_for_data", otString, pax_add.country_for_data);
  Qry.CreateVariable("doco_type", otString, pax_add.doco_type);
  Qry.CreateVariable("doco_no", otString, pax_add.doco_no);
  Qry.CreateVariable("country_issuance", otString, pax_add.country_issuance);
  Qry.CreateVariable("doco_expiry_date", otString, pax_add.doco_expiry_date);
  Qry.CreateVariable("num_street", otString, pax_add.num_street);
  Qry.CreateVariable("city", otString, pax_add.city);
  Qry.CreateVariable("state", otString, pax_add.state);
  Qry.CreateVariable("postal_code", otString, pax_add.postal_code);
  Qry.CreateVariable("redress_number", otString, pax_add.redress_number);
  Qry.CreateVariable("traveller_number", otString, pax_add.traveller_number);
  Qry.Execute();
}

APPSAction TPaxRequest::typeOfAction( const bool is_exists, const std::string& status,
                                      const bool is_the_same, const bool is_forced) const
{
  bool is_cancel = (trans.code == "CICX");

  if( !is_exists ) {
    if ( is_cancel ) {
      deleteAPPSAlarms( pax.pax_id,  int_flt.point_id );
      return NoAction;
    }
    return NeedNew;
  }

  if ( !status.empty() && is_cancel ) {
    deleteAPPSAlarms( pax.pax_id, int_flt.point_id );
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
    addAPPSAlarm(pax.pax_id, {Alarm::APPSConflict}, int_flt.point_id);
    return NoAction;
  }
}

void TPaxRequest::sendReq(Timing::Points &timing) const
{
  timing.start("saveData");
  saveData();
  timing.finish("saveData");
  timing.start("sendNewReq");
  sendNewReq( *this, trans.msg_id, int_flt.point_id, version );
  timing.finish("sendNewReq");
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
  /* 1 */ msg << "MRQ" << '/';
  /* 2 */ msg << FieldCount("MRQ", version) << '/';
  /* 3 */ msg << country << '/';
  /* 4 */ msg << mft_pax << '/';
  /* 5 */ msg << mft_crew;
  return msg.str();
}

bool TManifestRequest::init( const int point_id, const std::string& country_lat, const std::string& country_code )
{
  ProgTrace(TRACE5, "TManifestRequest::init: %d", point_id);
  TTripInfo info;
  if (not info.getByPointId( point_id ))
  {
    ProgTrace(TRACE5, "getByPointId returned false, point_id=%d", point_id);
    return false;
  }
  const TAirlinesRow &airline = (const TAirlinesRow&)base_tables.get("airlines").get_row("code", info.airline);
  if (airline.code_lat.empty())
    throw Exception("airline.code_lat empty (code=%s)",airline.code.c_str());
  version = GetVersion(airline.code, country_code);
  trans.init( false, "CIMR", airline, version );
  int_flt.init( point_id, "INM", version );
  mft_req.init( country_lat, version );
  return true;
}

std::string TManifestRequest::msg() const
{
  string msg  = trans.msg() + "/"
              + int_flt.msg() + "/"
              + mft_req.msg() + "/";
  return string(trans.header + "\x02" + msg + "\x03");
}

void TManifestRequest::sendReq() const
{
  sendNewReq( msg(), trans.msg_id, int_flt.point_id, version );
}

// APPS ANSWER ---------------------------------------------------------

static int getInt( const std::string& val )
{
  if ( val.empty() ) return ASTRA::NoExists;
  return ToInt( val );
}

void TAnsPaxData::init( std::string source, int ver )
{
  version = ver;
  /* PRS PCC */
  vector<string> tmp;
  boost::split( tmp, source, boost::is_any_of( "/" ) );

  string grp_id = tmp[0]; /* 1 */
  if ( grp_id != "PRS" && grp_id != "PCC" )
    throw Exception( "Incorrect grp_id: %s", grp_id.c_str() );

  int field_count = getInt(tmp[1]); /* 2 */
  if ( ( grp_id == "PRS" && field_count != FieldCount("PRS", version) ) ||
       ( grp_id == "PCC" && field_count != FieldCount("PCC", version) ) )
    throw Exception( "Incorrect field_count: %d", field_count );

  int seq_num = getInt(tmp[2]); /* 3 */
  if( seq_num != pax_seq_num ) // Passenger Sequence Number
    throw Exception( "Incorrect seq_num: %d", seq_num );

  country = tmp[3]; /* 4 */

  if ( grp_id == "PRS" )
  {
    /* PRS */
    int i = 22; // начальная позиция итератора (поле 23)
    if (version < APPS_VERSION_27) --i;
    if (version < APPS_VERSION_26) --i;
    code = getInt(tmp[i++]);         /* 23 */
    status = *(tmp[i++].begin());    /* 24 */
    apps_pax_id = tmp[i++];          /* 25 */
    error_code1 = getInt(tmp[i++]);  /* 26 */
    error_text1 = tmp[i++];          /* 27 */
    error_code2 = getInt(tmp[i++]);  /* 28 */
    error_text2 = tmp[i++];          /* 29 */
    error_code3 = getInt(tmp[i++]);  /* 30 */
    error_text3 = tmp[i];            /* 31 */
  }
  else
  {
    /* PCC */
    int i = 20; // начальная позиция итератора (поле 21)
    if (tmp.size() == 29) ++i; // workaround for incorrect data (excess field)
    code = getInt(tmp[i++]);         /* 21 */
    status = *(tmp[i++].begin());    /* 22 */
    error_code1 = getInt(tmp[i++]);  /* 23 */
    error_text1 = tmp[i++];          /* 24 */
    error_code2 = getInt(tmp[i++]);  /* 25 */
    error_text2 = tmp[i++];          /* 26 */
    error_code3 = getInt(tmp[i++]);  /* 27 */
    error_text3 = tmp[i];            /* 28 */
  }
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
  Qry.SQLText="SELECT send_attempts, msg_text, point_id, version "
              "FROM apps_messages "
              "WHERE msg_id = :msg_id";
  Qry.CreateVariable("msg_id", otInteger, msg_id);
  Qry.Execute();

  if(Qry.Eof)
    return false;

  point_id = Qry.FieldAsInteger("point_id");
  msg_text = Qry.FieldAsString("msg_text");
  send_attempts = Qry.FieldAsInteger("send_attempts");
  version = Qry.FieldIsNULL("version")? APPS_VERSION_21: Qry.FieldAsInteger("version");

  if( *(it++) == "ERR" )
  {
    size_t fld_count = getInt( *(it++) );
    if ( fld_count != ( tmp.size() - 3) )
      throw Exception( "Incorrect fld_count: %d", fld_count );
    while(it < tmp.end())
    {
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
  flightsForLock.Lock(__FUNCTION__);

  // выключим тревогу "Нет связи с APPS"
  set_alarm( point_id, Alarm::APPSOutage, false );
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
  if ( code == "CIRS" )
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

  string delim = (code == "CIRS")?"PRS":"PCC";
  delim = string("/") + delim + string("/");
  std::size_t pos1 = source.find( delim );
  while ( pos1 != string::npos )
  {
    std::size_t pos2 = source.find( delim, pos1 + delim.size() );
    TAnsPaxData data;
    data.init( source.substr( pos1 + 1, pos2 - pos1 ), version );
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

  if( /*!CheckIfNeedResend() &&*/ code == "CIRS" ) {
    addAPPSAlarm(pax_id, {Alarm::APPSConflict}); // рассинхронизация
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

    if ( code == "CICC" ) {
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
      addAPPSAlarm(pax_id, {Alarm::APPSNegativeDirective});
    }
    else if (it->status == "U" || it->status == "I" || it->status == "T" || it->status == "E") {
      addAPPSAlarm(pax_id, {Alarm::APPSError});
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
  if ( code == "CICC" ) {
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
    deleteAPPSAlarm(pax_id, {Alarm::APPSNegativeDirective, Alarm::APPSError});
  }
  // проверим, нужно ли гасить тревогу "рассинхронизация"
  TPaxRequest actual;
  TPaxRequest received;
  Timing::Points timing("Timing::processAnswer");
  timing.start("actual.init");
  bool pax_not_found = false;
  try
  {
    actual.init( pax_id, timing );
  }
  catch (AppsPaxNotFoundException)
  {
    pax_not_found = true;
    ProgTrace(TRACE5, "Passenger has not been found");
  }
  timing.finish("actual.init");
  if (!pax_not_found)
    received.fromDBByMsgId( msg_id );
  if ( pax_not_found || received == actual )
  {
    deleteAPPSAlarm(pax_id, {Alarm::APPSConflict});
  }
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

  CheckIn::TSimplePaxItem pax;
  if (pax.getByPaxId(pax_id))
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

  size_t pos = source.find("MAK");

  if ( pos != string::npos )
  {
    vector<string> tmp;
    string text = source.substr( pos );
    boost::split( tmp, text, boost::is_any_of( "/" ) );
    int fld_count = getInt(tmp[1]); /* 2 */
    if ( fld_count != FieldCount("MAK", version) )
      throw Exception( "Incorrect fld_count: %d", fld_count );

    country = tmp[2]; /* 3 */
    resp_code = getInt(tmp[3]); /* 4 */
    error_code = getInt(tmp[4]); /* 5 */
    error_text = tmp[5]; /* 6 */
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

//-----------------------------------------------------------------------------------

void ProcessChinaCusres(const edifact::Cusres& cusres)
{
    LogTrace(TRACE5) << __func__<< std::endl << cusres;
}

//-----------------------------------------------------------------------------------

std::string getAnsText( const std::string& tlg )
{
  string::size_type pos1 = tlg.find("\x02");
  string::size_type pos2 = tlg.find("\x03");
  if ( pos1 == std::string::npos )
    throw Exception( "Wrong answer format" );
  return tlg.substr( pos1 + 1, pos2 - pos1 - 1 );
}

bool processReply( const std::string& source_raw )
{
  try
  {
    string source = getAnsText(source_raw);

    if(source.empty())
      throw Exception("Answer is empty");

    string code = source.substr(0, 4);
    string answer = source.substr(5, source.size() - 6); // отрезаем код транзакции и замыкающий '/' (XXXX:text_to_parse/)
    boost::scoped_ptr<TAPPSAns> res;
    if ( code == "CIRS" || code == "CICC" )
      res.reset( new TPaxReqAnswer() );
    else if ( code == "CIMA" )
      res.reset( new TMftAnswer() );
    else
      throw Exception( std::string( "Unknown transaction code: " + code ) );

    if (!res->init( code, answer ))
      return false;
    ProgTrace( TRACE5, "Result: %s", res->toString().c_str() );
    res->beforeProcessAnswer();
    res->processErrors();
    res->processAnswer();
    return true;
  }
  catch(EOracleError &E)
  {
    ProgError(STDLOG,"EOracleError %d: %s",E.Code,E.what());
  }
  catch(std::exception &E)
  {
    ProgError(STDLOG,"std::exception: %s",E.what());
  }
  catch(...)
  {
    ProgError(STDLOG, "Unknown exception");
  }
  return false;
}

void processPax( const int pax_id, Timing::Points& timing, const std::string& override_type, const bool is_forced )
{
  ProgTrace( TRACE5, "processPax: %d", pax_id );
  timing.start("processPax");

  TPaxRequest new_data;
  timing.start("new_data.init");
  new_data.init( pax_id, timing, override_type );
  timing.finish("new_data.init");

  TPaxRequest actual_data;
  bool is_exists = actual_data.fromDBByPaxId( pax_id );

  APPSAction action = new_data.typeOfAction( is_exists, actual_data.getStatus(),
                                             ( new_data == actual_data ), is_forced );

  if (action == NoAction)
  {
    timing.finish("processPax");
    return;
  }

  if ( action == NeedCancel || action == NeedUpdate )
  {
    timing.start("actual_data.sendReq");
    actual_data.sendReq(timing);
    timing.finish("actual_data.sendReq");
  }

  if ( action == NeedUpdate || action == NeedNew )
  {
    timing.start("new_data.sendReq");
    new_data.sendReq(timing);
    timing.finish("new_data.sendReq");
  }

  timing.finish("processPax");
}

std::set<std::string> needFltCloseout( const std::set<std::string>& countries, const std::string airline )
{
  set<string> countries_need_req;

  if(countries.size() > 1)
  {
    // не отправляем для местных рейсов
    for (set<string>::const_iterator it = countries.begin(); it != countries.end(); it++)
    {
      TAppsSets sets(airline, *it);
      int flt_closeout;
      bool not_eof = sets.get_flt_closeout(flt_closeout);
      if (not_eof && flt_closeout)
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
  countries.insert( getCountryByAirp(r->airp).code );
  for(r++; r!=route.end(); r++)
  {
    // определим, нужно ли отправлять данные
    if( !checkAPPSSets( point_id, r->point_id ) )
      continue;
    countries.insert( getCountryByAirp(r->airp).code );

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
                "      pr_brd=0 AND apps_pax_data.status = 'B'  and pax_crew != 'C'";
    Qry.CreateVariable("point_dep", otInteger, point_id);
    Qry.CreateVariable("point_arv", otInteger, r->point_id);
    Qry.Execute();

    Timing::Points timing("Timing::APPSFlightCloseout");
    timing.start("for !Qry.Eof");
    for( ; !Qry.Eof; Qry.Next() )
    {
      TPaxRequest pax;
      pax.fromDBByMsgId( Qry.FieldAsInteger("cirq_msg_id") );
      if( pax.getStatus() != "B" )
        continue; // CICX request has already been send
      pax.sendReq(timing);
    }
    timing.finish("for !Qry.Eof");
  }
  set<string> countries_need_req = needFltCloseout( countries, route.front().airline );
  for( set<string>::const_iterator it = countries_need_req.begin(); it != countries_need_req.end(); it++ )
  {
    string country_lat = ((const TCountriesRow&)base_tables.get("countries").get_row("code",*it)).code_lat;
    TManifestRequest close_flt;
    if (close_flt.init( point_id, country_lat, *it ))
      close_flt.sendReq();
  }
}

bool IsAPPSAnswText(const std::string& tlg_body)
{
  return ( ( tlg_body.find( string("CIRS") + ":" ) != string::npos ) ||
           ( tlg_body.find( string("CICC") + ":" ) != string::npos ) ||
           ( tlg_body.find( string("CIMA") + ":" ) != string::npos ) );
}

/*
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
*/

// РЕФАКТОРИНГ ФУНКЦИИ НЕ ПРОИЗВОДИЛСЯ
// ВЕРСИИ ВЫШЕ 21 БУДУТ РАБОТАТЬ НЕКОРРЕКТНО
std::string emulateAnswer( const std::string& request )
{
  ostringstream answer;
  /*
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
  */
  return answer.str();
}

void reSendMsg( const int send_attempts, const std::string& msg_text, const int msg_id, const int version )
{
  if (version == APPS_VERSION_CHINA)
  {
//    EdiSessionHandler e;
//    sendTlg( getIAPIRotName(), OWN_CANON_NAME(), qpOutA, 20, msg_text,
//        ASTRA::NoExists, ASTRA::NoExists );
  }
  else
    sendTlg( getAPPSRotName(), OWN_CANON_NAME(), qpOutApp, 20, msg_text,
            ASTRA::NoExists, ASTRA::NoExists );

//  sendCmd("CMD_APPS_ANSWER_EMUL","H");
  sendCmd("CMD_APPS_HANDLER","H");

  TQuery Qry(&OraSession);
  Qry.SQLText = "UPDATE apps_messages "
                "SET send_attempts = :send_attempts, send_time = :send_time "
                "WHERE msg_id = :msg_id ";
  Qry.CreateVariable("msg_id", otInteger, msg_id);
  Qry.CreateVariable("send_attempts", otInteger, send_attempts + 1);
  Qry.CreateVariable("send_time", otDate, NowUTC());
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
  flightsForLock.Lock(__FUNCTION__);

  Timing::Points timing("Timing::sendAPPSInfo");
  timing.start("for !Qry.Eof");
  for ( ; !Qry.Eof; Qry.Next() )
    if ( checkAPPSSets( point_id, Qry.FieldAsString( "airp_arv" ) ) )
      processPax( Qry.FieldAsInteger( "pax_id" ), timing );
  timing.finish("for !Qry.Eof");
}

void sendAllAPPSInfo(const TTripTaskKey &task)
{
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText =
    "SELECT point_id_tlg "
    "FROM tlg_binding, points "
    "WHERE tlg_binding.point_id_spp=points.point_id AND "
    "      point_id_spp=:point_id_spp AND "
    "      points.pr_del=0 AND points.pr_reg<>0";
  Qry.CreateVariable("point_id_spp", otInteger, task.point_id);
  Qry.Execute();

  for( ; !Qry.Eof; Qry.Next() )
    sendAPPSInfo( task.point_id, Qry.FieldAsInteger("point_id_tlg") );
}

void sendNewAPPSInfo(const TTripTaskKey &task)
{
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText ="SELECT pax_id "
               "FROM crs_pax, crs_pnr, tlg_binding "
               "WHERE need_apps <> 0 AND point_id_spp = :point_id_spp AND "
               "      crs_pax.pnr_id = crs_pnr.pnr_id AND "
               "      crs_pnr.point_id = tlg_binding.point_id_tlg";
  Qry.CreateVariable("point_id_spp", otInteger, task.point_id);
  Qry.Execute();

  TFlights flightsForLock;
  flightsForLock.Get( task.point_id, ftTranzit );
  flightsForLock.Lock(__FUNCTION__);

  Timing::Points timing("Timing::sendNewAPPSInfo");
  timing.start("for !Qry.Eof");
  for( ; !Qry.Eof; Qry.Next() ) {
    int pax_id = Qry.FieldAsInteger( "pax_id" );
    processPax( pax_id, timing );

    TCachedQuery Qry( "UPDATE crs_pax SET need_apps=0 WHERE pax_id=:pax_id",
                      QParams() << QParam( "pax_id", otInteger, pax_id ) );
    Qry.get().Execute();
  }
  timing.finish("for !Qry.Eof");
}

int test_apps_tlg(int argc, char **argv)
{
  for (string text :
  {
    /* correct */
    "VDCICC:6225330/PCC/26/001/AE/P/RUS/RUS/715961901//P/20210830////KRASNOBORODKIN/DMITRY/19860613/M///8505/C///////",
    /* correct with excess field */
    "VDCICC:6225330/PCC/26/001/AE/P/RUS/RUS/715961901//P//20210830////KRASNOBORODKIN/DMITRY/19860613/M///8505/C///////",
    /* incorrect */
    "VDZZZZ:6225330/PCC/26/001/AE/P/RUS/RUS/715961901//P//20210830////KRASNOBORODKIN/DMITRY/19860613/M///8505/C///////",
    "VDCICC:6225330/ZZZ/26/001/AE/P/RUS/RUS/715961901//P//20210830////KRASNOBORODKIN/DMITRY/19860613/M///8505/C///////",
    "VDCICC:6225330/PCC/99/001/AE/P/RUS/RUS/715961901//P//20210830////KRASNOBORODKIN/DMITRY/19860613/M///8505/C///////",
  })
  {
    int tlg_num = saveTlg(OWN_CANON_NAME(), "zzzzz", "IAPP", text);
    putTlg2OutQueue_wrap(OWN_CANON_NAME(), "zzzzz", "IAPP", text, 0, tlg_num, 0);
    sendCmd("CMD_APPS_HANDLER", "H");
  }
  return 0;
}



