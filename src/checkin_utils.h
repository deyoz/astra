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
    boost::optional<CheckIn::PaxRems> rems;
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
      rems=boost::none;
      refuse = false;
    }

    TWebPaxFromReq& fromDB(TQuery &Qry);
    TWebPaxFromReq& fromXML(xmlNodePtr node);

    static bool isRemProcessingAllowed(const CheckIn::TPaxFQTItem& fqt);
    static bool isRemProcessingAllowed(const CheckIn::TPaxRemItem& rem);

    template<typename ContainerT, typename ItemT>
    static void remsFromXML(xmlNodePtr paxNode, boost::optional<ContainerT>& dest)
    {
      if (paxNode==nullptr) return;

      xmlNodePtr node2=paxNode->children;

      auto tagNames=ItemT::getWebXMLTagNames();

      xmlNodePtr itemNode = GetNodeFast(tagNames.first.c_str(), node2);
      if (itemNode!=nullptr)
      {
        //если тег пришел, то изменяем и перезаписываем ремарки
        dest=ContainerT();
        //читаем пришедшие ремарки
        for(itemNode=itemNode->children; itemNode!=NULL; itemNode=itemNode->next)
        {
          if (std::string((const char*)itemNode->name)!=tagNames.second) continue;
          ItemT item;
          item.fromWebXML(itemNode);
          if (!isRemProcessingAllowed(item)) continue;
          dest.get().insert(item);
        }
      };
    }

    template<typename ContainerT>
    static bool mergeRems(const boost::optional<ContainerT>& src, ContainerT& dest)
    {
      if (!src) return false;
      std::multiset<std::string> prior, curr;

      for(typename ContainerT::iterator i=dest.begin(); i!=dest.end();)
      {
        if (isRemProcessingAllowed(*i))
        {
          prior.insert(i->rem_text(false));
          i=dest.erase(i);
        }
        else
          ++i;
      }

      for(typename ContainerT::iterator i=src.get().begin(); i!=src.get().end(); ++i)
        curr.insert(i->rem_text(false));

      std::copy(src.get().begin(), src.get().end(),
                inserter(dest, dest.end()));

      return prior!=curr;
    }


    bool mergePaxFQT(std::set<CheckIn::TPaxFQTItem> &dest) const;
    bool mergePaxRem(CheckIn::PaxRems &dest) const;
    void paxFQTFromXML(xmlNodePtr paxNode);
    void paxRemFromXML(xmlNodePtr paxNode);

    bool isTest() const { return isTestPaxId(id); }
    bool checked() const
    {
      return !isTest() && TWebTids::checked();
    }
};

class TWebPaxForChng : public CheckIn::TSimplePaxGrpItem, public CheckIn::TSimplePaxItem, public TWebTids
{
  public:
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

    static const std::string& sql()
    {
      static const std::string result=
          "SELECT pax_grp.*, pax.*, "
          "       salons.get_seat_no(pax.pax_id,pax.seats,NULL,pax_grp.status,pax_grp.point_dep,'one',rownum) AS seat_no, "
          "       crs_pax.tid AS crs_pax_tid, "
          "       pax_grp.tid AS pax_grp_tid, "
          "       pax.tid AS pax_tid, "
          "       crs_pax.pnr_id "
          "FROM pax_grp,pax,crs_pax "
          "WHERE pax_grp.grp_id=pax.grp_id AND "
          "      pax.pax_id=crs_pax.pax_id(+) AND "
          "      crs_pax.pr_del(+)=0 AND "
          "      pax.pax_id=:pax_id";

      return result;
    }
};

class TWebPaxForCkin : public CheckIn::TSimplePnrItem, public CheckIn::TSimplePaxItem, public TWebTids
{
  public:
    TPnrAddrs pnr_addrs;
    TWebAPISItem apis;
    bool dont_check_payment;

    TWebPaxForCkin() { clear(); }

