#include "docs_emd.h"
#include "telegram.h"
#include "stat/stat_utils.h"
#include "docs_utils.h"

using namespace ASTRA;
using namespace std;
using namespace AstraLocale;

void EMD(TRptParams &rpt_params, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    if(rpt_params.rpt_type == rtEMDTXT)
        get_compatible_report_form("docTxt", reqNode, resNode);
    else
        get_compatible_report_form("EMD", reqNode, resNode);
    std::map<int, std::vector<std::string> > tab;
    size_t total = 0;
    EMDReport(rpt_params.point_id, tab, total);

    xmlNodePtr formDataNode = NewTextChild(resNode, "form_data");
    xmlNodePtr dataSetsNode = NewTextChild(formDataNode, "datasets");
    xmlNodePtr dataSetNode = NewTextChild(dataSetsNode, "v_emd");

    if(tab.size() != 0) {
        for( std::map<int, std::vector<std::string> >::iterator i = tab.begin(); i != tab.end(); i++) {
            xmlNodePtr rowNode = NewTextChild(dataSetNode, "row");
            NewTextChild(rowNode, "reg_no", i->first);
            const vector<string> &fields = i->second;
            for(size_t j = 0; j < fields.size(); j++) {
                switch(j) {
                    case 0:
                        NewTextChild(rowNode, "full_name", transliter(fields[j], 1, rpt_params.GetLang() != AstraLocale::LANG_RU));
                        break;
                    case 1:
                        NewTextChild(rowNode, "etkt_no", fields[j]);
                        break;
                    case 2:
                        NewTextChild(rowNode, "emd_no", fields[j]);
                        break;
                }
            }
        }
        xmlNodePtr rowNode = NewTextChild(dataSetNode, "row");
        NewTextChild(rowNode, "reg_no");
        NewTextChild(rowNode, "full_name", getLocaleText("Итого:", rpt_params.GetLang()));
        NewTextChild(rowNode, "etkt_no");
        NewTextChild(rowNode, "emd_no", (int)total);
    }

    // Теперь переменные отчета
    xmlNodePtr variablesNode = NewTextChild(formDataNode, "variables");
    PaxListVars(rpt_params.point_id, rpt_params, variablesNode);
    NewTextChild(variablesNode, "caption", getLocaleText("CAP.DOC.EMD",
                LParams() << LParam("flight", get_flight(variablesNode)), rpt_params.GetLang()));
    populate_doc_cap(variablesNode, rpt_params.GetLang());
}

void EMDTXT(TRptParams &rpt_params, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    EMD(rpt_params, reqNode, resNode);

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
        << setw(37) << getLocaleText("Ф.И.О.", rpt_params.GetLang())
        << setw(20)  << getLocaleText("№ Билета", rpt_params.GetLang())
        << setw(20)  << getLocaleText("№ EMD", rpt_params.GetLang());
    NewTextChild(variablesNode, "page_header_bottom", s.str() );
    NewTextChild(variablesNode, "page_footer_top",
            getLocaleText("CAP.ISSUE_DATE", LParams() << LParam("date", NodeAsString("date_issue",variablesNode)), rpt_params.GetLang()));
    xmlNodePtr dataSetNode = NodeAsNode("v_emd", dataSetsNode);
    xmlNodeSetName(dataSetNode, "table");
    vector<string> rows;
    map< string, vector<string> > fields;
    int row;
    xmlNodePtr rowNode=dataSetNode->children;
    const char col_sym = ' ';
    for(; rowNode != NULL; rowNode = rowNode->next)
    {
        SeparateString(NodeAsString("full_name", rowNode), 36, rows);
        fields["full_name"]=rows;

        SeparateString(NodeAsString("emd_no", rowNode), 19, rows);
        fields["emd_no"]=rows;

        row=0;
        s.str("");
        do
        {
            if (row != 0) s << endl;
            s
                << right << setw(3) << (row == 0 ? NodeAsString("reg_no", rowNode) : "") << col_sym
                << left << setw(36) << (!fields["full_name"].empty() ? *(fields["full_name"].begin()) : "") << col_sym
                << left <<  setw(19) << (row == 0 ? NodeAsString("etkt_no", rowNode) : "") << " " << col_sym
                << left << setw(19) << (!fields["emd_no"].empty() ? *(fields["emd_no"].begin()) : "");

            for(map< string, vector<string> >::iterator f = fields.begin(); f != fields.end(); f++)
                if (!f->second.empty()) f->second.erase(f->second.begin());
            row++;
        }
        while(
                !fields["surname"].empty() or
                !fields["emd_no"].empty()
                );
        NewTextChild(rowNode,"str",s.str());
    }
}

