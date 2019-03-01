#ifndef _STAT_ANNUL_BT_H_
#define _STAT_ANNUL_BT_H_

#include <vector>
#include "docs/common.h"
#include "stat/common.h"

struct TAnnulBTStatRow {
    std::vector<t_tag_nos_row> tags;
    std::string airline;
    std::string airp;
    int flt_no;
    std::string suffix;
    int id;
    std::string airp_dep;
    std::string airp_arv;
    std::string full_name;
    int pax_id;
    int point_id;
    int bag_type;
    std::string rfisc;
    TDateTime time_create;
    TDateTime time_annul;
    int amount;
    int weight;
    int user_id;
    std::string agent;
    std::string trfer_airline;
    int trfer_flt_no;
    std::string trfer_suffix;
    TDateTime trfer_scd;
    std::string trfer_airp_arv;
    void get_tags(TDateTime part_key, int id);
    TAnnulBTStatRow():
        flt_no(ASTRA::NoExists),
        id(ASTRA::NoExists),
        pax_id(ASTRA::NoExists),
        point_id(ASTRA::NoExists),
        bag_type(ASTRA::NoExists),
        time_create(ASTRA::NoExists),
        time_annul(ASTRA::NoExists),
        amount(ASTRA::NoExists),
        weight(ASTRA::NoExists),
        user_id(ASTRA::NoExists),
        trfer_flt_no(ASTRA::NoExists),
        trfer_scd(ASTRA::NoExists)
    {}

};

struct TAnnulBTStat {
    std::list<TAnnulBTStatRow> rows;
};

void RunAnnulBTStat(
        const TStatParams &params,
        TAnnulBTStat &AnnulBTStat,
        TPrintAirline &prn_airline,
        int point_id = ASTRA::NoExists
        );

// used in ANNUL_TAGS report
void RunAnnulBTStat(TAnnulBTStat &AnnulBTStat, int point_id);

struct TAnnulBTStatCombo : public TOrderStatItem
{
    TAnnulBTStatRow data;
    TAnnulBTStatCombo(const TAnnulBTStatRow &aData): data(aData) {}
    void add_header(std::ostringstream &buf) const;
    void add_data(std::ostringstream &buf) const;
};

template <class T>
void RunAnnulBTStatFile(const TStatParams &params, T &writer, TPrintAirline &prn_airline)
{
    TAnnulBTStat AnnulBTStat;
    RunAnnulBTStat(params, AnnulBTStat, prn_airline);
    for (std::list<TAnnulBTStatRow>::const_iterator i = AnnulBTStat.rows.begin(); i != AnnulBTStat.rows.end(); ++i)
        writer.insert(TAnnulBTStatCombo(*i));
}

void createXMLAnnulBTStat(
        const TStatParams &params,
        const TAnnulBTStat &AnnulBTStat,
        const TPrintAirline &prn_airline,
        xmlNodePtr resNode);

#endif
