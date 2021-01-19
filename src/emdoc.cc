#include "emdoc.h"
#include "edi_utils.h"
#include "astra_utils.h"
#include "qrys.h"
#include "date_time.h"
#include "alarms.h"
#include <serverlib/dbcpp_cursctl.h>
#include "PgOraConfig.h"

#define NICKNAME "VLAD"
#define NICKTRACE SYSTEM_TRACE
#include <serverlib/slogger.h>

using namespace std;
using namespace BASIC::date_time;
using namespace ASTRA;
using namespace Ticketing;
using namespace AstraEdifact;

namespace PaxASVCList
{

std::string getPaxsSQL(const TListType ltype)
{
  ostringstream sql;

  for(int pass=0; pass<2; pass++)
  {
    if (pass==0)
      sql << "  (SELECT e.pax_id, e.transfer_num, e.emd_no, e.emd_coupon \n";
    else
      sql << "   SELECT e.pax_id, 0 AS transfer_num, e.emd_no, e.emd_coupon \n";
    if (ltype==unboundByPointId ||
        ltype==unboundByPaxId)
      sql << "        , pax.grp_id \n";
    if (ltype==allWithTknByPointId ||
        ltype==oneWithTknByGrpId ||
        ltype==oneWithTknByPaxId)
      sql << "        , pax.grp_id, pax.surname, pax.name, pax.pers_type, pax.reg_no, \n"
             "          pax.ticket_no, pax.coupon_no, pax.ticket_rem, pax.ticket_confirm, \n"
             "          pax.pr_brd, pax.refuse \n";
    if (ltype==unboundByPointId ||
        ltype==unboundByPaxId ||
        ltype==allWithTknByPointId)
    {
      if (pass==0)
      sql << "   FROM pax_grp, pax, pax_emd e \n";
      else
      sql << "   FROM pax_grp, pax, pax_asvc e \n";

      sql << "   WHERE pax_grp.grp_id=pax.grp_id AND \n"
             "         pax.pax_id=e.pax_id AND \n";
      if (ltype==unboundByPointId ||
          ltype==allWithTknByPointId)
      sql << "         pax_grp.point_dep=:id AND \n";
      if (ltype==unboundByPaxId)
      sql << "         pax.pax_id=:id AND \n";

      sql << "         pax_grp.status NOT IN ('E') AND \n"
             "         pax.refuse IS NULL \n";

    }

    if (ltype==allByPaxId)
    {
      if (pass==0)
      sql << "   FROM pax_emd e \n";
      else
      sql << "   FROM pax_asvc e \n";

      sql << "   WHERE pax_id=:id \n";
    }

    if (ltype==allByGrpId)
    {
      if (pass==0)
      sql << "   FROM pax, pax_emd e \n";
      else
      sql << "   FROM pax, pax_asvc e \n";

      sql << "   WHERE pax.pax_id=e.pax_id AND pax.grp_id=:id \n";
    }

    if (ltype==oneWithTknByGrpId ||
        ltype==oneWithTknByPaxId)
    {
      if (pass==0)
      sql << "   FROM pax, pax_emd e \n";
      else
      sql << "   FROM pax, pax_asvc e \n";

      sql << "   WHERE pax.pax_id=e.pax_id AND \n"
             "         e.emd_no=:emd_no AND e.emd_coupon=:emd_coupon AND \n";
      if (ltype==oneWithTknByGrpId)
      sql << "         pax.grp_id=:grp_id \n";
      if (ltype==oneWithTknByPaxId)
      sql << "         pax.pax_id=:pax_id \n";
    }

    if (pass==0)
      sql << "   UNION \n";
    else
      sql << "  ) paxs \n";
  }

  return sql.str();
}

string GetSQL(const TListType ltype)
{
  ostringstream sql;

  if (ltype==asvcByGrpIdWithoutEMD ||
      ltype==asvcByPaxIdWithoutEMD)
  {
    sql << "SELECT c.pax_id, \n"
           "       0 AS transfer_num, \n"
           "       c.emd_no, \n"
           "       c.emd_coupon, \n"
           "       c.rfic, \n"
           "       c.rfisc, \n"
           "       c.service_quantity, \n"
           "       c.ssr_code, \n"
           "       c.service_name, \n"
           "       c.emd_type, \n"
           "       c.emd_no AS emd_no_base \n"
           "FROM pax, pax_asvc c \n"
           "WHERE pax.pax_id=c.pax_id AND \n"
           "      c.emd_type IS NULL AND \n"
           "      c.emd_no IS NULL AND \n"
           "      c.emd_coupon IS NULL AND \n";
    if (ltype==asvcByGrpIdWithoutEMD)
      sql << "      pax.grp_id=:id \n";
    if (ltype==asvcByPaxIdWithoutEMD)
      sql << "      pax.pax_id=:id \n";
    sql << "UNION ALL \n";
  }
  if (ltype==asvcByGrpIdWithEMD ||
      ltype==asvcByPaxIdWithEMD ||
      ltype==asvcByGrpIdWithoutEMD ||
      ltype==asvcByPaxIdWithoutEMD)
  {
    sql << "SELECT p.pax_id, \n"
           "       tckin_pax_grp.seg_no-p.seg_no AS transfer_num, \n"
           "       c.emd_no, \n"
           "       c.emd_coupon, \n"
           "       c.rfic, \n"
           "       c.rfisc, \n"
           "       c.service_quantity, \n"
           "       c.ssr_code, \n"
           "       c.service_name, \n"
           "       c.emd_type, \n"
           "       c.emd_no AS emd_no_base \n"
           "FROM pax, tckin_pax_grp, pax_asvc c, \n"
           "     (SELECT tckin_pax_grp.tckin_id, \n"
           "             tckin_pax_grp.first_reg_no-pax.reg_no AS distance, \n"
           "             pax.pax_id, \n"
           "             tckin_pax_grp.seg_no \n"
           "      FROM pax, tckin_pax_grp \n"
           "      WHERE pax.grp_id=tckin_pax_grp.grp_id AND tckin_pax_grp.transit_num=0 AND \n";
    if (ltype==asvcByGrpIdWithEMD ||
        ltype==asvcByGrpIdWithoutEMD)
      sql << "      pax.grp_id=:id \n";
    if (ltype==asvcByPaxIdWithEMD ||
        ltype==asvcByPaxIdWithoutEMD)
      sql << "      pax.pax_id=:id \n";
    sql << "     ) p \n"
           "WHERE tckin_pax_grp.tckin_id=p.tckin_id AND \n"
           "      pax.grp_id=tckin_pax_grp.grp_id AND \n"
           "      tckin_pax_grp.first_reg_no-pax.reg_no=p.distance AND tckin_pax_grp.transit_num=0 AND \n"
           "      pax.pax_id=c.pax_id AND \n";
    if (ltype==asvcByGrpIdWithEMD ||
        ltype==asvcByPaxIdWithEMD)
      sql << "      c.emd_type IS NOT NULL AND \n"
             "      c.emd_no IS NOT NULL AND \n"
             "      c.emd_coupon IS NOT NULL AND \n"
             "      tckin_pax_grp.seg_no-p.seg_no>=0 \n";
    if (ltype==asvcByGrpIdWithoutEMD ||
        ltype==asvcByPaxIdWithoutEMD)
      sql << "      c.emd_type IS NULL AND \n"
             "      c.emd_no IS NULL AND \n"
             "      c.emd_coupon IS NULL AND \n"
             "      tckin_pax_grp.seg_no-p.seg_no>0 \n"; //>0 потому что UNION ALL!

    return sql.str();
  }

  sql << "SELECT paxs.*, \n";

  if (ltype==allWithTknByPointId)
  sql << "       d.grp_id AS service_payment_grp_id, \n";

  sql << "       CASE WHEN b.emd_no IS NOT NULL THEN b.rfic \n"
         "                                      ELSE c.rfic END AS rfic, \n"
         "       CASE WHEN b.emd_no IS NOT NULL THEN b.rfisc \n"
         "                                      ELSE c.rfisc END AS rfisc, \n"
         "       CASE WHEN b.emd_no IS NOT NULL THEN b.service_quantity \n"
         "                                      ELSE c.service_quantity END AS service_quantity, \n"
         "       CASE WHEN b.emd_no IS NOT NULL THEN b.ssr_code \n"
         "                                      ELSE c.ssr_code END AS ssr_code, \n"
         "       CASE WHEN b.emd_no IS NOT NULL THEN b.service_name \n"
         "                                      ELSE c.service_name END AS service_name, \n"
         "       CASE WHEN b.emd_no IS NOT NULL THEN b.emd_type \n"
         "                                      ELSE c.emd_type END AS emd_type, \n"
         "       CASE WHEN b.emd_no IS NOT NULL THEN b.emd_no_base \n"
         "                                      ELSE c.emd_no END AS emd_no_base \n"
         "FROM pax_emd b, pax_asvc c, \n";
  if (ltype==unboundByPointId ||
      ltype==unboundByPaxId ||
      ltype==allWithTknByPointId)
  {
    sql << "  (SELECT pax_grp.grp_id, service_payment.doc_no AS emd_no, service_payment.doc_coupon AS emd_coupon \n";

    if (ltype==unboundByPointId ||
        ltype==allWithTknByPointId)
    sql << "   FROM pax_grp, service_payment \n";
    if (ltype==unboundByPaxId)
    sql << "   FROM pax_grp, service_payment, pax \n";

    sql << "   WHERE pax_grp.grp_id=service_payment.grp_id AND \n";

    if (ltype==unboundByPointId ||
        ltype==allWithTknByPointId)
    sql << "   pax_grp.point_dep=:id AND \n";
    if (ltype==unboundByPaxId)
    sql << "   pax.grp_id=pax_grp.grp_id AND \n"
           "   pax.pax_id=:id AND \n";

    sql << "   service_payment.doc_type IN ('EMDA', 'EMDS') AND \n"
           "   pax_grp.status NOT IN ('E') \n"
           "  ) d, \n";
  }

  sql << getPaxsSQL(ltype);

  sql << "WHERE /*paxs.emd_no=      a.emd_no(+) AND \n"
         "      paxs.emd_coupon=  a.emd_coupon(+) AND*/ \n"

         "      paxs.pax_id=      b.pax_id(+) AND \n"
         "      paxs.transfer_num=b.transfer_num(+) AND \n"
         "      paxs.emd_no=      b.emd_no(+) AND \n"
         "      paxs.emd_coupon=  b.emd_coupon(+) AND \n"

         "      paxs.pax_id=      c.pax_id(+) AND \n"
         "      paxs.emd_no=      c.emd_no(+) AND \n"
         "      paxs.emd_coupon=  c.emd_coupon(+) AND \n"

         "      paxs.emd_no IS NOT NULL AND \n"
         "      paxs.emd_coupon IS NOT NULL AND \n"

         "      (/*a.emd_no IS NOT NULL OR*/ \n"
         "       b.emd_no IS NOT NULL OR \n"
         "       c.emd_no IS NOT NULL AND paxs.transfer_num=0) ";

  if (ltype==unboundByPointId ||
      ltype==unboundByPaxId ||
      ltype==allWithTknByPointId)
  {
  sql << "AND \n"
         "      paxs.grp_id=d.grp_id(+) AND \n"
         "      paxs.emd_no=d.emd_no(+) AND \n"
         "      paxs.emd_coupon=d.emd_coupon(+) ";
  if (ltype==unboundByPointId ||
      ltype==unboundByPaxId)
  sql << "AND \n"
         "      d.grp_id IS NULL ";
  }

  sql << "\n";

  return sql.str();
}

void printSQLs()
{
  ProgTrace(TRACE5, "%s: SQL(unboundByPointId)=\n%s", __FUNCTION__, GetSQL(unboundByPointId).c_str());
  ProgTrace(TRACE5, "%s: SQL(unboundByPaxId)=\n%s", __FUNCTION__, GetSQL(unboundByPaxId).c_str());
  ProgTrace(TRACE5, "%s: SQL(allByPaxId)=\n%s", __FUNCTION__, GetSQL(allByPaxId).c_str());
  ProgTrace(TRACE5, "%s: SQL(allByGrpId)=\n%s", __FUNCTION__, GetSQL(allByGrpId).c_str());
  ProgTrace(TRACE5, "%s: SQL(asvcByPaxIdWithEMD)=\n%s", __FUNCTION__, GetSQL(asvcByPaxIdWithEMD).c_str());
  ProgTrace(TRACE5, "%s: SQL(asvcByGrpIdWithEMD)=\n%s", __FUNCTION__, GetSQL(asvcByGrpIdWithEMD).c_str());
  ProgTrace(TRACE5, "%s: SQL(asvcByPaxIdWithoutEMD)=\n%s", __FUNCTION__, GetSQL(asvcByPaxIdWithoutEMD).c_str());
  ProgTrace(TRACE5, "%s: SQL(asvcByGrpIdWithoutEMD)=\n%s", __FUNCTION__, GetSQL(asvcByGrpIdWithoutEMD).c_str());
  ProgTrace(TRACE5, "%s: SQL(allWithTknByPointId)=\n%s", __FUNCTION__, GetSQL(allWithTknByPointId).c_str());
  ProgTrace(TRACE5, "%s: SQL(oneWithTknByGrpId)=\n%s", __FUNCTION__, GetSQL(oneWithTknByGrpId).c_str());
  ProgTrace(TRACE5, "%s: SQL(oneWithTknByPaxId)=\n%s", __FUNCTION__, GetSQL(oneWithTknByPaxId).c_str());
}

int print_sql(int argc, char **argv)
{
  printSQLs();
  return 1;
}

void GetUnboundBagEMD(int id, multiset<CheckIn::TPaxASVCItem> &asvc, bool is_pax_id, bool only_one)
{
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText = PaxASVCList::GetSQL(is_pax_id?PaxASVCList::unboundByPaxId:
                                              PaxASVCList::unboundByPointId);
  Qry.CreateVariable( "id", otInteger, id );
  Qry.Execute();
  for(;!Qry.Eof;Qry.Next())
  {
    CheckIn::TPaxASVCItem item;
    item.fromDB(Qry);
    std::set<ASTRA::TRcptServiceType> service_types;
    item.rcpt_service_types(service_types);
    if (service_types.find(ASTRA::rstExcess)==service_types.end() &&
        service_types.find(ASTRA::rstPaid)==service_types.end()) continue; //когда это уберется, тогда переименовать процедуру в GetUnboundEMD
    asvc.insert(item);
    if (only_one) break;
  };
}

void GetUnboundBagEMD(int point_id, multiset<CheckIn::TPaxASVCItem> &asvc)
{
  GetUnboundBagEMD(point_id, asvc, false, false);
}

bool ExistsUnboundBagEMD(int point_id)
{
  multiset<CheckIn::TPaxASVCItem> asvc;
  GetUnboundBagEMD(point_id, asvc, false, true);
  return !asvc.empty();
}

bool ExistsPaxUnboundBagEMD(int pax_id)
{
  multiset<CheckIn::TPaxASVCItem> asvc;
  GetUnboundBagEMD(pax_id, asvc, true, true);
  return !asvc.empty();
}

void getWithoutEMD(int id, TGrpServiceAutoList &svcsAuto, bool is_pax_id)
{
  svcsAuto.clear();
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText = PaxASVCList::GetSQL(is_pax_id?PaxASVCList::asvcByPaxIdWithoutEMD:
                                              PaxASVCList::asvcByGrpIdWithoutEMD);
  Qry.CreateVariable( "id", otInteger, id );
  Qry.Execute();
  for(;!Qry.Eof;Qry.Next())
    svcsAuto.push_back(TGrpServiceAutoItem().fromDB(Qry));
}

} //namespace PaxASVCList

