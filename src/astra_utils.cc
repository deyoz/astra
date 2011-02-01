#include <stdarg.h>
#include <string>
#include <string.h>
#include "astra_utils.h"
#include "basic.h"
#include "oralib.h"
#include "astra_locale.h"
#include "stl_utils.h"
#include "xml_unit.h"
#include "misc.h"
#include "astra_elems.h"
#include "base_tables.h"
#include "term_version.h"
#include "tclmon/tcl_utils.h"
#include "serverlib/monitor_ctl.h"
#include "serverlib/cfgproc.h"
#include "serverlib/sirena_queue.h"
#include "jxtlib/JxtInterface.h"
#include "jxtlib/jxt_cont.h"
#include "jxtlib/xml_stuff.h"

#define NICKNAME "VLAD"
#include "serverlib/test.h"

using namespace std;
using namespace ASTRA;
using namespace BASIC;
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
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText=
    "SELECT region FROM cities,tz_regions "
    "WHERE cities.country=tz_regions.country(+) AND "
    "      cities.tz=tz_regions.tz(+) AND "
    "      cities.code=:city AND "
    "      cities.pr_del=0 AND "
    "      tz_regions.pr_del(+)=0";
  Qry.DeclareVariable( "city", otString );
  Qry.SetVariable( "city", city );
  Qry.Execute();
  if (Qry.Eof)
    throw EXCEPTIONS::Exception("TReqInfo::Initialize: city %s not found",city.c_str());
  if (Qry.FieldIsNULL("region"))
      throw AstraLocale::UserException((string)"TReqInfo::Initialize: region nod defined (city=" + city + ")");
  desk.city = city;
  desk.tz_region = Qry.FieldAsString( "region" );
  desk.time = UTCToLocal( NowUTC(), desk.tz_region );
  user.access.airlines_permit=false;
  user.access.airps_permit=false;
};

void TReqInfo::Initialize( TReqInfoInitData &InitData )
{
	if ( execute_time.is_not_a_date_time() )
		setPerform();
  clear();

  if (!InitData.lang.empty() /*desk.compatible(LATIN_VERSION)*/)
    desk.lang=InitData.lang;
  else
    desk.lang=AstraLocale::LANG_RU;

  TQuery Qry(&OraSession);
  ProgTrace( TRACE5, "screen=%s, pult=|%s|, opr=|%s|, checkCrypt=%d, pr_web=%d, desk.lang=%s",
            InitData.screen.c_str(), InitData.pult.c_str(), InitData.opr.c_str(), InitData.checkCrypt, InitData.pr_web, desk.lang.c_str() );
  screen.name = upperc( InitData.screen );
  desk.code = InitData.pult;
  desk.mode = DecodeOperMode(InitData.mode);
  if ( InitData.checkCrypt ) { // пришло не зашифрованное сообщение - проверка на то, что пользователь шифруется
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
      xmlNodePtr resNode = NodeAsNode("/term/answer", xmlRC->resDoc);
      AstraLocale::showProgError( "MSG.MESSAGEPRO.CRYPT_MODE_ERR.REPEAT" );
      resNode = ReplaceTextChild( resNode, "clear_certificates" );
      throw UserException2();
    }
  }

  string sql;

  Qry.Clear();
  Qry.SQLText = "SELECT id,version,pr_logon FROM screen WHERE exe = :exe";
  Qry.DeclareVariable( "exe", otString );
  Qry.SetVariable( "exe", screen.name );
  Qry.Execute();
  if ( Qry.RowCount() == 0 )
    throw EXCEPTIONS::Exception( (string)"Unknown screen " + screen.name );
  screen.id = Qry.FieldAsInteger( "id" );
  screen.version = Qry.FieldAsInteger( "version" );
  screen.pr_logon = Qry.FieldAsInteger( "pr_logon" );
