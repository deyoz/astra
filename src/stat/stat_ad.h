#ifndef _STAT_AD_H_
#define _STAT_AD_H_

#include "stat_common.h"

struct TADStatRow {
    TDateTime part_key;
    TDateTime scd_out;
    int point_id;
    int pax_id;
    std::string pnr;
    std::string cls;
    ASTRA::TClientType client_type;
    int bag_amount;
    int bag_weight;
    std::string seat_no;
    std::string seat_no_lat;
    std::string desk;
    std::string station;
    TADStatRow()
    {
        clear();
    }
    void clear()
    {
        part_key = ASTRA::NoExists;
        scd_out = ASTRA::NoExists;
        point_id = ASTRA::NoExists;
        pax_id = ASTRA::NoExists;
        pnr.clear();
        cls.clear();
        client_type = ASTRA::ctTypeNum;
        bag_amount = ASTRA::NoExists;
        bag_weight = ASTRA::NoExists;
        seat_no.clear();
        seat_no_lat.clear();
        desk.clear();
        station.clear();
    }
};

struct TADAbstractStat {
    TPrintAirline prn_airline;
    TFltInfoCache flt_cache;
    virtual ~TADAbstractStat() {};
    TADAbstractStat(): FRowCount(0) {};
    virtual void add(const TADStatRow &row) = 0;
    int FRowCount;
    size_t RowCount() const { return FRowCount; }
};

struct TADFullData {
    std::string pnr;
    ASTRA::TPerson pers_type;
    std::string cls;
    ASTRA::TClientType client_type;
    std::string baggage;
    std::string gate;
    std::string seat_no;
    TADFullData() {
        clear();
    }
    void clear() {
        pnr.clear();
        pers_type = ASTRA::NoPerson;
        cls.clear();
        client_type = ASTRA::ctTypeNum;
        baggage.clear();
        gate.clear();
        seat_no.clear();
    }
};

typedef std::map<int, TADFullData> TADFullPaxIdMap;
typedef std::map<std::string, TADFullPaxIdMap> TADFullNameMap;
typedef std::map<TDateTime, TADFullNameMap> TADFullSCDMap;
typedef std::map<std::string, TADFullSCDMap> TADFullFltMap;
typedef std::map<std::string, TADFullFltMap> TADFullAirpMap;
typedef std::map<std::string, TADFullAirpMap> TADFullAirlineMap;

struct TADFullStat: public TADFullAirlineMap, TADAbstractStat {
    int bag_amount;
    int bag_weight;
    std::string baggage() const;
    void add(const TADStatRow &row);
    TADFullStat():
        bag_amount(0),
        bag_weight(0)
    {}
};

void RunADStat(
        const TStatParams &params,
        TADAbstractStat &ADStat,
        bool full = false
        );

void createXMLADFullStat(
        const TStatParams &params,
        const TADFullStat &ADFullStat,
        xmlNodePtr resNode);

void RunADFullFile(const TStatParams &params, TOrderStatWriter &writer);

void get_stat_ad(int point_id);

#endif
