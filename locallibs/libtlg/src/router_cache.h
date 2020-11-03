#ifndef LIBTLG_ROUTER_CACHE_H
#define LIBTLG_ROUTER_CACHE_H

#include <list>
#include <map>
#include "telegrams.h"



template<typename CachedRoutersType, typename Manager>
void reinitAirsrvRoutersCache(const std::list<telegrams::RouterInfo>& routers, CachedRoutersType& cachedRouters, const Manager& mng)
{
    const CachedRoutersType oldCachedRouters(cachedRouters);
    cachedRouters.clear();
    for(const telegrams::RouterInfo& ri:  routers) {
        typename CachedRoutersType::mapped_type cr;
        typename CachedRoutersType::const_iterator it = mng.find(oldCachedRouters, ri);
        if (it == oldCachedRouters.end()) {
            cr.ri = ri;
        } else {
            cr.msg_map = it->second.msg_map;
            cr.ri = ri;
        }
        mng.insert(cachedRouters, ri, cr);
    }
}

#endif /* LIBTLG_ROUTER_CACHE_H */

