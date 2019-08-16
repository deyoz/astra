#include "docs_vo.h"
#include "docs_utils.h"

using namespace AstraLocale;
using namespace std;

void VOUCHERS(TRptParams &rpt_params, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    get_compatible_report_form("vouchers", reqNode, resNode);

    TCachedQuery Qry(
            "select "
            "    confirm_print.pax_id, "
            "    pax.surname||' '||pax.name AS full_name, "
            "    pax.pers_type, "
            "    pax.reg_no, "
            "    pax.ticket_no, "
            "    pax.coupon_no, "
            "    pax.ticket_rem, "
            "    pax.ticket_confirm, "
            "    pax.pers_type, "
            "    pax.seats, "
            "    confirm_print.voucher, "
            "    count(*) total "
            "from "
            "    pax_grp, "
            "    pax, "
            "    confirm_print "
            "where "
            "    pax_grp.point_dep = :point_id and "
            "    pax_grp.grp_id = pax.grp_id and "
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
            "    pax.pers_type, "
            "    pax.seats, "
            "    confirm_print.voucher "
            "order by "
            "    confirm_print.voucher, "
            "    pax.reg_no ",
            QParams()
            << QParam("point_id", otInteger, rpt_params.point_id));
    Qry.get().Execute();

    xmlNodePtr formDataNode = NewTextChild(resNode, "form_data");
    xmlNodePtr variablesNode = NewTextChild(formDataNode, "variables");
    xmlNodePtr dataSetsNode = NewTextChild(formDataNode, "datasets");
    // переменные отчёта
    PaxListVars(rpt_params.point_id, rpt_params, variablesNode);
    // заголовок отчёта
    NewTextChild(variablesNode, "caption", getLocaleText("CAP.DOC.VOUCHERS",
                LParams() << LParam("flight", get_flight(variablesNode)), rpt_params.GetLang()));
    populate_doc_cap(variablesNode, rpt_params.GetLang());

    if(not Qry.get().Eof) {
        xmlNodePtr dataSetNode = NewTextChild(dataSetsNode, "v_vouchers");
        // строки отчёта

        map<string, int> totals;
        for(; not Qry.get().Eof; Qry.get().Next()) {
            string voucher = Qry.get().FieldAsString("voucher");
            int reg_no = Qry.get().FieldAsInteger("reg_no");
            xmlNodePtr rowNode = NewTextChild(dataSetNode, "row");
            string category = rpt_params.ElemIdToReportElem(etVoucherType, voucher, efmtNameLong);
            NewTextChild(rowNode, "category", category);
            NewTextChild(rowNode, "reg_no", reg_no);
            NewTextChild(rowNode, "fio", transliter(Qry.get().FieldAsString("full_name"), 1, rpt_params.GetLang() != AstraLocale::LANG_RU));
            NewTextChild(rowNode, "type", getLocaleText(Qry.get().FieldAsString("pers_type"), rpt_params.GetLang()));

            string ticket_no = Qry.get().FieldAsString("ticket_no");
            string coupon_no = Qry.get().FieldAsString("coupon_no");
            ostringstream ticket;
            if(not ticket_no.empty())
                ticket << ticket_no << (coupon_no.empty() ? "" : "/") << coupon_no;

            NewTextChild(rowNode, "tick_no", ticket.str());

            int quantity = Qry.get().FieldAsInteger("total");
            totals[category] += quantity;
            NewTextChild(rowNode, "quantity", quantity);

            set<string> rems;
            REPORT_PAX_REMS::get_rem_codes(Qry.get(), rpt_params.GetLang(), rems);
            ostringstream rems_list;
            for(set<string>::iterator i = rems.begin(); i != rems.end(); i++) {
                if(rems_list.str().size() != 0)
                    rems_list << " ";
                rems_list << *i;
            }
            NewTextChild(rowNode, "rem", rems_list.str());
        }
        dataSetNode = NewTextChild(dataSetsNode, "totals");
        int total_sum = 0;
        for(map<string, int>::iterator i = totals.begin(); i != totals.end(); i++) {
            xmlNodePtr rowNode = NewTextChild(dataSetNode, "row");
            ostringstream buf;
            buf << i->first << " " << i->second;
            total_sum += i->second;
            NewTextChild(rowNode, "item", buf.str());
        }
        xmlNodePtr rowNode = NewTextChild(dataSetNode, "row");
        ostringstream buf;
        buf << getLocaleText("Всего:", rpt_params.GetLang()) << " " << total_sum;
        NewTextChild(rowNode, "item", buf.str());
    }
}
