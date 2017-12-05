#ifndef _BI_STAT_H_
#define _BI_STAT_H_

#include "stat_common.h"
#include "bi_rules.h"

struct TBIStatRow {
    int point_id;
    TDateTime scd_out;
    int pax_id;
    BIPrintRules::TPrintType::Enum print_type;
    int terminal;
    int hall;
    ASTRA::TDevOper::Enum op_type;
    TBIStatRow():
        point_id(ASTRA::NoExists),
        scd_out(ASTRA::NoExists),
        pax_id(ASTRA::NoExists),
        print_type(BIPrintRules::TPrintType::None),
        terminal(ASTRA::NoExists),
        hall(ASTRA::NoExists),
        op_type(ASTRA::TDevOper::Unknown)
    {}
};

struct TBIAbstractStat {
    TPrintAirline prn_airline;
    virtual ~TBIAbstractStat() {};
    virtual void add(const TBIStatRow &row) = 0;
    virtual size_t RowCount() = 0;
};

struct TBIStatCounters {
    int all, two, one;
    int total() const { return all + two + one; }
    bool empty() { return all == 0 and two == 0 and one == 0; }
    void add(BIPrintRules::TPrintType::Enum print_type);
    TBIStatCounters():
        all(0),
        two(0),
        one(0)
    {}
};

typedef std::map<std::string, TBIStatCounters> TBIHallMap;
typedef std::map<std::string, TBIHallMap> TBITerminalMap;
typedef std::map<TDateTime, TBITerminalMap> TBIScdOutMap;
typedef std::map<std::string, TBIScdOutMap> TBIFltMap;
typedef std::map<std::string, TBIFltMap> TBIAirpMap;
typedef std::map<std::string, TBIAirpMap> TBIAirlineMap;

struct TFltInfoCacheItem {
    std::string airp;
    std::string airline;

    std::string view_airp;
    std::string view_airline;
    std::string view_flt_no;
};

struct TFltInfoCache:public std::map<int, TFltInfoCacheItem> {
    const TFltInfoCacheItem &get(int point_id);
};

struct TBIFullStat: public TBIAirlineMap, TBIAbstractStat  {
    TBIStatCounters totals;
    TFltInfoCache flt_cache;
    int FRowCount;
    void add(const TBIStatRow &row);
    size_t RowCount() { return FRowCount; }
    TBIFullStat(): FRowCount(0) {}
};

void RunBIStat(
        const TStatParams &params,
        TBIAbstractStat &BIStat,
        bool full = false
        );

void createXMLBIFullStat(
        const TStatParams &params,
        const TBIFullStat &BIFullStat,
        xmlNodePtr resNode);

void RunBIFullFile(const TStatParams &params, TOrderStatWriter &writer);

#endif
