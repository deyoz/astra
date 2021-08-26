#if HAVE_CONFIG_H
#include <config.h>
#endif

#include "astra_utils.h"
#include "oralib.h"
#include "astra_locale.h"
#include "stl_utils.h"
#include "xml_unit.h"
#include "misc.h"
#include "astra_elems.h"
#include "base_tables.h"
#include "term_version.h"
#include "qrys.h"
#include "astra_misc.h"
#include "dev_consts.h"
#include "dev_utils.h"
#include "db_savepoint.h"

#include <serverlib/tcl_utils.h>
#include <serverlib/monitor_ctl.h>
#include <serverlib/sirena_queue.h>
#include <serverlib/testmode.h>
#include <serverlib/lwriter.h>
#include <serverlib/dump_table.h>
#include <serverlib/EdiHelpManager.h>
#include <serverlib/query_runner.h>
#include <serverlib/commit_rollback.h>
#include <serverlib/posthooks.h>
#include <serverlib/algo.h>
#include <serverlib/pg_cursctl.h>
#include <jxtlib/JxtInterface.h>
#include <jxtlib/jxt_cont.h>
#include <jxtlib/xml_stuff.h>

#include <stdarg.h>
#include <string>
#include <string.h>

#include "PgOraConfig.h"

#define NICKNAME "VLAD"
#include <serverlib/slogger.h>

using namespace std;
using namespace ASTRA;
using namespace BASIC::date_time;
using namespace EXCEPTIONS;
using namespace boost::local_time;
using namespace boost::posix_time;
using namespace AstraLocale;

#ifdef XP_TESTING
namespace xp_testing {
    bool isSelfCheckinRequestOnJxtGrp(const std::string& login)
    {
        //добавляйте в WebLogins только те, которые отрабатывают как grp3_jxt, но при этом должны иметь client_type=ctWeb/ctKiosk/ctMobile
        //не добавляйте сюда логины для входящих http! Используйте, например, $(CREATE_USER)+$(CREATE_DESK)+$(ADD_HTTP_CLIENT) и вызов "!! req_type=http"
        static const std::vector<std::string> WebLogins { "KIOSK2" };
        LogTrace(TRACE3) << __func__ << " called for login: " << login;
        return algo::contains(WebLogins, login);
    }
}//namespace xp_testing
#endif//XP_TESTING

std::optional<int> getDeskGroupByCode(const std::string& desk)
{
    DbCpp::CursCtl cur = make_db_curs(
       "SELECT grp_id FROM desks WHERE code = :code",
        PgOra::getROSession("DESKS")
    );

    int group;

    cur.stb()
       .def(group)
       .bind(":code", desk)
       .exfet();

    if (DbCpp::ResultCode::NoDataFound == cur.err()) {
        return std::nullopt;
    }

    return group;
}

string AlignString(string str, int len, string align)
{
    TrimString(str);
    string result;
    if(str.size() > (u_int)len)
        result = str.substr(0, len);
    else {
        switch (*align.c_str())
        {
            case 'R':
                result.assign(len - str.size(), ' ');
                break;
            case 'C':
                result.assign((len - str.size())/2, ' ');
                break;
            case 'L':
                break;
        }
        result += str;
        string buf(len - result.size(), ' ');
        result += buf;
    }
    return result;
};

const std::string TDesk::system_code="SYSTEM";

bool TDesk::isValidVersion(const std::string &ver)
{
  if (ver.size()!=14) return false;
  string::const_iterator c=ver.begin();
  for(int i=0;c!=ver.end();c++,i++)
  {
    if (i>=0 && i<=5 && IsDigit(*c)) continue;
    if (i==6 && *c=='-') continue;
    if (i>=7 && i<=13 && IsDigit(*c)) continue;
    return false;
  };
  return true;
};

bool TDesk::compatible(const std::string &ver)
{
  //проверим правильность указанной версии
  if (!isValidVersion(ver))
    throw EXCEPTIONS::Exception("TDesk::compatible: wrong version param '%s'",ver.c_str());

  return (!version.empty() &&
          version!=UNKNOWN_VERSION &&
          version>=ver);
};

TReqInfo *TReqInfo::Instance()
{
  static TReqInfo *instance_ = 0;
  if ( !instance_ )
    instance_ = new TReqInfo();
  return instance_;
};

void TReqInfo::setPerform()
{
  execute_time = microsec_clock::universal_time();
}

void TReqInfo::clearPerform()
{
    execute_time = ptime( not_a_date_time );
}

void TReqInfo::Initialize( const std::string &city )
{
  clear();
  DB::TQuery Qry(PgOra::getROSession("CITIES"), STDLOG);
  Qry.SQLText=
    "SELECT tz_region FROM cities WHERE cities.code=:city AND cities.pr_del=0";
  Qry.DeclareVariable( "city", otString );
  Qry.SetVariable( "city", city );
  Qry.Execute();
  if (Qry.Eof)
    throw EXCEPTIONS::Exception("TReqInfo::Initialize: city %s not found",city.c_str());
  if (Qry.FieldIsNULL("tz_region"))
      throw AstraLocale::UserException((string)"TReqInfo::Initialize: tz_region not defined (city=" + city + ")");
  desk.city = city;
  desk.tz_region = Qry.FieldAsString( "tz_region" );
  desk.time = UTCToLocal( NowUTC(), desk.tz_region );
  user.access.set_total_permit();
}

