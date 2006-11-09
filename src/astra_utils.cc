#include "setup.h"
#include "astra_utils.h"
#include "astra_consts.h"
#include "JxtInterface.h"

#include <stdarg.h>
#include "basic.h"
#include "oralib.h"
#include "exceptions.h"
#define NICKNAME "VLAD"
#include "test.h"
#include <string.h>
#include "stl_utils.h"
#include "xml_unit.h"
#include "monitor_ctl.h"
#include "cfgproc.h"

using namespace std;
using namespace ASTRA;
using namespace BASIC;
using namespace EXCEPTIONS;
using namespace JxtContext;
using namespace boost::local_time;
using namespace boost::posix_time;

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
}

const string COMMON_ORAUSER()
{
  static string CORAUSER;
  if ( CORAUSER.empty() ) {
    char r[100];
    r[0]=0;
    if ( get_param( "COMMON_ORAUSER", r, sizeof( r ) ) < 0 )
      throw Exception( "Can't read param COMMON_ORAUSER" );
    CORAUSER = r;
    ProgTrace( TRACE5, "COMMON_ORAUSER=%s", CORAUSER.c_str() );
  }
  return CORAUSER;
}

TScreen::TScreen()
{
  id=0;
  version=0;
};

void TScreen::clear()
{
  id=0;
  version=0;
  name.clear();
};

TDesk::TDesk()
{
  time = 0;
};

void TDesk::clear()
{
  code.clear();
  city.clear();
  tz_region.clear();
  time = 0;
};

TUser::TUser()
{
  time_form = tfUnknown;
  user_id = -1;
};

void TUser::clear()
{
  login.clear();
  descr.clear();
  access.clear();
  time_form = tfUnknown;
  user_id=-1;
};

void TAccess::clear()
{
  rights.clear();
  airlines.clear();
  airps.clear();
}

TReqInfo::TReqInfo()
{
};

void TReqInfo::clear()
{
  desk.clear();
  user.clear();
  screen.clear();
};

TReqInfo *TReqInfo::Instance()
{
  static TReqInfo *instance_ = 0;
  if ( !instance_ )
    instance_ = new TReqInfo();
  return instance_;
};