/*  if ( Qry.FieldAsInteger( "pr_logon" ) == 0 )
  	return; //???*/
  Qry.Clear();
  Qry.SQLText =
    "SELECT city,airp,airline,version,NVL(under_constr,0) AS under_constr,currency,desks.grp_id "
    "FROM desks,desk_grp "
    "WHERE desks.code = UPPER(:pult) AND desks.grp_id = desk_grp.grp_id ";
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

  ProgTrace( TRACE5, "terminal version='%s'", desk.version.c_str() );

  Qry.Clear();
  Qry.SQLText=
    "SELECT region FROM cities,tz_regions "
    "WHERE cities.country=tz_regions.country(+) AND "
    "      cities.tz=tz_regions.tz(+) AND "
    "      cities.code=:city AND "
    "      cities.pr_del=0 AND "
    "      tz_regions.pr_del(+)=0";
  Qry.DeclareVariable( "city", otString );
  Qry.SetVariable( "city", desk.city );
  Qry.Execute();
  if (Qry.Eof)
    throw AstraLocale::UserException("MSG.DESK_CITY_NOT_DEFINED");
  if (Qry.FieldIsNULL("region"))
    throw AstraLocale::UserException("MSG.CITY.REGION_NOT_DEFINED", LParams() << LParam("city", ElemIdToCodeNative(etCity,desk.city)));
  desk.tz_region = Qry.FieldAsString( "region" );
  desk.time = UTCToLocal( NowUTC(), desk.tz_region );
  if ( !screen.pr_logon ||
       !InitData.pr_web && !InitData.checkUserLogon )
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
  if ( !Qry.Eof && !InitData.pr_web ||
  	   Qry.Eof && InitData.pr_web ) //???
    	throw AstraLocale::UserException( "MSG.USER.ACCESS_DENIED" );

  if ( InitData.pr_web )
  	client_type = DecodeClientType( Qry.FieldAsString( "client_type" ) );
  else
    client_type = ctTerm;

  //если служащий порта - проверим пульт с которого он заходит
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

  Qry.Clear();
  Qry.SQLText=
    "SELECT DISTINCT role_rights.right_id "
    "FROM user_roles,role_rights "
    "WHERE user_roles.role_id=role_rights.role_id AND "
    "      user_roles.user_id=:user_id ";
  Qry.DeclareVariable( "user_id",otInteger );
  Qry.SetVariable( "user_id", user.user_id );
  Qry.Execute();
  for(;!Qry.Eof;Qry.Next())
    user.access.rights.push_back(Qry.FieldAsInteger("right_id"));
  Qry.Clear();
  Qry.CreateVariable( "user_id", otInteger, user.user_id );
  Qry.SQLText="SELECT airline FROM aro_airlines WHERE aro_id=:user_id";
  for(Qry.Execute();!Qry.Eof;Qry.Next())
    user.access.airlines.push_back(Qry.FieldAsString("airline"));
  Qry.SQLText="SELECT airp FROM aro_airps WHERE aro_id=:user_id";
  for(Qry.Execute();!Qry.Eof;Qry.Next())
    user.access.airps.push_back(Qry.FieldAsString("airp"));
  if (user.access.airlines.empty() &&
      (user.user_type==utSupport||
       user.user_type==utAirport)) user.access.airlines_permit=false;
  if (user.access.airps.empty() &&
      (user.user_type==utSupport||
       user.user_type==utAirline)) user.access.airps_permit=false;

  //проверим ограничение доступа по сессии
  vector<string> airlines;
  SeparateString(JxtContext::getJxtContHandler()->sysContext()->read("session_airlines"),'/',airlines);

  if (!airlines.empty())
    MergeAccess(user.access.airlines,user.access.airlines_permit,airlines,true);

  //проверим ограничение доступа по собственникам пульта
  Qry.Clear();
  Qry.SQLText=
    "SELECT airline,pr_denial FROM desk_owners "
    "WHERE desk=:desk ORDER BY DECODE(airline,NULL,0,1)";
  Qry.CreateVariable("desk",otString,InitData.pult);
  Qry.Execute();
  bool pr_denial_all=true;
  if (!Qry.Eof && Qry.FieldIsNULL("airline"))
  {
    pr_denial_all=Qry.FieldAsInteger("pr_denial")!=0;
    Qry.Next();
  };

  airlines.clear();
  for(;!Qry.Eof;Qry.Next())
  {
    if (Qry.FieldIsNULL("airline")) continue;
    bool pr_denial=Qry.FieldAsInteger("pr_denial")!=0;
    if (!pr_denial_all && pr_denial ||
        pr_denial_all && !pr_denial)
      airlines.push_back(Qry.FieldAsString("airline"));
  };
  MergeAccess(user.access.airlines,user.access.airlines_permit,airlines,pr_denial_all);
  if (airlines.empty() && pr_denial_all)
    throw AstraLocale::UserException( "MSG.DESK.TURNED_OFF" );

  //пользовательские настройки
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
      char* field;
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

bool TReqInfo::CheckAirline(const string &airline)
{
  if (user.access.airlines_permit)
  {
    //список разрешенных
    return find(user.access.airlines.begin(),
                user.access.airlines.end(),airline)!=user.access.airlines.end();
  }
  else
  {
    //список запрещенных
    return find(user.access.airlines.begin(),
                user.access.airlines.end(),airline)==user.access.airlines.end();
  };
};

bool TReqInfo::CheckAirp(const string &airp)
{
  if (user.access.airps_permit)
  {
    //список разрешенных
    return find(user.access.airps.begin(),
                user.access.airps.end(),airp)!=user.access.airps.end();
  }
  else
  {
    //список запрещенных
    return find(user.access.airps.begin(),
                user.access.airps.end(),airp)==user.access.airps.end();
  };
};

void TReqInfo::MergeAccess(vector<string> &a, bool &ap,
                           vector<string> b, bool bp)
{
  if (a.empty() && ap ||
      b.empty() && bp)
  {
    //все запрещено
    a.clear();
    ap=true;
  }
  else
  {
    if (a.empty() && !ap ||
        b.empty() && !bp)
    {
      //копируем доступ
      if (a.empty())
      {
        a.swap(b);
        ap=bp;
      };
    }
    else
    {
      //4 случая
      if (ap)
      {
        if (bp)
        {
          //a and b
          for(vector<string>::iterator i=a.begin();i!=a.end();)
          {
            if (find(b.begin(),b.end(),*i)!=b.end())
              i++;
            else
              i=a.erase(i);
          };
          ap=true;
        }
        else
        {
          //a minus b
          for(vector<string>::iterator i=a.begin();i!=a.end();)
          {
            if (find(b.begin(),b.end(),*i)!=b.end())
              i=a.erase(i);
            else
              i++;
          };
          ap=true;
        };
      }
      else
      {
        if (bp)
        {
          //b minus a
          for(vector<string>::iterator i=b.begin();i!=b.end();)
          {
            if (find(a.begin(),a.end(),*i)!=a.end())
              i=b.erase(i);
            else
              i++;
          };
          a.swap(b);
          ap=true;
        }
        else
        {
          //a or b
          for(vector<string>::iterator i=b.begin();i!=b.end();i++)
          {
            if (find(a.begin(),a.end(),*i)==a.end())
              a.push_back(*i);
          };
          ap=false;
        };
      };
    };
  };
};

