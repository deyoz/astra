#ifndef _EMDOC_H_
#define _EMDOC_H_

#include <list>
#include "passenger.h"
#include "baggage.h"
#include "remarks.h"
#include "ticket_types.h"
#include "etick/tick_data.h"

namespace PaxASVCList
{

void GetUnboundEMD(int point_id, std::multiset<CheckIn::TPaxASVCItem> &asvc);
bool ExistsUnboundEMD(int point_id);
bool ExistsPaxUnboundEMD(int pax_id);

void GetBoundPaidBagEMD(int grp_id, std::list< std::pair<CheckIn::TPaxASVCItem, CheckIn::TPaidBagEMDItem> > &emd);


class TEMDDisassocListItem
{
  public:    
    CheckIn::TPaxASVCItem asvc;
    CheckIn::TPaxItem pax;
    Ticketing::CouponStatus status;
    int grp_id;
    TEMDDisassocListItem() :
      grp_id(ASTRA::NoExists) {};
};

void GetEMDDisassocList(const int point_id,
                        const bool in_final_status,
                        std::list< TEMDDisassocListItem > &assoc,
                        std::list< TEMDDisassocListItem > &disassoc);

}; //namespace PaxASVCList

class TEMDocItem
{
  public:
    std::string emd_no, et_no;
    int emd_coupon, et_coupon;
    Ticketing::CouponStatus status;
    Ticketing::CpnStatAction::CpnStatAction_t action;
    std::string error;
    TEMDocItem()
    {
      clear();
    };

    void clear()
    {
      emd_no.clear();
      et_no.clear();
      emd_coupon=ASTRA::NoExists;
      et_coupon=ASTRA::NoExists;
      status=Ticketing::CouponStatus(Ticketing::CouponStatus::Unavailable);
      error.clear();
      action=Ticketing::CpnStatAction::associate;
    };

    bool empty() const
    {
      return emd_no.empty() || emd_coupon==ASTRA::NoExists;
    };

    const TEMDocItem& toDB() const;
    TEMDocItem& fromDB(const std::string &v_emd_no,
                       const int v_emd_coupon,
                       const bool lock);
};

#endif
