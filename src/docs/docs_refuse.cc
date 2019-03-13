#include "docs_refuse.h"
#include "stat/stat_utils.h"
#include "docs_utils.h"
#include "serverlib/xmllibcpp.h"
#include "docs_pax_list.h"

#define NICKNAME "DENIS"
#include "serverlib/slogger.h"

using namespace ASTRA;
using namespace std;
using namespace AstraLocale;

void REFUSE(TRptParams &rpt_params, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    if(rpt_params.rpt_type == rtREFUSETXT)
        get_compatible_report_form("docTxt", reqNode, resNode);
    else
        get_compatible_report_form("ref", reqNode, resNode);

    xmlNodePtr formDataNode = NewTextChild(resNode, "form_data");
    xmlNodePtr dataSetsNode = NewTextChild(formDataNode, "datasets");
    xmlNodePtr dataSetNode = NewTextChild(dataSetsNode, "v_ref");

    if(old_cbbg()) {
        TQuery Qry(&OraSession);
        string SQLText =
            "SELECT point_dep AS point_id, "
            "       reg_no, "
            "       surname||' '||pax.name family, "
            "       pax.pers_type, "
            "       ticket_no, "
            "       refusal_types.code refuse, "
            "       ckin.get_birks2(pax.grp_id,pax.pax_id,pax.bag_pool_num,:lang) AS tags "
            "FROM   pax_grp,pax,refusal_types "
            "WHERE  pax_grp.grp_id=pax.grp_id AND "
            "       pax.refuse = refusal_types.code AND "
            "       pax_grp.status NOT IN ('E') AND "
            "       pax.refuse IS NOT NULL and "
            "       point_dep = :point_id "
            "order by ";
        switch(rpt_params.sort) {
            case stServiceCode:
            case stSeatNo:
            case stRegNo:
                SQLText += " pax.reg_no, pax.seats DESC ";
                break;
            case stSurname:
                SQLText += " family, pax.reg_no, pax.seats DESC ";
                break;
        }
        Qry.SQLText = SQLText;
        Qry.CreateVariable("point_id", otInteger, rpt_params.point_id);
        Qry.CreateVariable("lang", otString, rpt_params.GetLang());
        Qry.Execute();
        while(!Qry.Eof) {
            xmlNodePtr rowNode = NewTextChild(dataSetNode, "row");

            NewTextChild(rowNode, "point_id", Qry.FieldAsInteger("point_id"));
            NewTextChild(rowNode, "reg_no", Qry.FieldAsInteger("reg_no"));
            NewTextChild(rowNode, "family", transliter(Qry.FieldAsString("family"), 1, rpt_params.GetLang() != AstraLocale::LANG_RU));
            NewTextChild(rowNode, "pers_type", rpt_params.ElemIdToReportElem(etPersType, Qry.FieldAsString("pers_type"), efmtCodeNative));
            NewTextChild(rowNode, "ticket_no", Qry.FieldAsString("ticket_no"));
            NewTextChild(rowNode, "refuse", rpt_params.ElemIdToReportElem(etRefusalType, Qry.FieldAsString("refuse"), efmtNameLong));
            NewTextChild(rowNode, "tags", Qry.FieldAsString("tags"));

            Qry.Next();
        }
    } else {
        REPORTS::TPaxList pax_list(rpt_params.point_id);

        pax_list.options.lang = rpt_params.GetLang();
        pax_list.options.sort = rpt_params.sort;
        pax_list.options.flags.setFlag(REPORTS::oeTags);

        pax_list.fromDB();
        pax_list.sort(REPORTS::pax_cmp);
        for(const auto &pax: pax_list) {
            if(pax->simple.refuse.empty()) continue;

            xmlNodePtr rowNode = NewTextChild(dataSetNode, "row");

            NewTextChild(rowNode, "reg_no", pax->simple.reg_no);
            NewTextChild(rowNode, "family", pax->full_name_view());
            NewTextChild(rowNode, "pers_type", rpt_params.ElemIdToReportElem(etPersType, EncodePerson(pax->simple.pers_type), efmtCodeNative));
            NewTextChild(rowNode, "ticket_no", pax->tkn_str());
            NewTextChild(rowNode, "refuse", rpt_params.ElemIdToReportElem(etRefusalType, pax->simple.refuse, efmtNameLong));
            NewTextChild(rowNode, "tags", pax->get_tags());
        }
    }

    // Теперь переменные отчета
    xmlNodePtr variablesNode = NewTextChild(formDataNode, "variables");
    PaxListVars(rpt_params.point_id, rpt_params, variablesNode);
    NewTextChild(variablesNode, "caption", getLocaleText("CAP.DOC.REFUSE", LParams() << LParam("flight", get_flight(variablesNode)), rpt_params.GetLang())); //!!!param 100%error
    populate_doc_cap(variablesNode, rpt_params.GetLang());
}

