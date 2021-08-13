#include "docs_annul_tags.h"
#include "stat/stat_annul_bt.h"
#include "stat/stat_utils.h"
#include "docs_utils.h"
#include "docs_text_grid.h"

#define NICKNAME "DENIS"
#include "serverlib/slogger.h"

using namespace ASTRA;
using namespace AstraLocale;
using namespace std;

void ANNUL(TRptParams &rpt_params, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    if(rpt_params.rpt_type == rtANNULTXT)
        get_compatible_report_form("docTxt", reqNode, resNode);
    else
        get_compatible_report_form("annul_tags", reqNode, resNode);

    xmlNodePtr formDataNode = NewTextChild(resNode, "form_data");
    xmlNodePtr dataSetsNode = NewTextChild(formDataNode, "datasets");
    xmlNodePtr dataSetNode = NewTextChild(dataSetsNode, "v_annul");
    // ��६���� �����
    xmlNodePtr variablesNode = NewTextChild(formDataNode, "variables");
    PaxListVars(rpt_params.point_id, rpt_params, variablesNode);
    // ��������� �����
    NewTextChild(variablesNode, "caption",
            getLocaleText("CAP.DOC.ANNUL_TAGS", LParams() << LParam("flight", get_flight(variablesNode)), rpt_params.GetLang()));
    populate_doc_cap(variablesNode, rpt_params.GetLang());

    NewTextChild(variablesNode, "doc_cap_annul_reg_no", getLocaleText("�", rpt_params.GetLang()));
    NewTextChild(variablesNode, "doc_cap_annul_fio", getLocaleText("�.�.�.", rpt_params.GetLang()));
    NewTextChild(variablesNode, "doc_cap_annul_no", getLocaleText("�� ���. ��ப", rpt_params.GetLang()));
    NewTextChild(variablesNode, "doc_cap_annul_weight", getLocaleText("�� ���", rpt_params.GetLang()));
    NewTextChild(variablesNode, "doc_cap_annul_bag_type", getLocaleText("��� ������/RFISC", rpt_params.GetLang()));
    NewTextChild(variablesNode, "doc_cap_annul_trfer", getLocaleText("����", rpt_params.GetLang()));
    NewTextChild(variablesNode, "doc_cap_annul_trfer_dir", getLocaleText("�� ����", rpt_params.GetLang()));

    TAnnulBTStat AnnulBTStat;
    RunAnnulBTStat(AnnulBTStat, rpt_params.point_id);

    DB::TCachedQuery paxQry(
          PgOra::getROSession("PAX"),
          "SELECT reg_no, name, surname "
          "FROM pax "
          "WHERE pax_id = :pax_id",
          QParams() << QParam("pax_id", otInteger),
          STDLOG);

    struct TPaxInfo {
        int reg_no;
        string surname;
        string name;
        TPaxInfo(): reg_no(NoExists) {}
    };

    map<int, TPaxInfo> pax_map;

    for(list<TAnnulBTStatRow>::const_iterator i = AnnulBTStat.rows.begin(); i != AnnulBTStat.rows.end(); i++) {
        xmlNodePtr rowNode = NewTextChild(dataSetNode, "row");

        map<int, TPaxInfo>::iterator iPax = pax_map.find(i->pax_id);

        if(iPax == pax_map.end()) {
            TPaxInfo pax;
            if(i->pax_id != NoExists) {
                paxQry.get().SetVariable("pax_id", i->pax_id);
                paxQry.get().Execute();
                if(not paxQry.get().Eof) {
                    pax.reg_no = paxQry.get().FieldAsInteger("reg_no");
                    pax.name = paxQry.get().FieldAsString("name");
                    pax.surname = paxQry.get().FieldAsString("surname");
                }
            }
            pair<map<int, TPaxInfo>::iterator, bool> ret =
                pax_map.insert(make_pair(i->pax_id, pax));
            iPax = ret.first;
        }

        //  ����
        if(iPax->second.reg_no == NoExists)
            NewTextChild(rowNode, "reg_no");
        else
            NewTextChild(rowNode, "reg_no", iPax->second.reg_no);
        //  ���ᠦ�� ���
        ostringstream buf;
        if(iPax->second.reg_no != NoExists)
            buf
                << transliter(iPax->second.surname, TranslitFormat::V1, rpt_params.GetLang() != AstraLocale::LANG_RU) << " "
                << transliter(iPax->second.name, TranslitFormat::V1, rpt_params.GetLang() != AstraLocale::LANG_RU);
        NewTextChild(rowNode, "fio", buf.str());
        //  ����� ��ન
        NewTextChild(rowNode, "no", get_tag_range(i->tags, LANG_EN));
        //  ���祭�� �� ����
        if (i->weight != NoExists)
            NewTextChild(rowNode, "weight", i->weight);
        else
            NewTextChild(rowNode, "weight");

        //  ⨯ ������
        buf.str("");
        if(not i->rfisc.empty())
            buf << i->rfisc;
        else if(i->bag_type != NoExists)
            buf << ElemIdToNameLong(etBagType, i->bag_type);
        NewTextChild(rowNode, "bag_type", buf.str());

        if(i->trfer_airline.empty()) {
            //  �ਧ�.�࠭���
            NewTextChild(rowNode, "pr_trfer", getLocaleText("���", rpt_params.GetLang()));
            //  ���ࠢ����� ����
            NewTextChild(rowNode, "trfer_airp_arv");
        } else {
            NewTextChild(rowNode, "pr_trfer", getLocaleText("��", rpt_params.GetLang()));
            NewTextChild(rowNode, "trfer_airp_arv", rpt_params.ElemIdToReportElem(etAirp, i->trfer_airp_arv, efmtCodeNative));
        }
    }
}

