#include "rem.h"
#include "stat/utils.h"
#include "utils.h"
#include "serverlib/xmllibcpp.h"
#include "pax_list.h"

#define NICKNAME "DENIS"
#include "serverlib/slogger.h"

using namespace ASTRA;
using namespace std;
using namespace AstraLocale;

namespace REPORTS {
    struct TREMPaxList: public TPaxList {
        bool is_spec;
        std::map< TRemCategory, std::vector<std::string> > &rems;
        boost::optional<TRemGrp> spec_rems;

        TPaxPtr getPaxPtr();
        TREMPaxList(TRptParams &_rpt_params):
            TPaxList(_rpt_params.point_id),
            rems(_rpt_params.rems)
        {
            options.lang = _rpt_params.GetLang();
            options.sort = _rpt_params.sort;
            options.pr_brd = boost::in_place(REPORTS::TBrdVal::bvNOT_NULL);
            options.flags.setFlag(REPORTS::oeSeatNo);
            is_spec = _rpt_params.rpt_type == rtSPEC or _rpt_params.rpt_type == rtSPECTXT;
            spec_rems = boost::none;
        }
    };

    struct TREMPax: public TPax {
        multiset<CheckIn::TPaxRemItem> _final_rems;
        multiset<CheckIn::TPaxRemItem> final_rems() const;
        string final_rems_str() const;

        TREMPaxList &get_pax_list() const;
        void fromDB(TQuery &Qry);
        void clear()
        {
            TPax::clear();
            _final_rems.clear();
        }

        TREMPax(TPaxList &_pax_list):
            TPax(_pax_list)
        {
            clear();
        }
    };

    TPaxPtr TREMPaxList::getPaxPtr()
    {
        return TPaxPtr(new TREMPax(*this));
    }

    void TREMPax::fromDB(TQuery &Qry)
    {
        TPax::fromDB(Qry);
        REPORT_PAX_REMS::get(Qry, pax_list.options.lang, get_pax_list().rems, _final_rems);
        if(get_pax_list().is_spec) {
            if(not get_pax_list().spec_rems) {
                get_pax_list().spec_rems = boost::in_place();
                get_pax_list().spec_rems.get().Load(retRPT_SS, pax_list.point_id);
            }
            for(multiset<CheckIn::TPaxRemItem>::iterator r=_final_rems.begin();r!=_final_rems.end();)
            {
                if (!get_pax_list().spec_rems.get().exists(r->code)) r=Erase(_final_rems, r); else ++r;
            };
        }
    }

    string TREMPax::final_rems_str() const
    {
        auto tmp_rems = final_rems();
        set<string> text_list;
        for(auto r: tmp_rems) text_list.insert(r.text);
        ostringstream rem_info;
        for(auto r: text_list) {
            rem_info << ".R/" << r << " ";
        }
        return rem_info.str();
    }

    multiset<CheckIn::TPaxRemItem> TREMPax::final_rems() const
    {
        multiset<CheckIn::TPaxRemItem> result = _final_rems;
        for(const auto &cbbg: cbbg_list) {
            if(cbbg.pax_info) {
                const REPORTS::TREMPax &pax = dynamic_cast<const REPORTS::TREMPax&>(*cbbg.pax_info);
                result.insert(
                        pax._final_rems.begin(),
                        pax._final_rems.end());
            }
        }
        return result;
    }

    TREMPaxList &TREMPax::get_pax_list() const
    {
        return dynamic_cast<TREMPaxList&>(pax_list);
    }

}

