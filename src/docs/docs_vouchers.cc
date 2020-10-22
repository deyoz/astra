#include "docs_vouchers.h"
#include "docs_utils.h"

#define NICKNAME "DENIS"
#include "serverlib/slogger.h"

using namespace std;
using namespace ASTRA;
using namespace AstraLocale;

void TVouchers::TItems::add(TQuery &Qry, bool pr_del)
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

void TVouchers::TPaxInfo::fromDB(TQuery &Qry, bool pr_del)
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
    TCachedQuery Qry(
            "begin "
            "  insert into del_vo ( "
            "    point_id, "
            "    full_name, "
            "    pers_type, "
            "    reg_no, "
            "    ticket_no, "
            "    coupon_no, "
            "    rem_codes, "
            "    voucher, "
            "    total "
            "  ) values ( "
            "    :point_id, "
            "    :full_name, "
            "    :pers_type, "
            "    :reg_no, "
            "    :ticket_no, "
            "    :coupon_no, "
            "    :rem_codes, "
            "    :voucher, "
            "    :total "
            "  ); "
            "exception "
            "  when dup_val_on_index then "
            "    update del_vo set total = total + :total where "
            "      point_id = :point_id and "
            "      full_name = :full_name and "
            "      pers_type = :pers_type and "
            "      reg_no = :reg_no and "
            "      nvl(ticket_no, ' ') = nvl(:ticket_no, ' ') and "
            "      nvl(coupon_no, 0) = nvl(:coupon_no, 0) and "
            "      nvl(rem_codes, ' ') = nvl(:rem_codes, ' ') and "
            "      voucher = :voucher;"
            "end; ",
        QParams()
            << QParam("point_id", otInteger, point_id)
            << QParam("full_name", otString)
            << QParam("pers_type", otString)
            << QParam("reg_no", otInteger)
            << QParam("ticket_no", otString)
            << QParam("coupon_no", otInteger)
            << QParam("rem_codes", otString)
            << QParam("voucher", otString)
            << QParam("total", otInteger));
    for(const auto &i: items) {
        Qry.get().SetVariable("full_name", i.first.full_name);
        Qry.get().SetVariable("pers_type", i.first.pers_type);
        Qry.get().SetVariable("reg_no", i.first.reg_no);
        Qry.get().SetVariable("ticket_no", i.first.ticket_no);
        if(i.first.coupon_no != NoExists)
            Qry.get().SetVariable("coupon_no", i.first.coupon_no);
        else
            Qry.get().SetVariable("coupon_no", FNull);
        Qry.get().SetVariable("rem_codes", i.first.rem_codes.substr(0, 500));
        Qry.get().SetVariable("voucher", i.first.voucher);
        Qry.get().SetVariable("total", i.second);
        Qry.get().Execute();
    }
}

const TVouchers &TVouchers::fromDB(int point_id)
{
    return fromDB(point_id, NoExists);
}

const TVouchers &TVouchers::fromDB(int point_id, int grp_id)
{
    tst();
    clear();
    this->point_id = point_id;
    string SQLText =
        "select "
        "    confirm_print.pax_id, "
        "    pax.surname||' '||pax.name AS full_name, "
        "    pax.pers_type, "
        "    pax.reg_no, "
        "    pax.ticket_no, "
        "    pax.coupon_no, "
        "    pax.ticket_rem, "
        "    pax.ticket_confirm, "
        "    pax.seats, "
        "    confirm_print.voucher, "
        "    count(*) total "
        "from ";
    if(grp_id == NoExists)
        SQLText += "    pax_grp, ";
    SQLText +=
        "    pax, "
        "    confirm_print "
        "where ";
    if(grp_id == NoExists)
        SQLText +=
            "    pax_grp.point_dep = :id and "
            "    pax_grp.grp_id = pax.grp_id and ";
    else
        SQLText +=
            "    pax.grp_id = :id and "
            "    pax.refuse = '€' and ";
    SQLText +=
        "    pax.pax_id = confirm_print.pax_id and "
        "    confirm_print.voucher is not null and "
        "    confirm_print.pr_print <> 0 "
        "group by "
        "    confirm_print.pax_id, "
        "    pax.surname||' '||pax.name, "
        "    pax.pers_type, "
        "    pax.reg_no, "
        "    pax.ticket_no, "
        "    pax.coupon_no, "
        "    pax.ticket_rem, "
        "    pax.ticket_confirm, "
        "    pax.seats, "
        "    confirm_print.voucher ";
    TCachedQuery Qry(SQLText, QParams() << QParam("id", otInteger, (grp_id == NoExists ? point_id : grp_id)));
    items.add(Qry.get(), false);
    tst();
    if(grp_id == NoExists) {
        tst();
        TCachedQuery delVoQry("select * from del_vo where point_id = :point_id",
                QParams() << QParam("point_id", otInteger, point_id));
        delVoQry.get().Execute();
        items.add(delVoQry.get(), true);
    tst();
        TCachedQuery unregQry(
                "select "
                "   surname||' '||name AS full_name, "
                "   voucher, "
                "   count(*) total "
                "from "
                "  confirm_print_vo_unreg "
                "where "
                "   point_id = :point_id and "
                "   pr_print <> 0 "
                "group by "
                "   surname, "
                "   name, "
                "   voucher ",
                QParams() << QParam("point_id", otInteger, point_id));
        unregQry.get().Execute();
        items.add(unregQry.get(), false);
    }
    return *this;
}