string GetSQLEnum(const vector<string> &values)
{
  string res;
  for(vector<string>::const_iterator i=values.begin();i!=values.end();i++)
  {
    if (i->empty()) continue;
    if (!res.empty()) res.append(",");
    res.append("'"+(*i)+"'");
  };
  if (!res.empty()) res=" ("+res+") ";
  return res;
};

long TReqInfo::getExecuteMSec()
{
	ptime t( microsec_clock::universal_time() );
	time_duration pt = t - execute_time;
	return pt.total_milliseconds();
}

void MsgToLog(TLogMsg &msg, const string &screen, const string &user, const string &desk)
{
    TQuery Qry(&OraSession);
    Qry.SQLText =
        "BEGIN "
        "  INSERT INTO events(type,time,ev_order,msg,screen,ev_user,station,id1,id2,id3) "
        "  VALUES(:type,system.UTCSYSDATE,events__seq.nextval,"
        "         SUBSTR(:msg,1,250),:screen,:ev_user,:station,:id1,:id2,:id3) "
        "  RETURNING time,ev_order INTO :ev_time,:ev_order; "
        "END;";
    Qry.DeclareVariable("type", otString);
    Qry.DeclareVariable("msg", otString);
    Qry.DeclareVariable("screen", otString);
    Qry.DeclareVariable("ev_user", otString);
    Qry.DeclareVariable("station", otString);
    Qry.DeclareVariable("id1", otInteger);
    Qry.DeclareVariable("id2", otInteger);
    Qry.DeclareVariable("id3", otInteger);
    Qry.CreateVariable("ev_time", otDate, FNull);
    Qry.CreateVariable("ev_order", otInteger, FNull);
    Qry.SetVariable("type", EncodeEventType(msg.ev_type));
    Qry.SetVariable("msg", msg.msg);
    Qry.SetVariable("screen", screen);
    Qry.SetVariable("ev_user", user);
    Qry.SetVariable("station", desk);
    if(msg.id1!=0)
        Qry.SetVariable("id1", msg.id1);
    else
        Qry.SetVariable("id1", FNull);
    if(msg.id2!=0)
        Qry.SetVariable("id2", msg.id2);
    else
        Qry.SetVariable("id2", FNull);
    if(msg.id3!=0)
        Qry.SetVariable("id3", msg.id3);
    else
        Qry.SetVariable("id3", FNull);
    Qry.Execute();
    if (!Qry.VariableIsNULL("ev_time"))
      msg.ev_time=Qry.GetVariableAsDateTime("ev_time");
    else
      msg.ev_time=ASTRA::NoExists;
    if (!Qry.VariableIsNULL("ev_order"))
      msg.ev_order=Qry.GetVariableAsInteger("ev_order");
    else
      msg.ev_order=ASTRA::NoExists;
};

void MsgToLog(std::string msg, ASTRA::TEventType ev_type, int id1, int id2, int id3)
{
    TLogMsg msgh;
    msgh.msg = msg;
    msgh.ev_type = ev_type;
    msgh.id1 = id1;
    msgh.id2 = id2;
    msgh.id3 = id3;
    MsgToLog(msgh,"","","");
};

void TReqInfo::MsgToLog(string msg, TEventType ev_type, int id1, int id2, int id3)
{
    TLogMsg msgh;
    msgh.msg = msg;
    msgh.ev_type = ev_type;
    msgh.id1 = id1;
    msgh.id2 = id2;
    msgh.id3 = id3;
    MsgToLog(msgh);
}

void TReqInfo::MsgToLog(TLogMsg &msg)
{
    ::MsgToLog(msg,screen.name,user.descr,desk.code);
};


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

TDocType DecodeDocType(const char* s)
{
  unsigned int i;
  for(i=0;i<sizeof(TDocTypeS)/sizeof(TDocTypeS[0]);i+=1) if (strcmp(s,TDocTypeS[i])==0) break;
  if (i<sizeof(TDocTypeS)/sizeof(TDocTypeS[0]))
    return (TDocType)i;
  else
    return dtUnknown;
};

const char* EncodeDocType(TDocType doc)
{
  return TDocTypeS[doc];
};

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

namespace ASTRA {
void showProgError(const std::string &message, int code )
{
  XMLRequestCtxt *xmlRC = getXmlCtxt();
  xmlNodePtr resNode = NodeAsNode("/term/answer", xmlRC->resDoc);
  resNode = ReplaceTextChild( ReplaceTextChild( resNode, "command" ), "error", message );
  SetProp(resNode, "code", code);
};

void showError(const std::string &message, int code)
{
  XMLRequestCtxt *xmlRC = getXmlCtxt();
  xmlNodePtr resNode = NodeAsNode("/term/answer", xmlRC->resDoc);
  resNode = ReplaceTextChild( ReplaceTextChild( resNode, "command" ), "user_error", message );
  SetProp(resNode, "code", code);
};

void showErrorMessage(const std::string &message, int code )
{
  XMLRequestCtxt *xmlRC = getXmlCtxt();
  xmlNodePtr resNode = NodeAsNode("/term/answer", xmlRC->resDoc);
  resNode =  ReplaceTextChild( ReplaceTextChild( resNode, "command" ), "user_error_message", message );
  SetProp(resNode, "code", code);
};

void showErrorMessageAndRollback(const std::string &message, int code )
{
  showErrorMessage(message,code);
  throw UserException2();
}

void showMessage(const std::string &message, int code )
{
  XMLRequestCtxt *xmlRC = getXmlCtxt();
  xmlNodePtr resNode = NodeAsNode("/term/answer", xmlRC->resDoc);
  resNode = ReplaceTextChild( ReplaceTextChild( resNode, "command" ), "message", message );
  SetProp(resNode, "code", code);
};
} // end namespace ASTRA

