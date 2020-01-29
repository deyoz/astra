#pragma once

#include <list>
#include <functional>
#include "period.h"

template<typename T>
struct PeriodHandler {
    static Period getDatePeriod(const T& obj) { return obj.period; }
    static void setDatePeriod(const Period& p, T& obj) { obj.period = p; }
};

template<typename T>
struct PeriodHandler<std::pair<Period,T> >
{
    static Period getDatePeriod(const std::pair<Period,T>& obj) { return obj.first; }
    static void setDatePeriod(const Period& p, std::pair<Period,T>& obj) { obj.first = p; }
};

template<typename T, typename PeriodHandlerType=PeriodHandler<T> >
class DataPeriodJoiner {
    static Period getJoinedPeriodDefault(const T& lhs, const T& rhs, bool allowBiweek) {
        return Period::join(PeriodHandlerType::getDatePeriod(lhs), PeriodHandlerType::getDatePeriod(rhs), allowBiweek);
    }
  public:
    DataPeriodJoiner(
        std::function<bool(const T&, const T&)> isJoinableDataFnc,
        std::function<Period(const T&, const T&, bool)> periodJoinerFnc = getJoinedPeriodDefault
    )
        : isJoinableData(isJoinableDataFnc), periodJoiner(periodJoinerFnc)
    {}

    template<typename Container>
    void joinInPairs(Container& container, bool allowBiweek = true) const {
        std::list<T> elements(container.begin(), container.end());
        for (auto it1 = elements.begin(); it1 != elements.end(); ++it1) {
            auto it2 = elements.begin();
            while (it2 != elements.end()) {
                if (it2 != it1 && isJoinableData(*it1, *it2)) {
                   Period p = periodJoiner(*it1, *it2, allowBiweek);
                   if (!p.empty()) {
                       PeriodHandlerType::setDatePeriod(p, *it1);
                       elements.erase(it2);
                       it2 = elements.begin();
                       continue;
                   }
                }
                ++it2;
            }
        }
        container = Container(elements.begin(), elements.end());
    }

    template<typename Container>
    void joinBreakAndAssemble(Container& container, bool allowBiweek = true) const {
        typedef std::vector<typename Container::const_iterator> item_refs;
        std::list< item_refs > groups;
        for (auto i = container.begin(), e = container.end(); i != e; ++i) {
            auto is_joinable = std::bind(isJoinableData, std::cref(*i), std::placeholders::_1);
            auto gi = std::find_if(groups.begin(), groups.end(),
                [&is_joinable] (const item_refs& x) { return is_joinable(*x.front()); }
            );
            if (gi == groups.end()) {
                groups.emplace_back( item_refs{i} );
            } else {
                gi->emplace_back(i);
            }
        }

        std::list<T> out;
        for (const item_refs& refs : groups) {
            Periods ps;
            for (auto r : refs) {
                ps.emplace_back(PeriodHandlerType::getDatePeriod(*r));
            }
            for (const Period& p : joinByDates(ps, allowBiweek)) {
                out.emplace_back(*refs.front());
                PeriodHandlerType::setDatePeriod(p, out.back());
            }
        }
        out.sort([] (const T& x, const T& y) {
            return PeriodHandlerType::getDatePeriod(x) < PeriodHandlerType::getDatePeriod(y);
        });
        container = Container(out.begin(), out.end());
    }

    template<typename Container>
    void join(Container& container, bool allowBiweek = true) const {
        const size_t origSize = container.size();
        auto fn = periodJoiner.template target<Period(*)(const T&, const T&, bool)>();

        if (fn && *fn == getJoinedPeriodDefault) {
            Container tmp = container;
            joinBreakAndAssemble(tmp, allowBiweek);
            if (origSize > tmp.size()) {
                container.swap(tmp);
            }
        } else {
            joinInPairs(container, allowBiweek);
        }
    }

  private:
    std::function<bool(const T&, const T&)> isJoinableData;
    std::function<Period(const T&, const T&, bool)> periodJoiner;
};

Periods join(const Periods& ps, bool allowBiweek = true);

