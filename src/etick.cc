#include "etick.h"
#include <string>
#include "xml_unit.h"
#include "tlg/edi_tlg.h"
#include "astra_ticket.h"
#include "etick_change_status.h"
#include "astra_tick_view_xml.h"
#include "astra_emd_view_xml.h"
#include "astra_tick_read_edi.h"
#include "exceptions.h"
#include "environ.h"
#include "astra_locale.h"
#include "astra_consts.h"
#include "astra_misc.h"
#include "astra_context.h"
#include "base_tables.h"
#include "checkin.h"
#include "web_main.h"
#include "term_version.h"
#include "misc.h"
#include "qrys.h"
#include "request_dup.h"
#include <jxtlib/jxtlib.h>
#include <jxtlib/xml_stuff.h>
#include <serverlib/query_runner.h>
#include <edilib/edi_func_cpp.h>
#include <serverlib/testmode.h>

#include "emdoc.h"
#include "astra_pnr.h"
#include "astra_emd.h"
#include "astra_emd_view_xml.h"
#include "edi_utils.h"
#include "points.h"
#include "brd.h"
#include "astra_elems.h"
#include "rfisc_calc.h"
#include "service_eval.h"
#include "AirportControl.h"
#include "passenger.h"
#include "ckin_search.h"
#include "zamar_dsm.h"
#include "flt_settings.h"
#include "tlg/et_disp_request.h"
#include "tlg/emd_disp_request.h"
#include "tlg/emd_system_update_request.h"
#include "tlg/emd_cos_request.h"
#include "tlg/emd_edifact.h"
#include "tlg/et_rac_request.h"
#include "tlg/remote_results.h"
#include "tlg/remote_system_context.h"
#include "tlg/postpone_edifact.h"
#include "cuws_main.h"
#include <serverlib/dbcpp_cursctl.h>
#include "PgOraConfig.h"
#include "hooked_session.h"

#define NICKNAME "VLAD"
#define NICKTRACE SYSTEM_TRACE
#include <serverlib/slogger.h>


using namespace std;
using namespace Ticketing;
using namespace edilib;
using namespace Ticketing::TickReader;
using namespace Ticketing::TickView;
using namespace Ticketing::TickMng;
using namespace Ticketing::ChangeStatus;
using namespace Ticketing::RemoteSystemContext;
using namespace boost::gregorian;
using namespace boost::posix_time;
using namespace ASTRA;
using namespace AstraLocale;
using namespace BASIC::date_time;
using namespace EXCEPTIONS;
using namespace AstraEdifact;

#define MAX_TICKETS_IN_TLG 5

namespace PaxETList
{

bool isDisplayedEt(const std::string& ticket_no, int coupon_no)
{
  LogTrace(TRACE5) << __func__
                   << ": ticket_no=" << ticket_no
                   << ", coupon_no=" << coupon_no;
  auto cur = make_db_curs(
        "SELECT 1 FROM eticks_display "
        "WHERE ticket_no=:ticket_no "
        "AND coupon_no=:coupon_no ",
        PgOra::getROSession("ETICKS_DISPLAY"));
  cur.stb()
      .bind(":ticket_no", ticket_no)
      .bind(":coupon_no", coupon_no)
      .EXfet();

  return cur.err() != DbCpp::ResultCode::NoDataFound;
}

std::string GetSQL(const TListType ltype)
{
  ostringstream sql;

  if (ltype==notDisplayedByPointIdTlg ||
      ltype==notDisplayedByPaxIdTlg)
  {
    sql << "SELECT tlg_binding.point_id_spp AS point_id, crs_pax_tkn.ticket_no, crs_pax_tkn.coupon_no, \n"
           "       tlg_trips.airline, tlg_trips.flt_no, tlg_trips.airp_dep \n"
           "FROM crs_pax_tkn, crs_pax, crs_pnr, tlg_trips, tlg_binding \n"
           "WHERE crs_pax_tkn.pax_id=crs_pax.pax_id AND \n"
           "      crs_pax.pnr_id=crs_pnr.pnr_id AND \n"
           "      crs_pnr.point_id=tlg_binding.point_id_tlg AND \n"
           "      crs_pnr.point_id=tlg_trips.point_id AND \n"
           "      tlg_binding.point_id_tlg=:point_id_tlg AND \n";
    if (ltype==notDisplayedByPointIdTlg) {
      sql << "      tlg_binding.point_id_spp=:id AND \n";
    }
    if (ltype==notDisplayedByPaxIdTlg) {
      sql << "      crs_pax.pax_id=:id AND \n";
    }
  };

  if (ltype==allByPointIdAndTickNoFromTlg ||
      ltype==allByTickNoAndCouponNoFromTlg)
  {
    sql << "SELECT crs_pax_tkn.ticket_no, crs_pax_tkn.coupon_no, \n"
           "       tlg_binding.point_id_spp AS point_id, \n"
           "       crs_pax.* \n"
           "FROM crs_pax_tkn, crs_pax, crs_pnr, tlg_binding \n"
           "WHERE crs_pax_tkn.pax_id=crs_pax.pax_id AND \n"
           "      crs_pax.pnr_id=crs_pnr.pnr_id AND \n"
           "      crs_pnr.point_id=tlg_binding.point_id_tlg AND \n";
    if (ltype==allByPointIdAndTickNoFromTlg) {
      sql <<
           "      tlg_binding.point_id_spp=:point_id AND \n"
           "      crs_pax_tkn.ticket_no=:ticket_no AND \n";
    }
    if (ltype==allByTickNoAndCouponNoFromTlg) {
      sql <<
           "      crs_pax_tkn.ticket_no=:ticket_no AND \n"
           "      crs_pax_tkn.coupon_no=:coupon_no AND \n";
    }
  }

  if (ltype==notDisplayedByPointIdTlg ||
      ltype==notDisplayedByPaxIdTlg ||
      ltype==allByPointIdAndTickNoFromTlg ||
      ltype==allByTickNoAndCouponNoFromTlg)
  {
    sql << "      crs_pnr.system='CRS' AND \n"
           "      crs_pax.pr_del=0 AND \n"
           "      crs_pax_tkn.rem_code='TKNE'\n";
  };

  if (ltype==allCheckedByTickNoAndCouponNo)
  {
    sql << "SELECT pax_grp.point_dep AS point_id, \n"
           "       pax_grp.airp_dep, pax_grp.airp_arv, pax_grp.class, \n"
           "       pax.* \n"
           "FROM pax_grp,pax \n"
           "WHERE pax_grp.grp_id=pax.grp_id AND pax.ticket_rem='TKNE' AND \n"
           "      pax.ticket_no=:ticket_no AND \n"
           "      pax.coupon_no=:coupon_no \n";
  }
  //ProgTrace(TRACE5, "%s: SQL=\n%s", __FUNCTION__, sql.str().c_str());
  return sql.str();
}

void GetNotDisplayedET(int point_id_tlg, int id, bool is_pax_id, std::set<ETSearchByTickNoParams> &searchParams)
{
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText = GetSQL(is_pax_id?notDisplayedByPaxIdTlg:
                                 notDisplayedByPointIdTlg);
  Qry.CreateVariable( "id", otInteger, id );
  Qry.CreateVariable( "point_id_tlg", otInteger, point_id_tlg );
  Qry.Execute();
  for(;!Qry.Eof;Qry.Next())
  {
    if (isDisplayedEt(Qry.FieldAsString("ticket_no"),
                      Qry.FieldAsInteger("coupon_no")))
    {
      continue;
    }
    ETSearchByTickNoParams params(Qry.FieldAsInteger("point_id"), Qry.FieldAsString("ticket_no"));
    searchParams.insert(params);
    //������塞 ⥫��ࠬ��� ३�
    params.airline=Qry.FieldAsString("airline");
    params.flt_no=Qry.FieldIsNULL("flt_no")?ASTRA::NoExists:Qry.FieldAsInteger("flt_no");
    params.airp_dep=Qry.FieldAsString("airp_dep");
    searchParams.insert(params);
  };
}

bool existsPaxWithEt(const std::string& ticket_no, int coupon_no)
{
  LogTrace(TRACE5) << __func__
                   << ": ticket_no=" << ticket_no
                   << ", coupon_no=" << coupon_no;
  auto cur = make_db_curs(
        "SELECT 1 FROM pax "
        "WHERE ticket_no=:ticket_no "
        "AND coupon_no=:coupon_no ",
        PgOra::getROSession("PAX"));
  cur.stb()
      .bind(":ticket_no", ticket_no)
      .bind(":coupon_no", coupon_no)
      .EXfet();

  return cur.err() != DbCpp::ResultCode::NoDataFound;
}

void GetAllStatusesByPointId(TListType type, int point_id, std::set<TETickItem> &list, bool clear_list=true)
{
  if (clear_list) list.clear();

  if (type==allStatusesByPointIdFromTlg) {
    TQuery Qry(&OraSession);
    Qry.Clear();
    Qry.SQLText =
        "SELECT crs_pax_tkn.ticket_no, crs_pax_tkn.coupon_no, \n"
        "       tlg_trips.airp_dep, crs_pnr.airp_arv, \n"
        "       tlg_binding.point_id_spp AS point_id \n"
        "FROM crs_pax_tkn, crs_pax, crs_pnr, tlg_trips, tlg_binding \n"
        "WHERE crs_pax_tkn.pax_id=crs_pax.pax_id AND \n"
        "      crs_pax.pnr_id=crs_pnr.pnr_id AND \n"
        "      crs_pnr.point_id=tlg_binding.point_id_tlg AND \n"
        "      crs_pnr.point_id=tlg_trips.point_id AND \n"
        "      tlg_binding.point_id_spp=:point_id AND \n"
        "      crs_pnr.system='CRS' AND \n"
        "      crs_pax.pr_del=0 AND \n"
        "      crs_pax_tkn.rem_code='TKNE'\n";
    Qry.CreateVariable( "point_id", otInteger, point_id );
    Qry.Execute();
    for(;!Qry.Eof;Qry.Next())
    {
      TETickItem item;
      item.clear();

      item.et.fromDB(Qry);
      item.point_id=Qry.FieldIsNULL("point_id")?ASTRA::NoExists:
                                           Qry.FieldAsInteger("point_id");
      item.airp_dep=Qry.FieldAsString("airp_dep");
      item.airp_arv=Qry.FieldAsString("airp_arv");

      TETickItem eticket;
      eticket.fromDB(item.et.no, item.et.coupon, TETickItem::ChangeOfStatus, false /*lock*/);
      if (not eticket.empty() && eticket.et.status == CouponStatus(CouponStatus::Unavailable)) {
        item.et.status = eticket.et.status;
        item.change_status_error=eticket.change_status_error;
      }
      list.insert(item);
    };
  }

  if (type==allNotCheckedStatusesByPointId) {
    const std::list<TETickItem> items = TETickItem::fromDB(point_id, false /*lock*/);
    for (const TETickItem& item: items) {
      if (item.empty() || not existsPaxWithEt(item.et.no, item.et.coupon)) {
        continue;
      }
      list.insert(item);
    }
  }
}

void GetByTickNoFromTlg(int point_id, const std::string& tick_no, std::set<TETCtxtItem> &items)
{
  items.clear();
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText=GetSQL(allByPointIdAndTickNoFromTlg);
  Qry.CreateVariable("point_id", otInteger, point_id);
  Qry.CreateVariable("ticket_no", otString, tick_no);
  Qry.Execute();
  for(;!Qry.Eof;Qry.Next())
  {
    if (Qry.FieldIsNULL("coupon_no")) continue;
    items.insert(TETCtxtItem().fromDB(Qry, true));
  };
}

void GetByCouponNoFromTlg(const std::string& tick_no, int coupon_no, std::list<TETCtxtItem> &items) //list �� ���� ⠪. ���� � �� �� �㯮� ����� ���� �ਢ易� �� ������ ३ᠬ
{
  items.clear();
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText=GetSQL(allByTickNoAndCouponNoFromTlg);
  Qry.CreateVariable("ticket_no", otString, tick_no);
  Qry.CreateVariable("coupon_no", otInteger, coupon_no);
  Qry.Execute();
  for(;!Qry.Eof;Qry.Next())
    items.push_back(TETCtxtItem().fromDB(Qry, true));
}

void GetCheckedByCouponNo(const std::string& tick_no, int coupon_no, std::list<TETCtxtItem> &items)
{
  items.clear();
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText=GetSQL(allCheckedByTickNoAndCouponNo);
  Qry.CreateVariable("ticket_no", otString, tick_no);
  Qry.CreateVariable("coupon_no", otInteger, coupon_no);
  Qry.Execute();
  for(;!Qry.Eof;Qry.Next())
  {
    TETCtxtItem ctxt;
    ctxt.fromDB(Qry, false);
    if (!ctxt.pax.refuse.empty()) continue;
    items.push_back(ctxt);
  }
}


} //namespace PaxETList

class ETDisplayKey
{
  public:
    string tick_no;
    string airline;
    pair<string,string> addrs;
    bool operator < (const ETDisplayKey &key) const
    {
      if (tick_no!=key.tick_no)
        return tick_no<key.tick_no;
      if (airline!=key.airline)
        return airline<key.airline;
      return addrs<key.addrs;
    }
    std::string traceStr() const
    {
      std::ostringstream s;
      s << "tick_no=" << tick_no
        << ", airline=" << airline
        << ", addrs=(" << addrs.first << "; " << addrs.second << ")";
      return s.str();
    }
};

class ETDisplayProps
{
  public:
    bool exchange;
    bool interactive;
    string airline;
    pair<string,string> addrs;
    ETDisplayProps() : exchange(false), interactive(true) {}
    std::string traceStr() const
    {
      std::ostringstream s;
      s << "exchange=" << (exchange?"true":"false")
        << ", interactive=" << (interactive?"true":"false")
        << ", airline=" << airline
        << ", addrs=(" << addrs.first << "; " << addrs.second << ")";
      return s.str();
    }
};

void TlgETDisplay(int point_id_tlg, const set<int> &ids, bool is_pax_id)
{
  map<ETWideSearchParams, ETDisplayProps> ets_props;
  set<ETDisplayKey> eticks;

  for(set<int>::const_iterator i=ids.begin(); i!=ids.end(); ++i)
  try
  {
    set<ETSearchByTickNoParams> params;
    PaxETList::GetNotDisplayedET(point_id_tlg, *i, is_pax_id, params);
    for(set<ETSearchByTickNoParams>::const_iterator p=params.begin(); p!=params.end(); ++p)
    {
      map<ETWideSearchParams, ETDisplayProps>::iterator iETSProps=ets_props.find(*p);
      if (iETSProps==ets_props.end())
      {
        ETDisplayProps props;
        TTripInfo flt;
        if (p->existsAdditionalFltInfo())
          p->set(flt);
        else
          flt.getByPointId(p->point_id);

        if (!flt.airline.empty() &&
            flt.flt_no!=ASTRA::NoExists &&
            !flt.airp.empty())
        {
          props.airline=flt.airline;
          props.exchange=/*checkETSExchange(flt, false) && �ਭ�� �襭�� �����஢��� �⬥�� ���ࠪ⨢� � ���뢠�� ⮫쪮 ����ன�� ���ᮢ ��� */
                         get_et_addr_set(flt.airline, flt.flt_no, props.addrs);
          props.interactive=checkETSInteract(flt, false);
        };

        iETSProps=ets_props.insert(make_pair(*p, props)).first;
        ProgTrace(TRACE5, "%s: ets_props.insert(%s; %s)", __FUNCTION__, p->traceStr().c_str(), props.traceStr().c_str());

      }
      if (iETSProps==ets_props.end()) throw EXCEPTIONS::Exception("%s: iETSProps==ets_props.end()!", __FUNCTION__);

      if (!iETSProps->second.exchange) continue;      //��� ������ � ���
      if (!iETSProps->second.interactive) continue;   //����஫�� ��⮤: ��ᯫ�� �� ����訢���

      //��� � ��� ����
      ETDisplayKey eticksKey;
      eticksKey.tick_no=p->tick_no;
      eticksKey.airline=iETSProps->second.airline;
      eticksKey.addrs=iETSProps->second.addrs;

      if (eticks.find(eticksKey)!=eticks.end()) continue;

      try
      {
        ETSearchInterface::SearchET(*p, ETSearchInterface::spTlgETDisplay, edifact::KickInfo());
        eticks.insert(eticksKey);
        ProgTrace(TRACE5, "%s: ETSearchInterface::SearchET (%s)", __FUNCTION__, p->traceStr().c_str());
      }
      catch(const UserException &e)
      {
        ProgTrace(TRACE5, "%s: %s", __FUNCTION__, e.what());
        ProgTrace(TRACE5, "%s: ETSearchByTickNoParams (%s)", __FUNCTION__, p->traceStr().c_str());
        ProgTrace(TRACE5, "%s: ETDisplayKey (%s)", __FUNCTION__, eticksKey.traceStr().c_str());
      };
    }
  }
  catch(const std::exception &e)
  {
    ProgError(STDLOG, "%s: %s", __FUNCTION__, e.what());
  }
  catch(...)
  {
    ProgError(STDLOG, "%s: unknown error", __FUNCTION__);
  };
}

void TlgETDisplay(int point_id_tlg, int id, bool is_pax_id)
{
  set<int> ids;
  ids.insert(id);
  TlgETDisplay(point_id_tlg, ids, is_pax_id);
}

void TlgETDisplay(int point_id_spp)
{
  set<int> ids;
  ids.insert(point_id_spp);

  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText = "SELECT point_id_tlg FROM tlg_binding WHERE point_id_spp=:point_id_spp";
  Qry.CreateVariable( "point_id_spp", otInteger, point_id_spp );
  Qry.Execute();
  for(;!Qry.Eof;Qry.Next())
    TlgETDisplay(Qry.FieldAsInteger("point_id_tlg"), ids, false);
}

static bool deleteDisplayTlg(const TETickItem& item)
{
  LogTrace(TRACE5) << __func__
                   << ": ticket_no=" << item.et.no
                   << ", coupon_no=" << item.et.coupon;
  auto cur = make_db_curs(
        "DELETE FROM eticks_display_tlgs "
        "WHERE ticket_no=:ticket_no "
        "AND coupon_no=:coupon_no",
        PgOra::getRWSession("ETICKS_DISPLAY_TLGS"));
  cur.stb()
      .bind(":ticket_no", item.et.no)
      .bind(":coupon_no", item.et.coupon)
      .exec();

  LogTrace(TRACE5) << __func__
                   << ": rowcount=" << cur.rowcount();
  return cur.rowcount() > 0;
}

static bool insertDisplayTlg(const TETickItem& item)
{
  LogTrace(TRACE5) << __func__
                   << ": ticket_no=" << item.et.no
                   << ", coupon_no=" << item.et.coupon;

  short notNull = 0;
  short null = -1;
  auto cur = make_db_curs(
        "INSERT INTO eticks_display_tlgs( "
        "ticket_no, coupon_no, page_no, tlg_text, tlg_type, last_display "
        ") VALUES( "
        ":ticket_no, :coupon_no, :page_no, :tlg_text, :tlg_type, :last_display "
        ")",
        PgOra::getRWSession("ETICKS_DISPLAY_TLGS"));
  cur.stb()
      .bind(":ticket_no", item.et.no)
      .bind(":coupon_no", item.et.coupon)
      .bind(":tlg_type",
            item.ediPnr ? int(item.ediPnr.get().ediType()) : 0,
            item.ediPnr ? &notNull : &null)
      .bind(":last_display", DateTimeToBoost(NowUTC()));
  execWithPager(cur, ":tlg_text", item.ediPnr->ediTextWithCharset("IATA"), false, 1000);

  LogTrace(TRACE5) << __func__
                   << ": rowcount=" << cur.rowcount();
  return cur.rowcount() > 0;
}

void TETickItem::saveDisplayTlg() const
{
  if (ediPnr)
  {
    deleteDisplayTlg(*this);
    insertDisplayTlg(*this);
  }
}

static void bindDisplayItem(DbCpp::CursCtl& cur,
                            const TETickItem& item)
{
  short notNull = 0;
  short null = -1;
  cur.stb()
      .bind(":ticket_no", item.et.no)
      .bind(":coupon_no", item.et.coupon)
      .bind(":issue_date", DateTimeToBoost(item.issue_date))
      .bind(":surname", item.surname)
      .bind(":name", item.name)
      .bind(":fare_basis", item.fare_basis)
      .bind(":bag_norm",
            item.bagNorm ? item.bagNorm.get().getQuantity() : 0,
            item.bagNorm ? &notNull : &null)
      .bind(":bag_norm_unit",
            item.bagNorm ? TBagUnit(item.bagNorm.get().getUnit()).get_db_form() : "",
            item.bagNorm ? &notNull : &null)
      .bind(":fare_class", item.fare_class)
      .bind(":last_display", DateTimeToBoost(NowUTC()));
}

static bool updateDisplay(const TETickItem& item)
{
  LogTrace(TRACE5) << __func__
                   << ": ticket_no=" << item.et.no
                   << ", coupon_no=" << item.et.coupon;
  auto cur = make_db_curs(
        "UPDATE eticks_display "
        "SET issue_date=:issue_date, "
        "    surname=:surname, "
        "    name=:name, "
        "    fare_basis=:fare_basis, "
        "    bag_norm=:bag_norm, "
        "    bag_norm_unit=:bag_norm_unit, "
        "    fare_class=:fare_class, "
        "    last_display=:last_display "
        "WHERE ticket_no=:ticket_no "
        "AND coupon_no=:coupon_no ",
        PgOra::getRWSession("ETICKS_DISPLAY"));
  bindDisplayItem(cur, item);
  cur.exec();

  LogTrace(TRACE5) << __func__
                   << ": rowcount=" << cur.rowcount();
  return cur.rowcount() > 0;
}

static bool insertDisplay(const TETickItem& item)
{
  LogTrace(TRACE5) << __func__
                   << ": ticket_no=" << item.et.no
                   << ", coupon_no=" << item.et.coupon;
  auto cur = make_db_curs(
        "INSERT INTO eticks_display("
        "ticket_no, coupon_no, issue_date, surname, name, fare_basis, bag_norm, "
        "bag_norm_unit, fare_class, orig_fare_class, last_display) "
        "VALUES("
        ":ticket_no, :coupon_no, :issue_date, :surname, :name, :fare_basis, :bag_norm, "
        ":bag_norm_unit, :fare_class, :fare_class, :last_display)",
        PgOra::getRWSession("ETICKS_DISPLAY"));
  bindDisplayItem(cur, item);
  cur.exec();

  LogTrace(TRACE5) << __func__
                   << ": rowcount=" << cur.rowcount();
  return cur.rowcount() > 0;
}

void TETickItem::saveDisplay() const
{
  const bool updated = updateDisplay(*this);
  if (not updated) {
    const bool inserted = insertDisplay(*this);
    if (inserted) {
      saveDisplayTlg();
      syncOriginalSubclass(this->et);
    }
  }
}

