#ifndef PNR_INFORM_H
#define PNR_INFORM_H

#include "astra_consts.h"
#include "passenger.h"
#include "web_search.h"
#include "apis_utils.h"

namespace CheckIn
{

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

void tryGenerateBagTags(xmlNodePtr reqNode);

#endif // PNR_INFORM_H
