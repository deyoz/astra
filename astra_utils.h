#ifndef _ASTRA_UTILS_H_
#define _ASTRA_UTILS_H_

#include <string>

enum TEventType {evtSeason,evtDisp,evtFlt,evtGraph,evtPax,evtPay,evtComp,evtTlg,
                 evtAccess,evtSystem,evtCodif,evtPeriod,evtProgError,evtUnknown,evtTypeNum};
                 
struct TLogMsg { 
  std::string msg;
  TEventType ev_type;
  int id1,id2,id3;
  TLogMsg() {
    msg = "";  	
    id1 = 0;
    id2 = 0;
    id3 = 0;
  }
};
  
TEventType DecodeEventType( const std::string ev_type );
std::string EncodeEventType( const TEventType ev_type );

#endif /*_ASTRA_UTILS_H_*/

