#include "docs_crs.h"
#include "stat/stat_utils.h"
#include "docs_utils.h"

#define NICKNAME "DENIS"
#include "serverlib/slogger.h"

using namespace ASTRA;
using namespace std;
using namespace AstraLocale;
using namespace EXCEPTIONS;

void CRS(TRptParams &rpt_params, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    if(rpt_params.rpt_type == rtCRSTXT or rpt_params.rpt_type == rtCRSUNREGTXT or rpt_params.rpt_type == rtBDOCSTXT)
        get_compatible_report_form("docTxt", reqNode, resNode);
    else
        get_compatible_report_form((rpt_params.rpt_type == rtBDOCS ? "bdocs" : "crs"), reqNode, resNode);
    bool pr_unreg = rpt_params.rpt_type == rtCRSUNREG or rpt_params.rpt_type == rtCRSUNREGTXT;
    xmlNodePtr formDataNode = NewTextChild(resNode, "form_data");
    TQuery Qry(&OraSession);
    string SQLText =
        "SELECT "
        "      crs_pax.pax_id, "
        "      crs_pax.surname||' '||crs_pax.name family ";
    if(rpt_params.rpt_type != rtBDOCS) {
        SQLText +=
            "      , tlg_binding.point_id_spp AS point_id, "
            "      crs_pax.pers_type, "
            "      crs_pnr.class, "
            "      salons.get_crs_seat_no(crs_pax.seat_xname,crs_pax.seat_yname,crs_pax.seats,crs_pnr.point_id,'_seats',rownum) AS seat_no, "
            "      crs_pnr.airp_arv AS target, "
            "      crs_pnr.airp_arv_final AS last_target, "
            "      crs_pnr.pnr_id, "
            "      report.get_TKNO(crs_pax.pax_id) ticket_no, "
            "      report.get_PSPT(crs_pax.pax_id, 1, :lang) AS document ";
        Qry.CreateVariable("lang", otString, rpt_params.GetLang());
    }
    SQLText +=
        "FROM crs_pnr,tlg_binding,crs_pax ";
    if(pr_unreg)
        SQLText += " , pax ";
    SQLText +=
        "WHERE crs_pnr.point_id=tlg_binding.point_id_tlg AND "
        "      crs_pnr.system='CRS' AND "
        "      crs_pnr.pnr_id=crs_pax.pnr_id AND "
        "      crs_pax.pr_del=0 and "
        "      tlg_binding.point_id_spp = :point_id ";
    if(pr_unreg)
        SQLText +=
            "    and crs_pax.pax_id = pax.pax_id(+) and "
            "    pax.pax_id is null ";
    SQLText +=
        "order by ";
    switch(rpt_params.sort) {
        case stServiceCode:
        case stRegNo:
        case stSurname:
            SQLText += " family ";
            break;
        case stSeatNo:
            if(rpt_params.rpt_type != rtBDOCS)
                SQLText += " seat_no,";
            SQLText += " family ";
            break;
    }
    Qry.SQLText = SQLText;
    Qry.CreateVariable("point_id", otInteger, rpt_params.point_id);
    Qry.Execute();
    xmlNodePtr dataSetsNode = NewTextChild(formDataNode, "datasets");
    xmlNodePtr dataSetNode = NewTextChild(dataSetsNode, "v_crs");

    TQuery docsQry(&OraSession);
    docsQry.SQLText = "select * from crs_pax_doc where pax_id = :pax_id and rem_code = 'DOCS'";
    docsQry.DeclareVariable("pax_id", otInteger);
    //ремарки пассажиров

    TRemGrp rem_grp;
    if(rpt_params.rpt_type != rtBDOCS)
        rem_grp.Load(retPNL_SEL, rpt_params.point_id);
    for(; !Qry.Eof; Qry.Next()) {
        int pax_id=Qry.FieldAsInteger("pax_id");
        if(
                rpt_params.rpt_type == rtBDOCS or
                rpt_params.rpt_type == rtBDOCSTXT
          ) {
            docsQry.SetVariable("pax_id", pax_id);
            docsQry.Execute();
            if (!docsQry.Eof)
            {
                if (
                        !docsQry.FieldIsNULL("type") &&
                        !docsQry.FieldIsNULL("issue_country") &&
                        !docsQry.FieldIsNULL("no") &&
                        !docsQry.FieldIsNULL("nationality") &&
                        !docsQry.FieldIsNULL("birth_date") &&
                        !docsQry.FieldIsNULL("gender") &&
                        !docsQry.FieldIsNULL("expiry_date") &&
                        !docsQry.FieldIsNULL("surname") &&
                        !docsQry.FieldIsNULL("first_name")
                   )
                    continue;

                xmlNodePtr rowNode = NewTextChild(dataSetNode, "row");
                NewTextChild(rowNode, "family", transliter(Qry.FieldAsString("family"), 1, rpt_params.GetLang() != AstraLocale::LANG_RU));
                NewTextChild(rowNode, "type", rpt_params.ElemIdToReportElem(etPaxDocType, docsQry.FieldAsString("type"), efmtCodeNative));
                NewTextChild(rowNode, "issue_country", rpt_params.ElemIdToReportElem(etPaxDocCountry, docsQry.FieldAsString("issue_country"), efmtCodeNative));
                NewTextChild(rowNode, "no", docsQry.FieldAsString("no"));
                NewTextChild(rowNode, "nationality", rpt_params.ElemIdToReportElem(etPaxDocCountry, docsQry.FieldAsString("nationality"), efmtCodeNative));
                if (!docsQry.FieldIsNULL("birth_date"))
                    NewTextChild(rowNode, "birth_date", DateTimeToStr(docsQry.FieldAsDateTime("birth_date"), "dd.mm.yyyy"));
                NewTextChild(rowNode, "gender", rpt_params.ElemIdToReportElem(etGenderType, docsQry.FieldAsString("gender"), efmtCodeNative));
                if (!docsQry.FieldIsNULL("expiry_date"))
                    NewTextChild(rowNode, "expiry_date", DateTimeToStr(docsQry.FieldAsDateTime("expiry_date"), "dd.mm.yyyy"));
                NewTextChild(rowNode, "surname", docsQry.FieldAsString("surname"));
                NewTextChild(rowNode, "first_name", docsQry.FieldAsString("first_name"));
                NewTextChild(rowNode, "second_name", docsQry.FieldAsString("second_name"));
            } else {
                xmlNodePtr rowNode = NewTextChild(dataSetNode, "row");
                NewTextChild(rowNode, "family", transliter(Qry.FieldAsString("family"), 1, rpt_params.GetLang() != AstraLocale::LANG_RU));
            }
        } else {
            xmlNodePtr rowNode = NewTextChild(dataSetNode, "row");
            NewTextChild(rowNode, "family", transliter(Qry.FieldAsString("family"), 1, rpt_params.GetLang() != AstraLocale::LANG_RU));
            NewTextChild(rowNode, "point_id", Qry.FieldAsInteger("point_id"));
            string pnr_addr=TPnrAddrs().firstAddrByPnrId(Qry.FieldAsInteger("pnr_id"), TPnrAddrInfo::AddrOnly); //пока не надо выводить компанию, может быть потом...
            NewTextChild(rowNode, "pnr_ref", pnr_addr);
            NewTextChild(rowNode, "pers_type", rpt_params.ElemIdToReportElem(etPersType, Qry.FieldAsString("pers_type"), efmtCodeNative));
            NewTextChild(rowNode, "class", rpt_params.ElemIdToReportElem(etClass, Qry.FieldAsString("class"), efmtCodeNative));
            NewTextChild(rowNode, "seat_no", Qry.FieldAsString("seat_no"));
            NewTextChild(rowNode, "target", rpt_params.ElemIdToReportElem(etAirp, Qry.FieldAsString("target"), efmtCodeNative));
            string last_target = Qry.FieldAsString("last_target");
            TElemFmt fmt;
            string last_target_id = ElemToElemId(etAirp, last_target, fmt);
            if(not last_target_id.empty())
                last_target = rpt_params.ElemIdToReportElem(etAirp, last_target_id, efmtCodeNative);

            NewTextChild(rowNode, "last_target", last_target);
            NewTextChild(rowNode, "ticket_no", Qry.FieldAsString("ticket_no"));
            NewTextChild(rowNode, "document", Qry.FieldAsString("document"));
            NewTextChild(rowNode, "remarks", GetCrsRemarkStr(rem_grp, pax_id));
        }
    }

    // Теперь переменные отчета
    xmlNodePtr variablesNode = NewTextChild(formDataNode, "variables");
    PaxListVars(rpt_params.point_id, rpt_params, variablesNode);
    if(pr_unreg)
        NewTextChild(variablesNode, "caption", getLocaleText("CAP.DOC.CRSUNREG",
                    LParams() << LParam("flight", get_flight(variablesNode)), rpt_params.GetLang()));
    else if(
            rpt_params.rpt_type == rtBDOCS or
            rpt_params.rpt_type == rtBDOCSTXT
           )
        NewTextChild(variablesNode, "caption", getLocaleText("CAP.DOC.BDOCS",
                    LParams() << LParam("flight", get_flight(variablesNode)), rpt_params.GetLang()));
    else
        NewTextChild(variablesNode, "caption", getLocaleText("CAP.DOC.CRS",
                    LParams() << LParam("flight", get_flight(variablesNode)), rpt_params.GetLang()));
    populate_doc_cap(variablesNode, rpt_params.GetLang());
    if(rpt_params.rpt_type == rtBDOCS) {
        NewTextChild(variablesNode, "doc_cap_bdocs", getLocaleText("Документ из DOCS", rpt_params.GetLang()));
        NewTextChild(variablesNode, "doc_cap_pax", getLocaleText("Пассажир", rpt_params.GetLang()));
        NewTextChild(variablesNode, "doc_cap_issue_country", getLocaleText("Гос-во выдачи", rpt_params.GetLang()));
        NewTextChild(variablesNode, "doc_cap_number", getLocaleText("Номер", rpt_params.GetLang()));
        NewTextChild(variablesNode, "doc_cap_nation", getLocaleText("Граж.", rpt_params.GetLang()));
        NewTextChild(variablesNode, "doc_cap_birth", getLocaleText("CAP.PAX_DOC.BIRTH_DATE", rpt_params.GetLang()));
        NewTextChild(variablesNode, "doc_cap_sex", getLocaleText("Пол", rpt_params.GetLang()));
        NewTextChild(variablesNode, "doc_cap_expiry", getLocaleText("Оконч. действия", rpt_params.GetLang()));
        NewTextChild(variablesNode, "doc_cap_first_name", getLocaleText("Имя", rpt_params.GetLang()));
        NewTextChild(variablesNode, "doc_cap_second_name", getLocaleText("Отчество", rpt_params.GetLang()));
    }

}

