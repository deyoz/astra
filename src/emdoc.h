#ifndef _EMDOC_H_
#define _EMDOC_H_

#include <list>
#include "passenger.h"
#include "baggage.h"
#include "remarks.h"
#include "ticket_types.h"
#include "etick/tick_data.h"

namespace CheckIn
{
typedef std::list< std::pair<TPaxASVCItem, TPaidBagEMDItem> > PaidBagEMDList;
}; //namespace PaxASVCList

namespace PaxASVCList
{

void GetUnboundEMD(int point_id, std::multiset<CheckIn::TPaxASVCItem> &asvc);
bool ExistsUnboundEMD(int point_id);
bool ExistsPaxUnboundEMD(int pax_id);

void GetBoundPaidBagEMD(int grp_id, CheckIn::PaidBagEMDList &emd);

}; //namespace PaxASVCList

class TEdiOriginCtxt
{
  public:
    std::string screen;
    std::string user_descr;
    std::string desk_code;
    TEdiOriginCtxt()
    {
      clear();
    }

    void clear()
    {
      screen.clear();
      user_descr.clear();
      desk_code.clear();
    }

    static void toXML(xmlNodePtr node);
    TEdiOriginCtxt& fromXML(xmlNodePtr node);
};

class TEdiPaxCtxt
{
  public:
    CheckIn::TPaxItem pax;
    int point_id, grp_id;
    std::string flight;

    TEdiPaxCtxt()
    {
      clear();
    }

    void clear()
    {
      pax.clear();
      point_id=ASTRA::NoExists;
      grp_id=ASTRA::NoExists;
      flight.clear();
    }

    bool paxUnknown() const
    {
      return pax.id==ASTRA::NoExists;
    }

    const TEdiPaxCtxt& toXML(xmlNodePtr node) const;
    TEdiPaxCtxt& fromXML(xmlNodePtr node);
    TEdiPaxCtxt& paxFromDB(TQuery &Qry);
};

class TEdiCtxtItem : public TEdiPaxCtxt, public TEdiOriginCtxt {};

class TEMDCtxtItem : public TEdiCtxtItem
{
  public:
    CheckIn::TPaxASVCItem asvc;    
    Ticketing::CouponStatus status;
    Ticketing::CpnStatAction::CpnStatAction_t action;    
    TEMDCtxtItem()
    {
      clear();
    }    

    void clear()
    {
      TEdiPaxCtxt::clear();
      TEdiOriginCtxt::clear();
      asvc.clear();      
      status=Ticketing::CouponStatus(Ticketing::CouponStatus::Unavailable);
      action=Ticketing::CpnStatAction::associate;      
    }    

    const TEMDCtxtItem& toXML(xmlNodePtr node) const;
    TEMDCtxtItem& fromXML(xmlNodePtr node, xmlNodePtr originNode=NULL);
};

void GetEMDDisassocList(const int point_id,
                        const bool in_final_status,
                        std::list< TEMDCtxtItem > &emds);

void GetBoundEMDStatusList(const int grp_id,
                           const bool in_final_status,
                           std::list<TEMDCtxtItem> &emds);

class TEMDocItem
{  
  public:

    enum TEdiAction{ChangeOfStatus, SystemUpdate};

    std::string emd_no, et_no;
    int emd_coupon, et_coupon;
    Ticketing::CouponStatus status;
    Ticketing::CpnStatAction::CpnStatAction_t action;
    std::string change_status_error, system_update_error;
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
      action=Ticketing::CpnStatAction::associate;
      change_status_error.clear();
      system_update_error.clear();
    };

    bool empty() const
    {
      return emd_no.empty() || emd_coupon==ASTRA::NoExists;
    };

    std::string emd_no_str() const
    {
      std::ostringstream s;
      s << emd_no;
      if (emd_coupon!=ASTRA::NoExists)
        s << "/" << emd_coupon;
      return s.str();
    };

    const TEMDocItem& toDB(const TEdiAction ediAction) const;
    TEMDocItem& fromDB(const std::string &v_emd_no,
                       const int v_emd_coupon,
                       const bool lock);
};

void ProcEdiError(const AstraLocale::LexemaData &error,
                  const xmlNodePtr errorCtxtNode,
                  const bool isGlobal);
typedef std::list< std::pair<AstraLocale::LexemaData, bool> > EdiErrorList;

void GetEdiError(const xmlNodePtr errorCtxtNode,
                 EdiErrorList &errors);

void ProcEdiEvent(const TLogLocale &event,
                  const TEdiCtxtItem &ctxt,
                  const xmlNodePtr eventCtxtNode,
                  const bool repeated);

bool ActualEMDEvent(const TEMDCtxtItem &EMDCtxt,
                    const xmlNodePtr eventCtxtNode,
                    TLogLocale &event);

#endif