const TEMDCtxtItem& TEMDCtxtItem::toXML(xmlNodePtr node) const
{
  if (node==NULL) return *this;

  TPaxCtxt::toXML(node);

  NewTextChild(node, "doc_no", emd.no);
  emd.coupon!=NoExists?NewTextChild(node, "coupon_no", emd.coupon):
                       NewTextChild(node, "coupon_no");
  emd.status!=CouponStatus::Unavailable?NewTextChild(node, "coupon_status", emd.status->dispCode()):
                                        NewTextChild(node, "coupon_status", FNull);
  NewTextChild(node, "associated", (int)(emd.action==CpnStatAction::associate));
  NewTextChild(node, "associated_no", pax.tkn.no);
  pax.tkn.coupon !=NoExists?NewTextChild(node, "associated_coupon", pax.tkn.coupon):
                            NewTextChild(node, "associated_coupon");
  return *this;
};

TEMDCtxtItem& TEMDCtxtItem::fromXML(xmlNodePtr node, xmlNodePtr originNode)
{
  clear();
  if (node==NULL) return *this;

  TPaxCtxt::fromXML(node);
  if (originNode!=NULL) TOriginCtxt::fromXML(originNode);

  xmlNodePtr node2=node->children;
  emd.no=NodeAsStringFast("doc_no", node2);
  emd.coupon=NodeAsIntegerFast("coupon_no", node2, NoExists);
  emd.action=NodeAsIntegerFast("associated", node2)!=0?CpnStatAction::associate:
                                                       CpnStatAction::disassociate;
  emd.status=NodeIsNULLFast("coupon_status", node2)?CouponStatus(CouponStatus::Unavailable):
                                                    CouponStatus(CouponStatus::fromDispCode(NodeAsStringFast("coupon_status", node2)));
  pax.tkn.no=NodeAsStringFast("associated_no", node2);
  pax.tkn.coupon=NodeAsIntegerFast("associated_coupon", node2, NoExists);
  return *this;
};

