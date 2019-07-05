#include "docs_reseat.h"
#include "stat/stat_utils.h"
#include "docs_utils.h"
#include "telegram.h"

#define NICKNAME "DENIS"
#include "serverlib/slogger.h"

using namespace ASTRA;
using namespace AstraLocale;
using namespace std;

static std::string getClassView(const TRptParams &rpt_params,
                                const std::string& orig_cls,
                                const std::string& cabin_cls)
{
  ostringstream result;
  result << rpt_params.ElemIdToReportElem(etClass, orig_cls, efmtCodeNative);
  if (!cabin_cls.empty() && !orig_cls.empty() && cabin_cls!=orig_cls)
    result << "->" << rpt_params.ElemIdToReportElem(etClass, cabin_cls, efmtCodeNative);
  return result.str();
}

void RESEAT(TRptParams &rpt_params, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    if(rpt_params.rpt_type == rtRESEATTXT)
        get_compatible_report_form("docTxt", reqNode, resNode);
    else
        get_compatible_report_form("reseat", reqNode, resNode);

    xmlNodePtr formDataNode = NewTextChild(resNode, "form_data");
    xmlNodePtr dataSetsNode = NewTextChild(formDataNode, "datasets");
    xmlNodePtr dataSetNode = NewTextChild(dataSetsNode, "v_reseat");
    // переменные отчёта
    xmlNodePtr variablesNode = NewTextChild(formDataNode, "variables");
    PaxListVars(rpt_params.point_id, rpt_params, variablesNode);
    // заголовок отчёта
    NewTextChild(variablesNode, "caption",
        getLocaleText("CAP.DOC.RESEAT", LParams() << LParam("flight", get_flight(variablesNode)), rpt_params.GetLang()));
    populate_doc_cap(variablesNode, rpt_params.GetLang());

    TTlgCompLayerList complayers;
    TAdvTripInfo flt_info;
    flt_info.getByPointId(rpt_params.point_id);
    if(not SALONS2::isFreeSeating(flt_info.point_id) and not SALONS2::isEmptySalons(flt_info.point_id))
        getSalonLayers( flt_info.point_id, flt_info.point_num, flt_info.first_point, flt_info.pr_tranzit, complayers, false );

    map<bool,map <int,TSeatRanges> > seats; // true - старые, false - новые места
    SALONS2::getPaxSeatsWL(rpt_params.point_id, seats);

    // приведем seats к нормальному виду prepared_seats
    map<int, pair<string, string> > prepared_seats;

    // Пробег по паксам со старыми местами (в мэпе seats[true])
    for(map <int,TSeatRanges>::iterator old_seats = seats[true].begin();
            old_seats != seats[true].end(); old_seats++) {

        pair<string, string> &pair_seats = prepared_seats[old_seats->first];

        // Старые места выводим
        pair_seats.first = GetSeatRangeView(old_seats->second, "list", complayers.pr_craft_lat or rpt_params.GetLang() != AstraLocale::LANG_RU);

        // Ищем, есть ли у пакса новые места (в мэпе seats[false])
        // Если есть, выводим
        map<int,TSeatRanges>::iterator new_seats = seats[false].find(old_seats->first);
        if(new_seats != seats[false].end())
            pair_seats.second = GetSeatRangeView(new_seats->second, "list", complayers.pr_craft_lat or rpt_params.GetLang() != AstraLocale::LANG_RU);
    }

    struct TSortedPax {
        CheckIn::TSimplePaxItem pax;
        pair<string, string> seats;
    };

    map<int, TSortedPax> sorted_pax;
    for(map<int, pair<string, string> >::iterator i = prepared_seats.begin();
            i != prepared_seats.end(); i++) {
        CheckIn::TSimplePaxItem pax;
        pax.getByPaxId(i->first);
        sorted_pax[pax.reg_no].pax = pax;
        sorted_pax[pax.reg_no].seats = i->second;
    }

    // строки отчёта
    map<int, string> classes;
    for(map<int, TSortedPax>::iterator i = sorted_pax.begin();
            i != sorted_pax.end(); i++) {
        xmlNodePtr rowNode = NewTextChild(dataSetNode, "row");
        NewTextChild(rowNode, "reg_no", i->second.pax.reg_no);
        NewTextChild(rowNode, "full_name", transliter(i->second.pax.full_name(), 1, rpt_params.GetLang() != AstraLocale::LANG_RU));
        NewTextChild(rowNode, "pr_brd", i->second.pax.pr_brd);
        NewTextChild(rowNode, "seats", i->second.pax.seats);
        NewTextChild(rowNode, "old_seat_no", i->second.seats.first);
        NewTextChild(rowNode, "new_seat_no", i->second.seats.second);

        map<int, string>::iterator i_cls = classes.find(i->second.pax.grp_id);
        if(i_cls == classes.end()) {
            CheckIn::TSimplePaxGrpItem grp;
            grp.getByGrpId(i->second.pax.grp_id);
            pair<map<int, string>::iterator, bool> ret;
            ret = classes.insert(make_pair(i->second.pax.grp_id, grp.cl));
            i_cls = ret.first;
        }

        NewTextChild(rowNode, "cls", getClassView(rpt_params,
                                                  i_cls->second,
                                                  i->second.pax.cabin.cl.empty()?i_cls->second:i->second.pax.cabin.cl));

        NewTextChild(rowNode, "document", CheckIn::GetPaxDocStr(NoExists, i->second.pax.id, false, rpt_params.GetLang()));

        ostringstream ticket_no;
        ticket_no << i->second.pax.tkn.no;
        if(i->second.pax.tkn.coupon != NoExists)
            ticket_no << "/" << i->second.pax.tkn.coupon;

        NewTextChild(rowNode, "ticket_no", ticket_no.str());
    }
}

