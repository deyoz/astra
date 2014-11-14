#include "emdoc.h"
#include "edi_utils.h"
#include "qrys.h"

#define NICKNAME "VLAD"
#define NICKTRACE SYSTEM_TRACE
#include "serverlib/test.h"

using namespace std;
using namespace BASIC;
using namespace Ticketing;
using namespace AstraEdifact;

namespace PaxASVCList
{

enum TListType {unboundByPointId, unboundByPaxId, allWithTknByPointId};

string GetSQL(const TListType ltype)
{
  ostringstream sql;
  sql << "SELECT a.rfic, \n"
         "       a.rfisc, \n"
         "       a.ssr_code, \n"
         "       a.service_name, \n"
         "       a.emd_type, \n"
         "       a.emd_no, \n"
         "       a.emd_coupon \n";
  if (ltype==allWithTknByPointId)
    sql << ",      a.grp_id, \n"
           "       a.pax_id, \n"
           "       a.surname, \n"
           "       a.name, \n"
           "       a.pers_type, \n"
           "       a.reg_no, \n"
           "       a.ticket_no, \n"
           "       a.coupon_no, \n"
           "       a.ticket_rem, \n"
           "       a.ticket_confirm, \n"
           "       a.pr_brd, \n"
           "       a.refuse, \n"
           "       b.grp_id AS paid_bag_emd_grp_id \n";
  sql << "FROM \n"
         " (SELECT pax_grp.grp_id, \n"
         "         pax_asvc.rfic, \n"
         "         pax_asvc.rfisc, \n"
         "         pax_asvc.ssr_code, \n"
         "         pax_asvc.service_name, \n"
         "         pax_asvc.emd_type, \n"
         "         pax_asvc.emd_no, \n"
         "         pax_asvc.emd_coupon \n";
  if (ltype==allWithTknByPointId)
    sql << ",        pax.pax_id, \n"
           "         pax.surname, \n"
           "         pax.name, \n"
           "         pax.pers_type, \n"
           "         pax.reg_no, \n"
           "         pax.ticket_no, \n"
           "         pax.coupon_no, \n"
           "         pax.ticket_rem, \n"
           "         pax.ticket_confirm, \n"
           "         pax.pr_brd, \n"
           "         pax.refuse \n";

  sql << "  FROM pax_grp, pax, pax_asvc \n"
         "  WHERE pax_grp.grp_id=pax.grp_id AND \n"
         "        pax.pax_id=pax_asvc.pax_id AND \n";
  if (ltype==unboundByPointId ||
      ltype==allWithTknByPointId)
    sql << "        pax_grp.point_dep=:id AND \n";
  if (ltype==unboundByPaxId)
    sql << "        pax.pax_id=:id AND \n";
  sql << "        pax_grp.status NOT IN ('E') AND \n"
         "        pax.refuse IS NULL) a, \n"
         " (SELECT pax_grp.grp_id, paid_bag_emd.emd_no, paid_bag_emd.emd_coupon \n";
  if (ltype==unboundByPointId ||
      ltype==allWithTknByPointId)
    sql << "  FROM pax_grp, paid_bag_emd \n"
           "  WHERE pax_grp.grp_id=paid_bag_emd.grp_id AND \n"
           "        pax_grp.point_dep=:id AND \n";
  if (ltype==unboundByPaxId)
    sql << "  FROM pax_grp, paid_bag_emd, pax \n"
           "  WHERE pax_grp.grp_id=paid_bag_emd.grp_id AND \n"
           "        pax.grp_id=pax_grp.grp_id AND \n"
           "        pax.pax_id=:id AND \n";
  sql << "        pax_grp.status NOT IN ('E')) b \n"
         "WHERE a.grp_id=b.grp_id(+) AND \n"
         "      a.emd_no=b.emd_no(+) AND \n"
         "      a.emd_coupon=b.emd_coupon(+) \n";
  if (ltype==unboundByPointId ||
      ltype==unboundByPaxId)
    sql << "AND b.grp_id IS NULL \n";
  //ProgTrace(TRACE5, "%s: SQL=\n %s", __FUNCTION__, sql.str().c_str());
  return sql.str();
}

void GetEMDDisassocList(const int point_id,
                        const bool in_final_status,
                        list< TEMDDisassocListItem > &assoc,
                        list< TEMDDisassocListItem > &disassoc)
{
  assoc.clear();
  disassoc.clear();  

  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText = PaxASVCList::GetSQL(PaxASVCList::allWithTknByPointId);
  Qry.CreateVariable( "id", otInteger, point_id );
  Qry.Execute();
  for(;!Qry.Eof;Qry.Next())
  {
    TEMDDisassocListItem item;
    item.asvc.fromDB(Qry);
    std::set<ASTRA::TRcptServiceType> service_types;
    item.asvc.rcpt_service_types(service_types);
    if (service_types.find(ASTRA::rstExcess)==service_types.end() &&
        service_types.find(ASTRA::rstPaid)==service_types.end()) continue;

    item.pax.id=Qry.FieldAsInteger("pax_id");
    item.pax.surname=Qry.FieldAsString("surname");
    item.pax.name=Qry.FieldAsString("name");
    item.pax.pers_type=DecodePerson(Qry.FieldAsString("pers_type"));
    item.pax.refuse=Qry.FieldAsString("refuse");
    item.pax.reg_no=Qry.FieldAsInteger("reg_no");
    item.pax.pr_brd=!Qry.FieldIsNULL("pr_brd") && Qry.FieldAsInteger("pr_brd")!=0;
    item.pax.tkn.fromDB(Qry);
    item.grp_id=Qry.FieldAsInteger("grp_id");

    item.status=calcPaxCouponStatus(item.pax.refuse,
                                    item.pax.pr_brd,
                                    in_final_status);
    if (item.status==CouponStatus::Flown) continue; //если финальный статус, то никаких ассоциаций

    if (item.status==CouponStatus::Boarded &&
        Qry.FieldIsNULL("paid_bag_emd_grp_id"))
      disassoc.push_back(item);
    else
      assoc.push_back(item);
  };
};


void GetUnboundEMD(int id, multiset<CheckIn::TPaxASVCItem> &asvc, bool is_pax_id, bool only_one)
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
        service_types.find(ASTRA::rstPaid)==service_types.end()) continue;
    asvc.insert(item);
    if (only_one) break;
  };
};