static void bindEticketsItem(DbCpp::CursCtl& cur,
                             const TETickItem& item)
{
  short notNull = 0;
  short null = -1;
  cur.stb()
      .bind(":point_id", item.point_id)
      .bind(":airp_dep", item.airp_dep)
      .bind(":airp_arv", item.airp_arv)
      .bind(":coupon_status",
            item.et.status!=CouponStatus::Unavailable ? item.et.status->dispCode()
                                                      : "",
            item.et.status!=CouponStatus::Unavailable ? &notNull : &null)
      .bind(":error", item.change_status_error.substr(0,100))
      .bind(":ticket_no", item.et.no)
      .bind(":coupon_no", item.et.coupon);
}

static bool updateEtickets(const TETickItem& item)
{
  LogTrace(TRACE5) << __func__
                   << ": ticket_no=" << item.et.no
                   << ", coupon_no=" << item.et.coupon;
  auto cur = make_db_curs(
        "UPDATE etickets "
        "SET point_id=:point_id, "
        "    airp_dep=:airp_dep, "
        "    airp_arv=:airp_arv, "
        "    coupon_status=:coupon_status, "
        "    error=:error "
        "WHERE ticket_no=:ticket_no "
        "AND coupon_no=:coupon_no ",
        PgOra::getRWSession("ETICKETS"));
  bindEticketsItem(cur, item);
  cur.exec();

  LogTrace(TRACE5) << __func__
                   << ": rowcount=" << cur.rowcount();
  return cur.rowcount() > 0;
}

static bool updateEticketsError(const TETickItem& item)
{
  LogTrace(TRACE5) << __func__
                   << ": ticket_no=" << item.et.no
                   << ", coupon_no=" << item.et.coupon;
  auto cur = make_db_curs(
        "UPDATE etickets "
        "SET error=:error "
        "WHERE ticket_no=:ticket_no "
        "AND coupon_no=:coupon_no ",
        PgOra::getRWSession("ETICKETS"));
  cur.stb()
      .bind(":error", item.change_status_error.substr(0,100))
      .bind(":ticket_no", item.et.no)
      .bind(":coupon_no", item.et.coupon);
  cur.exec();

  LogTrace(TRACE5) << __func__
                   << ": rowcount=" << cur.rowcount();
  return cur.rowcount() > 0;
}

static bool insertEtickets(const TETickItem& item)
{
  LogTrace(TRACE5) << __func__
                   << ": ticket_no=" << item.et.no
                   << ", coupon_no=" << item.et.coupon;
  auto cur = make_db_curs(
        "INSERT INTO etickets( "
        "point_id, airp_dep, airp_arv, coupon_status, error, ticket_no, coupon_no) "
        "VALUES( "
        ":point_id, :airp_dep, :airp_arv, :coupon_status, :error, :ticket_no, :coupon_no) ",
        PgOra::getRWSession("ETICKETS"));
  bindEticketsItem(cur, item);
  cur.exec();

  LogTrace(TRACE5) << __func__
                   << ": rowcount=" << cur.rowcount();
  return cur.rowcount() > 0;
}

bool deleteEtickets(int point_id)
{
  LogTrace(TRACE5) << __func__
                   << ": point_id=" << point_id;
  auto cur = make_db_curs(
        "DELETE FROM etickets "
        "WHERE point_id=:point_id ",
        PgOra::getRWSession("ETICKETS"));
  cur.stb()
      .bind(":point_id", point_id)
      .exec();

  LogTrace(TRACE5) << __func__
                   << ": rowcount=" << cur.rowcount();
  return cur.rowcount() > 0;
}

void TETickItem::saveChangeOfStatus() const
{
  LogTrace(TRACE5) << __func__
                   << ": ticket_no=" << et.no
                   << ", coupon_no=" << et.coupon;
  bool updated = false;
  if (not change_status_error.empty()) {
    updated = updateEticketsError(*this);
  } else {
    updated = updateEtickets(*this);
  }
  if (not updated) {
    insertEtickets(*this);
  }
}

const TETickItem& TETickItem::toDB(const TEdiAction ediAction) const
{
  if (et.empty())
    throw EXCEPTIONS::Exception("TETickItem::toDB: empty eticket");

  switch(ediAction)
  {
    case DisplayTlg:
      saveDisplayTlg();
      break;
    case Display:
      saveDisplay();
      break;
    case ChangeOfStatus:
      saveChangeOfStatus();
      break;
  };
  return *this;
}

TETCoupon& TETCoupon::fromDB(TQuery &Qry)
{
  clear();
  no=Qry.FieldAsString("ticket_no");
  coupon=Qry.FieldIsNULL("coupon_no")?ASTRA::NoExists:
                                      Qry.FieldAsInteger("coupon_no");
  return *this;
}

TETCtxtItem& TETCtxtItem::fromDB(TQuery &Qry, bool from_crs)
{
  clear();
  et.fromDB(Qry);
  point_id=Qry.FieldIsNULL("point_id")?ASTRA::NoExists:
                                       Qry.FieldAsInteger("point_id");
  paxFromDB(Qry, from_crs);

  return *this;
}

TETickItem& TETickItem::loadDisplayTlg(const std::string& _et_no,
                                       int _et_coupon,
                                       bool lock)
{
  LogTrace(TRACE5) << __func__
                   << ": ticket_no=" << _et_no
                   << ", coupon_no=" << _et_coupon;
  int page_no = 0;
  Dates::DateTime_t last_display;
  std::string tlg_text;
  int tlg_type = 0;
  auto cur = make_db_curs(
        "SELECT coupon_no, last_display, page_no, ticket_no, tlg_text, tlg_type "
        "FROM eticks_display_tlgs "
        "WHERE ticket_no=:ticket_no "
        "AND coupon_no=:coupon_no "
        "ORDER BY page_no "
        + std::string(lock ? " FOR UPDATE" : ""),
        lock ? PgOra::getRWSession("ETICKS_DISPLAY_TLGS")
             : PgOra::getROSession("ETICKS_DISPLAY_TLGS"));
  cur.stb()
      .def(et.coupon)
      .def(last_display)
      .def(page_no)
      .def(et.no)
      .def(tlg_text)
      .def(tlg_type)
      .bind(":ticket_no", _et_no)
      .bind(":coupon_no", _et_coupon)
      .exec();

  while (!cur.fen()) {
    string text=ediPnr?ediPnr.get().ediText():"";
    ediPnr=Ticketing::EdiPnr(text + tlg_text,
                             static_cast<edifact::EdiMessageType>(tlg_type));
  }

  return *this;
}

class DisplaySqlHelper
{
public:
  explicit DisplaySqlHelper(const char* n, const char* f, int l, const std::string& sql,
                            DbCpp::Session& session)
      : cur(session, n, f, l, sql.c_str())
  {}
  static std::string fields();
  DbCpp::CursCtl& def();
  DbCpp::CursCtl& cursor() { return cur; }
  void fill(TETickItem& item);

private:
  DbCpp::CursCtl cur;
  int coupon_no = ASTRA::NoExists;
  std::string ticket_no;
  int bag_norm = ASTRA::NoExists;
  std::string bag_norm_unit;
  Dates::DateTime_t issue_date;
  Dates::DateTime_t last_display;
  std::string orig_fare_class;
  std::string name;
  std::string surname;
  std::string fare_basis;
  std::string fare_class;
};

std::string DisplaySqlHelper::fields()
{
  return "bag_norm, bag_norm_unit, coupon_no, fare_basis, fare_class, "
         "issue_date, last_display, name, orig_fare_class, surname, ticket_no ";
}

DbCpp::CursCtl& DisplaySqlHelper::def()
{
  cur.stb()
      .defNull(bag_norm, ASTRA::NoExists)
      .defNull(bag_norm_unit, "")
      .def(coupon_no)
      .def(fare_basis)
      .defNull(fare_class, "")
      .def(issue_date)
      .def(last_display)
      .defNull(name, "")
      .defNull(orig_fare_class, "")
      .def(surname)
      .def(ticket_no);
  return cur;
}

void DisplaySqlHelper::fill(TETickItem& item)
{
  if (bag_norm != ASTRA::NoExists) {
    item.bagNorm=boost::in_place(bag_norm, bag_norm_unit);
  }
  item.et.coupon = coupon_no;
  item.fare_basis = fare_basis;
  item.fare_class = fare_class;
  item.issue_date = issue_date.is_special() ? ASTRA::NoExists : BoostToDateTime(issue_date);
  item.name = name;
  item.orig_fare_class = orig_fare_class;
  item.surname = surname;
  item.et.no = ticket_no;
}

TETickItem& TETickItem::loadDisplay(const std::string& _et_no,
                                    int _et_coupon,
                                    bool lock)
{
  LogTrace(TRACE5) << __func__
                   << ": ticket_no=" << _et_no
                   << ", coupon_no=" << _et_coupon;
  clear();
  auto sqlh = DisplaySqlHelper(STDLOG,
        "SELECT " + DisplaySqlHelper::fields() +
        "FROM eticks_display "
        "WHERE ticket_no=:ticket_no "
        "AND coupon_no=:coupon_no "
        + std::string(lock ? " FOR UPDATE" : ""),
        lock ? PgOra::getRWSession("ETICKS_DISPLAY")
             : PgOra::getROSession("ETICKS_DISPLAY"));

  sqlh.def()
      .bind(":ticket_no", _et_no)
      .bind(":coupon_no", _et_coupon)
      .EXfet();

  if (sqlh.cursor().err() != DbCpp::ResultCode::NoDataFound) {
    sqlh.fill(*this);
  }

  return *this;
}

std::list<TETickItem> TETickItem::loadDisplay(const std::string& _et_no, bool lock)
{
  LogTrace(TRACE5) << __func__
                   << ": ticket_no=" << _et_no;
  std::list<TETickItem> result;
  auto sqlh = DisplaySqlHelper(STDLOG,
        "SELECT " + DisplaySqlHelper::fields() +
        "FROM eticks_display "
        "WHERE ticket_no=:ticket_no "
        + std::string(lock ? " FOR UPDATE" : ""),
        lock ? PgOra::getRWSession("ETICKS_DISPLAY")
             : PgOra::getROSession("ETICKS_DISPLAY"));

  sqlh.def()
      .bind(":ticket_no", _et_no)
      .exec();

  while (!sqlh.cursor().fen()) {
    TETickItem item;
    sqlh.fill(item);
    result.push_back(item);
  }
  LogTrace(TRACE5) << __func__
                   << ": count=" << result.size();
  return result;
}

class EticketsSqlHelper
{
public:
  explicit EticketsSqlHelper(const char* n, const char* f, int l, const std::string& sql,
                            DbCpp::Session& session)
      : cur(session, n, f, l, sql.c_str())
  {}
  static std::string fields();
  DbCpp::CursCtl& def();
  DbCpp::CursCtl& cursor() { return cur; }
  void fill(TETickItem& item);

private:
  DbCpp::CursCtl cur;
  std::string airp_dep;
  std::string airp_arv;
  int coupon_no = ASTRA::NoExists;
  std::string coupon_status;
  std::string change_status_error;
  int point_id = ASTRA::NoExists;
  std::string ticket_no;
};

std::string EticketsSqlHelper::fields()
{
  return "airp_arv, airp_dep, coupon_no, coupon_status, error, point_id, ticket_no ";
}

DbCpp::CursCtl& EticketsSqlHelper::def()
{
  cur.stb()
      .def(airp_arv)
      .def(airp_dep)
      .def(coupon_no)
      .defNull(coupon_status, CouponStatus(CouponStatus::Unavailable)->dispCode())
      .defNull(change_status_error, "")
      .defNull(point_id, ASTRA::NoExists)
      .def(ticket_no);
  return cur;
}

void EticketsSqlHelper::fill(TETickItem& item)
{
  item.airp_arv = airp_arv;
  item.airp_dep = airp_dep;
  item.et.coupon = coupon_no;
  item.et.status = CouponStatus(CouponStatus::fromDispCode(coupon_status));
  item.change_status_error = change_status_error;
  item.point_id = point_id;
  item.et.no = ticket_no;
}

TETickItem& TETickItem::loadChangeOfStatus(const std::string& _et_no,
                                           int _et_coupon,
                                           bool lock)
{
  LogTrace(TRACE5) << __func__
                   << ": ticket_no=" << _et_no
                   << ", coupon_no=" << _et_coupon;
  clear();
  auto sqlh = EticketsSqlHelper(STDLOG,
        "SELECT "+ EticketsSqlHelper::fields() +
        "FROM etickets "
        "WHERE ticket_no=:ticket_no "
        "AND coupon_no=:coupon_no "
        + std::string(lock ? " FOR UPDATE" : ""),
        lock ? PgOra::getRWSession("ETICKETS")
             : PgOra::getROSession("ETICKETS"));
  sqlh.def()
      .bind(":ticket_no", _et_no)
      .bind(":coupon_no", _et_coupon)
      .EXfet();

  if (sqlh.cursor().err() != DbCpp::ResultCode::NoDataFound) {
    sqlh.fill(*this);
  }

  LogTrace(TRACE5) << __func__
                   << ": found="
                   << (sqlh.cursor().err() != DbCpp::ResultCode::NoDataFound);
  return *this;
}

std::list<TETickItem> TETickItem::loadChangeOfStatus(int _point_id, bool lock)
{
  LogTrace(TRACE5) << __func__
                   << ": point_id=" << _point_id;
  std::list<TETickItem> result;
  auto sqlh = EticketsSqlHelper(STDLOG,
        "SELECT "+ EticketsSqlHelper::fields() +
        "FROM etickets "
        "WHERE point_id=:point_id "
        + std::string(lock ? " FOR UPDATE" : ""),
        lock ? PgOra::getRWSession("ETICKETS")
             : PgOra::getROSession("ETICKETS"));
  sqlh.def()
      .bind(":point_id", _point_id)
      .EXfet();

  while (!sqlh.cursor().fen()) {
    TETickItem item;
    sqlh.fill(item);
    result.push_back(item);
  }
  LogTrace(TRACE5) << __func__
                   << ": count=" << result.size();
  return result;
}

std::list<TETickItem> TETickItem::loadChangeOfStatus(const std::string& _et_no, bool lock)
{
  LogTrace(TRACE5) << __func__
                   << ": ticket_no=" << _et_no;
  std::list<TETickItem> result;
  auto sqlh = EticketsSqlHelper(STDLOG,
        "SELECT " + EticketsSqlHelper::fields() +
        "FROM etickets "
        "WHERE ticket_no=:ticket_no "
        + std::string(lock ? " FOR UPDATE" : ""),
        lock ? PgOra::getRWSession("ETICKETS")
             : PgOra::getROSession("ETICKETS"));

  sqlh.def()
      .bind(":ticket_no", _et_no)
      .exec();

  while (!sqlh.cursor().fen()) {
    TETickItem item;
    sqlh.fill(item);
    result.push_back(item);
  }
  LogTrace(TRACE5) << __func__
                   << ": count=" << result.size();
  return result;
}

TETickItem& TETickItem::fromDB(const std::string &_et_no,
                               const int _et_coupon,
                               const TEdiAction ediAction,
                               const bool lock)
{
  clear();

  if (_et_no.empty() || _et_coupon==ASTRA::NoExists)
    throw EXCEPTIONS::Exception("TETickItem::fromDB: empty eticket");

  switch(ediAction)
  {
    case DisplayTlg:
      return loadDisplayTlg(_et_no, _et_coupon, lock);
    case Display:
      return loadDisplay(_et_no, _et_coupon, lock);
    case ChangeOfStatus:
      return loadChangeOfStatus(_et_no, _et_coupon, lock);
  }

  return *this;
}

std::list<TETickItem> TETickItem::fromDB(int _point_id, const bool lock)
{
  if (_point_id==ASTRA::NoExists)
    throw EXCEPTIONS::Exception("TETickItem::fromDB: invalid point_id");

  return loadChangeOfStatus(_point_id, lock);
}

list<TETickItem> TETickItem::fromDB(const std::string &_et_no,
                                    const TEdiAction ediAction)
{
  if (_et_no.empty())
    throw EXCEPTIONS::Exception("TETickItem::fromDB: empty eticket");

  switch(ediAction)
  {
    case DisplayTlg:
      throw EXCEPTIONS::Exception("TETickItem::fromDB: DisplayTlg not supported");
    case Display:
      return loadDisplay(_et_no, false /*lock*/);
    case ChangeOfStatus:
      return loadChangeOfStatus(_et_no, false /*lock*/);
  }
  throw EXCEPTIONS::Exception("TETickItem::fromDB: Invalid ediAction");;
}

static boost::optional<Itin> getCouponItin(const Ticketing::TicketNum_t& ticknum,
                                           const Ticketing::CouponNum_t& cpnnum,
                                           const Ticketing::Pnr& pnr)
{
  for(const Ticket &tick : pnr.ltick())
  {
    if(tick.actCode() == TickStatAction::oldtick ||
       tick.actCode() == TickStatAction::inConnectionWith) continue;

    if (tick.ticknum()!=ticknum.get()) continue;

    for(const Coupon &cpn : tick.getCoupon())
    {
      if (cpn.couponInfo().num()!=(unsigned)cpnnum.get()) continue;
      if (cpn.haveItin())
        return cpn.itin();
      else
        return boost::none;
    };
  }

  return boost::none;
}

boost::optional<Itin> getEtDispCouponItin(const Ticketing::TicketNum_t& ticknum,
                                          const Ticketing::CouponNum_t& cpnnum)
{
  TETickItem item;
  item.fromDB(ticknum.get(), cpnnum.get(), TETickItem::DisplayTlg, false);
  if (!item.ediPnr) LogTrace(TRACE5) << __FUNCTION__ << ": item.ediPnr==boost::none";

  if (item.ediPnr)
  {
    const Ticketing::Pnr pnr=readPnr(item.ediPnr.get());
    return getCouponItin(ticknum, cpnnum, pnr);
  }

  return boost::none;
}

static boost::optional<Itin> getWcCouponItin(const Ticketing::TicketNum_t& ticknum,
                                             const Ticketing::CouponNum_t& cpnnum)
{
  boost::optional<WcPnr> wcPnr=loadWcPnr(ticknum);
  if(!wcPnr) LogTrace(TRACE5) << __FUNCTION__ << ": wcPnr==boost::none";

  if (wcPnr)
    return getCouponItin(ticknum, cpnnum, wcPnr.get().pnr());

  return boost::none;
}

static std::string airlineToXML(const std::string& code)
{
  return airlineToPrefferedCode(code, OutputLang(LANG_RU, {OutputLang::OnlyTrueIataCodes}));
}

static boost::optional<std::string> segmentAirpSpecView(const AirportCode_t& airp,
                                                        const bool isDepartureAirp,
                                                        const Ticketing::TicketNum_t& ticknum,
                                                        const Ticketing::CouponNum_t& cpnnum)
{
  if (airp.get()=="���")
  {
    boost::optional<Itin> etDispItin=getEtDispCouponItin(ticknum, cpnnum);
    if (!etDispItin) return {};

    const std::string& airpFromET=isDepartureAirp?etDispItin.get().depPointCode():
                                                  etDispItin.get().arrPointCode();

    if (airpFromET=="TSE" || airpFromET=="NQZ") return airpFromET;
  }
  return {};
}

std::string airpDepToPrefferedCode(const AirportCode_t& airp,
                                   const boost::optional<CheckIn::TPaxTknItem>& tkn,
                                   const OutputLang &lang)
{
  if (tkn && tkn.get().validET())
  {
    const auto special=segmentAirpSpecView(airp,
                                           true,
                                           Ticketing::TicketNum_t(tkn.get().no),
                                           Ticketing::CouponNum_t(tkn.get().coupon));
    if (special) return special.get();
  }
  return airpToPrefferedCode(airp.get(), lang);
}

std::string airpArvToPrefferedCode(const AirportCode_t& airp,
                                   const boost::optional<CheckIn::TPaxTknItem>& tkn,
                                   const OutputLang &lang)
{
  if (tkn && tkn.get().validET())
  {
    const auto special=segmentAirpSpecView(airp,
                                           false,
                                           Ticketing::TicketNum_t(tkn.get().no),
                                           Ticketing::CouponNum_t(tkn.get().coupon));
    if (special) return special.get();
  }
  return airpToPrefferedCode(airp.get(), lang);
}

Ticketing::Ticket TETickItem::makeTicket(const AstraEdifact::TFltParams& fltParams,
                                         const std::string& subclass,
                                         const CouponStatus& real_status) const
{
  Coupon_info ci(et.coupon, et.status);
  TDateTime scd_local=UTCToLocal(fltParams.fltInfo.scd_out,
                                 AirpTZRegion(fltParams.fltInfo.airp));
  ptime scd(DateTimeToBoost(scd_local));

  Ticketing::TicketNum_t ticketNum(et.no);
  Ticketing::CouponNum_t couponNum(et.coupon);

  boost::optional<Itin> wcItin;
  if (fltParams.control_method)
  {
    wcItin=getWcCouponItin(ticketNum, couponNum);

    if (wcItin)
      LogTrace(TRACE5) << __FUNCTION__ << ": wcItin.get().airCode()=" << wcItin.get().airCode()
                       << " wcItin.get().flightnum()=" << wcItin.get().flightnum();
    else
      LogTrace(TRACE5) << __FUNCTION__ << ": wcItin==boost::none";
  };

  const auto airpDepSpecView=segmentAirpSpecView(AirportCode_t(airp_dep),
                                                 true,
                                                 ticketNum,
                                                 couponNum);

  const auto airpArvSpecView=segmentAirpSpecView(AirportCode_t(airp_arv),
                                                 false,
                                                 ticketNum,
                                                 couponNum);

  Itin itin(wcItin?wcItin.get().airCode():
                   airlineToXML(fltParams.fltInfo.airline),   //marketing carrier
            "",                                  //operating carrier
            wcItin?wcItin.get().flightnum():
                   fltParams.fltInfo.flt_no,0,
            (!subclass.empty() && real_status==CouponStatus::Flown)?SubClass(subclass):SubClass(),
            scd.date(),
            time_duration(not_a_date_time), // not a date time
            airp_dep,
            airp_arv);
  if (airpDepSpecView) itin.setSpecDepPoint(airpDepSpecView.get());
  if (airpArvSpecView) itin.setSpecArrPoint(airpArvSpecView.get());

  Coupon cpn(ci,itin);

  list<Coupon> lcpn;
  lcpn.push_back(cpn);

  return Ticket(et.no, lcpn);
}

struct PaxData4Sync
{
  int view_order;
  std::string etick_subclass;
  std::string etick_class;
  int class_priority;
  std::string ticket_no;
  int coupon_no;

  static std::vector<PaxData4Sync> load(int pax_id);
};

