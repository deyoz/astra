#include "docs_notpres.h"
#include "stat/stat_utils.h"
#include "docs_utils.h"
#include "serverlib/xmllibcpp.h"
#include "docs_pax_list.h"

#define NICKNAME "DENIS"
#include "serverlib/slogger.h"

using namespace ASTRA;
using namespace std;
using namespace AstraLocale;

void NOTPRES(TRptParams &rpt_params, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    if(rpt_params.rpt_type == rtNOTPRESTXT)
        get_compatible_report_form("docTxt", reqNode, resNode);
    else
        get_compatible_report_form("notpres", reqNode, resNode);

    xmlNodePtr formDataNode = NewTextChild(resNode, "form_data");
    xmlNodePtr dataSetsNode = NewTextChild(formDataNode, "datasets");
    xmlNodePtr dataSetNode = NewTextChild(dataSetsNode, "v_notpres");

    if(old_cbbg()) {
        DB::TQuery Qry(PgOra::getROSession({"PAX_GRP","PAX"}), STDLOG); // salons.get_seat_no, ckin.get_bagAmount2, ckin.get_birks2
        string SQLText =
            "SELECT point_dep AS point_id, "
            "       pax.*, "
            "       salons.get_seat_no(pax.pax_id,pax.seats,pax.is_jmp,pax_grp.status,pax_grp.point_dep,'_seats',rownum,:pr_lat) AS seat_no, "
            "       ckin.get_bagAmount2(pax.grp_id,pax.pax_id,pax.bag_pool_num,rownum) AS bagAmount, "
            "       ckin.get_birks2(pax.grp_id,pax.pax_id,pax.bag_pool_num,:lang) AS tags "
            "FROM   pax_grp,pax "
            "WHERE  pax_grp.grp_id=pax.grp_id AND "
            "       pax.pr_brd=0 and "
            "       point_dep = :point_id and "
            "       pax_grp.status NOT IN ('E') "
            "order by ";
        switch(rpt_params.sort) {
            case stServiceCode:
            case stRegNo:
                SQLText += " pax.reg_no, pax.seats DESC ";
                break;
            case stSurname:
                SQLText += " pax.surname, pax.name, pax.reg_no, pax.seats DESC ";
                break;
            case stSeatNo:
                SQLText += " seat_no, pax.reg_no, pax.seats DESC ";
                break;
        }
        Qry.SQLText = SQLText;
        Qry.CreateVariable("point_id", otInteger, rpt_params.point_id);
        Qry.CreateVariable("lang", otString, rpt_params.GetLang());
        Qry.CreateVariable("pr_lat", otInteger, rpt_params.IsInter());
        Qry.Execute();
        while(!Qry.Eof) {
            CheckIn::TSimplePaxItem pax;
            pax.fromDB(Qry);
            xmlNodePtr rowNode = NewTextChild(dataSetNode, "row");

            LogTrace(TRACE5) << "old seat_no: '" << pax.seat_no << "'";

            NewTextChild(rowNode, "point_id", Qry.FieldAsInteger("point_id"));
            NewTextChild(rowNode, "reg_no", pax.reg_no);
            NewTextChild(rowNode, "family", transliter(pax.full_name(), 1, rpt_params.GetLang() != AstraLocale::LANG_RU));
            NewTextChild(rowNode, "pers_type", rpt_params.ElemIdToReportElem(etPersType, EncodePerson(pax.pers_type), efmtCodeNative));
            NewTextChild(rowNode, "seat_no", pax.seat_no);
            NewTextChild(rowNode, "bagamount", Qry.FieldAsInteger("bagamount"));
            NewTextChild(rowNode, "tags", Qry.FieldAsString("tags"));

            Qry.Next();
        }
    } else {
        REPORTS::TPaxList pax_list(rpt_params.point_id);

        pax_list.options.lang = rpt_params.GetLang();
        pax_list.options.sort = rpt_params.sort;
        pax_list.options.pr_brd = REPORTS::TBrdVal::bvFALSE;
        pax_list.options.flags.setFlag(REPORTS::oeBagAmount);
        pax_list.options.flags.setFlag(REPORTS::oeSeatNo);
        pax_list.options.flags.setFlag(REPORTS::oeTags);

        pax_list.fromDB();
        pax_list.sort(REPORTS::pax_cmp);
        for(const auto &pax: pax_list) {
            xmlNodePtr rowNode = NewTextChild(dataSetNode, "row");

            NewTextChild(rowNode, "reg_no", pax->simple.reg_no);
            NewTextChild(rowNode, "family", pax->full_name_view());
            NewTextChild(rowNode, "pers_type", rpt_params.ElemIdToReportElem(etPersType, EncodePerson(pax->simple.pers_type), efmtCodeNative));
            NewTextChild(rowNode, "seat_no", pax->seat_no());
            NewTextChild(rowNode, "bagamount", pax->bag_amount());
            NewTextChild(rowNode, "tags", pax->get_tags());
        }
    }

    // ������ ��६���� ����
    xmlNodePtr variablesNode = NewTextChild(formDataNode, "variables");
    PaxListVars(rpt_params.point_id, rpt_params, variablesNode);
    NewTextChild(variablesNode, "caption", getLocaleText("CAP.DOC.NOTPRES",
                LParams() << LParam("flight", get_flight(variablesNode)), rpt_params.GetLang()));
    populate_doc_cap(variablesNode, rpt_params.GetLang());
}

