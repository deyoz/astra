#ifndef PNR_INFORM_H
#define PNR_INFORM_H

#include "astra_consts.h"
#include "passenger.h"
#include "web_search.h"
#include "apis_utils.h"

struct TWebPaxFromReq
{
  int crs_pax_id;
  std::string seat_no;
  CheckIn::TPaxDocItem doc;
  CheckIn::TPaxDocoItem doco;
  std::vector<std::string> fqt_rems;
  bool fqt_rems_present;
  std::set<TCheckInfoType> present_in_req;
  bool refuse;
  int crs_pnr_tid;
    int crs_pax_tid;
    int pax_grp_tid;
    int pax_tid;
  TWebPaxFromReq() {
        crs_pax_id = ASTRA::NoExists;
        fqt_rems_present = false;
        refuse = false;
        crs_pnr_tid = ASTRA::NoExists;
        crs_pax_tid	= ASTRA::NoExists;
        pax_grp_tid = ASTRA::NoExists;
        pax_tid = ASTRA::NoExists;
    };
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
  std::string seat_no;
  int seats;

  CheckIn::TPaxDocItem doc;
  CheckIn::TPaxDocoItem doco;
  std::set<TCheckInfoType> present_in_req;
};

struct TWebPaxForCkin
{
  int crs_pax_id;

  std::string surname;
  std::string name;
  std::string pers_type;
  std::string seat_no;
  std::string seat_type;
  int seats;
  std::string eticket;
  std::string ticket;
  CheckIn::TAPISItem apis;
  std::set<TCheckInfoType> present_in_req;
  std::string subclass;
  int reg_no;

  TWebPaxForCkin()
  {
    crs_pax_id = ASTRA::NoExists;
    seats = ASTRA::NoExists;
    reg_no = ASTRA::NoExists;
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
void CreateEmulRems(xmlNodePtr paxNode, TQuery &RemQry, const std::vector<std::string> &fqtv_rems);
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
