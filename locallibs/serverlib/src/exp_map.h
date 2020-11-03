#ifndef _SERVERLIB_EXP_MAP_H
#define _SERVERLIB_EXP_MAP_H

#include <map>
#include <set>
#include <cassert>

/*
 * Like std::map, but with limited life time of each element.
 * K = key type
 * V = value type
 * T = time type with < operator, e.g. boost::posix_time::ptime
 *
 * IsExpired function determines life time of elements.
 */
template<class K, class V, class T, bool IsExpired(const T&, const T&)>
class MapWithExpiration {
public:
    struct VT {
        V v;
        T t;
        VT(const V& v, const T& t): v(v), t(t) {}
    };

    typedef std::map<K, VT> KMap;
    typedef typename KMap::const_iterator const_iterator;

    void removeExpired(const T& t)
    {
        /* tm_ elements are ordered by time (key), so we can stop on first !IsExpired */
        while (!tm_.empty() && IsExpired(tm_.begin()->first, t)) {
            const std::set<K>& keys = tm_.begin()->second;
            assert(!keys.empty());
            for (typename std::set<K>::const_iterator keyIt = keys.begin(); keyIt != keys.end(); ++keyIt)
                km_.erase(*keyIt);
            tm_.erase(tm_.begin());
        }
    }

    void update(const K& k, const V& v, const T& t)
    {
        removeExpired(t);

        typename KMap::iterator kmIt = km_.find(k);
        if (kmIt != km_.end()) {
            /* find entry in time map by old time */
            typename TMap::iterator tmIt = tm_.find(kmIt->second.t);
            assert(tmIt != tm_.end());
            /* find key in the key set for old time */
            typename std::set<K>::iterator keyIt = tmIt->second.find(k);
            assert(keyIt != tmIt->second.end());
            /* remove key from the key set, then check for empty key set */
            tmIt->second.erase(keyIt);
            if (tmIt->second.empty())
                tm_.erase(tmIt);

            /* set new value and time for this key */
            kmIt->second = VT(v, t);
        } else
            km_.insert(typename KMap::value_type(k, VT(v, t)));

        /* create or update entry in the time map */
        tm_[t].insert(k);

        assert(tm_.size() <= km_.size());
    }

    const_iterator find(const K& k, const T& t) const
    {
        typename KMap::const_iterator kmIt = km_.find(k);
        return (kmIt == km_.end() || IsExpired(kmIt->second.t, t)) ?
            km_.end() : kmIt;
    }

    bool empty() const { return km_.empty(); }
    std::size_t size() const { return km_.size(); }
    /* non-const iterators unsupported intentionally */
    const_iterator begin() const { return km_.begin(); }
    const_iterator end() const { return km_.end(); }
private:
    typedef std::map<T, std::set<K> > TMap;

    KMap km_;
    TMap tm_;
};

#endif /* #ifndef _SERVERLIB_EXP_MAP_H */