std::string TEMDCtxtItem::no_str() const
{
  ostringstream s;
  s << emd.no;
  if (emd.coupon!=ASTRA::NoExists)
    s << "/" << emd.coupon;
  else
    s << "/?";
  return s.str();
}

void GetBagEMDDisassocList(const int point_id,
                           const bool in_final_status,
                           std::list<TEMDCtxtItem> &emds)
{
  emds.clear();

  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText = PaxASVCList::GetSQL(PaxASVCList::allWithTknByPointId);
  Qry.CreateVariable( "id", otInteger, point_id );
  Qry.Execute();
  for(;!Qry.Eof;Qry.Next())
  {
    CheckIn::TPaxASVCItem asvc;
    asvc.fromDB(Qry);
    if (asvc.isEmdS()) continue;
    std::set<ASTRA::TRcptServiceType> service_types;
    asvc.rcpt_service_types(service_types);
    if (service_types.find(ASTRA::rstExcess)==service_types.end() &&
        service_types.find(ASTRA::rstPaid)==service_types.end()) continue; //когда это уберется, тогда переименовать процедуру в GetEMDDisassocList

    TEMDCtxtItem item;
    item.emd.no=asvc.emd_no;
    item.emd.coupon=asvc.emd_coupon;
    item.paxFromDB(Qry, false);
    item.point_id=point_id;

    item.emd.status=calcPaxCouponStatus(item.pax.refuse,
                                        item.pax.pr_brd,
                                        in_final_status);
    if (item.emd.status==CouponStatus::Flown) continue; //если финальный статус, то никаких ассоциаций

    if (item.emd.status==CouponStatus::Boarded &&
        Qry.FieldIsNULL("service_payment_grp_id"))
      item.emd.action=CpnStatAction::disassociate;
    else
      item.emd.action=CpnStatAction::associate;
    emds.push_back(item);
  };
};