void TReqInfo::Initialize( TReqInfoInitData &InitData )
{
    if ( execute_time.is_not_a_date_time() )
        setPerform();
  clear();

  duplicate=InitData.duplicate;

  if (!InitData.lang.empty())
    desk.lang=InitData.lang;
  else
    desk.lang=AstraLocale::LANG_RU;

  ProgTrace( TRACE5, "screen=%s, pult=|%s|, opr=|%s|, checkCrypt=%d, pr_web=%d, desk.lang=%s",
            InitData.screen.c_str(), InitData.pult.c_str(), InitData.opr.c_str(), InitData.checkCrypt, InitData.pr_web, desk.lang.c_str() );
  screen.name = upperc( InitData.screen );
  desk.code = InitData.pult;
  desk.mode = DecodeOperMode(InitData.mode);
  std::string deskUpper = StrUtils::ToUpper(InitData.pult);
  std::optional<int> deskGroup = getDeskGroupByCode(deskUpper);

  if (InitData.checkCrypt
   && !InitData.duplicate
   && deskGroup.has_value()) { // пришло не зашифрованное сообщение - проверка на то, что пользователь шифруется
    DB::TQuery Qry(PgOra::getROSession("CRYPT_SETS"), STDLOG);
    Qry.SQLText =
     "SELECT pr_crypt FROM crypt_sets "
     "WHERE desk_grp_id = :grp_id "
       "AND (desk = :desk OR desk IS NULL) "
     "ORDER BY desk";
    Qry.CreateVariable( "desk", otString, deskUpper );
    Qry.CreateVariable( "grp_id", otInteger, *deskGroup );
    Qry.Execute();
    if ( !Qry.Eof && Qry.FieldAsInteger( "pr_crypt" ) != 0 ) {
      XMLRequestCtxt *xmlRC = getXmlCtxt();
      if (xmlRC->resDoc!=NULL)
      {
        xmlNodePtr resNode = NodeAsNode("/term/answer", xmlRC->resDoc);
        AstraLocale::showProgError( "MSG.MESSAGEPRO.CRYPT_MODE_ERR.REPEAT" );
        resNode = ReplaceTextChild( resNode, "clear_certificates" );
      };
      throw UserException2();
    }
  }

  DB::TQuery QryScreen(PgOra::getROSession("SCREEN"), STDLOG);
  QryScreen.SQLText = "SELECT id,pr_logon FROM screen WHERE exe = :exe";
  QryScreen.DeclareVariable( "exe", otString );
  QryScreen.SetVariable( "exe", screen.name );
  QryScreen.Execute();
  if ( QryScreen.RowCount() == 0 )
    throw EXCEPTIONS::Exception( (string)"Unknown screen " + screen.name );
  screen.id = QryScreen.FieldAsInteger( "id" );
  screen.pr_logon = QryScreen.FieldAsInteger( "pr_logon" );

  DB::TQuery QryDesks(PgOra::getROSession({"DESKS","DESK_GRP"}), STDLOG);
  QryDesks.SQLText =
    "SELECT desks.version, COALESCE(desks.under_constr,0) AS under_constr, "
    "       desks.currency, desks.term_id, "
    "       desk_grp.grp_id, desk_grp.city, desk_grp.airp, desk_grp.airline "
    "FROM desks,desk_grp "
    "WHERE desks.grp_id = desk_grp.grp_id AND desks.code = :pult";
  QryDesks.DeclareVariable( "pult", otString );
  QryDesks.SetVariable( "pult", deskUpper );
  QryDesks.Execute();
  if ( QryDesks.RowCount() == 0 )
    throw AstraLocale::UserException( "MSG.PULT_NOT_REGISTERED");
  if (QryDesks.FieldAsInteger("under_constr")!=0)
    throw AstraLocale::UserException( "MSG.SERVER_TEMPORARILY_UNAVAILABLE" );
  desk.city = QryDesks.FieldAsString( "city" );
  desk.airp = QryDesks.FieldAsString( "airp" );
  desk.airline = QryDesks.FieldAsString( "airline" );
  desk.version = QryDesks.FieldAsString( "version" );
  desk.currency = QryDesks.FieldAsString( "currency" );
  desk.grp_id = QryDesks.FieldAsInteger( "grp_id" );
  desk.term_id = QryDesks.FieldIsNULL("term_id")?NoExists:QryDesks.FieldAsFloat( "term_id" );

  ProgTrace( TRACE5, "terminal version='%s'", desk.version.c_str() );

  DB::TQuery QryCities(PgOra::getROSession("CITIES"), STDLOG);
  QryCities.SQLText=
    "SELECT tz_region FROM cities WHERE cities.code=:city AND cities.pr_del=0";
  QryCities.DeclareVariable( "city", otString );
  QryCities.SetVariable( "city", desk.city );
  QryCities.Execute();
  if (QryCities.Eof)
    throw AstraLocale::UserException("MSG.DESK_CITY_NOT_DEFINED");
  if (QryCities.FieldIsNULL("tz_region"))
    throw AstraLocale::UserException("MSG.CITY.REGION_NOT_DEFINED", LParams() << LParam("city", ElemIdToCodeNative(etCity,desk.city)));
  desk.tz_region = QryCities.FieldAsString( "tz_region" );
  desk.time = UTCToLocal( NowUTC(), desk.tz_region );
  if ( !screen.pr_logon ||
       (!InitData.pr_web && !InitData.checkUserLogon) )
    return;
  DB::TQuery QryUsers2(PgOra::getROSession("USERS2"), STDLOG);
  if ( InitData.pr_web ) {
    QryUsers2.SQLText =
      "SELECT user_id, login, descr, type, pr_denial "
      "FROM users2 "
      "WHERE login = :login ";
    QryUsers2.CreateVariable( "login", otString, InitData.opr );
    QryUsers2.Execute();
    if (QryUsers2.Eof)
      throw AstraLocale::UserException( "MSG.USER.ACCESS_DENIED");
  }
  else {
    if (InitData.term_id!=desk.term_id)
      throw AstraLocale::UserException( "MSG.USER.NEED_TO_LOGIN" );

    QryUsers2.SQLText =
      "SELECT user_id, login, descr, type, pr_denial "
      "FROM users2 "
      "WHERE desk = :pult ";
    QryUsers2.CreateVariable( "pult", otString, deskUpper );
    QryUsers2.Execute();
    if (QryUsers2.Eof)
      throw AstraLocale::UserException( "MSG.USER.NEED_TO_LOGIN" );
    if ( !InitData.opr.empty() )
      if ( InitData.opr != QryUsers2.FieldAsString( "login" ) )
        throw AstraLocale::UserException( "MSG.USER.NEED_TO_LOGIN" );
  };

  if ( QryUsers2.FieldAsInteger( "pr_denial" ) == -1 )
    throw AstraLocale::UserException( "MSG.USER.DELETED");
  if ( QryUsers2.FieldAsInteger( "pr_denial" ) != 0 )
    throw AstraLocale::UserException( "MSG.USER.ACCESS_DENIED");
  user.user_id = QryUsers2.FieldAsInteger( "user_id" );
  user.descr = QryUsers2.FieldAsString( "descr" );
  user.user_type = (TUserType)QryUsers2.FieldAsInteger( "type" );
  user.login = QryUsers2.FieldAsString( "login" );


#ifdef XP_TESTING
      if(inTestMode()) {
           InitData.pr_web = InitData.pr_web || xp_testing::isSelfCheckinRequestOnJxtGrp(user.login);
      }
#endif//XP_TESTING

  DB::TQuery QryWebClients(PgOra::getROSession("WEB_CLIENTS"), STDLOG);
  QryWebClients.SQLText =
    "SELECT client_type FROM web_clients "
    "WHERE desk=:desk OR user_id=:user_id";
  QryWebClients.CreateVariable( "desk", otString, deskUpper );
  QryWebClients.CreateVariable( "user_id", otInteger, user.user_id );
  QryWebClients.Execute();
  if ( (!QryWebClients.Eof && !InitData.pr_web) ||
        (QryWebClients.Eof && InitData.pr_web) ) //???
        throw AstraLocale::UserException( "MSG.USER.ACCESS_DENIED" );

  if ( InitData.pr_web )
    client_type = DecodeClientType( QryWebClients.FieldAsString( "client_type" ).c_str() );
  else
    client_type = ctTerm;

  user.access.fromDB(user.user_id, user.user_type);

  TAccessElems<std::string> airlines;
  TAccessElems<std::string> airps;
  vector<string> elems;

  //проверим ограничение доступа по сессии
  SeparateString(JxtContext::getJxtContHandler()->sysContext()->read("session_airlines"),'/',elems);
  airlines.clear();
  airlines.set_elems_permit(!elems.empty());
  for(vector<string>::const_iterator e=elems.begin(); e!=elems.end(); ++e) airlines.add_elem(*e);
  user.access.merge_airlines(airlines);

  //проверим ограничение доступа по фильтрам
  SeparateString(JxtContext::getJxtContHandler()->sysContext()->read("filter_airlines"),'/',elems);
  airlines.clear();
  airlines.set_elems_permit(!elems.empty());
  for(vector<string>::const_iterator e=elems.begin(); e!=elems.end(); ++e) airlines.add_elem(*e);
  user.access.merge_airlines(airlines);

  SeparateString(JxtContext::getJxtContHandler()->sysContext()->read("filter_airps"),'/',elems);
  airps.clear();
  airps.set_elems_permit(!elems.empty());
  for(vector<string>::const_iterator e=elems.begin(); e!=elems.end(); ++e) airps.add_elem(*e);
  user.access.merge_airps(airps);

  //проверим ограничение доступа по собственникам пульта
  DB::TQuery QryDeskOwners(PgOra::getROSession("DESK_OWNERS"), STDLOG);
  QryDeskOwners.SQLText=
    "SELECT airline,pr_denial FROM desk_owners "
    "WHERE desk=:desk "
    "ORDER BY airline NULLS FIRST";
  QryDeskOwners.CreateVariable("desk",otString,InitData.pult);
  QryDeskOwners.Execute();
  bool pr_denial_all=true;
  if (!QryDeskOwners.Eof && QryDeskOwners.FieldIsNULL("airline"))
  {
    pr_denial_all=QryDeskOwners.FieldAsInteger("pr_denial")!=0;
    QryDeskOwners.Next();
  };

  airlines.clear();
  airlines.set_elems_permit(pr_denial_all);
  for(;!QryDeskOwners.Eof;QryDeskOwners.Next())
  {
    if (QryDeskOwners.FieldIsNULL("airline")) continue;
    bool pr_denial=QryDeskOwners.FieldAsInteger("pr_denial")!=0;
    if ((!pr_denial_all && pr_denial) ||
        (pr_denial_all && !pr_denial))
      airlines.add_elem(QryDeskOwners.FieldAsString("airline"));
  };
  user.access.merge_airlines(airlines);
  if (airlines.totally_not_permitted())
    throw AstraLocale::UserException( "MSG.DESK.TURNED_OFF" );

  //пользовательские настройки
  DB::TQuery QryUserSets(PgOra::getROSession("USER_SETS"), STDLOG);
  QryUserSets.SQLText=
    "SELECT time,disp_airline,disp_airp,disp_craft,disp_suffix "
    "FROM user_sets "
    "WHERE user_id=:user_id";
  QryUserSets.CreateVariable("user_id",otInteger,user.user_id);
  QryUserSets.Execute();
  if (!QryUserSets.Eof)
  {
    for(int i=0;i<5;i++)
    {
      const char* field;
      switch(i)
      {
        case 0: field="time"; break;
        case 1: field="disp_airline"; break;
        case 2: field="disp_airp"; break;
        case 3: field="disp_craft"; break;
        case 4: field="disp_suffix"; break;
      };
      if (!QryUserSets.FieldIsNULL(field))
      {
        switch(QryUserSets.FieldAsInteger(field))
        {
          case ustTimeUTC:
          case ustTimeLocalDesk:
          case ustTimeLocalAirp:
            if (i==0)
              user.sets.time=(TUserSettingType)QryUserSets.FieldAsInteger(field);
            break;
          case ustCodeNative:
          case ustCodeInter:
          case ustCodeICAONative:
          case ustCodeICAOInter:
          case ustCodeMixed:
            if (i==1)
              user.sets.disp_airline=(TUserSettingType)QryUserSets.FieldAsInteger(field);
            if (i==2)
              user.sets.disp_airp=(TUserSettingType)QryUserSets.FieldAsInteger(field);
            if (i==3)
              user.sets.disp_craft=(TUserSettingType)QryUserSets.FieldAsInteger(field);
            break;
          case ustEncNative:
          case ustEncLatin:
          case ustEncMixed:
            if (i==4)
              user.sets.disp_suffix=(TUserSettingType)QryUserSets.FieldAsInteger(field);
            break;
          default: ;
        };
      };

    };
  };
  if ( InitData.pr_web ) { //web
    user.sets.time=ustTimeLocalAirp;
  }
}

bool TReqInfo::tracing()
{
  if (vtracing_init) return vtracing;
  vtracing_init=true;
  vtracing=isSelfCkinClientType();
  DB::TQuery Qry(PgOra::getROSession("WEB_CLIENTS"), STDLOG);
  Qry.SQLText="SELECT tracing_search FROM web_clients WHERE desk=:desk";
  Qry.CreateVariable("desk", otString, TReqInfo::Instance()->desk.code);
  Qry.Execute();
  if (!Qry.Eof && !Qry.FieldIsNULL("tracing_search"))
    vtracing=Qry.FieldAsInteger("tracing_search")!=0;
  return vtracing;
}

void TReqInfo::traceToMonitor( TRACE_SIGNATURE, const char *format,  ...)
{
  va_list ap;
  va_start(ap,format);
  if (tracing())
    write_log_message(ERROR_PARAMS, format, ap);
   else
    write_log_message(TRACE_PARAMS, format, ap);
  va_end(ap);
}

void TProfiledRights::toXML(xmlNodePtr node) const
{
    xmlNodePtr lstNode = nullptr;
    for(const int& rightId : items)
    {
        if (rightId==191 || rightId==192) continue; //проверяем запрет этих прав только на сервере
        if(not lstNode)
            lstNode = NewTextChild(node, "profile_rights");
        NewTextChild(lstNode, "item", rightId);
    }
}

void TProfiledRights::fromDB(const AirlineCode_t &airline, const AirportCode_t &airp)
{
  if(TReqInfo::Instance()->user.user_type != utAirport) return;

  auto cur = make_db_curs(
    "SELECT profile_rights.right_id FROM airline_profiles, profile_rights "
    "WHERE airline_profiles.profile_id=profile_rights.profile_id AND "
    "      airline_profiles.airline = :airline AND "
    "      airline_profiles.airp = :airp",
    PgOra::getROSession("PROFILE_RIGHTS"));

  int rightId;

  cur.stb()
     .def(rightId)
     .bind(":airline", airline.get())
     .bind(":airp", airp.get())
     .exec();

  while (!cur.fen())
    items.insert(rightId);
}

TProfiledRights::TProfiledRights(const AirlineCode_t &airline, const AirportCode_t &airp)
{
    if(TReqInfo::Instance()->user.user_type != utAirport) return;

    fromDB(airline, airp);
}

TProfiledRights::TProfiledRights(const PointId_t& pointId)
{
    if(TReqInfo::Instance()->user.user_type != utAirport) return;

    TTripInfo flt;
    if (!flt.getByPointId(pointId.get())) return;
    if (flt.airline.empty()) return; //такое бывает, когда последний пункт в маршруте. Может надо брать а/к на прилет?
    fromDB(AirlineCode_t(flt.airline), AirportCode_t(flt.airp));
}

bool TAccess::profiledRightPermitted(const AirportCode_t &airp, const AirlineCode_t &airline, const int right_id)
{
  if (!TReqInfo::Instance()->user.access.rights().permitted(right_id)) return false;

  bool result = true;
  if(TReqInfo::Instance()->user.user_type == utAirport)
  {
    auto cur = make_db_curs(
      "SELECT profile_rights.right_id FROM airline_profiles, profile_rights "
      "WHERE airline_profiles.profile_id=profile_rights.profile_id AND "
      "      airline_profiles.airline = :airline AND "
      "      airline_profiles.airp = :airp AND "
      "      profile_rights.right_id = :right_id",
      PgOra::getROSession("PROFILE_RIGHTS"));

    int rightId;

    cur.stb()
       .def(rightId)
       .bind(":airline", airline.get())
       .bind(":airp", airp.get())
       .bind(":right_id", right_id)
       .EXfet();

    result= (cur.err() == DbCpp::ResultCode::NoDataFound);
  }
  return result;
}

bool TAccess::profiledRightPermittedForCrsPax(const PaxId_t& paxId, const int right_id)
{
    TTripInfo flt;
    if ( !flt.getByCRSPaxId(paxId.get()) ) return false;
    return profiledRightPermitted(AirportCode_t(flt.airp), AirlineCode_t(flt.airline), right_id);
}

