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
#include "misc.h"
#include "base_tables.h"
#include "jxt_cont.h"

#include "tcl_utils.h"

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

void TReqInfo::Initialize( const std::string &vscreen, const std::string &vpult,
                           const std::string &vopr, const std::string &vmode, bool checkUserLogon )
{
	if ( execute_time.is_not_a_date_time() )
		setPerform();
  clear();
  TQuery Qry(&OraSession);
  ProgTrace( TRACE5, "screen=%s, pult=|%s|, opr=|%s|", vscreen.c_str(), vpult.c_str(), vopr.c_str() );
  screen.name = upperc( vscreen );
  desk.code = vpult;
  if (vmode=="CUSE") desk.mode = omCUSE;
  if (vmode=="CUTE") desk.mode = omCUTE;
  string sql;

  Qry.Clear();
  Qry.SQLText = "SELECT id,version,pr_logon FROM screen WHERE exe = :exe";
  Qry.DeclareVariable( "exe", otString );
  Qry.SetVariable( "exe", screen.name );
  Qry.Execute();
  if ( Qry.RowCount() == 0 )
    throw Exception( (string)"Unknown screen " + screen.name );
  screen.id = Qry.FieldAsInteger( "id" );
  screen.version = Qry.FieldAsInteger( "version" );
  if ( Qry.FieldAsInteger( "pr_logon" ) == 0 )
  	return; //???
  Qry.Clear();
  Qry.SQLText =
    "SELECT city "
    "FROM desks,desk_grp "
    "WHERE desks.code = UPPER(:pult) AND desks.grp_id = desk_grp.grp_id ";
  Qry.DeclareVariable( "pult", otString );
  Qry.SetVariable( "pult", vpult );
  Qry.Execute();
  if ( Qry.RowCount() == 0 )
    throw UserException( "Пульт не зарегистрирован в системе. Обратитесь к администратору." );
  desk.city = Qry.FieldAsString( "city" );

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
    throw UserException("Не определен город пульта");
  if (Qry.FieldIsNULL("region"))
    throw UserException("Для города %s не задан регион",desk.city.c_str());
  desk.tz_region = Qry.FieldAsString( "region" );
  desk.time = UTCToLocal( NowUTC(), desk.tz_region );

  Qry.Clear();
  Qry.SQLText =
    "SELECT user_id, login, descr, type, pr_denial "
    "FROM users2 "
    "WHERE desk = UPPER(:pult) ";
  Qry.DeclareVariable( "pult", otString );
  Qry.SetVariable( "pult", vpult );
  Qry.Execute();
  if ( Qry.RowCount() == 0 )
  {
    if (!checkUserLogon)
     	return;
    else
      throw UserException( "Пользователю необходимо войти в систему с данного пульта. Используйте главный модуль." );
  };
  if ( !vopr.empty() )
    if ( vopr != Qry.FieldAsString( "login" ) )
      throw UserException( "Пользователю необходимо войти в систему с данного пульта. Используйте главный модуль." );
  if ( Qry.FieldAsInteger( "pr_denial" ) == -1 )
  	throw UserException( "Пользователь удален из системы" );
  if ( Qry.FieldAsInteger( "pr_denial" ) != 0 )
    throw UserException( "Пользователю отказано в доступе" );
  user.user_id = Qry.FieldAsInteger( "user_id" );
  user.descr = Qry.FieldAsString( "descr" );
  user.user_type = (TUserType)Qry.FieldAsInteger( "type" );
  user.login = Qry.FieldAsString( "login" );

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
      throw UserException( "Пользователю отказано в доступе с пульта %s", desk.code.c_str() );
  };*/

  Qry.Clear();
  Qry.SQLText=
    "SELECT DISTINCT screen_rights.right_id "
    "FROM user_roles,role_rights,screen_rights "
    "WHERE user_roles.role_id=role_rights.role_id AND "
    "      role_rights.right_id=screen_rights.right_id AND "
    "      user_roles.user_id=:user_id AND screen_rights.screen_id=:screen_id ";
  Qry.DeclareVariable( "user_id",otInteger );
  Qry.DeclareVariable( "screen_id", otInteger );
  Qry.SetVariable( "user_id", user.user_id );
  Qry.SetVariable( "screen_id", screen.id );
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
  SeparateString(getJxtContHandler()->sysContext()->read("session_airlines"),'/',airlines);

  if (!airlines.empty())
    MergeAccess(user.access.airlines,user.access.airlines_permit,airlines,true);

  //проверим ограничение доступа по собственникам пульта
  Qry.Clear();
  Qry.SQLText=
    "SELECT airline,pr_denial FROM desk_owners "
    "WHERE desk=:desk ORDER BY DECODE(airline,NULL,0,1)";
  Qry.CreateVariable("desk",otString,vpult);
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
    throw UserException( "Пульт отключен" );

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
          case ustCodeIATA:
          case ustCodeNativeICAO:
          case ustCodeIATAICAO:
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

