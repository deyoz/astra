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

const string ETS_CANON_NAME()
{
  static string ETSNAME;	
  if ( ETSNAME.empty() ) {
    char r[100];
    r[0]=0;
    if ( get_param( "ETS_CANON_NAME", r, sizeof( r ) ) < 0 )
      throw Exception( "Can't read param ETS_CANON_NAME" );
    ETSNAME = r;
    ProgTrace( TRACE5, "ETS_CANON_NAME=%s", ETSNAME.c_str() );  	
  }  
  return ETSNAME;	
}

const string OWN_CANON_NAME()
{
  static string OWNNAME;	
  if ( OWNNAME.empty() ) {
    char r[100];
    r[0]=0;
    if ( get_param( "OWN_CANON_NAME", r, sizeof( r ) ) < 0 )
      throw Exception( "Can't read param OWN_CANON_NAME" );
    OWNNAME = r;
    ProgTrace( TRACE5, "OWN_CANON_NAME=%s", OWNNAME.c_str() );  	
  }  
  return OWNNAME;	
}

TDesk::TDesk()
{
  time = 0;	
};

void TDesk::clear()
{
  code.clear();
  city.clear();
  airp.clear();
  time = 0;
};

TUser::TUser()
{
  access_code = 0;
  user_id = -1;	
};

void TUser::clear()
{
  login.clear();
  descr.clear();
  access_code = 0;    	
  access.clearFlags();
  user_id=-1;
};	

TReqInfo::TReqInfo()
{
  screen_id = 0;
};

void TReqInfo::clear()
{
  screen_id = 0;
  desk.clear();
  user.clear();  
  
  opt.airport.clear();
  opt.airport_lat.clear();
  opt.city.clear();
  opt.airport_name.clear();
  opt.city_name.clear();  
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
  TQuery &Qry = *OraSession.CreateQuery();
  ProgTrace( TRACE5, "screen=%s, pult=|%s|, opr=|%s|", vscreen.c_str(), vpult.c_str(), vopr.c_str() );
  screen = upperc( vscreen );	
  desk.code = vpult;        
  string sql;
  try {
    Qry.Clear();
    sql = string( "SELECT id FROM " ) + COMMON_ORAUSER() + ".screen WHERE exe = :screen";
    Qry.SQLText = sql;
    Qry.DeclareVariable( "screen", otString );
    Qry.SetVariable( "screen", screen );
    Qry.Execute();
    tst();
    if ( Qry.RowCount() == 0 )    
      throw Exception( (string)"Unknown screen " + screen );  
    screen_id = Qry.FieldAsInteger( "id" );
        
    Qry.Clear();
    sql = string("SELECT pr_denial, city, airp FROM ") +COMMON_ORAUSER()+ ".desks," +
          COMMON_ORAUSER() + ".sale_points " + 
          " WHERE desks.code = UPPER(:pult) AND desks.point = sale_points.code ";
          
    Qry.SQLText = sql;
    Qry.DeclareVariable( "pult", otString );
    Qry.SetVariable( "pult", vpult );
    Qry.Execute();
    tst();
    if ( Qry.RowCount() == 0 )
      throw UserException( "Пульт не зарегистрирован в системе. Обратитесь к администратору." );         	
    if ( Qry.FieldAsInteger( "pr_denial" ) != 0 )
      throw UserException( "Пульт отключен" );         	
    desk.city = Qry.FieldAsString( "city" );
    desk.airp = Qry.FieldAsString( "airp" );
    Qry.Clear();
    sql = string("SELECT SYSDATE+tz/24 as time FROM ") + COMMON_ORAUSER() + 
          ".cities WHERE cod=:city";
    Qry.SQLText = sql;
    Qry.DeclareVariable( "city", otString );
    Qry.SetVariable( "city", desk.city );
    Qry.Execute();    
    tst();
    desk.time = Qry.FieldAsDateTime( "time" );
    
    Qry.Clear();
    sql = "SELECT user_id, login, descr, pr_denial FROM " + COMMON_ORAUSER() + ".users2 "+   
          " WHERE desk = UPPER(:pult) ";
    Qry.SQLText = sql;
    Qry.DeclareVariable( "pult", otString );
    Qry.SetVariable( "pult", vpult );
    Qry.Execute();
    tst();
    
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
    user.login = Qry.FieldAsString( "login" );    
    
    Qry.Clear();
    sql = "SELECT 1 AS priority,access_code FROM " + COMMON_ORAUSER() + ".user_perms " +
          " WHERE user_perms.screen_id=:screen_id AND user_perms.user_id=:user_id " +
          " UNION " +
          "SELECT 2,MAX(access_code) FROM " + COMMON_ORAUSER() + ".user_roles,"+
          COMMON_ORAUSER() + ".role_perms "+
          " WHERE user_roles.role_id=role_perms.role_id AND "+
          "       role_perms.screen_id=:screen_id AND "+
          "       user_roles.user_id=:user_id "+
          "ORDER BY priority";
    
    Qry.SQLText = sql;
                
    Qry.DeclareVariable( "user_id",otInteger );
    Qry.DeclareVariable( "screen_id", otString );
    Qry.SetVariable( "user_id", user.user_id );
    Qry.SetVariable( "screen_id", screen_id );
    Qry.Execute();
    if ( Qry.RowCount() > 0 )
      user.access_code = Qry.FieldAsInteger( "access_code" );
    else
      user.access_code = 0;
    Qry.Clear();
    Qry.SQLText = "SELECT airps.cod AS air_cod,airps.lat AS air_cod_lat,airps.name AS air_name, "\
                  "       cities.cod AS city_cod,cities.name AS city_name,SYSDATE "\
                  "FROM options,airps,cities "\
                  "WHERE options.cod=airps.cod AND airps.city=cities.cod";
    Qry.Execute();
    if ( Qry.RowCount() ) {
      opt.airport = Qry.FieldAsString( "AIR_COD" );
      opt.airport_lat = Qry.FieldAsString( "AIR_COD_LAT" );
      opt.airport_name = Qry.FieldAsString( "AIR_NAME" );
      opt.city = Qry.FieldAsString( "CITY_COD" );
      opt.city_name = Qry.FieldAsString( "CITY_NAME" );
    }          
  }
  catch( ... ) {
    OraSession.DeleteQuery( Qry );
    throw;
  };
  OraSession.DeleteQuery( Qry );
  user.setAccessPair();
}

