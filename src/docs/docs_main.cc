#include "docs_main.h"
#include "docs_common.h"
#include "stat/stat_utils.h"
#include "docs_ptm.h"
#include "docs_btm.h"
#include "docs_ptm_btm_txt.h"
#include "docs_exam.h"
#include "docs_refuse.h"
#include "docs_notpres.h"
#include "docs_rem.h"
#include "docs_crs.h"
#include "docs_emd.h"
#include "docs_wb_msg.h"
#include "docs_annul_tags.h"
#include "docs_vo.h"
#include "docs_services.h"
#include "docs_reseat.h"
#include "docs_komplekt.h"

#define NICKNAME "DENIS"
#include "serverlib/slogger.h"

using namespace std;
using namespace ASTRA;
using namespace AstraLocale;

void SPPCentrovka(TDateTime date, xmlNodePtr resNode)
{
}

void SPPCargo(TDateTime date, xmlNodePtr resNode)
{
}

void SPPCex(TDateTime date, xmlNodePtr resNode)
{
}

void  DocsInterface::RunSPP(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    xmlNodePtr node = reqNode->children;
    string name = NodeAsStringFast("name", node);
    get_new_report_form(name, reqNode, resNode);
    xmlNodePtr formDataNode = NewTextChild(resNode, "form_data");
    xmlNodePtr variablesNode = NewTextChild(formDataNode, "variables");
    NewTextChild(variablesNode, "test_server", STAT::bad_client_img_version() ? 2 : get_test_server());
}

int GetNumCopies(TRptParams rpt_params)
{
  TTripInfo info;
  info.getByPointId(rpt_params.point_id);
  TQuery Qry( &OraSession );
  Qry.Clear();
  Qry.SQLText =
    "SELECT num, "
    "    DECODE(airline,NULL,0,8)+ "
    "    DECODE(flt_no,NULL,0,2)+ "
    "    DECODE(airp_dep,NULL,0,4) AS priority "
    "FROM doc_num_copies "
    "WHERE report_type = :report_type AND "
    "      (airline IS NULL OR airline=:airline) AND "
    "      (flt_no IS NULL OR flt_no=:flt_no) AND "
    "      (airp_dep IS NULL OR airp_dep=:airp_dep) "
    "ORDER BY priority DESC";
  Qry.CreateVariable("report_type",otString,EncodeRptType(rpt_params.orig_rpt_type));
  Qry.CreateVariable("airline",otString,info.airline);
  Qry.CreateVariable("flt_no",otInteger,info.flt_no);
  Qry.CreateVariable("airp_dep",otString,info.airp);
  Qry.Execute();
  return Qry.Eof ? NoExists : Qry.FieldAsInteger("num");
}

void  DocsInterface::RunReport2(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    xmlNodePtr node = reqNode->children;
    TRptParams rpt_params;
    rpt_params.Init(node);
    switch(rpt_params.rpt_type) {
        case rtPTM:
            REPORTS::PTM(rpt_params, reqNode, resNode);
            break;
        case rtPTMTXT:
            PTMBTMTXT(rpt_params, reqNode, resNode);
            break;
        case rtBTM:
            BTM(rpt_params, reqNode, resNode);
            break;
        case rtBTMTXT:
            PTMBTMTXT(rpt_params, reqNode, resNode);
            break;
        case rtWEB:
            WEB(rpt_params, reqNode, resNode);
            break;
        case rtWEBTXT:
            WEBTXT(rpt_params, reqNode, resNode);
            break;
        case rtREFUSE:
            REFUSE(rpt_params, reqNode, resNode);
            break;
        case rtREFUSETXT:
            REFUSETXT(rpt_params, reqNode, resNode);
            break;
        case rtNOTPRES:
            NOTPRES(rpt_params, reqNode, resNode);
            break;
        case rtNOTPRESTXT:
            NOTPRESTXT(rpt_params, reqNode, resNode);
            break;
        case rtSPEC:
        case rtREM:
            REM(rpt_params, reqNode, resNode);
            break;
        case rtSPECTXT:
        case rtREMTXT:
            REMTXT(rpt_params, reqNode, resNode);
            break;
        case rtCRS:
        case rtCRSUNREG:
        case rtBDOCS:
            CRS(rpt_params, reqNode, resNode);
            break;
        case rtCRSTXT:
        case rtCRSUNREGTXT:
            CRSTXT(rpt_params, reqNode, resNode);
            break;
        case rtEXAM:
        case rtNOREC:
        case rtGOSHO:
            EXAM(rpt_params, reqNode, resNode);
            break;
        case rtEXAMTXT:
        case rtNORECTXT:
        case rtGOSHOTXT:
            EXAMTXT(rpt_params, reqNode, resNode);
            break;
        case rtEMD:
            EMD(rpt_params, reqNode, resNode);
            break;
        case rtEMDTXT:
            EMDTXT(rpt_params, reqNode, resNode);
            break;
        case rtLOADSHEET:
        case rtNOTOC:
        case rtLIR:
            WB_MSG(rpt_params, reqNode, resNode);
            break;
        case rtANNUL_TAGS:
            ANNUL_TAGS(rpt_params, reqNode, resNode);
            break;
        case rtVOUCHERS:
            VOUCHERS(rpt_params, reqNode, resNode);
            break;
        case rtSERVICES:
            SERVICES(rpt_params, reqNode, resNode);
            break;
        case rtSERVICESTXT:
            SERVICESTXT(rpt_params, reqNode, resNode);
            break;
        case rtRESEAT:
            RESEAT(rpt_params, reqNode, resNode);
            break;
        case rtRESEATTXT:
            RESEATTXT(rpt_params, reqNode, resNode);
            break;
        case rtKOMPLEKT:
            KOMPLEKT(rpt_params, reqNode, resNode);
            break;
        default:
            throw AstraLocale::UserException("MSG.TEMPORARILY_NOT_SUPPORTED");
    }
    NewTextChild(resNode, "copies", GetNumCopies(rpt_params), NoExists);
}

