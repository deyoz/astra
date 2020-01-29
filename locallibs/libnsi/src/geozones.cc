#include <chrono>
#include "nsi.h"
#include "callbacks.h"

namespace {

class ZoneCache
{
    using chrono_st = std::chrono::steady_clock;
    using Zones = std::set<nsi::GeozoneId>;
    using Data = std::pair<Zones, chrono_st::time_point>;

    using cache_data_t = std::tuple<
        std::map<nsi::CountryId, Data>,
        std::map<nsi::RegionId, Data>,
        std::map<nsi::CityId, Data>
    >;

    cache_data_t data_;

    static chrono_st::duration lifeTime()
    {
        return std::chrono::seconds(300);
    }

public:
    static ZoneCache& instance()
    {
        static ZoneCache v;
        return v;
    }

    void clear()
    {
        cache_data_t().swap(data_);
    }

    template <typename T>
    const Zones& getZones(const T& cid)
    {
        auto& data = std::get< typename std::map<T, Data> >(data_);
        auto i = data.find(cid);
        if (i == data.end()) {
            return data.emplace(
                cid,
                Data { nsi::callbacks().getGeozones(cid), chrono_st::now() }
            ).first->second.first;
        }

        Data& zd = i->second;
        if ((chrono_st::now() - zd.second) >= lifeTime()) {
            zd.first = nsi::callbacks().getGeozones(cid);
            zd.second = chrono_st::now();
        }
        return zd.first;
    }
};

} //anonymous

namespace nsi {

void clearCachedGeozoneRelations()
{
    ZoneCache::instance().clear();
}

std::set<GeozoneId> getGeozones(const CountryId& id)
{
    return ZoneCache::instance().getZones(id);
}

std::set<GeozoneId> getGeozones(const RegionId& id)
{
    return ZoneCache::instance().getZones(id);
}

std::set<GeozoneId> getGeozones(const CityId& id)
{
    return ZoneCache::instance().getZones(id);
}

} //nsi