namespace AstraLocale {

void getLexemaText( LexemaData lexemaData, string &text, string &master_lexema_id, string lang = "" )
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

std::string getLocaleText(LexemaData lexemaData)
{
    string text, master_lexema_id;
    getLexemaText( lexemaData, text, master_lexema_id );
    return text;
}

string getLocaleText(const std::string &vlexema, std::string lang)
{
    LexemaData lexemaData;
    lexemaData.lexema_id = vlexema;
    string text, master_lexema_id;
    getLexemaText( lexemaData, text, master_lexema_id, lang );
    return text;
}

string getLocaleText(const std::string &vlexema, LParams &aparams, string lang)
{
    LexemaData lexemaData;
    lexemaData.lexema_id = vlexema;
    lexemaData.lparams = aparams;
    string text, master_lexema_id;
    getLexemaText( lexemaData, text, master_lexema_id, lang );
    return text;
}

void showErrorMessage( std::string vlexema, LParams &aparams, int code)
{
	LexemaData lexemaData;
	lexemaData.lexema_id = vlexema;
	lexemaData.lparams = aparams;
    showErrorMessage(lexemaData, code);
}

void showMessage( std::string vlexema, LParams &aparams, int code)
{
	LexemaData lexemaData;
	lexemaData.lexema_id = vlexema;
	lexemaData.lparams = aparams;
    showMessage(lexemaData, code);
}

void showMessage(LexemaData lexemaData, int code)
{
  string text, master_lexema_id;
  getLexemaText( lexemaData, text, master_lexema_id );
  XMLRequestCtxt *xmlRC = getXmlCtxt();
  xmlNodePtr resNode = NodeAsNode("/term/answer", xmlRC->resDoc);
  resNode = ReplaceTextChild( ReplaceTextChild( resNode, "command" ), "message", text );
  SetProp(resNode, "lexema_id", master_lexema_id);
  SetProp(resNode, "code", code);
}

void showMessage(const std::string &lexema_id, int code)
{
	LexemaData lexemaData;
	lexemaData.lexema_id = lexema_id;
	showMessage( lexemaData, code );
}

void showErrorMessage(LexemaData lexemaData, int code)
{
  string text, master_lexema_id;
  getLexemaText( lexemaData, text, master_lexema_id );
  XMLRequestCtxt *xmlRC = getXmlCtxt();
  xmlNodePtr resNode = NodeAsNode("/term/answer", xmlRC->resDoc);
  resNode =  ReplaceTextChild( ReplaceTextChild( resNode, "command" ), "user_error_message", text );
  SetProp(resNode, "lexema_id", master_lexema_id);
  SetProp(resNode, "code", code);
};

void showErrorMessage(const std::string &lexema_id, int code)
{
	LexemaData lexemaData;
	lexemaData.lexema_id = lexema_id;
	showErrorMessage( lexemaData, code );
}

void showError(LexemaData lexemaData, int code)
{
  string text, master_lexema_id;
  getLexemaText( lexemaData, text, master_lexema_id );
  XMLRequestCtxt *xmlRC = getXmlCtxt();
  xmlNodePtr resNode = NodeAsNode("/term/answer", xmlRC->resDoc);
  resNode = ReplaceTextChild( ReplaceTextChild( resNode, "command" ), "user_error", text );
  SetProp(resNode, "lexema_id", master_lexema_id);
  SetProp(resNode, "code", code);
};

void showError(const std::string &lexema_id, int code)
{
	LexemaData lexemaData;
	lexemaData.lexema_id = lexema_id;
	showError( lexemaData, code );
}

void showProgError(LexemaData lexemaData, int code )
{
  string text, master_lexema_id;
  getLexemaText( lexemaData, text, master_lexema_id );
  XMLRequestCtxt *xmlRC = getXmlCtxt();
  xmlNodePtr resNode = NodeAsNode("/term/answer", xmlRC->resDoc);
  resNode = ReplaceTextChild( ReplaceTextChild( resNode, "command" ), "error", text );
  SetProp(resNode, "lexema_id", master_lexema_id);
  SetProp(resNode, "code", code);
};

void showProgError(const std::string &lexema_id, int code )
{
	LexemaData lexemaData;
	lexemaData.lexema_id = lexema_id;
	showProgError( lexemaData, code );
}

void showErrorMessageAndRollback(const std::string &lexema_id, int code )
{
	LexemaData lexemaData;
	lexemaData.lexema_id = lexema_id;
	showErrorMessageAndRollback( lexemaData, code );
}

void showErrorMessageAndRollback(LexemaData lexemaData, int code )
{
  showErrorMessage(lexemaData,code);
  throw UserException2();
}

} // end namespace AstraLocale

