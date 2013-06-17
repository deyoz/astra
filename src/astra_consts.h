#ifndef _ASTRA_CONSTS_H_
#define _ASTRA_CONSTS_H_


#include <limits.h>
#include <string>

namespace ASTRA
{

enum TTlgOption {
    toLDM_cabin_weight,
    toNum
};
extern const char* TlgOptionS[toNum];

enum TClientType { ctTerm, ctWeb, ctKiosk, ctTypeNum };
extern const char* ClientTypeS[ctTypeNum];

enum TOperMode { omCUSE, omCUTE, omMUSE, omRESA, omSTAND, omTypeNum };
extern const char* OperModeS[omTypeNum];

enum TEventType {evtSeason,evtDisp,evtFlt,evtGraph,evtPax,evtPay,evtComp,evtTlg,
                 evtAccess,evtSystem,evtCodif,evtPeriod,evtProgError,evtUnknown,evtTypeNum};
extern const char* EventTypeS[evtTypeNum];

typedef enum {F,C,Y,NoClass} TClass;
extern const char* TClassS[4];

typedef enum {adult,child,baby,NoPerson} TPerson;
extern const char* TPersonS[4];

typedef enum {DoubleTr,DoubleOk,ChangeCl,WL,GoShow,NoQueue} TQueue;
extern const int TQueueS[6];

typedef enum { psCheckin, psTCheckin, psTransit, psGoshow } TPaxStatus;
extern const char* TPaxStatusS[4];

const std::string refuseAgentError="�";

const int NoExists = INT_MIN;
const std::string NoDays = ".......";
const std::string AllDays = "1234567";

enum tmodify { fnochange, fchange, finsert, fdelete };

const char rus_pnr[]="0123456789�����������������";
const char lat_pnr[]="0123456789BVGDKLMNPRSTFXCZW";

const char rus_seat[]="�������������������������";
const char lat_seat[]="ABCDEFGHJKLMNOPQRSTUVWXYZ";

enum TCompLayerType { cltBlockCent, cltTranzit, cltCheckin, cltTCheckin, cltGoShow, cltBlockTrzt, cltSOMTrzt, cltPRLTrzt,
                      cltProtBeforePay, cltProtAfterPay, cltPNLBeforePay, cltPNLAfterPay,
	                    cltProtTrzt, cltPNLCkin, cltProtCkin, cltProtect, cltUncomfort, cltSmoke, cltDisable, cltUnknown, cltTypeNum };
extern const char* CompLayerTypeS[cltTypeNum];

enum TBagNormType { bntFree, bntOrdinary, bntPaid,
                    bntFreeExcess, bntFreeOrdinary, bntFreePaid, bntOrdinaryPaid,
                    bntUnknown, bntTypeNum };

extern const char* BagNormTypeS[bntTypeNum];

enum TRptType {
    rtPTM,
    rtPTMTXT,
    rtBTM,
    rtBTMTXT,
    rtWEB,
    rtWEBTXT,
    rtREFUSE,
    rtREFUSETXT,
    rtNOTPRES,
    rtNOTPRESTXT,
    rtREM,
    rtREMTXT,
    rtCRS,
    rtCRSTXT,
    rtCRSUNREG,
    rtCRSUNREGTXT,
    rtEXAM,
    rtEXAMTXT,
    rtNOREC,
    rtNORECTXT,
    rtGOSHO,
    rtGOSHOTXT,
    rtBDOCS,
    rtSPEC,
    rtSPECTXT,
    rtUnknown,
    rtTypeNum
};
extern const char *RptTypeS[rtTypeNum];

#define TRACE_SIGNATURE int Level, const char *nickname, const char *filename, int line
#define TRACE_PARAMS Level, nickname, filename, line

};

#endif

