#include "setup.h"
#include "astra_utils.h"
#include "astra_consts.h"

#include <stdarg.h>
#include "basic.h"
#include "oralib.h"

#include <string.h>

using namespace std;
using namespace ASTRA;
using namespace BASIC;

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
    return CharReplace( Value, "ABCDEFGHIJK", "€‚ƒ„…†‡ˆŠ‹" );
  else return Value;
};

char *DecodeSeatNo( char *Value )
{
  return CharReplace( Value, "€‚ƒ„…†‡ˆŠ‹", "ABCDEFGHIJK" );
};

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


