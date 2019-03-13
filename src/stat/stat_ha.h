#ifndef _STAT_HA_H_
#define _STAT_HA_H_

#include "stat_common.h"

struct THAStatRow {
    TDateTime part_key;
    int point_id;
    int hotel_id;
    int room_type;
    TDateTime scd_out;
    int adt;
    int chd;
    int inf;
    int total() const { return adt + chd + inf; }
    THAStatRow():
        part_key(ASTRA::NoExists),
        point_id(ASTRA::NoExists),
        hotel_id(ASTRA::NoExists),
        room_type(ASTRA::NoExists),
        scd_out(ASTRA::NoExists),
        adt(0),
        chd(0),
        inf(0)
    {}
};

struct THAAbstractStat {
    TPrintAirline prn_airline;
    TFltInfoCache flt_cache;
    virtual ~THAAbstractStat() {};
    THAAbstractStat(): FRowCount(0) {};
    virtual void add(const THAStatRow &row) = 0;
    int FRowCount;
    size_t RowCount() { return FRowCount; }
};

struct THAFullCounters {
    int room_single;
    int room_double;
    int adt;
    int chd;
    int inf;
    int total() const { return adt + chd + inf; }
    THAFullCounters():
        room_single(0),
        room_double(0),
        adt(0),
        chd(0),
        inf(0)
    {}
    THAFullCounters &operator +=(const THAStatRow &rhs);
};

typedef std::map<std::string, THAFullCounters> THAFullHotelMap;
typedef std::map<TDateTime, THAFullHotelMap> THAFullSCDMap;
typedef std::map<std::string, THAFullSCDMap> THAFullFltMap;
typedef std::map<std::string, THAFullFltMap> THAFullAirpMap;
typedef std::map<std::string, THAFullAirpMap> THAFullAirlineMap;

struct THAFullStat: public THAFullAirlineMap, THAAbstractStat {
    THAFullCounters total;
    void add(const THAStatRow &row);
};

typedef std::map<std::string, int> THAShortHotelMap;
typedef std::map<std::string, THAShortHotelMap> THAShortAirpMap;
typedef std::map<std::string, THAShortAirpMap> THAShortAirlineMap;

struct THAShortStat: public THAShortAirlineMap, THAAbstractStat {
    int total;
    void add(const THAStatRow &row);
    THAShortStat(): total(0) {}
};

void RunHAStat(
        const TStatParams &params,
        THAAbstractStat &HAStat,
        bool full = false
        );

void createXMLHAFullStat(
        const TStatParams &params,
        const THAFullStat &HAFullStat,
        xmlNodePtr resNode);

void createXMLHAShortStat(
        const TStatParams &params,
        const THAShortStat &HAShortStat,
        xmlNodePtr resNode);

void RunHAFullFile(const TStatParams &params, TOrderStatWriter &writer);
void RunHAShortFile(const TStatParams &params, TOrderStatWriter &writer);

void get_stat_ha(int point_id);

#endif