std::vector<PaxData4Sync> PaxData4Sync::load(int pax_id)
{
  LogTrace(TRACE5) << __func__
                   << ": pax_id=" << pax_id;
  std::vector<PaxData4Sync> result;
  PaxData4Sync item;
  auto cur = make_db_curs(
        "SELECT 1 AS view_order, "
        "       crs_rbd.fare_class AS etick_subclass, "
        "       classes.code AS etick_class, "
        "       classes.priority AS class_priority,"
        "       crs_pax_tkn.ticket_no AS ticket_no, "
        "       crs_pax_tkn.coupon_no AS coupon_no "
        "FROM crs_pax, crs_pax_tkn, crs_pnr, pnr_market_flt, crs_rbd, subcls, classes "
        "WHERE crs_pax_tkn.pax_id=crs_pax.pax_id AND "
        "      crs_pax_tkn.rem_code='TKNE' AND "
        "      crs_pnr.pnr_id=crs_pax.pnr_id AND "
        "      crs_pnr.system='CRS' AND "
        "      crs_pnr.pnr_id=pnr_market_flt.pnr_id(+) AND "
        "      pnr_market_flt.pnr_id IS NULL AND "
        "      crs_rbd.point_id=crs_pnr.point_id AND "
        "      crs_rbd.sender=crs_pnr.sender AND "
        "      crs_rbd.system=crs_pnr.system AND "
        "      subcls.code=crs_rbd.compartment AND "
        "      classes.code=subcls.class AND "
        "      crs_pax.pax_id=:pax_id "
        "UNION ALL "
        "SELECT 2 AS view_order, "
        "       subcls.code AS etick_subclass, "
        "       classes.code AS etick_class, "
        "       classes.priority AS class_priority, "
        "       crs_pax_tkn.ticket_no AS ticket_no, "
        "       crs_pax_tkn.coupon_no AS coupon_no "
        "FROM crs_pax, crs_pax_tkn, crs_pnr, pnr_market_flt, subcls, classes "
        "WHERE crs_pax_tkn.pax_id=crs_pax.pax_id AND "
        "      crs_pax_tkn.rem_code='TKNE' AND "
        "      crs_pnr.pnr_id=crs_pax.pnr_id AND "
        "      crs_pnr.system='CRS' AND "
        "      crs_pnr.pnr_id=pnr_market_flt.pnr_id(+) AND "
        "      pnr_market_flt.pnr_id IS NULL AND "
        "      classes.code=subcls.class AND "
        "      crs_pax.pax_id=:pax_id "
        "ORDER BY view_order, class_priority, etick_subclass ",
        *get_main_ora_sess(STDLOG));
  cur.stb()
      .def(item.view_order)
      .def(item.etick_subclass)
      .def(item.etick_class)
      .def(item.class_priority)
      .def(item.ticket_no)
      .def(item.coupon_no)
      .bind(":pax_id", pax_id)
      .exec();

  while (!cur.fen()) {
    result.push_back(item);
  }

  return result;
}

static bool updateCrsPaxSubclass(int pax_id,
                                 const std::string& etick_subclass,
                                 const std::string& etick_class)
{
  LogTrace(TRACE5) << __func__
                   << ": pax_id=" << pax_id;
  auto cur = make_db_curs(
        "UPDATE crs_pax "
        "SET etick_subclass=:etick_subclass, "
        "    etick_class=:etick_class "
        "WHERE pax_id=:pax_id "
        "AND (etick_subclass<>:etick_subclass "
        "     OR etick_subclass IS NULL AND :etick_subclass IS NOT NULL "
        "     OR etick_subclass IS NOT NULL AND :etick_subclass IS NULL "
        "     OR etick_class<>:etick_class "
        "     OR etick_class IS NULL AND :etick_class IS NOT NULL "
        "     OR etick_class IS NOT NULL AND :etick_class IS NULL) ",


        PgOra::getRWSession("CRS_PAX"));
  cur.stb()
      .bind(":etick_subclass", etick_subclass)
      .bind(":etick_class", etick_class)
      .bind(":pax_id", pax_id);
  cur.exec();

  LogTrace(TRACE5) << __func__
                   << ": rowcount=" << cur.rowcount();
  return cur.rowcount() > 0;
}

bool TETickItem::syncOriginalSubclass(int pax_id)
{
  LogTrace(TRACE5) << __func__
                   << ": pax_id=" << pax_id;

  bool updated = false;
  const std::vector<PaxData4Sync> paxItems = PaxData4Sync::load(pax_id);
  for (const PaxData4Sync& paxData: paxItems) {
    TETickItem etick;
    etick.loadDisplay(paxData.ticket_no, paxData.coupon_no);
    if (etick.et.empty() or paxData.etick_subclass != etick.orig_fare_class) {
      continue;
    }
    updated = updateCrsPaxSubclass(pax_id,
                                   paxData.etick_subclass,
                                   paxData.etick_class);
    if (updated) {
      ProgTrace(TRACE5, "%s: pax_id=%d updated", __FUNCTION__, pax_id);
    }
    break;
  }
  return updated;
}

void TETickItem::syncOriginalSubclass(const TETCoupon& et)
{
  if (et.empty()) return;

  CheckIn::TSimplePaxList paxs;
  CheckIn::Search search(paxPnl);
  search(paxs, CheckIn::TPaxTknItem(et.no, et.coupon));

  for(const CheckIn::TSimplePaxItem& pax : paxs)
    syncOriginalSubclass(pax.id);
}

void ETDisplayToDB(const Ticketing::EdiPnr& ediPnr)
{
  TETickItem ETickItem;
  ETickItem.ediPnr=ediPnr;

  const Ticketing::Pnr pnr=readPnr(ediPnr);

  for(list<Ticket>::const_iterator i=pnr.ltick().begin(); i!=pnr.ltick().end(); ++i)
  {
    const Ticket &tick = *i;
    if(tick.actCode() == TickStatAction::oldtick ||
       tick.actCode() == TickStatAction::inConnectionWith) continue;

    ETickItem.et.no=tick.ticknum();
    if (pnr.rci().dateOfIssue().is_special())
    {
      ProgError(STDLOG, "%s: pnr.rci().dateOfIssue().is_special()! (ticket=%s)",
                        __FUNCTION__, ETickItem.et.no_str().c_str());
      continue;
    };
    ETickItem.issue_date=BoostToDateTime(pnr.rci().dateOfIssue());
    if (pnr.pass().surname().empty())
    {
      ProgError(STDLOG, "%s: pnr.pass().surname().empty()! (ticket=%s)",
                        __FUNCTION__, ETickItem.et.no_str().c_str());
      continue;
    }
    ETickItem.surname=pnr.pass().surname();
    ETickItem.name=pnr.pass().name();


    for(list<Coupon>::const_iterator j=tick.getCoupon().begin(); j!=tick.getCoupon().end(); ++j)
    {
      const Coupon &cpn = *j;
      ETickItem.et.coupon=cpn.couponInfo().num();
      if(!cpn.haveItin())
      {
        ProgError(STDLOG, "%s: !cpn.haveItin()! (ticket=%s)",
                          __FUNCTION__, ETickItem.et.no_str().c_str());
        continue;
      };

      const Itin &itin = cpn.itin();
      ETickItem.fare_basis=itin.fareBasis();
      TrimString(ETickItem.fare_basis); //��� �������� �ࠪ⨪�, fareBasis() ����� ᮤ�ঠ�� �஡��� � ����
      if (ETickItem.fare_basis.empty())
      {
        ProgError(STDLOG, "%s: itin.fareBasis().empty()! (ticket=%s)",
                          __FUNCTION__, ETickItem.et.no_str().c_str());
        continue;
      }
      if(itin.luggage().haveLuggage())
        ETickItem.bagNorm=boost::in_place(itin.luggage()->quantity(),
                                          itin.luggage()->chargeQualifier());
      else
        ETickItem.bagNorm=boost::none;

      ETickItem.fare_class = itin.rbd()->code(RUSSIAN);

      ETickItem.toDB(TETickItem::Display);
    }
  };
}

void ETSearchInterface::SearchET(const ETSearchParams& searchParams,
                                 const SearchPurpose searchPurpose,
                                 const edifact::KickInfo& kickInfo)
{
  const ETWideSearchParams& params = dynamic_cast<const ETWideSearchParams&>(searchParams);
  Ticketing::TicketNum_t tickNum=checkDocNum(searchPurpose==spETRequestControl?
                                             dynamic_cast<const ETRequestControlParams&>(searchParams).tick_no:
                                             dynamic_cast<const ETSearchByTickNoParams&>(searchParams).tick_no, true);
  Ticketing::CouponNum_t cpnNum=searchPurpose==spETRequestControl?
                                Ticketing::CouponNum_t(dynamic_cast<const ETRequestControlParams&>(searchParams).coupon_no):
                                Ticketing::CouponNum_t();

  TTripInfo info;
  if (params.existsAdditionalFltInfo())
  {
    params.set(info);
  }
  else
  {
    if (!info.getByPointId(params.point_id))
      throw AstraLocale::UserException("MSG.FLIGHT.CHANGED.REFRESH_DATA");
  };

  if (!(searchPurpose==spTlgETDisplay && kickInfo.background_mode()))
    checkETSExchange(info, true);
  if ((searchPurpose==spEMDDisplay ||
       searchPurpose==spEMDRefresh) &&
      !kickInfo.background_mode())
    checkEDSExchange(info, true);

  if (searchPurpose!=spETRequestControl &&
      searchPurpose!=spETDisplay)
    checkETSInteract(info, true);

  if(!inTestMode())
  {
    pair<string,string> edi_addrs;
    if (!get_et_addr_set(info.airline,info.flt_no,edi_addrs))
        throw AstraLocale::UserException("MSG.ETICK.ETS_ADDR_NOT_DEFINED_FOR_FLIGHT",
                                         LParams() << LParam("flight", ElemIdToCodeNative(etAirline,info.airline) + IntToString(info.flt_no)));

    set_edi_addrs(edi_addrs);
  }

  ProgTrace(TRACE5,"ETSearch: oper_carrier=%s edi_addr=%s edi_own_addr=%s",
                   info.airline.c_str(),get_edi_addr().c_str(),get_edi_own_addr().c_str());

  const OrigOfRequest &org=kickInfo.background_mode()?OrigOfRequest(airlineToXML(info.airline)):
                                                      OrigOfRequest(airlineToXML(info.airline), *TReqInfo::Instance());

  xmlNodePtr rootNode;
  XMLDoc xmlCtxt("context", rootNode, __FUNCTION__);
  NewTextChild(rootNode,"point_id",params.point_id);
  kickInfo.toXML(rootNode);
  OrigOfRequest::toXML(org, getTripAirline(info), getTripFlightNum(info), rootNode);

  SetProp(rootNode,"req_ctxt_id", kickInfo.reqCtxtId, ASTRA::NoExists);
  switch(searchPurpose)
  {
    case spETDisplay:
    case spTlgETDisplay:
      SetProp(rootNode,"purpose","ETDisplay");
      break;
    case spEMDDisplay:
      SetProp(rootNode,"purpose","EMDDisplay");
      break;
    case spEMDRefresh:
      SetProp(rootNode,"purpose","EMDRefresh");
      break;
    case spETRequestControl:
      SetProp(rootNode,"purpose","ETRequestControl");
      break;
  };

  if (searchPurpose==spETRequestControl)
    edifact::SendEtRacRequest(edifact::EtRacParams(org,
                                                   "",
                                                   kickInfo,
                                                   info.airline,
                                                   Ticketing::FlightNum_t(info.flt_no),
                                                   tickNum,
                                                   cpnNum));
  else
    edifact::SendEtDispByNumRequest(edifact::EtDispByNumParams(org,
                                                               XMLTreeToText(xmlCtxt.docPtr()),
                                                               kickInfo,
                                                               info.airline,
                                                               Ticketing::FlightNum_t(info.flt_no),
                                                               tickNum));
}

void ETSearchInterface::SearchETByTickNo(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  int point_id=NodeAsInteger("point_id",reqNode);
  if(point_id < 0) {
      throw UserException("MSG.ETICK.UNABLE_TO_DISPLAY_ET_ON_EDI_SEGMENT");
  }
  TicketNum_t tickNum=checkDocNum(NodeAsString("TickNoEdit",reqNode), true);

  TTripInfo flt;
  if (!checkETSInteract(point_id, false, flt))
  {
    LogTrace(TRACE5) << __FUNCTION__ << ": control method applies (point_id=" << point_id << ")";

    CouponNum_t cpnNum;
    xmlNodePtr paxNode=GetNode("pax", reqNode);
    if (paxNode!=nullptr)
    {
      CheckIn::TPaxTknItem paxTkn;
      paxTkn.fromXML(paxNode);
      if (paxTkn.no==tickNum.get())
      {
        if (!paxTkn.validET())
          throw UserException("MSG.ETICK.FOR_PASSENGER_NOT_SET_TICK_AND_COUPON_NO");
        cpnNum=CouponNum_t(paxTkn.coupon);
      }
    }

    if (!cpnNum)
    {
      set<TETCtxtItem> ETCtxtItems;
      PaxETList::GetByTickNoFromTlg(point_id, tickNum.get(), ETCtxtItems);
      if (ETCtxtItems.empty()) throw UserException("MSG.ETICK.NOT_FOUND_IN_PNL", LParams()<<LParam("etick", tickNum.get()));
      if (ETCtxtItems.size()>1) throw  UserException("MSG.ETICK.DUPLICATED_IN_PNL", LParams()<<LParam("etick", tickNum.get()));
      cpnNum=CouponNum_t(ETCtxtItems.begin()->et.coupon);
    }

    bool existsAC=existsAirportControl(tickNum, cpnNum, false);

    CheckIn::TPaxTknItem tkn(tickNum.get(), cpnNum.get());
    TFltParams ediFltParams;
    if (!ediFltParams.get(point_id))
      throw AstraLocale::UserException("MSG.FLIGHT.CHANGED.REFRESH_DATA");
    int ETCount=tkn.checkedInETCount();
    LogTrace(TRACE5) << __FUNCTION__ << ": tkn.checkedInETCount()=" << ETCount
                                     << " existsAC=" << boolalpha << existsAC
                                     << " in_final_status=" << boolalpha << ediFltParams.in_final_status;
    if (ETCount==0 &&
        !existsAC &&
        !ediFltParams.in_final_status)
    {
      //��६ ��ய��⮢� ����஫�
      edifact::KickInfo kickInfo=createKickInfo(AstraContext::SetContext("TERM_REQUEST",XMLTreeToText(reqNode->doc)),
                                                "ETRequestControl");

      LogTrace(TRACE5) << __FUNCTION__ << ": SearchET(ETSearchInterface::spETRequestControl)";
      SearchET(ETRequestControlParams(point_id, tickNum.get(), cpnNum.get()),
               ETSearchInterface::spETRequestControl,
               kickInfo);
    }
    else
    {
      boost::optional<WcPnr> wcPnr=loadWcPnrWithActualStatuses(tickNum);
      if (!wcPnr) LogTrace(TRACE5) << __FUNCTION__ << ": wcPnr==boost::none";
      if (wcPnr)
      {
        boost::optional<EdiPnr> ediPnr;
        loadWcEdiPnr(wcPnr.get().recloc(), ediPnr);
        if (ediPnr) ETDisplayToDB(ediPnr.get()); //�� ��直� ��砩

        xmlNodePtr dataNode=getNode(astra_iface(resNode, "ETViewForm"),"data");
        PnrDisp::doDisplay(PnrXmlView(dataNode), wcPnr.get().pnr());
//        if (!existsAC)
//          showErrorMessage("��� ����஫� �㯮��. ������� ��ࠧ �� �� ������ ������ ����஫� �� DCS"); //�� �뢮����� � xml, �� �� �������� ����� � �ନ���� :(
        return; //�뢥�� working copy � �����...
      }
      else if (existsAC)
        throw EXCEPTIONS::Exception("%s: strange situation: existsAC && !wcPnr", __FUNCTION__);
      else if (ediFltParams.in_final_status)
        throw UserException("MSG.ETICK.FLIGHT_IN_FINAL_STATUS_UNABLE_RECEIVE_CONTROL",
                            LParams()<<LParam("eticket", tickNum.get())
                                     <<LParam("coupon", IntToString(cpnNum.get())));
    }

    if (!isDoomedToWait())
    {
      ProgError(STDLOG, "%s: !isDoomedToWait()", __FUNCTION__);
      throw UserException("MSG.ETICK.UNABLE_RECEIVE_CONTROL",
                          LParams()<<LParam("eticket", tickNum.get())
                                   <<LParam("coupon", IntToString(cpnNum.get())));
    }
  }

  if (!isDoomedToWait())
  {
    edifact::KickInfo kickInfo=createKickInfo(AstraContext::SetContext("TERM_REQUEST",XMLTreeToText(reqNode->doc)),
                                              "ETSearchForm");

    LogTrace(TRACE5) << __FUNCTION__ << ": SearchET(ETSearchInterface::spETDisplay)";
    SearchET(ETSearchByTickNoParams(point_id, tickNum.get()),
             ETSearchInterface::spETDisplay,
             kickInfo);
  }

  if (isDoomedToWait())
  {
    //� ��� ������⢨� �裡
    xmlNodePtr errNode=NewTextChild(resNode,"ets_connect_error");
    SetProp(errNode,"internal_msgid",get_internal_msgid_hex());
    NewTextChild(errNode,"message",getLocaleText("MSG.ETS_CONNECT_ERROR"));
  };
}

void EMDSearchInterface::EMDTextView(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
/*
  <term>
    <query handle="0" id="EMDSearch" ver="1" opr="VLAD" screen="AIR.EXE" mode="STAND" lang="EN" term_id="2051148234">
      <EMDTextView>
        <point_id>2626213</point_id>
        <pax_id>26980490</pax_id>
        <ticket_no>2982408009963</ticket_no>
        <coupon_no>1</coupon_no>
        <ticket_rem>TKNE</ticket_rem>
      </EMDTextView>
    </query>
  </term>
*/

  ETSearchByTickNoParams params(NodeAsInteger("point_id",reqNode),
                                NodeAsString("ticket_no",reqNode));

  edifact::KickInfo kickInfo=createKickInfo(AstraContext::SetContext("TERM_REQUEST",XMLTreeToText(reqNode->doc)),
                                            "EMDDisplay");

  ETSearchInterface::SearchET(params, ETSearchInterface::spEMDDisplay, kickInfo);

  AstraLocale::showProgError("MSG.ETS_EDS_CONNECT_ERROR");
}

Pnr readDispPnr(const string &tlg_text)
{
  int ret = ReadEdiMessage(tlg_text.c_str());
  if(ret == EDI_MES_STRUCT_ERR){
    throw EXCEPTIONS::Exception("Error in message structure: %s",EdiErrGetString());
  } else if( ret == EDI_MES_NOT_FND){
    throw EXCEPTIONS::Exception("No message found in template: %s",EdiErrGetString());
  } else if( ret == EDI_MES_ERR) {
    throw EXCEPTIONS::Exception("Edifact error ");
  }

  return readDispPnr(GetEdiMesStruct());
}

Ticketing::Pnr readDispPnr(EDI_REAL_MES_STRUCT *pMes)
{
    int num = GetNumSegGr(pMes, 3);
    if(!num){
      if(GetNumSegment(pMes, "ERC")){
        const char *errc = GetDBFName(pMes, DataElement(9321),
                                      "ET_NEG",
                                      CompElement("C901"),
                                      SegmElement("ERC"));
        ProgTrace(TRACE1, "ETS: ERROR %s", errc);
        const char * err_msg = GetDBFName(pMes,
                                          DataElement(4440),
                                          SegmElement("IFT"));
        if (*err_msg==0)
        {
            tst();
          throw AstraLocale::UserException("MSG.ETICK.ETS_ERROR", LParams() << LParam("msg", errc));
        }
        else
        {
          ProgTrace(TRACE1, "ETS: %s", err_msg);
          throw AstraLocale::UserException("MSG.ETICK.ETS_ERROR", LParams() << LParam("msg", err_msg));
        }
      }
      throw EXCEPTIONS::Exception("ETS error");
    } else if(num==1){
      try{
        Pnr pnr = PnrRdr::doRead<Pnr>(PnrEdiRead(pMes, edifact::EdiDisplRes));
        Pnr::Trace(TRACE2, pnr);
        return pnr;
      }
      catch(const edilib::EdiExcept &e)
      {
        throw EXCEPTIONS::Exception("edilib: %s", e.what());
      }
    } else {
      throw AstraLocale::UserException("MSG.ETICK.ET_LIST_VIEW_UNSUPPORTED"); //���� �� �����ন������
    }
}

Ticketing::Pnr readUacPnr(EDI_REAL_MES_STRUCT *pMes)
{
    try{
      Pnr pnr = PnrRdr::doRead<Pnr>(PnrEdiRead(pMes, edifact::EdiUacReq));
      Pnr::Trace(TRACE2, pnr);
      return pnr;
    }
    catch(const edilib::EdiExcept &e)
    {
      throw EXCEPTIONS::Exception("edilib: %s", e.what());
    }
}

void ETSearchInterface::KickHandler(XMLRequestCtxt *ctxt,
                                    xmlNodePtr reqNode, xmlNodePtr resNode)
{
    int req_ctxt_id=NodeAsInteger("@req_ctxt_id",reqNode);
    AstraContext::ClearContext("TERM_REQUEST", req_ctxt_id);

    edifact::pRemoteResults remRes = edifact::RemoteResults::readSingle();
    ASSERT(remRes);
    try {
        if(remRes->status() == edifact::RemoteStatus::Success) {
            Pnr pnr=readDispPnr(remRes->tlgSource());
            xmlNodePtr dataNode=getNode(astra_iface(resNode, "ETViewForm"),"data");
            PnrDisp::doDisplay(PnrXmlView(dataNode), pnr);
        } else {
            HandleNotSuccessEtsResult(*remRes, resNode);
        }
    }
    catch(const edilib::EdiExcept &e) {
        throw EXCEPTIONS::Exception("edilib: %s", e.what());
    }
}

void ETRequestControlInterface::KickHandler(XMLRequestCtxt *ctxt,
                                            xmlNodePtr reqNode, xmlNodePtr resNode)
{
    int req_ctxt_id=NodeAsInteger("@req_ctxt_id",reqNode);
    AstraContext::ClearContext("TERM_REQUEST", req_ctxt_id);

    edifact::pRemoteResults remRes = edifact::RemoteResults::readSingle();
    ASSERT(remRes);
    try {
        if(remRes->status() == edifact::RemoteStatus::Success) {
            Ticketing::EdiPnr ediPnr(remRes->tlgSource(), edifact::EdiRacRes);
            Pnr pnr=readPnr(ediPnr);
            xmlNodePtr dataNode=getNode(astra_iface(resNode, "ETViewForm"),"data");
            PnrDisp::doDisplay(PnrXmlView(dataNode), pnr);
        } else {
            HandleNotSuccessEtsResult(*remRes, resNode);
        }
    }
    catch(const edilib::EdiExcept &e) {
        throw EXCEPTIONS::Exception("edilib: %s", e.what());
    }
}