void TReqInfo::Initialize( const std::string &vscreen, const std::string &vpult,
                           const std::string &vopr, bool checkUserLogon )
{
  clear();
  TQuery Qry(&OraSession);
  ProgTrace( TRACE5, "screen=%s, pult=|%s|, opr=|%s|", vscreen.c_str(), vpult.c_str(), vopr.c_str() );
  screen.name = upperc( vscreen );
  desk.code = vpult;
  string sql;

  Qry.Clear();
  sql = string( "SELECT id,version FROM " ) + COMMON_ORAUSER() + ".screen WHERE exe = :exe";
  Qry.SQLText = sql;
  Qry.DeclareVariable( "exe", otString );
  Qry.SetVariable( "exe", screen.name );
  Qry.Execute();
  if ( Qry.RowCount() == 0 )
    throw Exception( (string)"Unknown screen " + screen.name );
  screen.id = Qry.FieldAsInteger( "id" );
  screen.version = Qry.FieldAsInteger( "version" );

  Qry.Clear();
  sql = string("SELECT pr_denial, city, system.CityTZRegion(city) AS tz_region FROM ") +COMMON_ORAUSER()+ ".desks," +
        COMMON_ORAUSER() + ".sale_points " +
        " WHERE desks.code = UPPER(:pult) AND desks.point = sale_points.code ";

  Qry.SQLText = sql;
  Qry.DeclareVariable( "pult", otString );
  Qry.SetVariable( "pult", vpult );
  Qry.Execute();
  if ( Qry.RowCount() == 0 )
    throw UserException( "Пульт не зарегистрирован в системе. Обратитесь к администратору." );
  if ( Qry.FieldAsInteger( "pr_denial" ) != 0 )
    throw UserException( "Пульт отключен" );
  desk.city = Qry.FieldAsString( "city" );
  desk.tz_region = Qry.FieldAsString( "tz_region" );
  desk.time = UTCToLocal( NowUTC(), desk.tz_region );

  Qry.Clear();
  sql = "SELECT user_id, login, descr, type, pr_denial, time_form FROM " + COMMON_ORAUSER() + ".users2 "+
        " WHERE desk = UPPER(:pult) ";
  Qry.SQLText = sql;
  Qry.DeclareVariable( "pult", otString );
  Qry.SetVariable( "pult", vpult );
  Qry.Execute();

  if ( Qry.RowCount() == 0 )
  {
    if (!checkUserLogon)
     	return;
    else
      throw UserException( "Пользователь не вошел в систему. Используйте главный модуль." );
  };
  if ( !vopr.empty() )
    if ( vopr != Qry.FieldAsString( "login" ) )
      throw UserException( "Пользователь не вошел в систему. Используйте главный модуль." );

    if ( Qry.FieldAsInteger( "pr_denial" ) != 0 )
      throw UserException( "Пользователю отказано в доступе" );
  user.user_id = Qry.FieldAsInteger( "user_id" );
  user.descr = Qry.FieldAsString( "descr" );
  user.user_type = (TUserType)Qry.FieldAsInteger( "type" );
  user.login = Qry.FieldAsString( "login" );
  user.time_form = tfUnknown;
  if (strcmp(Qry.FieldAsString( "time_form" ),"UTC")==0)        user.time_form = tfUTC;
  if (strcmp(Qry.FieldAsString( "time_form" ),"LOCAL_DESK")==0) user.time_form = tfLocalDesk;
  if (strcmp(Qry.FieldAsString( "time_form" ),"LOCAL_ALL")==0)  user.time_form = tfLocalAll;

  //если служащий порта - проверим пульт с которого он заходит
  /*if (user.user_type==utAirport)
  {
    Qry.Clear();
    sql = string( "SELECT airps.city " ) +
                  "FROM " + COMMON_ORAUSER() + ".aro_airps,airps " +
                  "WHERE aro_airps.airp=airps.code AND "
                  "      airps.city=:city AND aro_airps.aro_id=:user_id AND rownum<2 ";
    Qry.SQLText=sql;
    Qry.CreateVariable("city",otString,desk.city);
    Qry.CreateVariable("user_id",otInteger,user.user_id);
    Qry.Execute();
    if (Qry.Eof)
      throw UserException( "Пользователю отказано в доступе с пульта %s", desk.code.c_str() );
  };*/

  Qry.Clear();
  sql = string( "SELECT DISTINCT screen_rights.right_id " ) +
                "FROM " + COMMON_ORAUSER() + ".user_roles, " +
                          COMMON_ORAUSER() + ".role_rights, " +
                          COMMON_ORAUSER() + ".screen_rights "+
                "WHERE user_roles.role_id=role_rights.role_id AND "
                "      role_rights.right_id=screen_rights.right_id AND "
                "      user_roles.user_id=:user_id AND screen_rights.screen_id=:screen_id ";
  Qry.SQLText=sql;
  Qry.DeclareVariable( "user_id",otInteger );
  Qry.DeclareVariable( "screen_id", otInteger );
  Qry.SetVariable( "user_id", user.user_id );
  Qry.SetVariable( "screen_id", screen.id );
  Qry.Execute();
  for(;!Qry.Eof;Qry.Next())
    user.access.rights.push_back(Qry.FieldAsInteger("right_id"));
  Qry.Clear();
  Qry.CreateVariable( "user_id", otInteger, user.user_id );
  sql = string("SELECT airline FROM ") + COMMON_ORAUSER() + ".aro_airlines WHERE aro_id=:user_id";
  Qry.SQLText=sql;
  for(Qry.Execute();!Qry.Eof;Qry.Next())
    user.access.airlines.push_back(Qry.FieldAsString("airline"));
  sql = string("SELECT airp FROM ") + COMMON_ORAUSER() + ".aro_airps WHERE aro_id=:user_id";
  Qry.SQLText=sql;
  for(Qry.Execute();!Qry.Eof;Qry.Next())
    user.access.airps.push_back(Qry.FieldAsString("airp"));
}

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
    TQuery *Qry = OraSession.CreateQuery();
    Qry->SQLText =
        "INSERT INTO astra.events(type,time,ev_order,msg,screen,ev_user,station,id1,id2,id3) "
        "VALUES(:type,system.UTCSYSDATE,events__seq.nextval,SUBSTR(:msg,1,250),:screen,:ev_user,:station,:id1,:id2,:id3) ";
    Qry->DeclareVariable("type", otString);
    Qry->DeclareVariable("msg", otString);
    Qry->DeclareVariable("screen", otString);
    Qry->DeclareVariable("ev_user", otString);
    Qry->DeclareVariable("station", otString);
    Qry->DeclareVariable("id1", otInteger);
    Qry->DeclareVariable("id2", otInteger);
    Qry->DeclareVariable("id3", otInteger);
    Qry->SetVariable("type", EncodeEventType(msg.ev_type));
    Qry->SetVariable("msg", msg.msg);
    Qry->SetVariable("screen", screen.name);
    Qry->SetVariable("ev_user", user.descr);
    Qry->SetVariable("station", desk.code);
    if(msg.id1)
        Qry->SetVariable("id1", msg.id1);
    else
        Qry->SetVariable("id1", FNull);
    if(msg.id2)
        Qry->SetVariable("id2", msg.id2);
    else
        Qry->SetVariable("id2", FNull);
    if(msg.id3)
        Qry->SetVariable("id3", msg.id3);
    else
        Qry->SetVariable("id3", FNull);
    try {
      Qry->Execute();
    }
    catch( ... ) {
      OraSession.DeleteQuery(*Qry);
      throw;
    }
    OraSession.DeleteQuery(*Qry);
}