bool TAccess::profiledRightPermitted(const PointId_t& pointId, const int right_id)
{
    TTripInfo flt;
    if ( !flt.getByPointId(pointId.get()) ) return false;
    return profiledRightPermitted(AirportCode_t(flt.airp), AirlineCode_t(flt.airline), right_id);
}

void TAccess::fromDB(int user_id, TUserType user_type)
{
  clear();
  DB::TQuery QryRoles(PgOra::getROSession({"USER_ROLES","ROLE_RIGHTS"}), STDLOG);

  //права
  QryRoles.SQLText=
    "SELECT DISTINCT role_rights.right_id "
    "FROM user_roles,role_rights "
    "WHERE user_roles.role_id=role_rights.role_id AND "
    "      user_roles.user_id=:user_id ";
  QryRoles.CreateVariable( "user_id", otInteger, user_id );
  QryRoles.Execute();
  for(;!QryRoles.Eof;QryRoles.Next())
    _rights.add_elem(QryRoles.FieldAsInteger("right_id"));

  //доступ к а/к
  DB::TQuery QryAroAL(PgOra::getROSession("ARO_AIRLINES"), STDLOG);
  QryAroAL.SQLText="SELECT airline FROM aro_airlines WHERE aro_id=:user_id";
  QryAroAL.CreateVariable("user_id", otInteger, user_id);
  QryAroAL.Execute();
  if (!QryAroAL.Eof)
    for(;!QryAroAL.Eof;QryAroAL.Next())
      _airlines.add_elem(QryAroAL.FieldAsString("airline"));
  else
    if (user_type==utSupport ||
        user_type==utAirport) _airlines.set_elems_permit(false);

  //доступ к а/п
  DB::TQuery QryAroAP(PgOra::getROSession("ARO_AIRPS"), STDLOG);
  QryAroAP.SQLText="SELECT airp FROM aro_airps WHERE aro_id=:user_id";
  QryAroAP.CreateVariable("user_id", otInteger, user_id);
  QryAroAP.Execute();
  if (!QryAroAP.Eof)
    for(;!QryAroAP.Eof;QryAroAP.Next())
      _airps.add_elem(QryAroAP.FieldAsString("airp"));
  else
    if (user_type==utSupport ||
        user_type==utAirline) _airps.set_elems_permit(false);
}

void TAccess::toXML(xmlNodePtr accessNode)
{
  if (accessNode==NULL) return;
  //права доступа к операциям
  xmlNodePtr node;
  node = NewTextChild(accessNode, "rights");
  for(set<int>::const_iterator i=_rights.elems().begin();
      i!=_rights.elems().end();++i)
    NewTextChild(node, "right", *i);
  if (_rights.elems().count(191)>0 && _rights.elems().count(192)==0)
    NewTextChild(node, "right", 192);
  //NewTextChild(accessNode, "rights_permit", (int)_rights.elems_permit());
  //права доступа к авиакомпаниям
  node = NewTextChild(accessNode, "airlines");
  for(set<string>::const_iterator i=_airlines.elems().begin();
      i!=_airlines.elems().end();++i)
    NewTextChild(node,"airline",*i);
  NewTextChild(accessNode, "airlines_permit", (int)_airlines.elems_permit());
  //права доступа к аэропортам
  node = NewTextChild(accessNode, "airps");
  for(set<string>::const_iterator i=_airps.elems().begin();
      i!=_airps.elems().end();++i)
    NewTextChild(node,"airp",*i);
  NewTextChild(accessNode, "airps_permit", (int)_airps.elems_permit());
}

void TAccess::fromXML(xmlNodePtr accessNode)
{
  clear();
  if (accessNode==NULL) return;
  xmlNodePtr node2=accessNode->children;
  //права доступа к операциям
  _rights.set_total_permit();
  //права доступа к авиакомпаниям
  xmlNodePtr node = NodeAsNodeFast("airlines", node2);
  for(node=GetNode("airline", node);node!=NULL;node=node->next)
    _airlines.add_elem(NodeAsString(node));
  if (!NodeIsNULLFast("airlines_permit", node2))
    _airlines.set_elems_permit((bool)(NodeAsIntegerFast("airlines_permit", node2)!=0));
  else
    _airlines.set_elems_permit(true);
  //права доступа к аэропортам
  node = NodeAsNodeFast("airps", node2);
  for(node=GetNode("airp", node);node!=NULL;node=node->next)
    _airps.add_elem(NodeAsString(node));
  if (!NodeIsNULLFast("airps_permit", node2))
    _airps.set_elems_permit((bool)(NodeAsIntegerFast("airps_permit", node2)!=0));
  else
    _airps.set_elems_permit(true);
}

long TReqInfo::getExecuteMSec()
{
    ptime t( microsec_clock::universal_time() );
    time_duration pt = t - execute_time;
    return pt.total_milliseconds();
}

void TReqInfo::LocaleToLog(const string &vlexema, TEventType ev_type, int id1, int id2, int id3)
{
 TLogLocale msgh;
 msgh.lexema_id = vlexema;
 msgh.ev_type = ev_type;
 msgh.id1 = id1;
 msgh.id2 = id2;
 msgh.id3 = id3;
 LocaleToLog(msgh);
}

void TReqInfo::LocaleToLog(const string &vlexema, const LEvntPrms &prms, TEventType ev_type, const std::string &sub_type, int id1, int id2, int id3)
{
 TLogLocale msgh;
 msgh.lexema_id = vlexema;
 msgh.prms = prms;
 msgh.ev_type = ev_type;
 msgh.sub_type = sub_type;
 msgh.id1 = id1;
 msgh.id2 = id2;
 msgh.id3 = id3;
 LocaleToLog(msgh);
}

void TReqInfo::LocaleToLog(const string &vlexema, const LEvntPrms &prms, TEventType ev_type, int id1, int id2, int id3)
{
 TLogLocale msgh;
 msgh.lexema_id = vlexema;
 msgh.prms = prms;
 msgh.ev_type = ev_type;
 msgh.id1 = id1;
 msgh.id2 = id2;
 msgh.id3 = id3;
 LocaleToLog(msgh);
}

void TReqInfo::LocaleToLog(TLogLocale &msg)
{
  msg.toDB(screen.name, user.descr, desk.code);
}

void TLogMsg::toDB(const std::string &screen,
                   const std::string &user_descr,
                   const std::string &desk_code,
                   const std::string &lang)
{
    LogTrace(TRACE3) << __func__;
    if(ev_time == ASTRA::NoExists) {
        ev_time = NowUTC();
    }
    if(ev_order == ASTRA::NoExists) {
        ev_order = PgOra::getSeqNextVal_int("EVENTS__SEQ");
    }

    QParams QryParams;
    QryParams << QParam("type", otString, EncodeEventType(ev_type))
              << QParam("sub_type", otString, "")
              << QParam("screen", otString, screen)
              << QParam("ev_user", otString, user_descr)
              << QParam("station", otString, desk_code)
              << QParam("msg", otString)
              << QParam("lang", otString, lang)
              << QParam("part_num", otInteger)
              << QParam("ev_time", otDate, ev_time)
              << QParam("ev_order", otInteger, ev_order);

    if(id1!=0 && id1!=NoExists)
      QryParams << QParam("id1", otInteger, id1);
    else
      QryParams << QParam("id1", otInteger, FNull);

    if(id2!=0 && id2!=NoExists)
      QryParams << QParam("id2", otInteger, id2);
    else
      QryParams << QParam("id2", otInteger, FNull);

    if(id3!=0 && id3!=NoExists)
      QryParams << QParam("id3", otInteger, id3);
    else
      QryParams << QParam("id3", otInteger, FNull);

    LogTrace(TRACE0) << "ant " << QryParams;

    DB::TCachedQuery CachedQry(PgOra::getRWSession("EVENTS_BILINGUAL"),
          "  INSERT INTO events_bilingual(type,sub_type,time,ev_order,part_num,msg,screen,ev_user,station,id1,id2,id3,lang) "
          "  VALUES(:type,:sub_type,:ev_time,:ev_order,:part_num,"
          "         :msg,:screen,:ev_user,:station,:id1,:id2,:id3,:lang) ",
          QryParams, STDLOG);

    DB::TQuery &Qry = CachedQry.get();

    int part_num = 0;
    vector<string> strs;
    SeparateString(msg.c_str(), 250, strs);
    for (const auto& i: strs) {
        Qry.SetVariable("part_num", ++part_num);
        Qry.SetVariable("msg", i);
        Qry.Execute();
    }
}

void TLogLocale::toXML(xmlNodePtr node) const
{
  if (node==NULL) return;
  xmlNodePtr eventNode=NewTextChild(node, "event");
  LocaleToXML(eventNode, lexema_id, prms);
  if (ev_time!=ASTRA::NoExists)
    NewTextChild(eventNode, "ev_time", DateTimeToStr(ev_time, ServerFormatDateTimeAsString));
  if (ev_order!=ASTRA::NoExists)
    NewTextChild(eventNode, "ev_order", ev_order);
}

void TLogLocale::fromXML(xmlNodePtr node)
{
  clear();
  if (node==NULL) return;
  xmlNodePtr eventNode=NodeAsNode("event", node);
  LocaleFromXML(eventNode, lexema_id, prms);
  xmlNodePtr node2=eventNode->children;
  ev_time=NodeAsDateTimeFast("ev_time", node2, ASTRA::NoExists);
  ev_order=NodeAsIntegerFast("ev_order", node2, ASTRA::NoExists);
}

void TLogLocale::toDB(const string &screen, const string &user_descr, const string &desk_code)
{
  ev_time = NowUTC();
  ev_order = PgOra::getSeqNextVal_int("EVENTS__SEQ");
  QParams QryParams;
  QryParams << QParam("type", otString, EncodeEventType(ev_type))
            << QParam("sub_type", otString, sub_type.substr(0, 100))
            << QParam("screen", otString, screen)
            << QParam("ev_user", otString, user_descr)
            << QParam("station", otString, desk_code)
            << QParam("msg", otString)
            << QParam("lang", otString)
            << QParam("part_num", otInteger)
            << QParam("ev_time", otDate, ev_time)
            << QParam("ev_order", otInteger, ev_order);

  if(id1!=0 && id1!=NoExists)
    QryParams << QParam("id1", otInteger, id1);
  else
    QryParams << QParam("id1", otInteger, FNull);

  if(id2!=0 && id2!=NoExists)
    QryParams << QParam("id2", otInteger, id2);
  else
    QryParams << QParam("id2", otInteger, FNull);

  if(id3!=0 && id3!=NoExists)
    QryParams << QParam("id3", otInteger, id3);
  else
    QryParams << QParam("id3", otInteger, FNull);

  DB::TCachedQuery CachedQry(PgOra::getRWSession("EVENTS_BILINGUAL"),
        "  INSERT INTO events_bilingual(type,sub_type,time,ev_order,part_num,msg,screen,ev_user,station,id1,id2,id3,lang) "
        "  VALUES(:type,:sub_type,:ev_time,:ev_order,:part_num,"
        "         :msg,:screen,:ev_user,:station,:id1,:id2,:id3,:lang) ",
        QryParams, STDLOG);

  DB::TQuery &Qry = CachedQry.get();

  for (std::vector<std::string>::iterator lang = vlangs.begin(); lang != vlangs.end(); lang++) {
    int part_num = 0;
    (*lang == AstraLocale::LANG_RU)?Qry.SetVariable("lang", AstraLocale::LANG_RU):
                                    Qry.SetVariable("lang", AstraLocale::LANG_EN);
    std::string message;
    message = AstraLocale::getLocaleText(lexema_id, prms.GetParams(*lang), *lang);
    vector<string> strs;
    SeparateString(message.c_str(), 250, strs);
    for (vector<string>::iterator i=strs.begin(); i!=strs.end(); i++) {
      Qry.SetVariable("part_num", ++part_num);
      Qry.SetVariable("msg", *i);
      Qry.Execute();
    }
  }
}

