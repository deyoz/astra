#include "docs_vouchers.h"
#include "docs_utils.h"

#define NICKNAME "DENIS"
#include "serverlib/slogger.h"

using namespace std;
using namespace ASTRA;
using namespace AstraLocale;

void TVouchers::TItems::add(DB::TQuery &Qry, bool pr_del)
{
    Qry.Execute();
    for(; not Qry.Eof; Qry.Next()) {
        int total = Qry.FieldAsInteger("total");
        auto res = emplace(TPaxInfo(Qry, pr_del), total);
        if(not res.second) // already exists
            res.first->second += total;
    }
}

void TVouchers::TPaxInfo::clear()
{
    full_name.clear();
    pers_type.clear();
    reg_no = NoExists;
    ticket_no.clear();
    coupon_no = NoExists;
    rem_codes.clear();
    voucher.clear();
    pr_del = false;
}

TVouchers::TPaxInfo::TPaxInfo(const CheckIn::TSimplePaxItem& pax,
                              const string& voucher)
{
  clear();
  if (pax.reg_no >= 0) {
      pers_type = EncodePerson(pax.pers_type);
      reg_no = pax.reg_no;
      ticket_no = pax.tkn.no;
      if (pax.tkn.coupon != ASTRA::NoExists and pax.tkn.coupon > 0) {
          coupon_no = pax.tkn.coupon;
      }
      std::set<std::string> rems;
      REPORT_PAX_REMS::get_rem_codes(pax.paxId(), pax.pers_type, pax.seats,
                                     pax.tkn, LANG_EN, rems);
      for(set<string>::iterator i = rems.begin(); i != rems.end(); i++) {
          if(rem_codes.size() != 0)
              rem_codes += " ";
          rem_codes += *i;
      }
  }
  full_name = pax.full_name();
  this->voucher = voucher;
  this->pr_del = false;
}

void TVouchers::TPaxInfo::fromDB(DB::TQuery &Qry, bool pr_del)
{
    clear();
    if(Qry.Eof) return;
    if(Qry.GetFieldIndex("reg_no") >= 0) {
        pers_type = Qry.FieldAsString("pers_type");
        reg_no = Qry.FieldAsInteger("reg_no");
        ticket_no = Qry.FieldAsString("ticket_no");
        if(not Qry.FieldIsNULL("coupon_no"))
            coupon_no = Qry.FieldAsInteger("coupon_no");
        if(pr_del)
            rem_codes = Qry.FieldAsString("rem_codes");
        else {
            set<string> rems;
            REPORT_PAX_REMS::get_rem_codes(Qry, LANG_EN, rems);
            for(set<string>::iterator i = rems.begin(); i != rems.end(); i++) {
                if(rem_codes.size() != 0)
                    rem_codes += " ";
                rem_codes += *i;
            }
        }
    }
    full_name = Qry.FieldAsString("full_name");
    voucher = Qry.FieldAsString("voucher");
    this->pr_del = pr_del;
}

bool TVouchers::TPaxInfo::operator < (const TPaxInfo &item) const
{
    if(voucher != item.voucher)
        return voucher < item.voucher;
    if(pr_del != item.pr_del)
        return pr_del > item.pr_del;
    if(reg_no != item.reg_no)
        return reg_no < item.reg_no;
    if(full_name != item.full_name)
        return full_name < item.full_name;
    if(pers_type != item.pers_type)
        return pers_type < item.pers_type;
    if(ticket_no != item.ticket_no)
        return ticket_no < item.ticket_no;
    if(coupon_no != item.coupon_no)
        return coupon_no < item.coupon_no;
    return rem_codes < item.rem_codes;
}

void TVouchers::clear()
{
    point_id = NoExists;
    items.clear();
}