void CRSTXT(TRptParams &rpt_params, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    CRS(rpt_params, reqNode, resNode);

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
    vector<string> rows;
    string str;
    SeparateString(NodeAsString("caption", variablesNode), max_symb_count, rows);
    s.str("");
    for(vector<string>::iterator iv = rows.begin(); iv != rows.end(); iv++) {
        if(iv != rows.begin())
            s << endl;
        s << right << setw(((page_width - iv->size()) / 2) + iv->size()) << *iv;
    }
    NewTextChild(variablesNode, "page_header_top", s.str());
    s.str("");
    s
        << right << setw(3)  << getLocaleText("№", rpt_params.GetLang()) << " "
        << left
        << setw(7)  << "PNR"
        << setw(22) << getLocaleText("Ф.И.О.", rpt_params.GetLang())
        << setw(5)  << getLocaleText("Пас", rpt_params.GetLang())
        << setw(3) << getLocaleText("Кл", rpt_params.GetLang())
        << setw(8)  << getLocaleText("№ м", rpt_params.GetLang())
        << setw(4)  << getLocaleText("CAP.DOC.AIRP_ARV", rpt_params.GetLang())
        << setw(7)  << getLocaleText("CAP.DOC.TO", rpt_params.GetLang())
        << setw(10) << getLocaleText("Билет", rpt_params.GetLang())
        << setw(10) << getLocaleText("Документ", rpt_params.GetLang());
    NewTextChild(variablesNode, "page_header_bottom", s.str() );
    NewTextChild(variablesNode, "page_footer_top",
            getLocaleText("CAP.ISSUE_DATE", LParams() << LParam("date", NodeAsString("date_issue",variablesNode)), rpt_params.GetLang()));
    xmlNodePtr dataSetNode = NodeAsNode("v_crs", dataSetsNode);
    xmlNodeSetName(dataSetNode, "table");
    map< string, vector<string> > fields;
    int row;
    xmlNodePtr rowNode=dataSetNode->children;
    const char col_sym = ' ';
    int row_num = 1;
    for(; rowNode != NULL; rowNode = rowNode->next)
    {
        SeparateString(NodeAsString("pnr_ref", rowNode), 6, rows);
        fields["pnrs"]=rows;

        SeparateString(NodeAsString("family", rowNode), 21, rows);
        fields["surname"]=rows;

        SeparateString(NodeAsString("ticket_no", rowNode), 9, rows);
        fields["tkts"]=rows;

        SeparateString(NodeAsString("document", rowNode), 10, rows);
        fields["docs"]=rows;


        row=0;
        s.str("");
        do
        {
            if (row != 0) s << endl;
            s
                << right << setw(3) << (row == 0 ? IntToString(row_num++) : "") << col_sym
                << left << setw(6) << (!fields["pnrs"].empty() ? *(fields["pnrs"].begin()) : "") << col_sym
                << left << setw(21) << (!fields["surname"].empty() ? *(fields["surname"].begin()) : "") << col_sym
                << right <<  setw(4) << (row == 0 ? NodeAsString("pers_type", rowNode, "ВЗ") : "") << col_sym
                << left << setw(2) << (row == 0 ? NodeAsString("class", rowNode) : "") << col_sym
                << left <<  setw(7) << (row == 0 ? NodeAsString("seat_no", rowNode, "") : "") << col_sym
                << left << setw(3) << (row == 0 ? NodeAsString("target", rowNode) : "") << col_sym
                << left << setw(6) << (row == 0 ? NodeAsString("last_target", rowNode) : "") << col_sym
                << left << setw(9) << (!fields["tkts"].empty() ? *(fields["tkts"].begin()) : "") << col_sym
                << left << setw(10) << (!fields["docs"].empty() ? *(fields["docs"].begin()) : "");
            for(map< string, vector<string> >::iterator f = fields.begin(); f != fields.end(); f++)
                if (!f->second.empty()) f->second.erase(f->second.begin());
            row++;
        }
        while(
                !fields["pnrs"].empty() ||
                !fields["surname"].empty() ||
                !fields["tkts"].empty() ||
                !fields["docs"].empty()
             );
        NewTextChild(rowNode,"str",s.str());
    }
}

