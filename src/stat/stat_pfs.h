#ifndef _STAT_PFS_H_
#define _STAT_PFS_H_

#include "stat_common.h"

struct TPFSStatRow {
    int point_id;
    int pax_id;
    TDateTime scd_out;
    std::string flt;
    std::string status;
    std::string route;
    int seats;
    std::string subcls;
    std::string pnr;
    std::string surname;
    std::string name;
    std::string gender;
    TDateTime birth_date;
    TPFSStatRow():
        point_id(ASTRA::NoExists),
        pax_id(ASTRA::NoExists),
        scd_out(ASTRA::NoExists),
        seats(ASTRA::NoExists),
        birth_date(ASTRA::NoExists)
    {}

    bool operator < (const TPFSStatRow &val) const;
};

struct TPFSAbstractStat {
    virtual ~TPFSAbstractStat() {};
    virtual void add(const TPFSStatRow &row) = 0;
    virtual size_t RowCount() = 0;
    virtual void dump() = 0;
};

struct TPFSStat: public  std::multiset<TPFSStatRow>, TPFSAbstractStat {
    void dump() {};
    void add(const TPFSStatRow &row)
    {
        this->insert(row);
    }
    size_t RowCount()
    {
        return this->size();
    }
};

typedef std::map<std::string, int> TPFSStatusMap;
typedef std::map<std::string, TPFSStatusMap> TPFSRouteMap;
typedef std::map<std::string, TPFSRouteMap> TPFSFltMap;
typedef std::map<TDateTime, TPFSFltMap> TPFSScdOutMap;

struct TPFSShortStat: public TPFSScdOutMap, TPFSAbstractStat {
    int FRowCount;
    void dump();
    void add(const TPFSStatRow &row);
    size_t RowCount() { return FRowCount; }
    TPFSShortStat(): FRowCount(0) {}
};

void RunPFSStat(
        const TStatParams &params,
        TPFSAbstractStat &PFSStat,
        TPrintAirline &prn_airline
        );

void createXMLPFSStat(
        const TStatParams &params,
        const TPFSStat &PFSStat,
        const TPrintAirline &prn_airline,
        xmlNodePtr resNode);

void createXMLPFSShortStat(
        const TStatParams &params,
        TPFSShortStat &PFSShortStat,
        const TPrintAirline &prn_airline,
        xmlNodePtr resNode);

void RunPFSShortFile(const TStatParams &params, TOrderStatWriter &writer, TPrintAirline &prn_airline);
void RunPFSFullFile(const TStatParams &params, TOrderStatWriter &writer, TPrintAirline &prn_airline);

#endif