int getTCLParam(const char* name, int min, int max, int def)
{
  int res=NoExists;
  char r[100];
  r[0]=0;
  try
  {
    if ( get_param( name, r, sizeof( r ) ) < 0 )
      throw EXCEPTIONS::Exception( "Can't read TCL param %s", name );
    if ( StrToInt(r,res)==EOF ||
         min!=NoExists && res<min ||
         max!=NoExists && res>max)
      throw EXCEPTIONS::Exception( "Wrong TCL param %s=%s", name, r );
  }
  catch(std::exception &e)
  {
    if (def==NoExists) throw;
    res=def;
    ProgError( STDLOG, e.what() );
  };

  ProgTrace( TRACE5, "TCL param %s=%d", name, res );
  return res;
};

bool get_enable_unload_pectab()
{
    bool result = true;
    Tcl_Obj *obj;
    obj=Tcl_GetVar2Ex(getTclInterpretator(),
            "ENABLE_UNLOAD_PECTAB",0,TCL_GLOBAL_ONLY);
    if(!obj)
        result = false;
    else {
      static char buf[200];
      buf[199]=0;
      strcpy(buf,Tcl_GetString(obj));
      int ENABLE_UNLOAD_PECTAB;
      if(StrToInt(buf, ENABLE_UNLOAD_PECTAB) == EOF)
          result = false;
      else
          result = ENABLE_UNLOAD_PECTAB != 0;
    }
    return result;
}

bool get_test_server()
{
    bool result = true;
    if(TReqInfo::Instance()->desk.version != "201101-0118747") {
        Tcl_Obj *obj;
        obj=Tcl_GetVar2Ex(getTclInterpretator(),
                "TEST_SERVER",0,TCL_GLOBAL_ONLY);
        if(!obj)
            result = true;
        else {
            static char buf[200];
            buf[199]=0;
            strcpy(buf,Tcl_GetString(obj));
            int TEST_SERVER;
            if(StrToInt(buf, TEST_SERVER) == EOF)
                result = true;
            else
                result = TEST_SERVER != 0;
        }
    }
    return result;
}

bool get_enable_fr_design()
{
    bool result = true;
    Tcl_Obj *obj;
    obj=Tcl_GetVar2Ex(getTclInterpretator(),
            "ENABLE_FR_DESIGN",0,TCL_GLOBAL_ONLY);
    if(!obj)
        result = false;
    else {
      static char buf[200];
      buf[199]=0;
      strcpy(buf,Tcl_GetString(obj));
      int ENABLE_FR_DESIGN;
      if(StrToInt(buf, ENABLE_FR_DESIGN) == EOF)
          result = false;
      else
          result = ENABLE_FR_DESIGN != 0;
    }
    return result;
}

const char* OWN_POINT_ADDR()
{
  static string OWNADDR;
  if ( OWNADDR.empty() ) {
    char r[100];
    r[0]=0;
    if ( get_param( "OWN_POINT_ADDR", r, sizeof( r ) ) < 0 )
      throw EXCEPTIONS::Exception( "Can't read param OWN_POINT_ADDR" );
    OWNADDR = r;
  }
  return OWNADDR.c_str();
};

const char* SERVER_ID()
{
  static string SERVERID;
  if ( SERVERID.empty() ) {
    char r[100];
    r[0]=0;
    if ( get_param( "SERVER_ID", r, sizeof( r ) ) < 0 )
      throw EXCEPTIONS::Exception( "Can't read param SERVER_ID" );
    SERVERID = r;
  }
  return SERVERID.c_str();
};

const bool USE_SEANCES()
{
  static int VAR=NoExists;
  if (VAR==NoExists)
    VAR=getTCLParam("USE_SEANCES",NoExists,NoExists,0);
  return VAR!=0;
};