void  DocsInterface::LogPrintEvent(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    int copies = NodeAsInteger("copies", reqNode, NoExists);
    int printed_copies = NodeAsInteger("printed_copies", reqNode, NoExists);
    if(printed_copies == NoExists) { // старая версия терминала не присылает тег printed_copies
        TReqInfo::Instance()->LocaleToLog("EVT.PRINT_REPORT", LEvntPrms()
                << PrmElem<std::string>("report", etReportType, NodeAsString("rpt_type", reqNode), efmtNameLong)
                << PrmSmpl<std::string>("copies", ""),
                ASTRA::evtPrn, NodeAsInteger("point_id", reqNode));
    } else {
        ostringstream str;
        str << getLocaleText("Напечатано копий") << ": " << printed_copies;
        if(copies != NoExists and copies != printed_copies)
            str << "; " << getLocaleText("Задано копий") << ": " << copies;
        TReqInfo::Instance()->LocaleToLog("EVT.PRINT_REPORT",
                LEvntPrms()
                << PrmElem<std::string>("report", etReportType, NodeAsString("rpt_type", reqNode), efmtNameLong)
                << PrmSmpl<std::string>("copies", str.str()),
                ASTRA::evtPrn, NodeAsInteger("point_id", reqNode));
    }
}

void  DocsInterface::LogExportEvent(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    TReqInfo::Instance()->LocaleToLog("EVT.EXPORT_REPORT", LEvntPrms()
                                       << PrmElem<std::string>("report", etReportType, NodeAsString("rpt_type", reqNode), efmtNameLong)
                                       << PrmSmpl<std::string>("fmt", NodeAsString("export_name", reqNode)),
                                       ASTRA::evtPrn, NodeAsInteger("point_id", reqNode));
}

void  DocsInterface::SaveReport(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    TQuery Qry(&OraSession);
    if(NodeIsNULL("name", reqNode))
        throw UserException("Form name can't be null");
    string name = NodeAsString("name", reqNode);
    string version = NodeAsString("version", reqNode, "");
    ProgTrace(TRACE5, "VER. %s", version.c_str());
    if(version == "")
        version = get_report_version(name);

    string form = NodeAsString("form", reqNode);
    Qry.SQLText = "update fr_forms2 set form = :form where name = :name and version = :version";
    Qry.CreateVariable("version", otString, version);
    Qry.CreateVariable("name", otString, name);
    Qry.CreateLongVariable("form", otLong, (void *)form.c_str(), form.size());
    Qry.Execute();
    if(!Qry.RowsProcessed())
      throw UserException("MSG.REPORT_UPDATE_FAILED.NOT_FOUND", LParams() << LParam("report_name", name));
    TReqInfo::Instance()->LocaleToLog("EVT.UPDATE_REPORT", LEvntPrms() << PrmSmpl<std::string>("name", name), evtSystem);
}

void DocsInterface::GetFonts(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    tst();
  NewTextChild(resNode,"fonts","");
  tst();
}

