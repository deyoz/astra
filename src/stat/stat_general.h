#ifndef _STAT_GENERAL_H_
#define _STAT_GENERAL_H_

#include "stat_common.h"
#include "stat_utils.h"
#include "baggage_base.h"

struct TFullStatKey {
    std::string col1, col2;
    std::string airp; // в сортировке не участвует, нужен для AirpTZRegion
    int flt_no;
    TDateTime scd_out;
    int point_id;
    TStatPlaces places;
    TFullStatKey():
        flt_no(ASTRA::NoExists),
        scd_out(ASTRA::NoExists),
        point_id(ASTRA::NoExists)
    {}
    bool operator == (const TFullStatKey &item) const;
};

/* GRISHA */
struct TInetStat {
    int web;
    int kiosk;
    int mobile;
    int web_bp, kiosk_bp;   // Кол-во пассажиров из веб и киоска, которые распечатали посадочный на стойке
    int web_bag, kiosk_bag; //        ___,,____                 , которые зарег. багаж на стойке
    int mobile_bp, mobile_bag;

    TInetStat():
        web(0),
        kiosk(0),
        mobile(0),
        web_bp(0), kiosk_bp(0),
        web_bag(0), kiosk_bag(0),
        mobile_bp(0), mobile_bag(0)
    {}

    bool operator == (const TInetStat &item) const;
    void operator += (const TInetStat &item);
    void toXML(xmlNodePtr headerNode, xmlNodePtr rowNode);
};

struct TFullStatRow {
    int pax_amount;
    TInetStat i_stat;
    int adult;
    int child;
    int baby;
    int rk_weight;
    int bag_amount;
    int bag_weight;
    TBagKilos excess_wt;
    TBagPieces excess_pc;
    TFullStatRow():
        pax_amount(0),
        adult(0),
        child(0),
        baby(0),
        rk_weight(0),
        bag_amount(0),
        bag_weight(0),
        excess_wt(0),
        excess_pc(0)
    {}
    bool operator == (const TFullStatRow &item) const;
    void operator += (const TFullStatRow &item);
};

struct TFullCmp {
    bool operator() (const TFullStatKey &lr, const TFullStatKey &rr) const;
};

typedef std::map<TFullStatKey, TFullStatRow, TFullCmp> TFullStat;

struct TDetailStatKey {
    std::string pact_descr, col1, col2;
    bool operator == (const TDetailStatKey &item) const;
};

struct TDetailStatRow {
    int flt_amount, pax_amount;
    int f, c, y;
    TInetStat i_stat;
    std::set<int> flts;
    TDetailStatRow():
        flt_amount(0),
        pax_amount(0),
        f(0), c(0), y(0)
    {};
    bool operator == (const TDetailStatRow &item) const;
    void operator += (const TDetailStatRow &item);
};

struct TDetailCmp {
    bool operator() (const TDetailStatKey &lr, const TDetailStatKey &rr) const;
};

typedef std::map<TDetailStatKey, TDetailStatRow, TDetailCmp> TDetailStat;

void RunPactDetailStat(const TStatParams &params,
                       TDetailStat &DetailStat, TDetailStatRow &DetailStatTotal,
                       TPrintAirline &prn_airline, bool full = false);

void RunDetailStat(const TStatParams &params,
                   TDetailStat &DetailStat, TDetailStatRow &DetailStatTotal,
                   TPrintAirline &airline, bool full = false);

void RunFullStat(const TStatParams &params,
                 TFullStat &FullStat, TFullStatRow &FullStatTotal,
                 TPrintAirline &airline, bool full = false);

void RunDetailStatFile(const TStatParams &params, TOrderStatWriter &writer, TPrintAirline &prn_airline);
void RunFullStatFile(const TStatParams &params, TOrderStatWriter &writer, TPrintAirline &prn_airline);

void createXMLDetailStat(const TStatParams &params, bool pr_pact,
                         const TDetailStat &DetailStat, const TDetailStatRow &DetailStatTotal,
                         const TPrintAirline &airline, xmlNodePtr resNode);
void createXMLFullStat(const TStatParams &params,
                       const TFullStat &FullStat, const TFullStatRow &FullStatTotal,
                       const TPrintAirline &airline, xmlNodePtr resNode);

#endif