//---------------------------------------------------------------------------------------

void ETRequestACInterface::RequestControl(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    LogTrace(TRACE3) << __FUNCTION__;

    int pointId  = NodeAsInteger("point_id", reqNode);
    Ticketing::TicketNum_t tickNum(NodeAsString("TickNoEdit", reqNode));
    CouponNum_t cpnNum(NodeAsInteger("TickCpnNo", reqNode));

    LogTrace(TRACE3) << pointId << "/" << tickNum << cpnNum;

    TTripInfo info;
    info.getByPointId(pointId);

    const OrigOfRequest org = OrigOfRequest(airlineToXML(info.airline), *TReqInfo::Instance());


    SendEtRacRequest(edifact::EtRacParams(org,
                                          "",
                                          edifact::KickInfo(),
                                          info.airline,
                                          Ticketing::FlightNum_t(info.flt_no),
                                          tickNum,
                                          cpnNum));

}

void ETRequestACInterface::KickHandler(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    LogTrace(TRACE3) << __FUNCTION__;
}

//---------------------------------------------------------------------------------------

void ETStatusInterface::SetTripETStatus(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  int point_id=NodeAsInteger("point_id",reqNode);
  int new_pr_etstatus=sign(NodeAsInteger("pr_etstatus",reqNode));
  TQuery Qry(&OraSession);
  Qry.SQLText=
    "SELECT pr_etstatus FROM trip_sets WHERE point_id=:point_id FOR UPDATE";
  Qry.CreateVariable("point_id",otInteger,point_id);
  Qry.Execute();
  if (Qry.Eof) throw AstraLocale::UserException("MSG.FLIGHT.CHANGED.REFRESH_DATA");

  int old_pr_etstatus=sign(Qry.FieldAsInteger("pr_etstatus"));
  if (old_pr_etstatus==0 && new_pr_etstatus<0)
  {
    TFltParams::setETSExchangeStatus(point_id, ETSExchangeStatus::NotConnected);
    TReqInfo::Instance()->LocaleToLog("EVT.ETICK.CANCEL_INTERACTIVE_WITH_ETC", evtFlt, point_id);
  }
  else
    if (old_pr_etstatus==new_pr_etstatus)
      throw AstraLocale::UserException("MSG.ETICK.FLIGHT_IN_THIS_MODE_ALREADY");
    else
      throw AstraLocale::UserException("MSG.ETICK.FLIGHT_CANNOT_BE_SET_IN_THIS_MODE");
}

//----------------------------------------------------------------------------------------------------------------

void SearchEMDsByTickNo(const set<Ticketing::TicketNum_t> &emds,
                        const edifact::KickInfo& kickInfo,
                        const OrigOfRequest &org,
                        const std::string& airline,
                        const Ticketing::FlightNum_t& flNum)
{
  try
  {
    for(set<Ticketing::TicketNum_t>::const_iterator e=emds.begin(); e!=emds.end(); ++e)
    {

      xmlNodePtr rootNode;
      XMLDoc xmlCtxt("context", rootNode, __FUNCTION__);
      SetProp(rootNode, "req_ctxt_id", kickInfo.reqCtxtId, ASTRA::NoExists);
      NewTextChild(rootNode, "emd_no", e->get());
      edifact::EmdDispByNum emdDispParams(org,
                                          xmlCtxt.text(),
                                          kickInfo,
                                          airline,
                                          flNum,
                                          *e);
      edifact::EmdDispRequestByNum ediReq(emdDispParams);
      ediReq.sendTlg();
    };
  }
  catch(const RemoteSystemContext::system_not_found &e)
  {
    throw AstraLocale::UserException("MSG.EMD.EDS_ADDR_NOT_DEFINED_FOR_FLIGHT",
                                     LEvntPrms() << PrmFlight("flight", e.airline(), e.flNum()?e.flNum().get():ASTRA::NoExists, "") );
  };
}

void EMDSearchInterface::SearchEMDByDocNo(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    LogTrace(TRACE3) << "SearchEMDByDocNo";

    set<Ticketing::TicketNum_t> emds;
    bool trueTermRequest=GetNode("emd_no", reqNode)!=NULL;

    Ticketing::TicketNum_t emdNum = checkDocNum(NodeAsString(trueTermRequest?"emd_no":"EmdNoEdit", reqNode), false);
    emds.insert(emdNum);

    int pointId = NodeAsInteger("point_id",reqNode);
    TTripInfo info;
    checkEDSExchange(pointId, true, info);

    std::string airline = getTripAirline(info);
    Ticketing::FlightNum_t flNum = getTripFlightNum(info);

    OrigOfRequest org(airline, *TReqInfo::Instance());

    edifact::KickInfo kickInfo=trueTermRequest?createKickInfo(AstraContext::SetContext("TERM_REQUEST",XMLTreeToText(reqNode->doc)), "EMDDisplay"):
                                               createKickInfo(ASTRA::NoExists, "EMDSearch");

    SearchEMDsByTickNo(emds, kickInfo, org, airline, flNum);

    if (trueTermRequest)
    {
      xmlNodePtr ediResCtxtNode;
      XMLDoc ediResCtxt("context", ediResCtxtNode, __FUNCTION__);
      addToEdiResponseCtxt(kickInfo.reqCtxtId, ediResCtxtNode->children, "");
    };
}

void EMDSearchInterface::KickHandler(XMLRequestCtxt *ctxt,
                                     xmlNodePtr reqNode, xmlNodePtr resNode)
{
    FuncIn(KickHandler);

    using namespace edifact;
    pRemoteResults res = RemoteResults::readSingle();
    if(res->status() != RemoteStatus::Success)
    {
        LogTrace(TRACE1) << "Remote error!";
        throw AstraLocale::UserException("MSG.ETICK.ETS_ERROR", LParams() << LParam("msg", "Remote error!"));
        //AddRemoteResultsMessageBox(resNode, *res); //!!!vlad
    }
    else
    {
        std::list<Emd> emdList = EmdEdifactReader::readList(res->tlgSource());
        if(emdList.size() == 1)
        {
            LogTrace(TRACE3) << "Show EMD:\n" << emdList.front();
            EmdDisp::doDisplay(EmdXmlView(resNode, emdList.front()));
        }
        else
        {
            LogError(STDLOG) << "Unable to display " << emdList.size() << " EMDs";
            throw AstraLocale::UserException("MSG.ETICK.EMD_LIST_VIEW_UNSUPPORTED");
        }
    }

    FuncOut(KickHandler);
}

static bool timeoutOccured(const std::list<edifact::RemoteResults> lRemRes)
{
    for(const edifact::RemoteResults& remRes: lRemRes) {
        if(remRes.status() == edifact::RemoteStatus::Timeout) {
            LogWarning(STDLOG) << "Timeout occured for edissesion: " << remRes.ediSession();
            return true;
        }
    }

    return false;
}

void EMDDisplayInterface::KickHandler(XMLRequestCtxt *ctxt,
                                      xmlNodePtr reqNode, xmlNodePtr resNode)
{
  list<edifact::RemoteResults> lres;
  edifact::RemoteResults::readDb(lres);

  if(timeoutOccured(lres)) {
      AstraLocale::showProgError("MSG.ETS_EDS_CONNECT_ERROR");
      throw UserException2();
  }

  int req_ctxt_id=NodeAsInteger("@req_ctxt_id",reqNode);
  XMLDoc ediResCtxt;
  getEdiResponseCtxt(req_ctxt_id, true, "EMDDisplayInterface::KickHandler", ediResCtxt);
  EdiErrorList errList;
  GetEdiError(NodeAsNode("/context",ediResCtxt.docPtr()), errList);
  if (!errList.empty())
    throw AstraLocale::UserException(errList.front().first.lexema_id,
                                     errList.front().first.lparams);

  map<string, Emd> emds;
  map<string, LexemaData> errors;
  for(list<edifact::RemoteResults>::const_iterator r=lres.begin(); r!=lres.end(); ++r)
  {
    string emd_no;

    XMLDoc ediSessCtxt;
    AstraEdifact::getEdiSessionCtxt(r->ediSession().get(), true, "EMDDisplayInterface::KickHandler", ediSessCtxt, false);
    if(ediSessCtxt.docPtr()!=NULL)
      emd_no=NodeAsString("/context/emd_no",ediSessCtxt.docPtr());

    if (emd_no.empty())
    {
      LogError(STDLOG) << "EMDDisplayInterface::KickHandler: strange situation - empty EDI_SESSION context";
      continue;
    };
    if (r->status() == edifact::RemoteStatus::Success)
    {
      std::list<Emd> emdList = EmdEdifactReader::readList(r->tlgSource());
      if (emdList.empty())
      {
        LogError(STDLOG) << "EMDDisplayInterface::KickHandler: strange situation - emdList.empty() for " << emd_no;
        continue;
      };
      if(emdList.size() == 1)
      {
        if (!emds.insert(make_pair(emd_no, emdList.front())).second)
          LogError(STDLOG) << "EMDDisplayInterface::KickHandler: duplicate EDI_SESSION context for " << emd_no;
      }
      else
      {
        LogError(STDLOG) << "Unable to display " << emdList.size() << " EMDs for " << emd_no;
        continue;
      };
    };
    if (r->status() == edifact::RemoteStatus::CommonError)
    {
      string msg=r->remark().empty()?r->ediErrCode():r->remark();
      if (!errors.insert(make_pair(emd_no, LexemaData("MSG.EMD.EDS_ERROR", LParams() << LParam("msg", msg)))).second)
        LogError(STDLOG) << "EMDDisplayInterface::KickHandler: duplicate EDI_SESSION context for " << emd_no;
    };
  };

  ostringstream text;
  bool unknownPnrExists=false;
  for(map<string, LexemaData>::const_iterator e=errors.begin(); e!=errors.end(); ++e)
  {
    string err, master_lexema_id;
    getLexemaText( e->second, err, master_lexema_id );
    text << "EMD#" << e->first << endl
         << err << endl
         << string(100,'=') << endl;
  };

  set<string> base_emds;
  for(map<string, Emd>::const_iterator e=emds.begin(); e!=emds.end(); ++e)
  {
    string base_emd_no;
    string emd_text=Ticketing::TickView::EmdXmlViewToText(e->second, unknownPnrExists, base_emd_no);
    if (base_emds.find(base_emd_no)!=base_emds.end()) continue;
    if (!base_emd_no.empty()) base_emds.insert(base_emd_no);
    text << emd_text << string(100,'=') << endl;
  };

  NewTextChild(resNode, "text", text.str());
}

//----------------------------------------------------------------------------------------------------------------

void EMDSystemUpdateInterface::SysUpdateEmdCoupon(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    if (TReqInfo::Instance()->duplicate) return;

    LogTrace(TRACE3) << __FUNCTION__;
    Ticketing::TicketNum_t etTickNum(NodeAsString("TickNoEdit", reqNode));
    CouponNum_t etCpnNum(NodeAsInteger("TickCpnNo", reqNode));

    Ticketing::TicketNum_t emdDocNum(NodeAsString("EmdNoEdit", reqNode));
    CouponNum_t emdCpnNum(NodeAsInteger("EmdCpnNo", reqNode));

    Ticketing::CpnStatAction::CpnStatAction_t cpnStatAction(CpnStatAction::disassociate);
    if( strcmp((const char*)reqNode->name, "AssociateEMD") == 0)
      cpnStatAction=CpnStatAction::associate;

    int pointId = NodeAsInteger("point_id",reqNode);
    TTripInfo info;
    checkEDSExchange(pointId, true, info);

    std::string airline = getTripAirline(info);
    Ticketing::FlightNum_t flNum = getTripFlightNum(info);

    OrigOfRequest org(airlineToXML(airline), *TReqInfo::Instance());

    edifact::KickInfo kickInfo=createKickInfo(ASTRA::NoExists, "EMDSystemUpdate");

    edifact::EmdDisassociateRequestParams disassocParams(org,
                                                         "",
                                                         kickInfo,
                                                         airline,
                                                         flNum,
                                                         Ticketing::TicketCpn_t(etTickNum, etCpnNum),
                                                         Ticketing::TicketCpn_t(emdDocNum, emdCpnNum),
                                                         cpnStatAction);
    edifact::EmdDisassociateRequest ediReq(disassocParams);
    //throw_if_request_dup("EMDSystemUpdateInterface::SysUpdateEmdCoupon");
    ediReq.sendTlg();
}

string TEMDSystemUpdateItem::traceStr() const
{
  ostringstream s;
  s << "airline_oper: " << airline_oper << " "
       "flt_no_oper: " << flt_no_oper << " "
       "et: " << et << " "
       "emd: " << emd << " "
       "action: " << CpnActionTraceStr(action);
  return s.str();
}

string TEMDChangeStatusKey::traceStr() const
{
  ostringstream s;
  s << "airline_oper: " << airline_oper << " "
       "flt_no_oper: " << flt_no_oper << " "
       "coupon_status: " << coupon_status;
  return s.str();
}

string TEMDChangeStatusItem::traceStr() const
{
  ostringstream s;
  s << "emd: " << emd;
  return s.str();
}

void EMDSystemUpdateInterface::EMDCheckDisassociation(const int point_id,
                                                      TEMDSystemUpdateList &emdList)
{
  emdList.clear();

  if (TReqInfo::Instance()->duplicate) return;

  AstraEdifact::TFltParams fltParams;
  if (!fltParams.get(point_id)) return;

  list< TEMDCtxtItem > emds;
  GetBagEMDDisassocList(point_id, fltParams.in_final_status, emds);

  string flight=GetTripName(fltParams.fltInfo,ecNone,true,false);

  if (!emds.empty() && fltParams.eds_no_exchange)
    throw AstraLocale::UserException("MSG.EMD.INTERACTIVE_MODE_NOT_ALLOWED");

  for(list< TEMDCtxtItem >::iterator i=emds.begin(); i!=emds.end(); ++i)
  {
    if (i->pax.tkn.no.empty() ||
        i->pax.tkn.coupon==ASTRA::NoExists ||
        i->pax.tkn.rem!="TKNE")
    {
      //  ���������� �஢��� ������ ���樠樨/����樠樨 ��-�� ������⢨� ��� �������� ���ଠ樨 �� �. ������ ���ᠦ��
      //  �������� ���� ����� ⮭��
      TLogLocale event;
      event.ev_type=ASTRA::evtPay;
      event.lexema_id= i->emd.action==CpnStatAction::associate?"EVT.EMD_ASSOCIATION_MISTAKE":
                                                               "EVT.EMD_DISASSOCIATION_MISTAKE";
      event.prms << PrmSmpl<std::string>("emd_no", i->emd.no)
                 << PrmSmpl<int>("emd_coupon", i->emd.coupon);

      if (i->pax.tkn.rem!="TKNE" || i->pax.tkn.no.empty())
        event.prms << PrmLexema("err", "MSG.ETICK.NUMBER_NOT_SET");
      else
        event.prms << PrmLexema("err", "MSG.ETICK.COUPON_NOT_SET", LEvntPrms() << PrmSmpl<std::string>("etick", i->pax.tkn.no));

      AstraEdifact::ProcEvent(event, *i, NULL, false);
      continue;
    };

    TEMDocItem EMDocItem;
    EMDocItem.fromDB(TEMDocItem::SystemUpdate, i->emd.no, i->emd.coupon, false);
    if (!EMDocItem.empty())
    {
      if (EMDocItem.emd.action==i->emd.action) continue;
      if (i->pax.tkn.empty())
      {
        i->pax.tkn.no=EMDocItem.et.no;
        i->pax.tkn.coupon=EMDocItem.et.coupon;
        i->pax.tkn.rem="TKNE";
      }
    }
    else
    {
      //� emdocs ��� ��祣�
      if (i->emd.action==CpnStatAction::associate) continue; //��⠥� �� ���樠�� ᤥ���� �� 㬮�砭��
    }

    emdList.push_back(make_pair(TEMDSystemUpdateItem(), XMLDoc()));

    TEMDSystemUpdateItem &item=emdList.back().first;
    item.airline_oper=getTripAirline(fltParams.fltInfo);
    item.flt_no_oper=getTripFlightNum(fltParams.fltInfo);
    item.et=Ticketing::TicketCpn_t(i->pax.tkn.no, i->pax.tkn.coupon);
    item.emd=Ticketing::TicketCpn_t(i->emd.no, i->emd.coupon);
    item.action=i->emd.action;

    XMLDoc &ctxt=emdList.back().second;
    if (ctxt.docPtr()==NULL)
    {
      ctxt.set("context");
      if (ctxt.docPtr()==NULL)
        throw EXCEPTIONS::Exception("%s: CreateXMLDoc failed", __FUNCTION__);
      xmlNodePtr node=NewTextChild(NodeAsNode("/context",ctxt.docPtr()),"emdoc");

      i->flight=flight;
      i->toXML(node);
    }
  }
}

bool EMDSystemUpdateInterface::EMDChangeDisassociation(const edifact::KickInfo &kickInfo,
                                                       const TEMDSystemUpdateList &emdList)
{
  bool result=false;

  try
  {
    for(TEMDSystemUpdateList::const_iterator i=emdList.begin();i!=emdList.end();i++)
    {

      xmlNodePtr rootNode=NodeAsNode("/context",i->second.docPtr());

      SetProp(rootNode, "req_ctxt_id", kickInfo.reqCtxtId, ASTRA::NoExists);
      TOriginCtxt::toXML(rootNode);

      string ediCtxt=XMLTreeToText(i->second.docPtr());

      edifact::EmdDisassociateRequestParams disassocParams(OrigOfRequest(airlineToXML(i->first.airline_oper),
                                                                         *TReqInfo::Instance()),
                                                           ediCtxt,
                                                           kickInfo,
                                                           i->first.airline_oper,
                                                           i->first.flt_no_oper,
                                                           i->first.et,
                                                           i->first.emd,
                                                           i->first.action);
      edifact::EmdDisassociateRequest ediReq(disassocParams);
      //throw_if_request_dup("EMDSystemUpdateInterface::EMDChangeDisassociation");
      ediReq.sendTlg();

      LogTrace(TRACE5) << __FUNCTION__ << ": " << i->first.traceStr();

      result=true;
    }
  }
  catch(const Ticketing::RemoteSystemContext::system_not_found &e)
  {
    throw AstraLocale::UserException("MSG.EMD.EDS_ADDR_NOT_DEFINED_FOR_FLIGHT",
                                     LEvntPrms() << PrmFlight("flight", e.airline(), e.flNum()?e.flNum().get():ASTRA::NoExists, "") );
  };

  return result;
}

void EMDSystemUpdateInterface::KickHandler(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    FuncIn(KickHandler);

    using namespace edifact;
    pRemoteResults res = RemoteResults::readSingle();
    MakeAnAnswer(resNode, res);

    FuncOut(KickHandler);
}

void EMDSystemUpdateInterface::MakeAnAnswer(xmlNodePtr resNode, edifact::pRemoteResults remRes)
{
    using namespace edifact;
    bool success = remRes->status() == RemoteStatus::Success;
    LogTrace(TRACE3) << "Handle " << ( success? "successfull" : "unsuccessfull") << " disassociation";
    xmlNodePtr answerNode = newChild(resNode, "result");
    xmlSetProp(answerNode, "status", remRes->status()->description());
    if(!success)
    {
        xmlSetProp(answerNode, "edi_error_code", remRes->ediErrCode());
        xmlSetProp(answerNode, "remark", remRes->remark());
    }
}

//----------------------------------------------------------------------------------------------------------------

void ChangeAreaStatus(TETCheckStatusArea area, XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  //�� �㭪�� ��뢠���� ⮫쪮 �� �⫮������ ᬥ�� ����� ��, �� ��뢠���� �� ���-ॣ����樨
  bool tckin_version=GetNode("segments",reqNode)!=NULL;

  bool only_one;
  xmlNodePtr segNode;
  if (tckin_version)
  {
    segNode=NodeAsNode("segments/segment",reqNode);
    only_one=segNode->next==NULL;
  }
  else
  {
    segNode=reqNode;
    only_one=true;
  }
  TETChangeStatusList mtick;
  for(;segNode!=NULL;segNode=segNode->next)
  {
    set<int> ids;
    switch (area)
    {
      case csaFlt:
        ids.insert(NodeAsInteger("point_id",segNode));
        break;
      case csaGrp:
        ids.insert(NodeAsInteger("grp_id",segNode));
        break;
      case csaPax:
        {
          xmlNodePtr node=GetNode("passengers/pax_id", segNode);
          if (node!=NULL)
          {
            for(; node!=NULL; node=node->next)
              ids.insert(NodeAsInteger(node));
          }
          else ids.insert(NodeAsInteger("pax_id",segNode));
        }
        break;
      default: throw EXCEPTIONS::Exception("ChangeAreaStatus: wrong area");
    }

    if (ids.empty()) throw EXCEPTIONS::Exception("ChangeAreaStatus: ids.empty()");

    xmlNodePtr node=GetNode("check_point_id",segNode);
    int check_point_id=(node==NULL?NoExists:NodeAsInteger(node));

    for(set<int>::const_iterator i=ids.begin(); i!=ids.end(); ++i)
    try
    {
      ETStatusInterface::ETCheckStatus(*i,
                                       area,
                                       (i==ids.begin()?check_point_id:NoExists),
                                       false,
                                       mtick);
    }
    catch(const AstraLocale::UserException &e)
    {
      if (!only_one)
      {
        TQuery Qry(&OraSession);
        Qry.Clear();
        switch (area)
        {
          case csaFlt:
            Qry.SQLText=
              "SELECT airline,flt_no,suffix,airp,scd_out "
              "FROM points "
              "WHERE point_id=:id";
            break;
          case csaGrp:
            Qry.SQLText=
              "SELECT airline,flt_no,suffix,airp,scd_out "
              "FROM points,pax_grp "
              "WHERE points.point_id=pax_grp.point_dep AND "
              "      grp_id=:id";
            break;
          case csaPax:
            Qry.SQLText=
              "SELECT airline,flt_no,suffix,airp,scd_out "
              "FROM points,pax_grp,pax "
              "WHERE points.point_id=pax_grp.point_dep AND "
              "      pax_grp.grp_id=pax.grp_id AND "
              "      pax_id=:id";
            break;
          default: throw;
        }
        Qry.CreateVariable("id",otInteger,*i);
        Qry.Execute();
        if (!Qry.Eof)
        {
          TTripInfo fltInfo(Qry);
          throw AstraLocale::UserException("WRAP.FLIGHT",
                                           LParams()<<LParam("flight",GetTripName(fltInfo,ecNone,true,false))
                                                    <<LParam("text",e.getLexemaData( )));
        }
        else
          throw;
      }
      else
        throw;
    }
    if (!tckin_version) break; //���� �ନ���
  }

  if (!mtick.empty())
  {
    if (!ETStatusInterface::ETChangeStatus(reqNode,mtick))
      throw EXCEPTIONS::Exception("ChangeAreaStatus: Wrong mtick");

/*  �� �����, ����� �ନ���� ���� �⫮������ ���⢥ত���� ⮦� ��ࠡ��뢠�� �१ ets_connect_error!!!
    if (TReqInfo::Instance()->client_type==ctTerm)
    {
      xmlNodePtr errNode=NewTextChild(resNode,"ets_connect_error");
      SetProp(errNode,"internal_msgid",get_internal_msgid_hex());
      NewTextChild(errNode,"message",getLocaleText("MSG.ETS_CONNECT_ERROR"));
    }
    else*/
      AstraLocale::showProgError("MSG.ETS_CONNECT_ERROR");
  }
}