struct TTextGrid {
    private:
        size_t tab_width;

        list<pair<string, size_t>> hdr;

        struct TRow:public vector<string> {
            private:
                TTextGrid &grd;
            public:
            TRow &add(const string &data);
            void toXML(xmlNodePtr rowNode);
            TRow(TTextGrid &grd): grd(grd) {}
        };

        list<TRow> grid;

        bool check_fileds_empty(const map<int, vector<string>> &fields);
        void fill_grid(ostringstream &s, boost::optional<const TRow &> row = boost::none);

    public:
        void headerToXML(xmlNodePtr variablesNode);
        void addCol(const string &caption, int length = NoExists);
        TRow &addRow();
        void trace();
        void clear()
        {
            tab_width = 0;
            hdr.clear();
            grid.clear();
        }
        TTextGrid() { clear(); }
};

void TTextGrid::headerToXML(xmlNodePtr variablesNode)
{
    ostringstream s;
    s << left;
    fill_grid(s);
    NewTextChild(variablesNode,"page_header_bottom",s.str());
}

void TTextGrid::TRow::toXML(xmlNodePtr rowNode)
{
    ostringstream s;
    s << left;
    grd.fill_grid(s, *this);
    NewTextChild(rowNode,"str",s.str());
}

void TTextGrid::addCol(const string &caption, int length)
{
    if(length == NoExists)
        length = caption.size() + 1;
    tab_width += length;
    hdr.push_back(make_pair(caption, length));
}

