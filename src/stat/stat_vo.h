#ifndef _STAT_VO_H_
#define _STAT_VO_H_

#include "stat_common.h"

struct TVOStatRow {
    TDateTime part_key;
    int point_id;
    std::string voucher;
    TDateTime scd_out;
    int amount;
    TVOStatRow():
        part_key(ASTRA::NoExists),
        point_id(ASTRA::NoExists),
        scd_out(ASTRA::NoExists),
        amount(ASTRA::NoExists)
    {}
};

struct TVOAbstractStat {
    TPrintAirline prn_airline;
    TFltInfoCache flt_cache;
    virtual ~TVOAbstractStat() {};
    TVOAbstractStat(): FRowCount(0) {};
    virtual void add(const TVOStatRow &row) = 0;
    int FRowCount;
    size_t RowCount() { return FRowCount; }
};

typedef std::map<std::string, int> TVOFullVOMap;
typedef std::map<TDateTime, TVOFullVOMap> TVOFullSCDMap;
typedef std::map<std::string, TVOFullSCDMap> TVOFullFltMap;
typedef std::map<std::string, TVOFullFltMap> TVOFullAirpMap;
typedef std::map<std::string, TVOFullAirpMap> TVOFullAirlineMap;

struct TVOFullStat: public TVOFullAirlineMap, TVOAbstractStat {
    int total;
    void add(const TVOStatRow &row);
    TVOFullStat(): total(0) {}
};

typedef std::map<std::string, int> TVOShortVOMap;
typedef std::map<std::string, TVOShortVOMap> TVOShortAirpMap;
typedef std::map<std::string, TVOShortAirpMap> TVOShortAirlineMap;

struct TVOShortStat: public TVOShortAirlineMap, TVOAbstractStat {
    int total;
    void add(const TVOStatRow &row);
    TVOShortStat(): total(0) {}
};

void RunVOStat(
        const TStatParams &params,
        TVOAbstractStat &VOStat
        );

void createXMLVOFullStat(
        const TStatParams &params,
        const TVOFullStat &VOFullStat,
        xmlNodePtr resNode);

void createXMLVOShortStat(
        const TStatParams &params,
        const TVOShortStat &VOShortStat,
        xmlNodePtr resNode);

void RunVOFullFile(const TStatParams &params, TOrderStatWriter &writer);
void RunVOShortFile(const TStatParams &params, TOrderStatWriter &writer);

void get_stat_vo(int point_id);

#endif