void ETStatusInterface::ChangePaxStatus(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  ChangeAreaStatus(csaPax,ctxt,reqNode,resNode);
}

void ETStatusInterface::ChangeGrpStatus(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  ChangeAreaStatus(csaGrp,ctxt,reqNode,resNode);
}

void ETStatusInterface::ChangeFltStatus(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  ChangeAreaStatus(csaFlt,ctxt,reqNode,resNode);
}

void ETStatusInterface::ETRollbackStatus(xmlDocPtr ediResDocPtr,
                                         bool check_connect)
{
  if (ediResDocPtr==NULL) return;

  vector<int> point_ids;

  //ProgTrace(TRACE5, "ediResDoc=%s", XMLTreeToText(ediResDocPtr).c_str());

  xmlNodePtr ticketNode=GetNode("/context/tickets",ediResDocPtr);
  if (ticketNode!=NULL) ticketNode=ticketNode->children;
  for(xmlNodePtr node=ticketNode;node!=NULL;node=node->next)
  {
    //横� �� ����⠬
    xmlNodePtr node2=node->children;

    ProgTrace(TRACE5,"ETRollbackStatus: ticket_no=%s coupon_no=%d",
              NodeAsStringFast("ticket_no",node2),
              NodeAsIntegerFast("coupon_no",node2));

    if (GetNodeFast("coupon_status",node2)==NULL) continue;
    int point_id=NodeAsIntegerFast("prior_point_id",node2,
                                   NodeAsIntegerFast("point_id",node2));
    if (find(point_ids.begin(),point_ids.end(),point_id)==point_ids.end())
      point_ids.push_back(point_id);
  }

  TETChangeStatusList mtick;
  for(vector<int>::iterator i=point_ids.begin();i!=point_ids.end();i++)
  {
    ProgTrace(TRACE5,"ETRollbackStatus: rollback point_id=%d",*i);
    ETStatusInterface::ETCheckStatusForRollback(*i,ediResDocPtr,false,mtick);
  }
  ETStatusInterface::ETChangeStatus(NULL,mtick);
}

xmlNodePtr TETChangeStatusList::addTicket(const TETChangeStatusKey &key,
                                          const TETickItem& ETItem,
                                          const AstraEdifact::TFltParams& fltParams,
                                          const std::string& subclass)
{
  return addTicket(key,
                   ETItem.makeTicket(fltParams,
                                     subclass,
                                     CouponStatus(key.coupon_status)),
                   fltParams.strictlySingleTicketInTlg());
}

xmlNodePtr TETChangeStatusList::addTicket(const TETChangeStatusKey &key,
                                          const Ticketing::Ticket &tick,
                                          bool onlySingleTicketInTlg)
{
  size_t MaxTicketsInTlg = onlySingleTicketInTlg?1:MAX_TICKETS_IN_TLG; //���宥 �襭��, ���� ������뢠���� � ����ன��� et_addr_set
//  if(TReqInfo::Instance()->api_mode) {
//      MaxTicketsInTlg = 99;
//  }
  if(inTestMode()) {
      MaxTicketsInTlg = 1;
  }
  LogTrace(TRACE5) << "MaxTicketsInTlg=" << MaxTicketsInTlg;
  if ((*this)[key].empty() ||
      (*this)[key].back().first.size()>=MaxTicketsInTlg)
  {
    (*this)[key].push_back(TETChangeStatusItem());
  }

  TETChangeStatusItem &ltick=(*this)[key].back();
  if (ltick.second.docPtr()==NULL)
  {
    ltick.second.set("context");
    if (ltick.second.docPtr()==NULL)
      throw EXCEPTIONS::Exception("ETCheckStatus: CreateXMLDoc failed");
    NewTextChild(NodeAsNode("/context",ltick.second.docPtr()),"tickets");
  }
  ltick.first.push_back(tick);
  return NewTextChild(NodeAsNode("/context/tickets",ltick.second.docPtr()),"ticket");
}

void ETStatusInterface::ETCheckStatusForRollback(int point_id,
                                                 xmlDocPtr ediResDocPtr,
                                                 bool check_connect,
                                                 TETChangeStatusList &mtick)
{
  //mtick.clear(); ������塞 㦥 � ������������

  if (ediResDocPtr==NULL) return;
  if (TReqInfo::Instance()->duplicate) return;

  TFltParams fltParams;
  if (fltParams.get(point_id))
  {
    try
    {
      if ((fltParams.ets_exchange_status!=ETSExchangeStatus::NotConnected || check_connect) && !fltParams.ets_no_exchange)
      {
        TQuery Qry(&OraSession);
        Qry.Clear();
        Qry.SQLText=PaxETList::GetSQL(PaxETList::allCheckedByTickNoAndCouponNo);
        Qry.DeclareVariable("ticket_no",otString);
        Qry.DeclareVariable("coupon_no",otInteger);

        TETChangeStatusKey key;
        bool init_edi_addrs=false;

        xmlNodePtr ticketNode=NodeAsNode("/context/tickets",ediResDocPtr);
        for(ticketNode=ticketNode->children;ticketNode!=NULL;ticketNode=ticketNode->next)
        {
          //横� �� ����⠬
          xmlNodePtr node2=ticketNode->children;
          if (GetNodeFast("coupon_status",node2)==NULL) continue;
          if (NodeAsIntegerFast("prior_point_id",node2,
                                NodeAsIntegerFast("point_id",node2))!=point_id) continue;

          string ticket_no=NodeAsStringFast("ticket_no",node2);
          int coupon_no=NodeAsIntegerFast("coupon_no",node2);

          string airp_dep=NodeAsStringFast("prior_airp_dep",node2,
                                           NodeAsStringFast("airp_dep",node2));
          string airp_arv=NodeAsStringFast("prior_airp_arv",node2,
                                           NodeAsStringFast("airp_arv",node2));
          CouponStatus status=CouponStatus::fromDispCode(NodeAsStringFast("coupon_status",node2));
          CouponStatus prior_status=CouponStatus::fromDispCode(NodeAsStringFast("prior_coupon_status",node2));
          //���� ���᫨�� ॠ��� �����
          Qry.SetVariable("ticket_no", ticket_no);
          Qry.SetVariable("coupon_no", coupon_no);
          Qry.Execute();

          CouponStatus real_status(CouponStatus::OriginalIssue);
          TETCtxtItem ETCtxt(*TReqInfo::Instance(), point_id);
          if (!Qry.Eof)
          {
            ETCtxt.paxFromDB(Qry, false);
            real_status=calcPaxCouponStatus(ETCtxt.pax.refuse,
                                            ETCtxt.pax.pr_brd,
                                            fltParams.in_final_status);
          };

          if (fltParams.equalETStatus(status,real_status)) continue;

          TETickItem ETItem(ticket_no, coupon_no, point_id, airp_dep, airp_arv, real_status);
          if (ETStatusInterface::ChangeStatusLocallyOnly(fltParams, ETItem, ETCtxt)) continue;
          if (!init_edi_addrs)
          {
            key.airline_oper=fltParams.fltInfo.airline;
            key.flt_no_oper=fltParams.fltInfo.flt_no;
            if (!get_et_addr_set(fltParams.fltInfo.airline,fltParams.fltInfo.flt_no,key.addrs))
            {
              ostringstream flt;
              flt << ElemIdToCodeNative(etAirline,fltParams.fltInfo.airline)
                  << setw(3) << setfill('0') << fltParams.fltInfo.flt_no;
              throw AstraLocale::UserException("MSG.ETICK.ETS_ADDR_NOT_DEFINED_FOR_FLIGHT",
                                               LParams() << LParam("flight", flt.str()));
            }
            init_edi_addrs=true;
          }
          key.coupon_status=real_status->codeInt();

          ProgTrace(TRACE5,"status=%s prior_status=%s real_status=%s",
                           status->dispCode(),prior_status->dispCode(),real_status->dispCode());

          xmlNodePtr node=mtick.addTicket(key, ETItem, fltParams);

          NewTextChild(node,"ticket_no",ticket_no);
          NewTextChild(node,"coupon_no",coupon_no);
          NewTextChild(node,"point_id",point_id);
          NewTextChild(node,"airp_dep",airp_dep);
          NewTextChild(node,"airp_arv",airp_arv);
          NewTextChild(node,"flight",GetTripName(fltParams.fltInfo,ecNone,true,false));
          if (GetNodeFast("grp_id",node2)!=NULL)
          {
            NewTextChild(node,"grp_id",NodeAsIntegerFast("grp_id",node2));
            NewTextChild(node,"pax_id",NodeAsIntegerFast("pax_id",node2));
            if (GetNodeFast("reg_no",node2)!=NULL)
              NewTextChild(node,"reg_no",NodeAsIntegerFast("reg_no",node2));
            NewTextChild(node,"pax_full_name",NodeAsStringFast("pax_full_name",node2));
            NewTextChild(node,"pers_type",NodeAsStringFast("pers_type",node2));
          }

          ProgTrace(TRACE5,"ETCheckStatus %s/%d->%s",
                           ticket_no.c_str(),
                           coupon_no,
                           real_status->dispCode());
        }
      }
    }
    catch(const CheckIn::UserException&)
    {
      throw;
    }
    catch(const AstraLocale::UserException &e)
    {
      throw CheckIn::UserException(e.getLexemaData(), point_id);
    }
  }
}

void TEMDChangeStatusList::addEMD(const TEMDChangeStatusKey &key,
                                  const TEMDCtxtItem &item,
                                  bool control_method)
{
  if (control_method) return; //���� �� �����塞 ����� ��� EMD �� ����஫쭮� ��⮤�, �� �� �����....

  TEMDChangeStatusList::iterator i=find(key);
  if (i==end()) i=insert(make_pair(key, list<TEMDChangeStatusItem>())).first;
  if (i==end()) throw EXCEPTIONS::Exception("%s: i==end()", __FUNCTION__);

  list<TEMDChangeStatusItem>::iterator j=i->second.insert(i->second.end(), TEMDChangeStatusItem());
  j->emd=Ticketing::TicketCpn_t(item.emd.no, item.emd.coupon);
  xmlNodePtr emdocsNode=NULL;
  if (j->ctxt.docPtr()==NULL)
  {
    j->ctxt.set("context");
    if (j->ctxt.docPtr()==NULL) throw EXCEPTIONS::Exception("%s: j->ctxt.docPtr()==NULL", __FUNCTION__);
    emdocsNode=NewTextChild(NodeAsNode("/context",j->ctxt.docPtr()),"emdocs");
  }
  else
    emdocsNode=NodeAsNode("/context/emdocs",j->ctxt.docPtr());

  item.toXML(NewTextChild(emdocsNode, "emdoc"));
}


void EMDStatusInterface::EMDCheckStatus(const int grp_id,
                                        const CheckIn::TServicePaymentListWithAuto &prior_payment,
                                        TEMDChangeStatusList &emdList)
{
  if (TReqInfo::Instance()->duplicate) return;
  TAdvTripInfo fltInfo;
  if (!fltInfo.getByGrpId(grp_id)) return;

  TFltParams fltParams;
  if (!fltParams.get(fltInfo)) return;
  if (fltParams.eds_no_exchange) return;

  list<TEMDCtxtItem> added_emds, deleted_emds;
  GetEMDStatusList(grp_id, fltParams.in_final_status, prior_payment, added_emds, deleted_emds);
  string flight=GetTripName(fltParams.fltInfo,ecNone,true,false);

  for(int pass=0; pass<2; pass++)
  {
    list<TEMDCtxtItem> &emds = pass==0?added_emds:deleted_emds;
    for(list<TEMDCtxtItem>::iterator e=emds.begin(); e!=emds.end(); ++e)
    {
      if (e->paxUnknown())
      {
        if (pass==0)
          throw UserException("MSG.EMD_MANUAL_INPUT_TEMPORARILY_UNAVAILABLE",
                              LParams() << LParam("emd", e->no_str()));
        else
          continue;
      };

      CouponStatus paxStatus=calcPaxCouponStatus(e->pax.refuse,
                                                 e->pax.pr_brd,
                                                 fltParams.in_final_status);
      if (paxStatus==CouponStatus::Flown) continue; //䨭���� �����

      if (paxStatus==CouponStatus::OriginalIssue ||
          paxStatus==CouponStatus::Unavailable) continue;  //���ᠦ�� ࠧॣ���������

      e->point_id=fltInfo.point_id;
      e->flight=flight;
      e->emd.status = pass==0?CouponStatus(paxStatus):CouponStatus(CouponStatus::OriginalIssue);

      TEMDChangeStatusKey key;
      key.airline_oper=fltParams.fltInfo.airline;
      key.flt_no_oper=fltParams.fltInfo.flt_no;
      key.coupon_status=e->emd.status;

      emdList.addEMD(key, *e, fltParams.control_method);
    };
  };

}

bool EMDStatusInterface::EMDChangeStatus(const edifact::KickInfo &kickInfo,
                                         const TEMDChangeStatusList &emdList)
{
  bool result=false;

  try
  {
    for(TEMDChangeStatusList::const_iterator i=emdList.begin(); i!=emdList.end(); ++i)
    {
      for(list<TEMDChangeStatusItem>::const_iterator j=i->second.begin(); j!=i->second.end(); ++j)
      {
        xmlNodePtr rootNode=NodeAsNode("/context",j->ctxt.docPtr());

        SetProp(rootNode, "req_ctxt_id", kickInfo.reqCtxtId, ASTRA::NoExists);
        TOriginCtxt::toXML(rootNode);

        string ediCtxt=XMLTreeToText(j->ctxt.docPtr());
        //ProgTrace(TRACE5, "ediCosCtxt=%s", ediCtxt.c_str());

        edifact::EmdCosParams cosParams(OrigOfRequest(airlineToXML(i->first.airline_oper), *TReqInfo::Instance()),
                                        ediCtxt,
                                        kickInfo,
                                        i->first.airline_oper,
                                        Ticketing::FlightNum_t(i->first.flt_no_oper),
                                        j->emd.ticket(),
                                        j->emd.cpn(),
                                        i->first.coupon_status);

        edifact::EmdCosRequest ediReq(cosParams);
        //throw_if_request_dup("EMDStatusInterface::EMDChangeStatus");
        ediReq.sendTlg();

        LogTrace(TRACE5) << __FUNCTION__ << ": " << i->first.traceStr() << " " << j->traceStr();

        result=true;
      };
    };
  }
  catch(const Ticketing::RemoteSystemContext::system_not_found &e)
  {
    throw AstraLocale::UserException("MSG.EMD.EDS_ADDR_NOT_DEFINED_FOR_FLIGHT",
                                     LEvntPrms() << PrmFlight("flight", e.airline(), e.flNum()?e.flNum().get():ASTRA::NoExists, "") );
  };

  return result;
}

bool ETStatusInterface::ChangeStatusLocallyOnly(const TFltParams& fltParams,
                                                const TETickItem& item,
                                                const TETCtxtItem& ctxt)
{
  if (!fltParams.control_method) return false;

  LogTrace(TRACE5) << __FUNCTION__;

  try
  {
    changeOfStatusWcCoupon(item.et.no, item.et.coupon, item.et.status, true);
    if (fltParams.in_final_status &&
        (item.et.status==CouponStatus::OriginalIssue ||
         item.et.status==CouponStatus::Flown))
    {
      LogTrace(TRACE5) << __FUNCTION__ << ": try to return control for " << item.et.no_str()
                                       << " in_final_status=" << boolalpha << fltParams.in_final_status
                                       << " coupon_status=" << item.et.status->dispCode();
      return false; //�⤠�� ����஫�
    }
    item.toDB(TETickItem::ChangeOfStatus);
    //����襬 ��������� ����� � ���
    TLogLocale event;
    event.ev_type=ASTRA::evtPax;
    event.lexema_id="EVT.ETICKET_CHANGE_STATUS";
    event.prms << PrmSmpl<std::string>("ticket_no", item.et.no)
               << PrmSmpl<int>("coupon_no", item.et.coupon)
               << PrmSmpl<std::string>("disp_code", item.et.status->dispCode());
    AstraEdifact::ProcEvent(event, ctxt, nullptr, false);

  }
  catch(const AirportControlNotFound&)
  {
    ProgTrace(TRACE5, "%s: AirportControlNotFound for %s (point_id=%d)",
                      __FUNCTION__, item.et.no_str().c_str(), item.point_id);
//    if (!readWcCoupon(BaseTables::Company(fltParams.fltInfo.airline)->ida(),
//                      Ticketing::TicketNum_t(ticket_no),
//                      Ticketing::CouponNum_t(coupon_no)))
//      //᪮॥ �ᥣ� ����� �㯮� ࠡ�⠫ �� ���ࠪ⨢�
//      return false;
  }
  catch(const WcCouponNotFound&)
  {
    ProgTrace(TRACE5, "%s: WcCouponNotFound for %s (point_id=%d)",
                      __FUNCTION__, item.et.no_str().c_str(), item.point_id);
  }

  return true;
}

bool ETStatusInterface::ToDoNothingWhenChangingStatus(const TFltParams& fltParams,
                                                      const TETickItem& item)
{
   bool res=item.et.status == CouponStatus::Airport &&
            existsAirportControl(item.et.no, item.et.coupon, false);
   LogTrace(TRACE5) << __FUNCTION__ << " returned " << std::boolalpha << res;
   return res;
}

void ETStatusInterface::AfterReceiveAirportControl(const Ticketing::WcCoupon& cpn)
{
  list<TETCtxtItem> ETCtxtItems;
  PaxETList::GetByCouponNoFromTlg(cpn.tickNum().get(), cpn.cpnNum().get(), ETCtxtItems);
  if (ETCtxtItems.empty())
    PaxETList::GetCheckedByCouponNo(cpn.tickNum().get(), cpn.cpnNum().get(), ETCtxtItems);
  for(const TETCtxtItem& ctxt : ETCtxtItems)
  {
    TLogLocale event;
    event.ev_type=ASTRA::evtFlt;
    event.lexema_id="EVT.ETICKET_CONTROL_RECEIVED";
    event.prms << PrmSmpl<std::string>("ticket_no", ctxt.et.no)
               << PrmSmpl<int>("coupon_no", ctxt.et.coupon);

    AstraEdifact::ProcEvent(event, ctxt, NULL, false);
  }
}

void ETStatusInterface::AfterReturnAirportControl(const Ticketing::WcCoupon& cpn)
{
  list<TETCtxtItem> ETCtxtItems;
  PaxETList::GetCheckedByCouponNo(cpn.tickNum().get(), cpn.cpnNum().get(), ETCtxtItems);
  if (ETCtxtItems.empty())
    PaxETList::GetByCouponNoFromTlg(cpn.tickNum().get(), cpn.cpnNum().get(), ETCtxtItems);
  for(const TETCtxtItem& ctxt : ETCtxtItems)
  {
    TLogLocale event;
    event.ev_type=ASTRA::evtFlt;
    event.lexema_id="EVT.ETICKET_CONTROL_RETURNED";
    event.prms << PrmSmpl<std::string>("ticket_no", ctxt.et.no)
               << PrmSmpl<int>("coupon_no", ctxt.et.coupon)
               << PrmSmpl<std::string>("disp_code", cpn.status()->dispCode());

    AstraEdifact::ProcEvent(event, ctxt, NULL, false);
  }
}

bool ETStatusInterface::ReturnAirportControl(const TFltParams& fltParams,
                                             const TETickItem& item)
{
  LogTrace(TRACE5) << __FUNCTION__;

  try
  {
    changeOfStatusWcCoupon(item.et.no, item.et.coupon, item.et.status, true);
    LogTrace(TRACE5) << __FUNCTION__ << ": " << item.et.no_str()
                                     << " in_final_status=" << boolalpha << fltParams.in_final_status
                                     << " coupon_status=" << item.et.status->dispCode();

    return returnWcCoupon(Ticketing::TicketNum_t(item.et.no),
                          Ticketing::CouponNum_t(item.et.coupon),
                          false);
  }
  catch(const AirportControlNotFound&)
  {
    ProgTrace(TRACE5, "%s: AirportControlNotFound for %s (point_id=%d)",
                      __FUNCTION__, item.et.no_str().c_str(), item.point_id);
  }
  catch(const WcCouponNotFound&)
  {
    ProgTrace(TRACE5, "%s: WcCouponNotFound for %s (point_id=%d)",
                      __FUNCTION__, item.et.no_str().c_str(), item.point_id);
  }
  return false;
}