TTextGrid::TRow &TTextGrid::addRow()
{
    grid.emplace_back(TRow(*this));
    return grid.back();
}

TTextGrid::TRow &TTextGrid::TRow::add(const string &data)
{
    push_back(data);
    return *this;
}

bool TTextGrid::check_fileds_empty(const map<int, vector<string>> &fields)
{
    bool result = false;
    for(const auto &i: fields)
        if(not i.second.empty()) {
            result = true;
            break;
        }
    return result;
}

void TTextGrid::fill_grid(ostringstream &s, boost::optional<const TRow &> row)
{
    if(row and row->size() != hdr.size())
        throw Exception("hdr not equal row: %d %d", row->size(), hdr.size());
    map< int, vector<string> > fields;
    int fields_idx = 0;
    for(const auto i: hdr) {
        const string &cell_data = (row ? (*row)[fields_idx] : i.first);
        if(cell_data.size() >= i.second) {
            vector<string> rows;
            SeparateString(cell_data.c_str(), i.second - 1, rows);
            fields[fields_idx++] = rows;
        } else
            fields[fields_idx++].push_back(cell_data);
    }
    int row_idx = 0;
    do {
        fields_idx = 0;
        if(row_idx != 0) s << endl;
        for(const auto &i: hdr) {
            s << setw(i.second) << (fields[fields_idx].empty() ? "" : *(fields[fields_idx].begin()));
            fields_idx++;
        }
        for(auto &i: fields)
            if(not i.second.empty()) i.second.erase(i.second.begin());
        row_idx++;
    } while(check_fileds_empty(fields));
}

