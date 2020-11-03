#include <serverlib/helpcpp.h>
#include <serverlib/str_utils.h>

#include "ssim_create.h"
#include "dei_subsets.h"
#include "ssim_schedule.h"
#include "ssim_utils.h"

#define NICKNAME "DAG"
#include <serverlib/slogger.h>

namespace phs = std::placeholders;

namespace ssim { namespace details {

static bool can_be_multiline(ct::DeiCode dei)
{
    const DeiApplication da = getDeiApplication(dei);
    if (da != DA_SegInfo && da != DA_ShortSegInfo) {
        return false;
    }

    return HelpCpp::contains(
        std::vector<int>{ 8, 109, 111 },
        dei.get()
    );
}

typedef std::map< nsi::DepArrPoints, std::vector<std::string> > leg_data;
typedef std::map< ct::DeiCode, leg_data > dei_leg_map;

static boost::optional<std::string> common_value(const ssim::Route& route, const leg_data& v)
{
    if (v.empty() || v.begin()->second.size() > 1) {
        return boost::none;
    }

    const std::string prv = v.begin()->second.front();
    for (const ssim::Leg& lg : route.legs) {
        auto i = v.find(lg.s.dap());
        if (i == v.end() || i->second.size() > 1 || i->second.front() != prv) {
            return boost::none;
        }
    }
    return prv;
}

static std::string clarify_value(ct::DeiCode dei, const std::string& v)
{
    if (dei.get() == 210) {
        return "PLANE CHANGE";
    }
    if (dei.get() == 201) {
        return "SUBJ GOVT APPROVAL";
    }
    if (dei.get() == 507) {
        return "REQ ALL RES";
    }
    return v;
}

static void fixup_dependencies(dei_leg_map& data)
{
    for (int i = 3; i <= 5; ++i) {
        auto x = data.find(ct::DeiCode(110 + i));
        if (x == data.end()) {
            continue;
        }
        for (auto& v : x->second) {
            data[ct::DeiCode(i)][v.first] = { "X" };
        }
    }
}

class DeiAgrHelper
{
    dei_leg_map data;
    const supported_dei_fn& deiFilter;
public:
    DeiAgrHelper(const supported_dei_fn& filter) : deiFilter(filter) {}

    void append(ct::DeiCode dei, const nsi::DepArrPoints& dap, const std::string& value)
    {
        if (!deiFilter(dei)) {
            LogTrace(TRACE5) << "Skip DEI " << dei;
            return;
        }

        if (can_be_multiline(dei)) {
            data[dei][dap].push_back(value);
        } else {
            data[dei][dap] = { value };
        }
    }