void TUser::setAccessPair()
{
  access.clearFlags();
  switch( access_code ) {
    case 0: break;
    case 1:
    case 2:
    case 3:
    case 4: access.setFlag( amRead );
            break;
    case 5:
    case 6: access.setFlag( amRead );
            access.setFlag( amPartialWrite );
            break;
    default:access.setFlag( amRead );
            access.setFlag( amPartialWrite );
            access.setFlag( amWrite );
  }
}


void TUser::check_access( TAccessMode mode )
{
  if ( !access.isFlag( mode ) )
    throw UserException( "Недостаточно прав. Доступ к информации невозможен" );
}

bool TUser::getAccessMode( TAccessMode mode )
{
  return access.isFlag( mode );
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
        "VALUES(:type,SYSDATE,events__seq.nextval,SUBSTR(:msg,1,250),:screen,:ev_user,:station,:id1,:id2,:id3) ";
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
    Qry->SetVariable("screen", screen);
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
    NewTextChild(node, "access_code",reqInfo->user.access_code);
    NewTextChild(node, "login",reqInfo->user.login);
  };    
  if (!reqInfo->desk.code.empty())
  {
    node = NewTextChild(resNode,"desk");
    NewTextChild(node,"airp",reqInfo->desk.airp);
    NewTextChild(node,"city",reqInfo->desk.city);
    NewTextChild(node,"time",DateTimeToStr( reqInfo->desk.time ) ); 
  };  
  node = NewTextChild( resNode, "opt" );
  NewTextChild( node, "airport", reqInfo->opt.airport );
  NewTextChild( node,"airport_lat", reqInfo->opt.airport_lat );
  NewTextChild( node,"city", reqInfo->opt.city );
  NewTextChild( node,"airport_name", reqInfo->opt.airport_name );
  NewTextChild( node,"city_name", reqInfo->opt.city_name );  
};

/***************************************************************************************/
void SysReqInterface::ErrorToLog(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{   
    ProgError( STDLOG, "Модуль: %s. Пульт: %s. Оператор: %s. Время: %s. Ошибка: %s.",
               NodeAsString("/term/query/@screen", ctxt->reqDoc),
               ctxt->pult.c_str(), ctxt->opr.c_str(),
               DateTimeToStr(Now(),"dd.mm.yyyy hh:nn:ss").c_str(),
               NodeAsString( "msg", reqNode ) ) ;
}


