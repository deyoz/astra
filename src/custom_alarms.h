#ifndef _CUSTOM_ALARMS_H_
#define _CUSTOM_ALARMS_H_

#include <map>
#include <set>
#include <libxml/tree.h>
#include "serverlib/trace_signature.h"

struct TCustomAlarms {
    std::map<int, std::set<int>> items;

    const TCustomAlarms &getByGrpId(int pax_id);
    const TCustomAlarms &getByPaxId(int pax_id, bool pr_clear = true, const std::string &vairline = "");

    void toDB() const;
    void fromDB(int point_id);

    void toXML(xmlNodePtr paxNode, int pax_id);

    void trace(TRACE_SIGNATURE) const;

    void clear() { items.clear(); }
};

void init_rfisc_callbacks();
void init_fqt_callbacks();
void init_ticket_callbacks();


#endif