/***************************************************************************************/

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

TClass DecodeClass(char* s)
{
  unsigned int i;
  for(i=0;i<sizeof(TClassS);i+=1) if (strcmp(s,TClassS[i])==0) break;
  if (i<sizeof(TClassS))
    return (TClass)i;
  else
    return NoClass;
};

char* EncodeClass(TClass cl)
{
  return (char*)TClassS[cl];
};

TPerson DecodePerson(char* s)
{
  unsigned int i;
  for(i=0;i<sizeof(TPersonS);i+=1) if (strcmp(s,TPersonS[i])==0) break;
  if (i<sizeof(TPersonS))
    return (TPerson)i;
  else
    return NoPerson;
};

char* EncodePerson(TPerson p)
{
  return (char*)TPersonS[p];
};

TQueue DecodeQueue(int q)
{
  unsigned int i;
  for(i=0;i<sizeof(TQueueS);i+=1) if (q==TQueueS[i]) break;
  if (i<sizeof(TQueueS))
    return (TQueue)i;
  else
    return NoQueue;
};

int EncodeQueue(TQueue q)
{
  return (int)TQueueS[q];
};

char DecodeStatus(char* s)
{
  unsigned int i;
  for(i=0;i<sizeof(TStatusS);i+=1) if (strcmp(s,TStatusS[i])==0) break;
  if (i<sizeof(TStatusS))
    return TStatusS[i][0];
  else
    return '\0';
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

char *EncodeSeatNo( char *Value, bool pr_latseat )
{
  if ( !pr_latseat )
    return CharReplace( Value, "ABCDEFGHIJK", "АБВГДЕЖЗИКЛ" );
  else return Value;
};

char *DecodeSeatNo( char *Value )
{
  return CharReplace( Value, "АБВГДЕЖЗИКЛ", "ABCDEFGHIJK" );
};

void showProgError(const std::string &message )
{
  XMLRequestCtxt *xmlRC = getXmlCtxt();
  xmlNodePtr resNode = NodeAsNode("/term/answer", xmlRC->resDoc);
  ReplaceTextChild( ReplaceTextChild( resNode, "command" ), "error", message );
};

void showError(const std::string &message, int code = 0 )
{
  XMLRequestCtxt *xmlRC = getXmlCtxt();
  xmlNodePtr resNode = NodeAsNode("/term/answer", xmlRC->resDoc);
  resNode = ReplaceTextChild( ReplaceTextChild( resNode, "command" ), "user_error", message );
  SetProp(resNode, "code", code);
};

void showErrorMessage(const std::string &message )
{
  XMLRequestCtxt *xmlRC = getXmlCtxt();
  xmlNodePtr resNode = NodeAsNode("/term/answer", xmlRC->resDoc);
  ReplaceTextChild( ReplaceTextChild( resNode, "command" ), "user_error_message", message );
};

void showErrorMessageAndRollback(const std::string &message )
{
  showErrorMessage(message);
  throw UserException2();
}

void showMessage(const std::string &message )
{
  XMLRequestCtxt *xmlRC = getXmlCtxt();
  xmlNodePtr resNode = NodeAsNode("/term/answer", xmlRC->resDoc);
  ReplaceTextChild( ReplaceTextChild( resNode, "command" ), "message", message );
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

  // достанем пользователя и пароль oracle
  string buf = get_connect_string();
  buf = buf.substr(0, buf.find("@"));
  string::size_type pos = buf.find("/");
  NewTextChild(resNode, "orauser", buf.substr(0, pos));
  NewTextChild(resNode, "orapasswd", buf.substr(pos + 1, string::npos));

  if (!reqInfo->user.login.empty())
  {
    node = NewTextChild(resNode,"user");
    NewTextChild(node, "login",reqInfo->user.login);
    NewTextChild(node, "type",reqInfo->user.user_type);
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
    //права доступа к аэропортам
    node = NewTextChild(accessNode, "airps");
    for(vector<string>::const_iterator i=reqInfo->user.access.airps.begin();
                                       i!=reqInfo->user.access.airps.end();i++)
      NewTextChild(node,"airp",*i);
  };
  if (!reqInfo->desk.code.empty())
  {
    node = NewTextChild(resNode,"desk");
    NewTextChild(node,"city",reqInfo->desk.city);
    NewTextChild(node,"time",DateTimeToStr( reqInfo->desk.time ) );
  };
  node = NewTextChild( resNode, "screen" );
  NewTextChild( node, "version", reqInfo->screen.version );
};

/***************************************************************************************/
void SysReqInterface::ErrorToLog(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    ProgError( STDLOG, "Модуль: %s. Пульт: %s. Оператор: %s. Время: %s. Ошибка: %s.",
               NodeAsString("/term/query/@screen", ctxt->reqDoc),
               ctxt->pult.c_str(), ctxt->opr.c_str(),
               DateTimeToStr(NowUTC(),"dd.mm.yyyy hh:nn:ss").c_str(),
               NodeAsString( "msg", reqNode ) ) ;
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
      throw Exception("File 'date_time_zonespec.csv' not found");
    }
    catch (boost::local_time::bad_field_count)
    {
      throw Exception("File 'date_time_zonespec.csv' wrong format");
    };
  }
  return tz_db;
}

