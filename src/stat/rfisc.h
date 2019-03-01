#ifndef _STAT_RFISC_H_
#define _STAT_RFISC_H_

#include "stat/common.h"

void get_rfisc_stat(int point_id);

struct TRFISCStatRow:public TOrderStatItem {
    std::string rfisc;
    int point_id;
    int point_num;
    int pr_trfer;
    TDateTime scd_out;
    int flt_no;
    std::string suffix;
    std::string airp;
    std::string airp_arv;
    std::string craft;
    TDateTime travel_time;
    int trfer_flt_no;
    std::string trfer_suffix;
    std::string trfer_airp_arv;
    std::string desk;
    std::string user_login;
    std::string user_descr;
    TDateTime time_create;
    double tag_no;
    std::string fqt_no;
    int excess;
    int paid;

    TRFISCStatRow():
        point_id(ASTRA::NoExists),
        point_num(ASTRA::NoExists),
        pr_trfer(ASTRA::NoExists),
        scd_out(ASTRA::NoExists),
        flt_no(ASTRA::NoExists),
        travel_time(ASTRA::NoExists),
        trfer_flt_no(ASTRA::NoExists),
        time_create(ASTRA::NoExists),
        tag_no(ASTRA::NoExists),
        excess(ASTRA::NoExists),
        paid(ASTRA::NoExists)
    {}

    bool operator < (const TRFISCStatRow &val) const
    {
        if(point_id != val.point_id)
            return point_id < val.point_id;
        if(point_num != val.point_num)
            return point_num < val.point_num;
        if(pr_trfer != val.pr_trfer)
            return pr_trfer > val.pr_trfer;
        if(trfer_airp_arv != val.trfer_airp_arv)
            return trfer_airp_arv < val.trfer_airp_arv;
        if(rfisc != val.rfisc)
            return rfisc < val.rfisc;
        return tag_no < val.tag_no;
    }

    void add_header(std::ostringstream &buf) const;
    void add_data(std::ostringstream &buf) const;
};

typedef std::multiset<TRFISCStatRow> TRFISCStat;

template <class T>
void RunRFISCStat(
        const TStatParams &params,
        T &RFISCStat,
        TPrintAirline &prn_airline
        );
void createXMLRFISCStat(const TStatParams &params, const TRFISCStat &RFISCStat, const TPrintAirline &prn_airline, xmlNodePtr resNode);

#endif