    void clear()
    {
      CheckIn::TSimplePnrItem::clear();
      CheckIn::TSimplePaxItem::clear();
      TWebTids::clear();
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

    TWebPaxForCkin& initFromMeridian(const std::string& _airp_arv, const ASTRA::TCrewType::Enum& _crew_type)
    {
      clear();
      airp_arv=_airp_arv;
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

    static const std::string& sql(bool isTest)
    {
      static const std::string result1=
          "SELECT crs_pnr.pnr_id, "
          "       crs_pnr.airp_arv, "
          "       crs_pnr.class, "
          "       crs_pnr.subclass, "
          "       crs_pnr.status, "
          "       crs_pax.pax_id, "
          "       crs_pax.surname,crs_pax.name,crs_pax.pers_type, "
          "       salons.get_crs_seat_no(crs_pax.pax_id,crs_pax.seat_xname,crs_pax.seat_yname,crs_pax.seats,crs_pnr.point_id,'one',rownum) AS seat_no, "
          "       crs_pax.seat_type, "
          "       crs_pax.seats, "
          "       crs_pnr.tid AS crs_pnr_tid, "
          "       crs_pax.tid AS crs_pax_tid, "
          "       pax.tid AS pax_tid "
          "FROM crs_pnr,crs_pax,pax "
          "WHERE crs_pnr.pnr_id=crs_pax.pnr_id AND "
          "      crs_pax.pax_id=pax.pax_id(+) AND "
          "      crs_pax.pax_id=:pax_id AND "
          "      crs_pax.pr_del=0 ";

      static const std::string result2=
          "SELECT test_pax.id AS pnr_id, "
          "       NULL AS airp_arv, "
          "       subcls.class, "
          "       subclass, "
          "       NULL AS status, "
          "       test_pax.id AS pax_id, "
          "       surname, name, "
          "       NULL AS seat_no, "
          "       doc_no, tkn_no, pnr_airline, pnr_addr, "
          "       test_pax.id AS crs_pnr_tid, test_pax.id AS crs_pax_tid "
          "FROM test_pax, subcls "
          "WHERE test_pax.subclass=subcls.code AND test_pax.id=:pax_id";

      return isTest?result2:result1;
    }
};

class TWebPaxFromReqList : public std::list<TWebPaxFromReq>
{
  public:
    int notCheckedCount() const
    {
      int result=0;
      for(const TWebPaxFromReq& pax : *this)
        if (!pax.checked() && !pax.refuse) result++;
      return result;
    }
    int refusalCount() const
    {
      int result=0;
      for(const TWebPaxFromReq& pax : *this)
        if (pax.checked() && pax.refuse) result++;
      return result;
    }
    int notRefusalCount() const
    {
      int result=0;
      for(const TWebPaxFromReq& pax : *this)
        if (!pax.refuse) result++;
      return result;
    }
    const TWebPaxFromReq& get(int id, const std::string& whence) const;
};

class TWebPaxForChngList : public std::list<TWebPaxForChng>
{
  public:
    void checkUniquenessAndAdd(const TWebPaxForChng& newPax);
};

class TWebPaxForCkinList : public std::list<TWebPaxForCkin>
{
  public:
    ASTRA::TPaxStatus status;

    TWebPaxForCkinList()
    {
      status = ASTRA::psCheckin;
    }

    bool infantsMoreThanAdults() const
    {
      return CheckIn::infantsMoreThanAdults(*this);
    }

    void checkUniquenessAndAdd(const TWebPaxForCkin& newPax);
};

class TWebPaxForSaveSeg
{
  public:
    int point_id;
    int pnr_id;
    TWebPaxFromReqList paxFromReq;
    TWebPaxForChngList paxForChng;
    TWebPaxForCkinList paxForCkin;

    TWebPaxForSaveSeg(int _point_id, int _pnr_id=ASTRA::NoExists) :
      point_id(_point_id), pnr_id(_pnr_id) {}

    void checkAndSortPaxForCkin(const TWebPaxForSaveSeg& seg);
};

class TWebPaxForSaveSegs : public std::vector<TWebPaxForSaveSeg>
{
  public:
    TWebPaxForSaveSegs(const TWebPaxForSaveSeg& seg)
    {
      push_back(seg);
    }
    TWebPaxForSaveSegs() {}

    void checkAndSortPaxForCkin();
    void checkSegmentsFromReq(int& firstPointIdForCkin);
};

class TMultiPNRSegInfo : public std::map<int/*pnr_id*/, WebSearch::TPNRSegInfo>
{
  private:
    std::map<int/*point_id*/, TTripRoute> routes;

  public:
    void add(const TAdvTripInfo &operFlt, const TWebPaxForCkin& pax, bool first_segment);
};

class TMultiPnrData
{
  public:
    WebSearch::TFlightInfo flt;
    WebSearch::TDestInfo dest;
    TMultiPNRSegInfo segs;

    void checkJointCheckInAndComplete();
};

class TMultiPnrDataSegs : public std::list<TMultiPnrData>
{
  public:
    TMultiPnrData& add(int point_id, bool first_segment, bool with_additional);
};

void CheckSeatNoFromReq(int point_id,
                        int crs_pax_id,
                        const std::string &prior_seat_no,
                        const std::string &curr_seat_no,
                        std::string &curr_xname,
                        std::string &curr_yname,
                        bool &changed);
void CreateEmulRems(xmlNodePtr paxNode, const std::multiset<CheckIn::TPaxRemItem> &rems, const std::set<CheckIn::TPaxFQTItem> &fqts);
void CreateEmulXMLDoc(xmlNodePtr reqNode, XMLDoc &emulDoc);
void CreateEmulXMLDoc(XMLDoc &emulDoc);
void CopyEmulXMLDoc(const XMLDoc &srcDoc, XMLDoc &destDoc);
void CreateEmulDocs(const TWebPaxForSaveSegs &segs,
                    const TMultiPnrDataSegs &multiPnrDataSegs,
                    const XMLDoc &emulDocHeader,
                    XMLDoc &emulCkinDoc);
void CreateEmulDocs(const TWebPaxForSaveSegs &segs,
                    const XMLDoc &emulDocHeader,
                    std::map<int,XMLDoc> &emulChngDocs);

void tryGenerateBagTags(xmlNodePtr reqNode);

#endif // PNR_INFORM_H
