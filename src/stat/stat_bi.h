#ifndef _BI_STAT_H_
#define _BI_STAT_H_

#include "stat_common.h"
#include "bi_rules.h"

struct TBIStatRow {
    TDateTime part_key;
    int point_id;
    TDateTime scd_out;
    int pax_id;
    BIPrintRules::TPrintType::Enum print_type;
    int terminal;
    int hall;
    ASTRA::TDevOper::Enum op_type;
    TBIStatRow():
        part_key(ASTRA::NoExists),
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
    TFltInfoCache flt_cache;
    virtual ~TBIAbstractStat() {};
    TBIAbstractStat(): FRowCount(0) {};
    virtual void add(const TBIStatRow &row) = 0;
    int FRowCount;
    size_t RowCount() { return FRowCount; }
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

struct TBIFullStat: public TBIAirlineMap, TBIAbstractStat  {
    TBIStatCounters totals;
    void add(const TBIStatRow &row);
};

typedef std::map<std::string, int> TBIDetailHallMap;
typedef std::map<std::string, TBIDetailHallMap> TBIDetailTerminalMap;
typedef std::map<std::string, TBIDetailTerminalMap> TBIDetailAirpMap;
typedef std::map<std::string, TBIDetailAirpMap> TBIDetailAirlineMap;

struct TBIDetailStat: public TBIDetailAirlineMap, TBIAbstractStat {
    int total;
    void add(const TBIStatRow &row);
    TBIDetailStat(): total(0) {}
};

typedef std::map<std::string, int> TBIShortAirpMap;
typedef std::map<std::string, TBIShortAirpMap> TBIShortAirlineMap;

struct TBIShortStat: public TBIShortAirlineMap, TBIAbstractStat {
    int total;
    void add(const TBIStatRow &row);
    TBIShortStat(): total(0) {}
};


void RunBIStat(
        const TStatParams &params,
        TBIAbstractStat &BIStat
        );

void createXMLBIFullStat(
        const TStatParams &params,
        const TBIFullStat &BIFullStat,
        xmlNodePtr resNode);

void createXMLBIDetailStat(
        const TStatParams &params,
        const TBIDetailStat &BIDetailStat,
        xmlNodePtr resNode);

void createXMLBIShortStat(
        const TStatParams &params,
        const TBIShortStat &BIShortStat,
        xmlNodePtr resNode);

void RunBIFullFile(const TStatParams &params, TOrderStatWriter &writer);
void RunBIDetailFile(const TStatParams &params, TOrderStatWriter &writer);
void RunBIShortFile(const TStatParams &params, TOrderStatWriter &writer);

#endif
