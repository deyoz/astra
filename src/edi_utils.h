#ifndef _EDI_UTILS_H_
#define _EDI_UTILS_H_

#include "astra_misc.h"
#include "tlg/CheckinBaseTypes.h"
#include "tlg/EdifactRequest.h"
#include <etick/tick_data.h>

namespace AstraEdifact
{

Ticketing::CouponStatus calcPaxCouponStatus(const std::string& refuse,
                                            const bool pr_brd,
                                            const bool in_final_status);

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

void checkDocNum(const std::string& doc_no);

bool checkETSInteract(const int point_id,
                      const bool with_exception,
                      TTripInfo& info);

bool checkEDSInteract(const int point_id,
                      const bool with_exception,
                      TTripInfo& info);

std::string getTripAirline(const TTripInfo& ti);

Ticketing::FlightNum_t getTripFlightNum(const TTripInfo& ti);

bool get_et_addr_set(const std::string &airline,
                     const int flt_no,
                     std::pair<std::string,std::string> &addrs);

bool get_et_addr_set(const std::string &airline,
                     const int flt_no,
                     std::pair<std::string,std::string> &addrs,
                     int &id);

std::string get_canon_name(const std::string& edi_addr);

void confirm_notify_levb(const int edi_sess_id);
std::string make_xml_kick(const edifact::KickInfo &kickInfo);

void cleanOldRecords(const int min_ago);

} //namespace AstraEdifact

#endif /*_EDI_UTILS_H_*/