/***************************************************************************************/

TRptType DecodeRptType( const string rpt_type )
{
  int i;
  for( i=0; i<(int)rtTypeNum; i++ )
    if ( rpt_type == RptTypeS[ i ] )
      break;
  if ( i == rtTypeNum )
    return rtUnknown;
  else
    return (TRptType)i;
}

const string EncodeRptType(TRptType s)
{
  return RptTypeS[s];
};

TClientType DecodeClientType(const char* s)
{
  unsigned int i;
  for(i=0;i<sizeof(ClientTypeS)/sizeof(ClientTypeS[0]);i+=1) if (strcmp(s,ClientTypeS[i])==0) break;
  if (i<sizeof(ClientTypeS)/sizeof(ClientTypeS[0]))
    return (TClientType)i;
  else
    return ctTerm;
};

const char* EncodeClientType(TClientType s)
{
  return ClientTypeS[s];
};

TOperMode DecodeOperMode( const string mode )
{
  int i;
  for( i=0; i<(int)omTypeNum; i++ )
    if ( mode == OperModeS[ i ] )
      break;
  if ( i == omTypeNum )
    return omSTAND;
  else
    return (TOperMode)i;
}

string EncodeOperMode(const TOperMode mode )
{
  string s = OperModeS[ mode ];
  return s;
}

TEventType DecodeEventType( const string ev_type )
{
  int i;
  for( i=0; i<(int)evtTypeNum; i++ )
    if ( ev_type == EventTypeS[ i ] )
      break;
  if ( i == evtTypeNum )
    return evtUnknown;
  else
    return (TEventType)i;
}

string EncodeEventType(const TEventType ev_type )
{
  string s = EventTypeS[ ev_type ];
  return s;
}

TClass DecodeClass(const char* s)
{
  unsigned int i;
  for(i=0;i<sizeof(TClassS)/sizeof(TClassS[0]);i+=1) if (strcmp(s,TClassS[i])==0) break;
  if (i<sizeof(TClassS)/sizeof(TClassS[0]))
    return (TClass)i;
  else
    return NoClass;
};

const char* EncodeClass(TClass cl)
{
  return TClassS[cl];
};

TPerson DecodePerson(const char* s)
{
  unsigned int i;
  for(i=0;i<sizeof(TPersonS)/sizeof(TPersonS[0]);i+=1) if (strcmp(s,TPersonS[i])==0) break;
  if (i<sizeof(TPersonS)/sizeof(TPersonS[0]))
    return (TPerson)i;
  else
    return NoPerson;
};
ASTRA::TPerson DecodePerson(const std::string &s)
{
  return DecodePerson(s.c_str());
}

const char* EncodePerson(TPerson p)
{
  return TPersonS[p];
};

TQueue DecodeQueue(int q)
{
  unsigned int i;
  for(i=0;i<sizeof(TQueueS)/sizeof(TQueueS[0]);i+=1) if (q==TQueueS[i]) break;
  if (i<sizeof(TQueueS)/sizeof(TQueueS[0]))
    return (TQueue)i;
  else
    return NoQueue;
};

int EncodeQueue(TQueue q)
{
  return (int)TQueueS[q];
};

TPaxStatus DecodePaxStatus(const char* s)
{
  unsigned int i;
  for(i=0;i<sizeof(TPaxStatusS)/sizeof(TPaxStatusS[0]);i+=1) if (strcmp(s,TPaxStatusS[i])==0) break;
  if (i<sizeof(TPaxStatusS)/sizeof(TPaxStatusS[0]))
    return (TPaxStatus)i;
  else
    return psCheckin;
};
ASTRA::TPaxStatus DecodePaxStatus(const std::string &s)
{
  return DecodePaxStatus(s.c_str());
}

const char* EncodePaxStatus(TPaxStatus s)
{
  return TPaxStatusS[s];
};

TCompLayerType DecodeCompLayerType(const char* s)
{
  unsigned int i;
  for(i=0;i<sizeof(CompLayerTypeS)/sizeof(CompLayerTypeS[0]);i+=1) if (strcmp(s,CompLayerTypeS[i])==0) break;
  if (i<sizeof(CompLayerTypeS)/sizeof(CompLayerTypeS[0]))
    return (TCompLayerType)i;
  else
    return cltUnknown;
};

const char* EncodeCompLayerType(TCompLayerType s)
{
  return CompLayerTypeS[s];
};

TBagNormType DecodeBagNormType(const char* s)
{
  unsigned int i;
  for(i=0;i<sizeof(BagNormTypeS)/sizeof(BagNormTypeS[0]);i+=1) if (strcmp(s,BagNormTypeS[i])==0) break;
  if (i<sizeof(BagNormTypeS)/sizeof(BagNormTypeS[0]))
    return (TBagNormType)i;
  else
    return bntUnknown;
};

const char* EncodeBagNormType(TBagNormType s)
{
  return BagNormTypeS[s];
};

TDateTime DecodeTimeFromSignedWord( signed short int Value )
{
  int Day, Hour;
  Day = Value/1440;
  Value -= Day*1440;
  Hour = Value/60;
  Value -= Hour*60;
  TDateTime VTime;
  EncodeTime( Hour, Value, 0, VTime );
  return VTime + Day;
};

signed short int EncodeTimeToSignedWord( TDateTime Value )
{
  int Hour, Min, Sec;
  DecodeTime( Value, Hour, Min, Sec );
  return ( (int)Value )*1440 + Hour*60 + Min;
};

