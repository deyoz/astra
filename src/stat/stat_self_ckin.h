#ifndef _STAT_SELF_CKIN_H_
#define _STAT_SELF_CKIN_H_

#include "stat_common.h"
#include "stat_utils.h"

struct TSelfCkinStatRow {
    int pax_amount;
    int term_bp;
    int term_bag;
    int term_ckin_service;
    int adult;
    int child;
    int baby;
    int tckin;
    std::set<int> flts;
    TSelfCkinStatRow():
        pax_amount(0),
        term_bp(0),
        term_bag(0),
        term_ckin_service(0),
        adult(0),
        child(0),
        baby(0),
        tckin(0)
    {};
    bool operator == (const TSelfCkinStatRow &item) const;
    void operator += (const TSelfCkinStatRow &item);
};

struct TSelfCkinStatKey {
    std::string client_type, descr, ak, ap;
    std::string desk, desk_airp;
    int flt_no;
    int point_id;
    TStatPlaces places;
    TDateTime scd_out;
    TSelfCkinStatKey(): flt_no(ASTRA::NoExists) {};
};

struct TKioskCmp {
    bool operator() (const TSelfCkinStatKey &lr, const TSelfCkinStatKey &rr) const;
};

typedef std::map<TSelfCkinStatKey, TSelfCkinStatRow, TKioskCmp> TSelfCkinStat;

void RunSelfCkinStat(const TStatParams &params,
                  TSelfCkinStat &SelfCkinStat, TSelfCkinStatRow &SelfCkinStatTotal,
                  TPrintAirline &prn_airline, bool full = false);

void createXMLSelfCkinStat(const TStatParams &params,
                        const TSelfCkinStat &SelfCkinStat, const TSelfCkinStatRow &SelfCkinStatTotal,
                        const TPrintAirline &airline, xmlNodePtr resNode);

void RunSelfCkinStatFile(const TStatParams &params, TOrderStatWriter &writer, TPrintAirline &prn_airline);

int nosir_self_ckin(int argc,char **argv);

#endif
