#include "docs_exam.h"
#include "brd.h"
#include "baggage_calc.h"
#include "stat/stat_utils.h"
#include "docs_utils.h"
#include "xml_unit.h"
#include "baggage_ckin.h"
#include "crafts/SeatsPax.h"

#include "serverlib/xmllibcpp.h"

#define NICKNAME "DENIS"
#include "serverlib/slogger.h"

using namespace ASTRA;
using namespace AstraLocale;
using namespace std;
using namespace EXCEPTIONS;
namespace {

struct PaxData4Exam
{
  CheckIn::TSimplePaxItem pax;
  int grp_id;
  std::string user_descr;
  int bag_refuse;
  std::optional<int> opt_bag_pool_num;
  int excess_wt;
  int excess_pc;
};

bool compareBySeatNo(const PaxData4Exam& p1, const PaxData4Exam& p2)
{
  if (p1.pax.seat_no != p2.pax.seat_no) {
    return p1.pax.seat_no < p2.pax.seat_no;
  }
  if (p1.pax.reg_no != p2.pax.reg_no) {
    return p1.pax.reg_no < p2.pax.reg_no;
  }
  return p1.pax.seats > p2.pax.seats;
}

} // namespace

void EXAM(TRptParams &rpt_params, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    bool pr_web = (rpt_params.rpt_type == rtWEB or rpt_params.rpt_type == rtWEBTXT);
    bool pr_norec = (rpt_params.rpt_type == rtNOREC or rpt_params.rpt_type == rtNORECTXT);
    bool pr_gosho = (rpt_params.rpt_type == rtGOSHO or rpt_params.rpt_type == rtGOSHOTXT);
    if(
            rpt_params.rpt_type == rtEXAMTXT or
            rpt_params.rpt_type == rtWEBTXT or
            rpt_params.rpt_type == rtNORECTXT or
            rpt_params.rpt_type == rtGOSHOTXT
      )
        get_compatible_report_form("docTxt", reqNode, resNode);
    else
        get_compatible_report_form(pr_web ? "web" : "exam", reqNode, resNode);

    DB::TQuery Qry(PgOra::getROSession({"PAX","PAX_GRP","CRS_PAX","TCKIN_PAX_GRP","PAX_CALC_DATA","USERS2"}), STDLOG);
    BrdInterface::GetPaxQuery(Qry, rpt_params.point_id, NoExists, NoExists, rpt_params.rpt_type, rpt_params.client_type, rpt_params.sort);
    Qry.Execute();
    xmlNodePtr formDataNode = NewTextChild(resNode, "form_data");
    xmlNodePtr datasetsNode = NewTextChild(formDataNode, "datasets");
    xmlNodePtr passengersNode = NewTextChild(datasetsNode, "passengers");
    if(old_cbbg()) {
        TRemGrp rem_grp;
        if(!Qry.Eof)
        {
            rem_grp.Load(retBRD_VIEW, rpt_params.point_id);

            bool check_pay_on_tckin_segs=false;
            TTripInfo fltInfo;
            if (fltInfo.getByPointId(rpt_params.point_id))
                check_pay_on_tckin_segs=GetTripSets(tsCheckPayOnTCkinSegs, fltInfo);

            TComplexBagExcessNodeList excessNodeList(OutputLang(rpt_params.GetLang()), {}, "+");
            using namespace CKIN;
            BagReader bag_reader(PointId_t(rpt_params.point_id), std::nullopt, READ::BAGS_AND_TAGS);
            MainPax viewPax;

            std::vector<PaxData4Exam> paxDataItems;
            SEATSPAX::TSeatPaxListCached seatPaxList;
            for( ; !Qry.Eof; Qry.Next()) {
                CheckIn::TSimplePaxItem pax;
                pax.fromDB(Qry);
                pax.seat_no = seatPaxList.get_seat_no( PaxId_t(Qry.FieldAsInteger("pax_id")),
                                                       Qry.FieldAsInteger("seats"),
                                                       Qry.FieldAsInteger("is_jmp"),
                                                       DecodePaxStatus(Qry.FieldAsString("status")),
                                                       PointId_t(Qry.FieldAsInteger("point_dep")),
                                                       SEATSPAX::TSeatPaxListCached::ef_Seats);

                int grp_id = Qry.FieldAsInteger("grp_id");
                const int bag_refuse = Qry.FieldAsInteger("bag_refuse");
                const std::string user_descr = Qry.FieldAsString("user_descr");
                std::optional<int> opt_bag_pool_num;
                if(!Qry.FieldIsNULL("bag_pool_num")) {
                    opt_bag_pool_num = Qry.FieldAsInteger("bag_pool_num");
                }
                const int excess_wt = Qry.FieldAsInteger("excess_wt");
                const int excess_pc = Qry.FieldAsInteger("excess_pc");
                const PaxData4Exam pax_data = { pax, grp_id, user_descr, bag_refuse, opt_bag_pool_num, excess_wt, excess_pc };
                paxDataItems.push_back(pax_data);
            }
            if (rpt_params.sort == stSeatNo) {
              std::sort(paxDataItems.begin(), paxDataItems.end(), compareBySeatNo);
            }
            for (const PaxData4Exam& pax_data: paxDataItems) {
              const CheckIn::TSimplePaxItem& pax = pax_data.pax;
              int grp_id = pax_data.grp_id;
              const int bag_refuse = pax_data.bag_refuse;
              std::optional<int> opt_bag_pool_num = pax_data.opt_bag_pool_num;
              const int excess_wt_raw = pax_data.excess_wt;
              const int excess_pc = pax_data.excess_pc;

              xmlNodePtr paxNode = NewTextChild(passengersNode, "pax");
              NewTextChild(paxNode, "reg_no", pax.reg_no);
              NewTextChild(paxNode, "surname", transliter(pax.surname, TranslitFormat::V1, rpt_params.GetLang() != AstraLocale::LANG_RU));
              NewTextChild(paxNode, "name", transliter(pax.name, TranslitFormat::V1, rpt_params.GetLang() != AstraLocale::LANG_RU));
              if(pr_web)
                  NewTextChild(paxNode, "user_descr", transliter(pax_data.user_descr, TranslitFormat::V1, rpt_params.GetLang() != AstraLocale::LANG_RU));
              NewTextChild(paxNode, "pers_type", rpt_params.ElemIdToReportElem(etPersType, EncodePerson(pax.pers_type), efmtCodeNative));
              NewTextChild(paxNode, "pr_exam", (int)pax.pr_exam, (int)false);
              NewTextChild(paxNode, "pr_brd", (int)pax.pr_brd, (int)false);
              NewTextChild(paxNode, "seat_no", pax.seat_no);
              NewTextChild(paxNode, "document", CheckIn::GetPaxDocStr(std::nullopt, pax.id, false, rpt_params.GetLang()));
              NewTextChild(paxNode, "ticket_no", pax.tkn.no);
              NewTextChild(paxNode, "coupon_no", pax.tkn.coupon);

              if(opt_bag_pool_num) {
                  viewPax.saveMainPax(GrpId_t(grp_id), PaxId_t(pax.paxId()), bag_refuse!=0);
              }

              NewTextChild(paxNode, "bag_amount", bag_reader.bagAmount(GrpId_t(grp_id), opt_bag_pool_num));
              NewTextChild(paxNode, "bag_weight", bag_reader.bagWeight(GrpId_t(grp_id), opt_bag_pool_num));
              NewTextChild(paxNode, "rk_amount", bag_reader.rkAmount(GrpId_t(grp_id), opt_bag_pool_num));
              NewTextChild(paxNode, "rk_weight", bag_reader.rkWeight(GrpId_t(grp_id), opt_bag_pool_num));
              excessNodeList.add(paxNode, "excess", TBagPieces(excess_pc),
                      TBagKilos(viewPax.excessWt(GrpId_t(grp_id), PaxId_t(pax.paxId()), excess_wt_raw)));
              bool pr_payment=RFISCPaymentCompleted(grp_id, pax.id, check_pay_on_tckin_segs) &&
                  WeightConcept::BagPaymentCompleted(grp_id);
              NewTextChild(paxNode, "pr_payment", (int)pr_payment);
              NewTextChild(paxNode, "tags", bag_reader.tags(GrpId_t(grp_id), opt_bag_pool_num, rpt_params.GetLang()));
              NewTextChild(paxNode, "remarks", GetRemarkStr(rem_grp, pax.id, rpt_params.GetLang()));
            }
        }
    } else {
        if(!Qry.Eof)
        {
            bool check_pay_on_tckin_segs=false;
            TTripInfo fltInfo;
            if (fltInfo.getByPointId(rpt_params.point_id))
                check_pay_on_tckin_segs=GetTripSets(tsCheckPayOnTCkinSegs, fltInfo);
            TComplexBagExcessNodeList excessNodeList(OutputLang(rpt_params.GetLang()), {}, "+");

            REPORTS::TPaxList pax_list(rpt_params.point_id);
            pax_list.options.rem_event_type = retBRD_VIEW;
            pax_list.options.lang = rpt_params.GetLang();
            pax_list.options.flags.setFlag(REPORTS::oeSeatNo);
            pax_list.options.flags.setFlag(REPORTS::oeTags);

            pax_list.fromDB(Qry, true /*calcExcessPC*/);

            SEATSPAX::TSeatPaxListCached seatPaxList;
            for(const auto &pax: pax_list) {
                std::string seat_no = seatPaxList.get_seat_no( PaxId_t(Qry.FieldAsInteger("pax_id")),
                                                       Qry.FieldAsInteger("seats"),
                                                       Qry.FieldAsInteger("is_jmp"),
                                                       DecodePaxStatus(Qry.FieldAsString("status")),
                                                       PointId_t(Qry.FieldAsInteger("point_dep")),
                                                       SEATSPAX::TSeatPaxListCached::ef_Seats);
                if(pax_list.options.flags.isFlag(REPORTS::oeSeatNo) && Qry.FieldAsInteger("seats") > 1) {
                  seat_no += "+" + std::to_string(Qry.FieldAsInteger("seats") - 1);
                }
                xmlNodePtr paxNode = NewTextChild(passengersNode, "pax");
                NewTextChild(paxNode, "reg_no", pax->simple.reg_no);
                NewTextChild(paxNode, "surname", transliter(pax->simple.surname, TranslitFormat::V1, rpt_params.GetLang() != AstraLocale::LANG_RU));
                NewTextChild(paxNode, "name", transliter(pax->simple.name, TranslitFormat::V1, rpt_params.GetLang() != AstraLocale::LANG_RU));
                if(pr_web)
                    NewTextChild(paxNode, "user_descr", transliter(pax->user_descr, TranslitFormat::V1, rpt_params.GetLang() != AstraLocale::LANG_RU));
                NewTextChild(paxNode, "pers_type", rpt_params.ElemIdToReportElem(etPersType, EncodePerson(pax->simple.pers_type), efmtCodeNative));
                NewTextChild(paxNode, "pr_exam", (int)pax->simple.pr_exam, (int)false);
                NewTextChild(paxNode, "pr_brd", (int)pax->simple.pr_brd, (int)false);
                NewTextChild(paxNode, "seat_no", seat_no);
                NewTextChild(paxNode, "document", CheckIn::GetPaxDocStr(std::nullopt, pax->simple.id, false, rpt_params.GetLang()));
                NewTextChild(paxNode, "ticket_no", pax->tkn_str());
                NewTextChild(paxNode, "bag_amount", pax->bag_amount());
                NewTextChild(paxNode, "bag_weight", pax->bag_weight());
                NewTextChild(paxNode, "rk_amount", pax->rk_amount());
                NewTextChild(paxNode, "rk_weight", pax->rk_weight());
                NewTextChild(paxNode, "excess", TComplexBagExcess(pax->baggage.excess_wt, pax->baggage.excess_pc).
                        view(OutputLang(rpt_params.GetLang()), true));
                bool pr_payment=RFISCPaymentCompleted(pax->simple.grp_id, pax->simple.id, check_pay_on_tckin_segs) &&
                    WeightConcept::BagPaymentCompleted(pax->simple.grp_id);
                NewTextChild(paxNode, "pr_payment", (int)pr_payment);
                NewTextChild(paxNode, "tags", pax->get_tags());
                NewTextChild(paxNode, "remarks", pax->rems());
            }
        }
    }

    // ������ ��६���� ����
    xmlNodePtr variablesNode = NewTextChild(formDataNode, "variables");
    BrdInterface::readTripCounters(rpt_params.point_id, rpt_params, variablesNode, rpt_params.rpt_type, rpt_params.client_type);
    PaxListVars(rpt_params.point_id, rpt_params, variablesNode);
    if ( pr_web) {
        if(!rpt_params.client_type.empty()) {
            string ls_type;
            switch(DecodeClientType(rpt_params.client_type.c_str())) {
                case ctWeb:
                    ls_type = getLocaleText("CAP.PAX_LIST.WEB", rpt_params.GetLang());
                    break;
                case ctMobile:
                    ls_type = getLocaleText("CAP.PAX_LIST.MOBILE", rpt_params.GetLang());
                    break;
                case ctKiosk:
                    ls_type = getLocaleText("CAP.PAX_LIST.KIOSK", rpt_params.GetLang());
                    break;
                default:
                    throw Exception("Unexpected client_type: %s", rpt_params.client_type.c_str());
            }
            NewTextChild(variablesNode, "paxlist_type", ls_type);
        }
    } else if(pr_norec)
        NewTextChild(variablesNode, "paxlist_type", "NOREC");
    else if(pr_gosho)
        NewTextChild(variablesNode, "paxlist_type", "GOSHO");
    else
        NewTextChild(variablesNode, "paxlist_type", getLocaleText("CAP.PAX_LIST.BRD", rpt_params.GetLang()));
    if(pr_web and rpt_params.client_type.empty())
        NewTextChild(variablesNode, "caption", getLocaleText("CAP.DOC.PAX_LIST.SELF_CKIN",
                    LParams()
                    << LParam("flight", get_flight(variablesNode)), rpt_params.GetLang())
                );
    else
        NewTextChild(variablesNode, "caption", getLocaleText("CAP.DOC.PAX_LIST",
                    LParams()
                    << LParam("list_type", NodeAsString("paxlist_type", variablesNode))
                    << LParam("flight", get_flight(variablesNode)), rpt_params.GetLang())
                );
    xmlNodePtr currNode = variablesNode->children;
    xmlNodePtr totalNode = NodeAsNodeFast("total", currNode);
    NodeSetContent(totalNode, getLocaleText("CAP.TOTAL.VAL", LParams() << LParam("total", NodeAsString(totalNode)), rpt_params.GetLang()));
    populate_doc_cap(variablesNode, rpt_params.GetLang());
    NewTextChild(variablesNode, "kg", getLocaleText("��", rpt_params.GetLang()));
    NewTextChild(variablesNode, "pc", getLocaleText("�", rpt_params.GetLang()));
}