void GetEMDStatusList(const int grp_id,
                      const bool in_final_status,
                      const CheckIn::TServicePaymentListWithAuto &prior_payment,
                      std::list<TEMDCtxtItem> &added_emds,
                      std::list<TEMDCtxtItem> &deleted_emds)
{
  added_emds.clear();
  deleted_emds.clear();

  if (in_final_status) return;

  CheckIn::TServicePaymentListWithAuto curr_payment;
  curr_payment.fromDB(grp_id);
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText= GetSQL(PaxASVCList::oneWithTknByGrpId);
  Qry.CreateVariable("grp_id", otInteger, grp_id);
  Qry.DeclareVariable("emd_no", otString);
  Qry.DeclareVariable("emd_coupon", otInteger);
  for(int pass=0; pass<2; pass++)
  {
    //1 проход - добавленные EMD
    //2 проход - удаленные EMD
    const CheckIn::TServicePaymentListWithAuto &payment1 = pass==0?curr_payment:prior_payment;
    const CheckIn::TServicePaymentListWithAuto &payment2 = pass==0?prior_payment:curr_payment;
    std::list<TEMDCtxtItem> &emds = pass==0?added_emds:deleted_emds;
    for(CheckIn::TServicePaymentListWithAuto::const_iterator p1=payment1.begin(); p1!=payment1.end(); ++p1)
    {
      CheckIn::TServicePaymentListWithAuto::const_iterator p2=payment2.begin();
      for(; p2!=payment2.end(); ++p2)
      {
        if (p1->sameDoc(*p2)) break;
      };
      if (p2!=payment2.end()) continue;
      if (!p1->isEMD()) continue; //работаем только с EMD
      if (p1->trfer_num!=0) continue; //работаем только со статусом 'нашего' сегмента

      TEMDCtxtItem item;
      item.emd.no=p1->doc_no;
      item.emd.coupon=p1->doc_coupon;

      Qry.SetVariable("emd_no", p1->doc_no);
      Qry.SetVariable("emd_coupon", p1->doc_coupon);
      Qry.Execute();
      if (!Qry.Eof)
      {
        //вообще когда одинаковые EMD принадлежат разным пассажиром - это плохо и наверное надо что-то с этим делать
        //пока выбираем пассажира с более ранним регистрационным номером
        item.paxFromDB(Qry, false);
      };

      emds.push_back(item);
    };
  };
};

TEMDocItem::TEMDocItem(const Emd& _emd,
                       const EmdCoupon& _emdCpn,
                       const std::set<std::string> &connected_emd_no)
{
  clear();
  RFIC=_emd.rfic()->code();
  RFISC=_emdCpn.rfisc()?_emdCpn.rfisc().get().rfisc():"";
  service_name=_emdCpn.rfisc()?_emdCpn.rfisc().get().description():"";
  if (service_name.empty()) service_name=RFISC;
  emd_type=(_emd.type()==DocType::EmdA ? "A" : "S");
  if (RFIC=="C" &&
      _emdCpn.itin().luggage().haveLuggage() &&
      _emdCpn.itin().luggage()->chargeQualifier()==Ticketing::Baggage::NumPieces &&
      _emdCpn.itin().luggage()->quantity()>0)
    service_quantity=_emdCpn.quantity() * _emdCpn.itin().luggage()->quantity();
  else
    service_quantity=_emdCpn.quantity();

  if(!_emdCpn.tickNum().empty())
  {
    emd.no=_emdCpn.tickNum().get();
    if (_emdCpn.num())
      emd.coupon=_emdCpn.num().get();
  }
  if(!_emdCpn.associatedTickNum().empty())
  {
    et.no=_emdCpn.associatedTickNum().get();
    if (_emdCpn.associatedNum())
      et.coupon=_emdCpn.associatedNum().get();
  }

  emd_no_base=connected_emd_no.empty()?emd.no:*(connected_emd_no.begin());
}

static void bindDisplayItem(DbCpp::CursCtl& cur,
                            const TEMDocItem& item)
{
  short notNull = 0;
  short null = -1;
  cur.stb()
      .bind(":doc_no", item.emd.no)
      .bind(":coupon_no", item.emd.coupon)
      .bind(":rfic", "", &null)
      .bind(":rfisc", "", &null)
      .bind(":service_quantity", 0, &null)
      .bind(":ssr_code", "", &null)
      .bind(":service_name", "", &null)
      .bind(":emd_type", "", &null)
      .bind(":emd_no_base", item.emd_no_base)
      .bind(":et_no", item.et.no)
      .bind(":et_coupon", item.et.coupon,
            item.et.coupon != ASTRA::NoExists ? &notNull : &null)
      .bind(":last_display", DateTimeToBoost(NowUTC()));
}

static bool updateDisplay(const TEMDocItem& item)
{
  LogTrace(TRACE5) << __func__
                   << ": emd_no=" << item.emd.no
                   << ", coupon_no=" << item.emd.coupon;
  auto cur = make_db_curs(
        "UPDATE emdocs_display "
        "SET rfic=:rfic, "
        "    rfisc=:rfisc, "
        "    service_quantity=:service_quantity, "
        "    ssr_code=:ssr_code, "
        "    service_name=:service_name, "
        "    emd_type=:emd_type, "
        "    emd_no_base=:emd_no_base, "
        "    et_no=:et_no, "
        "    et_coupon=:et_coupon, "
        "    last_display=:last_display "
        "WHERE emd_no=:doc_no "
        "AND emd_coupon=:coupon_no ",
        PgOra::getRWSession("EMDOCS_DISPLAY"));
  bindDisplayItem(cur, item);

  LogTrace(TRACE5) << __func__
                   << ": rowcount=" << cur.rowcount();
  return cur.rowcount() > 0;
}

static bool insertDisplay(const TEMDocItem& item)
{
  LogTrace(TRACE5) << __func__
                   << ": ticket_no=" << item.emd.no
                   << ", coupon_no=" << item.emd.coupon;
  auto cur = make_db_curs(
        "INSERT INTO emdocs_display( "
        "emd_no, emd_coupon, rfic, rfisc, service_quantity, ssr_code, "
        "service_name, emd_type, emd_no_base, et_no, et_coupon, last_display, "
        ") VALUES ( "
        ":doc_no, :coupon_no, :rfic, :rfisc, :service_quantity, :ssr_code, "
        ":service_name, :emd_type, :emd_no_base, :et_no, :et_coupon, :last_display "
        ") ",
        PgOra::getRWSession("EMDOCS_DISPLAY"));
  bindDisplayItem(cur, item);
  cur.exec();

  LogTrace(TRACE5) << __func__
                   << ": rowcount=" << cur.rowcount();
  return cur.rowcount() > 0;
}

