#pragma once

#include "stat_common.h"
#include "zamar_dsm.h"

void set_stat_zamar(ZamarType type, const AirlineCode_t &airline, const AirportCode_t &airp, bool pr_ok);

struct TZamarStatRow {
    AirportCode_t airp;
    AirlineCode_t airline;
    int amount_ok;
    int amount_fault;
    TZamarStatRow(
            const AirportCode_t &_airp,
            const AirlineCode_t &_airline,
            int _amount_ok,
            int _amount_fault
            ):
        airp(_airp),
        airline(_airline),
        amount_ok(_amount_ok),
        amount_fault(_amount_fault)
    {}
};

struct TZamarAbstractStat {
    TPrintAirline prn_airline;
    virtual ~TZamarAbstractStat() {};
    TZamarAbstractStat(): FRowCount(0) {};
    virtual void add(const TZamarStatRow &row) = 0;
    int FRowCount;
    size_t RowCount() { return FRowCount; }
};

struct TZamarStatCounters {
    int amount_ok, amount_fault;
    bool empty() { return amount_ok == 0 and amount_fault == 0; }
    void add(const TZamarStatRow &row);
    TZamarStatCounters():
        amount_ok(0),
        amount_fault(0)
    {}
};

typedef std::map<std::string, TZamarStatCounters> TZamarAirpMap;
typedef std::map<std::string, TZamarAirpMap> TZamarArilineMap;

struct TZamarFullStat: public TZamarArilineMap, TZamarAbstractStat {
    TZamarStatCounters totals;
    void add(const TZamarStatRow &row);
};

void RunZamarStat(
        const TStatParams &params,
        TZamarAbstractStat &ZamarStat
        );

void createXMLZamarFullStat(
        const TStatParams &params,
        const TZamarFullStat &ZamarFullStat,
        xmlNodePtr resNode);

void RunZamarFullFile(const TStatParams &params, TOrderStatWriter &writer);