void showBasicInfo(void)
{
  XMLRequestCtxt *xmlRC = getXmlCtxt();
  TReqInfo *reqInfo = TReqInfo::Instance();
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
  NewTextChild(resNode, "use_seances", (int)USE_SEANCES());

  NewTextChild(resNode, "server_id", SERVER_ID() );

  TQuery Qry(&OraSession);

  if (!reqInfo->user.login.empty())
  {
    node = NewTextChild(resNode,"user");
    NewTextChild(node, "login",reqInfo->user.login);
    NewTextChild(node, "type",reqInfo->user.user_type);
    NewTextChild(node, "time_form",reqInfo->user.sets.time);
    //настройки пользователя
    xmlNodePtr setsNode = NewTextChild(node, "settings");
    NewTextChild(setsNode, "time", reqInfo->user.sets.time);
    NewTextChild(setsNode, "disp_airline", reqInfo->user.sets.disp_airline);
    NewTextChild(setsNode, "disp_airp", reqInfo->user.sets.disp_airp);
    NewTextChild(setsNode, "disp_craft", reqInfo->user.sets.disp_craft);
    NewTextChild(setsNode, "disp_suffix", reqInfo->user.sets.disp_suffix);
    //доступ
    xmlNodePtr accessNode = NewTextChild(node, "access");
    //права доступа к операциям
    node = NewTextChild(accessNode, "rights");
    for(vector<int>::const_iterator i=reqInfo->user.access.rights.begin();
                                    i!=reqInfo->user.access.rights.end();i++)
      NewTextChild(node,"right",*i);
    //права доступа к авиакомпаниям
    node = NewTextChild(accessNode, "airlines");
    for(vector<string>::const_iterator i=reqInfo->user.access.airlines.begin();
                                       i!=reqInfo->user.access.airlines.end();i++)
      NewTextChild(node,"airline",*i);
    NewTextChild(accessNode, "airlines_permit", (int)reqInfo->user.access.airlines_permit);
    //права доступа к аэропортам
    node = NewTextChild(accessNode, "airps");
    for(vector<string>::const_iterator i=reqInfo->user.access.airps.begin();
                                       i!=reqInfo->user.access.airps.end();i++)
      NewTextChild(node,"airp",*i);
    NewTextChild(accessNode, "airps_permit", (int)reqInfo->user.access.airps_permit);
  };
  if (!reqInfo->desk.code.empty())
  {
    node = NewTextChild(resNode,"desk");
    NewTextChild(node,"city",reqInfo->desk.city);
    if (!reqInfo->desk.compatible(LATIN_VERSION))
      NewTextChild(node,"lang",reqInfo->desk.lang);
    NewTextChild(node,"currency",reqInfo->desk.currency);
    NewTextChild(node,"time",DateTimeToStr( reqInfo->desk.time ) );
    NewTextChild(node,"time_utc",DateTimeToStr(NowUTC()) );
    //настройки пользователя
    xmlNodePtr setsNode = NewTextChild(node, "settings");
    if (reqInfo->desk.compatible(DEFER_ETSTATUS_VERSION))
    {
      Qry.Clear();
      Qry.SQLText="SELECT defer_etstatus FROM desk_grp_sets WHERE grp_id=:grp_id";
      Qry.CreateVariable("grp_id",otInteger,reqInfo->desk.grp_id);
      Qry.Execute();
      if (!Qry.Eof && !Qry.FieldIsNULL("defer_etstatus"))
        NewTextChild(setsNode,"defer_etstatus",(int)(Qry.FieldAsInteger("defer_etstatus")!=0));
      else
        NewTextChild(setsNode,"defer_etstatus",(int)false);
    }
    else NewTextChild(setsNode,"defer_etstatus",(int)true);

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

    xmlNodePtr modeNode=NewTextChild(node,EncodeOperMode(reqInfo->desk.mode).c_str());
    if (reqInfo->desk.mode==omCUTE)
    {
      //передаем параметры для CUTE

      //if (!reqInfo->user.login.empty())
      {
        xmlNodePtr accessNode=NewTextChild(modeNode,"airlines");
        TAirlines &airlines=(TAirlines&)(base_tables.get("airlines"));
        if (reqInfo->user.access.airlines_permit)
          for(vector<string>::const_iterator i=reqInfo->user.access.airlines.begin();
                                             i!=reqInfo->user.access.airlines.end();i++)
          {
            try
            {
              TAirlinesRow &row=(TAirlinesRow&)(airlines.get_row("code",*i));
              xmlNodePtr airlineNode=NewTextChild(accessNode,"airline");
              NewTextChild(airlineNode,"code",row.code);
              NewTextChild(airlineNode,"code_lat",row.code_lat,"");
              int aircode;
              if (StrToInt(row.aircode.c_str(),aircode)!=EOF && row.aircode.size()==3)
                NewTextChild(airlineNode,"aircode",row.aircode,"");
            }
            catch(EBaseTableError) {}
          };
      };
      xmlNodePtr devNode,paramNode;
      //ATB
      devNode=NewTextChild(modeNode,"ATB");
      paramNode=NewTextChild(devNode,"fmt_params");
      NewTextChild(paramNode,"pr_lat",0);
      NewTextChild(paramNode,"encoding","WINDOWS-1251");
      paramNode=NewTextChild(devNode,"mode_params");
      NewTextChild(paramNode,"multisession",(int)true,(int)false);
      NewTextChild(paramNode,"smode","S","S");
      NewTextChild(paramNode,"prn_type",90);
      //BTP
      devNode=NewTextChild(modeNode,"BTP");
      paramNode=NewTextChild(devNode,"fmt_params");
      NewTextChild(paramNode,"pr_lat",0);
      NewTextChild(paramNode,"encoding","WINDOWS-1251");
      paramNode=NewTextChild(devNode,"mode_params");
      NewTextChild(paramNode,"multisession",(int)true,(int)false);
      NewTextChild(paramNode,"smode","S","S");
      NewTextChild(paramNode,"logonum","01","01");
      NewTextChild(paramNode,"prn_type",91);
      //DCP
      devNode=NewTextChild(modeNode,"DCP");
      paramNode=NewTextChild(devNode,"fmt_params");
      NewTextChild(paramNode,"encoding","WINDOWS-1251");
      paramNode=NewTextChild(devNode,"mode_params");
      NewTextChild(paramNode,"multisession",(int)true,(int)true);
      //LSR
      devNode=NewTextChild(modeNode,"LSR");
      paramNode=NewTextChild(devNode,"fmt_params");
      NewTextChild(paramNode,"prefix","31");
      //NewTextChild(paramNode,"prefix","");
      NewTextChild(paramNode,"postfix","0D");
      paramNode=NewTextChild(devNode,"mode_params");
      NewTextChild(paramNode,"multisession",(int)true,(int)true);
    };

    //новый терминал
    Qry.Clear();
    Qry.SQLText=
      "SELECT term_mode,op_type,param_type,param_name,subparam_name,param_value "
      "FROM dev_params "
      "WHERE term_mode=:term_mode AND (desk_grp_id=:desk_grp_id OR desk_grp_id IS NULL) "
      "ORDER BY op_type,param_type,param_name,subparam_name NULLS FIRST,desk_grp_id NULLS LAST ";
    Qry.CreateVariable("term_mode",otString,EncodeOperMode(reqInfo->desk.mode));
    Qry.CreateVariable("desk_grp_id",otInteger,reqInfo->desk.grp_id);
    Qry.Execute();


    xmlNodePtr operTypeNode=NULL,paramTypeNode=NULL,paramNameNode=NULL;
    string op_type,param_type,param_name,subparam_name;
    for(;!Qry.Eof;Qry.Next())
    {
      if (op_type==Qry.FieldAsString("op_type") &&
          param_type==Qry.FieldAsString("param_type") &&
          param_name==Qry.FieldAsString("param_name") &&
          subparam_name==Qry.FieldAsString("subparam_name")) continue;

      op_type=Qry.FieldAsString("op_type");
      param_type=Qry.FieldAsString("param_type");
      param_name=Qry.FieldAsString("param_name");
      subparam_name=Qry.FieldAsString("subparam_name");

      if (operTypeNode==NULL || (const char*)operTypeNode->name!=op_type)
      {
        operTypeNode=NewTextChild(modeNode,op_type.c_str());
        paramTypeNode=NULL;
        paramNameNode=NULL;
      };
      if (paramTypeNode==NULL || (const char*)paramTypeNode->name!=param_type)
      {
        paramTypeNode=NewTextChild(operTypeNode,param_type.c_str());
        paramNameNode=NULL;
      };
      if (paramNameNode==NULL || (const char*)paramNameNode->name!=param_name)
      {
        if (subparam_name.empty())
          paramNameNode=NewTextChild(paramTypeNode,param_name.c_str(),Qry.FieldAsString("param_value"));
        else
          paramNameNode=NewTextChild(paramTypeNode,param_name.c_str());
      };
      if (!subparam_name.empty())
        NewTextChild(paramNameNode,subparam_name.c_str(),Qry.FieldAsString("param_value"));
    };
  };
  node = NewTextChild( resNode, "screen" );
  NewTextChild( node, "version", reqInfo->screen.version );
};