const TEMDocItem& TEMDocItem::saveDisplay() const
{
  LogTrace(TRACE5) << __func__
                   << ": emd_no=" << emd.no
                   << ", coupon_no=" << emd.coupon;
  const bool updated = updateDisplay(*this);
  if (not updated) {
    insertDisplay(*this);
  }
  return *this;
}

static void bindChangeOfStatusItem(DbCpp::CursCtl& cur,
                                   const TEMDocItem& item)
{
  short notNull = 0;
  short null = -1;
  cur.stb()
      .bind(":point_id", item.point_id)
      .bind(":coupon_status",
            item.emd.status!=CouponStatus::Unavailable ? item.emd.status->dispCode()
                                                       : "",
            item.emd.status!=CouponStatus::Unavailable ? &notNull : &null)
      .bind(":change_status_error", item.change_status_error.substr(0,100))
      .bind(":doc_no", item.emd.no)

      .bind(":coupon_no", item.emd.coupon);
}

static bool updateChangeOfStatus(const TEMDocItem& item)
{
  LogTrace(TRACE5) << __func__
                   << ": emd_no=" << item.emd.no
                   << ", coupon_no=" << item.emd.coupon;
  auto cur = make_db_curs(
        "UPDATE emdocs "
        "SET change_status_error=:change_status_error, "
        "    coupon_status=:coupon_status, "
        "    point_id=:point_id "
        "WHERE doc_no=:doc_no "
        "AND coupon_no=:coupon_no ",
        PgOra::getRWSession("EMDOCS"));
  bindChangeOfStatusItem(cur, item);
  cur.exec();

  LogTrace(TRACE5) << __func__
                   << ": rowcount=" << cur.rowcount();
  return cur.rowcount() > 0;
}

static bool insertChangeOfStatus(const TEMDocItem& item)
{
  LogTrace(TRACE5) << __func__
                   << ": ticket_no=" << item.emd.no
                   << ", coupon_no=" << item.emd.coupon;
  auto cur = make_db_curs(
        "INSERT INTO emdocs( "
        "doc_no, coupon_no, coupon_status, change_status_error, associated, point_id "
        ") VALUES ( "
        ":doc_no, :coupon_no, :coupon_status, :change_status_error, 1, :point_id "
        ") ",
        PgOra::getRWSession("EMDOCS"));
  bindChangeOfStatusItem(cur, item);
  cur.exec();

  LogTrace(TRACE5) << __func__
                   << ": rowcount=" << cur.rowcount();
  return cur.rowcount() > 0;
}

const TEMDocItem& TEMDocItem::saveChangeOfStatus() const
{
  LogTrace(TRACE5) << __func__
                   << ": emd_no=" << emd.no
                   << ", coupon_no=" << emd.coupon;
  const bool updated = updateChangeOfStatus(*this);
  if (not updated) {
    insertChangeOfStatus(*this);
  }
  return *this;
}

static void bindSystemUpdateItem(DbCpp::CursCtl& cur,
                                   const TEMDocItem& item)
{
  cur.stb()
      .bind(":point_id", item.point_id)
      .bind(":associated", int(item.emd.action==CpnStatAction::associate))
      .bind(":associated_no", item.et.no)
      .bind(":associated_coupon", item.et.coupon)
      .bind(":system_update_error", item.change_status_error.substr(0,100))
      .bind(":doc_no", item.emd.no)
      .bind(":coupon_no", item.emd.coupon);
}

static bool updateSystemUpdate(const TEMDocItem& item)
{
  LogTrace(TRACE5) << __func__
                   << ": emd_no=" << item.emd.no
                   << ", coupon_no=" << item.emd.coupon;
  auto cur = make_db_curs(
        "UPDATE emdocs "
        "SET system_update_error=:system_update_error, "
        "    associated=:associated, "
        "    associated_no=:associated_no, "
        "    associated_coupon=:associated_coupon, "
        "    point_id=:point_id "
        "WHERE doc_no=:doc_no "
        "AND coupon_no=:coupon_no ",
        PgOra::getRWSession("EMDOCS"));
  bindSystemUpdateItem(cur, item);
  cur.exec();

  LogTrace(TRACE5) << __func__
                   << ": rowcount=" << cur.rowcount();
  return cur.rowcount() > 0;
}

static bool insertSystemUpdate(const TEMDocItem& item)
{
  LogTrace(TRACE5) << __func__
                   << ": ticket_no=" << item.emd.no
                   << ", coupon_no=" << item.emd.coupon;
  auto cur = make_db_curs(
        "INSERT INTO emdocs( "
        "doc_no, coupon_no, system_update_error, associated, associated_no, associated_coupon, point_id "
        ") VALUES( "
        ":doc_no, :coupon_no, :system_update_error, :associated, :associated_no, :associated_coupon, :point_id "
        ") ",
        PgOra::getRWSession("EMDOCS"));
  bindSystemUpdateItem(cur, item);
  cur.exec();

  LogTrace(TRACE5) << __func__
                   << ": rowcount=" << cur.rowcount();
  return cur.rowcount() > 0;
}

const TEMDocItem& TEMDocItem::saveSystemUpdate() const
{
  LogTrace(TRACE5) << __func__
                   << ": emd_no=" << emd.no
                   << ", coupon_no=" << emd.coupon;
  const bool updated = updateSystemUpdate(*this);
  if (not updated) {
    insertSystemUpdate(*this);
  }
  return *this;
}

bool deleteEmdocs(int point_id)
{
  LogTrace(TRACE5) << __func__
                   << ": point_id=" << point_id;
  auto cur = make_db_curs(
        "DELETE FROM emdocs "
        "WHERE point_id=:point_id ",
        PgOra::getRWSession("EMDOCS"));
  cur.stb()
      .bind(":point_id", point_id)
      .exec();

  LogTrace(TRACE5) << __func__
                   << ": rowcount=" << cur.rowcount();
  return cur.rowcount() > 0;
}

const TEMDocItem& TEMDocItem::toDB(const TEdiAction ediAction) const
{
  if (emd.empty())
    throw EXCEPTIONS::Exception("TEMDocItem::toDB: empty emd");

  switch(ediAction)
  {
    case Display:
      return saveDisplay();
    case ChangeOfStatus:
      return saveChangeOfStatus();
    case SystemUpdate:
      return saveSystemUpdate();
  }

  return *this;
}

