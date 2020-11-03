#include "meal_service.h"
#include "ssim_utils.h"
#include "ssim_proc.h"

#define NICKNAME "ASH"
#include <serverlib/slogger.h>

namespace ssim {

bool operator== (const MealServiceInfo& x, const MealServiceInfo& y)
{
    return x.rbdMeals == y.rbdMeals && x.groupMeals == y.groupMeals;
}

std::string mealsToString(const MealServicesMap& mls)
{
    std::string out;
    for (const auto& v : mls) {
        out += ct::rbdCode(v.first);
        for (const auto& s : v.second) {
            out += nsi::MealService(s).code(ENGLISH).toUtf();
        }
        out += '/';
    }
    if (!out.empty()) {
        out.pop_back();
    }
    return out;
}

std::string mealsToString(const MealServiceInfo& mls)
{
    std::string out = mealsToString(mls.rbdMeals);
    if (!mls.groupMeals.empty()) {
        out += '/';
        if (!out.empty()) {
            out += '/';
        }
        for (const auto& s : mls.groupMeals) {
            out += nsi::MealService(s).code(ENGLISH).toUtf();
        }
    }
    return out;
}

boost::optional<MealServiceInfo> getMealsMap(const std::string& deiCnt)
{
    LogTrace(TRACE5) << __FUNCTION__ << ": " << deiCnt;

    MealServiceInfo out;
    for (auto i = deiCnt.begin(), ie = deiCnt.end(); i != ie;) {
        boost::optional<ct::Rbd> rbd;
        // '/' - for all remaining rbds
        if (*i != '/') {
            if (!(rbd = ct::rbdFromStr(std::string(1, *i)))) {
                return boost::none;
            }
            if (out.rbdMeals.find(*rbd) != out.rbdMeals.end()) {
                return boost::none;
            }
        }

        ssim::MealServices meals;
        for (++i; i != ie && *i != '/'; ++i) {
            if (!nsi::MealService::find(EncString::from866(std::string(1, *i)))) {
                return boost::none;
            }
            meals.push_back(nsi::MealService(EncString::from866(std::string(1, *i))).id());
        }
        if (i != ie && *i == '/') {
            ++i;
        }

        if (meals.size() > 2) {
            return boost::none;
        }

        if (!out.groupMeals.empty()) {
            //single short form in the end
            return boost::none;
        }

        if (rbd) {
            out.rbdMeals.emplace(*rbd, meals);
        } else {
            out.groupMeals.swap(meals);
        }
    }
    if (out.rbdMeals.empty() && out.groupMeals.empty()) {
        return boost::none;
    }
    return out;
}

static MealServicesMap unfoldMeals(const MealServiceInfo& msi, const ct::Rbds& rbds)
{
    if (msi.groupMeals.empty()) {
        //запись без сокращений
        return msi.rbdMeals;
    }

    //итак у нас есть сокращенная запись, которую надо применить
    //на все оставшиеся rbd, при этом meals должны передаваться в том же порядке что и prbd
    //так что находим последний упомянутый явно rbd в порядке классов
    ct::Rbds::const_iterator rbi = rbds.cbegin();
    if (!msi.rbdMeals.empty()) {
        for (const auto& v : msi.rbdMeals) {
            auto i = std::find(rbds.cbegin(), rbds.cend(), v.first);
            if (std::distance(rbds.cbegin(), rbi) < std::distance(rbds.cbegin(), i)) {
                rbi = i;
            }
        }
        if (rbi != rbds.cend()) {
            ++rbi;
        }
    }

    MealServicesMap meals = msi.rbdMeals;
    for (; rbi != rbds.cend(); ++rbi) {
        meals.emplace(*rbi, msi.groupMeals);
    }
    return meals;
}

static Expected<MealServiceInfo> translateOprMeals(const MealServiceInfo& msi, const RbdMappings& rbdMap, bool byAcv)
{
    if (msi.rbdMeals.empty()) {
        return msi;
    }

    LogTrace(TRACE5) << __FUNCTION__;

    MealServiceInfo out;
    out.groupMeals = msi.groupMeals;
    for (const auto& v : msi.rbdMeals) {
        if (byAcv) {
            const ct::Cabins mcs = details::remapOprToMan(ct::Cabin(v.first.get()), rbdMap);
            for (ct::Cabin c : mcs) {
                out.rbdMeals.emplace(ct::Rbd(c.get()), v.second);
            }
        } else {
            const ct::Rbds mrs = details::remapOprToMan(v.first, rbdMap);
            for (ct::Rbd r : mrs) {
                out.rbdMeals.emplace(r, v.second);
            }
        }
    }
    return out;
}

static Expected<MealServiceInfo> filterMktMeals(const MealServiceInfo& msi, const RbdMappings& rbdMap, bool byAcv)
{
    if (msi.rbdMeals.empty()) {
        return msi;
    }

    LogTrace(TRACE5) << __FUNCTION__;

    MealServiceInfo out;
    out.groupMeals = msi.groupMeals;
    for (const auto& v : msi.rbdMeals) {
        auto ri = std::find_if(rbdMap.begin(), rbdMap.end(), [byAcv, &v] (const RbdMapping& r) {
            return (byAcv
                ? r.manCabin == ct::Cabin(v.first.get())
                : r.manRbd == v.first
            );
        });

        if (ri != rbdMap.end()) {
            out.rbdMeals.emplace(v.first, v.second);
        }
    }
    return out;
}

Expected<MealServiceInfo> adjustedMeals(
        const MealServiceInfo& msi, const nsi::DepArrPoints& dap,
        const boost::optional<ssim::details::CshContext>& cs, bool byAcv
    )
{
    //для начала надо привести классы в соответствие
    if (cs) {
        const auto rbdMap = cs->getMapping(dap);
        if (rbdMap.empty()) {
            return Message(STDLOG, _("No mapping found for meal service processing"));
        }

        if (!cs->inverse) {
            CALL_EXP_RET(v, filterMktMeals(msi, rbdMap, byAcv));
            return *v;
        } else {
            CALL_EXP_RET(v, translateOprMeals(msi, rbdMap, byAcv));
            if (!v) {
                return v.err();
            }
            return *v;
        }
    }
    return msi;
}

Expected<MealServicesMap> unfoldedMeals(
        const ct::RbdLayout& order, const MealServiceInfo& msi,
        const nsi::DepArrPoints& dap, bool byAcv
    )
{
    //теоретически meal services могут присылаться по ACV
    const ct::Rbds rbds = order.rbds();
    const ct::Cabins cabs = order.cabins();

    if (!msi.rbdMeals.empty()) {
        //убедимся, что все классы из meals нам известны
        for (const auto& v : msi.rbdMeals) {
            if ( (!byAcv && rbds.end() == std::find(rbds.begin(), rbds.end(), v.first)) ||
                 (byAcv && cabs.end() == std::find(cabs.begin(), cabs.end(), ct::Cabin(v.first.get())))
               )
            {
                return Message(STDLOG, _("Incorrect meal service on %1%%2%"))
                    .bind(pointCode(dap.dep)).bind(pointCode(dap.arr));
            }
        }
    }

    if (!byAcv) {
        return unfoldMeals(msi, rbds);
    }

    //meals by ACV classes
    ct::Rbds crs;
    for (ct::Cabin c : cabs) {
        crs.emplace_back(c.get());
    }

    const MealServicesMap meals = unfoldMeals(msi, crs);
    MealServicesMap out;
    for (const auto& v : meals) {
        const ct::Rbds rs = order.rbds(ct::Cabin(v.first.get()));
        for (ct::Rbd r : rs) {
            out.emplace(r, v.second);
        }
    }
    return out;
}

static void appendLongMeals(const MealServiceCollector& setupFn, ct::DeiCode code, const std::string& mealsData)
{
    // если длина превышает 58. нужно бить на части
    if (mealsData.size() < 58) {
        setupFn(code, mealsData);
    } else {
        // ищем первый попавшийся разделитель после 40 символа, и там рапиливаем
        size_t pos = mealsData.find_first_of("/", 40);
        ASSERT(pos != std::string::npos);
        setupFn(code, mealsData.substr(0, pos));
        setupFn(code, mealsData.substr(pos + 1));
    }
}

void setupTlgMeals(
        const MealServiceCollector& setupFn,
        const boost::optional<MealServicesMap>& meals,
        const ct::Rbds& order, bool segOvrd
    )
{
    if (!meals || meals->empty()) {
        return;
    }

    std::string dei;
    size_t count = 0;

    // информация должна быть представлена в том же порядке, что и в порядке подклассов
    std::list<ct::Rbd> rbds;
    std::vector<std::string> mids; //лепим сразу строки, потому что может быть 1 или 2 еды на подкласс
    bool allowShortFormat = true; // если какой-то из подклассов не имеет ms, короткий формат использовать нельзя
    for (const ct::Rbd r : order) {
        auto it = meals->find(r);
        if (it == meals->end()) {
            allowShortFormat = false;
            continue;
        }
        rbds.push_back(r);
        std::string meals = nsi::MealService(it->second.front()).code(ENGLISH).toUtf();
        if (it->second.size() > 1) {
            meals += nsi::MealService(it->second[1]).code(ENGLISH).toUtf();
        }
        if (it->second.size() > 2) {
            LogError(STDLOG) << "Too many meal service codes: rbd " << r << " meals sz=" << it->second.size();
        }
        mids.push_back(meals);
    }
    std::list<ct::Rbd>::const_iterator r = rbds.begin();
    for (auto m = mids.begin(); r != rbds.end() && m != mids.end(); ++r, ++m) {
        count++;
        // проверяем, что среди последующих есть какая-нибудь другая еда
        // eсли нет, то используем сокращенный формат
        if (allowShortFormat && std::none_of(m+1, mids.end(), [m](auto&s){ return *m != s; })) {
            dei += "//" + *m;
            break;
        } else {
            dei += "/" + ct::rbdCode(*r, ENGLISH) + *m;
        }
    }
    if (dei.empty()) {
        return;
    }

    if (segOvrd) {
        // override
        appendLongMeals(setupFn, ct::DeiCode(111), dei.substr(1));
        return;
    }

    // обычная ситуация - meal service на плече
    if (count <= 5) {
        setupFn(ct::DeiCode(7), dei.substr(1));
        return;
    }

    setupFn(ct::DeiCode(7), "XX");
    appendLongMeals(setupFn, ct::DeiCode(109), dei.substr(1));
}

} //ssim
