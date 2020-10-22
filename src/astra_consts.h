#ifndef _ASTRA_CONSTS_H_
#define _ASTRA_CONSTS_H_


#include <limits.h>
#include <string>
#include <list>
#include <map>
#include <boost/optional.hpp>
#include "exceptions.h"

#define APIS_TEST 0

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
        //�� ��諨
        if (unknown) return unknown.get();
        std::ostringstream s;
        s << className() << "." << where << ": " << value << " not found";  //�� �襭� �஡���� � �������� ४��ᨢ�� �맮���!!!
        throw EXCEPTIONS::EConvertError(s.str().c_str());
      }
      else
      {
        //��諨
        TO result=i->second;
        ++i;
        if (i==map.end() || i->first!=value) return result;
        std::ostringstream s;
        s << className() << "." << where << ": " << value << " duplicated";  //�� �襭� �஡���� � �������� ४��ᨢ�� �맮���!!!
        throw EXCEPTIONS::EConvertError(s.str().c_str());
      }
    }
  public:
    PairList(const std::initializer_list< std::pair<T1, T2> > &pairs,
             const boost::optional<T1>& unk1,
             const boost::optional<T2>& unk2)
    {
      for(const auto& i : pairs)
      {
        map1.emplace(i.first, i.second);
        map2.emplace(i.second, i.first);
      }
      unknown1=unk1;
      unknown2=unk2;
    }

    PairList(const std::list< std::pair<T1, T2> > &pairs,
             const boost::optional<T1>& unk1,
             const boost::optional<T2>& unk2)
    {
      for(const auto& i : pairs)
      {
        map1.emplace(i.first, i.second);
        map2.emplace(i.second, i.first);
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

template<class T>
const T& singletone()
{
  static T result;
  return result;
}

enum TClientType { ctTerm, ctWeb, ctKiosk, ctPNL, ctHTTP, ctMobile, ctEDI, ctTypeNum };
extern const char* ClientTypeS[ctTypeNum];

enum TOperMode { omCUSE, omCUTE, omMUSE, omRESA, omSTAND, omTypeNum };
extern const char* OperModeS[omTypeNum];

enum TEventType {evtSeason,evtDisp,evtFlt,evtGraph,evtFltTask,evtTimatic,evtPax,evtPay,evtComp,evtTlg,
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

const std::string refuseAgentError="�";

const int NoExists = INT_MIN;
const std::string NoDays = ".......";
const std::string AllDays = "1234567";

enum tmodify { fnochange, fchange, finsert, fdelete };

enum TCompLayerType { cltBlockCent, cltTranzit, cltCheckin, cltTCheckin, cltGoShow, cltBlockTrzt, cltSOMTrzt, cltPRLTrzt,
                      cltProtBeforePay, cltProtAfterPay, cltPNLBeforePay, cltPNLAfterPay, cltProtSelfCkin,
                      cltProtTrzt, cltPNLCkin, cltProtCkin, cltProtect, cltUncomfort, cltSmoke, cltDisable, cltUnknown,
                      cltTypeNum };
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
    rtBDOCSTXT,
    rtSPEC,
    rtSPECTXT,
    rtEMD,
    rtEMDTXT,
    rtLOADSHEET,
    rtNOTOC,
    rtLIR,
    rtANNUL,
    rtANNULTXT,
    rtVOUCHERS,
    rtSERVICES,
    rtSERVICESTXT,
    rtRESEAT,
    rtRESEATTXT,
    rtKOMPLEKT,
    rtCOM,
    rtUnknown,
    rtTypeNum
};
extern const char *RptTypeS[rtTypeNum];

class TGender
{
  public:
    enum Enum
    {
      Male,
      Female,
      Unknown
    };
};

class TTrickyGender
{
  public:
    enum Enum
    {
      Male,
      Female,
      Child,
      Infant,
      Unknown
    };

    static const std::list< std::pair<Enum, std::string> >& pairsForTlg()
    {
      static std::list< std::pair<Enum, std::string> > l;
      if (l.empty())
      {
        l.push_back(std::make_pair(Male,       "M"));
        l.push_back(std::make_pair(Female,     "F"));
        l.push_back(std::make_pair(Child,      "C"));
        l.push_back(std::make_pair(Infant,     "I"));
        l.push_back(std::make_pair(Unknown,    ""));
      }
      return l;
    }

    static const std::list< std::pair<Enum, std::string> >& pairsForDoc()
    {
      static std::list< std::pair<Enum, std::string> > l;
      if (l.empty())
      {
        l.push_back(std::make_pair(Male,       "ADL"));
        l.push_back(std::make_pair(Female,     "ADL"));
        l.push_back(std::make_pair(Child,      "CHD"));
        l.push_back(std::make_pair(Infant,     "INF"));
        l.push_back(std::make_pair(Unknown,    ""));
      }
      return l;
    }
};

class TTlgTrickyGenders : public ASTRA::PairList<TTrickyGender::Enum, std::string>
{
  private:
    virtual std::string className() const { return "TTlgTrickyGenders"; }
  public:
    TTlgTrickyGenders() : ASTRA::PairList<TTrickyGender::Enum, std::string>(TTrickyGender::pairsForTlg(),
                                                                            TTrickyGender::Unknown,
                                                                            boost::none) {}
};

class TDocTrickyGenders : public ASTRA::PairList<TTrickyGender::Enum, std::string>
{
  private:
    virtual std::string className() const { return "TDocTrickyGenders"; }
  public:
    TDocTrickyGenders() : ASTRA::PairList<TTrickyGender::Enum, std::string>(TTrickyGender::pairsForDoc(),
                                                                            boost::none,
                                                                            boost::none) {}
};

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

struct TPaxTypeExt
{
  TPaxStatus       _pax_status;
  TCrewType::Enum  _crew_type;
  TPaxTypeExt(TPaxStatus ps, TCrewType::Enum ct)
    : _pax_status(ps), _crew_type(ct) {}
  std::string ToString() const;
  private:
  TPaxTypeExt();
};

#define TRACE_PARAMS level, nick, file, line
#define ERROR_PARAMS    -1, nick, file, line

enum TIdType {idFlt, idGrp, idPax};

};

enum PaxOrigin { paxCheckIn, paxPnl, paxTest };

const ASTRA::TCrewTypes& CrewTypes();
const ASTRA::TTlgTrickyGenders& TlgTrickyGenders();
const ASTRA::TDocTrickyGenders& DocTrickyGenders();

const std::string TIMEOUT_OCCURRED = "Timeout occurred";
const std::string ACCESS_DENIED = "Access denied";

// TODO: get rid of this outrageous define
#define OP_TYPE_COND(COL) "nvl(" COL", :op_type) = :op_type" // c++11 standard requires space before identifier: ..." COL"...

class DCSAction
{
  public:
    enum Enum
    {
      PrintBPOnDesk,
      ChangeSeatOnDesk
    };

    static const std::list< std::pair<Enum, std::string> >& pairs()
    {
      static std::list< std::pair<Enum, std::string> > l =
      {
        {PrintBPOnDesk,    "PRINT_BP_ON_DESK"},
        {ChangeSeatOnDesk, "CHG_SEAT_ON_DESK"}
      };
      return l;
    }
};

class DCSActionsContainer : public ASTRA::PairList<DCSAction::Enum, std::string>
{
  private:
    virtual std::string className() const { return "DCSActions"; }
  public:
    DCSActionsContainer() : ASTRA::PairList<DCSAction::Enum, std::string>(DCSAction::pairs(),
                                                                          boost::none,
                                                                          boost::none) {}
};

const DCSActionsContainer& dcsActions();

class TAlignment
{
  public:
    enum Enum
    {
      LeftJustify,
      RightJustify,
      Center
    };

    static const std::list< std::pair<Enum, std::string> >& pairs()
    {
      static std::list< std::pair<Enum, std::string> > l;
      if (l.empty())
      {
        l.push_back(std::make_pair(LeftJustify,    "taLeftJustify"));
        l.push_back(std::make_pair(RightJustify,   "taRightJustify"));
        l.push_back(std::make_pair(Center,         "taCenter"));
      }
      return l;
    }
};

class TAlignments : public ASTRA::PairList<TAlignment::Enum, std::string>
{
  private:
    virtual std::string className() const { return "TAlignments"; }
  public:
    TAlignments() : ASTRA::PairList<TAlignment::Enum, std::string>(TAlignment::pairs(),
                                                                   boost::none,
                                                                   boost::none) {}
};

const TAlignments& Alignments();

const std::string fsBold="fsBold";

#endif