void ANNULTXT(TRptParams &rpt_params, xmlNodePtr reqNode, xmlNodePtr resNode)
{   
    ANNUL(rpt_params, reqNode, resNode);

    xmlNodePtr variablesNode=NodeAsNode("form_data/variables",resNode);
    xmlNodePtr dataSetsNode=NodeAsNode("form_data/datasets",resNode);
    int page_width=80;
    NewTextChild(variablesNode, "page_width", page_width);
    NewTextChild(variablesNode, "test_server", STAT::bad_client_img_version() ? 2 : get_test_server());
    if(STAT::bad_client_img_version())
        NewTextChild(variablesNode, "doc_cap_test", " ");
    NewTextChild(variablesNode, "test_str", get_test_str(page_width, rpt_params.GetLang()));
    ostringstream s;
    vector<string> rows;
    string str;
    SeparateString(NodeAsString("caption", variablesNode), page_width, rows);
    s.str("");
    for(vector<string>::iterator iv = rows.begin(); iv != rows.end(); iv++) {
        if(iv != rows.begin())
            s << endl;
        s << right << setw(((page_width - iv->size()) / 2) + iv->size()) << *iv;
    }
    NewTextChild(variablesNode, "page_header_top", s.str());
    s.str("");
    NewTextChild(variablesNode, "page_footer_top",
            getLocaleText("CAP.ISSUE_DATE", LParams() << LParam("date", NodeAsString("date_issue",variablesNode)), rpt_params.GetLang()));

    TTextGrid tab;

    tab.addCol(getLocaleText("�", rpt_params.GetLang()),                3);
    tab.addCol(getLocaleText("�.�.�.", rpt_params.GetLang()),           rpt_params.IsInter() ? 27 : 30);
    tab.addCol(getLocaleText("�� ���. ��ப", rpt_params.GetLang()),    20);
    tab.addCol(getLocaleText("�� ���", rpt_params.GetLang()),           6);
    tab.addCol(getLocaleText("��� ������/RFISC", rpt_params.GetLang()), rpt_params.IsInter() ? 11 : 8);
    tab.addCol(getLocaleText("����", rpt_params.GetLang()),             5);
    tab.addCol(getLocaleText("�� ����", rpt_params.GetLang()),          8);
    tab.headerToXML(variablesNode);

    xmlNodePtr dataSetNode = NodeAsNode("v_annul", dataSetsNode);
    xmlNodeSetName(dataSetNode, "table");
    xmlNodePtr rowNode=dataSetNode->children;

    for(; rowNode != NULL; rowNode = rowNode->next)
    {
        xmlNodePtr curNode = rowNode->children;
        auto &row = tab.addRow();
        row
            .add(NodeAsStringFast("reg_no", curNode))
            .add(NodeAsStringFast("fio", curNode))
            .add(NodeAsStringFast("no", curNode))
            .add(NodeAsStringFast("weight", curNode))
            .add(NodeAsStringFast("bag_type", curNode))
            .add(NodeAsStringFast("pr_trfer", curNode))
            .add(NodeAsStringFast("trfer_airp_arv", curNode));
        row.toXML(rowNode);
    }
}
