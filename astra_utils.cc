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
#include "xml_unit.h"

using namespace std;
using namespace ASTRA;
using namespace BASIC;
using namespace EXCEPTIONS;
using namespace JxtContext;

TReqInfo::TReqInfo()
{
  screen_id = 0;
};

TReqInfo *TReqInfo::Instance()
{
  static TReqInfo *instance_ = 0;
  if ( !instance_ )
    instance_ = new TReqInfo();
  return instance_;
};

void TReqInfo::Clear()
{
    desk.code.clear();
    desk.city.clear();
    desk.airp.clear();
    desk.time = 0;
    user.login.clear();
    user.descr.clear();
    user.access_code = 0;    	
    user.access.clearFlags();
    user.user_id=-1;
};

void TReqInfo::Initialize( const std::string &vscreen, const std::string &vpult, 
                           const std::string &vopr, bool checkBasicInfo )
{
  Clear();
  TQuery &Qry = *OraSession.CreateQuery();
  ProgTrace( TRACE5, "screen=%s, pult=|%s|, opr=|%s|", vscreen.c_str(), vpult.c_str(), vopr.c_str() );
  screen = vscreen;	
  desk.code = vpult;        
  try {
    Qry.Clear();
    Qry.SQLText = "SELECT id FROM screen WHERE exe = UPPER(:screen)";
    Qry.DeclareVariable( "screen", otString );
    Qry.SetVariable( "screen", vscreen );
    Qry.Execute();
    if ( Qry.RowCount() == 0 )    
      throw Exception( (string)"Unknown screen " + vscreen );  
    screen_id = Qry.FieldAsInteger( "id" );
        
    Qry.Clear();
    Qry.SQLText = "SELECT pr_denial, city, airp FROM desks, sale_points "\
                  " WHERE desks.code = UPPER(:pult) AND desks.point = sale_points.code ";
    Qry.DeclareVariable( "pult", otString );
    Qry.SetVariable( "pult", vpult );
    Qry.Execute();
    if ( Qry.RowCount() == 0 )
      throw UserException( "Пульт не зарегистрирован в системе. Обратитесь к администратору." );         	
    if ( Qry.FieldAsInteger( "pr_denial" ) != 0 )
      throw UserException( "Пульт отключен" );         	
    desk.city = Qry.FieldAsString( "city" );
    desk.airp = Qry.FieldAsString( "airp" );
    Qry.Clear();
    Qry.SQLText = "SELECT SYSDATE+tz/24 as time FROM cities WHERE cod=:city";
    Qry.DeclareVariable( "city", otString );
    Qry.SetVariable( "city", desk.city );
    Qry.Execute();    
    desk.time = Qry.FieldAsDateTime( "time" );
    
    Qry.Clear();
    Qry.SQLText = "SELECT user_id, login, descr, pr_denial FROM astra.users2 "\
                  " WHERE desk = UPPER(:pult) ";
    Qry.DeclareVariable( "pult", otString );
    Qry.SetVariable( "pult", vpult );
    Qry.Execute();
    
    if ( Qry.RowCount() == 0 )
    {      
      if (checkBasicInfo)
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
    Qry.SQLText = "SELECT 1 AS priority,access_code FROM astra.user_perms "\
                  " WHERE user_perms.screen_id=:screen_id AND user_perms.user_id=:user_id "\
                  " UNION "\
                  "SELECT 2,MAX(access_code) FROM astra.user_roles,astra.role_perms "\
                  " WHERE user_roles.role_id=role_perms.role_id AND "\
                  "       role_perms.screen_id=:screen_id AND "\
                  "       user_roles.user_id=:user_id "\
                  "ORDER BY priority";
                
    Qry.DeclareVariable( "user_id",otInteger );
    Qry.DeclareVariable( "screen_id", otString );
    Qry.SetVariable( "user_id", user.user_id );
    Qry.SetVariable( "screen_id", screen_id );
    Qry.Execute();
    if ( Qry.RowCount() > 0 )
      user.access_code = Qry.FieldAsInteger( "access_code" );
    else
      user.access_code = 0;
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


void SendTlgType(const char* receiver,
                 const char* sender,
                 bool isEdi,
                 int ttl,
                 const std::string &text)
{
    try
    {
        TQuery Qry(&OraSession);
        Qry.SQLText=
                "INSERT INTO "
                "tlg_queue(id,sender,tlg_num,receiver,type,status,time,ttl) "
                "VALUES"
                "(tlgs_id.nextval,:sender,tlgs_id.nextval,:receiver,"
                ":type,'PUT',SYSDATE,:ttl)";
        Qry.CreateVariable("sender",otString,sender);
        Qry.CreateVariable("receiver",otString,receiver);
        Qry.CreateVariable("type",otString,isEdi?"OUTA":"OUTB");
        if (isEdi&&ttl>0)
          Qry.CreateVariable("ttl",otInteger,ttl);
        else
          Qry.CreateVariable("ttl",otInteger,FNull);
        Qry.Execute();
        Qry.SQLText=
                "INSERT INTO "
                "tlgs(id,sender,tlg_num,receiver,type,status,time,tlg_text,error) "
                "VALUES"
                "(tlgs_id.currval,:sender,tlgs_id.currval,:receiver,"
                ":type,'PUT',SYSDATE,:text,NULL)";
        Qry.DeclareVariable("text",otLong);
        Qry.SetLongVariable("text",(void *)text.c_str(),text.size());
        Qry.DeleteVariable("ttl");
        Qry.Execute();
        Qry.Close();
    }
    catch( std::exception &e)
    {
        ProgError(STDLOG, e.what());
    }
    catch(...)
    {
        ProgError(STDLOG, "SendTlgType: Unknown error while trying to send tlg");
        throw;
    };
}
void SendTlg(const char* receiver, const char* sender, const char *format, ...)
{
  char Message[500];
  if (receiver==NULL||sender==NULL||format==NULL) return;
  va_list ap;
  va_start(ap, format);
  sprintf(Message,"Sender: %s\n",sender);
  int len=strlen(Message);
  vsnprintf(Message+len, sizeof(Message)-len, format, ap);
  Message[sizeof(Message)-1]=0;
  va_end(ap);
  try
  {
    static TQuery Qry(&OraSession);
    if (Qry.SQLText.IsEmpty())
    {
      Qry.SQLText=
        "INSERT INTO tlgs(id,sender,tlg_num,receiver,type,status,time,tlg_text)\
         VALUES(tlgs_id.nextval,:sender,tlgs_id.nextval,:receiver,'OUT','PUT',SYSDATE,:text)";
      Qry.DeclareVariable("sender",otString);
      Qry.DeclareVariable("receiver",otString);
      Qry.DeclareVariable("text",otLong);
    };
    Qry.SetVariable("sender",sender);
    Qry.SetVariable("receiver",receiver);
    Qry.SetLongVariable("text",Message,strlen(Message));
    Qry.Execute();
    Qry.Close();
  }
  catch(...) {};
};

void showMessage( xmlNodePtr resNode, const std::string &message )
{
  xmlNodePtr node = NewTextChild( resNode, "command" );
  NewTextChild( node, "message", message );
}

/***************************************************************************************/
void SysReqInterface::ErrorToLog(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    ProgError( STDLOG, "!!! Ошибка клиента: msg=%s, id1=%d, id2=%d, id3=%d", 
               NodeAsString( "msg", reqNode ), NodeAsInteger( "id1", reqNode ), 
               NodeAsInteger( "id2", reqNode ),
               NodeAsInteger( "id3", reqNode ) ) ;
}

void SysReqInterface::Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
};
/***************************************************************************************/

void SysReqInterface::CheckBasicInfo(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    tst();
    TReqInfo *reqinfo = TReqInfo::Instance();
    if(reqinfo->user.user_id<0)
        return;

    TQuery *Qry = OraSession.CreateQuery();
    try {
        Qry->SQLText =
            "SELECT 1 AS priority,MAX(access_code) AS access_code, "
            "       screen.id,screen.name,screen.exe "
            "FROM user_perms,screen "
            "WHERE user_perms.screen_id=screen.id AND "
            "      user_perms.user_id=:user_id "
            "GROUP BY screen.id,screen.name,screen.exe "
            "UNION "
            "SELECT 2,MAX(access_code), "
            "       screen.id,screen.name,screen.exe "
            "FROM user_roles,role_perms,screen "
            "WHERE user_roles.role_id=role_perms.role_id AND "
            "      role_perms.screen_id=screen.id AND "
            "      user_roles.user_id=:user_id "
            "GROUP BY screen.id,screen.name,screen.exe "
            "ORDER BY id,priority ";
        Qry->DeclareVariable("user_id", otInteger);
        Qry->SetVariable("user_id", reqinfo->user.user_id);
        Qry->Execute();
        xmlNodePtr modulesNode = NewTextChild(resNode, "modules");
        int screen_id = -1;
        bool modules_exists = false;
        while(!Qry->Eof) {
            if(screen_id != Qry->FieldAsInteger("id")) {
                if(Qry->FieldAsInteger("access_code") > 0) {
                    modules_exists = true;
                    xmlNodePtr moduleNode = NewTextChild(modulesNode, "module");
                    NewTextChild(moduleNode, "id", Qry->FieldAsInteger("id"));
                    NewTextChild(moduleNode, "name", Qry->FieldAsString("name"));
                    NewTextChild(moduleNode, "exe", Qry->FieldAsString("exe"));
                }
                screen_id = Qry->FieldAsInteger("id");
            }
            Qry->Next();
        }
        if(!modules_exists) throw UserException("Пользователю закрыт доступ ко всем модулям");
    } catch(...) {
        OraSession.DeleteQuery(*Qry);
        throw;
    }
    OraSession.DeleteQuery(*Qry);
}
