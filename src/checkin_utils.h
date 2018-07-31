#ifndef PNR_INFORM_H
#define PNR_INFORM_H

#include "astra_consts.h"
#include "passenger.h"
#include "web_search.h"
#include "apis_utils.h"

struct TSegListItem
{
  TTripInfo flt;
  CheckIn::TPaxGrpItem grp;
  CheckIn::TPaxList paxs;
};

namespace CheckIn
{

void seatingWhenNewCheckIn(const TSegListItem& seg,
                           const TAdvTripInfo& fltAdvInfo,
                           const TTripInfo& markFltInfo);

class OverloadException: public AstraLocale::UserException
{
  public:
    OverloadException(const std::string &msg):AstraLocale::UserException(msg) {}
    virtual ~OverloadException() throw(){}
};

class UserException:public AstraLocale::UserException
{
    public:
      std::map<int, std::map <int, AstraLocale::LexemaData> > segs;

      UserException(const AstraLocale::LexemaData &lexemeData,
                  int point_id,
                  int pax_id = ASTRA::NoExists):AstraLocale::UserException(lexemeData.lexema_id, lexemeData.lparams)
    {
      addError(lexemeData, point_id, pax_id);
    };
    UserException():AstraLocale::UserException("Empty CheckIn::UserException!", AstraLocale::LParams()) {};
    ~UserException() throw(){};
    void addError(const AstraLocale::LexemaData &lexemeData,
                  int point_id,
                  int pax_id = ASTRA::NoExists)
    {
      if (segs.empty()) setLexemaData(lexemeData);
      segs[point_id][pax_id]=lexemeData;
    }
/*  если кто-то надумает раскомментарить этот кусок, обратитесь сначала к Владу
    void addError(const std::string &lexema_id, const AstraLocale::LParams &lparams,
                  int point_id,
                  int pax_id = ASTRA::NoExists)
    {
      AstraLocale::LexemaData data;
      data.lexema_id = lexema_id;
        data.lparams = lparams;
        addError(data, point_id, pax_id);
    };
    void addError(const std::string &lexema_id,
                  int point_id,
                  int pax_id = ASTRA::NoExists)
    {
        addError(lexema_id, AstraLocale::LParams(), point_id, pax_id);
    };*/
    bool empty() { return segs.empty(); }
};

void showError(const std::map<int, std::map <int, AstraLocale::LexemaData> > &segs);

class RegNoRange
{
  public:
    int first_no;
    int count;
    RegNoRange(int _first_no, int _last_no)
    {
      first_no=_first_no;
      count=_last_no>=_first_no?_last_no-_first_no+1:0;
    }

    bool operator < (const RegNoRange& range) const
    {
      if (count!=range.count)
        return count<range.count;
      return first_no<range.first_no;
    }
};

class UsedRegNo
{
  public:
    int value;
    int grp_id;
    bool without_seat;
    UsedRegNo(int _value, int _grp_id, bool _without_seat) :
      value(_value), grp_id(_grp_id), without_seat(_without_seat) {}

    bool hasGap(const UsedRegNo& usedRegNo) const
    {
      return grp_id!=usedRegNo.grp_id &&
             abs(value-usedRegNo.value)>1;
    }

    bool operator < (const UsedRegNo& usedRegNo) const
    {
      return value<usedRegNo.value;
    }
};

class RegNoGenerator
{
  public:
    enum Type {Negative, Positive};
    enum Strategy {AbsoluteDefragAnytime, //это баловство :)
                   AbsoluteDefragAtLast,  //это тоже баловство ;)
                   DefragAnytime,
                   DefragAtLast};

  private:
    int _point_id;
    Type _type;
    int _maxAbsRegNo;
    std::set<RegNoRange> unusedRanges;
    boost::optional<RegNoRange> lastUnusedRange;
    void getUsedRegNo(std::set<UsedRegNo>& usedRegNo) const;
    void fillUnusedRanges(const std::set<UsedRegNo>& usedRegNo);
  public:
    RegNoGenerator(int point_id, Type type, int maxAbsRegNo=999);
    RegNoGenerator(const std::set<UsedRegNo>& usedRegNo, Type type, int maxAbsRegNo=999);
    boost::optional<RegNoRange> getRange(int count, Strategy strategy) const;
    void traceUnusedRanges() const;
    static std::string traceStr(const boost::optional<RegNoRange>& range);
};

} //namespace CheckIn

class TWebTids
{
  public:
    int crs_pnr_tid;
    int crs_pax_tid;
    int pax_grp_tid;
    int pax_tid;
    bool passengerAlreadyChecked;

    TWebTids() { clear(); }

