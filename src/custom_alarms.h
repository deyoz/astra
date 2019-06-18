#ifndef _CUSTOM_ALARMS_H_
#define _CUSTOM_ALARMS_H_

#include <map>
#include <set>
#include <libxml/tree.h>
#include "serverlib/trace_signature.h"
#include "brands.h"

struct TCustomAlarms {
    private:

        struct TSets {
            TBrands &brands;
            typedef std::map<std::string, int> TFQTLvlMap;
            typedef std::map<std::string, TFQTLvlMap> TFQTAirlineMap;
            typedef std::map<std::string, TFQTAirlineMap> TBrandCodeMap;
            typedef std::map<std::string, TBrandCodeMap> TBrandAirlineMap;
            typedef std::map<std::string, TBrandAirlineMap> TRFISCMap;
            typedef std::map<std::string, boost::optional<TRFISCMap>> TAirlineMap;
            
            TAirlineMap items;

            void fromDB(const std::string &airline, int pax_id, std::set<int> &alarms);
            TSets(TBrands &_brands): brands(_brands) {};
        };

        TSets sets;
        TBrands brands;
        std::map<int, std::set<int>> items;

    public:

        const TCustomAlarms &getByGrpId(int pax_id);
        const TCustomAlarms &getByPaxId(int pax_id, bool pr_clear = true, const std::string &vairline = "");

        void toDB() const;
        void fromDB(int point_id);

        void toXML(xmlNodePtr paxNode, int pax_id);

        void trace(TRACE_SIGNATURE) const;

        void clear() { items.clear(); }
        TCustomAlarms(): sets(brands) {}
};

void init_rfisc_callbacks();
void init_fqt_callbacks();
void init_ticket_callbacks();


#endif
