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

            struct TRow {
                std::string
                    rfisc,
                    rfisc_tlg,
                    brand_airline,
                    brand_code,
                    fqt_airline,
                    fqt_tier_level;
                int alarm;

                bool operator < (const TRow &val) const;
                std::string str() const;
                size_t not_empty_amount() const;

                TRow(
                    const std::string &_rfisc,
                    const std::string &_rfisc_tlg,
                    const std::string &_brand_airline,
                    const std::string &_brand_code,
                    const std::string &_fqt_airline,
                    const std::string &_fqt_tier_level,
                    int _alarm
                    ):
                    rfisc(_rfisc),
                    rfisc_tlg(_rfisc_tlg),
                    brand_airline(_brand_airline),
                    brand_code(_brand_code),
                    fqt_airline(_fqt_airline),
                    fqt_tier_level(_fqt_tier_level),
                    alarm(_alarm)
                {}
            };

            typedef std::set<TRow> TRowList;
            typedef std::map<std::string, boost::optional<TRowList>> TAirlineMap;

            TAirlineMap items;

            bool get(const std::string &airline);
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
        void fromDB(bool all, int id);

        void toXML(xmlNodePtr paxNode, int pax_id);

        void trace(TRACE_SIGNATURE) const;

        void clear() { items.clear(); }
        TCustomAlarms(): sets(brands) {}
};

void init_rfisc_callbacks();
void init_fqt_callbacks();
void init_ticket_callbacks();
void init_asvc_callbacks();


#endif
