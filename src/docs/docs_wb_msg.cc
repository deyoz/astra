#include "docs_wb_msg.h"
#include "db_tquery.h"
#include "PgOraConfig.h"

#define NICKNAME "DEN"
#include <serverlib/slogger.h>

using namespace AstraLocale;
using namespace std;

void WB_MSG(TRptParams &rpt_params, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    get_compatible_report_form("WB_msg", reqNode, resNode);

    xmlNodePtr formDataNode = NewTextChild(resNode, "form_data");
    xmlNodePtr dataSetsNode = NewTextChild(formDataNode, "datasets");
    xmlNodePtr dataSetNode = NewTextChild(dataSetsNode, "msg");

    DB::TQuery Qry(PgOra::getROSession("WB_MSG"));
    Qry.SQLText = "select id, time_receive from wb_msg where point_id = :point_id and msg_type = :msg_type order by id desc";
    Qry.CreateVariable("point_id", otInteger, rpt_params.point_id);
    Qry.CreateVariable("msg_type", otString, EncodeRptType(rpt_params.rpt_type));
    Qry.Execute();
    if(Qry.Eof)
        throw UserException("MSG.NOT_DATA");

    xmlNodePtr rowNode = NewTextChild(dataSetNode, "row");
    int id = Qry.FieldAsInteger("id");
    // TDateTime time_receive = Qry.get().FieldAsDateTime("time_receive");
    DB::TQuery txtQry(PgOra::getROSession("WB_MSG_TEXT"));
    txtQry.SQLText = "select text from wb_msg_text where id = :id order by page_no";
    txtQry.CreateVariable("id", otInteger, id);
    txtQry.Execute();
    string text;
    for(; not txtQry.Eof; txtQry.Next())
        text += txtQry.FieldAsString("text");
    NewTextChild(rowNode, "text", text);

    // Теперь переменные отчета
    xmlNodePtr variablesNode = NewTextChild(formDataNode, "variables");
    NewTextChild(variablesNode, "caption", EncodeRptType(rpt_params.rpt_type));
}

