#ifndef _ASTRA_CONSTS_H_
#define _ASTRA_CONSTS_H_
/*
#ifdef __WIN32__
#define TClass TClasss
#endif*/

#include <limits.h>
#include <string>

namespace ASTRA
{

enum TOperMode { omCUSE, omCUTE, omMUSE, omSTAND, omTypeNum };
extern const char* OperModeS[omTypeNum];

enum TEventType {evtSeason,evtDisp,evtFlt,evtGraph,evtPax,evtPay,evtComp,evtTlg,
                 evtAccess,evtSystem,evtCodif,evtPeriod,evtProgError,evtUnknown,evtTypeNum};
extern const char* EventTypeS[evtTypeNum];

typedef enum {dtBP, dtBT, dtReceipt, dtFltDoc, dtArchive, dtDisp, dtTlg, dtUnknown} TDocType;
extern const char* TDocTypeS[8];

typedef enum {F,C,Y,NoClass} TClass;
extern const char* TClassS[4];

typedef enum {adult,child,baby,NoPerson} TPerson;
extern const char* TPersonS[4];

typedef enum {DoubleTr,DoubleOk,ChangeCl,WL,GoShow,NoQueue} TQueue;
extern const int TQueueS[6];

typedef enum { psCheckin, psTCheckin, psTransit, psGoshow } TPaxStatus;
extern const char* TPaxStatusS[4];

const int NoExists = INT_MIN;
const std::string NoDays = ".......";
const std::string AllDays = "1234567";

enum tmodify { fnochange, fchange, finsert, fdelete };

const char rus_pnr[]="0123456789БВГДКЛМНПРСТФХЦЖШ";
const char lat_pnr[]="0123456789BVGDKLMNPRSTFXCZW";

const char rus_suffix[]="АВСДЕФЖХИЙКЛМНОПЯРСТУЮШЬЫЗ";
const char lat_suffix[]="ABCDEFGHIJKLMNOPQRSTUVWXYZ";

const char rus_seat[]="АБВГДЕЖЗИКЛМНОПРСТУФХЦЧШЩ";
const char lat_seat[]="ABCDEFGHJKLMNOPQRSTUVWXYZ";

enum TCompLayerType { cltBlockCent, cltTranzit, cltCheckin, cltTCheckin, cltGoShow, cltBlockTrzt, cltSOMTrzt, cltPRLTrzt,
	                    cltProtTrzt, cltPNLCkin, cltProtCkin, cltProtect, cltUncomfort, cltSmoke, cltUnknown, cltTypeNum };
extern const char* CompLayerTypeS[cltTypeNum];

const int arx_trip_date_range=5;  //изменять можно только в большую сторону из-за уже проведенной архивации

#define UNKNOWN_VERSION "UNKNOWN"

};

#endif