    void clear()
    {
      crs_pnr_tid = ASTRA::NoExists;
      crs_pax_tid = ASTRA::NoExists;
      pax_grp_tid = ASTRA::NoExists;
      pax_tid     = ASTRA::NoExists;
      passengerAlreadyChecked = false;
    }

    TWebTids& fromDB(TQuery &Qry);
    TWebTids& fromXML(xmlNodePtr node);
    const TWebTids& toXML(xmlNodePtr node) const;

    bool checked() const
    {
      return passengerAlreadyChecked ||
             !(pax_grp_tid==ASTRA::NoExists && pax_tid==ASTRA::NoExists);
    }
    bool norec() const
    {
      return crs_pnr_tid==ASTRA::NoExists && crs_pax_tid==ASTRA::NoExists;
    }
};

class TWebAPISItem : public CheckIn::TAPISItem
{
  private:
    std::set<TAPIType> presentAPITypes;
  public:

    TWebAPISItem() { clear(); }

    void clear()
    {
      CheckIn::TAPISItem::clear();
      presentAPITypes.clear();
    }

    TWebAPISItem& fromXML(xmlNodePtr node);

    bool isPresent(const TAPIType& type) const
    {
      return presentAPITypes.find(type)!=presentAPITypes.end();
    }

    void setNormalized(const TWebAPISItem& apis)
    {
      clear();
      if (apis.isPresent(apiDoc))
        set(NormalizeDoc(apis.doc));

      if (apis.isPresent(apiDoco))
        set(NormalizeDoco(apis.doco));
    }

    void set(const CheckIn::TPaxAPIItem& item);
};

class TWebPaxFromReq : public TWebTids
{
  public:
    int id;
    bool dont_check_payment;
    std::string seat_no;
    TWebAPISItem apis;
    boost::optional<std::set<CheckIn::TPaxFQTItem>> fqtv_rems;
    bool refuse;

    TWebPaxFromReq() { clear(); }

    void clear()
    {
      TWebTids::clear();
      id = ASTRA::NoExists;
      dont_check_payment = false;
      seat_no.clear();
      apis.clear();
      fqtv_rems=boost::none;
      refuse = false;
    }

    TWebPaxFromReq& fromDB(TQuery &Qry);
    TWebPaxFromReq& fromXML(xmlNodePtr node);

    bool mergePaxFQT(std::set<CheckIn::TPaxFQTItem> &fqts) const;
    bool isTest() const { return isTestPaxId(id); }
    bool checked() const
    {
      return !isTest() && TWebTids::checked();
    }
};

class TWebPaxForChng : public CheckIn::TSimplePaxGrpItem, public CheckIn::TSimplePaxItem, public TWebTids
{
  public:
//    int crs_pax_id;
//    int grp_id;
//    int point_dep;
//    int point_arv;
//    std::string airp_dep;
//    std::string airp_arv;
//    std::string cl;
//    int excess;
//    bool bag_refuse;

//    std::string surname;
//    std::string name;
//    std::string pers_type;
//    ASTRA::TCrewType::Enum crew_type;
//    std::string seat_no;
//    int seats;

//    CheckIn::TPaxDocItem doc;
//    CheckIn::TPaxDocoItem doco;
//    std::set<TAPIType> present_in_req;

    TWebAPISItem apis;

    TWebPaxForChng() { clear(); }

    void clear()
    {
      CheckIn::TSimplePaxGrpItem::clear();
      CheckIn::TSimplePaxItem::clear();
      TWebTids::clear();
      apis.clear();
    }

    TWebPaxForChng& fromDB(TQuery &Qry);

    TWebPaxForChng& addFromReq(const TWebPaxFromReq& pax)
    {
      apis.setNormalized(pax.apis);
      return *this;
    }
};

class TWebPaxForCkin : public CheckIn::TSimplePaxItem, public TWebTids
{
  public:
//  int crs_pax_id;

//  std::string surname;
//  std::string name;
//  std::string pers_type;
//  ASTRA::TCrewType::Enum crew_type;
//  std::string seat_no;
//  std::string seat_type;
//  int seats;
//  std::string eticket;
//  std::string ticket;
//  CheckIn::TAPISItem apis;
//  std::set<TAPIType> present_in_req;
//  std::string subclass;
//  int reg_no;

    std::string pnr_status;
    TPnrAddrs pnr_addrs;
    TWebAPISItem apis;
    bool dont_check_payment;

    TWebPaxForCkin() { clear(); }

    void clear()
    {
      CheckIn::TSimplePaxItem::clear();
      TWebTids::clear();
      pnr_status.clear();
      pnr_addrs.clear();
      apis.clear();
      dont_check_payment = false;
    }