void ETStatusInterface::ETCheckStatus(int id,
                                      TETCheckStatusArea area,
                                      int check_point_id,  //�.�. NoExists
                                      bool check_connect,
                                      TETChangeStatusList &mtick,
                                      bool before_checkin)
{
  LogTrace(TRACE5) << __func__
                   << ": id=" << id
                   << ", area=" << area;
  //mtick.clear(); ������塞 㦥 � ������������
  if (TReqInfo::Instance()->duplicate) return;
  int point_id=NoExists;
  TQuery Qry(&OraSession);
  Qry.Clear();
  switch (area)
  {
    case csaFlt:
      point_id=id;
      break;
    case csaGrp:
      Qry.SQLText="SELECT point_dep FROM pax_grp WHERE pax_grp.grp_id=:grp_id";
      Qry.CreateVariable("grp_id",otInteger,id);
      Qry.Execute();
      if (!Qry.Eof) point_id=Qry.FieldAsInteger("point_dep");
      break;
    case csaPax:
      Qry.SQLText="SELECT point_dep FROM pax_grp,pax WHERE pax_grp.grp_id=pax.grp_id AND pax.pax_id=:pax_id";
      Qry.CreateVariable("pax_id",otInteger,id);
      Qry.Execute();
      if (!Qry.Eof) point_id=Qry.FieldAsInteger("point_dep");
      break;
    default: ;
  }

  TFltParams fltParams;
  if (point_id!=NoExists && fltParams.get(point_id))
  {
    try
    {
      if (check_point_id!=NoExists && check_point_id!=point_id) check_point_id=point_id;

      if ((fltParams.ets_exchange_status!=ETSExchangeStatus::NotConnected || check_connect) && !fltParams.ets_no_exchange)
      {
        Qry.Clear();
        ostringstream sql;
        sql <<
          "SELECT pax_grp.airp_dep, pax_grp.airp_arv, pax_grp.class, pax.* "
          "FROM pax_grp,pax "
          "WHERE pax_grp.grp_id=pax.grp_id AND pax_grp.status<>:transit AND "
          "      pax.ticket_rem='TKNE' AND "
          "      pax.ticket_no IS NOT NULL AND pax.coupon_no IS NOT NULL AND ";
        switch (area)
        {
          case csaFlt:
            sql << " pax_grp.point_dep=:point_id ";
            Qry.CreateVariable("point_id",otInteger,id);
            break;
          case csaGrp:
            sql << " pax.grp_id=:grp_id ";
            Qry.CreateVariable("grp_id",otInteger,id);
            break;
          case csaPax:
            sql << " pax.pax_id=:pax_id ";
            Qry.CreateVariable("pax_id",otInteger,id);
            break;
          default: ;
        }
        //�� ���� ���ᠦ�஢ � ��������� ����⮬/�㯮��� �ਮ���� ���� ��ࠧॣ����஢����
        sql << "ORDER BY pax.ticket_no,pax.coupon_no,DECODE(pax.refuse,NULL,0,1)";

        Qry.SQLText=sql.str().c_str();
        Qry.CreateVariable("transit", otString, EncodePaxStatus(ASTRA::psTransit));
        Qry.Execute();
        if (!Qry.Eof)
        {
          string ticket_no;
          int coupon_no=-1;
          TETChangeStatusKey key;
          bool init_edi_addrs=false;
          for(;!Qry.Eof;Qry.Next())
          {
            TETCtxtItem ETCtxt(*TReqInfo::Instance(), point_id);
            ETCtxt.paxFromDB(Qry, false);

            if (ticket_no==Qry.FieldAsString("ticket_no") &&
                coupon_no==Qry.FieldAsInteger("coupon_no")) continue; //�㡫�஢���� ����⮢

            ticket_no=Qry.FieldAsString("ticket_no");
            coupon_no=Qry.FieldAsInteger("coupon_no");

            TETickItem EticketItem;
            EticketItem.fromDB(ticket_no, coupon_no, TETickItem::ChangeOfStatus, false /*lock*/);

            string airp_dep=Qry.FieldAsString("airp_dep");
            string airp_arv=Qry.FieldAsString("airp_arv");
            string cabin_subclass=Qry.FieldIsNULL("cabin_subclass")?Qry.FieldAsString("subclass"):
                                                                    Qry.FieldAsString("cabin_subclass");

            CouponStatus status;
            if (EticketItem.empty() || EticketItem.et.status == CouponStatus(CouponStatus::Unavailable)) // IS NULL
              status=CouponStatus(CouponStatus::OriginalIssue);//CouponStatus::Notification ???
            else
              status=EticketItem.et.status;

            CouponStatus real_status;
            real_status=calcPaxCouponStatus(ETCtxt.pax.refuse,
                                            ETCtxt.pax.pr_brd,
                                            fltParams.in_final_status);

            if (!fltParams.equalETStatus(status, real_status) ||
                (!EticketItem.empty() &&
                 EticketItem.point_id != ASTRA::NoExists &&
                 (EticketItem.point_id!=point_id ||
                  EticketItem.airp_dep!=airp_dep ||
                  EticketItem.airp_arv!=airp_arv)))
            {
              TETickItem ETItem(ticket_no, coupon_no, point_id, airp_dep, airp_arv, real_status);
              if (ETStatusInterface::ChangeStatusLocallyOnly(fltParams, ETItem, ETCtxt)) continue;

              if (!init_edi_addrs)
              {
                key.airline_oper=fltParams.fltInfo.airline;
                key.flt_no_oper=fltParams.fltInfo.flt_no;
                if (!get_et_addr_set(fltParams.fltInfo.airline,fltParams.fltInfo.flt_no,key.addrs))
                {
                  ostringstream flt;
                  flt << ElemIdToCodeNative(etAirline,fltParams.fltInfo.airline)
                      << setw(3) << setfill('0') << fltParams.fltInfo.flt_no;
                  throw AstraLocale::UserException("MSG.ETICK.ETS_ADDR_NOT_DEFINED_FOR_FLIGHT",
                                                   LParams() << LParam("flight", flt.str()));
                }
                init_edi_addrs=true;
              }
              key.coupon_status=real_status->codeInt();

              ProgTrace(TRACE5,"status=%s real_status=%s",status->dispCode(),real_status->dispCode());

              xmlNodePtr node=mtick.addTicket(key, ETItem, fltParams, cabin_subclass);

              NewTextChild(node,"ticket_no",ticket_no);
              NewTextChild(node,"coupon_no",coupon_no);
              NewTextChild(node,"point_id",point_id);
              NewTextChild(node,"airp_dep",airp_dep);
              NewTextChild(node,"airp_arv",airp_arv);
              NewTextChild(node,"flight",GetTripName(fltParams.fltInfo,ecNone,true,false));
              NewTextChild(node,"grp_id",Qry.FieldAsInteger("grp_id"));
              NewTextChild(node,"pax_id",Qry.FieldAsInteger("pax_id"));
              if (!before_checkin)
                NewTextChild(node,"reg_no",Qry.FieldAsInteger("reg_no"));
              ostringstream pax;
              pax << Qry.FieldAsString("surname")
                  << (Qry.FieldIsNULL("name")?"":" ") << Qry.FieldAsString("name");
              NewTextChild(node,"pax_full_name",pax.str());
              NewTextChild(node,"pers_type",Qry.FieldAsString("pers_type"));

              NewTextChild(node,"prior_coupon_status",status->dispCode());
              if (!EticketItem.empty() && EticketItem.point_id != ASTRA::NoExists)
              {
                NewTextChild(node,"prior_point_id",EticketItem.point_id);
                NewTextChild(node,"prior_airp_dep",EticketItem.airp_dep);
                NewTextChild(node,"prior_airp_arv",EticketItem.airp_arv);
              }

              ProgTrace(TRACE5,"ETCheckStatus %s/%d->%s",
                               ticket_no.c_str(),
                               coupon_no,
                               real_status->dispCode());
            }
          }
        }
      }
    }
    catch(const CheckIn::UserException&)
    {
      throw;
    }
    catch(const AstraLocale::UserException &e)
    {
      throw CheckIn::UserException(e.getLexemaData(), point_id);
    }
  }

  if (check_point_id!=NoExists && fltParams.get(check_point_id))
  {
    //�஢�ઠ ����⮢, ���ᠦ��� ������ ࠧॣ����஢��� (�� �ᥬ� ३��)
    try
    {
      CouponStatus real_status=CouponStatus(CouponStatus::OriginalIssue);
      if ((fltParams.ets_exchange_status!=ETSExchangeStatus::NotConnected || check_connect) && !fltParams.ets_no_exchange)
      {
        std::set<TETickItem> ETickItems;
        PaxETList::GetAllStatusesByPointId(PaxETList::allNotCheckedStatusesByPointId, check_point_id, ETickItems);
        if (fltParams.control_method && fltParams.in_final_status)
          PaxETList::GetAllStatusesByPointId(PaxETList::allStatusesByPointIdFromTlg, check_point_id, ETickItems, false);

        if (!ETickItems.empty())
        {
          TETChangeStatusKey key;
          key.airline_oper=fltParams.fltInfo.airline;
          key.flt_no_oper=fltParams.fltInfo.flt_no;
          if (!get_et_addr_set(fltParams.fltInfo.airline,fltParams.fltInfo.flt_no,key.addrs))
          {
            ostringstream flt;
            flt << ElemIdToCodeNative(etAirline,fltParams.fltInfo.airline)
                << setw(3) << setfill('0') << fltParams.fltInfo.flt_no;
            throw AstraLocale::UserException("MSG.ETICK.ETS_ADDR_NOT_DEFINED_FOR_FLIGHT",
                                               LParams() << LParam("flight", flt.str()));
          }
          key.coupon_status=real_status->codeInt();

          for(const TETickItem& ET : ETickItems)
          {
            TETickItem ETItem(ET, real_status);
            if (ETStatusInterface::ChangeStatusLocallyOnly(fltParams,
                                                           ETItem,
                                                           TETCtxtItem(*TReqInfo::Instance(), ET.point_id))) continue;

            xmlNodePtr node=mtick.addTicket(key, ETItem, fltParams);

            NewTextChild(node,"ticket_no",ET.et.no);
            NewTextChild(node,"coupon_no",ET.et.coupon);
            NewTextChild(node,"point_id",ET.point_id);
            NewTextChild(node,"airp_dep",ET.airp_dep);
            NewTextChild(node,"airp_arv",ET.airp_arv);
            NewTextChild(node,"flight",GetTripName(fltParams.fltInfo,ecNone,true,false));
            NewTextChild(node,"prior_coupon_status",ET.et.status->dispCode());

            ProgTrace(TRACE5,"ETCheckStatus %s/%d->%s",
                             ET.et.no.c_str(),
                             ET.et.coupon,
                             real_status->dispCode());
          }
        }
      }
    }
    catch(const CheckIn::UserException&)
    {
      throw;
    }
    catch(const AstraLocale::UserException &e)
    {
      throw CheckIn::UserException(e.getLexemaData(), check_point_id);
    }
  }
}

bool ETStatusInterface::ETChangeStatus(const xmlNodePtr reqNode,
                                       const TETChangeStatusList &mtick)
{
  bool result=false;

  if (!mtick.empty())
  {
    const edifact::KickInfo &kickInfo=
        reqNode!=NULL?createKickInfo(AstraContext::SetContext("TERM_REQUEST",XMLTreeToText(reqNode->doc)), "ChangeStatus"):
                      edifact::KickInfo();
    result=ETChangeStatus(kickInfo, mtick);
  }
  return result;
}

struct TlgHaveSent
{
    edilib::EdiSessionId_t m_sessId;
    int                    m_reqCtxtId;

    TlgHaveSent(edilib::EdiSessionId_t sessId, int reqCtxtId)
        : m_sessId(sessId),
          m_reqCtxtId(reqCtxtId)
    {}
};

bool ETStatusInterface::ETChangeStatus(const edifact::KickInfo &kickInfo,
                                       const TETChangeStatusList &mtick)
{
  bool result=false;

  std::list<TlgHaveSent> ths; // tlgs have sent
  if (!mtick.empty())
  {
    for(TETChangeStatusList::const_iterator i=mtick.begin();i!=mtick.end();i++)
    {
      for(vector<TETChangeStatusItem>::const_iterator j=i->second.begin();j!=i->second.end();j++)
      {
        const TTicketList &ltick=j->first;
        if (ltick.empty()) continue;

        if (i->first.airline_oper.empty())
          throw EXCEPTIONS::Exception("ETChangeStatus: unkown operation carrier");
        if (i->first.flt_no_oper==ASTRA::NoExists)
          throw EXCEPTIONS::Exception("ETChangeStatus: unkown operation flight number");
        string oper_carrier=i->first.airline_oper;
        int oper_flight_no=i->first.flt_no_oper;

        /*
      try
      {
        TAirlinesRow& row=(TAirlinesRow&)base_tables.get("airlines").get_row("code",oper_carrier);
        if (!row.code_lat.empty()) oper_carrier=row.code_lat;
      }
      catch(const EBaseTableError&) {}
      */
        if (i->first.addrs.first.empty() ||
            i->first.addrs.second.empty())
          throw EXCEPTIONS::Exception("ETChangeStatus: edifact UNB-adresses not defined");
        set_edi_addrs(i->first.addrs);

        ProgTrace(TRACE5,"ETChangeStatus: oper_carrier=%s edi_addr=%s edi_own_addr=%s",
                  oper_carrier.c_str(),get_edi_addr().c_str(),get_edi_own_addr().c_str());

        xmlNodePtr rootNode=NodeAsNode("/context",j->second.docPtr());

        SetProp(rootNode, "req_ctxt_id", kickInfo.reqCtxtId, ASTRA::NoExists);
        TOriginCtxt::toXML(rootNode);

        string ediCtxt=XMLTreeToText(j->second.docPtr());

        const OrigOfRequest &org=kickInfo.background_mode()?OrigOfRequest(airlineToXML(oper_carrier)):
                                                            OrigOfRequest(airlineToXML(oper_carrier), *TReqInfo::Instance());

        //throw_if_request_dup("ETStatusInterface::ETChangeStatus");
        edilib::EdiSessionId_t sessId = ChangeStatus::ETChangeStatus(org,
                                                                     ltick,
                                                                     ediCtxt,
                                                                     kickInfo,
                                                                     oper_carrier,
                                                                     Ticketing::FlightNum_t(oper_flight_no));
        if(kickInfo.reqCtxtId != ASTRA::NoExists) {
            ths.push_back(TlgHaveSent(sessId, kickInfo.reqCtxtId));
        } else {
            LogTrace(TRACE0) << "kickInfo has uninitialized reqCtxtId!";
        }

        result=true;
      }
    }
  }

  if(result)
  {
      if(TReqInfo::Instance()->api_mode)
      {
          tlgnum_t tnum = *RemoteSystemContext::SystemContext::Instance(STDLOG).inbTlgInfo().tlgNum();
          if(!ths.empty()) {
              for(const auto& t: ths) {
                  AstraEdifact::WritePostponedContext(tnum, t.m_reqCtxtId);
                  TlgHandling::PostponeEdiHandling::postpone(tnum, t.m_sessId);
            }
            throw TlgHandling::TlgToBePostponed(tnum);
          }
      }
  }
  return result;
}

void EMDStatusInterface::ChangeStatus(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    if (TReqInfo::Instance()->duplicate) return;

    LogTrace(TRACE3) << __FUNCTION__;

    Ticketing::TicketNum_t emdDocNum(NodeAsString("EmdNoEdit", reqNode));
    Ticketing::CouponNum_t emdCpnNum(NodeAsInteger("CpnNoEdit", reqNode));
    Ticketing::CouponStatus emdCpnStatus;
    emdCpnStatus = Ticketing::CouponStatus::fromDispCode(NodeAsString("CpnStatusEdit", reqNode));
    int pointId = NodeAsInteger("point_id",reqNode);

    TTripInfo info;
    checkEDSExchange(pointId, true, info);
    std::string airline = getTripAirline(info);
    Ticketing::FlightNum_t flNum = getTripFlightNum(info);
    OrigOfRequest org(airlineToXML(airline), *TReqInfo::Instance());
    edifact::KickInfo kickInfo=createKickInfo(ASTRA::NoExists, "EMDStatus");

    edifact::EmdCosParams cosParams(org, "", kickInfo, airline, flNum, emdDocNum, emdCpnNum, emdCpnStatus);
    edifact::EmdCosRequest ediReq(cosParams);
    //throw_if_request_dup("EMDStatusInterface::ChangeStatus");
    ediReq.sendTlg();
}

void EMDStatusInterface::KickHandler(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    FuncIn(KickHandler);
    // TODO add kick handling
    FuncOut(KickHandler);
}

void ChangeStatusInterface::ChangeStatus(const xmlNodePtr reqNode,
                                         const TChangeStatusList &info)
{
  bool existsET=false;
  bool existsEMD=false;

  if (!info.empty())
  {
    const edifact::KickInfo &kickInfo=
        reqNode!=NULL?createKickInfo(AstraContext::SetContext("TERM_REQUEST",XMLTreeToText(reqNode->doc)), "ChangeStatus"):
                      edifact::KickInfo();
    existsET=ETStatusInterface::ETChangeStatus(kickInfo,info.ET);
    existsEMD=EMDStatusInterface::EMDChangeStatus(kickInfo,info.EMD);
  };
  if (existsET)
  {
    if (existsEMD)
      AstraLocale::showProgError("MSG.ETS_EDS_CONNECT_ERROR");
    else
      AstraLocale::showProgError("MSG.ETS_CONNECT_ERROR");
  }
  else
  {
    if (existsEMD)
      AstraLocale::showProgError("MSG.EDS_CONNECT_ERROR");
    else
      throw EXCEPTIONS::Exception("ChangeStatusInterface::ChangeStatus: empty info");
  };
}

static bool WasTimeout(const std::list<edifact::RemoteResults>& lRemRes)
{
    using namespace edifact;

    for(const RemoteResults& remRes: lRemRes) {
        if(remRes.status()->codeInt() == RemoteStatus::Timeout) {
            return true;
        }
    }

    return false;
}

void ChangeStatusInterface::KickHandler(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    using namespace edifact;

    std::list<RemoteResults> lRemRes;
    RemoteResults::readDb(lRemRes);
    if(WasTimeout(lRemRes)) {
        KickOnTimeout(reqNode, resNode);
    } else {
        KickOnAnswer(reqNode, resNode);
    }
}

void ChangeStatusInterface::KickOnAnswer(xmlNodePtr reqNode, xmlNodePtr resNode)
{
    string context;
    TReqInfo *reqInfo = TReqInfo::Instance();
    if (GetNode("@req_ctxt_id",reqNode)!=NULL)  //req_ctxt_id ���������, �᫨ ⥫��ࠬ�� ��ନ஢��� �� �� ����
    {
      int req_ctxt_id=NodeAsInteger("@req_ctxt_id",reqNode);

      XMLDoc termReqCtxt;
      getTermRequestCtxt(req_ctxt_id, true, "ChangeStatusInterface::KickHandler", termReqCtxt);

      XMLDoc ediResCtxt;
      getEdiResponseCtxt(req_ctxt_id, true, "ChangeStatusInterface::KickHandler", ediResCtxt);

      xmlNodePtr termReqNode=NodeAsNode("/term/query",termReqCtxt.docPtr())->children;
      if (termReqNode==NULL)
        throw EXCEPTIONS::Exception("ChangeStatusInterface::KickHandler: context TERM_REQUEST termReqNode=NULL");

      transformKickRequest(termReqNode, reqNode);

      string termReqName=(const char*)(termReqNode->name);
      bool defer_etstatus=(termReqName=="ChangePaxStatus" ||
                           termReqName=="ChangeGrpStatus" ||
                           termReqName=="ChangeFltStatus");
      ProgTrace( TRACE5, "termReqName=%s", termReqName.c_str() );

      xmlNodePtr ediResNode=NodeAsNode("/context",ediResCtxt.docPtr());


      map<string, pair< set<string>, vector< pair<string,string> > > > errors; //flight,������⢮ global_error, ����� ��� pax+ticket/coupon_error
      map<int, map<int, AstraLocale::LexemaData> > segs;

      xmlNodePtr emdNode=GetNode("emdocs", ediResNode);
      if (emdNode!=NULL) emdNode=emdNode->children;
      for(; emdNode!=NULL; emdNode=emdNode->next)
      {
        TEMDCtxtItem EMDCtxt;
        EMDCtxt.fromXML(emdNode);
        string pax;
        if (!EMDCtxt.paxUnknown())
          pax=getLocaleText("WRAP.PASSENGER",
                            LParams() << LParam("name", EMDCtxt.pax.full_name())
                                      << LParam("pers_type",ElemIdToCodeNative(etPersType,EncodePerson(EMDCtxt.pax.pers_type))));

        EdiErrorList errList;
        GetEdiError(emdNode, errList);
        if (!errList.empty())
        {
          pair< set<string>, vector< pair<string,string> > > &err=errors[EMDCtxt.flight];

          for(EdiErrorList::const_iterator e=errList.begin(); e!=errList.end(); ++e)
            if (e->second) //global
            {
              err.first.insert(getLocaleText(e->first));
              //�訡�� �஢�� ᥣ����
              segs[EMDCtxt.point_id][ASTRA::NoExists]=e->first;
            }
            else
            {
              err.second.push_back(make_pair(pax,getLocaleText(e->first)));
              //�訡�� �஢�� ���ᠦ��
              segs[EMDCtxt.point_id][EMDCtxt.pax.id]=e->first;
            };
        };
      };

      xmlNodePtr ticketNode=GetNode("tickets", ediResNode);
      if (ticketNode!=NULL) ticketNode=ticketNode->children;
      for(;ticketNode!=NULL;ticketNode=ticketNode->next)
      {
        string flight=NodeAsString("flight",ticketNode);
        string pax;
        if (GetNode("pax_full_name",ticketNode)!=NULL&&
            GetNode("pers_type",ticketNode)!=NULL)
          pax=getLocaleText("WRAP.PASSENGER", LParams() << LParam("name",NodeAsString("pax_full_name",ticketNode))
                                                        << LParam("pers_type",ElemIdToCodeNative(etPersType,NodeAsString("pers_type",ticketNode))));
        int point_id=NodeAsInteger("point_id",ticketNode);
        int pax_id=ASTRA::NoExists;
        if (GetNode("pax_id",ticketNode)!=NULL)
          pax_id=NodeAsInteger("pax_id",ticketNode);

        bool tick_event=false;
        for(xmlNodePtr node=ticketNode->children;node!=NULL;node=node->next)
        {
          if (strcmp((const char*)node->name,"coupon_status")==0) tick_event=true;

          if (!(strcmp((const char*)node->name,"global_error")==0 ||
                strcmp((const char*)node->name,"ticket_error")==0 ||
                strcmp((const char*)node->name,"coupon_error")==0)) continue;

          tick_event=true;

          pair< set<string>, vector< pair<string,string> > > &err=errors[flight];

          LexemaData lexemeData;
          LexemeDataFromXML(node, lexemeData);
          if (strcmp((const char*)node->name,"global_error")==0)
          {
            err.first.insert(getLocaleText(lexemeData));
            //�訡�� �஢�� ᥣ����
            segs[point_id][ASTRA::NoExists]=lexemeData;
          }
          else
          {
            err.second.push_back(make_pair(pax,getLocaleText(lexemeData)));
            //�訡�� �஢�� ���ᠦ��
            segs[point_id][pax_id]=lexemeData;
          }
        }
        if (!tick_event)
        {
          ostringstream ticknum;
          ticknum << NodeAsString("ticket_no",ticketNode) << "/"
                  << NodeAsInteger("coupon_no",ticketNode);

          LexemaData lexemeData;
          lexemeData.lexema_id="MSG.ETICK.CHANGE_STATUS_UNKNOWN_RESULT";
          lexemeData.lparams << LParam("ticknum",ticknum.str());
          string err_locale=getLocaleText(lexemeData);

          pair< set<string>, vector< pair<string,string> > > &err=errors[flight];
          err.second.push_back(make_pair(pax,err_locale));
          segs[point_id][pax_id]=lexemeData;
        }
      }

      if (!errors.empty())
      {
        bool use_flight=(GetNode("segments",termReqNode)!=NULL &&
                         NodeAsNode("segments/segment",termReqNode)->next!=NULL);  //��।���� �� ������ TERM_REQUEST;
        map<string, pair< set<string>, vector< pair<string,string> > > >::iterator i;
        if (!defer_etstatus ||
            reqInfo->client_type == ctWeb ||
            reqInfo->client_type == ctMobile ||
            reqInfo->client_type == ctKiosk)
        {
          if (reqInfo->client_type == ctWeb ||
              reqInfo->client_type == ctMobile ||
              reqInfo->client_type == ctKiosk)
          {
            if (!segs.empty())
              CheckIn::showError(segs);
            else
              AstraLocale::showError( "MSG.ETICK.CHANGE_STATUS_ERROR", LParams() << LParam("ticknum","") << LParam("error","") ); //!!! ���� �뢮���� ����� ����� � �訡��
          }
          else
          {
            ostringstream msg;
            for(i=errors.begin();i!=errors.end();i++)
            {
              if (use_flight)
                msg << getLocaleText("WRAP.FLIGHT", LParams() << LParam("flight",i->first) << LParam("text","") ) << std::endl;
              for(set<string>::const_iterator j=i->second.first.begin(); j!=i->second.first.end(); ++j)
              {
                if (use_flight) msg << "     ";
                msg << *j << std::endl;
              }
              for(vector< pair<string,string> >::const_iterator j=i->second.second.begin(); j!=i->second.second.end(); ++j)
              {
                if (use_flight) msg << "     ";
                if (!(j->first.empty()))
                {
                  msg << j->first << std::endl
                      << "     ";
                  if (use_flight) msg << "     ";
                }
                msg << j->second << std::endl;
              }
            }
            NewTextChild(resNode,"ets_error",msg.str());
          }
          //�⪠� ��� ���⢥ত����� ����ᮢ
          ETStatusInterface::ETRollbackStatus(ediResCtxt.docPtr(),false);
          return;
        }
        else
        {
          //�⪠� �� ������ �᫨ ࠧ���쭮� ���⢥ত���� ��� �ନ��� ��ᮢ���⨬
          for(i=errors.begin();i!=errors.end();i++)
            if (!i->second.first.empty())
              throw AstraLocale::UserException(*i->second.first.begin());
          for(i=errors.begin();i!=errors.end();i++)
            if (!i->second.second.empty())
              throw AstraLocale::UserException((i->second.second.begin())->second);
        }
      }

      if (defer_etstatus) return;

      ContinueCheckin(termReqNode, ediResNode, resNode);
    }
}

