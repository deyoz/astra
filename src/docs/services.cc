#include "services.h"
#include "stat/utils.h"
#include "utils.h"

#define NICKNAME "DENIS"
#include "serverlib/slogger.h"

using namespace std;
using namespace ASTRA;
using namespace AstraLocale;
using namespace EXCEPTIONS;

class TServiceFilter
{
    set<string> filterIncludeRFIC;
    set<string> filterExcludeRFIC;
public:
    void AddRFIC(string RFIC) { filterIncludeRFIC.insert(RFIC); }
    void ExcludeRFIC(string RFIC) { filterExcludeRFIC.insert(RFIC); }
    bool Check(const TServiceRow& row) const
    {
        if (filterIncludeRFIC.empty()) { if (filterExcludeRFIC.count(row.RFIC)) return false; else return true; }
        if (filterIncludeRFIC.count(row.RFIC)) return true; else return false;
    }
};

bool TServiceRow::operator < (const TServiceRow &other) const
{
    switch (mSortOrder)
    {
        case by_reg_no: return reg_no < other.reg_no;
        case by_family: return family < other.family;
        case by_seat_no: return seat_no < other.seat_no;
        case by_service_code: return RFISC < other.RFISC;
        default: throw EXCEPTIONS::Exception("TServiceRow::operator < : unexpected value");
    }
}

void TServiceRow::clear()
{
    pax_id = NoExists;

    airp_dep.clear();
    airp_arv.clear();

    seat_no.clear();
    family.clear();
    reg_no = NoExists;
    RFIC.clear();
    RFISC.clear();
    desc.clear();
    num.clear();
}

void TServiceRow::toXML(xmlNodePtr dataSetNode) const
{
    xmlNodePtr rowNode = NewTextChild(dataSetNode, "row");
    NewTextChild(rowNode, "seat_no", seat_no);
    NewTextChild(rowNode, "family", family);
    NewTextChild(rowNode, "reg_no", reg_no);
    NewTextChild(rowNode, "RFIC", RFIC);
    NewTextChild(rowNode, "RFISC", RFISC);
    NewTextChild(rowNode, "desc", desc);
    NewTextChild(rowNode, "num", num);
}

void TServiceList::fromDB(const TRptParams &rpt_params)
{
    clear();

    TQuery Qry(&OraSession);
    string SQLText =
    "select "
    "    pax_grp.*, "
    "    pax.*, "
    "    salons.get_seat_no(pax.pax_id,pax.seats,pax.is_jmp,pax_grp.status,pax_grp.point_dep,'_seats',rownum,:pr_lat) AS seat_no "
    "from "
    "    pax_grp, "
    "    pax "
    "where "
    "    pax_grp.point_dep = :point_id and "
    "    pax_grp.grp_id = pax.grp_id ";
    /*"order by "
    "    seat_no, pax.reg_no DESC ";*/
    Qry.SQLText = SQLText;
    Qry.CreateVariable("point_id", otInteger, rpt_params.point_id);
    Qry.CreateVariable("pr_lat", otInteger, rpt_params.IsInter());
    Qry.Execute();
    //  инициализация сортировки
    EServiceSortOrder sortOrder = by_reg_no;
    switch(rpt_params.sort)
    {
        case stRegNo: sortOrder = by_reg_no; break;
        case stSurname: sortOrder = by_family; break;
        case stSeatNo: sortOrder = by_seat_no; break;
        case stServiceCode: sortOrder = by_service_code; break;
    }
    //  инициализация фильтра
    TServiceFilter filter;
    if (pr_stat or TReqInfo::Instance()->desk.compatible( RFIC_FILTER_VERSION )) {
        for (list<string>::const_iterator iRFIC = rpt_params.rfic.begin(); iRFIC != rpt_params.rfic.end(); ++iRFIC) filter.AddRFIC(*iRFIC);
    } else {
        filter.ExcludeRFIC("C"); // Для старых терминалов в отчет должны попадать все услуги, кроме RFIC=C
    }
    if(not Qry.Eof) {
        int col_airp_dep = Qry.GetFieldIndex("airp_dep");
        int col_airp_arv = Qry.GetFieldIndex("airp_arv");

        //  цикл для каждого пакса в выборке
        CheckIn::TServiceReport service_report;
        for (; !Qry.Eof; Qry.Next())
        {
            CheckIn::TSimplePaxItem pax;
            pax.fromDB(Qry);
            int grp_id = Qry.FieldAsInteger("grp_id");

            const auto& services=service_report.get(grp_id);
            for(const auto &service: services) {

                const TPaidRFISCStatus &item =service.first;
                const boost::optional<CheckIn::TServicePaymentItem> &pay_info = service.second;

                if(item.pax_id != pax.id or item.trfer_num != 0) continue;

                TServiceRow row(sortOrder); // sortOrder для всех строк в контейнере должен быть одинаков!

                row.pax_id = item.pax_id;
                row.airp_dep = Qry.FieldAsString(col_airp_dep);
                row.airp_arv = Qry.FieldAsString(col_airp_arv);

                //  Место в салоне
                row.seat_no = pax.seat_no;
                //  ФИО пассажира
                row.family = transliter(pax.full_name(), 1, rpt_params.GetLang() != AstraLocale::LANG_RU);
                //  Рег. №
                row.reg_no = pax.reg_no;
                if(service.first.list_item) {
                    //  RFIC
                    row.RFIC = item.list_item->RFIC;
                    //  Код услуги
                    row.RFISC = item.list_item->RFISC;
                    //  Описание
                    row.desc = services.getRFISCName(item, rpt_params.GetLang());
                    if(pay_info) {
                        row.num = pay_info->no_str();
                    }
                }
                if (filter.Check(row)) push_back(row);
            }
        }
    }
}