void TVouchers::to_deleted() const
{
    if(items.empty()) return;

    for(const auto &i: items) {
        QParams params;
        params << QParam("point_id", otInteger, point_id)
               << QParam("full_name", otString, i.first.full_name)
               << QParam("pers_type", otString, i.first.pers_type)
               << QParam("reg_no", otInteger, i.first.reg_no)
               << QParam("ticket_no", otString, i.first.ticket_no)
               << QParam("coupon_no", otInteger, i.first.coupon_no, QParam::BindOption::NullOnEmpty)
               << QParam("rem_codes", otString, i.first.rem_codes.substr(0, 500))
               << QParam("voucher", otString, i.first.voucher)
               << QParam("total", otInteger, i.second);

        DB::TCachedQuery updQry(
              PgOra::getRWSession("DEL_VO"),
              "UPDATE del_vo "
              "SET total = total + :total "
              "WHERE "
              "  point_id = :point_id AND "
              "  full_name = :full_name AND "
              "  pers_type = :pers_type AND "
              "  reg_no = :reg_no AND "
              "  COALESCE(ticket_no, ' ') = COALESCE(:ticket_no, ' ') AND "
              "  COALESCE(coupon_no, 0) = COALESCE(:coupon_no, 0) AND "
              "  COALESCE(rem_codes, ' ') = COALESCE(:rem_codes, ' ') AND "
              "  voucher = :voucher ",
              params,
              STDLOG);
        updQry.get().Execute();

        if (updQry.get().RowsProcessed() == 0) {
            DB::TCachedQuery insQry(
                  PgOra::getRWSession("DEL_VO"),
                  "INSERT INTO del_vo ( "
                  "  point_id, "
                  "  full_name, "
                  "  pers_type, "
                  "  reg_no, "
                  "  ticket_no, "
                  "  coupon_no, "
                  "  rem_codes, "
                  "  voucher, "
                  "  total "
                  ") VALUES ( "
                  "  :point_id, "
                  "  :full_name, "
                  "  :pers_type, "
                  "  :reg_no, "
                  "  :ticket_no, "
                  "  :coupon_no, "
                  "  :rem_codes, "
                  "  :voucher, "
                  "  :total "
                  ") ",
                  params,
                  STDLOG);
            insQry.get().Execute();
        }
    }
}

const TVouchers &TVouchers::fromDB(int point_id)
{
    return fromDB(point_id, NoExists);
}

const TVouchers &TVouchers::fromDB(int point_id, int grp_id)
{
    clear();
    const std::list<CheckIn::TSimplePaxItem> paxes =
        grp_id == ASTRA::NoExists
        ? CheckIn::TSimplePaxItem::getByDepPointId(PointId_t(point_id))
        : CheckIn::TSimplePaxItem::getByGrpId(GrpId_t(grp_id));
    std::map<PaxId_t, CheckIn::TSimplePaxItem> pax_map;
    for (const CheckIn::TSimplePaxItem& pax: paxes) {
        if (pax.refuse != "A") {
          continue;
        }
        pax_map.emplace(PaxId_t(pax.paxId()), pax);
    }

    DB::TQuery QryConfirm(PgOra::getROSession("CONFIRM_PRINT"), STDLOG);
    QryConfirm.SQLText = "SELECT pax_id, voucher "
                         "FROM confirm_print "
                         "WHERE voucher IS NOT NULL "
                         "AND pr_print <> 0 ";
    if (grp_id == ASTRA::NoExists) {
        QryConfirm.SQLText += "AND point_id = :point_id ";
        QryConfirm.CreateVariable("point_id", otInteger, point_id);
    } else {
        QryConfirm.SQLText += "AND grp_id = :grp_id ";
        QryConfirm.CreateVariable("grp_id", otInteger, grp_id);
    }
    QryConfirm.Execute();

    for(; !QryConfirm.Eof; QryConfirm.Next()) {
        const PaxId_t pax_id(QryConfirm.FieldAsInteger("pax_id"));
        auto pax_pos = pax_map.find(pax_id);
        if (pax_pos == pax_map.end()) {
            continue;
        }
        const CheckIn::TSimplePaxItem& pax = pax_pos->second;
        const std::string& voucher = QryConfirm.FieldAsString("voucher");
        auto item_pos = items.emplace(TPaxInfo(pax, voucher), 1);
        const bool item_exists = !item_pos.second;
        if (item_exists) {
            int& total = item_pos.first->second;
            total += 1;
        }
    }
    tst();
    if (grp_id == ASTRA::NoExists) {
        tst();
        DB::TCachedQuery delVoQry(
              PgOra::getROSession("DEL_VO"),
              "SELECT * FROM del_vo "
              "WHERE point_id = :point_id",
              QParams() << QParam("point_id", otInteger, point_id),
              STDLOG);
        delVoQry.get().Execute();
        items.add(delVoQry.get(), true);
    tst();
        DB::TCachedQuery unregQry(
              PgOra::getROSession("CONFIRM_PRINT_VO_UNREG"),
              "SELECT "
              "   RTRIM(COALESCE(surname,'')||' '||COALESCE(name,'')) AS full_name, "
              "   voucher, "
              "   count(*) total "
              "FROM "
              "  confirm_print_vo_unreg "
              "WHERE "
              "   point_id = :point_id AND "
              "   pr_print <> 0 "
              "GROUP BY "
              "   surname, "
              "   name, "
              "   voucher ",
              QParams() << QParam("point_id", otInteger, point_id),
              STDLOG);
        unregQry.get().Execute();
        items.add(unregQry.get(), false);
    }
    return *this;
}