void transformKickRequest(xmlNodePtr termReqNode, xmlNodePtr &kickReqNode)
{
  if (termReqNode==nullptr) return;

  //�᫨ �㫨�㥬 ����� web-ॣ����樨 � �ନ���� - � ������ ������� client_type
  xmlNodePtr ifaceNode=GetNode("/term/query/@id",termReqNode->doc);
  if (ifaceNode!=NULL && strcmp(NodeAsString(ifaceNode), WEB_JXT_IFACE_ID)==0)
  {
    //�� web-ॣ������
    if (TReqInfo::Instance()->client_type==ctTerm)
      TReqInfo::Instance()->client_type=EMUL_CLIENT_TYPE;
  }

  if (kickReqNode==nullptr) return;

  if (isWebCheckinRequest(termReqNode) ||
      isTagAddRequestSBDO(termReqNode) ||
      isTagConfirmRequestSBDO(termReqNode) ||
      isTagRevokeRequestSBDO(termReqNode))
  {
    string termReqName=(const char*)(termReqNode->name);
    xmlNodePtr node = NodeAsNode("/term/query",kickReqNode->doc);
    xmlUnlinkNode( kickReqNode );
    xmlFreeNode( kickReqNode );
    kickReqNode = NewTextChild( node, termReqName.c_str() );
  }
}

void ContinueCheckin(xmlNodePtr reqNode, xmlNodePtr externalSysResNode, xmlNodePtr resNode)
{
  if(isTagCUWS(reqNode)) {
      CUWS::CUWSDispatcher(reqNode, externalSysResNode, resNode);
      return;
  }

  if (isTagAddRequestSBDO(reqNode))
  {
    ZamarSBDOInterface::PassengerBaggageTagAdd(reqNode, externalSysResNode, resNode);
    return;
  }

  if (isTagConfirmRequestSBDO(reqNode))
  {
    ZamarSBDOInterface::PassengerBaggageTagConfirm(reqNode, externalSysResNode, resNode);
    return;
  }

  if (isTagRevokeRequestSBDO(reqNode))
  {
    ZamarSBDOInterface::PassengerBaggageTagRevoke(reqNode, externalSysResNode, resNode);
    return;
  }

  try
  {
    if (isTermCheckinRequest(reqNode))
    {
      if (!CheckInInterface::SavePax(reqNode, externalSysResNode, resNode))
      {
        //�⪠�뢠�� ������ ⠪ ��� ������ ��㯯� ⠪ � �� ��諠
        ETStatusInterface::ETRollbackStatus(externalSysResNode->doc,false);
        return;
      }
    }

    if (isWebCheckinRequest(reqNode))
    {
      if (!AstraWeb::WebRequestsIface::SavePax(reqNode, externalSysResNode, resNode))
      {
        //�⪠�뢠�� ������ ⠪ ��� ������ ��㯯� ⠪ � �� ��諠
        ETStatusInterface::ETRollbackStatus(externalSysResNode->doc,false);
        return;
      }
    }

  }
  catch(ServerFramework::Exception &e)
  {
    ASTRA::rollback();
    jxtlib::JXTLib::Instance()->GetCallbacks()->HandleException(&e);
    ETStatusInterface::ETRollbackStatus(externalSysResNode->doc,false);
  }
}

void ChangeStatusInterface::KickOnTimeout(xmlNodePtr reqNode, xmlNodePtr resNode)
{
    // TODO handle COS timeout
}

bool EMDAutoBoundInterface::BeforeLock(const EMDAutoBoundId &id, int &point_id, GrpIds &grpIds)
{
  point_id=NoExists;
  grpIds.clear();

  QParams params;
  id.setSQLParams(params);
  TCachedQuery Qry(id.grpSQL(), params);
  Qry.get().Execute();
  if (Qry.get().Eof) return false; //�� �뢠�� ����� ࠧॣ������ �ᥩ ��㯯� �� �訡�� �����
  point_id=Qry.get().FieldAsInteger("point_dep");
  for(; !Qry.get().Eof; Qry.get().Next())
  {
    if (point_id!=Qry.get().FieldAsInteger("point_dep")) continue;
    GrpId_t grpId(Qry.get().FieldAsInteger("grp_id"));
    if (find(grpIds.begin(), grpIds.end(), grpId)==grpIds.end())
      grpIds.push_back(grpId);
  };

  TTripInfo info;
  if (!info.getByPointId(point_id)) return false;
  if (GetTripSets(tsNoEMDAutoBinding, info)) return false;
  return true;
}

bool EMDAutoBoundInterface::Lock(const EMDAutoBoundId &id, int &point_id, GrpIds &grpIds, const std::string &whence)
{
  point_id=NoExists;
  grpIds.clear();

  if (!BeforeLock(id, point_id, grpIds)) return false;

  TFlights flightsForLock;
  flightsForLock.Get( point_id, ftTranzit );
  flightsForLock.Lock(whence);
  return true;
}

bool EMDAutoBoundInterface::Lock(const EMDAutoBoundId &id, int &point_id, TCkinGrpIds &tckin_grp_ids, const std::string &whence)
{
  point_id=NoExists;
  tckin_grp_ids.clear();

  int grp_id=NoExists;
  GrpIds grpIds;
  bool res=BeforeLock(id, point_id, grpIds);
  if (!grpIds.empty()) grp_id=grpIds.front().get();
  if (grpIds.size()>1)
    throw EXCEPTIONS::Exception("EMDAutoBoundInterface::Lock: more than one grp_id found");
  if (grp_id!=NoExists) tckin_grp_ids.push_back(grp_id);
  if (!res) return false;

  TFlights flightsForLock;
  flightsForLock.GetForTCkinRouteDependent(grp_id, ftTranzit, tckin_grp_ids);
  flightsForLock.Lock(whence);
  return true;
}

static bool needTryCheckinServicesAuto(int id, bool is_grp_id)
{
  return is_grp_id?existsAlarmByGrpId(id, Alarm::SyncEmds):
                   existsAlarmByPaxId(id, Alarm::SyncEmds, paxCheckIn);

}

void EMDAutoBoundInterface::EMDRefresh(const EMDAutoBoundId &id, xmlNodePtr reqNode)
{
  //�᪮�����஢�� ��� �����, ��, ��ண�� ���, �⪫���� ���� ��⮯ਢ離� EMD, �஬� 䮭���� �� ������� ॣ����樨
  // try { dynamic_cast<const EMDAutoBoundPointId&>(id); } catch(const std::bad_cast&) { return; }

  int point_id=NoExists;
  GrpIds grpIds;
  if (!Lock(id, point_id, grpIds, __FUNCTION__)) return;

  //�஢�ਬ, �� ��� �ॢ��� "��� �裡 � ���"
  TFltParams fltParams;
  if (!(fltParams.get(point_id) && fltParams.ets_exchange_status!=ETSExchangeStatus::NotConnected)) return;

  //��㣨
  boost::optional< set<int> > pax_ids_for_refresh=set<int>();

  for(const GrpId_t& grpId : grpIds)
  {
    TPaidRFISCListWithAuto paid_rfisc;
    paid_rfisc.fromDB(grpId.get(), true);
    for(TPaidRFISCListWithAuto::const_iterator p=paid_rfisc.begin(); p!=paid_rfisc.end(); ++p)
      if (p->second.need_positive()) pax_ids_for_refresh.get().insert(p->second.pax_id);
  };
  //����� wt
  if (reqNode!=nullptr)
  {
    string termReqName=(const char*)(reqNode->name);
    if (termReqName=="TCkinLoadPax")
    {
      //��� ���⮢�� ��⥬� ������ refresh ⮫쪮 �� ����㧪� ��㯯�
      for(const GrpId_t& grpId : grpIds)
      {
        WeightConcept::TPaidBagList paid;
        WeightConcept::PaidBagFromDB(NoExists, grpId.get(), paid);
        CheckIn::TServicePaymentListWithAuto payment;
        payment.fromDB(grpId.get());
        for(WeightConcept::TPaidBagList::const_iterator p=paid.begin(); p!=paid.end(); ++p)
        {
          if (p->weight<=0) continue;
          if (payment.getDocWeight(*p)<p->weight)
          {
            pax_ids_for_refresh=boost::none;
            break;
          };
        };
      };
    };
  };

  EMDSearch(id, reqNode, point_id, pax_ids_for_refresh);

  if (isDoomedToWait())
  {
    AstraLocale::showErrorMessage("MSG.ETS_EDS_CONNECT_ERROR"); //��⮬ ��।����� �� MSG.ETS_CONNECT_ERROR, ����� ��ᯫ�� �㤥� �������� RFISC
    return;
  }

  //�� ����� ���� ����� ���஡����� ���� ��⮬���᪨ �ਢ易�� ��� ��ॣ����஢��� EMD?
  if (reqNode==nullptr) return;
  string termReqName=(const char*)(reqNode->name);

  {
    TCkinGrpIds tckin_grp_ids;
    int point_id=NoExists;
    if (Lock(id, point_id, tckin_grp_ids, string(__FUNCTION__)+"("+termReqName+")"))
    {
      if ((pax_ids_for_refresh && !pax_ids_for_refresh.get().empty()) ||
          any_of(tckin_grp_ids.begin(), tckin_grp_ids.end(), bind2nd(ptr_fun(needTryCheckinServicesAuto), true)))
      {
        id.toXML(reqNode);
        EMDTryBind(tckin_grp_ids, reqNode, NULL);
      }
    }
  }

  if (isDoomedToWait())
    AstraLocale::showErrorMessage("MSG.EDS_CONNECT_ERROR");
}

void EMDAutoBoundInterface::EMDSearch(const EMDAutoBoundId &id,
                                      xmlNodePtr reqNode,
                                      int point_id,
                                      const boost::optional< std::set<int> > &pax_ids)
{
  if (pax_ids && pax_ids.get().empty()) return;

  boost::optional<edifact::KickInfo> kickInfo;
  map<int /*grp_id*/, edifact::KickInfo> kicksByGrp;
  map<int /*grp_id*/, int /*point_id*/> pointIdsByGrp;

  QParams params;
  id.setSQLParams(params);
  TCachedQuery PaxQry(id.paxSQL(), params);
  PaxQry.get().Execute();
  set<string> tkns_set;
  for(;!PaxQry.get().Eof; PaxQry.get().Next())
  {
    int grp_id=PaxQry.get().FieldAsInteger("grp_id");
    pointIdsByGrp.emplace(grp_id, point_id);

    int pax_id=PaxQry.get().FieldAsInteger("pax_id");
    string refuse=PaxQry.get().FieldAsString("refuse");
    if (pax_ids && pax_ids.get().find(pax_id)==pax_ids.get().end()) continue;
    if (!refuse.empty()) continue;

    map<int, CheckIn::TCkinPaxTknItem> tkns;
    CheckIn::GetTCkinTickets(pax_id, tkns);
    if (tkns.empty()) //�� ᪢����� ���ᠦ��
      tkns.emplace(1, CheckIn::TCkinPaxTknItem().fromDB(PaxQry.get()));

    for(map<int, CheckIn::TCkinPaxTknItem>::const_iterator i=tkns.begin(); i!=tkns.end(); ++i)
    {
      if (i->second.no.empty()) continue;
      if (!tkns_set.insert(i->second.no).second) continue; //�⮡� �� �뫮 �㡫�஢���� �� ����⠬ �� ��ᯫ��
      auto iPointIdsByGrp=pointIdsByGrp.find(i->second.grp_id);
      if (iPointIdsByGrp==pointIdsByGrp.end())
      {
        TTripInfo fltInfo;
        if (!fltInfo.getByGrpId(i->second.grp_id)) continue;
        iPointIdsByGrp=pointIdsByGrp.emplace(i->second.grp_id, fltInfo.point_id).first;
        ProgTrace(TRACE5, "%s: pointIdsByGrp.emplace(%d, %d)", __FUNCTION__, i->second.grp_id, fltInfo.point_id);
      }
      try
      {
        if (reqNode!=nullptr)
        {
          if (!kickInfo)
          {
            id.toXML(reqNode);
            kickInfo=AstraEdifact::createKickInfo(AstraContext::SetContext("TERM_REQUEST",XMLTreeToText(reqNode->doc)),
                                                  "EMDAutoBound");
          }
        }
        else
        {
          map<int /*grp_id*/, edifact::KickInfo>::const_iterator iKick=kicksByGrp.find(grp_id);
          if (iKick==kicksByGrp.end())
          {
            xmlNodePtr ctxtNode;
            XMLDoc reqCtxt("context", ctxtNode, __FUNCTION__);
            EMDAutoBoundGrpId(grp_id).toXML(ctxtNode);

            kickInfo=AstraEdifact::createKickInfo(AstraContext::SetContext("TERM_REQUEST", reqCtxt.text()), point_id, EMD_TRY_BIND);
            kicksByGrp.emplace(grp_id, kickInfo.get());

          }
          else kickInfo=iKick->second;
        };

        ETSearchInterface::SearchET(ETSearchByTickNoParams(iPointIdsByGrp->second, i->second.no),
                                    ETSearchInterface::spEMDRefresh,
                                    kickInfo.get());
      }
      catch(const UserException& e)
      {
        LogTrace(TRACE5) << __FUNCTION__ << ": " << e.what();
      };
    };
  }
}

void EMDAutoBoundInterface::EMDTryBind(const TCkinGrpIds &tckin_grp_ids,
                                       const boost::optional< std::list<TEMDCtxtItem> > &confirmed_emd,
                                       TEMDChangeStatusList &emdList)
{
  emdList.clear();

  if (tckin_grp_ids.empty()) return;

  bool check_emd_status=!confirmed_emd;

  int first_grp_id=tckin_grp_ids.front();

  //�業�� ���
  TPaidRFISCList paid_rfisc;
  paid_rfisc.fromDB(first_grp_id, true);
  //����祭�� ��㣨
  CheckIn::TServicePaymentList payment;
  payment.fromDB(first_grp_id);
  //�-�� EMD (㤠����� � ��易��� ������ EMD)
  CheckIn::TGrpEMDProps emdProps;
  CheckIn::TGrpEMDProps::fromDB(first_grp_id, emdProps);
  //��⮬���᪨ ��ॣ����஢���� ��㣨
  TGrpServiceAutoList svcsAuto;
  svcsAuto.fromDB(first_grp_id, true);

  bool enlargedServicePayment=tryEnlargeServicePayment(paid_rfisc, payment, svcsAuto, tckin_grp_ids, emdProps, confirmed_emd);
  bool checkinServicesAuto=tryCheckinServicesAuto(svcsAuto, payment, tckin_grp_ids, emdProps, confirmed_emd);

  for(const int& grp_id : tckin_grp_ids)
    deleteAlarmByGrpId(grp_id, Alarm::SyncEmds);

  if (enlargedServicePayment || checkinServicesAuto)
  {
    list<CheckIn::TAfterSaveSegInfo> segs;
    for(const int& grp_id : tckin_grp_ids)
    {
      segs.push_back(CheckIn::TAfterSaveSegInfo());
      segs.back().grp_id=grp_id;
      TGrpToLogInfo &grpInfoBefore=segs.back().grpInfoBefore;

      GetGrpToLogInfo(grp_id, grpInfoBefore);
      CheckIn::TServicePaymentListWithAuto paymentBeforeWithAuto;
      if (check_emd_status)
        paymentBeforeWithAuto.fromDB(grp_id);

      CheckIn::TPaxGrpItem::UpdTid(grp_id);

      if (grp_id==tckin_grp_ids.front())
      {
        if (enlargedServicePayment)
        {
          paid_rfisc.toDB(grp_id);
          payment.toDB(grp_id);
        }
        if (checkinServicesAuto)
        {
          svcsAuto.toDB(grp_id);
        }
      }
      else
      {
        if (enlargedServicePayment)
        {
          TPaidRFISCList::copyDB(tckin_grp_ids.front(), grp_id);
          CheckIn::TServicePaymentList::copyDB(tckin_grp_ids.front(), grp_id);
        }
        if (checkinServicesAuto)
        {
          TGrpServiceAutoList::copyDB(tckin_grp_ids.front(), grp_id);
        }
      };

      if (check_emd_status)
        EMDStatusInterface::EMDCheckStatus(grp_id, paymentBeforeWithAuto, emdList);
    };

    if (emdList.empty())
    {
      //��� EMD �� ����� ���� �������� �����
      for(list<CheckIn::TAfterSaveSegInfo>::iterator s=segs.begin(); s!=segs.end(); ++s)
      {
        int grp_id=s->grp_id;
        const TGrpToLogInfo &grpInfoBefore=s->grpInfoBefore;
        TGrpToLogInfo &grpInfoAfter=s->grpInfoAfter;
        GetGrpToLogInfo(grp_id, grpInfoAfter);
        TAgentStatInfo agentStat;
        SaveGrpToLog(grpInfoBefore, grpInfoAfter, CheckIn::TGrpEMDProps(), agentStat);
      };
    }

  };

}

void EMDAutoBoundInterface::EMDTryBind(const TCkinGrpIds &tckin_grp_ids,
                                       xmlNodePtr termReqNode,
                                       xmlNodePtr ediResNode,
                                       const boost::optional<edifact::TripTaskForPostpone> &task)
{
  try
  {
    bool second_call=GetNode("second_call", termReqNode)!=NULL;

    boost::optional< list<TEMDCtxtItem> > confirmed_emd;
    if (second_call)
    {
      confirmed_emd=list<TEMDCtxtItem>();

      //���� �� � payment ������� ⮫쪮 �������訥 ����� EMD
      xmlNodePtr emdNode=GetNode("emdocs", ediResNode);
      if (emdNode!=NULL) emdNode=emdNode->children;
      for(; emdNode!=NULL; emdNode=emdNode->next)
      {
        TEMDCtxtItem EMDCtxt;
        EMDCtxt.fromXML(emdNode);

        if (EMDCtxt.paxUnknown()) continue;

        EdiErrorList errList;
        GetEdiError(emdNode, errList);
        if (!errList.empty()) continue;

        confirmed_emd.get().push_back(EMDCtxt);
      };
    };
    TEMDChangeStatusList EMDList;
    EMDAutoBoundInterface::EMDTryBind(tckin_grp_ids, confirmed_emd, EMDList);

    //����� ChangeStatus
    if (!EMDList.empty())
    {
      //��� �� ���� ���㬥�� �㤥� ��ࠡ��뢠����
      ASTRA::rollback();
      NewTextChild(termReqNode, "second_call");
      edifact::KickInfo kickInfo=!task?AstraEdifact::createKickInfo(AstraContext::SetContext("TERM_REQUEST",XMLTreeToText(termReqNode->doc)),
                                                                    "EMDAutoBound"):
                                       AstraEdifact::createKickInfo(AstraContext::SetContext("TERM_REQUEST",XMLTreeToText(termReqNode->doc)),
                                                                    task.get().point_id, task.get().name);

      EMDStatusInterface::EMDChangeStatus(kickInfo,EMDList);
      return;
    };

  }
  catch(const UserException &e)
  {
    ASTRA::rollback();
    ProgTrace(TRACE5, ">>>> %s: UserException: %s", __FUNCTION__, e.what());
  }
  catch(const std::exception &e)
  {
    ASTRA::rollback();
    ProgError(STDLOG, "%s: std::exception: %s", __FUNCTION__, e.what());
  }
}

void emd_refresh_task(const TTripTaskKey &task)
{
  LogTrace(TRACE5) << __FUNCTION__ << ": " << task;
  EMDAutoBoundInterface::EMDRefresh(EMDAutoBoundPointId(task.point_id), nullptr);
}

void emd_refresh_by_grp_task(const TTripTaskKey &task)
{
  LogTrace(TRACE5) << __FUNCTION__ << ": " << task;
  if ( !task.params.empty() ) {
    int grp_id;
    StrToInt( task.params.c_str(), grp_id );
    EMDAutoBoundInterface::EMDRefresh(EMDAutoBoundGrpId(grp_id), nullptr);
  }
}

