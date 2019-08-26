#include "docs_vo.h"
#include "docs_vouchers.h"

#define NICKNAME "DENIS"
#include "serverlib/slogger.h"

using namespace AstraLocale;
using namespace std;
using namespace ASTRA;

void VOUCHERS(TRptParams &rpt_params, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    get_compatible_report_form("vouchers", reqNode, resNode);

    TVouchers vouchers;
    vouchers.fromDB(rpt_params.point_id);

    xmlNodePtr formDataNode = NewTextChild(resNode, "form_data");
    xmlNodePtr variablesNode = NewTextChild(formDataNode, "variables");
    xmlNodePtr dataSetsNode = NewTextChild(formDataNode, "datasets");
    xmlNodePtr dataSetNode = NewTextChild(dataSetsNode, "v_vouchers");
    // переменные отчёта
    PaxListVars(rpt_params.point_id, rpt_params, variablesNode);
    // заголовок отчёта
    NewTextChild(variablesNode, "caption", getLocaleText("CAP.DOC.VOUCHERS",
                LParams() << LParam("flight", get_flight(variablesNode)), rpt_params.GetLang()));
    populate_doc_cap(variablesNode, rpt_params.GetLang());

    if(not vouchers.items.empty()) {
        // строки отчёта

        map<string, int> totals;
        for(const auto &i: vouchers.items) {
            string voucher = i.first.voucher;
            xmlNodePtr rowNode = NewTextChild(dataSetNode, "row");
            string category = rpt_params.ElemIdToReportElem(etVoucherType, voucher, efmtNameLong);
            NewTextChild(rowNode, "category", category);
            NewTextChild(rowNode, "pr_del", i.first.pr_del);
            if(i.first.reg_no == NoExists)
                NewTextChild(rowNode, "reg_no");
            else
                NewTextChild(rowNode, "reg_no", i.first.reg_no);
            NewTextChild(rowNode, "fio", transliter(i.first.full_name, 1, rpt_params.GetLang() != AstraLocale::LANG_RU));
            NewTextChild(rowNode, "type", getLocaleText(i.first.pers_type, rpt_params.GetLang()));

            ostringstream ticket;
            if(not i.first.ticket_no.empty()) {
                ticket << i.first.ticket_no;
                if(i.first.coupon_no != NoExists)
                    ticket << "/" << i.first.coupon_no;
            }

            NewTextChild(rowNode, "tick_no", ticket.str());

            int quantity = i.second;
            totals[category] += quantity;
            NewTextChild(rowNode, "quantity", quantity);

            NewTextChild(rowNode, "rem", i.first.rem_codes);
        }
    }
}