namespace AstraLocale {

void getLexemaText( LexemaData lexemaData, string &text, string &master_lexema_id, string lang )
{
  text.clear();
  master_lexema_id.clear();
  if ( lexemaData.lexema_id.empty() )
    return;
  try {
      buildMsg( (lang.empty() ? TReqInfo::Instance()->desk.lang : lang), lexemaData, text, master_lexema_id );
    }
  catch( std::exception &e ) {
    text = lexemaData.lexema_id;
    //ProgError( STDLOG, "showError buildMsg e.what()=%s, id=%s, lang=%s",
               //e.what(), lexemaData.lexema_id.c_str(), TReqInfo::Instance()->desk.lang.c_str() );
  }
  catch( ... ) {
    text = lexemaData.lexema_id;
    ProgError( STDLOG, "Unknown Exception on buildMsg!!!" );
  }
}

std::string getLocaleText(const LexemaData &lexemaData, const std::string &lang)
{
    string text, master_lexema_id;
    getLexemaText( lexemaData, text, master_lexema_id, lang );
    return text;
}

string getLocaleText(const std::string &vlexema, const std::string &lang)
{
    LexemaData lexemaData;
    lexemaData.lexema_id = vlexema;
    string text, master_lexema_id;
    getLexemaText( lexemaData, text, master_lexema_id, lang );
    return text;
}

string getLocaleText(const std::string &vlexema, const LParams &aparams, const string &lang)
{
    LexemaData lexemaData;
    lexemaData.lexema_id = vlexema;
    lexemaData.lparams = aparams;
    string text, master_lexema_id;
    getLexemaText( lexemaData, text, master_lexema_id, lang );
    return text;
}

void showError(const LexemaData &lexemaData, int code)
{
  string text, master_lexema_id;
  getLexemaText( lexemaData, text, master_lexema_id );
  XMLRequestCtxt *xmlRC = getXmlCtxt();
  if (xmlRC->resDoc!=NULL)
  {
    xmlNodePtr resNode = NodeAsNode("/term/answer", xmlRC->resDoc);
    resNode = ReplaceTextChild( ReplaceTextChild( resNode, "command" ), "user_error", text );
    SetProp(resNode, "lexema_id", master_lexema_id);
    SetProp(resNode, "code", code);
  };
};

void showError(const std::string &lexema_id, int code)
{
    LexemaData lexemaData;
    lexemaData.lexema_id = lexema_id;
    showError( lexemaData, code );
}

void showError(const std::string &vlexema, const LParams &aparams, int code)
{
  LexemaData lexemaData;
    lexemaData.lexema_id = vlexema;
    lexemaData.lparams = aparams;
  showError( lexemaData, code );
}

void showErrorMessage(const LexemaData &lexemaData, int code)
{
  string text, master_lexema_id;
  getLexemaText( lexemaData, text, master_lexema_id );
  XMLRequestCtxt *xmlRC = getXmlCtxt();
  if (xmlRC->resDoc!=NULL)
  {
    xmlNodePtr resNode = NodeAsNode("/term/answer", xmlRC->resDoc);
    resNode =  ReplaceTextChild( ReplaceTextChild( resNode, "command" ), "user_error_message", text );
    SetProp(resNode, "lexema_id", master_lexema_id);
    SetProp(resNode, "code", code);
  };
}

void showErrorMessage(const std::string &lexema_id, int code)
{
    LexemaData lexemaData;
    lexemaData.lexema_id = lexema_id;
    showErrorMessage( lexemaData, code );
}

void showErrorMessage( const std::string &vlexema, const LParams &aparams, int code)
{
    LexemaData lexemaData;
    lexemaData.lexema_id = vlexema;
    lexemaData.lparams = aparams;
  showErrorMessage( lexemaData, code );
}

void showMessage(const LexemaData &lexemaData, int code)
{
  string text, master_lexema_id;
  getLexemaText( lexemaData, text, master_lexema_id );
  XMLRequestCtxt *xmlRC = getXmlCtxt();
  if (xmlRC->resDoc!=NULL)
  {
    xmlNodePtr resNode = NodeAsNode("/term/answer", xmlRC->resDoc);
    resNode = ReplaceTextChild( ReplaceTextChild( resNode, "command" ), "message", text );
    SetProp(resNode, "lexema_id", master_lexema_id);
    SetProp(resNode, "code", code);
  };
}

void showMessage(const std::string &lexema_id, int code)
{
    LexemaData lexemaData;
    lexemaData.lexema_id = lexema_id;
    showMessage( lexemaData, code );
}

void showMessage( const std::string &vlexema, const LParams &aparams, int code)
{
    LexemaData lexemaData;
    lexemaData.lexema_id = vlexema;
    lexemaData.lparams = aparams;
  showMessage( lexemaData, code );
}

void showProgError(const LexemaData &lexemaData, int code)
{
  string text, master_lexema_id;
  getLexemaText( lexemaData, text, master_lexema_id );
  XMLRequestCtxt *xmlRC = getXmlCtxt();
  if (xmlRC->resDoc!=NULL)
  {
    xmlNodePtr resNode = NodeAsNode("/term/answer", xmlRC->resDoc);
    resNode = ReplaceTextChild( ReplaceTextChild( resNode, "command" ), "error", text );
    SetProp(resNode, "lexema_id", master_lexema_id);
    SetProp(resNode, "code", code);
  };
};

void showProgError(const std::string &lexema_id, int code)
{
    LexemaData lexemaData;
    lexemaData.lexema_id = lexema_id;
    showProgError( lexemaData, code );
}

void showProgError(const std::string &vlexema, const LParams &aparams, int code)
{
  LexemaData lexemaData;
    lexemaData.lexema_id = vlexema;
    lexemaData.lparams = aparams;
  showProgError( lexemaData, code );
}

void showErrorMessageAndRollback(const LexemaData &lexemaData, int code)
{
  showErrorMessage(lexemaData,code);
  throw UserException2();
}

void showErrorMessageAndRollback(const std::string &lexema_id, int code)
{
    LexemaData lexemaData;
    lexemaData.lexema_id = lexema_id;
    showErrorMessageAndRollback( lexemaData, code );
}

void showErrorMessageAndRollback(const std::string &vlexema, const LParams &aparams, int code)
{
  LexemaData lexemaData;
    lexemaData.lexema_id = vlexema;
    lexemaData.lparams = aparams;
  showErrorMessageAndRollback( lexemaData, code );
}

std::string getLocaleText(xmlNodePtr lexemeNode)
{
  if (lexemeNode==NULL) return "";
  LexemaData lexemeData;
  LexemeDataFromXML(lexemeNode, lexemeData);
  return getLocaleText(lexemeData);
}

static const std::map< std::string, std::list<std::string> > LexemeDataToBeLocalized = {
    { "MSG.TABLE.NOT_SET_FIELD_VALUE",        { "fieldname" } },
    { "MSG.TABLE.INVALID_FIELD_VALUE",        { "fieldname" } },
    { "MSG.FIELD_INCLUDE_INVALID_CHARACTER1", { "field_name" } },
    { "MSG.CODE_ALREADY_USED_FOR_AIRLINE",    { "field1", "field2" } },
    { "MSG.CODE_ALREADY_USED_FOR_COUNTRY",    { "field1", "field2" } },
    { "MSG.CODE_ALREADY_USED_FOR_CITY",       { "field1", "field2" } },
    { "MSG.CODE_ALREADY_USED_FOR_AIRPORT",    { "field1", "field2" } },
    { "MSG.CODE_ALREADY_USED_FOR_AIRCRAFT",   { "field1", "field2" } },
};

void LexemeDataToXML(const LexemaData &lexemeData, xmlNodePtr lexemeNode)
{
  if (lexemeNode==NULL) return;
  NewTextChild(lexemeNode,"id",lexemeData.lexema_id);
  xmlNodePtr node=NewTextChild(lexemeNode,"params");
  for(LParams::const_iterator p=lexemeData.lparams.begin();p!=lexemeData.lparams.end();p++)
  {
    NewTextChild(node,p->first.c_str(),lexemeData.lparams.StringValue(p->first));
  }
}

void LexemeDataFromXML(xmlNodePtr lexemeNode, LexemaData &lexemeData)
{
  lexemeData.lexema_id.clear();
  lexemeData.lparams.clear();
  if (lexemeNode==NULL) return;
  lexemeData.lexema_id=NodeAsString("id",lexemeNode);
  xmlNodePtr node=NodeAsNode("params",lexemeNode)->children;
  const auto additionalLocalization = algo::find_opt<std::optional>(LexemeDataToBeLocalized,
                                                                    lexemeData.lexema_id);
  for(;node!=NULL;node=node->next)
  {
    std::string param_name((const char*)node->name);
    std::string param_value = NodeAsString(node);
    // дополнительная локализация, которая делалась в oracle в пакете ADM
    if(additionalLocalization && algo::contains(*additionalLocalization, param_name)) {
        param_value = AstraLocale::getLocaleText(param_value);
    }
    lexemeData.lparams << LParam(param_name, param_value);
  }
}

xmlNodePtr selectPriorityMessage(xmlNodePtr resNode, std::string& error_code, std::string& error_message)
{
  // если есть тег <error> || <checkin_user_error> || <user_error>, то все остальное удаляем их из xml дерева
  xmlNodePtr errNode=NULL;
  int errPriority=ASTRA::NoExists;
  for(xmlNodePtr node=resNode->children; node!=NULL; node=node->next)
  {
    if (strcmp((const char*)node->name,"command")==0)
    {
      for(xmlNodePtr cmdNode=node->children; cmdNode!=NULL; cmdNode=cmdNode->next)
      {
        int priority=ASTRA::NoExists;
        if (strcmp((const char*)cmdNode->name,"error")==0) priority=1;
        if (strcmp((const char*)cmdNode->name,"checkin_user_error")==0) priority=2;
        if (strcmp((const char*)cmdNode->name,"user_error")==0) priority=3;
        if (priority!=ASTRA::NoExists &&
          (errPriority==ASTRA::NoExists || priority<errPriority))
        {
          errNode=cmdNode;
          errPriority = priority;
        };
      };
    };
  };

  if (errNode!=NULL)
  {
    if (strcmp((const char*)errNode->name,"error")==0 ||
        strcmp((const char*)errNode->name,"user_error")==0)
    {
      error_message = NodeAsString(errNode);
      error_code = NodeAsString( "@lexema_id", errNode, "" );
    };
    if (strcmp((const char*)errNode->name,"checkin_user_error")==0)
    {
      xmlNodePtr segNode=NodeAsNode("segments",errNode)->children;
      for(;segNode!=NULL; segNode=segNode->next)
      {
        if (GetNode("error_code",segNode)!=NULL)
        {
          error_message = NodeAsString("error_message", segNode, "");
          error_code = NodeAsString("error_code", segNode);
          break;
        };
        xmlNodePtr paxNode=NodeAsNode("passengers",segNode)->children;
        for(;paxNode!=NULL; paxNode=paxNode->next)
        {
          if (GetNode("error_code",paxNode)!=NULL)
          {
            error_message = NodeAsString("error_message", paxNode, "");
            error_code = NodeAsString("error_code", paxNode);
            break;
          };
        };
        if (paxNode!=NULL) break;
      };
    };
    //отцепляем
    xmlUnlinkNode(errNode);
  };

  for(xmlNodePtr node=resNode->children; node!=NULL;)
  {
      //отцепляем и удаляем либо все, либо <command> внутри <answer>
    xmlNodePtr node2=node->next;
    if (errNode!=NULL || strcmp((const char*)node->name,"command")==0)
    {
      xmlUnlinkNode(node);
      xmlFreeNode(node);
    };
    node=node2;
  };
  return errNode;
};

} // end namespace AstraLocale

int getTCLParam(const char* name, int min, int max, int def)
{
  int res=NoExists;
  try
  {
    string r=readStringFromTcl( name, "");
    if ( r.empty() )
      throw EXCEPTIONS::Exception( "Can't read TCL param %s", name );
    if ( StrToInt(r.c_str(),res)==EOF ||
         (min!=NoExists && res<min) ||
         (max!=NoExists && res>max))
      throw EXCEPTIONS::Exception( "Wrong TCL param %s=%s", name, r.c_str() );
  }
  catch(EXCEPTIONS::Exception &e)
  {
    if (def==NoExists) throw;
    res=def;
    ProgTrace( TRACE0, "%s", e.what() );
  };

  ProgTrace( TRACE5, "TCL param %s=%d", name, res );
  return res;
};

//если def==NULL, тогда в случае ненахождения name ругаемся
string getTCLParam(const char* name, const char* def)
{
  string res;
  try
  {
    string r=readStringFromTcl( name, "");
    if ( r.empty() )
      throw EXCEPTIONS::Exception( "Can't read TCL param %s", name );
    res=r;
  }
  catch(EXCEPTIONS::Exception &e)
  {
    if (def==NULL) throw;
    res=def;
    ProgTrace( TRACE0, "%s", e.what() );
  };

  ProgTrace( TRACE5, "TCL param %s='%s'", name, res.c_str() );
  return res;
};

bool get_enable_unload_pectab()
{
  static int VAR=NoExists;
  if (VAR==NoExists)
    VAR=getTCLParam("ENABLE_UNLOAD_PECTAB",0,1,0);
  return VAR!=0;
}

bool get_test_server()
{
  static int VAR=NoExists;
  if (VAR==NoExists)
    VAR=getTCLParam("TEST_SERVER",0,1,1);
  return VAR!=0;
}

bool DEMO_MODE()
{
  static int VAR=NoExists;
  if (VAR==NoExists)
    VAR=getTCLParam("DEMO_MODE",0,1,0);
  return VAR!=0;
}

bool get_enable_fr_design()
{
  static int VAR=NoExists;
  if (VAR==NoExists)
    VAR=getTCLParam("ENABLE_FR_DESIGN",0,1,0);
  return VAR!=0;
}

const char* OWN_POINT_ADDR()
{
  static string VAR;
  if ( VAR.empty() )
    VAR=getTCLParam("OWN_POINT_ADDR",NULL);
  return VAR.c_str();
};

const char* SERVER_ID()
{
  static string VAR;
  if ( VAR.empty() )
    VAR=getTCLParam("SERVER_ID",NULL);
  return VAR.c_str();
};

int ARX_TRIP_DATE_RANGE()
{
  static int VAR=NoExists;
  if (VAR==NoExists)
    VAR=getTCLParam("ARX_TRIP_DATE_RANGE",1,999,1);
  return VAR;
};

const char* ORDERS_PATH()
{
  static string VAR;
  if ( VAR.empty() )
    VAR=getTCLParam("ORDERS_PATH","stat_orders/");
  return VAR.c_str();
};

int ORDERS_TIMEOUT()
{
    static int VAR = NoExists;
    if(VAR == NoExists) {
        VAR = getTCLParam("ORDERS_TIMEOUT", NoExists, NoExists, 3);
    }
    return VAR;
}

int ORDERS_BLOCK_SIZE()
{
    static int VAR = NoExists;
    if(VAR == NoExists) {
        string param_val = getTCLParam("ORDERS_BLOCK_SIZE", "1M");
        VAR = (size_t)round(getFileSizeDouble(param_val)); // приведение double к size_t в случае переполнения приведет к 0
        if(VAR == 0) // type overflow occurred
            throw EXCEPTIONS::Exception("wrong tcl param ORDERS_BLOCK_SIZE '%s'", param_val.c_str());
    }
    return VAR;
}

double ORDERS_MAX_SIZE()
{
    static double VAR = NoExists;
    if(VAR == NoExists)
        VAR = getFileSizeDouble(getTCLParam("ORDERS_MAX_SIZE", "10G"));
    return VAR;
}

double ORDERS_MAX_TOTAL_SIZE()
{
    static double VAR = NoExists;
    if(VAR == NoExists)
        VAR = getFileSizeDouble(getTCLParam("ORDERS_MAX_TOTAL_SIZE", "1T"));
    return VAR;
}

