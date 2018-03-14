#ifndef PNR_INFORM_H
#define PNR_INFORM_H

#include "astra_consts.h"
#include "passenger.h"
#include "web_search.h"
#include "apis_utils.h"

namespace CheckIn
{

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
    UsedRegNo(int _value, int _grp_id) :
      value(_value), grp_id(_grp_id) {}

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

struct TWebPaxFromReq
{
  int crs_pax_id;
  bool dont_check_payment;
  std::string seat_no;
  CheckIn::TPaxDocItem doc;
  CheckIn::TPaxDocoItem doco;
  std::set<CheckIn::TPaxFQTItem> fqtv_rems;
  bool fqtv_rems_present;
  std::set<TAPIType> present_in_req;
  bool refuse;
  int crs_pnr_tid;
  int crs_pax_tid;
  int pax_grp_tid;
  int pax_tid;
  TWebPaxFromReq() {
        crs_pax_id = ASTRA::NoExists;
        dont_check_payment = false;
        fqtv_rems_present = false;
        refuse = false;
        crs_pnr_tid = ASTRA::NoExists;
        crs_pax_tid	= ASTRA::NoExists;
        pax_grp_tid = ASTRA::NoExists;
        pax_tid = ASTRA::NoExists;
    };
  bool mergePaxFQT(std::set<CheckIn::TPaxFQTItem> &fqts) const;
};

struct TWebPaxForChng
{
  int crs_pax_id;
  int grp_id;
  int point_dep;
  int point_arv;
  std::string airp_dep;
  std::string airp_arv;
  std::string cl;
  int excess;
  bool bag_refuse;

  std::string surname;
  std::string name;
  std::string pers_type;
  ASTRA::TCrewType::Enum crew_type;
  std::string seat_no;
  int seats;

  CheckIn::TPaxDocItem doc;
  CheckIn::TPaxDocoItem doco;
  std::set<TAPIType> present_in_req;

  TWebPaxForChng()
  {
    crew_type = ASTRA::TCrewType::Unknown;
  }
};

struct TWebPaxForCkin
{
  int crs_pax_id;

  std::string surname;
  std::string name;
  std::string pers_type;
  ASTRA::TCrewType::Enum crew_type;
  std::string seat_no;
  std::string seat_type;
  int seats;
  std::string eticket;
  std::string ticket;
  CheckIn::TAPISItem apis;
  std::set<TAPIType> present_in_req;
  std::string subclass;
  int reg_no;
  bool dont_check_payment;

  TWebPaxForCkin()
  {
    crs_pax_id = ASTRA::NoExists;
    seats = ASTRA::NoExists;
    reg_no = ASTRA::NoExists;
    dont_check_payment = false;
    crew_type = ASTRA::TCrewType::Unknown;
  }

  bool operator == (const TWebPaxForCkin &pax) const
  {
    return transliter_equal(surname,pax.surname) &&
           transliter_equal(name,pax.name) &&
           pers_type == pax.pers_type &&
           ((seats == 0 && pax.seats == 0) || (seats != 0 && pax.seats != 0));
  }
};

struct TWebPnrForSave
{
  int pnr_id;
  ASTRA::TPaxStatus status;
  std::vector<TWebPaxFromReq> paxFromReq;
  unsigned int refusalCountFromReq;
  std::list<TWebPaxForChng> paxForChng;
  std::list<TWebPaxForCkin> paxForCkin;

  TWebPnrForSave() {
    pnr_id = ASTRA::NoExists;
    status = ASTRA::psCheckin;
    refusalCountFromReq = 0;
  }
};

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

#endif // PNR_INFORM_H
