#ifndef _STAT_SALON_H_
#define _STAT_SALON_H_

#include "stat_common.h"
#include "astra_locale_adv.h"

struct TSalonStatRow:public TOrderStatItem {
    int point_id;
    TDateTime scd_out;
    TDateTime time;
    std::string login;
    std::string msg;
    std::string airline;
    int flt_no;
    std::string suffix;

    void clear()
    {
        point_id = ASTRA::NoExists;
        scd_out = ASTRA::NoExists;
        time = ASTRA::NoExists;
        login.clear();
        msg.clear();
        airline.clear();
        flt_no = ASTRA::NoExists;
        suffix.clear();
    }

    TSalonStatRow() { clear(); }

    bool operator < (const TSalonStatRow &val) const
    {
        if(point_id != val.point_id)
            return point_id < val.point_id;
        if(time != val.time)
            return time < val.time;
        return login < val.login;
    }

    void add_header(std::ostringstream &buf) const;
    void add_data(std::ostringstream &buf) const;
};

typedef std::multiset<TSalonStatRow> TSalonStat;

template <class T>
void RunSalonStat(
        const TStatParams &params,
        T &SalonStat,
        TPrintAirline &prn_airline
        );
void createXMLSalonStat(const TStatParams &params, const TSalonStat &SalonStat, const TPrintAirline &prn_airline, xmlNodePtr resNode);

void to_stat_salon(int point_id, const PrmEnum &msg, const std::string &op_type);

#endif