TEMDocItem& TEMDocItem::loadDisplay(const std::string& _emd_no,
                                    int _emd_coupon,
                                    bool lock)
{
  LogTrace(TRACE5) << __func__
                   << ": emd_no=" << _emd_no
                   << ", coupon_no=" << _emd_coupon;
  clear();
  Dates::DateTime_t last_display;
  auto cur = make_db_curs(
        "SELECT "
        "emd_coupon, emd_no, emd_no_base, emd_type, et_coupon, et_no, "
        "last_display, rfic, rfisc, service_name, service_quantity, ssr_code "
        "FROM emdocs_display "
        "WHERE emd_no=:doc_no "
        "AND emd_coupon=:coupon_no "
        + std::string(lock ? " FOR UPDATE" : ""),
        lock ? PgOra::getRWSession("EMDOCS_DISPLAY")
             : PgOra::getROSession("EMDOCS_DISPLAY"));
  cur.stb()
      .def(emd.coupon)
      .def(emd.no)
      .def(emd_no_base)
      .def(emd_type)
      .defNull(et.coupon, ASTRA::NoExists)
      .defNull(et.no, "")
      .def(last_display)
      .def(RFIC)
      .def(RFISC)
      .def(service_name)
      .defNull(service_quantity, 0)
      .defNull(ssr_code, "")
      .bind(":doc_no", _emd_no)
      .bind(":coupon_no", _emd_coupon)
      .EXfet();

  LogTrace(TRACE5) << __func__
                   << ": found="
                   << (cur.err() != DbCpp::ResultCode::NoDataFound);
  return *this;
}

TEMDocItem& TEMDocItem::loadEmdocs(const std::string& _emd_no,
                                   int _emd_coupon,
                                   bool lock)
{
  LogTrace(TRACE5) << __func__
                   << ": emd_no=" << _emd_no
                   << ", coupon_no=" << _emd_coupon;
  clear();
  int associated = 0;
  std::string coupon_status;
  auto cur = make_db_curs(
        "SELECT "
        "doc_no AS emd_no, coupon_no AS emd_coupon, "
        "coupon_status, associated, associated_no, associated_coupon, "
        "change_status_error, system_update_error, point_id "
        "FROM emdocs "
        "WHERE doc_no=:doc_no "
        "AND coupon_no=:coupon_no "
        + std::string(lock ? " FOR UPDATE" : ""),
        lock ? PgOra::getRWSession("EMDOCS")
             : PgOra::getROSession("EMDOCS"));
  cur.stb()
      .def(emd.coupon)
      .def(emd.no)
      .defNull(coupon_status, CouponStatus(CouponStatus::Unavailable)->dispCode())
      .def(associated)
      .defNull(et.no, "")
      .defNull(et.coupon, ASTRA::NoExists)
      .defNull(change_status_error, "")
      .defNull(system_update_error, "")
      .def(point_id)
      .bind(":doc_no", _emd_no)
      .bind(":coupon_no", _emd_coupon)
      .EXfet();

  if (cur.err() != DbCpp::ResultCode::NoDataFound) {
    emd.action=associated!=0?CpnStatAction::associate:
                             CpnStatAction::disassociate;
    emd.status = CouponStatus(CouponStatus::fromDispCode(coupon_status));
  }

  LogTrace(TRACE5) << __func__
                   << ": found="
                   << (cur.err() != DbCpp::ResultCode::NoDataFound);
  return *this;
}

TEMDocItem& TEMDocItem::fromDB(const TEdiAction ediAction,
                               const string &_emd_no,
                               const int _emd_coupon,
                               const bool lock)
{
  clear();

  if (_emd_no.empty() || _emd_coupon==ASTRA::NoExists) {
    throw EXCEPTIONS::Exception("TEMDocItem::fromDB: empty emd");
  }

  if (ediAction==Display) {
    return loadDisplay(_emd_no, _emd_coupon, lock);
  } else {
    return loadEmdocs(_emd_no, _emd_coupon, lock);
  }

  return *this;
}

void TEMDocItem::deleteDisplay() const
{
  LogTrace(TRACE5) << __func__
                   << ": emd_no=" << emd.no
                   << ", coupon_no=" << emd.coupon;
  auto cur = make_db_curs(
        "DELETE FROM emdocs_display "
        "WHERE emd_no=:emd_no "
        "AND emd_coupon=:emd_coupon ",
        PgOra::getRWSession("EMDOCS_DISPLAY"));
  cur.stb()
      .bind(":emd_no", emd.no)
      .bind(":coupon_no", emd.coupon)
      .exec();

  LogTrace(TRACE5) << __func__
                   << ": rowcount=" << cur.rowcount();
}

bool ActualEMDEvent(const TEMDCtxtItem &EMDCtxt,
                    const xmlNodePtr eventCtxtNode,
                    TLogLocale &event)
{
  event.clear();

  if (EMDCtxt.paxUnknown() || eventCtxtNode==NULL) return false;

  TLogLocale ctxt_event;
  ctxt_event.fromXML(eventCtxtNode);

  if (ctxt_event.ev_time==NoExists || ctxt_event.ev_order==NoExists) return false;


  QParams QryParams;
  QryParams << QParam("pax_id", otInteger, EMDCtxt.pax.id)
            << QParam("emd_no", otString, EMDCtxt.emd.no)
            << QParam("emd_coupon", otInteger, EMDCtxt.emd.coupon);

  TCachedQuery Qry(PaxASVCList::GetSQL(PaxASVCList::oneWithTknByPaxId), QryParams);
  Qry.get().Execute();
  if (Qry.get().Eof) return false;

  TEMDCtxtItem EMDCtxtActual(EMDCtxt);
  EMDCtxtActual.paxFromDB(Qry.get(), false);

  if (EMDCtxtActual.pax.grp_id==EMDCtxt.pax.grp_id &&
      EMDCtxtActual.pax.reg_no==EMDCtxt.pax.reg_no) return false;

  QParams DelQryParams;
  DelQryParams << QParam("ev_time", otDate, ctxt_event.ev_time)
               << QParam("ev_order", otInteger, ctxt_event.ev_order);

  TCachedQuery DelQry("DELETE FROM events_bilingual WHERE time=:ev_time AND ev_order=:ev_order", DelQryParams);
  DelQry.get().Execute();

  event.fromXML(eventCtxtNode);
  event.ev_type=ASTRA::evtPay;
  event.id1=EMDCtxtActual.point_id;
  event.id2=EMDCtxtActual.pax.reg_no;
  event.id3=EMDCtxtActual.pax.grp_id;
  return true;
};

