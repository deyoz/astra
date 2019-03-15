#ifndef _EMDOC_H_
#define _EMDOC_H_

#include <list>
#include "passenger.h"
#include "baggage.h"
#include "remarks.h"
#include "ticket_types.h"
#include "qrys.h"
#include "edi_utils.h"
#include "astra_emd.h"
#include "etick/tick_data.h"
#include "tlg/remote_results.h"

namespace PaxASVCList
{

enum TListType {unboundByPointId,
                unboundByPaxId,
                allByPaxId,
                allByGrpId,
                asvcByPaxIdWithEMD,
                asvcByGrpIdWithEMD,
                asvcByPaxIdWithoutEMD,
                asvcByGrpIdWithoutEMD,
                allWithTknByPointId,
                oneWithTknByGrpId,
                oneWithTknByPaxId};
std::string GetSQL(const TListType ltype);
std::string getPaxsSQL(const TListType ltype);
void printSQLs();
int print_sql(int argc, char **argv);
void GetUnboundBagEMD(int point_id, std::multiset<CheckIn::TPaxASVCItem> &asvc);
bool ExistsUnboundBagEMD(int point_id);
bool ExistsPaxUnboundBagEMD(int pax_id);
void getWithoutEMD(int id, TGrpServiceAutoList &svcsAuto, bool is_pax_id);

} //namespace PaxASVCList

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

    TEMDCoupon& fromDB(TQuery &Qry);
    const TEMDCoupon& toDB(TQuery &Qry) const;
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
                      const CheckIn::TServicePaymentListWithAuto &prior_payment,
                      std::list<TEMDCtxtItem> &added_emds,
                      std::list<TEMDCtxtItem> &deleted_emds);

class TEMDocItem : public CheckIn::TServiceBasic
{
  public:

    enum TEdiAction{Display, ChangeOfStatus, SystemUpdate};

    TEMDCoupon emd;
    std::string emd_no_base;
    AstraEdifact::TCoupon et;
    std::string change_status_error, system_update_error;
    int point_id;
    TEMDocItem()
    {
      clear();
    }

    TEMDocItem(const Ticketing::Emd& _emd,
               const Ticketing::EmdCoupon& _emdCpn,
               const std::set<std::string>& connected_emd_no);

    void clear()
    {
      CheckIn::TServiceBasic::clear();
      emd.clear();
      emd_no_base.clear();
      et.clear();
      change_status_error.clear();
      system_update_error.clear();
      point_id=ASTRA::NoExists;
    }

    bool empty() const
    {
      return emd.empty();
    }

    bool validDisplay() const
    {
      return !RFIC.empty() &&
             !RFISC.empty() &&
             !service_name.empty() &&
             !emd_type.empty() &&
             !emd.empty() &&
             !emd_no_base.empty();
    }

    const TEMDocItem& toDB(const TEdiAction ediAction) const;
    TEMDocItem& fromDB(const TEdiAction ediAction, TQuery &Qry);
    TEMDocItem& fromDB(const TEdiAction ediAction,
                       const std::string &_emd_no,
                       const int _emd_coupon,
                       const bool lock);
    void deleteDisplay() const;
};

class TEMDocList : public std::list<TEMDocItem>
{
};

class TPaxEMDItem : public CheckIn::TPaxASVCItem
{
  public:
    int pax_id;
    int trfer_num;
    std::string emd_no_base;

    TPaxEMDItem()
    {
      clear();
    }

    TPaxEMDItem(const TEMDocItem& emdItem)
    {
      clear();
      TServiceBasic::operator = (emdItem);
      emd_no=emdItem.emd.no;
      emd_coupon=emdItem.emd.coupon;
      emd_no_base=emdItem.emd_no_base;
    }

    TPaxEMDItem(const CheckIn::TPaxASVCItem& asvcItem)
    {
      clear();
      CheckIn::TPaxASVCItem::operator = (asvcItem);
    }

    void clear()
    {
      CheckIn::TPaxASVCItem::clear();
      pax_id=ASTRA::NoExists;
      trfer_num=ASTRA::NoExists;
      emd_no_base.clear();
    }
    const TPaxEMDItem& toDB(TQuery &Qry) const;
    TPaxEMDItem& fromDB(TQuery &Qry);
    std::string traceStr() const;
    bool valid() const;
};

class TPaxEMDList : public std::set<TPaxEMDItem>
{
  public:
    TPaxEMDList() {}
    TPaxEMDList(const TPaxEMDItem& emd) { insert(emd); }
    void getPaxEMD(int id, PaxASVCList::TListType listType, bool doNotClear=false);
    void getAllPaxEMD(int pax_id, bool singleSegment);
    void getAllEMD(const TCkinGrpIds &tckin_grp_ids);
    void toDB() const;

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
