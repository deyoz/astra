#ifndef _EMDOC_H_
#define _EMDOC_H_

#include <list>
#include "passenger.h"
//#include "baggage.h"
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
DbCpp::Session& GetSession(const TListType ltype);
std::string GetSQL(const TListType ltype);
void printSQLs();
int print_sql(int argc, char **argv);
void GetUnboundBagEMD(int point_id, std::multiset<CheckIn::TPaxASVCItem> &asvc);
void GetUnboundEMD(int id, std::multiset<CheckIn::TPaxASVCItem> &asvc, bool is_pax_id, bool only_one, bool bag);
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
    TEMDocItem& fromDB(const TEdiAction ediAction,
                       const std::string &_emd_no,
                       const int _emd_coupon,
                       const bool lock);
    void deleteDisplay() const;

private:
    const TEMDocItem& saveDisplay() const;
    const TEMDocItem& saveChangeOfStatus() const;
    const TEMDocItem& saveSystemUpdate() const;

    TEMDocItem& loadDisplay(const std::string& _emd_no,
                            int _emd_coupon,
                            bool lock);
    TEMDocItem& loadEmdocs(const std::string& _emd_no,
                           int _emd_coupon,
                           bool lock);
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
    const TPaxEMDItem& toDB(DB::TQuery &Qry) const;
    TPaxEMDItem& fromDB(DB::TQuery &Qry);
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
    void getAllEMD(const TCkinGrpIds &tckinGrpIds);
    void toDB() const;

};

void GetPaxEMD(int pax_id, std::multiset<TPaxEMDItem> &emds);

bool ActualEMDEvent(const TEMDCtxtItem &EMDCtxt,
                    const xmlNodePtr eventCtxtNode,
                    TLogLocale &event);

class EMDAutoBoundId
{
  private:
    mutable std::optional<PointId_t> pointId_;
    mutable std::optional<GrpIds> grpIds_;
    mutable std::optional< std::list<CheckIn::TCkinPaxTknItem> > paxList_;
    void loadGrpIds() const;
    void loadPaxList() const;

  protected:
    virtual const std::list<std::string> grpTables() const=0;
    virtual const char* grpSQL() const=0;
    virtual const std::list<std::string> paxTables() const=0;
    virtual const char* paxSQL() const=0;
    virtual void setSQLParams(QParams &params) const=0;

  public:
    const std::optional<PointId_t>& pointIdOpt() const;
    const GrpIds& grpIds() const;
    const std::list<CheckIn::TCkinPaxTknItem>& paxList() const;
    virtual std::set<PaxId_t> paxIdsWithSyncEmdsAlarm() const;
    virtual void toXML(xmlNodePtr node) const=0;
    virtual ~EMDAutoBoundId() {}
};

class EMDAutoBoundGrpId : public EMDAutoBoundId
{
  private:
    GrpId_t grpId_;

  protected:
    virtual const std::list<std::string> grpTables() const { return {"PAX_GRP"}; }
    virtual const char* grpSQL() const
    {
      return "SELECT pax_grp.point_dep, pax_grp.grp_id "
             "FROM pax_grp "
             "WHERE pax_grp.grp_id=:grp_id";
    }
    virtual const std::list<std::string> paxTables() const { return {"PAX"}; }
    virtual const char* paxSQL() const
    {
      return "SELECT pax.grp_id, pax.pax_id, pax.refuse, "
             "       pax.ticket_no, pax.coupon_no, pax.ticket_rem, pax.ticket_confirm "
             "FROM pax "
             "WHERE pax.grp_id=:grp_id AND pax.refuse IS NULL";
    }
    virtual void setSQLParams(QParams &params) const
    {
      params.clear();
      params << QParam("grp_id", otInteger, grpId_.get());
    }

  public:
    const GrpId_t& grpId() const { return grpId_; }

    EMDAutoBoundGrpId(const GrpId_t& grpId) : grpId_(grpId) {}
    EMDAutoBoundGrpId(xmlNodePtr node) :
      grpId_(NodeAsInteger("emd_auto_bound_grp_id", node)) {}

    virtual void toXML(xmlNodePtr node) const
    {
      if (node==NULL) return;
      NewTextChild(node, "emd_auto_bound_grp_id", grpId_.get());
    }
    static bool exists(xmlNodePtr node)
    {
      if (node==NULL) return false;
      return GetNode("emd_auto_bound_grp_id", node)!=NULL;
    }
};

