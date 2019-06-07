#ifndef _CUSTOM_ALARMS_H_
#define _CUSTOM_ALARMS_H_

#include <map>
#include <vector>
#include <libxml/tree.h>

struct TCustomAlarms {
    std::map<int, std::vector<int>> items;

    const TCustomAlarms &getByGrpId(int pax_id);
    const TCustomAlarms &getByPaxId(int pax_id, bool pr_clear = true, const std::string &vairline = "");

    void toDB() const;
    void fromDB(int point_id);

    void toXML(xmlNodePtr paxNode, int pax_id);

    void clear() { items.clear(); }
};

void init_custom_alarm_callbacks();


#endif
