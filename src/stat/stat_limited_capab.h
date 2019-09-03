#ifndef _STAT_LIMITED_CAPAB_H_
#define _STAT_LIMITED_CAPAB_H_

#include "stat_common.h"

struct TFlight {
    int point_id;
    std::string airline;
    std::string airp;
    int flt_no;
    std::string suffix;
    TDateTime scd_out;
    TFlight():
        flt_no(ASTRA::NoExists),
        scd_out(ASTRA::NoExists)
    {}
    bool operator < (const TFlight &val) const
    {
        return point_id < val.point_id;
    }

};

struct TLimitedCapabStat {
    typedef std::map<std::string, int> TRems;
    typedef std::map<std::string, TRems> TAirpArv;
    typedef std::map<TFlight, TAirpArv> TRows;
    TRems total;
    TRows rows;
};

void RunLimitedCapabStat(
        const TStatParams &params,
        TLimitedCapabStat &LimitedCapabStat,
        TPrintAirline &prn_airline
        );
void createXMLLimitedCapabStat(const TStatParams &params, const TLimitedCapabStat &LimitedCapabStat, const TPrintAirline &prn_airline, xmlNodePtr resNode);
void RunLimitedCapabStatFile(const TStatParams &params, TOrderStatWriter &writer, TPrintAirline &prn_airline);

int nosir_lim_capab_stat(int argc,char **argv);
void get_limited_capability_stat(int point_id);

#endif
