#ifndef _CUSTOM_ALARMS_H_
#define _CUSTOM_ALARMS_H_

#include <map>
#include <vector>
#include <libxml/tree.h>

struct TCustomAlarms {
    std::map<int, std::vector<int>> items;

    const TCustomAlarms &get(int pax_id);
    void toDB() const;
    void fromDB(int point_id);
    void toXML(xmlNodePtr paxNode, int pax_id);
    void clear() { items.clear(); }
};

void init_custom_alarm_callbacks();


#endif