void NOTPRESTXT(TRptParams &rpt_params, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    NOTPRES(rpt_params, reqNode, resNode);

    xmlNodePtr variablesNode=NodeAsNode("form_data/variables",resNode);
    xmlNodePtr dataSetsNode=NodeAsNode("form_data/datasets",resNode);
    int page_width=80;
    int max_symb_count=rpt_params.IsInter()?page_width:60;
    NewTextChild(variablesNode, "page_width", page_width);
    NewTextChild(variablesNode, "test_server", STAT::bad_client_img_version() ? 2 : get_test_server());
    if(STAT::bad_client_img_version())
        NewTextChild(variablesNode, "doc_cap_test", " ");
    NewTextChild(variablesNode, "test_str", get_test_str(page_width, rpt_params.GetLang()));
    ostringstream s;
    s.str("");
    s << NodeAsString("caption", variablesNode);
    string str = s.str().substr(0, max_symb_count);
    s.str("");
    s << right << setw(((page_width - str.size()) / 2) + str.size()) << str;
    NewTextChild(variablesNode, "page_header_top", s.str());
    s.str("");
    s
        << right << setw(3)  << getLocaleText("�", rpt_params.GetLang()) << " "
        << left
        << setw(38) << getLocaleText("�.�.�.", rpt_params.GetLang())
        << setw(5)  << getLocaleText("���", rpt_params.GetLang())
        << setw(8)  << getLocaleText("� �", rpt_params.GetLang())
        << setw(6) << getLocaleText("���.", rpt_params.GetLang())
        << " " << setw(19) << getLocaleText("� �/�", rpt_params.GetLang());
    NewTextChild(variablesNode, "page_header_bottom", s.str() );
    NewTextChild(variablesNode, "page_footer_top",
            getLocaleText("CAP.ISSUE_DATE", LParams() << LParam("date", NodeAsString("date_issue",variablesNode)), rpt_params.GetLang()));
    xmlNodePtr dataSetNode = NodeAsNode("v_notpres", dataSetsNode);
    xmlNodeSetName(dataSetNode, "table");
    vector<string> rows;
    map< string, vector<string> > fields;
    int row;
    xmlNodePtr rowNode=dataSetNode->children;
    const char col_sym = ' ';
    for(; rowNode != NULL; rowNode = rowNode->next)
    {
        SeparateString(NodeAsString("family", rowNode), 37, rows);
        fields["surname"]=rows;

        SeparateString(NodeAsString("tags", rowNode), 19, rows);
        fields["tags"]=rows;

        row=0;
        s.str("");
        do
        {
            string bagamount = NodeAsString("bagamount", rowNode, "");
            if(bagamount == "0") bagamount.erase();
            if (row != 0) s << endl;
            s
                << right << setw(3) << (row == 0 ? NodeAsString("reg_no", rowNode) : "") << col_sym
                << left << setw(37) << (!fields["surname"].empty() ? *(fields["surname"].begin()) : "") << col_sym
                << right <<  setw(3) << (row == 0 ? NodeAsString("pers_type", rowNode, "��") : "") << " " << col_sym
                << left <<  setw(7) << (row == 0 ? NodeAsString("seat_no", rowNode, "") : "") << col_sym
                << left <<  setw(5) << (row == 0 ? bagamount : "") << col_sym
                << left << setw(19) << (!fields["tags"].empty() ? *(fields["tags"].begin()) : "");
            for(map< string, vector<string> >::iterator f = fields.begin(); f != fields.end(); f++)
                if (!f->second.empty()) f->second.erase(f->second.begin());
            row++;
        }
        while(
                !fields["surname"].empty() ||
                !fields["tags"].empty()
             );
        NewTextChild(rowNode,"str",s.str());
    }
}