string GetSQLEnum(vector<string> &values)
{
  string res;
  for(vector<string>::iterator i=values.begin();i!=values.end();i++)
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
    TQuery Qry(&OraSession);
    Qry.SQLText =
        "INSERT INTO events(type,time,ev_order,msg,screen,ev_user,station,id1,id2,id3) "
        "VALUES(:type,system.UTCSYSDATE,events__seq.nextval,SUBSTR(:msg,1,250),:screen,:ev_user,:station,:id1,:id2,:id3) ";
    Qry.DeclareVariable("type", otString);
    Qry.DeclareVariable("msg", otString);
    Qry.DeclareVariable("screen", otString);
    Qry.DeclareVariable("ev_user", otString);
    Qry.DeclareVariable("station", otString);
    Qry.DeclareVariable("id1", otInteger);
    Qry.DeclareVariable("id2", otInteger);
    Qry.DeclareVariable("id3", otInteger);
    Qry.SetVariable("type", EncodeEventType(msg.ev_type));
    Qry.SetVariable("msg", msg.msg);
    Qry.SetVariable("screen", screen.name);
    Qry.SetVariable("ev_user", user.descr);
    Qry.SetVariable("station", desk.code);
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

TDocType DecodeDocType(char* s)
{
  unsigned int i;
  for(i=0;i<sizeof(TDocTypeS);i+=1) if (strcmp(s,TDocTypeS[i])==0) break;
  if (i<sizeof(TDocTypeS))
    return (TDocType)i;
  else
    return dtUnknown;
};

char* EncodeDocType(TDocType doc)
{
  return (char*)TDocTypeS[doc];
};

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

TPaxStatus DecodePaxStatus(char* s)
{
  unsigned int i;
  for(i=0;i<sizeof(TPaxStatusS);i+=1) if (strcmp(s,TPaxStatusS[i])==0) break;
  if (i<sizeof(TPaxStatusS))
    return (TPaxStatus)i;
  else
    return psOk;
};

char* EncodePaxStatus(TPaxStatus s)
{
  return (char*)TPaxStatusS[s];
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

void showProgError(const std::string &message )
{
  XMLRequestCtxt *xmlRC = getXmlCtxt();
  xmlNodePtr resNode = NodeAsNode("/term/answer", xmlRC->resDoc);
  ReplaceTextChild( ReplaceTextChild( resNode, "command" ), "error", message );
};

void showError(const std::string &message, int code)
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
    NewTextChild(node,"time",DateTimeToStr( reqInfo->desk.time ) );
    if (reqInfo->desk.mode==omCUTE)
    {
      //передаем параметры для CUTE
      xmlNodePtr cuteNode=NewTextChild(node,"CUTE");
      xmlNodePtr accessNode=NewTextChild(cuteNode,"airlines");
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
      xmlNodePtr devNode,paramNode;
      //ATB
      devNode=NewTextChild(cuteNode,"ATB");
      paramNode=NewTextChild(devNode,"fmt_params");
      NewTextChild(paramNode,"pr_lat",0);
      NewTextChild(paramNode,"encoding","WINDOWS-1251");
      paramNode=NewTextChild(devNode,"mode_params");
      NewTextChild(paramNode,"multisession",(int)true,(int)false);
      NewTextChild(paramNode,"smode","S","S");
      NewTextChild(paramNode,"prn_type",90);
      //BTP
      devNode=NewTextChild(cuteNode,"BTP");
      paramNode=NewTextChild(devNode,"fmt_params");
      NewTextChild(paramNode,"pr_lat",0);
      NewTextChild(paramNode,"encoding","WINDOWS-1251");
      paramNode=NewTextChild(devNode,"mode_params");
      NewTextChild(paramNode,"multisession",(int)true,(int)false);
      NewTextChild(paramNode,"smode","S","S");
      NewTextChild(paramNode,"logonum","01","01");
      NewTextChild(paramNode,"prn_type",91);
      //DCP
      devNode=NewTextChild(cuteNode,"DCP");
      paramNode=NewTextChild(devNode,"fmt_params");
      NewTextChild(paramNode,"encoding","WINDOWS-1251");
      paramNode=NewTextChild(devNode,"mode_params");
      NewTextChild(paramNode,"multisession",(int)true,(int)true);
      //LSR
      devNode=NewTextChild(cuteNode,"LSR");
      paramNode=NewTextChild(devNode,"fmt_params");
      NewTextChild(paramNode,"prefix","31");
      NewTextChild(paramNode,"postfix","0D");
      paramNode=NewTextChild(devNode,"mode_params");
      NewTextChild(paramNode,"multisession",(int)true,(int)true);
    };
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

string& AirpTZRegion(string airp, bool with_exception)
{
  if (airp.empty()) throw Exception("Airport not specified");
  TAirpsRow& row=(TAirpsRow&)base_tables.get("airps").get_row("code",airp);
  return CityTZRegion(row.city,with_exception);
};

string& CityTZRegion(string city, bool with_exception)
{
  if (city.empty()) throw Exception("City not specified");
  TCitiesRow& row=(TCitiesRow&)base_tables.get("cities").get_row("code",city);
  if (row.region.empty() && with_exception)
    throw UserException("Для города %s не задан регион",city.c_str());
  return row.region;
};

string DeskCity(string desk, bool with_exception)
{
  if (desk.empty()) throw Exception("Desk not specified");
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
      throw UserException("Пульт %s не найден", desk.c_str());
    else
      return "";
  }
  return Qry.FieldAsString("city");
};

TDateTime UTCToLocal(TDateTime d, string region)
{
  if (region.empty()) throw Exception("Region not specified");
  tz_database &tz_db = get_tz_database();
  time_zone_ptr tz = tz_db.time_zone_from_region(region);
  if (tz==NULL) throw Exception("Region '%s' not found",region.c_str());
  local_date_time ld(DateTimeToBoost(d),tz);
  return BoostToDateTime(ld.local_time());
}

TDateTime LocalToUTC(TDateTime d, string region)
{
  if (region.empty()) throw Exception("Region not specified");
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
  switch (reqInfo->user.sets.time)
  {
    case ustTimeUTC:
      return d;
    case ustTimeLocalDesk:
      return UTCToLocal(d,reqInfo->desk.tz_region);
    case ustTimeLocalAirp:
      return UTCToLocal(d,region);
    default:
      throw Exception("Unknown sets.time for user %s (user_id=%d)",reqInfo->user.login.c_str(),reqInfo->user.user_id);
  };
};

TDateTime ClientToUTC(TDateTime d, string region)
{
  TReqInfo *reqInfo = TReqInfo::Instance();
  switch (reqInfo->user.sets.time)
  {
    case ustTimeUTC:
      return d;
    case ustTimeLocalDesk:
      return LocalToUTC(d,reqInfo->desk.tz_region);
    case ustTimeLocalAirp:
      return LocalToUTC(d,region);
    default:
      throw Exception("Unknown sets.time for user %s (user_id=%d)",reqInfo->user.login.c_str(),reqInfo->user.user_id);
  };
};

//форматы:
//  fmt=0 вн.код (рус. кодировка)
//  fmt=1 IATA код (лат. кодировка)
//  fmt=2 код ИКАО вн.
//  fmt=3 код ИKAO IATA
//  fmt=4 код ISO

inline void DoElemEConvertError( TElemContext ctxt,TElemType type, string code )
{
	string msg1, msg2;
	switch( ctxt ) {
		case ecDisp:
			msg1 = "ecDisp";
			break;
		case ecCkin:
			msg1 = "ecCkin";
			break;
		case ecTrfer:
			msg1 = "ecTrfer";
			break;
		case ecTlgTypeB:
			msg1 = "ecTlgTypeB";
			break;
	}
  switch( type ) {
  	case etCountry:
  		msg2 = "etCountry";
  		break;
  	case etCity:
  		msg2 = "etCity";
  		break;
  	case etAirline:
  		msg2 = "etAirline";
  		break;
  	case etAirp:
  		msg2 = "etAirp";
  		break;
  	case etCraft:
  		msg2 = "etCraft";
  		break;
  	case etClass:
  		msg2 = "etClass";
  		break;
  	case etSubcls:
  		msg2 = "etSubcls";
  		break;
  	case etPersType:
  		msg2 = "etPersType";
  		break;
  	case etGenderType:
  		msg2 = "etGenderType";
  		break;
  	case etPaxDocType:
  		msg2 = "etPaxDocType";
  		break;
  	case etPayType:
  		msg2 = "etPayType";
  		break;
  	case etCurrency:
  		msg2 = "etCurrency";
  		break;
    case etSuffix:
  		msg2 = "etSuffix";
  		break;
  }
  msg1 = string("Can't convert elem to id ") + msg1 + "," + msg2 + " ,values=" + code;
  throw EConvertError( msg1.c_str() );
}

string ElemCtxtToElemId(TElemContext ctxt,TElemType type, string code, int &fmt,
                        bool hard_verify, bool with_deleted)
{
  string id;
  fmt=-1;

  if (code.empty()) return id;

  id = ElemToElemId(type,code,fmt,with_deleted);

  //далее проверим а вообще имели ли мы право вводить в таком формате
  if ( hard_verify ) {
    if (ctxt==ecTlgTypeB && (type!=etCountry && fmt!=0 && fmt!=1 ||
                             type==etCountry && fmt!=0 && fmt!=1 && fmt!=4) ||
        ctxt==ecCkin && (fmt!=0) ||
        ctxt==ecTrfer && (fmt!=0))
    {
      //проблемы
      DoElemEConvertError( ctxt, type, code );
    };
  }
  if (ctxt==ecDisp)
  {
    if(type==etAirline ||
       type==etAirp ||
       type==etCraft ||
       type==etSuffix)
    {
      TReqInfo *reqInfo = TReqInfo::Instance();
      TUserSettingType user_fmt;
      if (type==etAirline) user_fmt=reqInfo->user.sets.disp_airline;
      if (type==etAirp) user_fmt=reqInfo->user.sets.disp_airp;
      if (type==etCraft) user_fmt=reqInfo->user.sets.disp_craft;
      if (type==etSuffix) user_fmt=reqInfo->user.sets.disp_suffix;
      if (type==etAirline ||
          type==etAirp ||
          type==etCraft)
      {
      	if ( hard_verify || fmt == -1 ) {
          if (!(user_fmt==ustCodeNative && fmt==0 ||
                user_fmt==ustCodeIATA && fmt==1 ||
                user_fmt==ustCodeNativeICAO && fmt==2 ||
                user_fmt==ustCodeIATAICAO && fmt==3 ||
                user_fmt==ustCodeMixed && (fmt==0||fmt==1||fmt==2||fmt==3)))
          {
            //проблемы
            DoElemEConvertError( ctxt, type, code );
          }
        }
        else {
          switch( user_fmt )  {
          	case ustCodeNative:
          		fmt = 0;
          		break;
          	case ustCodeIATA:
          		fmt = 1;
          		break;
          	case ustCodeNativeICAO:
          		fmt = 2;
          		break;
          	case ustCodeIATAICAO:
          		fmt = 3;
          		break;
          	default:;
          }
        }
      }
      else
      {
      	if ( hard_verify || fmt == -1 ) {
          if (!(user_fmt==ustEncNative && fmt==0 ||
                user_fmt==ustEncLatin && fmt==1 ||
                user_fmt==ustEncMixed && (fmt==0||fmt==1)))
          {
            //проблемы
            DoElemEConvertError( ctxt, type, code );
          }
        }
        else {
          switch( user_fmt )  {
          	case ustEncNative:
          		fmt = 0;
          		break;
          	case ustEncLatin:
          		fmt = 1;
          		break;
          	default:;
          }

        }
      };

    }
    else
    	if ( hard_verify || fmt == -1 ) {
        if (fmt!=0)
        {
          //проблемы
            DoElemEConvertError( ctxt, type, code );
        };
      }
      else {
      	fmt = 0;
      }
  };

  return id;
};

string ElemIdToElemCtxt(TElemContext ctxt,TElemType type, string id,
                         int fmt, bool with_deleted)
{
	int fmt2=0;
  if (ctxt==ecDisp)
  {
    if(type==etAirline ||
       type==etAirp ||
       type==etCraft ||
       type==etSuffix)
    {
      TReqInfo *reqInfo = TReqInfo::Instance();
      TUserSettingType user_fmt;
      if (type==etAirline) user_fmt=reqInfo->user.sets.disp_airline;
      if (type==etAirp) user_fmt=reqInfo->user.sets.disp_airp;
      if (type==etCraft) user_fmt=reqInfo->user.sets.disp_craft;
      if (type==etSuffix) user_fmt=reqInfo->user.sets.disp_suffix;
      if (type==etAirline ||
          type==etAirp ||
          type==etCraft)
      {
        switch(user_fmt)
        {
          case ustCodeNative:     fmt2=0; break;
          case ustCodeIATA:       fmt2=1; break;
          case ustCodeNativeICAO: fmt2=2; break;
          case ustCodeIATAICAO:   fmt2=3; break;
          case ustCodeMixed:      fmt2=fmt; break;
          default: ;
        };
      }
      else
      {
        switch(user_fmt)
        {
          case ustEncNative: fmt2=0; break;
          case ustEncLatin:  fmt2=1; break;
          case ustEncMixed:  fmt2=fmt; break;
          default: ;
        };
      };
    };
  };

  return ElemIdToElem(type,id,fmt2,with_deleted);
};

string ElemToElemId(TElemType type, string code, int &fmt, bool with_deleted)
{
  string id;
  fmt=-1;

  if (code.empty()) return id;

  char* table_name=NULL;
  switch(type)
  {
    case etCountry:
      table_name="countries";
      break;
    case etCity:
      table_name="cities";
      break;
    case etAirline:
      table_name="airlines";
      break;
    case etAirp:
      table_name="airps";
      break;
    case etCraft:
      table_name="crafts";
      break;
    case etClass:
      table_name="classes";
      break;
    case etSubcls:
      table_name="subcls";
      break;
    case etPersType:
      table_name="pers_types";
      break;
    case etGenderType:
      table_name="gender_types";
      break;
    case etPaxDocType:
      table_name="pax_doc_types";
      break;
    case etPayType:
      table_name="pay_types";
      break;
    case etCurrency:
      table_name="currency";
      break;
    default: ;
  };

  if (table_name!=NULL)
  {
    //это коды
    TBaseTable& BaseTable=base_tables.get(table_name);
    try
    {
      TCodeBaseTable& CodeBaseTable=dynamic_cast<TCodeBaseTable&>(BaseTable);
      //это code/code_lat
      try
      {
        id=((TCodeBaseTableRow&)CodeBaseTable.get_row("code",code,with_deleted)).code;
        fmt=0;
        return id;
      }
      catch (EBaseTableError) {};
      try
      {
        id=((TCodeBaseTableRow&)CodeBaseTable.get_row("code_lat",code,with_deleted)).code;
        fmt=1;
        return id;
      }
      catch (EBaseTableError) {};
    }
    catch (bad_cast) {};

    try
    {
      TICAOBaseTable& ICAOBaseTable=dynamic_cast<TICAOBaseTable&>(BaseTable);
      //это code_icao,code_icao_lat
      try
      {
        id=((TICAOBaseTableRow&)ICAOBaseTable.get_row("code_icao",code,with_deleted)).code;
        fmt=2;
        return id;
      }
      catch (EBaseTableError) {};
      try
      {
        id=((TICAOBaseTableRow&)ICAOBaseTable.get_row("code_icao_lat",code,with_deleted)).code;
        fmt=3;
        return id;
      }
      catch (EBaseTableError) {};
    }
    catch (bad_cast) {};

    try
    {
      TCountries& Countries=dynamic_cast<TCountries&>(BaseTable);
      //это code_iso
      try
      {
        id=((TCountriesRow&)Countries.get_row("code_iso",code,with_deleted)).code;
        fmt=4;
        return id;
      }
      catch (EBaseTableError) {};
    }
    catch (bad_cast) {};
  }
  else
  {
    //это просто данные
    switch(type)
    {
      case etSuffix:
        if (code.size()==1)
        {
          char *p;
          p=strchr(rus_suffix,*code.c_str());
          if (p!=NULL)
          {
            id=*p;
            fmt=0;
            return id;
          };
          p=strchr(lat_suffix,*code.c_str());
          if (p!=NULL)
          {
            id=rus_suffix[p-lat_suffix];
            fmt=1;
            return id;
          };
        };
        break;
      default: ;
    };
  };
  return id;
};

string ElemIdToElem(TElemType type, string id, int fmt, bool with_deleted)
{
	string code;
  code=id;

  if (id.empty()||fmt==0) return code;

  char* table_name=NULL;
  switch(type)
  {
    case etCountry:
      table_name="countries";
      break;
    case etCity:
      table_name="cities";
      break;
    case etAirline:
      table_name="airlines";
      break;
    case etAirp:
      table_name="airps";
      break;
    case etCraft:
      table_name="crafts";
      break;
    case etClass:
      table_name="classes";
      break;
    case etSubcls:
      table_name="subcls";
      break;
    case etPersType:
      table_name="pers_types";
      break;
    case etGenderType:
      table_name="gender_types";
      break;
    case etPaxDocType:
      table_name="pax_doc_types";
      break;
    case etPayType:
      table_name="pay_types";
      break;
    case etCurrency:
      table_name="currency";
      break;
    default: ;
  };

  if (table_name!=NULL)
  {
    //это коды
    try
    {
      TBaseTableRow& BaseTableRow=base_tables.get(table_name).get_row("code",id,with_deleted);

      try
      {
        TCodeBaseTableRow& row=dynamic_cast<TCodeBaseTableRow&>(BaseTableRow);
        if (fmt==0)
        {
          if (!row.code.empty()) code=row.code;
          return code;
        };
        if (fmt==1)
        {
          if (!row.code_lat.empty()) code=row.code_lat;
          return code;
        };

      }
      catch (bad_cast) {};
      try
      {
        TICAOBaseTableRow& row=dynamic_cast<TICAOBaseTableRow&>(BaseTableRow);
        if (fmt==2)
        {
          if (!row.code_icao.empty()) code=row.code_icao;
          return code;
        };
        if (fmt==3)
        {
          if (!row.code_icao_lat.empty()) code=row.code_icao_lat;
          return code;
        };

      }
      catch (bad_cast) {};
      try
      {
        TCountriesRow& row=dynamic_cast<TCountriesRow&>(BaseTableRow);
        if (fmt==4)
        {
          if (!row.code_iso.empty()) code=row.code_iso;
          return code;
        };
      }
      catch (bad_cast) {};
    }
    catch (EBaseTableError) {};
  }
  else
  {
    //это просто данные
    switch(type)
    {
      case etSuffix:
        if (id.size()==1)
        {
          char *p;
          p=strchr(rus_suffix,*code.c_str());
          if (p!=NULL)
          {
            if (fmt==0)
            {
              code=*p;
              return code;
            };
            if (fmt==1)
            {
              code=lat_suffix[p-rus_suffix];
              return code;
            };
          };
        };
        break;
      default: ;
    };
  };
  return code;
};


bool is_dst(TDateTime d, string region)
{
	if (region.empty()) throw Exception("Region not specified");
	ptime	utcd = DateTimeToBoost( d );
  tz_database &tz_db = get_tz_database();
  time_zone_ptr tz = tz_db.time_zone_from_region( region );
  if (tz==NULL) throw Exception("Region '%s' not found",region.c_str());
  local_date_time ld( utcd, tz ); /* определяем текущее время локальное */
  return ( tz->has_dst() && ld.is_dst() );
}

char ToLatSeatNo(char c)
{
  if ((unsigned char)c>=0x80)
  {
    ByteReplace(&c,1,rus_seat,lat_seat);
    if ((unsigned char)c>=0x80) c='?';
  };
  return c;
};

string convert_seat_no(const string &value, bool pr_lat)
{
  string result = value;
  if (pr_lat)
    transform(result.begin(), result.end(), result.begin(), ToLatSeatNo);
  return result;
};

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

char ToLatSuffix(char c)
{
  if ((unsigned char)c>=0x80)
  {
    ByteReplace(&c,1,rus_suffix,lat_suffix);
    if ((unsigned char)c>=0x80) c=0x00;
  };
  return c;
};

string convert_suffix(const string &value, bool pr_lat)
{
  string result = value;
  if (pr_lat)
    transform(result.begin(), result.end(), result.begin(), ToLatSuffix);
  return result;

};

string transliter(const string &value, bool pr_lat)
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
          case 'Ё': c2 = "IO"; break;
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
          case 'Щ': c2 = "SHCH"; break;
          case 'Ъ': c2 = ""; break;
          case 'Ы': c2 = "Y"; break;
          case 'Ь': c2 = ""; break;
          case 'Э': c2 = "E"; break;
          case 'Ю': c2 = "IU"; break;
          case 'Я': c2 = "IA"; break;
          default:  c2 = "?";
        };
        if (ToUpper(c)!=c) result+=lowerc(c2); else result+=c2;
      }
      else result+=c;
    };
  }
  else  result=value;
  return result;
}