/***************************************************************************************/
void SysReqInterface::ErrorToLog(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  if (reqNode==NULL) return;
  xmlNodePtr node=reqNode->children;
  for(;node!=NULL;node=node->next)
  {
    if (strcmp((char*)node->name,"msg")==0)
      ProgError( STDLOG, "Client error (ver. %s): %s.",
                         TReqInfo::Instance()->desk.version.c_str(),
                         NodeAsString(node) ) ;
  };
}

tz_database &get_tz_database()
{
  static bool init=false;
  static tz_database tz_db;
  if (!init) {
    try
    {
      tz_db.load_from_file("date_time_zonespec.csv");
      init=true;
    }
    catch (boost::local_time::data_not_accessible)
    {
      throw EXCEPTIONS::Exception("File 'date_time_zonespec.csv' not found");
    }
    catch (boost::local_time::bad_field_count)
    {
      throw EXCEPTIONS::Exception("File 'date_time_zonespec.csv' wrong format");
    };
  }
  return tz_db;
}

string& AirpTZRegion(string airp, bool with_exception)
{
  if (airp.empty()) throw EXCEPTIONS::Exception("Airport not specified");
  TAirpsRow& row=(TAirpsRow&)base_tables.get("airps").get_row("code",airp);
  return CityTZRegion(row.city,with_exception);
};

