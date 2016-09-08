#ifndef _ASTRA_CONSTS_H_
#define _ASTRA_CONSTS_H_


#include <limits.h>
#include <string>
#include <list>
#include <boost/optional.hpp>
#include "exceptions.h"

namespace ASTRA
{

template <typename T1, typename T2>
class PairList
{
  private:
    std::multimap<T1, T2> map1;
    std::multimap<T2, T1> map2;
    boost::optional<T1> unknown1;
    boost::optional<T2> unknown2;
    virtual std::string className() const=0;
    template <typename FROM, typename TO>
    TO convert(const FROM& value,
               const std::multimap<FROM, TO>& map,
               const boost::optional<TO>& unknown,
               const std::string& where) const
    {
      typename std::multimap<FROM, TO>::const_iterator i=map.find(value);
      if (i==map.end())
      {
        //не нашли
        if (unknown) return unknown.get();
        std::ostringstream s;
        s << className() << "." << where << ": " << value << " not found";  //не решена проблема с возможным рекурсивным вызовом!!!
        throw EXCEPTIONS::EConvertError(s.str().c_str());
      }
      else
      {
        //нашли
        TO result=i->second;
        ++i;
        if (i==map.end() || i->second!=result) return result;
        std::ostringstream s;
        s << className() << "." << where << ": " << value << " duplicated";  //не решена проблема с возможным рекурсивным вызовом!!!
        throw EXCEPTIONS::EConvertError(s.str().c_str());
      }
    }
  public:
    PairList(const std::list< std::pair<T1, T2> > &pairs,
             const boost::optional<T1>& unk1,
             const boost::optional<T2>& unk2)
    {
      for(typename std::list< std::pair<T1, T2> >::const_iterator i=pairs.begin(); i!=pairs.end(); ++i)
      {
        map1.insert(make_pair(i->first, i->second));
        map2.insert(make_pair(i->second, i->first));
      }
      unknown1=unk1;
      unknown2=unk2;
    }
    virtual ~PairList() {}


    T1 decode(const T2& value) const
    {
      return convert<T2, T1>(value, map2, unknown1, __FUNCTION__);
    }
    T2 encode(const T1& value) const
    {
      return convert<T1, T2>(value, map1, unknown2, __FUNCTION__);
    }
};

enum TClientType { ctTerm, ctWeb, ctKiosk, ctPNL, ctHTTP, ctMobile, ctTypeNum };
extern const char* ClientTypeS[ctTypeNum];

enum TOperMode { omCUSE, omCUTE, omMUSE, omRESA, omSTAND, omTypeNum };
extern const char* OperModeS[omTypeNum];

enum TEventType {evtSeason,evtDisp,evtFlt,evtGraph,evtFltTask,evtPax,evtPay,evtComp,evtTlg,
                 evtAccess,evtSystem,evtCodif,evtPeriod,evtPrn,evtProgError,evtUnknown,evtTypeNum};
extern const char* EventTypeS[evtTypeNum];

typedef enum {F,C,Y,NoClass} TClass;
extern const char* TClassS[4];

typedef enum {adult,child,baby,NoPerson} TPerson;
extern const char* TPersonS[4];

typedef enum {DoubleTr,DoubleOk,ChangeCl,WL,GoShow,NoQueue} TQueue;
extern const int TQueueS[6];

typedef enum { psCheckin, psTCheckin, psTransit, psGoshow, psCrew } TPaxStatus;
extern const char* TPaxStatusS[5];

const std::string refuseAgentError="А";

const int NoExists = INT_MIN;
const std::string NoDays = ".......";
const std::string AllDays = "1234567";

enum tmodify { fnochange, fchange, finsert, fdelete };

const char rus_pnr[]="0123456789БВГДКЛМНПРСТФХЦЖШ";
const char lat_pnr[]="0123456789BVGDKLMNPRSTFXCZW";

const char rus_seat[]="АБВГДЕЖЗИКЛМНОПРСТУФХЦЧШЩ";
const char lat_seat[]="ABCDEFGHJKLMNOPQRSTUVWXYZ";

enum TCompLayerType { cltBlockCent, cltTranzit, cltCheckin, cltTCheckin, cltGoShow, cltBlockTrzt, cltSOMTrzt, cltPRLTrzt,
                      cltProtBeforePay, cltProtAfterPay, cltPNLBeforePay, cltPNLAfterPay,
	                    cltProtTrzt, cltPNLCkin, cltProtCkin, cltProtect, cltUncomfort, cltSmoke, cltDisable, cltUnknown, cltTypeNum };
extern const char* CompLayerTypeS[cltTypeNum];

enum TBagNormType { bntFree, bntOrdinary, bntPaid,
                    bntFreeExcess, bntFreeOrdinary, bntFreePaid, bntOrdinaryPaid,
                    bntUnknown, bntTypeNum };

enum TRcptServiceType { rstExcess=1, rstPaid=2, rstDeclaredValue=3 };

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
    rtEMD,
    rtEMDTXT,
    rtLOADSHEET,
    rtNOTOC,
    rtLIR,
    rtUnknown,
    rtTypeNum
};
extern const char *RptTypeS[rtTypeNum];

class TCrewType
{
  public:
    enum Enum
    {
      Crew1,
      Crew2,
      Crew3,
      Crew4,
      Crew5,
      ExtraCrew,
      DeadHeadCrew,
      MiscOperStaff,
      Unknown
    };

    static const std::list< std::pair<Enum, std::string> >& pairs()
    {
      static std::list< std::pair<Enum, std::string> > l;
      if (l.empty())
      {
        l.push_back(std::make_pair(Crew1,         "CR1"));
        l.push_back(std::make_pair(Crew2,         "CR2"));
        l.push_back(std::make_pair(Crew3,         "CR3"));
        l.push_back(std::make_pair(Crew4,         "CR4"));
        l.push_back(std::make_pair(Crew5,         "CR5"));
        l.push_back(std::make_pair(ExtraCrew,     "XCR"));
        l.push_back(std::make_pair(DeadHeadCrew,  "DHC"));
        l.push_back(std::make_pair(MiscOperStaff, "MOS"));
        l.push_back(std::make_pair(Unknown,       ""));
      }
      return l;
    }
};

class TCrewTypes : public ASTRA::PairList<TCrewType::Enum, std::string>
{
  private:
    virtual std::string className() const { return "TCrewTypes"; }
  public:
    TCrewTypes() : ASTRA::PairList<TCrewType::Enum, std::string>(TCrewType::pairs(),
                                                                 TCrewType::Unknown,
                                                                 boost::none) {}
};

#define TRACE_SIGNATURE int Level, const char *nickname, const char *filename, int line
#define TRACE_PARAMS Level, nickname, filename, line
#define ERROR_PARAMS -1, nickname, filename, line

enum TIdType {idFlt, idGrp, idPax};

};

const ASTRA::TCrewTypes& CrewTypes();

const std::string TIMEOUT_OCCURRED = "Timeout occurred";
const std::string ACCESS_DENIED = "Access denied";

// TODO: get rid of this outrageous define
#define OP_TYPE_COND(COL) "nvl(" COL", :op_type) = :op_type" // c++11 standard requires space before identifier: ..." COL"...

#endif

