#ifndef _EDI_UTILS_H_
#define _EDI_UTILS_H_

#include "astra_misc.h"
#include "tlg/EdifactRequest.h"
#include <etick/tick_data.h>

namespace AstraEdifact
{

Ticketing::CouponStatus calcPaxCouponStatus(const std::string& refuse,
                                            bool pr_brd,
                                            bool in_final_status);

class TFltParams
{
  public:
    TTripInfo fltInfo;
    bool ets_no_interact, eds_no_interact, in_final_status;
    int pr_etstatus, et_final_attempt;
    void clear()
    {
      fltInfo.Clear();
      ets_no_interact=false;
      eds_no_interact=false;
      in_final_status=false;
      pr_etstatus=ASTRA::NoExists;
      et_final_attempt=ASTRA::NoExists;
    }
    bool get(int point_id);
};

Ticketing::TicketNum_t checkDocNum(const std::string& doc_no);

bool checkETSInteract(const TTripInfo& info,
                      bool with_exception);

bool checkEDSInteract(const TTripInfo& info,
                      bool with_exception);

bool checkETSInteract(int point_id,
                      bool with_exception,
                      TTripInfo& info);

bool checkEDSInteract(int point_id,
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
                        std::string &context);

void getEdiResponseCtxt(int ctxtId,
                        bool clear,
                        const std::string &where,
                        XMLDoc &xmlCtxt);

void getTermRequestCtxt(int ctxtId,
                        bool clear,
                        const std::string &where,
                        XMLDoc &xmlCtxt);

void getEdiSessionCtxt(int sessIda,
                       bool clear,
                       const std::string& where,
                       XMLDoc &xmlCtxt);

void getEdiSessionCtxt(const tlgnum_t& tlgNum,
                       bool clear,
                       const std::string& where,
                       XMLDoc &xmlCtxt);

void cleanOldRecords(int min_ago);

void ProcEdiError(const AstraLocale::LexemaData &error,
                  const xmlNodePtr errorCtxtNode,
                  bool isGlobal);
typedef std::list< std::pair<AstraLocale::LexemaData, bool> > EdiErrorList;

void GetEdiError(const xmlNodePtr errorCtxtNode, EdiErrorList &errors);

} //namespace AstraEdifact

#endif /*_EDI_UTILS_H_*/