string& CityTZRegion(string city, bool with_exception)
{
  if (city.empty()) throw EXCEPTIONS::Exception("City not specified");
  TCitiesRow& row=(TCitiesRow&)base_tables.get("cities").get_row("code",city);
  if (row.region.empty() && with_exception)
    throw AstraLocale::UserException("MSG.CITY.REGION_NOT_DEFINED",LParams() << LParam("city", city));
  return row.region;
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

TDateTime UTCToLocal(TDateTime d, string region)
{
  if (region.empty()) throw EXCEPTIONS::Exception("Region not specified");
  tz_database &tz_db = get_tz_database();
  time_zone_ptr tz = tz_db.time_zone_from_region(region);
  if (tz==NULL) throw EXCEPTIONS::Exception("Region '%s' not found",region.c_str());
  local_date_time ld(DateTimeToBoost(d),tz);
  return BoostToDateTime(ld.local_time());
}

TDateTime LocalToUTC(TDateTime d, string region, int is_dst)
{
  if (region.empty()) throw EXCEPTIONS::Exception("Region not specified");
  tz_database &tz_db = get_tz_database();
  time_zone_ptr tz = tz_db.time_zone_from_region(region);
  if (tz==NULL) throw EXCEPTIONS::Exception("Region '%s' not found",region.c_str());
  ptime pt=DateTimeToBoost(d);
  try {
    local_date_time ld(pt.date(),pt.time_of_day(),tz,local_date_time::EXCEPTION_ON_ERROR);
    return BoostToDateTime(ld.utc_time());
  }
  catch( boost::local_time::ambiguous_result ) {
  	if (is_dst == NoExists) throw;
  	local_date_time ld(pt.date(),pt.time_of_day(),tz,(bool)is_dst);
  	return BoostToDateTime(ld.utc_time());
  }
};

TDateTime UTCToClient(TDateTime d, string region)
{
  TReqInfo *reqInfo = TReqInfo::Instance();
  switch (reqInfo->user.sets.time)
  {
    case ustTimeUTC:
      return d;
    case ustTimeLocalDesk:
      return UTCToLocal(d,reqInfo->desk.tz_region);
    case ustTimeLocalAirp:
      return UTCToLocal(d,region);
    default:
      throw EXCEPTIONS::Exception("Unknown sets.time for user %s (user_id=%d)",reqInfo->user.login.c_str(),reqInfo->user.user_id);
  };
};

TDateTime ClientToUTC(TDateTime d, string region, int is_dst)
{
  TReqInfo *reqInfo = TReqInfo::Instance();
  switch (reqInfo->user.sets.time)
  {
    case ustTimeUTC:
      return d;
    case ustTimeLocalDesk:
      return LocalToUTC(d,reqInfo->desk.tz_region,is_dst);
    case ustTimeLocalAirp:
      return LocalToUTC(d,region,is_dst);
    default:
      throw EXCEPTIONS::Exception("Unknown sets.time for user %s (user_id=%d)",reqInfo->user.login.c_str(),reqInfo->user.user_id);
  };
};

bool is_dst(TDateTime d, string region)
{
	if (region.empty()) throw EXCEPTIONS::Exception("Region not specified");
	ptime	utcd = DateTimeToBoost( d );
  tz_database &tz_db = get_tz_database();
  time_zone_ptr tz = tz_db.time_zone_from_region( region );
  if (tz==NULL) throw EXCEPTIONS::Exception("Region '%s' not found",region.c_str());
  local_date_time ld( utcd, tz ); /* определяем текущее время локальное */
  return ( tz->has_dst() && ld.is_dst() );
}

char ToLatPnrAddr(char c)
{
  if ((unsigned char)c>=0x80)
  {
    ByteReplace(&c,1,rus_pnr,lat_pnr);
    if ((unsigned char)c>=0x80) c='?';
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

bool is_lat(const std::string &value)
{
    bool result = true;
    char c;
    for(string::const_iterator i=value.begin();i!=value.end();i++)
    {
        c=*i;
        if ((unsigned char)c>=0x80) {
            result = false;
            break;
        }
    }
    return result;
}

string transliter(const string &value, int fmt, bool pr_lat)
{
  string result;
  if (pr_lat)
  {
    char c;
    char *c2;
    for(string::const_iterator i=value.begin();i!=value.end();i++)
    {
      c=*i;
      if ((unsigned char)c>=0x80)
      {
        switch(ToUpper(c))
        {
          case 'А': c2 = "A"; break;
          case 'Б': c2 = "B"; break;
          case 'В': c2 = "V"; break;
          case 'Г': c2 = "G"; break;
          case 'Д': c2 = "D"; break;
          case 'Е': c2 = "E"; break;
          case 'Ё': switch(fmt)
                    {
                      case 2:  c2 = "YO";
                      default: c2 = "IO";
                    };
                    break;
          case 'Ж': c2 = "ZH"; break;
          case 'З': c2 = "Z"; break;
          case 'И': c2 = "I"; break;
          case 'Й': c2 = "I"; break;
          case 'К': c2 = "K"; break;
          case 'Л': c2 = "L"; break;
          case 'М': c2 = "M"; break;
          case 'Н': c2 = "N"; break;
          case 'О': c2 = "O"; break;
          case 'П': c2 = "P"; break;
          case 'Р': c2 = "R"; break;
          case 'С': c2 = "S"; break;
          case 'Т': c2 = "T"; break;
          case 'У': c2 = "U"; break;
          case 'Ф': c2 = "F"; break;
          case 'Х': c2 = "KH"; break;
          case 'Ц': c2 = "TS"; break;
          case 'Ч': c2 = "CH"; break;
          case 'Ш': c2 = "SH"; break;
          case 'Щ': switch(fmt)
                    {
                      case 2:  c2 = "SH";
                      default: c2 = "SHCH";
                    };
                    break;
          case 'Ъ': c2 = ""; break;
          case 'Ы': c2 = "Y"; break;
          case 'Ь': c2 = ""; break;
          case 'Э': c2 = "E"; break;
          case 'Ю': switch(fmt)
                    {
                      case 2:  c2 = "YU";
                      default: c2 = "IU";
                    };
                    break;
          case 'Я': switch(fmt)
                    {
                      case 2:  c2 = "YA";
                      default: c2 = "IA";
                    };
                    break;
          default:  c2 = "?";
        };
        if (ToUpper(c)!=c) result+=lowerc(c2); else result+=c2;
      }
      else result+=c;
    };
  }
  else  result=value;
  return result;
};

bool transliter_equal(const string &value1, const string &value2, int fmt)
{
  return transliter(value1,fmt,true)==transliter(value2,fmt,true);
};

bool transliter_equal(const string &value1, const string &value2)
{
  for(int fmt=1;fmt<=2;fmt++)
    if (transliter_equal(value1,value2,fmt)) return true;
  return false;
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
    lexemeData.lexema_id=NodeAsString("/lexeme_data/id",msgDoc.docPtr());
    xmlNodePtr node=NodeAsNode("/lexeme_data/params",msgDoc.docPtr())->children;
    for(;node!=NULL;node=node->next)
    {
      lexemeData.lparams << LParam((const char*)node->name, NodeAsString(node));
    };
    msg=getLocaleText(lexemeData);
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



















