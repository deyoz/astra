#ifndef _STAT_SALON_H_
#define _STAT_SALON_H_

#include "stat_common.h"
#include "astra_locale_adv.h"

struct TSalonStatRow {
    int point_id;
    int ev_order;
    TDateTime scd_out;
    TDateTime time;
    std::string login;
    std::string op_type;
    std::string msg;
    std::string airline;
    int flt_no;
    std::string suffix;

    void clear()
    {
        point_id = ASTRA::NoExists;
        ev_order = ASTRA::NoExists;
        scd_out = ASTRA::NoExists;
        time = ASTRA::NoExists;
        login.clear();
        op_type.clear();
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
        return ev_order < val.ev_order;
    }
};

typedef std::multiset<TSalonStatRow> TSalonStat;

void RunSalonStat(
        const TStatParams &params,
        TSalonStat &SalonStat
        );
void createXMLSalonStat(const TStatParams &params, const TSalonStat &SalonStat, xmlNodePtr resNode);

void RunSalonStatFile(const TStatParams &params, TOrderStatWriter &writer);

#endif
