#ifndef _EMDOC_H_
#define _EMDOC_H_

#include <list>
#include "passenger.h"
#include "baggage.h"
#include "remarks.h"
#include "ticket_types.h"
#include "qrys.h"
#include "etick/tick_data.h"
#include "tlg/remote_results.h"

namespace PaxASVCList
{

enum TListType {unboundByPointId,
                unboundByPaxId,
                allByPaxId,
                allWithTknByPointId,
                oneWithTknByGrpId,
                oneWithTknByPaxId};
std::string GetSQL(const TListType ltype);
void printSQLs();
void GetUnboundEMD(int point_id, std::multiset<CheckIn::TPaxASVCItem> &asvc);
bool ExistsUnboundEMD(int point_id);
bool ExistsPaxUnboundEMD(int pax_id);

void GetBoundPaidBagEMD(int grp_id, int trfer_num, CheckIn::PaidBagEMDList &emd);

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

void GetEMDStatusList(const int grp_id,
                      const bool in_final_status,
                      const CheckIn::PaidBagEMDList &priorBoundEMDs,
                      std::list<TEMDCtxtItem> &added_emds,
                      std::list<TEMDCtxtItem> &deleted_emds);

class TEMDocItem
{
  public:

    enum TEdiAction{ChangeOfStatus, SystemUpdate};

    std::string emd_no, et_no;
    int emd_coupon, et_coupon;
    Ticketing::CouponStatus status;
    Ticketing::CpnStatAction::CpnStatAction_t action;
    std::string change_status_error, system_update_error;
    int point_id;
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
      point_id=ASTRA::NoExists;
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
    TEMDocItem& fromDB(const std::string &_emd_no,
                       const int _emd_coupon,
                       const bool lock);
};

class TPaxEMDItem : public CheckIn::TPaxASVCItem
{
  public:
    std::string et_no;
    int et_coupon;
    int trfer_num;
    std::string emd_no_base;

