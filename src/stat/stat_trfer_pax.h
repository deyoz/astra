#ifndef _STAT_TRFER_PAX_H_
#define _STAT_TRFER_PAX_H_

#include "stat_common.h"

struct TTrferPaxStatItem:public TOrderStatItem {
    std::string airline;
    std::string airp;
    int flt_no1;
    std::string suffix1;

    TDateTime date1;

    std::string trfer_airp;
    std::string airline2;
    int flt_no2;
    std::string suffix2;

    TDateTime date2;

    std::string airp_arv;
    TSegCategories::Enum seg_category;
    std::string pax_name;
    std::string pax_doc;
    std::string tags;
    int pax_amount;
    int adult;
    int child;
    int baby;

    int rk_weight;
    int bag_amount;
    int bag_weight;

    TTrferPaxStatItem()
    {
        clear();
    }

    void clear();

    void add_header(std::ostringstream &buf) const;
    void add_data(std::ostringstream &buf) const;
};

typedef std::list<TTrferPaxStatItem> TTrferPaxStat;

void createXMLTrferPaxStat(
        const TStatParams &params,
        TTrferPaxStat &TrferPaxStat,
        const TPrintAirline &prn_airline,
        xmlNodePtr resNode);

template <class T>
void RunTrferPaxStat(
        const TStatParams &params,
        T &TrferPaxStat,
        TPrintAirline &prn_airline
        );

void get_trfer_pax_stat(int point_id);

#endif
