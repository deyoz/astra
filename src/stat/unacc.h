#ifndef _STAT_UNACC_H_
#define _STAT_UNACC_H_

#include "common.h"

struct TUNACCStatRow: public TOrderStatItem
{
    int point_id;
    std::string craft;
    std::string airline;
    int flt_no;
    std::string suffix;
    TDateTime scd_out;
    std::string airp;
    std::string airp_arv;

    std::string original_tag_no;
    std::string surname;
    std::string name;
    std::string prev_airline;
    int prev_flt_no;
    std::string prev_suffix;
    TDateTime prev_scd;

    int grp_id;
    std::string descr;
    std::string desk;
    TDateTime time_create;
    int bag_type;
    int num;
    int amount;
    int weight;
    std::string view_weight;
    double no;

    std::string trfer_airline;
    std::string trfer_airp_dep;
    int trfer_flt_no;
    std::string trfer_suffix;
    TDateTime trfer_scd;
    std::string trfer_airp_arv;

    void add_header(std::ostringstream &buf) const;
    void add_data(std::ostringstream &buf) const;

    bool operator < (const TUNACCStatRow &_row) const;

    TUNACCStatRow():
        point_id(ASTRA::NoExists),
        flt_no(ASTRA::NoExists),
        scd_out(ASTRA::NoExists),
        prev_flt_no(ASTRA::NoExists),
        prev_scd(ASTRA::NoExists),
        grp_id(ASTRA::NoExists),
        time_create(ASTRA::NoExists),
        bag_type(ASTRA::NoExists),
        num(ASTRA::NoExists),
        amount(ASTRA::NoExists),
        weight(ASTRA::NoExists),
        no(ASTRA::NoExists),
        trfer_flt_no(ASTRA::NoExists)
    {}
};

struct TUNACCAbstractStat {
    TPrintAirline prn_airline;
    virtual ~TUNACCAbstractStat() {};
    TUNACCAbstractStat(): FRowCount(0) {};
    virtual void add(const TUNACCStatRow &row) = 0;
    int FRowCount;
    size_t RowCount() { return FRowCount; }
};

typedef std::set<TUNACCStatRow> TUNACCSet;

struct TUNACCFullStat: public TUNACCSet, TUNACCAbstractStat {
    std::string getBagWeight(TUNACCFullStat::const_iterator &row) const;
    void add(const TUNACCStatRow &row);
};

void RunUNACCStat(
        const TStatParams &params,
        TUNACCAbstractStat &UNACCStat,
        bool full = false
        );

void createXMLUNACCFullStat(
        const TStatParams &params,
        const TUNACCFullStat &UNACCFullStat,
        xmlNodePtr resNode);

void RunUNACCFullFile(const TStatParams &params, TOrderStatWriter &writer);

#endif