void EXAMTXT(TRptParams &rpt_params, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    EXAM(rpt_params, reqNode, resNode);
    const char col_sym = ' '; //ᨬ��� ࠧ����⥫� �⮫�殢
    bool pr_web = rpt_params.rpt_type == rtWEBTXT;

    xmlNodePtr variablesNode=NodeAsNode("form_data/variables",resNode);
    xmlNodePtr dataSetsNode=NodeAsNode("form_data/datasets",resNode);
    int page_width=80;
    int max_symb_count=rpt_params.IsInter()?page_width:60;
    NewTextChild(variablesNode, "page_width", page_width);
    NewTextChild(variablesNode, "test_server", STAT::bad_client_img_version() ? 2 : get_test_server());
    if(STAT::bad_client_img_version())
        NewTextChild(variablesNode, "doc_cap_test", " ");
    string str;
    ostringstream s;
    s.str("");
    NewTextChild(variablesNode, "test_str", get_test_str(page_width, rpt_params.GetLang()));
    s.str("");
    s << NodeAsString("caption", variablesNode);
    str = s.str().substr(0, max_symb_count);
    s.str("");
    s << right << setw(((page_width - str.size()) / 2) + str.size()) << str;
    NewTextChild(variablesNode, "page_header_top", s.str());
    s.str("");
    s
        << right << setw(3)  << getLocaleText("�", rpt_params.GetLang()) << col_sym
        << left << setw(pr_web ? 19 : 30) << getLocaleText("�������", rpt_params.GetLang());
    if(pr_web)
        s
            << setw(9)  << getLocaleText("������", rpt_params.GetLang());
    s << setw(4)  << getLocaleText("���", rpt_params.GetLang());
    if(pr_web)
        s
            << setw(9)  << getLocaleText("� �", rpt_params.GetLang());
    else
        s
            << setw(3)  << getLocaleText("��", rpt_params.GetLang())
            << setw(4)  << getLocaleText("��", rpt_params.GetLang());
    s
        << setw(11) << getLocaleText("���㬥��", rpt_params.GetLang())
        << setw(14) << getLocaleText("�����", rpt_params.GetLang())
        << setw(10) << getLocaleText("� �/�", rpt_params.GetLang());
    NewTextChild(variablesNode, "page_header_bottom", s.str() );
    s.str("");
    s
        << NodeAsString("total", variablesNode) << endl
        << getLocaleText("CAP.ISSUE_DATE", LParams() << LParam("date", NodeAsString("date_issue",variablesNode)), rpt_params.GetLang());
    NewTextChild(variablesNode, "page_footer_top", s.str() );

    xmlNodePtr dataSetNode = NodeAsNode("passengers", dataSetsNode);
    xmlNodeSetName(dataSetNode, "table");
    vector<string> rows;
    map< string, vector<string> > fields;
    int row;
    xmlNodePtr rowNode=dataSetNode->children;
    for(; rowNode != NULL; rowNode = rowNode->next)
    {
        //ࠡ����� 䠬����, ���㬥��, �����, ��ન, ६�ન
        SeparateString(((string)NodeAsString("surname",rowNode) + " " + NodeAsString("name", rowNode, "")).c_str(),(pr_web ? 18 : 29),rows);
        fields["surname"]=rows;

        SeparateString(NodeAsString("document",rowNode, ""),10,rows);
        fields["docs"]=rows;

        SeparateString(NodeAsString("ticket_no",rowNode, ""),13,rows);
        fields["tkts"]=rows;

        SeparateString(NodeAsString("tags",rowNode, ""),10,rows);
        fields["tags"]=rows;

        SeparateString(NodeAsString("user_descr",rowNode, ""),8,rows);
        fields["user_descr"]=rows;

        row=0;
        s.str("");
        do
        {
            if (row != 0) s << endl;
            s
                << right << setw(3) << (row == 0 ? NodeAsString("reg_no", rowNode) : "") << col_sym
                << left << setw(pr_web ? 18 : 29) << (!fields["surname"].empty() ? *(fields["surname"].begin()) : "") << col_sym;
            if(pr_web)
                s
                    << left << setw(8) << (!fields["user_descr"].empty() ? *(fields["user_descr"].begin()) : "") << col_sym;
            s
                << right <<  setw(3) << (row == 0 ? NodeAsString("pers_type", rowNode) : "") << col_sym;
            if(pr_web) {
                s
                    << left <<  setw(8) << (row == 0 ? NodeAsString("seat_no", rowNode, "") : "") << col_sym;
            } else {
                s
                    << left <<  setw(3) << (row == 0 ? (strcmp(NodeAsString("pr_exam", rowNode, ""), "") == 0 ? "-" : "+") : "")
                    << left <<  setw(4) << (row == 0 ? (strcmp(NodeAsString("pr_brd", rowNode, ""), "") == 0 ? "-" : "+") : "");
            }
            s
                << left << setw(10) << (!fields["docs"].empty() ? *(fields["docs"].begin()) : "") << col_sym
                << left << setw(13) << (!fields["tkts"].empty() ? *(fields["tkts"].begin()) : "") << col_sym
                << left << setw(10) << (!fields["tags"].empty() ? *(fields["tags"].begin()) : "");
            for(map< string, vector<string> >::iterator f = fields.begin(); f != fields.end(); f++)
                if (!f->second.empty()) f->second.erase(f->second.begin());
            row++;
        }
        while(
                !fields["surname"].empty() ||
                !fields["docs"].empty() ||
                !fields["tkts"].empty() ||
                !fields["tags"].empty() ||
                !fields["user_descr"].empty()
             );
        NewTextChild(rowNode,"str",s.str());
    }
}

void WEB(TRptParams &rpt_params, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    NewTextChild(reqNode, "client_type", rpt_params.client_type);
    EXAM(rpt_params, reqNode, resNode);
}

void WEBTXT(TRptParams &rpt_params, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    NewTextChild(reqNode, "client_type", rpt_params.client_type);
    EXAMTXT(rpt_params, reqNode, resNode);
}

