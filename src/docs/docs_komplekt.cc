#include "docs_komplekt.h"
#include "docs_main.h"

#define NICKNAME "DENIS"
#include "serverlib/slogger.h"

using namespace AstraLocale;
using namespace std;
using namespace EXCEPTIONS;

void KOMPLEKT(TRptParams &rpt_params, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    if(not TReqInfo::Instance()->desk.compatible(KOMPLEKT_VERSION))
        throw UserException("MSG.KOMPLEKT_SUPPORT",
                LParams()
                << LParam("rpt", ElemIdToNameLong(etReportType, EncodeRptType(rpt_params.rpt_type)))
                << LParam("vers", KOMPLEKT_VERSION));
  struct TReportItem {
      string code;
      int num;
      int sort_order;
      bool operator < (const TReportItem &other) const
      {
          return sort_order < other.sort_order;
      }
      TReportItem(const TReportItem &other)
      {
          code = other.code;
          num = other.num;
          sort_order = other.sort_order;
      }
      TReportItem(
              const string &_code,
              int _num,
              int _sort_order):
          code(_code),
          num(_num),
          sort_order(_sort_order)
      {}
  };

  TTripInfo info;
  info.getByPointId(rpt_params.point_id);
  TQuery Qry( &OraSession );
  Qry.Clear();
  Qry.SQLText =
    "SELECT report_type, "
    "       num, "
    "       report_types.sort_order, "
    "    DECODE(airline,NULL,0,8)+ "
    "    DECODE(flt_no,NULL,0,2)+ "
    "    DECODE(airp_dep,NULL,0,4) AS priority "
    "FROM doc_num_copies, report_types "
    "WHERE (airline IS NULL OR airline=:airline) AND "
    "      (flt_no IS NULL OR flt_no=:flt_no) AND "
    "      (airp_dep IS NULL OR airp_dep=:airp_dep) and "
    "      doc_num_copies.report_type = report_types.code ";

  Qry.CreateVariable("airline",otString,info.airline);
  Qry.CreateVariable("flt_no",otInteger,info.flt_no);
  Qry.CreateVariable("airp_dep",otString,info.airp);
  Qry.Execute();

  // получаем для каждого типа отчёта все варианты количества
  map< TReportItem, map<int, int> > temp;
  while (!Qry.Eof)
  {
      TReportItem item(
              Qry.FieldAsString("report_type"),
              Qry.FieldAsInteger("num"),
              Qry.FieldAsInteger("sort_order"));
      int priority = Qry.FieldAsInteger("priority");
//    LogTrace(TRACE5) << "report_type=" << report_type
//                     << " priority=" << priority
//                     << " num=" << num;
      temp[item][priority] = item.num;
      Qry.Next();
  }


  // выбираем для каждого типа отчёта количество с наивысшим приоритетом
  set<TReportItem> nums;
  for (auto r : temp) {
      TReportItem item(r.first);
      item.num = r.second.rbegin()->second;
      nums.insert(item);
  }
  // формирование отчёта
  get_compatible_report_form("komplekt", reqNode, resNode);
  xmlNodePtr formDataNode = NewTextChild(resNode, "form_data");
  xmlNodePtr dataSetsNode = NewTextChild(formDataNode, "datasets");
  xmlNodePtr dataSetNode = NewTextChild(dataSetsNode, "v_komplekt");
  // переменные отчёта
  xmlNodePtr variablesNode = NewTextChild(formDataNode, "variables");
  PaxListVars(rpt_params.point_id, rpt_params, variablesNode);
  // заголовки отчёта
  NewTextChild(variablesNode, "caption", getLocaleText("CAP.DOC.KOMPLEKT", rpt_params.GetLang()));
  populate_doc_cap(variablesNode, rpt_params.GetLang());
  NewTextChild(variablesNode, "doc_cap_komplekt",
      getLocaleText("CAP.DOC.KOMPLEKT_HEADER",
                    LParams() << LParam("airline", NodeAsString("airline_name", variablesNode))
                              << LParam("flight", get_flight(variablesNode))
                              << LParam("route", NodeAsString("long_route", variablesNode)),
                    rpt_params.GetLang()));
  NewTextChild(variablesNode, "komplekt_empty", (nums.empty() ? getLocaleText("MSG.KOMPLEKT_EMPTY", rpt_params.GetLang()) : ""));
  for (auto r : nums)
  {
    xmlNodePtr rowNode = NewTextChild(dataSetNode, "row");
    NewTextChild(rowNode, "code", r.code);
    NewTextChild(rowNode, "name", ElemIdToNameLong(etReportType, r.code));
    NewTextChild(rowNode, "num", r.num);
  }
}

void DocsInterface::print_komplekt(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    xmlNodePtr codesNode = NodeAsNode("codes", reqNode);
    codesNode = codesNode->children;
    xmlNodePtr reportListNode = NULL;
    for(; codesNode; codesNode = codesNode->next) {
        xmlNodePtr currNode = codesNode->children;
        string code = NodeAsStringFast("code", currNode);

        xmlNodePtr LoadFormNode = GetNodeFast("LoadForm", currNode);
        xmlNodePtr reqLoadFormNode = GetNode("LoadForm", reqNode);

        if(reqLoadFormNode) {
            if(not LoadFormNode)
                RemoveNode(reqLoadFormNode);
        } else {
            if(LoadFormNode)
                NewTextChild(reqNode, "LoadForm");
        }

        xmlNodePtr rptTypeNode = GetNode("rpt_type", reqNode);
        if(rptTypeNode)
            NodeSetContent(rptTypeNode, code);
        else
            NewTextChild(reqNode, "rpt_type", code);

        // Некоторые отчеты, напр. GOSHO требуют, чтобы структура
        // XML была следующая: /term/answer
        XMLDoc doc("term");
        xmlNodePtr reportNode = doc.docPtr()->children;
        xmlNodePtr answerNode = NewTextChild(reportNode, "answer");
        SetProp(reportNode, "code", code);

        try {
            RunReport2(ctxt, reqNode, answerNode);

            if(not reportListNode)
                reportListNode = NewTextChild(resNode, "report_list");

            CopyNode(reportListNode, reportNode);
        } catch(Exception &E) {
            LogTrace(TRACE5) << __FUNCTION__ << ": " << code << " failed: " << E.what();
        }

        // break;
    }
}