int ARX_EVENTS_DISABLED()
{
  static int VAR=NoExists;
  if (VAR==NoExists)
    VAR=getTCLParam("ARX_EVENTS_DISABLED",0,1,0);
  return VAR;
};

void showBasicInfo(void)
{
  XMLRequestCtxt *xmlRC = getXmlCtxt();
  TReqInfo *reqInfo = TReqInfo::Instance();
  if (xmlRC->resDoc==NULL) return;
  xmlNodePtr resNode = NodeAsNode("/term/answer", xmlRC->resDoc);
  xmlNodePtr node = GetNode("basic_info", resNode);
  if (node!=NULL)
  {
    xmlUnlinkNode(node);
    xmlFreeNode(node);
  };
  resNode = NewTextChild(resNode,"basic_info");
  NewTextChild(resNode, "enable_fr_design", get_enable_fr_design());
  NewTextChild(resNode, "enable_unload_pectab", get_enable_unload_pectab());

  NewTextChild(resNode, "server_id", SERVER_ID() );

  if (!reqInfo->user.login.empty())
  {
    node = NewTextChild(resNode,"user");
    NewTextChild(node, "login",reqInfo->user.login);
    NewTextChild(node, "type",reqInfo->user.user_type);
    //настройки пользователя
    xmlNodePtr setsNode = NewTextChild(node, "settings");
    NewTextChild(setsNode, "time", reqInfo->user.sets.time);
    NewTextChild(setsNode, "disp_airline", reqInfo->user.sets.disp_airline);
    NewTextChild(setsNode, "disp_airp", reqInfo->user.sets.disp_airp);
    NewTextChild(setsNode, "disp_craft", reqInfo->user.sets.disp_craft);
    NewTextChild(setsNode, "disp_suffix", reqInfo->user.sets.disp_suffix);
    bool advanced_trip_list=
           !(reqInfo->user.user_type==utAirport && !reqInfo->user.access.rights().permitted(335));
    NewTextChild(setsNode, "advanced_trip_list", (int)advanced_trip_list);

    //доступ
    reqInfo->user.access.toXML(NewTextChild(node, "access"));
  };
  if (!reqInfo->desk.code.empty())
  {
    node = NewTextChild(resNode,"desk");
    NewTextChild(node,"city",reqInfo->desk.city);
    NewTextChild(node,"currency",reqInfo->desk.currency);
    NewTextChild(node,"time",DateTimeToStr( reqInfo->desk.time ) );
    NewTextChild(node,"time_utc",DateTimeToStr(NowUTC()) );
    //настройки пульта
    xmlNodePtr setsNode = NewTextChild(node, "settings");
    NewTextChild(setsNode,"defer_etstatus",(int)false);  //устаревшая технология отложенного подтверждения ЭБ удалена

    DB::TQuery QryLog(PgOra::getROSession("DESK_LOGGING"), STDLOG);
    QryLog.SQLText="SELECT file_size,send_size,send_portion,backup_num "
                   "FROM desk_logging WHERE desk=:desk";
    QryLog.CreateVariable("desk",otString,reqInfo->desk.code);
    QryLog.Execute();
    if (!QryLog.Eof)
    {
      xmlNodePtr loggingNode=NewTextChild(node,"logging");
      NewTextChild(loggingNode,"file_size",QryLog.FieldAsInteger("file_size"));
      NewTextChild(loggingNode,"send_size",QryLog.FieldAsInteger("send_size"));
      NewTextChild(loggingNode,"send_portion",QryLog.FieldAsInteger("send_portion"));
      NewTextChild(loggingNode,"backup_num",QryLog.FieldAsInteger("backup_num"));
      xmlNodePtr rangesNode=NewTextChild(loggingNode,"trace_ranges");
      DB::TQuery QryTrace(PgOra::getROSession("DESK_TRACES"), STDLOG);
      QryTrace.SQLText="SELECT first_trace,last_trace FROM desk_traces WHERE desk=:desk";
      QryTrace.CreateVariable("desk",otString,reqInfo->desk.code);
      QryTrace.Execute();
      int trace_level=-1;
      for(;!QryTrace.Eof;QryTrace.Next())
      {
        xmlNodePtr rangeNode=NewTextChild(rangesNode,"range");
        NewTextChild(rangeNode,"first_trace",QryTrace.FieldAsInteger("first_trace"));
        NewTextChild(rangeNode,"last_trace",QryTrace.FieldAsInteger("last_trace"));

        if (trace_level<QryTrace.FieldAsInteger("last_trace"))
          trace_level=QryTrace.FieldAsInteger("last_trace");
      };
      if (trace_level>=0) NewTextChild(node,"trace_level",trace_level);
    };
  };
};

/***************************************************************************************/
void SysReqInterface::ClientError(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  ErrorToLog(ctxt, reqNode, resNode);
};

void SysReqInterface::ErrorToLog(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  if (reqNode==NULL) return;

  DB::TQuery Qry(PgOra::getROSession({"CLIENT_ERROR_LIST","LOCALE_MESSAGES"}), STDLOG);
  Qry.SQLText =
      "SELECT client_error_list.type "
      "FROM client_error_list "
      "  LEFT OUTER JOIN locale_messages "
      "    ON client_error_list.text = locale_messages.id "
      "WHERE ( "
      "  :text LIKE client_error_list.text "
      "  OR ( "
      "    locale_messages.text IS NOT NULL "
      "    AND :text LIKE '%'||locale_messages.text||'%' "
      "  ) "
      ") ";
  Qry.DeclareVariable("text",otString);

  xmlNodePtr node=reqNode->children;
  for(;node!=NULL;node=node->next)
  {
    if (strcmp((const char*)node->name,"msg")==0)
    {
      string error_type="ERROR";
      if (TReqInfo::Instance()->desk.code.substr(0, 3)=="UFA")
        error_type="TRACE5";
      string text=NodeAsString(node);
      Qry.SetVariable("text", text.substr(0,250));
      Qry.Execute();
      if (!Qry.Eof) error_type=Qry.FieldAsString("type");

      if (error_type=="IGNORE") continue;

      if (error_type=="ERROR")
        ProgError( STDLOG, "Client error (ver. %s): %s.",
                           TReqInfo::Instance()->desk.version.c_str(),
                           text.c_str() ) ;
      else
        if (error_type=="TRACE0")
          ProgTrace( TRACE0, "Client error (ver. %s): %s.",
                             TReqInfo::Instance()->desk.version.c_str(),
                             text.c_str() ) ;
        else
          ProgTrace( TRACE5, "Client error (ver. %s): %s.",
                             TReqInfo::Instance()->desk.version.c_str(),
                             text.c_str() ) ;
    };
  };
}

const std::string& AirpTZRegion(string airp, bool with_exception)
{
  if (airp.empty()) throw EXCEPTIONS::Exception("Airport not specified");
  const TAirpsRow& row=(const TAirpsRow&)base_tables.get("airps").get_row("code",airp,true);
  return CityTZRegion(row.city,with_exception);
};

const std::string &CityTZRegion(string city, bool with_exception)
{
  if (city.empty()) throw EXCEPTIONS::Exception("City not specified");
  const TCitiesRow& row=(const TCitiesRow&)base_tables.get("cities").get_row("code",city,true);
  if (row.tz_region.empty() && with_exception)
    throw AstraLocale::UserException("MSG.CITY.REGION_NOT_DEFINED",LParams() << LParam("city", city));
  return row.tz_region;
};

string DeskCity(string desk, bool with_exception)
{
  if (desk.empty()) throw EXCEPTIONS::Exception("Desk not specified");
  DB::TQuery Qry(PgOra::getROSession({"DESK_GRP","DESKS"}), STDLOG);
  Qry.SQLText =
    "SELECT desk_grp.city "
    "FROM desk_grp,desks "
    "WHERE desks.code = :desk AND desks.grp_id = desk_grp.grp_id";
  Qry.CreateVariable("desk", otString, desk);
  Qry.Execute();
  if(Qry.Eof)
  {
    if (with_exception)
      throw AstraLocale::UserException("MSG.DESK.NOT_FOUND", LParams() << LParam("desk", desk));
    else
      return "";
  }
  return Qry.FieldAsString("city");
};

CityCode_t getCityByAirp(const AirportCode_t& airp)
{
  const TAirpsRow &airpRow = dynamic_cast<const TAirpsRow&>(base_tables.get("airps").get_row("code",airp.get()));
  return CityCode_t(airpRow.city);
}

TCountriesRow getCountryByAirp( const std::string& airp)
{
  const TAirpsRow &airpRow = (const TAirpsRow&)base_tables.get("airps").get_row("code",airp);
  const TCitiesRow &cityRow = (const TCitiesRow&)base_tables.get("cities").get_row("code",airpRow.city);
  return ((const TCountriesRow&)base_tables.get("countries").get_row("code",cityRow.country));
}

CountryCode_t getCountryByAirp(const AirportCode_t& airp)
{
  return CountryCode_t(getCountryByAirp(airp.get()).code);
}

class TTranslitLetter
{
  public:
    string lat1, lat2, lat3;
    TTranslitLetter(const char* l1, const char* l2, const char* l3):lat1(l1), lat2(l2), lat3(l3) {}
    TTranslitLetter() {}
};

string transliter(const string &value, TranslitFormat fmt, bool pr_lat)
{
  string result;
  if (pr_lat)
  {
    static map<char, TTranslitLetter> dicts;
    if (dicts.empty())
    {
      TQuery Qry(&OraSession);
      Qry.Clear();
      Qry.SQLText = "SELECT letter, lat1, lat2, lat3 FROM translit_dicts";
      Qry.Execute();
      for(;!Qry.Eof;Qry.Next())
      {
        const char* letter=Qry.FieldAsString("letter");
        dicts[*letter]=TTranslitLetter(Qry.FieldAsString("lat1"),
                                       Qry.FieldAsString("lat2"),
                                       Qry.FieldAsString("lat3"));
      };
      ProgTrace(TRACE5, "dicts loaded");
    };

    for(string::const_iterator i=value.begin();i!=value.end();i++)
    {
      char c=*i;
      char uc=ToUpper(*i);
      string c2;
      if (!IsAscii7(c))
      {
        if (fmt == TranslitFormat::V3)
        {
          string::const_iterator i2=i+1;
          if (i2!=value.end())
          {
            char uc2=ToUpper(*i2);
            if (uc=='К' && uc2=='С') { c2="X"; i=i2; };
            if (uc=='И' && uc2=='Й' && (i2+1)==value.end()) { c2="Y"; i=i2; };
          };
          if (uc=='Ч' && i!=value.begin() && string("АЕЁИОУЫЭЮЯ").find_first_of(ToUpper(*(i-1)))!=string::npos) c2="TCH";
          if (uc=='У' && i==value.begin()) c2="U";
        };
        if (c2.empty())
        {
          map<char, TTranslitLetter>::const_iterator letter=dicts.find(uc);
          if (letter!=dicts.end())
          {
            if      (fmt==TranslitFormat::V3) c2=letter->second.lat3;
            else if (fmt==TranslitFormat::V2) c2=letter->second.lat2;
            else if (fmt==TranslitFormat::V1) c2=letter->second.lat1;
            else             c2=uc;
          }
          else
            c2 = "?";
        };
        if (uc!=c) result+=lowerc(c2); else result+=c2;
      }
      else result+=c;
    };
  }
  else  result=value;
  return result;
}