void SERVICES(TRptParams &rpt_params, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    if(rpt_params.rpt_type == rtSERVICESTXT)
        get_compatible_report_form("docTxt", reqNode, resNode);
    else
        get_compatible_report_form("services", reqNode, resNode);

    xmlNodePtr formDataNode = NewTextChild(resNode, "form_data");
    xmlNodePtr dataSetsNode = NewTextChild(formDataNode, "datasets");
    xmlNodePtr dataSetNode = NewTextChild(dataSetsNode, "v_services");
    // переменные отчёта
    xmlNodePtr variablesNode = NewTextChild(formDataNode, "variables");
    PaxListVars(rpt_params.point_id, rpt_params, variablesNode);
    // заголовок отчёта
    NewTextChild(variablesNode, "caption",
        getLocaleText("CAP.DOC.SERVICES", LParams() << LParam("flight", get_flight(variablesNode)), rpt_params.GetLang()));
    populate_doc_cap(variablesNode, rpt_params.GetLang());
    TServiceList rows;
    rows.fromDB(rpt_params);
    rows.sort();
    for (list<TServiceRow>::const_iterator irow = rows.begin(); irow != rows.end(); ++irow)
        irow->toXML(dataSetNode);
    //  LogTrace(TRACE5) << GetXMLDocText(resNode->doc);
}

void SERVICESTXT(TRptParams &rpt_params, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    SERVICES(rpt_params, reqNode, resNode);

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
    s   << left
        << setw(8)  << getLocaleText("№ м", rpt_params.GetLang())
        << setw(20) << getLocaleText("Ф.И.О.", rpt_params.GetLang())
        << setw(3)  << getLocaleText("№", rpt_params.GetLang()) << " "
        << setw(5)  << getLocaleText("RFIC", rpt_params.GetLang())
        << setw(5)  << getLocaleText("Код", rpt_params.GetLang())
        << setw(20)  << getLocaleText("Описание", rpt_params.GetLang())
        << setw(20) << getLocaleText("№ квитанции", rpt_params.GetLang());
    NewTextChild(variablesNode, "page_header_bottom", s.str() );
    NewTextChild(variablesNode, "page_footer_top",
            getLocaleText("CAP.ISSUE_DATE", LParams() << LParam("date", NodeAsString("date_issue",variablesNode)), rpt_params.GetLang()));
    xmlNodePtr dataSetNode = NodeAsNode("v_services", dataSetsNode);
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
        SeparateString(NodeAsString("desc", rowNode), 19, rows);
        fields["desc"]=rows;
        SeparateString(NodeAsString("num", rowNode), 19, rows);
        fields["num"]=rows;

        row=0;
        s.str("");
        do
        {
            if (row != 0) s << endl;
            s   << left <<  setw(7) << (row == 0 ? NodeAsString("seat_no", rowNode, "") : "") << col_sym
                << setw(19) << (!fields["surname"].empty() ? *(fields["surname"].begin()) : "") << col_sym
                << setw(3) << (row == 0 ? NodeAsString("reg_no", rowNode) : "") << col_sym
                << setw(4) << (row == 0 ? NodeAsString("RFIC", rowNode) : "") << col_sym
                << setw(4) << (row == 0 ? NodeAsString("RFISC", rowNode) : "") << col_sym
                << setw(19) << (!fields["desc"].empty() ? *(fields["desc"].begin()) : "") << col_sym
                << setw(19) << (!fields["num"].empty() ? *(fields["num"].begin()) : "");
            for(map< string, vector<string> >::iterator f = fields.begin(); f != fields.end(); f++)
                if (!f->second.empty()) f->second.erase(f->second.begin());
            row++;
        }
        while(
                !fields["surname"].empty() ||
                !fields["desc"].empty() ||
                !fields["num"].empty()
             );
        NewTextChild(rowNode,"str",s.str());
    }
}