void REM(TRptParams &rpt_params, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    if(rpt_params.rpt_type == rtREMTXT or rpt_params.rpt_type == rtSPECTXT)
        get_compatible_report_form("docTxt", reqNode, resNode);
    else
        get_compatible_report_form("rem", reqNode, resNode);

    xmlNodePtr formDataNode = NewTextChild(resNode, "form_data");
    xmlNodePtr dataSetsNode = NewTextChild(formDataNode, "datasets");
    xmlNodePtr dataSetNode = NewTextChild(dataSetsNode, "v_rem");

    bool is_spec = rpt_params.rpt_type == rtSPEC or rpt_params.rpt_type == rtSPECTXT;

    if(old_cbbg()) {
        TRemGrp spec_rems;
        if(is_spec) spec_rems.Load(retRPT_SS, rpt_params.point_id);

        TQuery Qry(&OraSession);
        string SQLText =
            "SELECT pax_grp.point_dep AS point_id, "
            "       pax.*, "
            "       salons.get_seat_no(pax.pax_id,pax.seats,pax.is_jmp,pax_grp.status,pax_grp.point_dep,'_seats',rownum,:pr_lat) AS seat_no "
            "FROM   pax_grp,pax "
            "WHERE  pax_grp.grp_id=pax.grp_id AND "
            "       pr_brd IS NOT NULL and "
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
        Qry.CreateVariable("pr_lat", otInteger, rpt_params.IsInter());
        Qry.Execute();

        for(; !Qry.Eof; Qry.Next())
        {
            CheckIn::TSimplePaxItem pax;
            pax.fromDB(Qry);

            multiset<CheckIn::TPaxRemItem> final_rems;
            REPORT_PAX_REMS::get(Qry, rpt_params.GetLang(), rpt_params.rems, final_rems);

            if(is_spec)
            {
                for(multiset<CheckIn::TPaxRemItem>::iterator r=final_rems.begin();r!=final_rems.end();)
                {
                    if (!spec_rems.exists(r->code)) r=Erase(final_rems, r); else ++r;
                };
            }

            if (final_rems.empty()) continue;

            xmlNodePtr rowNode = NewTextChild(dataSetNode, "row");
            NewTextChild(rowNode, "point_id", Qry.FieldAsInteger("point_id"));
            NewTextChild(rowNode, "reg_no", pax.reg_no);
            NewTextChild(rowNode, "family", transliter(pax.full_name(), 1, rpt_params.GetLang() != AstraLocale::LANG_RU));
            NewTextChild(rowNode, "pers_type", rpt_params.ElemIdToReportElem(etPersType, EncodePerson(pax.pers_type), efmtCodeNative));
            NewTextChild(rowNode, "seat_no", pax.seat_no);

            ostringstream rem_info;
            for(multiset<CheckIn::TPaxRemItem>::const_iterator r=final_rems.begin();r!=final_rems.end();++r)
            {
                rem_info << ".R/" << r->text << " ";
            };
            NewTextChild(rowNode, "info", rem_info.str());
        }
    } else {
        REPORTS::TREMPaxList pax_list(rpt_params);

        pax_list.fromDB();
        pax_list.sort(REPORTS::pax_cmp);
        for(const auto &pax_item: pax_list) {
            const REPORTS::TREMPax &pax = dynamic_cast<const REPORTS::TREMPax&>(*pax_item);
            string info = pax.final_rems_str();
            if(info.empty()) continue;
            xmlNodePtr rowNode = NewTextChild(dataSetNode, "row");
            NewTextChild(rowNode, "reg_no", pax.simple.reg_no);
            NewTextChild(rowNode, "family", pax.full_name_view());
            NewTextChild(rowNode, "pers_type", rpt_params.ElemIdToReportElem(etPersType, EncodePerson(pax.simple.pers_type), efmtCodeNative));
            NewTextChild(rowNode, "seat_no", pax.seat_no());
            NewTextChild(rowNode, "info", info);
        }
    }

    // Теперь переменные отчета
    xmlNodePtr variablesNode = NewTextChild(formDataNode, "variables");
    PaxListVars(rpt_params.point_id, rpt_params, variablesNode);
    NewTextChild(variablesNode, "caption", getLocaleText(is_spec ? "CAP.DOC.SPEC" : "CAP.DOC.REM",
                LParams() << LParam("flight", get_flight(variablesNode)), rpt_params.GetLang()));
    populate_doc_cap(variablesNode, rpt_params.GetLang());
}

void REMTXT(TRptParams &rpt_params, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    REM(rpt_params, reqNode, resNode);

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
        << setw(38) << getLocaleText("Ф.И.О.", rpt_params.GetLang())
        << setw(5)  << getLocaleText("Тип", rpt_params.GetLang())
        << setw(8)  << getLocaleText("№ м", rpt_params.GetLang())
        << setw(25) << getLocaleText("Ремарки", rpt_params.GetLang());
    NewTextChild(variablesNode, "page_header_bottom", s.str() );
    NewTextChild(variablesNode, "page_footer_top",
            getLocaleText("CAP.ISSUE_DATE", LParams() << LParam("date", NodeAsString("date_issue",variablesNode)), rpt_params.GetLang()));
    xmlNodePtr dataSetNode = NodeAsNode("v_rem", dataSetsNode);
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

        SeparateString(NodeAsString("info", rowNode), 25, rows);
        fields["rems"]=rows;

        row=0;
        s.str("");
        do
        {
            if (row != 0) s << endl;
            s
                << right << setw(3) << (row == 0 ? NodeAsString("reg_no", rowNode) : "") << col_sym
                << left << setw(37) << (!fields["surname"].empty() ? *(fields["surname"].begin()) : "") << col_sym
                << right <<  setw(4) << (row == 0 ? NodeAsString("pers_type", rowNode, "ВЗ") : "") << col_sym
                << left <<  setw(7) << (row == 0 ? NodeAsString("seat_no", rowNode, "") : "") << col_sym
                << left << setw(25) << (!fields["rems"].empty() ? *(fields["rems"].begin()) : "");
            for(map< string, vector<string> >::iterator f = fields.begin(); f != fields.end(); f++)
                if (!f->second.empty()) f->second.erase(f->second.begin());
            row++;
        }
        while(
                !fields["surname"].empty() ||
                !fields["rems"].empty()
             );
        NewTextChild(rowNode,"str",s.str());
    }
}

