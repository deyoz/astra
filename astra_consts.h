#ifndef _ASTRA_CONSTS_H_
#define _ASTRA_CONSTS_H_
/*
#ifdef __WIN32__
#define TClass TClasss
#endif*/

namespace ASTRA
{

enum TEventType {evtSeason,evtDisp,evtFlt,evtGraph,evtPax,evtPay,evtComp,evtTlg,
                 evtAccess,evtSystem,evtCodif,evtPeriod,evtProgError,evtUnknown,evtTypeNum};

extern const char* EventTypeS[evtTypeNum];

typedef enum {F,C,Y,NoClass} TClass;
extern const char* TClassS[4];

typedef enum {adult,child,baby,NoPerson} TPerson;
extern const char* TPersonS[4];

typedef enum {DoubleTr,DoubleOk,ChangeCl,WL,GoShow,NoQueue} TQueue;
extern const int TQueueS[6];

extern const char* TStatusS[3];

};

#endif

