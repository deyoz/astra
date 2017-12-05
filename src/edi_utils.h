#ifndef _EDI_UTILS_H_
#define _EDI_UTILS_H_

#include "astra_misc.h"
#include "passenger.h"
#include "tlg/EdifactRequest.h"
#include "tlg/remote_results.h"
#include <etick/tick_data.h>

namespace AstraEdifact
{

Ticketing::CouponStatus calcPaxCouponStatus(const std::string& refuse,
                                            bool pr_brd,
                                            bool in_final_status);

class ETSExchangeStatus
{
  public:
    enum Enum { NotConnected,
                Online,
                Finalized,
                Unknown
              };
    static Enum fromDB(TQuery &Qry)
    {
      int pr_etstatus=Qry.FieldAsInteger("pr_etstatus");
      if (pr_etstatus<0) return NotConnected;
      if (pr_etstatus>0) return Finalized;
      return Online;

    }
    static void toDB(const Enum value, TQuery &Qry)
    {
      int pr_etstatus=0;
      switch(value)
      {
        case NotConnected:
          pr_etstatus=-1;
          break;
        case Online:
          pr_etstatus=0;
          break;
        case Finalized:
          pr_etstatus=1;
          break;
        case Unknown:
          return;
      }
      Qry.SetVariable("pr_etstatus", pr_etstatus);
    }
};

class TFltParams
{
  public:
    TAdvTripInfo fltInfo;
    bool ets_no_exchange;
    bool eds_no_exchange;
    bool control_method;
    bool in_final_status;
    ETSExchangeStatus::Enum ets_exchange_status;
    int et_final_attempt;
    void clear()
    {
      fltInfo.Clear();
      ets_no_exchange=false;
      eds_no_exchange=false;
      control_method=false;
      in_final_status=false;
      ets_exchange_status=ETSExchangeStatus::Unknown;
      et_final_attempt=ASTRA::NoExists;
    }
    bool get(int point_id);
    bool get(const TAdvTripInfo& _fltInfo);
    static void incFinalAttempts(int point_id);
    static void finishFinalAttempts(int point_id);
    static void setETSExchangeStatus(int point_id, ETSExchangeStatus::Enum status);
    static bool returnOnlineStatus(int point_id);
    static bool get(const TAdvTripInfo& fltInfo,
                    bool &control_method,
                    bool &in_final_status);
};

Ticketing::TicketNum_t checkDocNum(const std::string& doc_no, bool is_et);

bool checkETSExchange(const TTripInfo& info,
                      bool with_exception);

bool checkETSInteract(const TTripInfo& info,
                      bool with_exception);

bool checkEDSExchange(const TTripInfo& info,
                      bool with_exception);

bool checkETSExchange(int point_id,
                      bool with_exception,
                      TTripInfo& info);

bool checkETSInteract(int point_id,
                      bool with_exception,
                      TTripInfo& info);

bool checkEDSExchange(int point_id,
                      bool with_exception,
                      TTripInfo& info);

std::string getTripAirline(const TTripInfo& ti);

Ticketing::FlightNum_t getTripFlightNum(const TTripInfo& ti);

bool get_et_addr_set(const std::string &airline,
                     int flt_no,
                     std::pair<std::string,std::string> &addrs);

bool get_et_addr_set(const std::string &airline,
                     int flt_no,
                     std::pair<std::string,std::string> &addrs,
                     int &id);

std::string get_canon_name(const std::string& edi_addr);

void copy_notify_levb(int src_edi_sess_id,
                      int dest_edi_sess_id,
                      bool err_if_not_found);
void confirm_notify_levb(int edi_sess_id, bool err_if_not_found);
std::string make_xml_kick(const edifact::KickInfo &kickInfo);
edifact::KickInfo createKickInfo(int v_reqCtxtId,
                                 const std::string &v_iface);

void addToEdiResponseCtxt(int ctxtId,
                          const xmlNodePtr srcNode,
                          const std::string &destNodeName);

void getEdiResponseCtxt(int ctxtId,
                        bool clear,
                        const std::string &where,
                        std::string &context,
                        bool throwEmpty=true);

void getEdiResponseCtxt(int ctxtId,
                        bool clear,
                        const std::string &where,
                        XMLDoc &xmlCtxt,
                        bool throwEmpty=true);

void getTermRequestCtxt(int ctxtId,
                        bool clear,
                        const std::string &where,
                        XMLDoc &xmlCtxt);

void getHttpRequestCtxt(int ctxtId,
                        bool clear,
                        const std::string &where,
                        XMLDoc &xmlCtxt);

void getEdiSessionCtxt(int sessIda,
                       bool clear,
                       const std::string& where,
                       XMLDoc &xmlCtxt);

std::string getEdiSessionCtxt(int sessIda,
                              bool clear,
                              bool throwEmpty = true);

void cleanOldRecords(int min_ago);

void HandleNotSuccessEtsResult(const edifact::RemoteResults& res);

void ProcEdiError(const AstraLocale::LexemaData &error,
                  const xmlNodePtr errorCtxtNode,
                  bool isGlobal);
typedef std::list< std::pair<AstraLocale::LexemaData, bool> > EdiErrorList;

void GetEdiError(const xmlNodePtr errorCtxtNode, EdiErrorList &errors);

void WritePostponedContext(tlgnum_t tnum, int reqCtxtId);

int ReadPostponedContext(tlgnum_t tnum, bool clear);

void ClearPostponedContext(tlgnum_t tnum);

bool isTermCheckinRequest(xmlNodePtr reqNode);
bool isWebCheckinRequest(xmlNodePtr reqNode);

class TOriginCtxt
{
  public:
    std::string screen;
    std::string user_descr;
    std::string desk_code;
    TOriginCtxt()
    {
      clear();
    }
    TOriginCtxt(const TReqInfo &reqInfo)
    {
      screen=reqInfo.screen.name;
      user_descr=reqInfo.user.descr;
      desk_code=reqInfo.desk.code;
    }

    void clear()
    {
      screen.clear();
      user_descr.clear();
      desk_code.clear();
    }

    static void toXML(xmlNodePtr node);
    TOriginCtxt& fromXML(xmlNodePtr node);
};

class TPaxCtxt
{
  public:
    CheckIn::TSimplePaxItem pax;
    int point_id;
    std::string flight;

    TPaxCtxt()
    {
      clear();
    }

    void clear()
    {
      pax.clear();
      point_id=ASTRA::NoExists;
      flight.clear();
    }

    bool paxUnknown() const
    {
      return pax.id==ASTRA::NoExists;
    }

    const TPaxCtxt& toXML(xmlNodePtr node) const;
    TPaxCtxt& fromXML(xmlNodePtr node);
    TPaxCtxt& paxFromDB(TQuery &Qry);
};

class TCtxtItem : public TPaxCtxt, public TOriginCtxt
{
  public:
    void clear()
    {
      TPaxCtxt::clear();
      TOriginCtxt::clear();
    }
};

void ProcEvent(const TLogLocale &event,
               const AstraEdifact::TCtxtItem &ctxt,
               const xmlNodePtr eventCtxtNode,
               const bool repeated);

} //namespace AstraEdifact

#endif /*_EDI_UTILS_H_*/

