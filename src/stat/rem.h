#ifndef _STAT_REM_H_
#define _STAT_REM_H_

#include "common.h"

struct TRemStatRow:public TOrderStatItem {
    int point_id;
    std::string ticket_no;
    TDateTime scd_out;
    int flt_no;
    std::string suffix;
    std::string airp;
    std::string airp_last;
    std::string craft;
    TDateTime travel_time;
    std::string rem_code;
    std::string airline;
    std::string user;
    std::string desk;
    std::string rfisc;
    double rate;
    std::string rate_cur;

    std::string rate_str() const;

    TRemStatRow():
        point_id(ASTRA::NoExists),
        scd_out(ASTRA::NoExists),
        flt_no(ASTRA::NoExists),
        travel_time(ASTRA::NoExists),
        rate(ASTRA::NoExists)
    {}
    bool operator < (const TRemStatRow &val) const
    {
        if(point_id == val.point_id)
            return rem_code < val.rem_code;
        return point_id < val.point_id;
    }

    void add_header(std::ostringstream &buf) const;
    void add_data(std::ostringstream &buf) const;
};

typedef std::multiset<TRemStatRow> TRemStat;

template <class T>
void RunRemStat(
        const TStatParams &params,
        T &RemStat,
        TPrintAirline &prn_airline
        );
void createXMLRemStat(const TStatParams &params, const TRemStat &RemStat, const TPrintAirline &prn_airline, xmlNodePtr resNode);

void get_rem_stat(int point_id);

#endif