bool transliter_equal(const string &value1, const string &value2, TranslitFormat fmt)
{
  return transliter(value1, fmt, true)==transliter(value2, fmt, true);
}

bool transliter_equal(const string &value1, const string &value2)
{
  for(int fmt=int(TranslitFormat::V1);fmt<=int(TranslitFormat::V3);fmt++)
    if (transliter_equal(value1, value2, TranslitFormat(fmt))) return true;
  return false;
}

bool transliter_equal_begin(const string &str, const string &substr, TranslitFormat fmt)
{
  std::string v1(transliter(str,    fmt, true));
  std::string v2(transliter(substr, fmt, true));
  size_t minSize=v2.size();
  return v1.substr(0, minSize)==v2.substr(0, minSize);
}

bool transliter_equal_begin(const string &str, const string &substr)
{
  for(int fmt=int(TranslitFormat::V1);fmt<=int(TranslitFormat::V3);fmt++)
    if (transliter_equal_begin(str, substr, TranslitFormat(fmt))) return true;
  return false;
}

int best_transliter_similarity(const string &value1, const string &value2)
{
  if (value1==value2) return 100;

  int result=EditDistanceSimilarity(value1, value2);
  for(int fmt=int(TranslitFormat::V1);fmt<=int(TranslitFormat::V3);fmt++)
  {
    if (result>=100) return result;
    int i=EditDistanceSimilarity(transliter(value1, TranslitFormat(fmt), true),
                                 transliter(value2, TranslitFormat(fmt), true));
    if (i>result) result=i;
  }
  return result;
}

const char *rus_char_view = "АВСЕНКМОРТХасекморхь";
const char *lat_char_view = "ABCEHKMOPTXacekmopxb";

char ToLatCharView(char c)
{
  if (!IsAscii7(c))
  {
    ByteReplace(&c,1,rus_char_view,lat_char_view);
  };
  return c;
};

string convert_char_view(const string &value, bool pr_lat)
{
  string result = value;
  if (pr_lat)
    transform(result.begin(), result.end(), result.begin(), ToLatCharView);
  return result;

};

AstraLocale::UserException EOracleError2UserException(string& msg)
{
  //ProgTrace(TRACE5,"EOracleError2UserException: msg=%s",msg.c_str());
  size_t p;
  if (msg.substr( 0, 4 ) == "ORA-")
  {
    p = msg.find( ": " );
    if ( p != string::npos )
    {
      //отрезаем ORA- первой строки
      msg.erase( 0, p+2 );
    };
  };
  //ищем следующую строку, начинающуюся с ORA-
  p=0;
  while( (p = msg.find_first_of("\n\r",p)) != string::npos )
  {
    p++;
    if (msg.substr( p, 4 ) == "ORA-")
    {
      msg.erase( p );
      break;
    };
  };

  string msgXML=ConvertCodepage(msg,"CP866","UTF-8");
  XMLDoc msgDoc(msgXML);
  if (msgDoc.docPtr()!=NULL)
  {
    ProgTrace(TRACE5,"EOracleError2UserException: msg=%s",msg.c_str());
    xml_decode_nodelist(msgDoc.docPtr()->children);

    LexemaData lexemeData;
    LexemeDataFromXML(NodeAsNode("/lexeme_data",msgDoc.docPtr()), lexemeData);
    return AstraLocale::UserException(lexemeData.lexema_id, lexemeData.lparams);
  }
  return AstraLocale::UserException(msg);
}

bool isIgnoredEOracleError(const std::exception& e)
{
  const EOracleError *orae=dynamic_cast<const EOracleError*>(&e);
  return (orae!=NULL&&
          (orae->Code==4061 ||
           orae->Code==4068 ||
           orae->Code==60));
}

bool clearResponseAndRollbackIfDeadlock(const std::exception& e, xmlNodePtr resNode)
{
  const EOracleError *orae=dynamic_cast<const EOracleError*>(&e);
  if (orae!=nullptr && orae->Code==60)
  {
    RemoveChildNodes(resNode);
    ASTRA::rollback();
    return true;
  }
  return false;
}

string get_internal_msgid_hex()
{
  string str_msg_id((const char*)get_internal_msgid(),sizeof(int)*3);
  string hex_msg_id;
  StringToHex(str_msg_id,hex_msg_id);
  return hex_msg_id;
};

namespace ASTRA
{

void dumpTable(const std::string& table,
               int loglevel, const char* nick, const char* file, int line)
{
#ifdef ENABLE_ORACLE
    OciCpp::DumpTable dt(table);
    dt.exec(loglevel, nick, file, line);
#endif // ENABLE_ORACLE
}

void commit_(bool withCommitHooks)
{
    LogTrace(TRACE3) << "ASTRA::commit(" << (withCommitHooks ? "with commit hooks"
                                                             : "without commit hooks") << ")";
#ifdef XP_TESTING
    if(inTestMode()) {
        ::commit(); return;
    }
#endif//XP_TESTING

    OraSession.Commit();
    ::commit();
    if(withCommitHooks) {
        callPostHooksCommit();
    }
}

void rollback_(bool withRollbackHooks)
{
    LogTrace(TRACE3) << "ASTRA::rollback(" << (withRollbackHooks ? "with rollback hooks"
                                                                 : "without rollback hooks") << ")";
#ifdef XP_TESTING
    if(inTestMode()) {
        ::rollback(); return;
    }
#endif//XP_TESTING

    OraSession.Rollback();
    ::rollback();
    if(withRollbackHooks) {
        callRollbackPostHooks();
    }
}

void commit()
{
    commit_(false/*DO NOT call commit hooks*/);
}

void rollback()
{
    rollback_(false/*DO NOT call rollback hooks*/);
}

void commitAndCallCommitHooks()
{
    commit_(true/*call commit hooks*/);
}

void rollbackAndCallRollbackHooks()
{
    rollback_(true/*call rollback hooks*/);
}

void rollbackSavePax()
{
    LogTrace(TRACE3) << "ASTRA::rollbackSavePax()";
    DB::execSpCmd("rollback to savepoint sp_savepax");
}

void beforeSoftError()
{
    const std::string sp_name("SavePointNum1");
    DB::execSpCmd("savepoint " + sp_name);
    ProgTrace(TRACE1, "Making savepoint - %s", sp_name.c_str());
}

void afterSoftError()
{
    std::string sp_name("SavePointNum1");
    DB::execSpCmd("rollback to savepoint " + sp_name);
    ProgTrace(TRACE1, "RollBacking to savepoint - %s", sp_name.c_str());
}

tlgnum_t make_tlgnum(int n)
{
  return tlgnum_t(std::to_string(n));
}

XMLDoc createXmlDoc(const std::string& xml)
{
    XMLDoc doc;
    doc.set(ConvertCodepage(xml, "CP866", "UTF-8"));
    if(doc.docPtr() == NULL) {
        throw EXCEPTIONS::Exception("document %s has wrong XML format", xml.c_str());
    }
    xml_decode_nodelist(doc.docPtr()->children);
    return doc;
}

XMLDoc createXmlDoc2(const std::string& xml)
{
    XMLDoc doc;
    doc.set(xml);
    if(doc.docPtr() == NULL) {
        throw EXCEPTIONS::Exception("document %s has wrong XML format", xml.c_str());
    }
    xml_decode_nodelist(doc.docPtr()->children);
    return doc;
}

void syncHistory(const string& table_name, int id,
                 const std::string& sys_user_descr, const std::string& sys_desk_code)
{
  if(DEMO_MODE()) {
      // В ДЕМО НЕ ВЫЗЫВАЕМ ФУНКЦИИ ПАКЕТОВ ДО ИХ ПЕРЕВОДА НА C++
      TST();
      return;
  }
  TQuery Qry(&OraSession, STDLOG);
  Qry.SQLText =
      "BEGIN "
      "  hist.synchronize_history(:table_name,:id,:sys_user_descr,:sys_desk_code); "
      "END;";
  Qry.CreateVariable("table_name", otString, table_name);
  Qry.CreateVariable("id", otInteger, id);
  Qry.CreateVariable("sys_user_descr", otString, sys_user_descr);
  Qry.CreateVariable("sys_desk_code", otString, sys_desk_code);
  try {
      Qry.Execute();
  } catch(EOracleError &E) {
      if (E.Code >= 20000) {
          std::string str = E.what();
          EOracleError2UserException(str);
          throw UserException(str);
      } else {
          throw;
      }
  }
}

}// namespace ASTRA

void TRegEvents::fromArxDB(TDateTime part_key, int point_id)
{
    LogTrace5 << __FUNCTION__ << " part_key: " << DateTimeToBoost(part_key) << " point_id: " << point_id;
    if(ARX_EVENTS_DISABLED()) {
        throw AstraLocale::UserException("MSG.ERR_MSG.ARX_EVENTS_DISABLED");
    }
    int grp_id, reg_no;
    Dates::DateTime_t ckin_time, brd_time;
    std::string posStrCkinTime, posStrBrdTime;
    if(ARX::READ_PG()) {
        posStrCkinTime = " STRPOS(msg,'зарегистрирован') ";
        posStrBrdTime = " STRPOS(msg,'прошел посадку') ";
    } else {
        posStrCkinTime = " INSTR(msg,'зарегистрирован') ";
        posStrBrdTime = " INSTR(msg,'прошел посадку') ";
    }
    auto cur = make_db_curs(
        "SELECT id3 AS grp_id, id2 AS reg_no, "
        "  MIN(CASE " + posStrCkinTime + " WHEN 0 THEN NULL ELSE time END) AS ckin_time, "
        "  MAX(CASE " + posStrBrdTime + " WHEN 0 THEN NULL ELSE time END) AS brd_time "
        "FROM arx_events "
        "WHERE type=:evtPax AND part_key=:part_key AND (lang=:lang OR lang='ZZ') AND id1=:point_id AND "
        "      (msg like '%зарегистрирован%' OR msg like '%прошел посадку%') "
        "GROUP BY id3, id2",
        PgOra::getROSession("ARX_EVENTS"));
    cur
       .stb()
       .defNull(grp_id, ASTRA::NoExists)
       .defNull(reg_no, ASTRA::NoExists)
       .defNull(ckin_time, Dates::not_a_date_time) //
       .defNull(brd_time, Dates::not_a_date_time) //
       .bind(":lang", AstraLocale::LANG_RU)
       .bind(":evtPax", EncodeEventType(ASTRA::evtPax))
       .bind(":point_id", point_id)
       .bind(":part_key", DateTimeToBoost(part_key))
       .exec();

    while(!cur.fen())
    {
        TDateTime ctime = ckin_time.is_not_a_date_time() ? NoExists : BoostToDateTime(ckin_time);
        TDateTime btime = brd_time.is_not_a_date_time() ? NoExists : BoostToDateTime(brd_time);
         (*this)[ make_pair(grp_id, reg_no) ] = make_pair(ctime, btime);
    };
}

