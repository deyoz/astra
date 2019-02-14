#ifndef _DOCS_SERVICES_H_
#define _DOCS_SERVICES_H_

#include "common.h"

enum EServiceSortOrder
{
    by_reg_no,
    by_family,
    by_seat_no,
    by_service_code
};

struct TServiceRow
{
    int pax_id;
    TPaidRFISCItem paid_rfisc_item;


    std::string seat_no;
    std::string family;
    int reg_no;
    std::string RFIC;
    std::string RFISC;
    std::string desc;
    std::string num;

    const EServiceSortOrder mSortOrder;

    bool operator < (const TServiceRow &other) const;
    void toXML(xmlNodePtr dataSetNode) const;

    void clear();

    TServiceRow(EServiceSortOrder sortOrder = by_reg_no): mSortOrder(sortOrder) { clear(); }
};

struct TServiceList: public std::list<TServiceRow> {
    const bool pr_stat;
    TServiceList(bool _pr_stat = false): pr_stat(_pr_stat) {}
    void fromDB(const TRptParams &rpt_params);
};

void SERVICES(TRptParams &rpt_params, xmlNodePtr reqNode, xmlNodePtr resNode);
void SERVICESTXT(TRptParams &rpt_params, xmlNodePtr reqNode, xmlNodePtr resNode);

#endif