void emd_try_bind_task(const TTripTaskKey &task)
{
  LogTrace(TRACE5) << __FUNCTION__ << ": " << task;

  TReqInfo *reqInfo=TReqInfo::Instance();
  reqInfo->user.sets.time = ustTimeUTC;

  int req_ctxt_id=ToInt(task.params);

  XMLDoc reqCtxt;
  getTermRequestCtxt(req_ctxt_id, true, __FUNCTION__, reqCtxt);
  xmlNodePtr reqNode=NodeAsNode("/context", reqCtxt.docPtr());

  XMLDoc resCtxt;
  getEdiResponseCtxt(req_ctxt_id, true, __FUNCTION__, resCtxt, false);
  if(resCtxt.docPtr()==NULL)
  {
    LogTrace(TRACE5) << __FUNCTION__ << ": resCtxt.docPtr()==NULL";
    return;
  };
  xmlNodePtr resNode=NodeAsNode("/context", resCtxt.docPtr());

  EMDAutoBoundGrpId id(reqNode);
  TCkinGrpIds tckin_grp_ids;
  int point_id=NoExists;
  if (EMDAutoBoundInterface::Lock(id, point_id, tckin_grp_ids, __FUNCTION__))
  {
    EMDAutoBoundInterface::EMDTryBind(tckin_grp_ids, reqNode, resNode, edifact::TripTaskForPostpone(point_id, EMD_TRY_BIND));
  };
}

void EMDAutoBoundInterface::KickHandler(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  int req_ctxt_id=NodeAsInteger("@req_ctxt_id",reqNode);

  XMLDoc termReqCtxt;
  getTermRequestCtxt(req_ctxt_id, true, "EMDAutoBoundInterface::KickHandler", termReqCtxt);

  XMLDoc ediResCtxt;
  getEdiResponseCtxt(req_ctxt_id, true, "EMDAutoBoundInterface::KickHandler", ediResCtxt);

  xmlNodePtr termReqNode=NodeAsNode("/term/query",termReqCtxt.docPtr())->children;
  if (termReqNode==NULL)
    throw EXCEPTIONS::Exception("EMDAutoBoundInterface::KickHandler: context TERM_REQUEST termReqNode=NULL");

  string termReqName=(const char*)(termReqNode->name);
  LogTrace(TRACE5)<<termReqName;

  xmlNodePtr ediResNode=NodeAsNode("/context",ediResCtxt.docPtr());

  if (termReqName=="TCkinLoadPax" ||
      termReqName=="TCkinSavePax" ||
      termReqName=="TCkinSaveUnaccompBag")
  {
    bool afterSavePax=termReqName=="TCkinSavePax" ||
                      termReqName=="TCkinSaveUnaccompBag";

    EMDAutoBoundGrpId id(termReqNode);
    TCkinGrpIds tckin_grp_ids;
    int point_id=NoExists;
    if (Lock(id, point_id, tckin_grp_ids, string(__FUNCTION__)+"("+termReqName+")"))
    {
      EMDTryBind(tckin_grp_ids, termReqNode, ediResNode);
    };

    if (isDoomedToWait())  //ᯥ樠�쭮 � �⮬ ����, ��⮬� �� LoadPax ����� �뢥�� ����� ������ ᮮ�饭��
      AstraLocale::showErrorMessage("MSG.EDS_CONNECT_ERROR");

    try
    {
      CheckInInterface::LoadPaxByGrpId(id.grp_id, termReqNode, resNode, afterSavePax);
    }
    catch(...)
    {
      if (!isDoomedToWait()) throw;
    };
  }
  if (termReqName=="PaxByPaxId" ||
      termReqName=="PaxByRegNo" ||
      termReqName=="PaxByScanData")
  {
    EMDAutoBoundRegNo id(termReqNode);
    TCkinGrpIds tckin_grp_ids;
    int point_id=NoExists;
    if (Lock(id, point_id, tckin_grp_ids, string(__FUNCTION__)+"("+termReqName+")"))
    {
      EMDTryBind(tckin_grp_ids, termReqNode, ediResNode);
    };

    if (isDoomedToWait())
    {
      AstraLocale::showErrorMessage("MSG.EDS_CONNECT_ERROR");
      return;
    };

    BrdInterface::GetPax(termReqNode, resNode);
  }
  if (termReqName=="paid" ||
      termReqName=="evaluation")
  {
    EMDAutoBoundGrpId id(termReqNode);
    TCkinGrpIds tckin_grp_ids;
    int point_id=NoExists;
    if (Lock(id, point_id, tckin_grp_ids, string(__FUNCTION__)+"("+termReqName+")"))
    {
      EMDTryBind(tckin_grp_ids, termReqNode, ediResNode);
    };

    if (isDoomedToWait())
    {
      AstraLocale::showErrorMessage("MSG.EDS_CONNECT_ERROR");
      return;
    };

    ServiceEvalInterface::AfterPaid(termReqNode, resNode);
  }

}

//---------------------------------------------------------------------------------------

void handleEtDispResponse(const edifact::RemoteResults& remRes)
{
    edilib::EdiSessionId_t ediSessId = remRes.ediSession();

    boost::optional<Ticketing::Pnr> pnr;
    try
    {
        if (!pnr) {
            pnr = readDispPnr(remRes.tlgSource());
        }

        Ticketing::EdiPnr ediPnr(remRes.tlgSource(), edifact::EdiDisplRes);
        ETDisplayToDB(ediPnr);
    }
    catch(const UserException &e)
    {
      ProgTrace(TRACE5, ">>>> %s: %s", __FUNCTION__, e.what());
    }
    catch(const std::exception &e)
    {
        if (remRes.isSystemPult())
          ProgTrace(TRACE5, ">>>> %s: %s", __FUNCTION__, e.what());
        else
          ProgError(STDLOG, "%s: %s", __FUNCTION__, e.what());
    }
    catch(...)
    {
        if (remRes.isSystemPult())
          ProgTrace(TRACE5, ">>>> %s: unknown error", __FUNCTION__);
        else
          ProgError(STDLOG, "%s: unknown error", __FUNCTION__);
    }

    XMLDoc ediSessCtxt;
    AstraEdifact::getEdiSessionCtxt(ediSessId.get(), true, "handleEtDispResponse", ediSessCtxt, false);
    if(ediSessCtxt.docPtr()!=NULL)
    {
        xmlNodePtr rootNode=NodeAsNode("/context",ediSessCtxt.docPtr());
        int point_id=NodeAsInteger("point_id",rootNode);

        if (TFltParams::returnOnlineStatus(point_id))
        {
            //����襬 � ���
            TReqInfo::Instance()->LocaleToLog("EVT.RETURNED_INTERACTIVE_WITH_ETC", ASTRA::evtFlt, point_id );
        };

        std::string purpose=NodeAsString("@purpose",rootNode);

        if (purpose=="EMDDisplay" ||
            purpose=="EMDRefresh")
        {
            XMLDoc ediResCtxt("context");
            if (ediResCtxt.docPtr()==NULL)
                throw EXCEPTIONS::Exception("%s: CreateXMLDoc failed", __FUNCTION__);
            xmlNodePtr ediResCtxtNode=NodeAsNode("/context",ediResCtxt.docPtr());

            // ����頥� RemoteResults ��ᯫ�� �� - ��� ����� �� �㦭�
            edifact::RemoteResults::deleteDb(ediSessId);

            try
            {
                if (!pnr) pnr = readDispPnr(remRes.tlgSource());

                edifact::KickInfo kickInfo;
                kickInfo.fromXML(rootNode);
                kickInfo.parentSessId=ediSessId.get();
                OrigOfRequest org("");
                string airline;
                Ticketing::FlightNum_t flNum;
                OrigOfRequest::fromXML(rootNode, org, airline, flNum);
                std::set<Ticketing::TicketNum_t> emds;
                for(std::list<Ticket>::const_iterator i=pnr.get().ltick().begin(); i!=pnr.get().ltick().end(); ++i)
                  if (i->actCode() == TickStatAction::inConnectionWith)
                  {
                    emds.insert(i->connectedDocNum());
                    //ProgTrace(TRACE5, "%s: %s", __FUNCTION__, i->connectedDocNum().get().c_str());
                  };
                Ticket::Trace(TRACE5, pnr.get().ltick());
                SearchEMDsByTickNo(emds, kickInfo, org, airline, flNum);
            }
            catch(AstraLocale::UserException &e)
            {
                //��� ��⠫��� �訡�� ������
                ProcEdiError(e.getLexemaData(), ediResCtxtNode, true);
            }
            LogTrace(TRACE3) << "Before addToEdiResponseCtxt";
            int req_ctxt_id=NodeAsInteger("@req_ctxt_id", rootNode, ASTRA::NoExists);
            addToEdiResponseCtxt(req_ctxt_id, ediResCtxtNode->children, "");
        }
    }
}

static void ChangeStatusToLog(const xmlNodePtr statusNode,
                              const bool repeated,
                              const string lexema_id,
                              LEvntPrms& params,
                              const string &screen,
                              const string &user,
                              const string &desk)
{
  TLogLocale locale;
  locale.ev_type=ASTRA::evtPax;

  if (statusNode!=NULL)
  {
    xmlNodePtr node2=statusNode;

    locale.id1=NodeAsIntegerFast("point_id",node2);
    if (GetNodeFast("reg_no",node2)!=NULL)
    {
      locale.id2=NodeAsIntegerFast("reg_no",node2);
      locale.id3=NodeAsIntegerFast("grp_id",node2);
    };
    if (GetNodeFast("pax_full_name",node2)!=NULL &&
        GetNodeFast("pers_type",node2)!=NULL)
    {
      locale.lexema_id = "EVT.PASSENGER_DATA";
      locale.prms << PrmSmpl<string>("pax_name", NodeAsStringFast("pax_full_name",node2))
                  << PrmElem<string>("pers_type", etPersType, NodeAsStringFast("pers_type",node2));
      PrmLexema lexema("param", lexema_id);
      lexema.prms = params;
      locale.prms << lexema;
    }
    else
    {
      locale.lexema_id = lexema_id;
      locale.prms = params;
    }
  }
  else
  {
    locale.lexema_id = lexema_id;
    locale.prms = params;
  }
  if (!repeated) locale.toDB(screen, user, desk);

  if (statusNode!=NULL)
  {
    SetProp(statusNode,"repeated",(int)repeated);
    xmlNodePtr eventNode = NewTextChild(statusNode, "event");
    LocaleToXML (eventNode, locale.lexema_id, locale.prms);
    if (!repeated &&
        locale.ev_time!=ASTRA::NoExists &&
        locale.ev_order!=ASTRA::NoExists)
    {
      SetProp(eventNode,"ev_time",DateTimeToStr(locale.ev_time, ServerFormatDateTimeAsString));
      SetProp(eventNode,"ev_order",locale.ev_order);
    };
  };
}

void handleEtCosResponse(const edifact::RemoteResults& remRes)
{
    LogTrace(TRACE3) << __FUNCTION__ << " ediSess: " << remRes.ediSession();

    xmlNodePtr rootNode=NULL,ticketNode=NULL;
    int req_ctxt_id=ASTRA::NoExists;
    string screen,user,desk;

    XMLDoc ediSessCtxt;
    AstraEdifact::getEdiSessionCtxt(remRes.ediSession().get(), true, "handleEtCosResponse", ediSessCtxt, false);
    if (ediSessCtxt.docPtr()!=NULL)
    {
      rootNode=NodeAsNode("/context",ediSessCtxt.docPtr());
      ticketNode=NodeAsNode("tickets",rootNode)->children;
      if (GetNode("@req_ctxt_id",rootNode))
        req_ctxt_id=NodeAsInteger("@req_ctxt_id",rootNode);
      screen=NodeAsString("@screen",rootNode);
      user=NodeAsString("@user",rootNode);
      desk=NodeAsString("@desk",rootNode);
    };

    {
      int point_id=-1;
      for(xmlNodePtr node=ticketNode;node!=NULL;node=node->next)
      {
        if (point_id!=NodeAsInteger("point_id",node))
        {
          point_id=NodeAsInteger("point_id",node);
          if (TFltParams::returnOnlineStatus(point_id))
          {
            //����襬 � ���
            TReqInfo::Instance()->LocaleToLog("EVT.RETURNED_INTERACTIVE_WITH_ETC", ASTRA::evtFlt, point_id);
          };
        };
      };
    }

    if(remRes.status()->type() != edifact::RemoteStatus::Timeout)
    {
        ChngStatAnswer chngStatAns = ChngStatAnswer::readEdiTlg(remRes.tlgSource());
        chngStatAns.Trace(TRACE2);
        if (chngStatAns.isGlobErr())
        {
            string err,err_locale;
            LexemaData err_lexeme;
            if (chngStatAns.globErr().second.empty())
            {
              err="������ " + chngStatAns.globErr().first;
              err_lexeme.lexema_id="MSG.ETICK.ETS_ERROR";
              err_lexeme.lparams << LParam("msg", chngStatAns.globErr().first);
            }
            else
            {
              err=chngStatAns.globErr().second;
              err_lexeme.lexema_id="WRAP.ETS";
              err_lexeme.lparams << LParam("text", chngStatAns.globErr().second);
            };

            for(xmlNodePtr node=ticketNode;node!=NULL;node=node->next)
            {
              xmlNodePtr node2=node->children;
              LEvntPrms params;
              params << PrmSmpl<std::string>("ticket_no", NodeAsStringFast("ticket_no",node2))
                     << PrmSmpl<int>("coupon_no", NodeAsIntegerFast("coupon_no",node2))
                     << PrmSmpl<std::string>("err", err);
              xmlNodePtr errNode=NewTextChild(node,"global_error");
              LexemeDataToXML(err_lexeme, errNode);

              TETickItem ETickItem;
              ETickItem.et.no=NodeAsStringFast("ticket_no",node2);
              ETickItem.et.coupon=NodeAsIntegerFast("coupon_no",node2);
              ETickItem.point_id=NodeAsIntegerFast("point_id",node2);
              ETickItem.airp_dep=NodeAsStringFast("airp_dep",node2);
              ETickItem.airp_arv=NodeAsStringFast("airp_arv",node2);
              ETickItem.change_status_error=err;
              ETickItem.toDB(TETickItem::ChangeOfStatus);

              ChangeStatusToLog(errNode, false, "EVT.ETICKET", params, screen, user, desk);
            };
        }
        else
        {
          std::list<Ticket>::const_iterator currTick;

          for(currTick=chngStatAns.ltick().begin();currTick!=chngStatAns.ltick().end();currTick++)
          {
            //���஡㥬 �஠������஢��� �訡�� �஢�� �����
            string err=chngStatAns.err2Tick(currTick->ticknum(), 0);
            if (!err.empty())
            {
              ProgTrace(TRACE5,"ticket=%s error=%s",
                               currTick->ticknum().c_str(), err.c_str());
              LEvntPrms params;
              params << PrmSmpl<std::string>("tick_num", currTick->ticknum())
                     << PrmSmpl<std::string>("err", err);

              LexemaData err_lexeme;
              err_lexeme.lexema_id="MSG.ETICK.CHANGE_STATUS_ERROR";
              err_lexeme.lparams << LParam("ticknum",currTick->ticknum())
                                 << LParam("error",err);

              if (ticketNode!=NULL)
              {
                //���饬 �� ������
                for(xmlNodePtr node=ticketNode;node!=NULL;node=node->next)
                {
                  xmlNodePtr node2=node->children;
                  if (NodeAsStringFast("ticket_no",node2)==currTick->ticknum())
                  {
                    xmlNodePtr errNode=NewTextChild(node,"ticket_error");
                    LexemeDataToXML(err_lexeme, errNode);
                    //��諨 �����
                    TETickItem ETickItem;
                    ETickItem.et.no=NodeAsStringFast("ticket_no",node2);
                    ETickItem.et.coupon=NodeAsIntegerFast("coupon_no",node2);
                    ETickItem.point_id=NodeAsIntegerFast("point_id",node2);
                    ETickItem.airp_dep=NodeAsStringFast("airp_dep",node2);
                    ETickItem.airp_arv=NodeAsStringFast("airp_arv",node2);
                    ETickItem.change_status_error=err;
                    ETickItem.toDB(TETickItem::ChangeOfStatus);

                    ChangeStatusToLog(errNode, false, "EVT.ETICKET_CHANGE_STATUS_MISTAKE", params, screen, user, desk);
                  };
                };
              }
              else
              {
                ChangeStatusToLog(NULL, false, "EVT.ETICKET_CHANGE_STATUS_MISTAKE", params, screen, user, desk);
              };
              continue;
            };

            if (currTick->getCoupon().empty()) continue;

            //���஡㥬 �஠������஢��� �訡�� �஢�� �㯮��
            err = chngStatAns.err2Tick(currTick->ticknum(), currTick->getCoupon().front().couponInfo().num());
            if (!err.empty())
            {
              ProgTrace(TRACE5,"ticket=%s coupon=%d error=%s",
                        currTick->ticknum().c_str(),
                        currTick->getCoupon().front().couponInfo().num(),
                        err.c_str());
              LEvntPrms params;
              ostringstream msgh;
              msgh << currTick->ticknum() << "/" << currTick->getCoupon().front().couponInfo().num();
              params << PrmSmpl<std::string>("tick_num", msgh.str())
                     << PrmSmpl<std::string>("err", err);

              LexemaData err_lexeme;
              err_lexeme.lexema_id="MSG.ETICK.CHANGE_STATUS_ERROR";
              err_lexeme.lparams << LParam("ticknum",currTick->ticknum()+"/"+
                                                     IntToString(currTick->getCoupon().front().couponInfo().num()))
                                 << LParam("error",err);

              if (ticketNode!=NULL)
              {
                //���饬 �� ������
                for(xmlNodePtr node=ticketNode;node!=NULL;node=node->next)
                {
                  xmlNodePtr node2=node->children;
                  if (NodeAsStringFast("ticket_no",node2)==currTick->ticknum() &&
                      NodeAsIntegerFast("coupon_no",node2)==(int)currTick->getCoupon().front().couponInfo().num())
                  {
                    xmlNodePtr errNode=NewTextChild(node,"coupon_error");
                    LexemeDataToXML(err_lexeme, errNode);
                    //��諨 �����
                    TETickItem ETickItem;
                    ETickItem.et.no=NodeAsStringFast("ticket_no",node2);
                    ETickItem.et.coupon=NodeAsIntegerFast("coupon_no",node2);
                    ETickItem.point_id=NodeAsIntegerFast("point_id",node2);
                    ETickItem.airp_dep=NodeAsStringFast("airp_dep",node2);
                    ETickItem.airp_arv=NodeAsStringFast("airp_arv",node2);
                    ETickItem.change_status_error=err;
                    ETickItem.toDB(TETickItem::ChangeOfStatus);

                    ChangeStatusToLog(errNode, false, "EVT.ETICKET_CHANGE_STATUS_MISTAKE", params, screen, user, desk);
                  };
                };
              }
              else
              {
                ChangeStatusToLog(NULL, false, "EVT.ETICKET_CHANGE_STATUS_MISTAKE", params, screen, user, desk);
              };
              continue;
            };


            CouponStatus status(currTick->getCoupon().front().couponInfo().status());

            ProgTrace(TRACE5,"ticket=%s coupon=%d status=%s",
                             currTick->ticknum().c_str(),
                             currTick->getCoupon().front().couponInfo().num(),
                             status->dispCode());

            LEvntPrms params;
            params << PrmSmpl<std::string>("ticket_no", currTick->ticknum())
                   << PrmSmpl<int>("coupon_no", currTick->getCoupon().front().couponInfo().num())
                   << PrmSmpl<std::string>("disp_code", status->dispCode());

            if (ticketNode!=NULL)
            {
              //���饬 �� ������
              for(xmlNodePtr node=ticketNode;node!=NULL;node=node->next)
              {
                xmlNodePtr node2=node->children;
                if (NodeAsStringFast("ticket_no",node2)==currTick->ticknum() &&
                    NodeAsIntegerFast("coupon_no",node2)==(int)currTick->getCoupon().front().couponInfo().num())
                {
                  //������� ����� � ⠡��� etickets
                  //��諨 �����
                  xmlNodePtr statusNode=NewTextChild(node,"coupon_status",status->dispCode());

                  TETickItem ETickItem;
                  ETickItem.et.no=NodeAsStringFast("ticket_no",node2);
                  ETickItem.et.coupon=NodeAsIntegerFast("coupon_no",node2);
                  ETickItem.point_id=NodeAsIntegerFast("point_id",node2);
                  ETickItem.airp_dep=NodeAsStringFast("airp_dep",node2);
                  ETickItem.airp_arv=NodeAsStringFast("airp_arv",node2);
                  if (status->codeInt()!=CouponStatus::OriginalIssue)
                    ETickItem.et.status=CouponStatus(status);
                  else
                    ETickItem.et.status=CouponStatus(CouponStatus::Unavailable);

                  TFltParams fltParams;
                  if (fltParams.get(ETickItem.point_id))
                  {
                    if (!ETStatusInterface::ToDoNothingWhenChangingStatus(fltParams, ETickItem))
                    {
                      TETickItem priorETickItem;
                      priorETickItem.fromDB(ETickItem.et.no, ETickItem.et.coupon, TETickItem::ChangeOfStatus, true);

                      ETickItem.toDB(TETickItem::ChangeOfStatus);

                      bool repeated=(priorETickItem.et.status==ETickItem.et.status);

                      if (ETickItem.et.status==CouponStatus::Unavailable)
                        ETickItem.et.status=CouponStatus(CouponStatus::OriginalIssue);
                      if (!ETStatusInterface::ReturnAirportControl(fltParams, ETickItem))
                        ChangeStatusToLog(statusNode, repeated, "EVT.ETICKET_CHANGE_STATUS", params, screen, user, desk);
                    };
                  }
                };
              };
            }
            else
            {
                ChangeStatusToLog(NULL, false, "EVT.ETICKET_CHANGE_STATUS", params, screen, user, desk);
            }
          }
        }
    }

    addToEdiResponseCtxt(req_ctxt_id, ticketNode, "tickets");
}

void handleEtRacResponse(const edifact::RemoteResults& remRes)
{
    using namespace edifact;

    LogTrace(TRACE3) << __FUNCTION__;

    if(remRes.status() == RemoteStatus::Success)
    {
        try {
            int readStatus = ReadEdiMessage(remRes.tlgSource().c_str());
            ASSERT(readStatus == EDI_MES_OK);
            Ticketing::EdiPnr ediPnr(remRes.tlgSource(), EdiRacRes);
            Ticketing::saveWcPnr(ediPnr);
            ETDisplayToDB(ediPnr);
        }
        catch(std::exception &e) {
            ProgTrace(TRACE5, ">>>> %s: %s", __FUNCTION__, e.what());
        }
        catch(...) {
            ProgTrace(TRACE5, ">>>> %s: unknown error", __FUNCTION__);
        }
    } else {
        LogTrace(TRACE3) << "remote error code: " << remRes.ediErrCode();
    }
}

void handleEtUac(const std::string& uac)
{
    LogTrace(TRACE3) << __FUNCTION__;
    Ticketing::EdiPnr ediPnr(uac, edifact::EdiUacReq);
    Ticketing::saveWcPnr(ediPnr);
    ETDisplayToDB(ediPnr);
}