    TPaxEMDItem()
    {
      clear();
    }
    void clear()
    {
      CheckIn::TPaxASVCItem::clear();
      et_no.clear();
      et_coupon=ASTRA::NoExists;
      trfer_num=ASTRA::NoExists;
      emd_no_base.clear();
    }
    const TPaxEMDItem& toDB(TQuery &Qry) const;
    TPaxEMDItem& fromDB(TQuery &Qry);
    std::string traceStr() const;
    bool valid() const;
};

void GetPaxEMD(int pax_id, std::multiset<TPaxEMDItem> &emds);

void ProcEdiEvent(const TLogLocale &event,
                  const TEdiCtxtItem &ctxt,
                  const xmlNodePtr eventCtxtNode,
                  const bool repeated);

bool ActualEMDEvent(const TEMDCtxtItem &EMDCtxt,
                    const xmlNodePtr eventCtxtNode,
                    TLogLocale &event);

class EMDAutoBoundId
{
  public:
    virtual const char* grpSQL() const=0;
    virtual const char* paxSQL() const=0;
    virtual void setSQLParams(QParams &params) const=0;
    virtual void fromXML(xmlNodePtr node)=0;
    virtual void toXML(xmlNodePtr node) const=0;
    virtual ~EMDAutoBoundId() {};
};

class EMDAutoBoundGrpId : public EMDAutoBoundId
{
  public:
    int grp_id;
    EMDAutoBoundGrpId(int _grp_id) : grp_id(_grp_id) {}
    EMDAutoBoundGrpId(xmlNodePtr node) { fromXML(node); }
    void clear()
    {
      grp_id=ASTRA::NoExists;
    }
    virtual const char* grpSQL() const
    {
      return "SELECT pax_grp.point_dep, pax_grp.grp_id, pax_grp.piece_concept "
             "FROM pax_grp "
             "WHERE pax_grp.grp_id=:grp_id";
    }
    virtual const char* paxSQL() const
    {
      return "SELECT pax.grp_id, pax.pax_id, pax.refuse, "
             "       pax.ticket_no, pax.coupon_no, pax.ticket_rem, pax.ticket_confirm "
             "FROM pax "
             "WHERE pax.grp_id=:grp_id";
    }
    virtual void setSQLParams(QParams &params) const
    {
      params.clear();
      params << (grp_id!=ASTRA::NoExists?QParam("grp_id", otInteger, grp_id):
                                         QParam("grp_id", otInteger, FNull));
    }
    virtual void fromXML(xmlNodePtr node)
    {
      clear();
      if (node==NULL) return;
      if (!NodeIsNULL("emd_auto_bound_grp_id", node))
        grp_id=NodeAsInteger("emd_auto_bound_grp_id", node);
    }
    virtual void toXML(xmlNodePtr node) const
    {
      if (node==NULL) return;
      grp_id!=ASTRA::NoExists?NewTextChild(node, "emd_auto_bound_grp_id", grp_id):
                              NewTextChild(node, "emd_auto_bound_grp_id");
    }
    static bool exists(xmlNodePtr node)
    {
      if (node==NULL) return false;
      return GetNode("emd_auto_bound_grp_id", node)!=NULL;
    }
};

class EMDAutoBoundRegNo : public EMDAutoBoundId
{
  public:
    int point_id;
    int reg_no;
    EMDAutoBoundRegNo(int _point_id, int _reg_no) : point_id(_point_id), reg_no(_reg_no) {}
    EMDAutoBoundRegNo(xmlNodePtr node) { fromXML(node); }
    void clear()
    {
      point_id=ASTRA::NoExists;
      reg_no=ASTRA::NoExists;
    }
    virtual const char* grpSQL() const
    {
      return "SELECT pax_grp.point_dep, pax_grp.grp_id, pax_grp.piece_concept "
             "FROM pax_grp, pax "
             "WHERE pax_grp.grp_id=pax.grp_id AND "
             "      pax_grp.point_dep=:point_id AND pax.reg_no=:reg_no";
    }
    virtual const char* paxSQL() const
    {
      return "SELECT pax.grp_id, pax.pax_id, pax.refuse, "
             "       pax.ticket_no, pax.coupon_no, pax.ticket_rem, pax.ticket_confirm "
             "FROM pax_grp, pax "
             "WHERE pax_grp.grp_id=pax.grp_id AND "
             "      pax_grp.point_dep=:point_id AND pax.reg_no=:reg_no";
    }
    virtual void setSQLParams(QParams &params) const
    {
      params.clear();
      params << (point_id!=ASTRA::NoExists?QParam("point_id", otInteger, point_id):
                                           QParam("point_id", otInteger, FNull));
      params << (reg_no  !=ASTRA::NoExists?QParam("reg_no", otInteger, reg_no):
                                           QParam("reg_no", otInteger, FNull));
    }
    virtual void fromXML(xmlNodePtr node)
    {
      clear();
      if (node==NULL) return;
      if (!NodeIsNULL("emd_auto_bound_point_id", node))
        point_id=NodeAsInteger("emd_auto_bound_point_id", node);
      if (!NodeIsNULL("emd_auto_bound_reg_no", node))
        reg_no=NodeAsInteger("emd_auto_bound_reg_no", node);
    }
    virtual void toXML(xmlNodePtr node) const
    {
      if (node==NULL) return;
      point_id!=ASTRA::NoExists?NewTextChild(node, "emd_auto_bound_point_id", point_id):
                                NewTextChild(node, "emd_auto_bound_point_id");
      reg_no  !=ASTRA::NoExists?NewTextChild(node, "emd_auto_bound_reg_no", reg_no):
                                NewTextChild(node, "emd_auto_bound_reg_no");
    }
    static bool exists(xmlNodePtr node)
    {
      if (node==NULL) return false;
      return GetNode("emd_auto_bound_reg_no", node)!=NULL;
    }
};

void handleEmdDispResponse(const edifact::RemoteResults &remRes);

#endif