void TTextGrid::trace()
{
    LogTrace(TRACE5) << "TTextGrid::trace tab_width: " << tab_width;
    ostringstream s;
    s << left << endl;
    fill_grid(s);
    s << endl;
    s << string(tab_width, '-') << endl;

    for(const auto &i: grid) {
        fill_grid(s, i);
        s << endl;
    }

    LogTrace(TRACE5) << s.str();
}

void BDOCSTXT(TRptParams &rpt_params, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    CRS(rpt_params, reqNode, resNode);

    xmlNodePtr variablesNode=NodeAsNode("form_data/variables",resNode);
    xmlNodePtr dataSetsNode=NodeAsNode("form_data/datasets",resNode);
    int page_width=130;
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
    tab.addCol(getLocaleText("Пассажир", rpt_params.GetLang()),                 28);
    tab.addCol(getLocaleText("Тип", rpt_params.GetLang()),                      4);
    tab.addCol(getLocaleText("Гос-во выдачи", rpt_params.GetLang()),            5);
    tab.addCol(getLocaleText("Номер", rpt_params.GetLang()),                    11);
    tab.addCol(getLocaleText("Граж.", rpt_params.GetLang()),                    6);
    tab.addCol(getLocaleText("CAP.PAX_DOC.BIRTH_DATE", rpt_params.GetLang()),   11);
    tab.addCol(getLocaleText("Пол", rpt_params.GetLang()),                      4);
    tab.addCol(getLocaleText("Оконч. действия", rpt_params.GetLang()),          16);
    tab.addCol(getLocaleText("Фамилия", rpt_params.GetLang()),                  15);
    tab.addCol(getLocaleText("Имя", rpt_params.GetLang()),                      15);
    tab.addCol(getLocaleText("Отчество", rpt_params.GetLang()),                 15);
    tab.headerToXML(variablesNode);

    xmlNodePtr dataSetNode = NodeAsNode("v_crs", dataSetsNode);
    xmlNodeSetName(dataSetNode, "table");
    xmlNodePtr rowNode=dataSetNode->children;

    for(; rowNode != NULL; rowNode = rowNode->next)
    {
        xmlNodePtr curNode = rowNode->children;
        auto &row = tab.addRow();
        row
            .add(NodeAsStringFast("family", curNode))
            .add(NodeAsStringFast("type", curNode, ""))
            .add(NodeAsStringFast("issue_country", curNode, ""))
            .add(NodeAsStringFast("no", curNode, ""))
            .add(NodeAsStringFast("nationality", curNode, ""))
            .add(NodeAsStringFast("birth_date", curNode, ""))
            .add(NodeAsStringFast("gender", curNode, ""))
            .add(NodeAsStringFast("expiry_date", curNode, ""))
            .add(NodeAsStringFast("surname", curNode, ""))
            .add(NodeAsStringFast("first_name", curNode, ""))
            .add(NodeAsStringFast("second_name", curNode, ""));
        row.toXML(rowNode);
    }
}