#include "astra_emd.h"
#include "tlg/emd_edifact.h"

const TPaxEMDItem& TPaxEMDItem::toDB(TQuery &Qry) const
{
  CheckIn::TPaxASVCItem::toDB(Qry);
  Qry.SetVariable("pax_id", pax_id);
  Qry.SetVariable("transfer_num", trfer_num);
  Qry.SetVariable("emd_no_base", emd_no_base);
  return *this;
}

TPaxEMDItem& TPaxEMDItem::fromDB(TQuery &Qry)
{
  clear();
  CheckIn::TPaxASVCItem::fromDB(Qry);
  pax_id=Qry.FieldAsInteger("pax_id");
  trfer_num=Qry.FieldAsInteger("transfer_num");
  emd_no_base=Qry.FieldAsString("emd_no_base");
  return *this;
}

std::string TPaxEMDItem::traceStr() const
{
  std::ostringstream s;
  s << RFIC << " | "
    << RFISC << " | "
    << service_name << " | "
    << emd_type << " | "
    << emd_no;
  if (emd_coupon!=NoExists)
    s << "/" << emd_coupon;
  s << " | " << emd_no_base;
  return s.str();
}

bool TPaxEMDItem::valid() const
{
  return (!RFIC.empty() &&
          !RFISC.empty() &&
          !service_name.empty() &&
          !emd_type.empty() &&
          !emd_no.empty() &&
          emd_coupon!=NoExists &&
          !emd_no_base.empty());
}

void TPaxEMDList::getPaxEMD(int id, PaxASVCList::TListType listType, bool doNotClear)
{
  if (!doNotClear) clear();
  if (!(listType==PaxASVCList::allByGrpId ||
        listType==PaxASVCList::allByPaxId ||
        listType==PaxASVCList::asvcByGrpIdWithEMD||
        listType==PaxASVCList::asvcByPaxIdWithEMD)) return;

  TCachedQuery Qry(PaxASVCList::GetSQL(listType),
                   QParams() << QParam("id", otInteger, id));
  Qry.get().Execute();
  for(; !Qry.get().Eof; Qry.get().Next())
    insert(TPaxEMDItem().fromDB(Qry.get()));
}

void TPaxEMDList::getAllPaxEMD(int pax_id, bool singleSegment)
{
  clear();
  getPaxEMD(pax_id, PaxASVCList::allByPaxId, true);
  if (!singleSegment)
    //pax_asvc содержит только текущий сегмент,
    //поэтому при сквозной регистрации надо начитать asvc по всем сквозным сегментам
    getPaxEMD(pax_id, PaxASVCList::asvcByPaxIdWithEMD, true);
}

void TPaxEMDList::getAllEMD(const TCkinGrpIds &tckin_grp_ids)
{
  clear();
  if (tckin_grp_ids.empty()) return;
  getPaxEMD(tckin_grp_ids.front(), PaxASVCList::allByGrpId, true);
  if (tckin_grp_ids.size()>1)
    //pax_asvc содержит только текущий сегмент,
    //поэтому при сквозной регистрации надо начитать asvc по всем сквозным сегментам
    getPaxEMD(tckin_grp_ids.front(), PaxASVCList::asvcByGrpIdWithEMD, true);
}

void TPaxEMDList::toDB() const
{
  TCachedQuery Qry(
    "INSERT INTO pax_emd(pax_id, transfer_num, rfic, rfisc, service_quantity, ssr_code, service_name, emd_type, "
    "  emd_no, emd_coupon, emd_no_base) "
    "VALUES(:pax_id, :transfer_num, :rfic, :rfisc, :service_quantity, :ssr_code, :service_name, :emd_type, "
    "  :emd_no, :emd_coupon, :emd_no_base)",
    QParams() << QParam("pax_id", otInteger)
              << QParam("transfer_num", otInteger)
              << QParam("rfic", otString)
              << QParam("rfisc", otString)
              << QParam("service_quantity", otInteger)
              << QParam("ssr_code", otString)
              << QParam("service_name", otString)
              << QParam("emd_type", otString)
              << QParam("emd_no", otString)
              << QParam("emd_coupon", otInteger)
              << QParam("emd_no_base", otString));
  set<int> paxIds;
  for(const TPaxEMDItem& emd : *this)
  {
    emd.toDB(Qry.get());
    Qry.get().Execute();
    paxIds.insert(emd.pax_id);
  }

  for(const int& paxId : paxIds)
  {
    addAlarmByPaxId(paxId, {Alarm::SyncEmds}, {paxCheckIn});
    TPaxAlarmHook::set(Alarm::UnboundEMD, paxId);
  }
}

void SyncPaxEMD(const CheckIn::TTransferItem &trfer,
                const TEMDocItem &emdItem,
                const CouponStatus &emd_status,
                const set<string> &connected_et_no)
{
  TPaxEMDItem emd(emdItem);

  ProgTrace(TRACE5, "%s: %s %s->%s: %s %s",
                    __FUNCTION__,
                    trfer.operFlt.scd_out==NoExists?"??.??.??":
                                                    DateTimeToStr(trfer.operFlt.scd_out, "dd.mm.yy").c_str(),
                    trfer.operFlt.airp.c_str(),
                    trfer.airp_arv.c_str(),
                    emd.traceStr().c_str(),
                    emd_status->dispCode());

  if (trfer.operFlt.airp.empty() ||
      trfer.operFlt.scd_out==NoExists ||
      trfer.airp_arv.empty()) return;
  if (!emd.valid()) return;

  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText="DELETE FROM pax_emd WHERE emd_no=:emd_no AND emd_coupon=:emd_coupon";
  Qry.CreateVariable("emd_no", otString, emd.emd_no);
  Qry.CreateVariable("emd_coupon", otInteger, emd.emd_coupon);
  Qry.Execute();

  if (emd_status!=CouponStatus::OriginalIssue &&
      emd_status!=CouponStatus::Checked &&
      emd_status!=CouponStatus::Boarded &&
      emd_status!=CouponStatus::Flown &&
      emd_status!=CouponStatus::Notification) return;

  set< pair<int/*grp_id*/, int/*pax_id*/> > ids;

  Qry.Clear();
  Qry.SQLText="SELECT grp_id, pax_id FROM pax WHERE ticket_no=:et_no";
  Qry.DeclareVariable("et_no", otString);
  for(set<string>::const_iterator no=connected_et_no.begin(); no!=connected_et_no.end(); ++no)
  {
    Qry.SetVariable("et_no", *no);
    Qry.Execute();
    for(; !Qry.Eof; Qry.Next())
    {
      int grp_id=Qry.FieldAsInteger("grp_id");
      int pax_id=Qry.FieldAsInteger("pax_id");
      ids.insert(make_pair(grp_id, pax_id));
      map<int, CheckIn::TCkinPaxTknItem> tkns;
      CheckIn::GetTCkinTickets(pax_id, tkns);
      for(map<int, CheckIn::TCkinPaxTknItem>::const_iterator i=tkns.begin(); i!=tkns.end(); ++i)
        ids.insert(make_pair(i->second.grp_id, i->second.pax_id));
    };
  };

  for(set< pair<int/*grp_id*/, int/*pax_id*/> >::const_iterator i=ids.begin(); i!=ids.end(); ++i)
  {
    try
    {
      TTrferRoute route;
      route.GetRoute(i->first, trtWithFirstSeg);
      int trfer_num=0;
      for(TTrferRoute::iterator t=route.begin(); t!=route.end(); ++t, trfer_num++)
      {
        modf(t->operFlt.scd_out, &(t->operFlt.scd_out));
        if (t->operFlt.airp==trfer.operFlt.airp &&
            t->operFlt.scd_out==trfer.operFlt.scd_out &&
            t->airp_arv==trfer.airp_arv)
        {
          emd.pax_id=i->second;;
          emd.trfer_num=trfer_num;
          TPaxEMDList(emd).toDB();
          break;
        };
      }
    }
    catch(std::exception &e)
    {
      ProgError(STDLOG, "%s: %s", __FUNCTION__, e.what());
    };

  };
}