TDateTime UTCToLocal(TDateTime d, string region)
{
  if (region.empty()) throw Exception("Region not specified",region.c_str());
  tz_database &tz_db = get_tz_database();
  time_zone_ptr tz = tz_db.time_zone_from_region(region);
  if (tz==NULL) throw Exception("Region '%s' not found",region.c_str());
  local_date_time ld(DateTimeToBoost(d),tz);
  return BoostToDateTime(ld.local_time());
}

TDateTime LocalToUTC(TDateTime d, string region)
{
  if (region.empty()) throw Exception("Region not specified",region.c_str());
  tz_database &tz_db = get_tz_database();
  time_zone_ptr tz = tz_db.time_zone_from_region(region);
  if (tz==NULL) throw Exception("Region '%s' not found",region.c_str());
  ptime pt=DateTimeToBoost(d);
  local_date_time ld(pt.date(),pt.time_of_day(),tz,local_date_time::EXCEPTION_ON_ERROR );
  return BoostToDateTime(ld.utc_time());
};

TDateTime UTCToClient(TDateTime d, string region)
{
  TReqInfo *reqInfo = TReqInfo::Instance();
  switch (reqInfo->user.time_form)
  {
    case tfUTC:
      return d;
    case tfLocalDesk:
      return UTCToLocal(d,reqInfo->desk.tz_region);
    case tfLocalAll:
      return UTCToLocal(d,region);
    default:
      throw Exception("Unknown time_form for user %s (user_id=%d)",reqInfo->user.login.c_str(),reqInfo->user.user_id);
  };
};

TDateTime ClientToUTC(TDateTime d, string region)
{
  TReqInfo *reqInfo = TReqInfo::Instance();
  switch (reqInfo->user.time_form)
  {
    case tfUTC:
      return d;
    case tfLocalDesk:
      return LocalToUTC(d,reqInfo->desk.tz_region);
    case tfLocalAll:
      return LocalToUTC(d,region);
    default:
      throw Exception("Unknown time_form for user %s (user_id=%d)",reqInfo->user.login.c_str(),reqInfo->user.user_id);
  };
};

bool is_dst(TDateTime d, string region)
{
	if (region.empty()) throw Exception("Region not specified",region.c_str());
	ptime	utcd = DateTimeToBoost( d );
  tz_database &tz_db = get_tz_database();
  time_zone_ptr tz = tz_db.time_zone_from_region( region );
  if (tz==NULL) throw Exception("Region '%s' not found",region.c_str());
  local_date_time ld( utcd, tz ); /* определяем текущее время локальное */
  return ( tz->has_dst() && ld.is_dst() );
}
