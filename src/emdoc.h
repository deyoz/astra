#ifndef _EMDOC_H_
#define _EMDOC_H_

#include <list>
#include "passenger.h"
#include "baggage.h"
#include "remarks.h"
#include "ticket_types.h"
#include "qrys.h"
#include "edi_utils.h"
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
void GetUnboundBagEMD(int point_id, std::multiset<CheckIn::TPaxASVCItem> &asvc);
bool ExistsUnboundBagEMD(int point_id);
bool ExistsPaxUnboundBagEMD(int pax_id);

}; //namespace PaxASVCList

class TEMDCoupon : public AstraEdifact::TCoupon
{
  public:
    Ticketing::CpnStatAction::CpnStatAction_t action;

    TEMDCoupon()
    {
      clear();
    }

    void clear()
    {
      AstraEdifact::TCoupon::clear();
      action=Ticketing::CpnStatAction::associate;
    }
};

class TEMDCtxtItem : public AstraEdifact::TCtxtItem
{
  public:
    TEMDCoupon emd;
    TEMDCtxtItem()
    {
      clear();
    }

    void clear()
    {
      TCtxtItem::clear();
      emd.clear();
    }

    const TEMDCtxtItem& toXML(xmlNodePtr node) const;
    TEMDCtxtItem& fromXML(xmlNodePtr node, xmlNodePtr originNode=NULL);
    std::string no_str() const;
};

void GetBagEMDDisassocList(const int point_id,
                           const bool in_final_status,
                           std::list< TEMDCtxtItem > &emds);

void GetEMDStatusList(const int grp_id,
                      const bool in_final_status,
                      const CheckIn::TServicePaymentList &prior_payment,
                      std::list<TEMDCtxtItem> &added_emds,
                      std::list<TEMDCtxtItem> &deleted_emds);

class TEMDocItem
{
  public:

    enum TEdiAction{ChangeOfStatus, SystemUpdate};

    TEMDCoupon emd;
    AstraEdifact::TCoupon et;
    std::string change_status_error, system_update_error;
    int point_id;
    TEMDocItem()
    {
      clear();
    }

    void clear()
    {
      emd.clear();
      et.clear();
      change_status_error.clear();
      system_update_error.clear();
      point_id=ASTRA::NoExists;
    }

    bool empty() const
    {
      return emd.empty();
    }

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
      return "SELECT pax_grp.point_dep, pax_grp.grp_id "
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
      return "SELECT pax_grp.point_dep, pax_grp.grp_id "
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

class EMDAutoBoundPointId : public EMDAutoBoundId
{
  public:
    int point_id;
    EMDAutoBoundPointId(int _point_id) : point_id(_point_id) {}
    EMDAutoBoundPointId(xmlNodePtr node) { fromXML(node); }
    void clear()
    {
      point_id=ASTRA::NoExists;
    }
    virtual const char* grpSQL() const
    {
      return "SELECT pax_grp.point_dep, pax_grp.grp_id "
             "FROM pax_grp, pax "
             "WHERE pax_grp.grp_id=pax.grp_id AND "
             "      pax_grp.point_dep=:point_id";
    }
    virtual const char* paxSQL() const
    {
      return "SELECT pax.grp_id, pax.pax_id, pax.refuse, "
             "       pax.ticket_no, pax.coupon_no, pax.ticket_rem, pax.ticket_confirm "
             "FROM pax_grp, pax "
             "WHERE pax_grp.grp_id=pax.grp_id AND "
             "      pax_grp.point_dep=:point_id";
    }
    virtual void setSQLParams(QParams &params) const
    {
      params.clear();
      params << (point_id!=ASTRA::NoExists?QParam("point_id", otInteger, point_id):
                                           QParam("point_id", otInteger, FNull));
    }
    virtual void fromXML(xmlNodePtr node)
    {
      clear();
      if (node==NULL) return;
      if (!NodeIsNULL("emd_auto_bound_point_id", node))
        point_id=NodeAsInteger("emd_auto_bound_point_id", node);
    }
    virtual void toXML(xmlNodePtr node) const
    {
      if (node==NULL) return;
      point_id!=ASTRA::NoExists?NewTextChild(node, "emd_auto_bound_point_id", point_id):
                                NewTextChild(node, "emd_auto_bound_point_id");
    }
    static bool exists(xmlNodePtr node)
    {
      if (node==NULL) return false;
      return GetNode("emd_auto_bound_point_id", node)!=NULL;
    }
};

void handleEmdDispResponse(const edifact::RemoteResults &remRes);

#endif