void GetUnboundEMD(int point_id, multiset<CheckIn::TPaxASVCItem> &asvc)
{
  GetUnboundEMD(point_id, asvc, false, false);
};

bool ExistsUnboundEMD(int point_id)
{
  multiset<CheckIn::TPaxASVCItem> asvc;
  GetUnboundEMD(point_id, asvc, false, true);
  return !asvc.empty();
};

bool ExistsPaxUnboundEMD(int pax_id)
{
  multiset<CheckIn::TPaxASVCItem> asvc;
  GetUnboundEMD(pax_id, asvc, true, true);
  return !asvc.empty();
};

void GetBoundPaidBagEMD(int grp_id, list< pair<CheckIn::TPaxASVCItem, CheckIn::TPaidBagEMDItem> > &emd)
{
  TQuery Qry(&OraSession);
  Qry.Clear();
    Qry.SQLText =
    "SELECT paid_bag_emd.bag_type, "
    "       paid_bag_emd.emd_no, "
    "       paid_bag_emd.emd_coupon, "
    "       paid_bag_emd.weight, "
    "       'C' AS rfic, "
    "       NULL AS rfisc, "
    "       NULL AS ssr_code, "
    "       NULL AS service_name, "
    "       'A' AS emd_type "
    "FROM paid_bag_emd "
    "WHERE paid_bag_emd.grp_id=:grp_id";
 /*
    "SELECT paid_bag_emd.bag_type, "
    "       paid_bag_emd.emd_no, "
    "       paid_bag_emd.emd_coupon, "
    "       paid_bag_emd.weight, "
    "       pax_asvc.rfic, "
    "       pax_asvc.rfisc, "
    "       pax_asvc.ssr_code, "
    "       pax_asvc.service_name, "
    "       pax_asvc.emd_type "
    "FROM paid_bag_emd, pax, pax_asvc "
    "WHERE paid_bag_emd.grp_id=pax.grp_id AND "
    "      pax.pax_id=pax_asvc.pax_id AND "
    "      paid_bag_emd.emd_no=pax_asvc.emd_no AND "
    "      paid_bag_emd.emd_coupon=pax_asvc.emd_coupon AND "
    "      paid_bag_emd.grp_id=:grp_id AND "
    "      pax.refuse IS NULL";*/
  Qry.CreateVariable("grp_id", otInteger, grp_id);
  Qry.Execute();
  for(;!Qry.Eof;Qry.Next())
  {
    CheckIn::TPaxASVCItem asvcItem;
    CheckIn::TPaidBagEMDItem emdItem;
    asvcItem.fromDB(Qry);
    emdItem.fromDB(Qry);
    std::set<ASTRA::TRcptServiceType> service_types;
    asvcItem.rcpt_service_types(service_types);
    if (service_types.find(ASTRA::rstExcess)==service_types.end() &&
        service_types.find(ASTRA::rstPaid)==service_types.end()) continue;
    emd.push_back(make_pair(asvcItem, emdItem));
  };
};

}; //namespace PaxASVCList

