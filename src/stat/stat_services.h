#ifndef _STAT_SERVICES_H_
#define _STAT_SERVICES_H_

#include "stat/stat_common.h"

void get_stat_services(int point_id);

struct TServicesStatRow {
    TDateTime part_key;
    TDateTime scd_out;
    int point_id;
    int pax_id;
    std::string airp;
    std::string airline;
    std::string view_airline;
    std::string flt_no;
    std::string ticket_no;
    std::string full_name;
    std::string airp_dep;
    std::string airp_arv;
    std::string RFIC;
    std::string RFISC;
    std::string receipt_no;

    void clear()
    {
        part_key = ASTRA::NoExists;
        scd_out = ASTRA::NoExists;
        point_id = ASTRA::NoExists;
        pax_id = ASTRA::NoExists;
        airp.clear();
        airline.clear();
        view_airline.clear();
        flt_no.clear();
        ticket_no.clear();
        full_name.clear();
        airp_dep.clear();
        airp_arv.clear();
        RFIC.clear();
        RFISC.clear();
        receipt_no.clear();
    }

    bool operator < (const TServicesStatRow &val) const
    {
        if(point_id == val.point_id)
            return full_name < val.full_name;
        return point_id < val.point_id;
    }

    TServicesStatRow() { clear(); }
};

struct TServicesAbstractStat {
    TPrintAirline prn_airline;
    TFltInfoCache flt_cache;
    virtual ~TServicesAbstractStat() {};
    TServicesAbstractStat(): FRowCount(0) {};
    virtual void add(const TServicesStatRow &row) = 0;
    int FRowCount;
    size_t RowCount() const { return FRowCount; }
};

struct TServicesFullData: std::multiset<TServicesStatRow> {
};

typedef std::map<TDateTime, TServicesFullData> TServicesFullSCDMap;
typedef std::map<std::string, TServicesFullSCDMap> TServicesFullFltMap;
typedef std::map<std::string, TServicesFullFltMap> TServicesFullAirlineMap;
typedef std::map<std::string, TServicesFullAirlineMap> TServicesFullAirpMap;

struct TServicesFullStat: public TServicesFullAirpMap, TServicesAbstractStat  {
    void add(const TServicesStatRow &row);
};

typedef std::map<std::string, int> TServicesDetailRFISCMap;
typedef std::map<std::string, TServicesDetailRFISCMap> TServicesDetailRFICMap;
typedef std::map<TDateTime, TServicesDetailRFICMap> TServicesDetailSCDMap;
typedef std::map<std::string, TServicesDetailSCDMap> TServicesDetailFltMap;
typedef std::map<std::string, TServicesDetailFltMap> TServicesDetailAirlineMap;
typedef std::map<std::string, TServicesDetailAirlineMap> TServicesDetailAirpMap;

struct TServicesDetailStat: public TServicesDetailAirpMap, TServicesAbstractStat  {
    void add(const TServicesStatRow &row);
};

typedef std::map<std::string, int> TServicesShortRFISCMap;
typedef std::map<std::string, TServicesShortRFISCMap> TServicesShortRFICMap;
typedef std::map<std::string, TServicesShortRFICMap> TServicesShortAirlineMap;
typedef std::map<std::string, TServicesShortAirlineMap> TServicesShortAirpMap;

struct TServicesShortStat: public TServicesShortAirpMap, TServicesAbstractStat  {
    void add(const TServicesStatRow &row);
};

void RunServicesStat(
        const TStatParams &params,
        TServicesAbstractStat &ServicesStat,
        bool full = false
        );

void createXMLServicesFullStat(
        const TStatParams &params,
        const TServicesFullStat &ServicesFullStat,
        xmlNodePtr resNode);

void createXMLServicesDetailStat(
        const TStatParams &params,
        const TServicesDetailStat &ServicesDetailStat,
        xmlNodePtr resNode);

void createXMLServicesShortStat(
        const TStatParams &params,
        const TServicesShortStat &ServicesShortStat,
        xmlNodePtr resNode);

void RunServicesFullFile(const TStatParams &params, TOrderStatWriter &writer);
void RunServicesDetailFile(const TStatParams &params, TOrderStatWriter &writer);
void RunServicesShortFile(const TStatParams &params, TOrderStatWriter &writer);

#endif
