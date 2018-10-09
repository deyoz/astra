#ifndef _STAT_REPRINT_H_
#define _STAT_REPRINT_H_

#include "stat_common.h"

void get_stat_reprint(int point_id);

struct TReprintStatRow {
    std::string desk;
    std::string airline;
    std::string view_airline;
    std::string flt;
    TDateTime scd_out;
    std::string route;
    std::string ckin_type;
    int amount;

    void clear()
    {
        desk.clear();
        airline.clear();
        view_airline.clear();
        flt.clear();
        scd_out = ASTRA::NoExists;
        route.clear();
        ckin_type.clear();
        amount = 0;
    }

    TReprintStatRow() { clear(); }
};

struct TReprintAbstractStat {
    TPrintAirline prn_airline;
    virtual ~TReprintAbstractStat() {};
    TReprintAbstractStat(): FRowCount(0) {};
    virtual void add(const TReprintStatRow &row) = 0;
    int FRowCount;
    size_t RowCount() { return FRowCount; }
};

typedef std::map<std::string, int> TReprintCkinTypeMap;
typedef std::map<std::string, TReprintCkinTypeMap> TReprintRouteMap;
typedef std::map<TDateTime, TReprintRouteMap> TReprintScdOutMap;
typedef std::map<std::string, TReprintScdOutMap> TReprintFltMap;
typedef std::map<std::string, TReprintFltMap> TReprintAirlineMap;
typedef std::map<std::string, TReprintAirlineMap> TReprintDeskMap;

struct TReprintFullStat: public TReprintDeskMap, TReprintAbstractStat  {
    int totals;
    void add(const TReprintStatRow &row);
    void clear() { totals = 0; }
    TReprintFullStat() { clear(); }
};

typedef std::map<std::string, int> TReprintShortAirlineMap;
typedef std::map<std::string, TReprintShortAirlineMap> TReprintShortDeskMap;

struct TReprintShortStat: public TReprintShortDeskMap, TReprintAbstractStat  {
    int totals;
    void add(const TReprintStatRow &row);
    void clear() { totals = 0; }
    TReprintShortStat() { clear(); }
};

void RunReprintStat(
        const TStatParams &params,
        TReprintAbstractStat &ReprintStat,
        bool full = false
        );

void createXMLReprintFullStat(
        const TStatParams &params,
        const TReprintFullStat &ReprintFullStat,
        xmlNodePtr resNode);

void createXMLReprintShortStat(
        const TStatParams &params,
        const TReprintShortStat &ReprintShortStat,
        xmlNodePtr resNode);

void RunReprintFullFile(const TStatParams &params, TOrderStatWriter &writer);
void RunReprintShortFile(const TStatParams &params, TOrderStatWriter &writer);

#endif