class EMDAutoBoundRegNo : public EMDAutoBoundId
{
  private:
    PointId_t pointId_;
    RegNo_t regNo_;

  protected:
    virtual const std::list<std::string> grpTables() const { return {"PAX","PAX_GRP"}; }
    virtual const char* grpSQL() const
    {
      return "SELECT pax_grp.point_dep, pax_grp.grp_id "
             "FROM pax_grp, pax "
             "WHERE pax_grp.grp_id=pax.grp_id AND "
             "      pax_grp.point_dep=:point_id AND pax.reg_no=:reg_no";
    }
    virtual const std::list<std::string> paxTables() const { return {"PAX","PAX_GRP"}; }
    virtual const char* paxSQL() const
    {
      return "SELECT pax.grp_id, pax.pax_id, pax.refuse, "
             "       pax.ticket_no, pax.coupon_no, pax.ticket_rem, pax.ticket_confirm "
             "FROM pax_grp, pax "
             "WHERE pax_grp.grp_id=pax.grp_id AND "
             "      pax_grp.point_dep=:point_id AND pax.reg_no=:reg_no AND pax.refuse IS NULL";
    }
    virtual void setSQLParams(QParams &params) const
    {
      params.clear();
      params << QParam("point_id", otInteger, pointId_.get())
             << QParam("reg_no", otInteger, regNo_.get());
    }

  public:
    EMDAutoBoundRegNo(const PointId_t& pointId, const RegNo_t& regNo) :
      pointId_(pointId), regNo_(regNo) {}
    EMDAutoBoundRegNo(xmlNodePtr node) :
      pointId_(NodeAsInteger("emd_auto_bound_point_id", node)),
      regNo_(NodeAsInteger("emd_auto_bound_reg_no", node)) {}

    virtual void toXML(xmlNodePtr node) const
    {
      if (node==NULL) return;
      NewTextChild(node, "emd_auto_bound_point_id", pointId_.get());
      NewTextChild(node, "emd_auto_bound_reg_no", regNo_.get());
    }
    static bool exists(xmlNodePtr node)
    {
      if (node==NULL) return false;
      return GetNode("emd_auto_bound_reg_no", node)!=NULL;
    }
};

class EMDAutoBoundPointId : public EMDAutoBoundId
{
  private:
    PointId_t pointId_;

  protected:
    virtual const std::list<std::string> grpTables() const { return {"PAX","PAX_GRP"}; }
    virtual const char* grpSQL() const
    {
      return "SELECT pax_grp.point_dep, pax_grp.grp_id "
             "FROM pax_grp, pax "
             "WHERE pax_grp.grp_id=pax.grp_id AND "
             "      pax_grp.point_dep=:point_id";
    }
    virtual const std::list<std::string> paxTables() const { return {"PAX","PAX_GRP"}; }
    virtual const char* paxSQL() const
    {
      return "SELECT pax.grp_id, pax.pax_id, pax.refuse, "
             "       pax.ticket_no, pax.coupon_no, pax.ticket_rem, pax.ticket_confirm "
             "FROM pax_grp, pax "
             "WHERE pax_grp.grp_id=pax.grp_id AND "
             "      pax_grp.point_dep=:point_id AND pax.refuse IS NULL";
    }
    virtual void setSQLParams(QParams &params) const
    {
      params.clear();
      params << QParam("point_id", otInteger, pointId_.get());
    }

  public:
    EMDAutoBoundPointId(const PointId_t& pointId) : pointId_(pointId) {}
    EMDAutoBoundPointId(xmlNodePtr node) :
      pointId_(NodeAsInteger("emd_auto_bound_point_id", node)) {}

    virtual std::set<PaxId_t> paxIdsWithSyncEmdsAlarm() const;
    virtual void toXML(xmlNodePtr node) const
    {
      if (node==NULL) return;
      NewTextChild(node, "emd_auto_bound_point_id", pointId_.get());
    }
    static bool exists(xmlNodePtr node)
    {
      if (node==NULL) return false;
      return GetNode("emd_auto_bound_point_id", node)!=NULL;
    }
};

void handleEmdDispResponse(const edifact::RemoteResults &remRes);

#endif