void TRegEvents::fromDB(TDateTime part_key, int point_id)
{
    if (part_key!=NoExists)
    {
        return fromArxDB(part_key, point_id);
    }
    auto& sess = PgOra::getROSession("EVENTS_BILINGUAL");
    QParams QryParams;
    QryParams
        << QParam("lang", otString, AstraLocale::LANG_RU)
        << QParam("evtPax", otString, EncodeEventType(ASTRA::evtPax))
        << QParam("point_id", otInteger, point_id);

    std::string inStrCkin = sess.isOracle() ? "INSTR(msg,'зарегистрирован')"
                                            : "STRPOS(msg,'зарегистрирован')";
    std::string inStrBrd = sess.isOracle() ? "INSTR(msg,'прошел посадку')"
                                           : "STRPOS(msg,'прошел посадку')";
    ostringstream sql;
    sql <<
        "SELECT id3 AS grp_id, id2 AS reg_no, "
        "       MIN(CASE " << inStrCkin << " WHEN 0 THEN NULL ELSE time) AS ckin_time, "
        "       MAX(CASE " << inStrBrd << " WHEN 0 THEN NULL ELSE time) AS brd_time "
        "FROM events_bilingual "
        "WHERE lang=:lang AND type=:evtPax AND id1=:point_id AND "
        "   (msg like '%зарегистрирован%' OR msg like '%прошел посадку%') "
        "GROUP BY id3, id2";

    DB::TCachedQuery Qry(sess, sql.str(), QryParams, STDLOG);

    Qry.get().Execute();
    for(;!Qry.get().Eof;Qry.get().Next())
    {
        int grp_id=Qry.get().FieldIsNULL("grp_id")?NoExists:Qry.get().FieldAsInteger("grp_id");
        int reg_no=Qry.get().FieldIsNULL("reg_no")?NoExists:Qry.get().FieldAsInteger("reg_no");
        TDateTime ckin_time=Qry.get().FieldIsNULL("ckin_time")?NoExists:Qry.get().FieldAsDateTime("ckin_time");
        TDateTime brd_time=Qry.get().FieldIsNULL("brd_time")?NoExists:Qry.get().FieldAsDateTime("brd_time");
        (*this)[ make_pair(grp_id, reg_no) ] = make_pair(ckin_time, brd_time);
    };
}

void TEncodedFileStream::open()
{
    if(not of.is_open()) {
        of.open(filename.c_str());
        if(not of.is_open())
            throw Exception("TEncodedFileStream: error opening file %s", filename.c_str());
    }
}

/*
    template<typename T>
TEncodedFileStream &TEncodedFileStream::operator << (const T &val)
{
    open();
    ostringstream buf;
    buf << val;
    of << ConvertCodepage(buf.str(), "CP866", codepage);
    return *this;
}
*/

TEncodedFileStream &TEncodedFileStream::operator << (ostream &(*os)(ostream &))
{
    open();
    of << os;
    return *this;
}

TNearestDate::TNearestDate(TDateTime asrc_date): src_date(asrc_date)
{
    sorted_points[src_date] = NoExists;
}

int TNearestDate::get()
{
    int point_id = NoExists;
    if(sorted_points.size() == 1) {
        point_id = sorted_points.begin()->second;
    } else {
        const map<TDateTime, int>::iterator i_point = sorted_points.find(src_date);
        if(i_point->second != NoExists)
            point_id = i_point->second;
        else {
            map<TDateTime, int>::iterator curr_point = i_point;
            if(i_point == sorted_points.begin()) {
                curr_point = i_point;
                point_id = (++curr_point)->second;
            } else if(++curr_point == sorted_points.end()) {
                curr_point = i_point;
                point_id = (--curr_point)->second;
            } else {
                map<TDateTime, int>::iterator i_prev_point = i_point;
                --i_prev_point;
                map<TDateTime, int>::iterator i_next_point = i_point;
                ++i_next_point;
                if(i_next_point->first - src_date > src_date - i_prev_point->first)
                    point_id = i_prev_point->second;
                else
                    point_id = i_next_point->second;
            }
        }
    }
    return point_id;
}

void traceXML(const xmlDocPtr doc)
{
  string xml=XMLTreeToText(doc);
  size_t len=xml.size();
  int portion=4000;
  for(size_t pos=0; pos<len; pos+=portion)
    ProgTrace(TRACE5, "%s", xml.substr(pos,portion).c_str());
}

bool rus_airp(const string &airp)
{
    string city = base_tables.get("AIRPS").get_row("code", airp).AsString("city");
    return base_tables.get("CITIES").get_row("code", city).AsString("country") == "РФ";
}

string get_airp_country(const string &airp)
{
    string city = base_tables.get("AIRPS").get_row("code", airp).AsString("city");
    return base_tables.get("CITIES").get_row("code", city).AsString("country");
}

string getDocMonth(int month, bool pr_lat)
{
    const map<int, map<int, string> > months =
    {
        {0,
            {
                {1, "января"},
                {2, "февраля"},
                {3, "марта"},
                {4, "апреля"},
                {5, "мая"},
                {6, "июня"},
                {7, "июля"},
                {8, "августа"},
                {9, "сентября"},
                {10, "октября"},
                {11, "ноября"},
                {12, "декабря"}
            }
        },
        {1,
            {
                {1, "january"},
                {2, "february"},
                {3, "march"},
                {4, "april"},
                {5, "may"},
                {6, "june"},
                {7, "july"},
                {8, "august"},
                {9, "september"},
                {10, "october"},
                {11, "november"},
                {12, "december"}
            }
        }
    };

    if(month < 1 or month > 12)
        throw EXCEPTIONS::Exception("getDocMonth: wrong month: %d", month);

    return months.at(pr_lat).at(month);
}

string getDocMonth(TDateTime claim_date, bool pr_lat)
{
    int Year, Month, Day;
    DecodeDate(claim_date, Year, Month, Day);
    return getDocMonth(Month, pr_lat);
}

bool isDoomedToWait()
{
    return ServerFramework::getQueryRunner().getEdiHelpManager().mustWait();
}

std::optional<std::string> invalidSymbolInName(const std::string &value,
                                               const bool latinOnly,
                                               const std::string &additionalSymbols)
{

  const string::const_iterator i=
    std::find_if_not(value.begin(), value.end(),
                     [latinOnly, additionalSymbols](const auto &c)
                     {
                       return ((!latinOnly || IsAscii7(c)) && IsUpperLetter(c)) ||
                              IsDigit(c) ||
                              additionalSymbols.find(c)!=string::npos;
                     });
  if (i!=value.end()) return std::string(1, *i);

  return std::nullopt;
}

bool isValidName(const std::string &value, const bool latinOnly, const std::string &additionalSymbols)
{
  return !invalidSymbolInName(value, latinOnly, additionalSymbols);
}

bool isValidAirlineName(const std::string &value, const bool latinOnly)
{
  return isValidName(value, latinOnly, " ,.+-/:;()\"`'");
}

std::string checkAndNormalizeBort(const std::string& value)
{
  std::string normalValue=algo::transform(value, [](const auto &c)
                                                 {
                                                   if (c>=0 && c<' ')
                                                     return ' ';
                                                   else
                                                     return c;
                                                 });
  std::optional<std::string> invalidSymbol=invalidSymbolInName(normalValue, false, " -");
  if (invalidSymbol)
    throw AstraLocale::UserException("MSG.INVALID_CHARS_IN_BOARD_NUM",
                                     LParams() << LParam("symbol", invalidSymbol.value()));

  std::string result;
  std::string lastDelim;
  for(const auto& c : normalValue)
  {
    if (c=='-' || c==' ')
    {
      if (lastDelim!="-") lastDelim=c;
    }
    else
    {
      if (result.empty()) result=c; else result+=lastDelim+c;
      lastDelim.clear();
    }
  }

  if (result.size()==1 || result.size()>10)
    throw AstraLocale::UserException("MSG.INVALID_BOARD_NUM");

  return result;
}

void checkDateRange(TDateTime first_date, TDateTime last_date)
{
  if (first_date > last_date) {
    throw AstraLocale::UserException("MSG.TABLE.INVALID_RANGE");
  }
}

void dateTimeToDatePeriod(TDateTime first_datetime, TDateTime last_datetime,
                          TDateTime& first_date, TDateTime& last_date)
{
  std::modf(first_datetime, &first_date);
  std::modf(last_datetime, &last_date);
}

void checkPeriod(bool pr_new,
                 TDateTime first_datetime,
                 TDateTime last_datetime,
                 TDateTime now,
                 TDateTime& first_date,
                 TDateTime& last_date,
                 bool& pr_opd)
{
    if (first_datetime == ASTRA::NoExists && last_datetime == ASTRA::NoExists) {
        throw AstraLocale::UserException("MSG.TABLE.NOT_SET_RANGE");
    }
    dateTimeToDatePeriod(first_datetime, last_datetime,
                         first_date, last_date);

    if (first_datetime != ASTRA::NoExists
        && last_datetime != ASTRA::NoExists)
    {
        checkDateRange(first_date, last_date);
    }

    TDateTime today;
    std::modf(now,&today);

    if (first_date != ASTRA::NoExists) {
        if (first_date < today) {
            if (pr_new) {
                throw AstraLocale::UserException("MSG.TABLE.FIRST_DATE_BEFORE_TODAY");
            } else {
                first_date = today;
            }
        }
        if (first_date == today) {
            first_date = now;
        }
        pr_opd = false;
    } else {
        pr_opd = true;
    }

    if (last_date != ASTRA::NoExists) {
        if (last_date < today) {
            throw AstraLocale::UserException("MSG.TABLE.LAST_DATE_BEFORE_TODAY");
        }
        last_date = last_date + 1;
    }
    if (pr_opd) {
        first_date = last_date;
        last_date  = ASTRA::NoExists;
    }
}

void checkPeriodOverlaps(TDateTime first_date,
                         TDateTime last_date,
                         TDateTime prev_first_date,
                         TDateTime prev_last_date)
{
  if (
      (last_date == ASTRA::NoExists
       && prev_last_date == ASTRA::NoExists)
      || (last_date == ASTRA::NoExists
          && prev_last_date != ASTRA::NoExists
          && prev_last_date > first_date)
      || (last_date != ASTRA::NoExists
          && prev_last_date == ASTRA::NoExists
          && last_date >= prev_first_date)
      || (last_date != ASTRA::NoExists
          && prev_last_date != ASTRA::NoExists
          && (
               (first_date >= prev_first_date && last_date <= prev_last_date)
                || (first_date < prev_first_date && last_date >= prev_first_date)
                || (last_date > prev_last_date && first_date < prev_last_date)
              )
          )
  ) {
    throw AstraLocale::UserException("MSG.PERIOD_OVERLAPS_WITH_INTRODUCED");
  }
}

void checkEdiAddr(const std::string& addr)
{
    if(addr.length() < 2) {
        throw UserException("MSG.TLG.INVALID_ADDR_LENGTH",
                            LParams() << LParam("addr", addr));
    }

    if(!IsLatinUpperLettersOrDigits(addr)) {
        throw UserException("MSG.TLG.INVALID_ADDR_CHARS",
                            LParams() << LParam("addr", addr));
    }
}
