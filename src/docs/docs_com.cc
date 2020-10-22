#include "docs_com.h"
#include "typeb_utils.h"
#include "telegram.h"

namespace DOCS {

void COM(TRptParams &rpt_params, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    get_compatible_report_form("WB_msg", reqNode, resNode);

    xmlNodePtr formDataNode = NewTextChild(resNode, "form_data");
    xmlNodePtr dataSetsNode = NewTextChild(formDataNode, "datasets");
    xmlNodePtr dataSetNode = NewTextChild(dataSetsNode, "msg");

    TypeB::TCreateInfo info("COM", TypeB::TCreatePoint());
    info.point_id = rpt_params.point_id;
    TTypeBTypesRow tlgTypeInfo;
    int tlg_id = TelegramInterface::create_tlg(info, ASTRA::NoExists, tlgTypeInfo, true);
    TCachedQuery TlgQry("SELECT * FROM tlg_out WHERE id=:id ORDER BY num",
            QParams() << QParam("id", otInteger, tlg_id));
    TlgQry.get().Execute();
    for(;!TlgQry.get().Eof;TlgQry.get().Next())
    {
        TTlgOutPartInfo tlg;
        tlg.fromDB(TlgQry.get());
        xmlNodePtr rowNode = NewTextChild(dataSetNode, "row");
        NewTextChild(rowNode, "text", tlg.heading + tlg.body + tlg.ending);
    }

    TypeB::TDetailCreateInfo detail_info;
    detail_info.create_point = info.create_point;
    detail_info.copy(info);
    detail_info.point_id = info.point_id;
    detail_info.lang = AstraLocale::LANG_RU;
    detail_info.elem_fmt = prLatToElemFmt(efmtCodeNative, detail_info.get_options().is_lat);
    detail_info.time_create = NowUTC();

    TTlgSeatList seats;
//    seats.apply_comp(detail_info, false);

    // Теперь переменные отчета
    xmlNodePtr variablesNode = NewTextChild(formDataNode, "variables");
    NewTextChild(variablesNode, "caption", EncodeRptType(rpt_params.rpt_type));
    ASTRA::rollback();
}

}