/*
const std::string amadeus_emd=
    "UNB+IATA:1+ETICK+ASTRA+171213:1125+ASTRA00EZ50001+++T'"
    "UNH+1+TKCRES:06:1:IA+ASTRA00EZ5'"
    "MSG+:791+3'"
    "TIF+SIDOROV:A+SIDOR MR'"
    "TAI+7906+1005RE/SU:B'"
    "RCI+1A:S4XL4U:1+UT:6K6126:1'"
    "MON+B:4000:RUB+T:4000:RUB'"
    "FOP+CA:3:4000'"
    "PTK+++131217'"
    "ORG+1A:MUC+00040655:021648+MOW++T+RU+A1005RESU'"
    "EQN+1:TD'"
    "IFT+4:39+MOSCOW+VIP CORPORATE TRAVEL'"
    "IFT+4:5+9652600172'"
    "IFT+4:15:1+SGC UT MOW UT GOJ4000RUB4000END'"
    "IFT+4:47+BAGGAGE'"
    "IFT+4:733:1'"
    "PTS+++++C'"
    "TKT+2982903207956:J:1'"
    "CPN+1:I::E'"
    "TVL++SGC+VKO+UT'"
    "PTS++++++0GP'"
    "IFT+4:47+PIECE OF BAG UPTO23KG 203LCM'"
    "CPN+2:I::E'"
    "TVL++VKO+GOJ+UT'"
    "PTS++++++0GP'"
    "IFT+4:47+PIECE OF BAG UPTO23KG 203LCM'"
    "TKT+2982903207956:J::4::2981085213369'"
    "CPN+1:::::::1::702'"
    "PTS'"
    "CPN+2:::::::2::702'"
    "PTS'"
    "UNT+31+1'"
    "UNZ+1+ASTRA00EZ50001'";
*/

boost::optional<Itin> getEtDispCouponItin(const Ticketing::TicketNum_t& ticknum,
                                          const Ticketing::CouponNum_t& cpnnum);

void handleEmdDispResponse(const edifact::RemoteResults& remRes)
{
  std::list<Emd> emdList = EmdEdifactReader::readList(remRes.tlgSource());
  for(list<Emd>::const_iterator e=emdList.begin(); e!=emdList.end(); ++e)
  {
    const Emd &emd=*e;
    set<string> connected_et_no;
    set<string> connected_emd_no;
    for(int pass=0; pass<2; pass++)
    {
      //1 проход: набираем connected_et_no
      //2 проход: привязываем через SyncPaxEMD
      for(list<EmdTicket>::const_iterator t=emd.lTicket().begin(); t!=emd.lTicket().end(); ++t)
      {
        const EmdTicket &emdTick=*t;
        for(list<EmdCoupon>::const_iterator c=emdTick.lCpn().begin(); c!=emdTick.lCpn().end(); ++c)
        {
          const EmdCoupon &emdCpn=*c;
          if(!emdCpn.haveItin()) continue; //не имеем данных о сегменте

          if (pass==0)
          {
            if(!emdCpn.tickNum().empty())
              connected_emd_no.insert(emdCpn.tickNum().get());
            if(!emdCpn.associatedTickNum().empty())
              connected_et_no.insert(emdCpn.associatedTickNum().get());
          }
          else
          {
            CheckIn::TTransferItem trferItem;
            trferItem.operFlt.airline=ElemToElemId(etAirline, emdCpn.itin().airCode(), trferItem.operFlt.airline_fmt);
            trferItem.operFlt.flt_no=emdCpn.itin().flightnum();
            trferItem.operFlt.airp=ElemToElemId(etAirp, emdCpn.itin().depPointCode(), trferItem.operFlt.airp_fmt);
            if (!emdCpn.itin().date1().is_special())
              trferItem.operFlt.scd_out=BoostToDateTime(emdCpn.itin().date1());
            else
            {
              if (!emdCpn.associatedTickNum().empty() && emdCpn.associatedNum())
              {
                boost::optional<Itin> etDispItin=getEtDispCouponItin(emdCpn.associatedTickNum(), emdCpn.associatedNum());
                if (!etDispItin) LogTrace(TRACE5) << __FUNCTION__
                                                  << ": etDispItin==boost::none"
                                                  << ", emdCpn.associatedTickNum()=" << emdCpn.associatedTickNum()
                                                  << ", emdCpn.associatedNum()=" << emdCpn.associatedNum();

                if (etDispItin && !etDispItin.get().date1().is_special())
                  trferItem.operFlt.scd_out=BoostToDateTime(etDispItin.get().date1());
              }
            }
            trferItem.airp_arv=ElemToElemId(etAirp, emdCpn.itin().arrPointCode(), trferItem.airp_arv_fmt);

            TEMDocItem emdItem(emd,
                               emdCpn,
                               connected_emd_no);
            if (emdItem.validDisplay())
              emdItem.toDB(TEMDocItem::Display);

            SyncPaxEMD(trferItem, emdItem, emdCpn.couponInfo().status(), connected_et_no);
          }
        }
      }
    }
  }
}


