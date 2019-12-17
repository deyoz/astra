#ifndef _DOCS_COMMON_H_
#define _DOCS_COMMON_H_

#include <string>
#include "astra_elems.h"
#include "print.h"
#include "docs_consts.h"

const std::string ALL_CKIN_ZONES = " ";

struct TRptParams {
    private:
        bool route_inter;
        std::string route_country_lang; //язык страны, где выполняется внутренний рейс
        std::string req_lang; // Язык, затребованный с клиента, если пустой, то вычисляем язык на основе маршрута
        std::string GetLang(TElemFmt &fmt, std::string firm_lang) const;
    public:
        TPrnParams prn_params;
        TSortType sort;
        int point_id;
        ASTRA::TRptType rpt_type; // тип отчета в зависимости от параметра Текстовый формат
        ASTRA::TRptType orig_rpt_type; // тип отчета, как он пришел в запросе
        std::string cls;
        std::string subcls;
        std::string airp_arv;
        std::string ckin_zone;
        bool pr_et;
        bool pr_trfer;
        bool pr_brd;
        TSimpleMktFlight mkt_flt;
        std::string client_type;
        std::map< TRemCategory, std::vector<std::string> > rems;
        std::list<std::string> rfic;
        int text;
        std::string ElemIdToReportElem(TElemType type, const std::string &id, TElemFmt fmt, std::string firm_lang = "") const;
        std::string ElemIdToReportElem(TElemType type, int id, TElemFmt fmt, std::string firm_lang = "") const;
        bool IsInter() const;
        std::string GetLang() const;
        std::string dup_lang() { return GetLang()==AstraLocale::LANG_EN ? AstraLocale::LANG_RU : GetLang(); }; // lang for duplicated captions
        void Init(xmlNodePtr node);
        void trace(TRACE_SIGNATURE) const;
        TRptParams(std::string lang) {
            req_lang = lang;
        };
        TRptParams():
            route_inter(true),
            point_id(ASTRA::NoExists),
            pr_et(false),
            pr_trfer(false),
            pr_brd(false),
            text(ASTRA::NoExists)
    {};
};

void PaxListVars(int point_id, TRptParams &rpt_params, xmlNodePtr variablesNode,
                 BASIC::date_time::TDateTime part_key = ASTRA::NoExists);

struct TBagNameRow {
    int bag_type;
    std::string rfisc;
    std::string class_code;
    std::string airp;
    std::string airline;
    std::string name;
    std::string name_lat;
    TBagNameRow(): bag_type(ASTRA::NoExists) {}
};

struct TBagTagRow {
    int pr_trfer;
    std::string last_target;
    int point_num;
    int grp_id;
    std::string airp_arv;
    int class_priority;
    std::string class_code;
    std::string class_name;
    int to_ramp;
    std::string to_ramp_str;
    int bag_type;
    int bag_name_priority;
    std::string bag_name;
    int bag_num;
    int amount;
    int weight;
    int pr_liab_limit;
    std::string tag_type;
    std::string color;
    double no;
    std::string tag_range;
    int num;
    std::string rfisc;
    TBagTagRow()
    {
        pr_trfer = -1;
        point_num = -1;
        grp_id = -1;
        class_priority = -1;
        bag_type = ASTRA::NoExists;
        bag_name_priority = -1;
        bag_num = -1;
        amount = -1;
        weight = -1;
        pr_liab_limit = -1;
        to_ramp = -1;
        no = -1.;
        num = -1;
    }
};

class t_rpt_bm_bag_name {
    private:
        std::vector<TBagNameRow> bag_names;
    public:
        void init(const std::string &airp, const std::string &airline, bool pr_stat_fv = false);
        void get(std::string class_code, TBagTagRow &bag_tag_row, TRptParams &rpt_params);
};

std::string get_flight(xmlNodePtr variablesNode);
void SeasonListVars(int trip_id, int pr_lat, xmlNodePtr variablesNode, xmlNodePtr reqNode);
std::string vs_number(int number, bool pr_lat = false);
void populate_doc_cap(xmlNodePtr variablesNode, std::string lang);
TDateTime getReportSCDOut(int point_id);
std::vector<std::string> get_grp_zone_list(int point_id);
void GetZoneList(int point_id, xmlNodePtr dataNode);
std::string get_hall_list(std::string airp, TRptParams &rpt_params);
void trip_rpt_person(xmlNodePtr resNode, TRptParams &rpt_params);
void get_new_report_form(const std::string name, xmlNodePtr reqNode, xmlNodePtr resNode);
void get_compatible_report_form(const std::string name, xmlNodePtr reqNode, xmlNodePtr resNode);
std::string get_report_version(std::string name);

struct t_tag_nos_row {
    int pr_liab_limit;
    double no;
};

std::string get_tag_range(std::vector<t_tag_nos_row> tag_nos, std::string lang);

#endif
