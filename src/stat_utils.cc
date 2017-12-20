#include "stat_utils.h"
#include "xml_unit.h"
#include "astra_utils.h"

using namespace std;
using namespace AstraLocale;
using namespace BASIC::date_time;

bool STAT::bad_client_img_version()
{
    return TReqInfo::Instance()->desk.compatible("201101-0117116") and not TReqInfo::Instance()->desk.compatible("201101-0118748");
}


xmlNodePtr STAT::getVariablesNode(xmlNodePtr resNode)
{
    xmlNodePtr formDataNode = GetNode("form_data", resNode);
    if(!formDataNode)
        formDataNode = NewTextChild(resNode, "form_data");
    xmlNodePtr variablesNode = GetNode("variables", formDataNode);
    if(!variablesNode)
        variablesNode = NewTextChild(formDataNode, "variables");
    return variablesNode;
}

xmlNodePtr STAT::set_variables(xmlNodePtr resNode, string lang)
{
    if(lang.empty())
        lang = TReqInfo::Instance()->desk.lang;

    xmlNodePtr variablesNode = getVariablesNode(resNode);

    TReqInfo *reqInfo = TReqInfo::Instance();
    TDateTime issued = NowUTC();
    string tz;
    if(reqInfo->user.sets.time == ustTimeUTC)
        tz = "(GMT)";
    else if(
            reqInfo->user.sets.time == ustTimeLocalDesk ||
            reqInfo->user.sets.time == ustTimeLocalAirp
           ) {
        issued = UTCToLocal(issued,reqInfo->desk.tz_region);
        tz = "(" + ElemIdToCodeNative(etCity, reqInfo->desk.city) + ")";
    }

    NewTextChild(variablesNode, "print_date",
            DateTimeToStr(issued, "dd.mm.yyyy hh:nn:ss ") + tz);
    NewTextChild(variablesNode, "print_oper", reqInfo->user.login);
    NewTextChild(variablesNode, "print_term", reqInfo->desk.code);
    NewTextChild(variablesNode, "use_seances", false); //!!!потом убрать
    NewTextChild(variablesNode, "test_server", bad_client_img_version() ? 2 : get_test_server());
    if(bad_client_img_version())
        NewTextChild(variablesNode, "doc_cap_test", " ");
    NewTextChild(variablesNode, "cap_test", getLocaleText("CAP.TEST", lang));
    NewTextChild(variablesNode, "page_number_fmt", getLocaleText("CAP.PAGE_NUMBER_FMT", lang));
    NewTextChild(variablesNode, "short_page_number_fmt", getLocaleText("CAP.SHORT_PAGE_NUMBER_FMT", lang));
    NewTextChild(variablesNode, "oper_info", getLocaleText("CAP.DOC.OPER_INFO", LParams()
                << LParam("date", DateTimeToStr(issued, "dd.mm.yyyy hh:nn:ss ") + tz)
                << LParam("oper", reqInfo->user.login)
                << LParam("term", reqInfo->desk.code),
                lang
                ));
    NewTextChild(variablesNode, "skip_header", 0);
    return variablesNode;
}
