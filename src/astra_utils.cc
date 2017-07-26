#include <stdarg.h>
#include <string>
#include <string.h>
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
#include "serverlib/tcl_utils.h"
#include "serverlib/monitor_ctl.h"
#include "serverlib/sirena_queue.h"
#include "serverlib/testmode.h"
#include "serverlib/lwriter.h"
#include "jxtlib/JxtInterface.h"
#include "jxtlib/jxt_cont.h"
#include "jxtlib/xml_stuff.h"
#include "astra_misc.h"
#include "dev_consts.h"
#include "dev_utils.h"

#define NICKNAME "VLAD"
#include "serverlib/test.h"

using namespace std;
using namespace ASTRA;
using namespace BASIC::date_time;
using namespace EXCEPTIONS;
using namespace boost::local_time;
using namespace boost::posix_time;
using namespace AstraLocale;

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
  //�஢�ਬ �ࠢ��쭮��� 㪠������ ���ᨨ
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
  TQuery Qry(&OraSession);
  Qry.Clear();
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
};

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

  TQuery Qry(&OraSession);
  ProgTrace( TRACE5, "screen=%s, pult=|%s|, opr=|%s|, checkCrypt=%d, pr_web=%d, desk.lang=%s",
            InitData.screen.c_str(), InitData.pult.c_str(), InitData.opr.c_str(), InitData.checkCrypt, InitData.pr_web, desk.lang.c_str() );
  screen.name = upperc( InitData.screen );
  desk.code = InitData.pult;
  desk.mode = DecodeOperMode(InitData.mode);
  if ( InitData.checkCrypt && !InitData.duplicate ) { // ��諮 �� ����஢����� ᮮ�饭�� - �஢�ઠ �� �, �� ���짮��⥫� �������
    Qry.Clear();
    Qry.SQLText =
      "SELECT pr_crypt "
      "FROM desks,desk_grp,crypt_sets "
      "WHERE desks.code = UPPER(:desk) AND "
      "      desks.grp_id = desk_grp.grp_id AND "
      "      crypt_sets.desk_grp_id=desk_grp.grp_id AND "
      "      ( crypt_sets.desk IS NULL OR crypt_sets.desk=desks.code ) "
      "ORDER BY desk ASC ";
    Qry.CreateVariable( "desk", otString, InitData.pult );
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

  string sql;

  Qry.Clear();
  Qry.SQLText = "SELECT id,pr_logon FROM screen WHERE exe = :exe";
  Qry.DeclareVariable( "exe", otString );
  Qry.SetVariable( "exe", screen.name );
  Qry.Execute();
  if ( Qry.RowCount() == 0 )
    throw EXCEPTIONS::Exception( (string)"Unknown screen " + screen.name );
  screen.id = Qry.FieldAsInteger( "id" );
  screen.pr_logon = Qry.FieldAsInteger( "pr_logon" );

  Qry.Clear();
  Qry.SQLText =
    "SELECT desks.version, NVL(desks.under_constr,0) AS under_constr, "
    "       desks.currency, desks.term_id, "
    "       desk_grp.grp_id, desk_grp.city, desk_grp.airp, desk_grp.airline "
    "FROM desks,desk_grp "
    "WHERE desks.grp_id = desk_grp.grp_id AND desks.code = UPPER(:pult)";
  Qry.DeclareVariable( "pult", otString );
  Qry.SetVariable( "pult", InitData.pult );
  Qry.Execute();
  if ( Qry.RowCount() == 0 )
    throw AstraLocale::UserException( "MSG.PULT_NOT_REGISTERED");
  if (Qry.FieldAsInteger("under_constr")!=0)
    throw AstraLocale::UserException( "MSG.SERVER_TEMPORARILY_UNAVAILABLE" );
  desk.city = Qry.FieldAsString( "city" );
  desk.airp = Qry.FieldAsString( "airp" );
  desk.airline = Qry.FieldAsString( "airline" );
  desk.version = Qry.FieldAsString( "version" );
  desk.currency = Qry.FieldAsString( "currency" );
  desk.grp_id = Qry.FieldAsInteger( "grp_id" );
  desk.term_id = Qry.FieldIsNULL("term_id")?NoExists:Qry.FieldAsFloat( "term_id" );

  ProgTrace( TRACE5, "terminal version='%s'", desk.version.c_str() );

  Qry.Clear();
  Qry.SQLText=
    "SELECT tz_region FROM cities WHERE cities.code=:city AND cities.pr_del=0";
  Qry.DeclareVariable( "city", otString );
  Qry.SetVariable( "city", desk.city );
  Qry.Execute();
  if (Qry.Eof)
    throw AstraLocale::UserException("MSG.DESK_CITY_NOT_DEFINED");
  if (Qry.FieldIsNULL("tz_region"))
    throw AstraLocale::UserException("MSG.CITY.REGION_NOT_DEFINED", LParams() << LParam("city", ElemIdToCodeNative(etCity,desk.city)));
  desk.tz_region = Qry.FieldAsString( "tz_region" );
  desk.time = UTCToLocal( NowUTC(), desk.tz_region );
  if ( !screen.pr_logon ||
       (!InitData.pr_web && !InitData.checkUserLogon) )
    return;
  Qry.Clear();
  if ( InitData.pr_web ) {
    Qry.SQLText =
      "SELECT user_id, login, descr, type, pr_denial "
      "FROM users2 "
      "WHERE login = :login ";
    Qry.CreateVariable( "login", otString, InitData.opr );
    Qry.Execute();
    if (Qry.Eof)
      throw AstraLocale::UserException( "MSG.USER.ACCESS_DENIED");
  }
  else {
    if (InitData.term_id!=desk.term_id)
      throw AstraLocale::UserException( "MSG.USER.NEED_TO_LOGIN" );

    Qry.SQLText =
      "SELECT user_id, login, descr, type, pr_denial "
      "FROM users2 "
      "WHERE desk = UPPER(:pult) ";
    Qry.CreateVariable( "pult", otString, InitData.pult );
    Qry.Execute();
    if (Qry.Eof)
      throw AstraLocale::UserException( "MSG.USER.NEED_TO_LOGIN" );
    if ( !InitData.opr.empty() )
      if ( InitData.opr != Qry.FieldAsString( "login" ) )
        throw AstraLocale::UserException( "MSG.USER.NEED_TO_LOGIN" );
  };

  if ( Qry.FieldAsInteger( "pr_denial" ) == -1 )
    throw AstraLocale::UserException( "MSG.USER.DELETED");
  if ( Qry.FieldAsInteger( "pr_denial" ) != 0 )
    throw AstraLocale::UserException( "MSG.USER.ACCESS_DENIED");
  user.user_id = Qry.FieldAsInteger( "user_id" );
  user.descr = Qry.FieldAsString( "descr" );
  user.user_type = (TUserType)Qry.FieldAsInteger( "type" );
  user.login = Qry.FieldAsString( "login" );


  Qry.Clear();
  Qry.SQLText =
    "SELECT client_type FROM web_clients "
    "WHERE desk=UPPER(:desk) OR user_id=:user_id";
  Qry.CreateVariable( "desk", otString, InitData.pult );
  Qry.CreateVariable( "user_id", otInteger, user.user_id );
  Qry.Execute();
  if ( (!Qry.Eof && !InitData.pr_web) ||
        (Qry.Eof && InitData.pr_web) ) //???
        throw AstraLocale::UserException( "MSG.USER.ACCESS_DENIED" );

  if ( InitData.pr_web )
    client_type = DecodeClientType( Qry.FieldAsString( "client_type" ) );
  else
    client_type = ctTerm;

  //�᫨ �㦠騩 ���� - �஢�ਬ ���� � ���ண� �� ��室��
  /*if (user.user_type==utAirport)
  {
    Qry.Clear();
    sql = string( "SELECT airps.city " ) +
                  "FROM aro_airps,airps " +
                  "WHERE aro_airps.airp=airps.code AND "
                  "      airps.city=:city AND aro_airps.aro_id=:user_id AND rownum<2 ";
    Qry.SQLText=sql;
    Qry.CreateVariable("city",otString,desk.city);
    Qry.CreateVariable("user_id",otInteger,user.user_id);
    Qry.Execute();
    if (Qry.Eof)
      throw AstraLocale::UserException( "MSG.USER.ACCESS_DENIED_FROM_DESK", LParams() << LParam("desk", desk.code));
  };*/

  user.access.fromDB(user.user_id, user.user_type);

  TAccessElems<std::string> airlines;
  TAccessElems<std::string> airps;
  vector<string> elems;

  //�஢�ਬ ��࠭�祭�� ����㯠 �� ��ᨨ
  SeparateString(JxtContext::getJxtContHandler()->sysContext()->read("session_airlines"),'/',elems);
  airlines.clear();
  airlines.set_elems_permit(!elems.empty());
  for(vector<string>::const_iterator e=elems.begin(); e!=elems.end(); ++e) airlines.add_elem(*e);
  user.access.merge_airlines(airlines);

  //�஢�ਬ ��࠭�祭�� ����㯠 �� 䨫��ࠬ
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

  //�஢�ਬ ��࠭�祭�� ����㯠 �� ᮡ�⢥������ ����
  Qry.Clear();
  Qry.SQLText=
    "SELECT airline,pr_denial FROM desk_owners "
    "WHERE desk=:desk ORDER BY airline NULLS FIRST";
  Qry.CreateVariable("desk",otString,InitData.pult);
  Qry.Execute();
  bool pr_denial_all=true;
  if (!Qry.Eof && Qry.FieldIsNULL("airline"))
  {
    pr_denial_all=Qry.FieldAsInteger("pr_denial")!=0;
    Qry.Next();
  };

  airlines.clear();
  airlines.set_elems_permit(pr_denial_all);
  for(;!Qry.Eof;Qry.Next())
  {
    if (Qry.FieldIsNULL("airline")) continue;
    bool pr_denial=Qry.FieldAsInteger("pr_denial")!=0;
    if ((!pr_denial_all && pr_denial) ||
        (pr_denial_all && !pr_denial))
      airlines.add_elem(Qry.FieldAsString("airline"));
  };
  user.access.merge_airlines(airlines);
  if (airlines.totally_not_permitted())
    throw AstraLocale::UserException( "MSG.DESK.TURNED_OFF" );

  //���짮��⥫�᪨� ����ன��
  Qry.Clear();
  Qry.SQLText=
    "SELECT time,disp_airline,disp_airp,disp_craft,disp_suffix "
    "FROM user_sets "
    "WHERE user_id=:user_id";
  Qry.CreateVariable("user_id",otInteger,user.user_id);
  Qry.Execute();
  if (!Qry.Eof)
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
      if (!Qry.FieldIsNULL(field))
      {
        switch(Qry.FieldAsInteger(field))
        {
          case ustTimeUTC:
          case ustTimeLocalDesk:
          case ustTimeLocalAirp:
            if (i==0)
              user.sets.time=(TUserSettingType)Qry.FieldAsInteger(field);
            break;
          case ustCodeNative:
          case ustCodeInter:
          case ustCodeICAONative:
          case ustCodeICAOInter:
          case ustCodeMixed:
            if (i==1)
              user.sets.disp_airline=(TUserSettingType)Qry.FieldAsInteger(field);
            if (i==2)
              user.sets.disp_airp=(TUserSettingType)Qry.FieldAsInteger(field);
            if (i==3)
              user.sets.disp_craft=(TUserSettingType)Qry.FieldAsInteger(field);
            break;
          case ustEncNative:
          case ustEncLatin:
          case ustEncMixed:
            if (i==4)
              user.sets.disp_suffix=(TUserSettingType)Qry.FieldAsInteger(field);
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
  TQuery Qry(&OraSession);
  Qry.Clear();
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

void TProfiledRights::toXML(xmlNodePtr node)
{
    xmlNodePtr lstNode = NULL;
    for(set<int>::iterator i = items.begin(); i != items.end(); i++) {
        if(not lstNode)
            lstNode = NewTextChild(node, "profile_rights");
        NewTextChild(lstNode, "item", *i);
    }
}

void TProfiledRights::fromDB(const string &airline, const string &airp)
{
    if(TReqInfo::Instance()->user.user_type != utAirport) return;

    TCachedQuery Qry(
            "select profile_id from airline_profiles where "
            "   airline = :airline and "
            "   (airp is null or airp = :airp) "
            "order by airp nulls last ",
            QParams()
            << QParam("airline", otString, airline)
            << QParam("airp", otString, airp));
    Qry.get().Execute();
    if(not Qry.get().Eof) {
        TCachedQuery rightQry(
                "select right_id from profile_rights where "
                "   profile_id = :profile_id ",
                QParams()
                << QParam("profile_id", otInteger, Qry.get().FieldAsInteger("profile_id")));
        rightQry.get().Execute();
        for(; not rightQry.get().Eof; rightQry.get().Next())
            items.insert(rightQry.get().FieldAsInteger("right_id"));
    }
}

TProfiledRights::TProfiledRights(const string &airline, const string &airp)
{
    fromDB(airline, airp);
}

TProfiledRights::TProfiledRights(int point_id)
{
    TTripInfo flt;
    flt.getByPointId(point_id);
    fromDB(flt.airline, flt.airp);
}

bool TAccess::check_profile(const string &airp, const string &airline, int right_id)
{
    bool result = true;
    if(TReqInfo::Instance()->user.user_type == utAirport) {
        TCachedQuery Qry(
                "select profile_id from airline_profiles where "
                "   airline = :airline and "
                "   (airp is null or airp = :airp) "
                "order by airp nulls last ",
                QParams()
                << QParam("airline", otString, airline)
                << QParam("airp", otString, airp));
        Qry.get().Execute();
        if(not Qry.get().Eof) {
            TCachedQuery rightQry(
                    "select * from profile_rights where "
                    "   profile_id = :profile_id and "
                    "   right_id = :right_id ",
                    QParams()
                    << QParam("profile_id", otInteger, Qry.get().FieldAsInteger("profile_id"))
                    << QParam("right_id", otInteger, right_id));
            rightQry.get().Execute();
            result = rightQry.get().Eof;

        }
    }
    if(result) result = TReqInfo::Instance()->user.access.rights().permitted(right_id);
    return result;
}

bool TAccess::check_profile_by_crs_pax(int pax_id, int right_id)
{
    TTripInfo flt;
    flt.getByCRSPaxId(pax_id);
    return check_profile(flt.airp, flt.airline, right_id);
}

bool TAccess::check_profile(int point_id, int right_id)
{
    TTripInfo flt;
    flt.getByPointId(point_id);
    return check_profile(flt.airp, flt.airline, right_id);
}

void TAccess::fromDB(int user_id, TUserType user_type)
{
  clear();
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.CreateVariable( "user_id", otInteger, user_id );

  //�ࠢ�
  Qry.SQLText=
    "SELECT DISTINCT role_rights.right_id "
    "FROM user_roles,role_rights "
    "WHERE user_roles.role_id=role_rights.role_id AND "
    "      user_roles.user_id=:user_id ";
  Qry.Execute();
  for(;!Qry.Eof;Qry.Next())
    _rights.add_elem(Qry.FieldAsInteger("right_id"));

  //����� � �/�
  Qry.SQLText="SELECT airline FROM aro_airlines WHERE aro_id=:user_id";
  Qry.Execute();
  if (!Qry.Eof)
    for(;!Qry.Eof;Qry.Next())
      _airlines.add_elem(Qry.FieldAsString("airline"));
  else
    if (user_type==utSupport ||
        user_type==utAirport) _airlines.set_elems_permit(false);

  //����� � �/�
  Qry.SQLText="SELECT airp FROM aro_airps WHERE aro_id=:user_id";
  Qry.Execute();
  if (!Qry.Eof)
    for(;!Qry.Eof;Qry.Next())
      _airps.add_elem(Qry.FieldAsString("airp"));
  else
    if (user_type==utSupport ||
        user_type==utAirline) _airps.set_elems_permit(false);
}

void TAccess::toXML(xmlNodePtr accessNode)
{
  if (accessNode==NULL) return;
  //�ࠢ� ����㯠 � ������
  xmlNodePtr node;
  node = NewTextChild(accessNode, "rights");
  for(set<int>::const_iterator i=_rights.elems().begin();
      i!=_rights.elems().end();++i)
    NewTextChild(node,"right",*i);
  //NewTextChild(accessNode, "rights_permit", (int)_rights.elems_permit());
  //�ࠢ� ����㯠 � ������������
  node = NewTextChild(accessNode, "airlines");
  for(set<string>::const_iterator i=_airlines.elems().begin();
      i!=_airlines.elems().end();++i)
    NewTextChild(node,"airline",*i);
  NewTextChild(accessNode, "airlines_permit", (int)_airlines.elems_permit());
  //�ࠢ� ����㯠 � ��ய��⠬
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
  //�ࠢ� ����㯠 � ������
  _rights.set_total_permit();
  //�ࠢ� ����㯠 � ������������
  xmlNodePtr node = NodeAsNodeFast("airlines", node2);
  for(node=GetNode("airline", node);node!=NULL;node=node->next)
    _airlines.add_elem(NodeAsString(node));
  if (!NodeIsNULLFast("airlines_permit", node2))
    _airlines.set_elems_permit((bool)(NodeAsIntegerFast("airlines_permit", node2)!=0));
  else
    _airlines.set_elems_permit(true);
  //�ࠢ� ����㯠 � ��ய��⠬
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
  QParams QryParams;
  QryParams << QParam("type", otString, EncodeEventType(ev_type))
            << QParam("screen", otString, screen)
            << QParam("ev_user", otString, user_descr)
            << QParam("station", otString, desk_code)
            << QParam("msg", otString)
            << QParam("lang", otString)
            << QParam("part_num", otInteger)
            << QParam("ev_time", otDate, FNull)
            << QParam("ev_order", otInteger, FNull);

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

  TCachedQuery CachedQry(
        "BEGIN "
        "  IF :ev_time IS NULL OR :ev_order IS NULL THEN"
        "    SELECT system.UTCSYSDATE, events__seq.nextval INTO :ev_time, :ev_order FROM dual; "
        "  END IF; "
        "  IF :part_num IS NULL THEN :part_num:=1; ELSE :part_num:=:part_num+1; END IF; "
        "  INSERT INTO events_bilingual(type,time,ev_order,part_num,msg,screen,ev_user,station,id1,id2,id3,lang) "
        "  VALUES(:type,:ev_time,:ev_order,:part_num,"
        "         :msg,:screen,:ev_user,:station,:id1,:id2,:id3,:lang); "
        "END;", QryParams);

  TQuery &Qry=CachedQry.get();

  for (std::vector<std::string>::iterator lang = vlangs.begin(); lang != vlangs.end(); lang++) {
    Qry.SetVariable("part_num", FNull);
    (*lang == AstraLocale::LANG_RU)?Qry.SetVariable("lang", AstraLocale::LANG_RU):
                                    Qry.SetVariable("lang", AstraLocale::LANG_EN);
    std::string message;
    message = AstraLocale::getLocaleText(lexema_id, prms.GetParams(*lang), *lang);
    vector<string> strs;
    SeparateString(message.c_str(), 250, strs);
    for (vector<string>::iterator i=strs.begin(); i!=strs.end(); i++) {
      Qry.SetVariable("msg", *i);
      Qry.Execute();
    }
  }
  if (!Qry.VariableIsNULL("ev_time"))
    ev_time=Qry.GetVariableAsDateTime("ev_time");
  else
    ev_time=ASTRA::NoExists;
  if (!Qry.VariableIsNULL("ev_order"))
    ev_order=Qry.GetVariableAsInteger("ev_order");
  else
    ev_order=ASTRA::NoExists;
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

void LexemeDataToXML(const LexemaData &lexemeData, xmlNodePtr lexemeNode)
{
  if (lexemeNode==NULL) return;
  NewTextChild(lexemeNode,"id",lexemeData.lexema_id);
  xmlNodePtr node=NewTextChild(lexemeNode,"params");
  for(LParams::const_iterator p=lexemeData.lparams.begin();p!=lexemeData.lparams.end();p++)
  {
    NewTextChild(node,p->first.c_str(),lexemeData.lparams.StringValue(p->first));
  };
}

void LexemeDataFromXML(xmlNodePtr lexemeNode, LexemaData &lexemeData)
{
  lexemeData.lexema_id.clear();
  lexemeData.lparams.clear();
  if (lexemeNode==NULL) return;
  lexemeData.lexema_id=NodeAsString("id",lexemeNode);
  xmlNodePtr node=NodeAsNode("params",lexemeNode)->children;
  for(;node!=NULL;node=node->next)
  {
    lexemeData.lparams << LParam((const char*)node->name, NodeAsString(node));
  };
};

xmlNodePtr selectPriorityMessage(xmlNodePtr resNode, std::string& error_code, std::string& error_message)
{
  // �᫨ ���� ⥣ <error> || <checkin_user_error> || <user_error>, � �� ��⠫쭮� 㤠�塞 �� �� xml ��ॢ�
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
    //��楯�塞
    xmlUnlinkNode(errNode);
  };

  for(xmlNodePtr node=resNode->children; node!=NULL;)
  {
      //��楯�塞 � 㤠�塞 ���� ��, ���� <command> ����� <answer>
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
    ProgTrace( TRACE0, e.what() );
  };

  ProgTrace( TRACE5, "TCL param %s=%d", name, res );
  return res;
};

//�᫨ def==NULL, ⮣�� � ��砥 ����宦����� name �㣠����
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
    ProgTrace( TRACE0, e.what() );
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
        VAR = (size_t)round(getFileSizeDouble(param_val)); // �ਢ������ double � size_t � ��砥 ��९������� �ਢ���� � 0
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

  TQuery Qry(&OraSession);

  if (!reqInfo->user.login.empty())
  {
    node = NewTextChild(resNode,"user");
    NewTextChild(node, "login",reqInfo->user.login);
    NewTextChild(node, "type",reqInfo->user.user_type);
    //����ன�� ���짮��⥫�
    xmlNodePtr setsNode = NewTextChild(node, "settings");
    NewTextChild(setsNode, "time", reqInfo->user.sets.time);
    NewTextChild(setsNode, "disp_airline", reqInfo->user.sets.disp_airline);
    NewTextChild(setsNode, "disp_airp", reqInfo->user.sets.disp_airp);
    NewTextChild(setsNode, "disp_craft", reqInfo->user.sets.disp_craft);
    NewTextChild(setsNode, "disp_suffix", reqInfo->user.sets.disp_suffix);
    bool advanced_trip_list=
           !(reqInfo->user.user_type==utAirport && !reqInfo->user.access.rights().permitted(335));
    NewTextChild(setsNode, "advanced_trip_list", (int)advanced_trip_list);

    //�����
    reqInfo->user.access.toXML(NewTextChild(node, "access"));
  };
  if (!reqInfo->desk.code.empty())
  {
    node = NewTextChild(resNode,"desk");
    NewTextChild(node,"city",reqInfo->desk.city);
    NewTextChild(node,"currency",reqInfo->desk.currency);
    NewTextChild(node,"time",DateTimeToStr( reqInfo->desk.time ) );
    NewTextChild(node,"time_utc",DateTimeToStr(NowUTC()) );
    //����ன�� ����
    xmlNodePtr setsNode = NewTextChild(node, "settings");

    Qry.Clear();
    Qry.SQLText="SELECT defer_etstatus FROM desk_grp_sets WHERE grp_id=:grp_id";
    Qry.CreateVariable("grp_id",otInteger,reqInfo->desk.grp_id);
    Qry.Execute();
    if (!Qry.Eof && !Qry.FieldIsNULL("defer_etstatus"))
      NewTextChild(setsNode,"defer_etstatus",(int)(Qry.FieldAsInteger("defer_etstatus")!=0));
    else
      NewTextChild(setsNode,"defer_etstatus",(int)true);

    Qry.Clear();
    Qry.SQLText="SELECT file_size,send_size,send_portion,backup_num FROM desk_logging WHERE desk=:desk";
    Qry.CreateVariable("desk",otString,reqInfo->desk.code);
    Qry.Execute();
    if (!Qry.Eof)
    {
      xmlNodePtr loggingNode=NewTextChild(node,"logging");
      NewTextChild(loggingNode,"file_size",Qry.FieldAsInteger("file_size"));
      NewTextChild(loggingNode,"send_size",Qry.FieldAsInteger("send_size"));
      NewTextChild(loggingNode,"send_portion",Qry.FieldAsInteger("send_portion"));
      NewTextChild(loggingNode,"backup_num",Qry.FieldAsInteger("backup_num"));
      xmlNodePtr rangesNode=NewTextChild(loggingNode,"trace_ranges");
      Qry.Clear();
      Qry.SQLText="SELECT first_trace,last_trace FROM desk_traces WHERE desk=:desk";
      Qry.CreateVariable("desk",otString,reqInfo->desk.code);
      Qry.Execute();
      int trace_level=-1;
      for(;!Qry.Eof;Qry.Next())
      {
        xmlNodePtr rangeNode=NewTextChild(rangesNode,"range");
        NewTextChild(rangeNode,"first_trace",Qry.FieldAsInteger("first_trace"));
        NewTextChild(rangeNode,"last_trace",Qry.FieldAsInteger("last_trace"));

        if (trace_level<Qry.FieldAsInteger("last_trace"))
          trace_level=Qry.FieldAsInteger("last_trace");
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

  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText =
    "SELECT client_error_list.type "
    "FROM client_error_list,locale_messages "
    "WHERE client_error_list.text=locale_messages.id(+) AND "
    "      (:text like client_error_list.text OR "
    "       locale_messages.text IS NOT NULL AND :text like '%'||locale_messages.text||'%') ";
  Qry.DeclareVariable("text",otString);

  xmlNodePtr node=reqNode->children;
  for(;node!=NULL;node=node->next)
  {
    if (strcmp((char*)node->name,"msg")==0)
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

string& AirpTZRegion(string airp, bool with_exception)
{
  if (airp.empty()) throw EXCEPTIONS::Exception("Airport not specified");
  TAirpsRow& row=(TAirpsRow&)base_tables.get("airps").get_row("code",airp,true);
  return CityTZRegion(row.city,with_exception);
};

string& CityTZRegion(string city, bool with_exception)
{
  if (city.empty()) throw EXCEPTIONS::Exception("City not specified");
  TCitiesRow& row=(TCitiesRow&)base_tables.get("cities").get_row("code",city,true);
  if (row.tz_region.empty() && with_exception)
    throw AstraLocale::UserException("MSG.CITY.REGION_NOT_DEFINED",LParams() << LParam("city", city));
  return row.tz_region;
};

string DeskCity(string desk, bool with_exception)
{
  if (desk.empty()) throw EXCEPTIONS::Exception("Desk not specified");
  TQuery Qry(&OraSession);
  Qry.Clear();
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

TCountriesRow getCountryByAirp( const std::string& airp)
{
  TAirpsRow &airpRow = (TAirpsRow&)base_tables.get("airps").get_row("code",airp);
  TCitiesRow &cityRow = (TCitiesRow&)base_tables.get("cities").get_row("code",airpRow.city);
  return ((TCountriesRow&)base_tables.get("countries").get_row("code",cityRow.country));
}

char ToLatPnrAddr(char c)
{
  if (!IsAscii7(c))
  {
    ByteReplace(&c,1,rus_pnr,lat_pnr);
    if (!IsAscii7(c)) c='?';
  };
  return c;
};

string convert_pnr_addr(const string &value, bool pr_lat)
{
  string result = value;
  if (pr_lat)
    transform(result.begin(), result.end(), result.begin(), ToLatPnrAddr);
  return result;

};

class TTranslitLetter
{
  public:
    string lat1, lat2, lat3;
    TTranslitLetter(const char* l1, const char* l2, const char* l3):lat1(l1), lat2(l2), lat3(l3) {};
    TTranslitLetter() {};
};

string transliter(const string &value, int fmt, bool pr_lat)
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
        if (fmt==3)
        {
          string::const_iterator i2=i+1;
          if (i2!=value.end())
          {
            char uc2=ToUpper(*i2);
            if (uc=='�' && uc2=='�') { c2="X"; i=i2; };
            if (uc=='�' && uc2=='�' && (i2+1)==value.end()) { c2="Y"; i=i2; };
          };
          if (uc=='�' && i!=value.begin() && string("����������").find_first_of(ToUpper(*(i-1)))!=string::npos) c2="TCH";
          if (uc=='�' && i==value.begin()) c2="U";
        };
        if (c2.empty())
        {
          map<char, TTranslitLetter>::const_iterator letter=dicts.find(uc);
          if (letter!=dicts.end())
          {
            if      (fmt==3) c2=letter->second.lat3;
            else if (fmt==2) c2=letter->second.lat2;
            else if (fmt==1) c2=letter->second.lat1;
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
};

bool transliter_equal(const string &value1, const string &value2, int fmt)
{
  return transliter(value1, fmt, true)==transliter(value2, fmt, true);
};

bool transliter_equal(const string &value1, const string &value2)
{
  for(int fmt=1;fmt<=3;fmt++)
    if (transliter_equal(value1, value2, fmt)) return true;
  return false;
};

int best_transliter_similarity(const string &value1, const string &value2)
{
  if (value1==value2) return 100;

  int result=EditDistanceSimilarity(value1, value2);
  for(int fmt=1;fmt<=3;fmt++)
  {
    if (result>=100) return result;
    int i=EditDistanceSimilarity(transliter(value1, fmt, true), transliter(value2, fmt, true));
    if (i>result) result=i;
  }
  return result;
}

const char *rus_char_view = "������������ᥪ�����";
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

string& EOracleError2UserException(string& msg)
{
  //ProgTrace(TRACE5,"EOracleError2UserException: msg=%s",msg.c_str());
  size_t p;
  if (msg.substr( 0, 4 ) == "ORA-")
  {
    p = msg.find( ": " );
    if ( p != string::npos )
    {
      //��१��� ORA- ��ࢮ� ��ப�
      msg.erase( 0, p+2 );
    };
  };
  //�饬 ᫥������ ��ப�, ��稭������� � ORA-
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
    msg=getLocaleText(NodeAsNode("/lexeme_data",msgDoc.docPtr()));
  };
  return msg;
};

string get_internal_msgid_hex()
{
  string str_msg_id((const char*)get_internal_msgid(),sizeof(int)*3);
  string hex_msg_id;
  StringToHex(str_msg_id,hex_msg_id);
  return hex_msg_id;
};

namespace ASTRA
{

void commit()
{
    //inTestMode()?commit():OraSession.Commit();
    if(!inTestMode())
        OraSession.Commit();
}

void rollback()
{
    //inTestMode()?rollback():OraSession.Rollback();
    if(!inTestMode())
        OraSession.Rollback();
}

}// namespace ASTRA

void TRegEvents::fromDB(TDateTime part_key, int point_id)
{
    QParams QryParams;
    QryParams
        << QParam("lang", otString, AstraLocale::LANG_RU)
        << QParam("evtPax", otString, EncodeEventType(ASTRA::evtPax))
        << QParam("point_id", otInteger, point_id);
    ostringstream sql;
    sql <<
        "SELECT id3 AS grp_id, id2 AS reg_no, "
        "       MIN(DECODE(INSTR(msg,'��ॣ����஢��'),0,TO_DATE(NULL),time)) AS ckin_time, "
        "       MAX(DECODE(INSTR(msg,'��襫 ��ᠤ��'),0,TO_DATE(NULL),time)) AS brd_time ";
    if (part_key!=NoExists)
    {
        if(ARX_EVENTS_DISABLED())
            throw AstraLocale::UserException("MSG.ERR_MSG.ARX_EVENTS_DISABLED");
        sql <<
            "FROM arx_events "
            "WHERE type=:evtPax AND part_key=:part_key AND (lang=:lang OR lang=:lang_undef) AND id1=:point_id AND ";
        QryParams
            << QParam("part_key", otDate, part_key)
            << QParam("lang_undef", otString, "ZZ");
    }
    else
        sql <<
            "FROM events_bilingual "
            "WHERE lang=:lang AND type=:evtPax AND id1=:point_id AND ";
    sql <<
        "      (msg like '%��ॣ����஢��%' OR msg like '%��襫 ��ᠤ��%') "
        "GROUP BY id3, id2";

    TCachedQuery Qry(sql.str(), QryParams);

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

void longToDB(TQuery &Qry, const std::string &column_name, const std::string &src, bool nullable, int len)
{
  if (!src.empty())
  {
    std::string::const_iterator ib,ie;
    ib=src.begin();
    for(int page_no=1;ib<src.end();page_no++)
    {
      ie=ib+len;
      if (ie>src.end()) ie=src.end();
      Qry.SetVariable("page_no", page_no);
      Qry.SetVariable(column_name.c_str(), std::string(ib,ie));
      Qry.Execute();
      ib=ie;
    };
  }
  else
  {
    if (nullable)
    {
      Qry.SetVariable("page_no", 1);
      Qry.SetVariable(column_name.c_str(), FNull);
      Qry.Execute();
    };
  }
}

void traceXML(const xmlDocPtr doc)
{
  string xml=XMLTreeToText(doc);
  size_t len=xml.size();
  int portion=4000;
  for(size_t pos=0; pos<len; pos+=portion)
    ProgTrace(TRACE5, "%s", xml.substr(pos,portion).c_str());
}
