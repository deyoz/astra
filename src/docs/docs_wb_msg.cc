#include "docs_wb_msg.h"

using namespace AstraLocale;
using namespace std;

void WB_MSG(TRptParams &rpt_params, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    get_compatible_report_form("WB_msg", reqNode, resNode);

    xmlNodePtr formDataNode = NewTextChild(resNode, "form_data");
    xmlNodePtr dataSetsNode = NewTextChild(formDataNode, "datasets");
    xmlNodePtr dataSetNode = NewTextChild(dataSetsNode, "msg");

    TCachedQuery Qry("select id, time_receive from wb_msg where point_id = :point_id and msg_type = :msg_type order by id desc",
            QParams()
            << QParam("point_id", otInteger, rpt_params.point_id)
            << QParam("msg_type", otString, EncodeRptType(rpt_params.rpt_type))
            );
    Qry.get().Execute();
    if(Qry.get().Eof)
        throw UserException("MSG.NOT_DATA");

    xmlNodePtr rowNode = NewTextChild(dataSetNode, "row");
    int id = Qry.get().FieldAsInteger("id");
    // TDateTime time_receive = Qry.get().FieldAsDateTime("time_receive");
    TCachedQuery txtQry("select text from wb_msg_text where id = :id order by page_no",
            QParams() << QParam("id", otInteger, id));
    txtQry.get().Execute();
    string text;
    for(; not txtQry.get().Eof; txtQry.get().Next())
        text += txtQry.get().FieldAsString("text");
    NewTextChild(rowNode, "text", text);

    // Теперь переменные отчета
    xmlNodePtr variablesNode = NewTextChild(formDataNode, "variables");
    NewTextChild(variablesNode, "caption", EncodeRptType(rpt_params.rpt_type));
}