    void apply(const ssim::Route& route, DeiInfo& fdi, LegStuffList& lsl, SegmentInfoList& sl)
    {
        fixup_dependencies(data);

        std::map<ct::SegNum, SegmentInfoList> sls;

        for (const auto& v : data) {
            const DeiApplication app = getDeiApplication(v.first);

            if (app == DA_Complex) {
                if (auto s = common_value(route, v.second)) {
                    //на всех плечах значения одинаковы - ставим в flight info
                    fdi.di.insert(fdi.di.end(), std::make_pair(v.first, *s));
                    continue;
                }
            }

            for (const auto& x : v.second) {
                const nsi::DepArrPoints& dap = x.first;
                if (app == DA_Complex || app == DA_RouteInfo) {
                    auto i = std::find_if(lsl.begin(), lsl.end(),
                        [&dap] (const LegStuff& l) { return dap.dep == l.ri.dep && dap.arr == l.ri.arr; }
                    );
                    if (i != lsl.end()) {
                        i->ri.di.insert(i->ri.di.end(), std::make_pair(v.first, clarify_value(v.first, x.second.front())));
                    }
                } else {
                    const ct::SegNum sn = *ssim::segnum(route.legs, dap);
                    //не будем строго отличать Segment Information (как минимум с 505 мы его нарушаем)
                    for (const std::string& s : x.second) {
                        sls[sn].push_back(SegmentInfo(dap.dep, dap.arr, v.first, clarify_value(v.first, s)));
                    }
                }
            }
        }

        for (const auto& v : sls) {
            for (const auto& si : v.second) {
                sl.push_back(si);
            }
        }
    }
};
//#############################################################################
static void fillMeals(
        DeiAgrHelper& helper, const boost::optional<ssim::MealServicesMap> & meals,
        const nsi::DepArrPoints& dap, const ct::Rbds& order, bool segOvrd = false
    )
{
    ssim::setupTlgMeals(
        std::bind(&DeiAgrHelper::append, &helper, phs::_1, std::cref(dap), phs::_2),
        meals, order, segOvrd
    );
}

static bool isLatinString(const EncString& s)
{
    const std::string t = s.to866();
    return t.end() == std::find_if(t.begin(), t.end(), StrUtils::IsRusChar);
}

static void fillRestrictions(DeiAgrHelper& helper, const nsi::DepArrPoints& dap, const ssim::Restrictions& trs)
{
    ssim::setupTlgRestrictions(
        std::bind(&DeiAgrHelper::append, &helper, phs::_1, std::cref(dap), phs::_2),
        trs
    );
}
//#############################################################################
namespace {

struct FranchisePub
{
    std::string code;
    std::string name;
};

} //anonymous

static boost::optional<FranchisePub> makeFranchisePub(const boost::optional<ssim::Franchise>& in, const PubOptions& opts)
{
    if (!in) {
        return boost::none;
    }

    const ssim::Franchise& fr = *in;

    const EncString airCode = fr.code ? nsi::Company(*fr.code).code(ENGLISH) : EncString();
    const bool canUseCode = (!airCode.empty() && (!opts.latinOnly || isLatinString(airCode)));

    const FranchisePub out = {
        canUseCode ? airCode.toUtf() : std::string(),
        fr.name
    };

    if (out.code.empty() && out.name.empty()) {
        LogError(STDLOG) << "Cannot publish franchise: " << fr.name << '/' << airCode;
        return boost::none;
    }

    return out;
}

static void fillAgreements(DeiAgrHelper& helper, const ssim::Leg& lg, const PubOptions& opts)
{
    const nsi::DepArrPoints dap = lg.s.dap();

    if (!lg.manFlts.empty()) {
        helper.append(ct::DeiCode(10), dap,
            StrUtils::join("/", lg.manFlts, [] (const ct::Flight& x) { return x.toTlgStringUtf(ENGLISH); })
        );
    }

    //csh-opr:
    // - 2/OPR_CODE + 50/OPR_FLIGHT

    //franchise:
    // - 9/FR_CODE
    // - 9/X + 127/FR_CODE/FR_NAME
    // - 9/X + 127//FR_NAME

    //csh-opr + franchise:
    // - 2/FR_CODE + 50/OPR_FLIGHT
    // - 2/X + 50/OPR_FLIGHT + 127/FR_CODE/FR_NAME
    // - 2/X + 50/OPR_FLIGHT + 127//FR_NAME

    const boost::optional<FranchisePub> frp = makeFranchisePub(lg.franchise, opts);

    if (lg.oprFlt) {
        if (!frp) {
            helper.append(ct::DeiCode(2), dap, nsi::Company(lg.oprFlt->airline).code(ENGLISH).toUtf());
        }
        helper.append(ct::DeiCode(50), dap, lg.oprFlt->toTlgStringUtf(ENGLISH));
    }

    if (frp) {
        const FranchisePub& fr = *frp;

        helper.append(
            lg.oprFlt ? ct::DeiCode(2) : ct::DeiCode(9),
            dap,
            (fr.name.empty() && !fr.code.empty()) ? fr.code : "X"
        );

        if (!fr.name.empty()) {
            helper.append(ct::DeiCode(127), dap, fr.code + '/' + fr.name);
        }
    }
}
//#############################################################################
void collectDeis(DeiAgrHelper& helper, const ssim::ScdPeriod& scp, const PubOptions& opts)
{
    LogTrace(TRACE5) << __FUNCTION__;

    const ssim::Route& route = scp.route;
    for (const ssim::Leg& lg : route.legs) {
        const nsi::DepArrPoints dap = lg.s.dap();

        if (lg.arrTerm) {
            helper.append(ct::DeiCode(98), dap, HelpCpp::string_cast(*lg.arrTerm));
        }
        if (lg.depTerm) {
            helper.append(ct::DeiCode(99), dap, HelpCpp::string_cast(*lg.depTerm));
        }

        helper.append(ct::DeiCode(505), dap, (lg.et ? "ET" : "EN"));

        if (lg.secureFlight) {
            helper.append(ct::DeiCode(504), dap, "S");
        }
        
        fillAgreements(helper, lg, opts);

        fillMeals(helper, lg.meals.rbdMeals, lg.s.dap(), lg.subclOrder.rbds(), false);

        if (!lg.services.empty()) {
            helper.append(ct::DeiCode(503), dap, ssim::toString(lg.services));
        }
    }
    //-------------------------------------------------------------------------
    for (const ssim::SegmentsProps::value_type & vt : route.segProps) {
        auto lgs = ssim::getLegsRange(route.legs, vt.first);
        const nsi::DepArrPoints dap(lgs.first->s.from, lgs.second->s.to);

        if (vt.second.subclOrder) {
            helper.append(ct::DeiCode(101), dap, ct::rbdsCode(vt.second.subclOrder->rbds()));
        }
        if (vt.second.et && ct::isLongSeg(vt.first)) {
            helper.append(ct::DeiCode(505), dap, *vt.second.et ? "ET" : "EN");
        }
        if (vt.second.meals) {
            fillMeals(helper, vt.second.meals->rbdMeals, dap,
                (vt.second.subclOrder ? *vt.second.subclOrder : lgs.first->subclOrder).rbds(),
                true
            );
        }

        fillRestrictions(helper, dap, vt.second.restrictions);
    }
    //-------------------------------------------------------------------------
    for (const ssim::ExtraSegInfo& esi : scp.auxData) {
        //append extras only with explicit segment reference
        if (esi.depPt && esi.arrPt) {
            helper.append(esi.code, { *esi.depPt, *esi.arrPt }, esi.content);
        }
    }
}

void setupSsimData(
        const ssim::ScdPeriod& scp, ssim::DeiInfo& fdi,
        ssim::LegStuffList& lsl, ssim::SegmentInfoList& sl,
        const PubOptions& opts,
        const supported_dei_fn& deiFilter
    )
{
    LogTrace(TRACE5) << __FUNCTION__;

    //для начала общая информация
    for (const ssim::Leg& lg : scp.route.legs) {
        lsl.emplace_back(LegStuff {
            ssim::RoutingInfo(lg.s.from, lg.s.dep, "", lg.s.to, lg.s.arr, ""),
            ssim::EquipmentInfo(lg.serviceType, lg.aircraftType, ct::RbdsConfig(lg.subclOrder.rbdOrder(), lg.subclOrder.config()))
        });
    }

    //теперь собираем DEI
    DeiAgrHelper helper(deiFilter);
    collectDeis(helper, scp, opts);
    helper.apply(scp.route, fdi, lsl, sl);
}

} } //ssim::details