void REFUSETXT(TRptParams &rpt_params, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    REFUSE(rpt_params, reqNode, resNode);

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
        << right << setw(3)  << getLocaleText("№", rpt_params.GetLang()) << " "
        << left
        << setw(21) << getLocaleText("Ф.И.О.", rpt_params.GetLang())
        << setw(5)  << getLocaleText("Тип", rpt_params.GetLang())
        << setw(10) << getLocaleText("№ Билета", rpt_params.GetLang())
        << setw(24)  << getLocaleText("Причина невылета", rpt_params.GetLang())
        << setw(16) << getLocaleText("№ б/б", rpt_params.GetLang());
    NewTextChild(variablesNode, "page_header_bottom", s.str() );
    NewTextChild(variablesNode, "page_footer_top",
            getLocaleText("CAP.ISSUE_DATE", LParams() << LParam("date", NodeAsString("date_issue",variablesNode)), rpt_params.GetLang()));
    xmlNodePtr dataSetNode = NodeAsNode("v_ref", dataSetsNode);
    xmlNodeSetName(dataSetNode, "table");
    vector<string> rows;
    map< string, vector<string> > fields;
    int row;
    xmlNodePtr rowNode=dataSetNode->children;
    const char col_sym = ' ';
    for(; rowNode != NULL; rowNode = rowNode->next)
    {
        SeparateString(NodeAsString("family", rowNode), 20, rows);
        fields["surname"]=rows;

        SeparateString(NodeAsString("ticket_no", rowNode), 9, rows);
        fields["tkts"]=rows;

        SeparateString(NodeAsString("refuse", rowNode), 23, rows);
        fields["refuse"]=rows;

        SeparateString(NodeAsString("tags", rowNode), 16, rows);
        fields["tags"]=rows;

        row=0;
        s.str("");
        do
        {
            if (row != 0) s << endl;
            s
                << right << setw(3) << (row == 0 ? NodeAsString("reg_no", rowNode) : "") << col_sym
                << left << setw(20) << (!fields["surname"].empty() ? *(fields["surname"].begin()) : "") << col_sym
                << right <<  setw(3) << (row == 0 ? NodeAsString("pers_type", rowNode) : "") << " " << col_sym
                << left << setw(9) << (!fields["tkts"].empty() ? *(fields["tkts"].begin()) : "") << col_sym
                << left << setw(23) << (!fields["refuse"].empty() ? *(fields["refuse"].begin()) : "") << col_sym
                << left << setw(16) << (!fields["tags"].empty() ? *(fields["tags"].begin()) : "");
            for(map< string, vector<string> >::iterator f = fields.begin(); f != fields.end(); f++)
                if (!f->second.empty()) f->second.erase(f->second.begin());
            row++;
        }
        while(
                !fields["surname"].empty() ||
                !fields["tkts"].empty() ||
                !fields["refuse"].empty() ||
                !fields["tags"].empty()
             );
        NewTextChild(rowNode,"str",s.str());
    }
}

