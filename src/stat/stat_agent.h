#ifndef _STAT_AGENT_H_
#define _STAT_AGENT_H_

#include "stat_common.h"

namespace STAT {

    struct agent_stat_t {
        int inc, dec;
        agent_stat_t(int ainc, int adec): inc(ainc), dec(adec) {};
        bool operator == (const agent_stat_t &item) const
        {
            return inc == item.inc &&
                   dec == item.dec;
        };
        void operator += (const agent_stat_t &item)
        {
            inc += item.inc;
            dec += item.dec;
        };
    };
    void agent_stat_delta(
            int point_id,
            int user_id,
            const std::string &desk,
            TDateTime ondate,
            int pax_time,
            int pax_amount,
            agent_stat_t dpax_amount, // d prefix stands for delta
            agent_stat_t dtckin_amount,
            agent_stat_t dbag_amount,
            agent_stat_t dbag_weight,
            agent_stat_t drk_amount,
            agent_stat_t drk_weight
            );
    int agent_stat_delta(int argc,char **argv);
}

struct TAgentStatKey {
    int point_id;
    std::string airline_view;
    int flt_no;
    std::string suffix_view;
    std::string airp, airp_view;
    TDateTime scd_out, scd_out_local;
    std::string desk;
    int user_id;
    std::string user_descr;
    TAgentStatKey(): point_id(ASTRA::NoExists),
                     flt_no(ASTRA::NoExists),
                     scd_out(ASTRA::NoExists),
                     scd_out_local(ASTRA::NoExists),
                     user_id(ASTRA::NoExists) {};
};

struct TAgentStatRow {
    STAT::agent_stat_t dpax_amount;
    STAT::agent_stat_t dtckin_amount;
    STAT::agent_stat_t dbag_amount;
    STAT::agent_stat_t dbag_weight;
    STAT::agent_stat_t drk_amount;
    STAT::agent_stat_t drk_weight;
    int processed_pax;
    double time;
    TAgentStatRow():
        dpax_amount(0,0),
        dtckin_amount(0,0),
        dbag_amount(0,0),
        dbag_weight(0,0),
        drk_amount(0,0),
        drk_weight(0,0),
        processed_pax(0),
        time(0.0)
    {}
    bool operator == (const TAgentStatRow &item) const;
    void operator += (const TAgentStatRow &item);
};

struct TAgentCmp {
    bool operator() (const TAgentStatKey &lr, const TAgentStatKey &rr) const;
};

typedef std::map<TAgentStatKey, TAgentStatRow, TAgentCmp> TAgentStat;

void RunAgentStat(const TStatParams &params,
                  TAgentStat &AgentStat, TAgentStatRow &AgentStatTotal,
                  TPrintAirline &prn_airline, bool override_max_rows = false);
void createXMLAgentStat(const TStatParams &params,
                        const TAgentStat &AgentStat, const TAgentStatRow &AgentStatTotal,
                        const TPrintAirline &airline, xmlNodePtr resNode);
void RunAgentStatFile(const TStatParams &params, TOrderStatWriter &writer, TPrintAirline &prn_airline);

#endif
