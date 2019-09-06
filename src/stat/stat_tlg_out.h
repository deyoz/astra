#ifndef _STAT_TLG_OUT_H_
#define _STAT_TLG_OUT_H_

#include "stat_common.h"

struct TTlgOutStatKey {
    std::string sender_sita_addr;
    std::string receiver_descr;
    std::string receiver_sita_addr;
    std::string receiver_country_view;
    TDateTime time_send;
    std::string airline_view;
    int flt_no;
    std::string suffix_view;
    std::string airp_dep_view;
    TDateTime scd_local_date;
    std::string tlg_type;
    std::string airline_mark_view;
    std::string extra;
    TTlgOutStatKey():time_send(ASTRA::NoExists),
                     flt_no(ASTRA::NoExists),
                     scd_local_date(ASTRA::NoExists) {};
};

struct TTlgOutStatRow {
    int tlg_count;
    double tlg_len;
    TTlgOutStatRow():tlg_count(0),
                     tlg_len(0.0) {};
    bool operator == (const TTlgOutStatRow &item) const;
    void operator += (const TTlgOutStatRow &item);
};

struct TTlgOutStatCmp {
    bool operator() (const TTlgOutStatKey &key1, const TTlgOutStatKey &key2) const;
};

typedef std::map<TTlgOutStatKey, TTlgOutStatRow, TTlgOutStatCmp> TTlgOutStat;

void RunTlgOutStat(const TStatParams &params,
                   TTlgOutStat &TlgOutStat, TTlgOutStatRow &TlgOutStatTotal,
                   TPrintAirline &prn_airline);

void createXMLTlgOutStat(const TStatParams &params,
                         const TTlgOutStat &TlgOutStat, const TTlgOutStatRow &TlgOutStatTotal,
                         const TPrintAirline &airline, xmlNodePtr resNode);

void RunTlgOutStatFile(const TStatParams &params, TOrderStatWriter &writer, TPrintAirline &prn_airline);

#endif
