#include "stat_main.h"
#include "qrys.h"
#include "xml_unit.h"
#include "astra_utils.h"
#include "report_common.h"
#include "docs/docs_common.h"
#include "stat_utils.h"

#define NICKNAME "DENIS"
#include "serverlib/slogger.h"

using namespace std;
using namespace ASTRA;
using namespace EXCEPTIONS;
using namespace AstraLocale;
using namespace BASIC::date_time;

void StatInterface::AHM(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    get_compatible_report_form("stat", reqNode, resNode);
    TDateTime FirstDate = NoExists;
    TDateTime LastDate = NoExists;
    string airline, category, bort;
    xmlNodePtr curNode = reqNode->children;
    curNode = NodeAsNodeFast("ParamsPnl", curNode)->children;
    if(curNode) {
        FirstDate = NodeAsDateTimeFast("FirstDate", curNode);
        LastDate = NodeAsDateTimeFast("LastDate", curNode);
        TElemFmt fmt;
        airline = ElemToElemId(etAirline, NodeAsStringFast("Airline", curNode, ""), fmt);
        category = NodeAsStringFast("AHMCategory", curNode, "");
        bort = NodeAsStringFast("bort", curNode, "");
    } else
        throw Exception("wrong req");
    if(FirstDate > LastDate)
        throw UserException("MSG.INVALID_RANGE");
    if(FirstDate == LastDate)
        LastDate += 1;

    QParams QryParams;
    QryParams
        << QParam("lang", otString, TReqInfo::Instance()->desk.lang)
        << QParam("FirstDate", otDate, FirstDate)
        << QParam("LastDate", otDate, LastDate)
        << QParam("evtAhm", otString, EncodeEventType(evtAhm));

    string SQLText =
        "SELECT msg, time, ev_user, station, ev_order, ahm_dict.*, airlines.code airline_code "
        "FROM "
        "  events_bilingual, "
        "  ahm_dict, "
        "  airlines "
        "WHERE "
        "  events_bilingual.lang = :lang AND "
        "  events_bilingual.time >= :FirstDate and "
        "  events_bilingual.time < :LastDate and "
        "  events_bilingual.type = :evtAhm and "
        "  events_bilingual.id1 = ahm_dict.id and "
        "  ahm_dict.airline = airlines.id ";
    if(not airline.empty()) {
        SQLText += " and airlines.code = :airline ";
        QryParams << QParam("airline", otString, airline);
    }
    if(not category.empty()) {
        SQLText += " and category = :category ";
        QryParams << QParam("category", otString, category);
    }
    if(not bort.empty()) {
        SQLText += " and bort_num = :bort ";
        QryParams << QParam("bort", otString, bort);
    }

    SQLText +=
        " ORDER BY ev_order";

    TCachedQuery Qry(SQLText, QryParams);
    Qry.get().Execute();
    if(not Qry.get().Eof) {
        xmlNodePtr ctrlNode = NewTextChild(resNode, "ctrl");
        NewTextChild(ctrlNode, "name", "LogGrd");
        xmlNodePtr dataNode = NewTextChild(ctrlNode, "data");
        xmlNodePtr headerNode = NewTextChild(dataNode, "header");
        xmlNodePtr rowsNode = NewTextChild(dataNode, "rows");

        xmlNodePtr colNode = NewTextChild(headerNode, "col", getLocaleText("Агент"));
        SetProp(colNode, "width", 100);
        SetProp(colNode, "align", TAlignment::LeftJustify);
        SetProp(colNode, "sort", sortString);
        colNode = NewTextChild(headerNode, "col", getLocaleText("Стойка"));
        SetProp(colNode, "width", 100);
        SetProp(colNode, "align", TAlignment::LeftJustify);
        SetProp(colNode, "sort", sortString);
        colNode = NewTextChild(headerNode, "col", getLocaleText("АК"));
        SetProp(colNode, "width", 100);
        SetProp(colNode, "align", TAlignment::LeftJustify);
        SetProp(colNode, "sort", sortString);
        colNode = NewTextChild(headerNode, "col", getLocaleText("Категория"));
        SetProp(colNode, "width", 100);
        SetProp(colNode, "align", TAlignment::LeftJustify);
        SetProp(colNode, "sort", sortString);
        colNode = NewTextChild(headerNode, "col", getLocaleText("Борт"));
        SetProp(colNode, "width", 100);
        SetProp(colNode, "align", TAlignment::LeftJustify);
        SetProp(colNode, "sort", sortString);
        colNode = NewTextChild(headerNode, "col", getLocaleText("Время"));
        SetProp(colNode, "width", 110);
        SetProp(colNode, "align", TAlignment::LeftJustify);
        SetProp(colNode, "sort", sortString);
        colNode = NewTextChild(headerNode, "col", getLocaleText("Операция"));
        SetProp(colNode, "width", 1000);
        SetProp(colNode, "align", TAlignment::LeftJustify);
        SetProp(colNode, "sort", sortString);

        xmlNodePtr rowNode;
        for(; not Qry.get().Eof; Qry.get().Next()) {
            rowNode = NewTextChild(rowsNode, "row");
            NewTextChild(rowNode, "col", Qry.get().FieldAsString("ev_user"));
            NewTextChild(rowNode, "col", Qry.get().FieldAsString("station"));
            NewTextChild(rowNode, "col", ElemIdToCodeNative(etAirline, Qry.get().FieldAsString("airline_code")));
            NewTextChild(rowNode, "col", Qry.get().FieldAsString("category"));
            NewTextChild(rowNode, "col", Qry.get().FieldAsString("bort_num"));
            NewTextChild(rowNode, "col", DateTimeToStr(Qry.get().FieldAsDateTime("time")));
            NewTextChild(rowNode, "col", Qry.get().FieldAsString("msg"));
        }
        xmlNodePtr variablesNode = STAT::set_variables(resNode);
        NewTextChild(variablesNode, "stat_type", statAHM);
        NewTextChild(variablesNode, "stat_mode", "AHM");
        NewTextChild(variablesNode, "stat_type_caption");

        LogTrace(TRACE5) << GetXMLDocText(resNode->doc);
    } else
        throw UserException("MSG.NOT_DATA");
}
