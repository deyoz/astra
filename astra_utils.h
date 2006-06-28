#ifndef _ASTRA_UTILS_H_
#define _ASTRA_UTILS_H_

#include <string>
#include "astra_consts.h"
#include "basic.h"

ASTRA::TClass DecodeClass(char* s);
char* EncodeClass(ASTRA::TClass cl);
ASTRA::TPerson DecodePerson(char* s);
char* EncodePerson(ASTRA::TPerson p);
ASTRA::TQueue DecodeQueue(int q);
int EncodeQueue(ASTRA::TQueue q);
char DecodeStatus(char* s);
#define sign( x ) ( ( x ) > 0 ? 1 : ( x ) < 0 ? -1 : 0 )
BASIC::TDateTime DecodeTimeFromSignedWord( signed short int Value );
signed short int EncodeTimeToSignedWord( BASIC::TDateTime Value );
char *EncodeSeatNo( char *Value, bool pr_latseat );
char *DecodeSeatNo( char *Value );
void SendTlg(const char* receiver, const char* sender, const char *format, ...);

struct TLogMsg {
  std::string msg;
  ASTRA::TEventType ev_type;
  int id1,id2,id3;
  TLogMsg() {
    msg = "";
    id1 = 0;
    id2 = 0;
    id3 = 0;
  }
};

ASTRA::TEventType DecodeEventType( const std::string ev_type );
std::string EncodeEventType( const ASTRA::TEventType ev_type );

#endif /*_ASTRA_UTILS_H_*/