    bool operator == (const TWebPaxForCkin &pax) const
    {
      return transliter_equal(surname,pax.surname) &&
             transliter_equal(name,pax.name) &&
             pers_type == pax.pers_type &&
             ((seats == 0 && pax.seats == 0) || (seats != 0 && pax.seats != 0)) &&
             pnr_addrs.equalPnrExists(pax.pnr_addrs);
    }

    const std::string traceStr() const
    {
      std::ostringstream s;
      s << "pnr_addrs=" << pnr_addrs.traceStr() << ", "
           "surname=" << surname << ", "
           "name=" << name << ", "
           "pers_type=" << EncodePerson(pers_type) << ", "
           "seats=" << seats;
      return s.str();
    }

    TWebPaxForCkin& fromDB(TQuery &Qry);

    TWebPaxForCkin& addFromReq(const TWebPaxFromReq& pax)
    {
      apis.setNormalized(pax.apis);
      dont_check_payment=pax.dont_check_payment;
      return *this;
    }

    TWebPaxForCkin& initFromMeridian(const ASTRA::TCrewType::Enum& _crew_type)
    {
      clear();
      crew_type=_crew_type;
      pers_type = ASTRA::adult;
      seats = 1;
      return *this;
    }

    TWebPaxForCkin& setFromMeridian(const CheckIn::TPaxDocItem doc)
    {
      surname=doc.surname;
      name=doc.first_name+" "+doc.second_name;
      name=TrimString(name);
      apis.set(doc);
      return *this;
    }
};

class TWebPaxFromReqList : public std::list<TWebPaxFromReq>
{
  public:
    int notCheckedCount() const
    {
      int result=0;
      for(const TWebPaxFromReq& pax : *this)
        if (!pax.checked()) result++;
      return result;
    }
    int refusalCount() const
    {
      int result=0;
      for(const TWebPaxFromReq& pax : *this)
        if (pax.refuse) result++;
      return result;
    }
};

class TWebPaxForChngList : public std::list<TWebPaxForChng>
{

};

class TWebPaxForCkinList : public std::list<TWebPaxForCkin>
{
  public:
    bool infantsMoreThanAdults() const
    {
      int adult_count=0, without_seat_count=0;
      for(const TWebPaxForCkin& pax : *this)
      {
        if (pax.pers_type==ASTRA::adult) adult_count++;
        if (pax.pers_type==ASTRA::baby && pax.seats==0) without_seat_count++;
      }
      return without_seat_count>adult_count;
    }
};

class TWebPnrForSave
{
  public:
    int pnr_id;
    ASTRA::TPaxStatus status;
    TWebPaxFromReqList paxFromReq;
    TWebPaxForChngList paxForChng;
    TWebPaxForCkinList paxForCkin;

    TWebPnrForSave() {
      pnr_id = ASTRA::NoExists;
      status = ASTRA::psCheckin;
    }
};

//class TMultiWebPnrForSave : public std::map<int/*pnr_id*/, TWebPnrForSave> {};

//class TMultiWebPnrForSaveSeg : public TMultiWebPnrForSave
//{
//  public:
//    int point_id;

//    TMultiWebPnrForSaveSeg() {
//      point_id = ASTRA::NoExists;
//    }
//};

//class TMultiWebPnrForSaveSegs : public std::list<TMultiWebPnrForSaveSeg> {};

//class TMultiPnrData : public std::map<int/*pnr_id*/, WebSearch::TPnrData> {};

//class TMultiPnrDataSegs : public std::list<TMultiPnrData> {};

void CheckSeatNoFromReq(int point_id,
                        int crs_pax_id,
                        const std::string &prior_seat_no,
                        const std::string &curr_seat_no,
                        std::string &curr_xname,
                        std::string &curr_yname,
                        bool &changed);
void CreateEmulRems(xmlNodePtr paxNode, const std::multiset<CheckIn::TPaxRemItem> &rems, const std::set<CheckIn::TPaxFQTItem> &fqts);
void CompletePnrDataForCrew(const std::string &airp_arv, WebSearch::TPnrData &pnrData);
void CompletePnrData(bool is_test, int pnr_id, WebSearch::TPnrData &pnrData);
void CreateEmulXMLDoc(xmlNodePtr reqNode, XMLDoc &emulDoc);
void CreateEmulXMLDoc(XMLDoc &emulDoc);
void CopyEmulXMLDoc(const XMLDoc &srcDoc, XMLDoc &destDoc);
void CreateEmulDocs(const std::vector< std::pair<int/*point_id*/, TWebPnrForSave > > &segs,
                    const std::vector<WebSearch::TPnrData> &PNRs,
                    const XMLDoc &emulDocHeader,
                    XMLDoc &emulCkinDoc, std::map<int,XMLDoc> &emulChngDocs );

void tryGenerateBagTags(xmlNodePtr reqNode);

#endif // PNR_INFORM_H