void RESEATTXT(TRptParams &rpt_params, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    RESEAT(rpt_params, reqNode, resNode);

    xmlNodePtr variablesNode=NodeAsNode("form_data/variables",resNode);
    xmlNodePtr dataSetsNode=NodeAsNode("form_data/datasets",resNode);
    int page_width=80;
    NewTextChild(variablesNode, "page_width", page_width);
    NewTextChild(variablesNode, "test_server", STAT::bad_client_img_version() ? 2 : get_test_server());
    if(STAT::bad_client_img_version())
        NewTextChild(variablesNode, "doc_cap_test", " ");
    NewTextChild(variablesNode, "test_str", get_test_str(page_width, rpt_params.GetLang()));
    ostringstream s;
    s.str("");
    s << NodeAsString("caption", variablesNode);
    string str = s.str().substr(0, page_width);
    s.str("");
    s << right << setw(((page_width - str.size()) / 2) + str.size()) << str;
    NewTextChild(variablesNode, "page_header_top", s.str());
    s.str("");
    s
        << left
        << setw(3)  << getLocaleText("№", rpt_params.GetLang())
        << setw(26) << getLocaleText("Ф.И.О.", rpt_params.GetLang())
        << setw(4)  << getLocaleText("Пс", rpt_params.GetLang())
        << setw(3)  << (getLocaleText("Кл", rpt_params.GetLang()))
        << setw(4)  << (getLocaleText("Крс", rpt_params.GetLang()))
        << setw(6)  << (getLocaleText("Стар.", rpt_params.GetLang()))
        << setw(8)  << (getLocaleText("Нов.", rpt_params.GetLang()))
        << setw(11) << getLocaleText("Документ", rpt_params.GetLang())
        << setw(15) << getLocaleText("Билет", rpt_params.GetLang());
    NewTextChild(variablesNode, "page_header_bottom", s.str() );
    NewTextChild(variablesNode, "page_footer_top",
            getLocaleText("CAP.ISSUE_DATE", LParams() << LParam("date", NodeAsString("date_issue",variablesNode)), rpt_params.GetLang()));
    xmlNodePtr dataSetNode = NodeAsNode("v_reseat", dataSetsNode);
    xmlNodeSetName(dataSetNode, "table");
    vector<string> rows;
    map< string, vector<string> > fields;
    int row;
    xmlNodePtr rowNode=dataSetNode->children;
    const char col_sym = ' ';
    for(; rowNode != NULL; rowNode = rowNode->next)
    {
        SeparateString(NodeAsString("full_name", rowNode), 25, rows);
        fields["full_name"]=rows;

        row=0;
        s.str("");
        do
        {
            if (row != 0) s << endl;
            s
                << right << setw(2) << (row == 0 ? NodeAsString("reg_no", rowNode) : "") << col_sym
                << left << setw(25) << (!fields["full_name"].empty() ? *(fields["full_name"].begin()) : "") << col_sym
                << left <<  setw(3) << (row == 0 ? (strcmp(NodeAsString("pr_brd", rowNode, "0"), "0") == 0 ? "-" : "+") : "") << col_sym
                << left <<  setw(2) << (row == 0 ? NodeAsString("cls", rowNode) : "") << col_sym
                << left <<  setw(3) << (row == 0 ? NodeAsString("seats", rowNode) : "") << col_sym
                << left <<  setw(5) << (row == 0 ? NodeAsString("old_seat_no", rowNode) : "") << col_sym
                << left <<  setw(7) << (row == 0 ? NodeAsString("new_seat_no", rowNode) : "") << col_sym
                << left <<  setw(10) << (row == 0 ? NodeAsString("document", rowNode) : "") << col_sym
                << left <<  setw(15) << (row == 0 ? NodeAsString("ticket_no", rowNode) : "");
            for(map< string, vector<string> >::iterator f = fields.begin(); f != fields.end(); f++)
                if (!f->second.empty()) f->second.erase(f->second.begin());
            row++;
        }
        while(!fields["full_name"].empty());
        NewTextChild(rowNode,"str",s.str());
    }
}