const TEMDocItem& TEMDocItem::toDB() const
{
  if (emd_no.empty() || emd_coupon==ASTRA::NoExists)
    throw EXCEPTIONS::Exception("TEMDocItem::toDB: empty emd");

  QParams QryParams;
  QryParams << QParam("doc_no", otString, emd_no)
            << QParam("coupon_no", otInteger, emd_coupon)
            << (status!=Ticketing::CouponStatus::Unavailable?QParam("coupon_status", otString, status->dispCode()):
                                                             QParam("coupon_status", otString, FNull))
            << QParam("error", otString, error.substr(0,100))
            << QParam("associated", otInteger, (int)(action==Ticketing::CpnStatAction::associate))
            << QParam("associated_no", otString, et_no)
            << QParam("associated_coupon", otInteger, et_coupon);

  const char* sql=
    "BEGIN "
    "  UPDATE emdocs "
    "  SET coupon_status=NVL(:coupon_status, coupon_status), "
    "      error=:error, associated=:associated, associated_no=:associated_no, associated_coupon=:associated_coupon "
    "  WHERE doc_no=:doc_no AND coupon_no=:coupon_no; "
    "  IF SQL%NOTFOUND THEN "
    "    INSERT INTO emdocs(doc_no, coupon_no, coupon_status, error, associated, associated_no, associated_coupon) "
    "    VALUES(:doc_no, :coupon_no, :coupon_status, :error, :associated, :associated_no, :associated_coupon); "
    "  END IF; "
    "END;";

  TCachedQuery Qry(sql, QryParams);
  Qry.get().Execute();

  return *this;
};

TEMDocItem& TEMDocItem::fromDB(const string &v_emd_no,
                               const int v_emd_coupon,
                               const bool lock)
{
  clear();

  if (v_emd_no.empty() || v_emd_coupon==ASTRA::NoExists)
    throw EXCEPTIONS::Exception("TEMDocItem::fromDB: empty emd");

  QParams QryParams;
  QryParams << QParam("doc_no", otString, v_emd_no);
  QryParams << QParam("coupon_no", otInteger, v_emd_coupon);

  const string sql="SELECT coupon_status, error, associated, associated_no, associated_coupon "
                   "FROM emdocs "
                   "WHERE doc_no=:doc_no AND coupon_no=:coupon_no";

  TCachedQuery Qry(sql+(lock?" FOR UPDATE":""), QryParams);

  Qry.get().Execute();
  if (!Qry.get().Eof)
  {
    et_no=Qry.get().FieldAsString("associated_no");
    et_coupon=Qry.get().FieldAsInteger("associated_coupon");

    emd_no=v_emd_no;
    emd_coupon=v_emd_coupon;

    status=Qry.get().FieldIsNULL("coupon_status")?Ticketing::CouponStatus(Ticketing::CouponStatus::Unavailable):
                                                  Ticketing::CouponStatus(CouponStatus::fromDispCode(Qry.get().FieldAsString("coupon_status")));
    error=Qry.get().FieldAsString("error");

    action=Qry.get().FieldAsInteger("associated")!=0?Ticketing::CpnStatAction::associate:
                                                     Ticketing::CpnStatAction::disassociate;
  };

  return *this;
};


