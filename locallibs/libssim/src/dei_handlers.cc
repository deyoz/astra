#include <serverlib/helpcpp.h>
#include <serverlib/str_utils.h>

#include "dei_handlers.h"
#include "ssim_schedule.h"
#include "csh_settings.h"
#include "ssim_utils.h"
#include "callbacks.h"

#define NICKNAME "ASH"
#include <serverlib/slogger.h>

namespace ssim {

DefaultValueHandler::DefaultValueHandler(const nsi::DepArrPoints& dap, bool isLegOriented)
    : target(dap), byLeg(isLegOriented)
{}

DefaultValueHandler::~DefaultValueHandler()
{}

const nsi::DepArrPoints& DefaultValueHandler::getTarget() const
{
    return target;
}

Message DefaultValueHandler::setValue(const ct::Flight&, const Period&, ssim::PeriodicRoutes&, ct::LegNum) const
{
    return Message();
}

Message DefaultValueHandler::setValue(const ct::Flight&, const Period&, ssim::PeriodicRoutes&, ct::SegNum) const
{
    return Message();
}

Message DefaultValueHandler::setValue(const ct::Flight& flt, const Period& prd, ssim::PeriodicRoutes& r) const
{
    if (byLeg) {
        if (auto ln = ssim::legnum(r.front().second.legs, target)) {
            return setValue(flt, prd, r, *ln);
        }
    } else if (auto sn = ssim::segnum(r.front().second.legs, target)) {
        return setValue(flt, prd, r, *sn);
    }
    return Message();
}

namespace details {

struct AppliedDeis
{
    std::set<ct::DeiCode> deis;
};

static std::unique_ptr<AppliedDeis>& get_applied_deis()
{
    static std::unique_ptr<AppliedDeis> data;
    return data;
}

SegmentInfoCleaner::SegmentInfoCleaner()
{
    get_applied_deis().reset(new AppliedDeis());
}

SegmentInfoCleaner::~SegmentInfoCleaner()
{
    get_applied_deis().reset();
}

void SegmentInfoCleaner::markAsHandled(ct::DeiCode code)
{
    if (auto& p = get_applied_deis()) {
        p->deis.emplace(code);
    }
}

void SegmentInfoCleaner::getUnhandled(ssim::ExtraSegInfos& extras, const ssim::SegmentInfoList& in) const
{
    if (auto& p = get_applied_deis()) {
        const auto& deis = p->deis;

        for (const ssim::SegmentInfo& si : in) {
            if (deis.find(si.deiType) != deis.end()) {
                continue;
            }

            extras.emplace_back(ssim::ExtraSegInfo {
                si.dep.get() == 0 ? boost::none : boost::make_optional(si.dep),
                si.arr.get() == 0 ? boost::none : boost::make_optional(si.arr),
                si.deiType,
                EncString::from866(si.dei).toUtf()
            });
        }
    }
}

static void markValueAsExplicit(ssim::DefValueSetters& dvs, const nsi::DepArrPoints& dap)
{
    dvs.remove_if([&dap] (const ssim::DefValueSetter& x) { return x->getTarget() == dap; });
}

static Expected<ct::RbdLayout> remapRbdOverride(const ct::Rbds& rbds, const std::vector<ssim::RbdMapping>& mapping, bool marketingTlg)
{
    ct::Rbds rs;

    for (ct::Rbd r : rbds) {
        for (const ssim::RbdMapping& v : mapping) {
            if ((marketingTlg ? v.manRbd : v.opRbd) == r) {
                rs.push_back(v.manRbd);
                if (marketingTlg) {
                    //only rbd can be found here
                    break;
                }
            }
        }
    }

    if (auto r = ct::RbdOrder2::create(rs)) {
        return ct::RbdLayout::partialCreate( ct::RbdsConfig(*r, {}) );
    }
    return Message(STDLOG, _("Order from %1% can't be built by present rbdmapping")).bind(ct::rbdsCode(rbds));
}
//#############################################################################
DeiHandler::DeiHandler(int c, const ssim::SegmentInfoList& l)
    : dei(c), slist(l)
{}

DeiHandler::~DeiHandler()
{}

bool DeiHandler::fits(ct::DeiCode val) const
{
    return dei == val;
}

ssim::DefValueSetters DeiHandler::initialState(const ssim::Route&, const ProcContext&) const
{
    return ssim::DefValueSetters();
}


Message DeiHandler::apply(PeriodicRoutes& routes, const ProcContext& attr, const boost::optional<DefaultRequisites>& withDefaults) const
{
    ssim::DefValueSetters dvs = (withDefaults ? initialState(routes.front().second, attr) : ssim::DefValueSetters());
    for (PeriodicRoute& pr : routes) {
        ssim::Route& r(pr.second);
        for (const ssim::SegmentInfo& si : slist) {
            if (!this->fits(si.deiType)) {
                continue;
            }

            if (si.resetValue) {
                CALL_MSG_RET(this->resetValue(r, si));
            } else {
                CALL_MSG_RET(this->applyInner(r, si, dvs));
            }
        }
    }

    if (!dvs.empty() && withDefaults) {
        const DefaultRequisites& wd = *withDefaults;
        LogTrace(TRACE5) << "setup defaut values: DEI " << dei
                         << " for " << wd.flt << ' ' << wd.prd << "; setters: " << dvs.size();

        for (const ssim::DefValueSetter& dv : dvs) {
            CALL_MSG_RET(dv->setValue(wd.flt, wd.prd, routes));
        }
    }

    SegmentInfoCleaner::markAsHandled(dei);

    return Message();
}

Message DeiHandler::resetValue(ssim::Route&, const ssim::SegmentInfo& si) const
{
    return Message(STDLOG, _("NIL value unsupported for DEI %1%")).bind(si.deiType.get());
}
//#############################################################################
DeiHandlerTrafRestr::DeiHandlerTrafRestr(const ssim::SegmentInfoList& l)
    : DeiHandler(8, l)
{}

Message DeiHandlerTrafRestr::applyInner(ssim::Route& r, const ssim::SegmentInfo& si, ssim::DefValueSetters&) const
{
    if (si.trs) {
        for (ct::SegNum id : findSegs(r, si.dep, si.arr)) {
            r.segProps[id].restrictions = *si.trs;
        }
    }
    return Message();
}

Message DeiHandlerTrafRestr::resetValue(ssim::Route& r, const ssim::SegmentInfo& si) const
{
    for (ct::SegNum id : findSegs(r, si.dep, si.arr)) {
        auto i = r.segProps.find(id);
        if (i != r.segProps.end()) {
            i->second.restrictions = ssim::Restrictions { };
        }
    }
    return Message();
}
//#############################################################################
DeiHandlerMktLegDup::DeiHandlerMktLegDup(const ssim::SegmentInfoList& l)
    : DeiHandler(10, l)
{}

Message DeiHandlerMktLegDup::applyInner(ssim::Route& r, const ssim::SegmentInfo& si, ssim::DefValueSetters&) const
{
    for (size_t id : ssim::details::findLegs(r.legs, si.dep, si.arr)) {
        r.legs.at(id).manFlts = si.manFls;
    }
    return Message();
}
//#############################################################################
DeiHandlerOprLegDup::DeiHandlerOprLegDup(const ssim::SegmentInfoList& l)
    : DeiHandler(50, l)
{}

Message DeiHandlerOprLegDup::applyInner(ssim::Route& r, const ssim::SegmentInfo& si, ssim::DefValueSetters&) const
{
    for (size_t id : ssim::details::findLegs(r.legs, si.dep, si.arr)) {
        r.legs.at(id).oprFlt = *si.oprFl;
    }
    return Message();
}
//#############################################################################
static Message setTerm(ssim::Legs& legs, const ssim::SegmentInfo& si, ssim::DefValueSetters& dvs, bool dep)
{
    const nsi::TermId term(StrUtils::trim(si.dei));
    for (size_t id : ssim::details::findLegs(legs, si.dep, si.arr)) {
        ssim::Leg& lg = legs.at(id);
        (dep ? lg.depTerm : lg.arrTerm) = term;
        markValueAsExplicit(dvs, lg.s.dap());

        LogTrace(TRACE5) << "set " << (dep ? "dep " : "arr ") << term << " term for leg #" << id ;
    }
    return Message();
}

static Message resetTermValue(ssim::Route& r, const ssim::SegmentInfo& si, bool dep)
{
    for (size_t id : ssim::details::findLegs(r.legs, si.dep, si.arr)) {
        ssim::Leg& lg = r.legs.at(id);
        if (auto& val = (dep ? lg.depTerm : lg.arrTerm)) {
            LogTrace(TRACE5) << "Reset terminal: " << *val;
            val.reset();
        }
    }
    return Message();
}

DeiHandlerDepTerm::DeiHandlerDepTerm(const ssim::SegmentInfoList &l)
    : DeiHandler(99, l)
{}

Message DeiHandlerDepTerm::applyInner(ssim::Route& r, const ssim::SegmentInfo& si, ssim::DefValueSetters& dvs) const
{
    return setTerm(r.legs, si, dvs, true);
}
Message DeiHandlerDepTerm::resetValue(ssim::Route& r, const ssim::SegmentInfo& si) const
{
    return resetTermValue(r, si, true);
}
ssim::DefValueSetters DeiHandlerDepTerm::initialState(const ssim::Route& r, const ProcContext& attr) const
{
    ssim::DefValueSetters out;
    for (const ssim::Leg& lg : r.legs) {
        if (auto v = attr.callbacks->prepareDefaultValueSetter(dei, lg.s.dap(), true)) {
            out.push_back(v);
        }
    }
    return out;
}

DeiHandlerArrTerm::DeiHandlerArrTerm(const ssim::SegmentInfoList& l)
    : DeiHandler(98, l)
{}

Message DeiHandlerArrTerm::applyInner(ssim::Route& r, const ssim::SegmentInfo& si, ssim::DefValueSetters& dvs) const
{
    return setTerm(r.legs, si, dvs, false);
}
Message DeiHandlerArrTerm::resetValue(ssim::Route& r, const ssim::SegmentInfo& si) const
{
    return resetTermValue(r, si, false);
}
ssim::DefValueSetters DeiHandlerArrTerm::initialState(const ssim::Route& r, const ProcContext& attr) const
{
    ssim::DefValueSetters out;
    for (const ssim::Leg& lg : r.legs) {
        if (auto v = attr.callbacks->prepareDefaultValueSetter(dei, lg.s.dap(), true)) {
            out.push_back(v);
        }
    }
    return out;
}
//#############################################################################
DeiHandlerSegorder::DeiHandlerSegorder(const ssim::SegmentInfoList& s, const boost::optional<ssim::details::CshContext>& c)
    : DeiHandler(101, s), cs(c)
{ }

Message DeiHandlerSegorder::applyInner(ssim::Route& route, const ssim::SegmentInfo& si, ssim::DefValueSetters& dvs) const
{
    ASSERT(!si.dei.empty());
    boost::optional<ct::RbdOrder2> ordOverride = ct::RbdOrder2::getRbdOrder(si.dei);
    if (!ordOverride) {
        return Message(STDLOG, _("Invalid prbd override: %1%")).bind(si.dei);
    }

    for (ct::SegNum id : ssim::details::findSegs(route, si.dep, si.arr)) {
        const nsi::DepArrPoints dap = ssim::getSegDap(route, id);

        boost::optional<ct::RbdLayout> order;
        if (!cs) {
            order = ct::RbdLayout::partialCreate(ct::RbdsConfig(*ordOverride, ct::CabsConfig()));
        } else {
            auto mapping = cs->getMapping(dap);
            if (mapping.empty()) {
                return Message(STDLOG, _("No rbd mapping for %1%%2%"))
                    .bind(ssim::pointCode(dap.dep)).bind(ssim::pointCode(dap.arr));
            }
            CALL_EXP_RET(r, remapRbdOverride(ordOverride->rbds(), mapping, !cs->inverse));
            order = *r;
        }

        ASSERT(order);
        LogTrace(TRACE5) << "PRBD segment override: " << order->toString();
        //---------------------------------------------------------------------
        route.segProps[id].subclOrder = *order;
        markValueAsExplicit(dvs, dap);
    }
    return Message();
}

ssim::DefValueSetters DeiHandlerSegorder::initialState(const ssim::Route& r, const ProcContext& attr) const
{
    ssim::DefValueSetters out;
    for (size_t sn = 0; sn < r.segCount(); ++sn) {
        if (ct::isLongSeg(ct::SegNum(sn))) {
            if (auto v = attr.callbacks->prepareDefaultValueSetter(dei, ssim::getSegDap(r, ct::SegNum(sn)), false)) {
                out.push_back(v);
            }
        }
    }
    return out;
}

Message DeiHandlerSegorder::resetValue(ssim::Route& r, const ssim::SegmentInfo& si) const
{
    for (ct::SegNum id : ssim::details::findSegs(r, si.dep, si.arr)) {
        auto i = r.segProps.find(id);
        if (i != r.segProps.end() && i->second.subclOrder) {
            LogTrace(TRACE5) << "Reset PRBD segment override: " << i->second.subclOrder->toString();
            i->second.subclOrder.reset();
        }
    }
    return Message();
}
//#############################################################################
DeiHandlerInflightServices::DeiHandlerInflightServices(const ssim::SegmentInfoList& l)
    : DeiHandler(503, l)
{}

Message DeiHandlerInflightServices::applyInner(ssim::Route& r, const ssim::SegmentInfo& si, ssim::DefValueSetters&) const
{
    CALL_EXP_RET(srvs, ssim::getInflightServices(si.dei));
    for (size_t i : ssim::details::findLegs(r.legs, si.dep, si.arr)) {
        r.legs.at(i).services = *srvs;
    }
    return Message();
}
//#############################################################################
DeiHandlerSf::DeiHandlerSf(const ssim::SegmentInfoList& l)
    : DeiHandler(504, l)
{}

Message DeiHandlerSf::applyInner(ssim::Route& r, const ssim::SegmentInfo& si, ssim::DefValueSetters& dvs) const
{
    ASSERT(si.secureFlight);
    for (size_t i : ssim::details::findLegs(r.legs, si.dep, si.arr)) {
        r.legs.at(i).secureFlight = *si.secureFlight;
        markValueAsExplicit(dvs, r.legs.at(i).s.dap());
    }
    return Message();
}

ssim::DefValueSetters DeiHandlerSf::initialState(const ssim::Route& r, const ProcContext& attr) const
{
    ssim::DefValueSetters out;
    for (const ssim::Leg& lg : r.legs) {
        if (auto v = attr.callbacks->prepareDefaultValueSetter(dei, lg.s.dap(), true)) {
            out.push_back(v);
        }
    }
    return out;
}
//#############################################################################
DeiHandlerEt::DeiHandlerEt(const ssim::SegmentInfoList& l)
    : DeiHandler(505, l)
{}

Message DeiHandlerEt::applyInner(ssim::Route& r, const ssim::SegmentInfo& si, ssim::DefValueSetters& dvs) const
{
    ASSERT (si.et);
    const bool et = (*si.et ? true : false);
    for (size_t i : ssim::details::findLegs(r.legs, si.dep, si.arr)) {
        r.legs.at(i).et = et;
        markValueAsExplicit(dvs, r.legs.at(i).s.dap());
    }
    return Message();
}

ssim::DefValueSetters DeiHandlerEt::initialState(const ssim::Route& r, const ProcContext& attr) const
{
    ssim::DefValueSetters out;
    for (const ssim::Leg& lg : r.legs) {
        if (auto v = attr.callbacks->prepareDefaultValueSetter(dei, lg.s.dap(), true)) {
            out.push_back(v);
        }
    }
    return out;
}
//#############################################################################
DeiHandlerLegMeals::DeiHandlerLegMeals(const ssim::SegmentInfoList& l, const boost::optional<ssim::details::CshContext>& c)
    : DeiHandler(109, l), cs(c)
{}

Message DeiHandlerLegMeals::applyInner(ssim::Route& route, const ssim::SegmentInfo& si, ssim::DefValueSetters&) const
{
    for (size_t ln : ssim::details::findLegs(route.legs, si.dep, si.arr)) {
        ssim::Leg& lg = route.legs.at(ln);
        CALL_EXP_RET(appMls, ssim::adjustedMeals(*si.meals, lg.s.dap(), cs, false));
        lg.meals = *appMls;
    }
    return Message();
}

Message DeiHandlerLegMeals::resetValue(ssim::Route& route, const ssim::SegmentInfo& si) const
{
    for (size_t ln : ssim::details::findLegs(route.legs, si.dep, si.arr)) {
        ssim::Leg& lg = route.legs.at(ln);
        lg.meals = ssim::MealServiceInfo();
    }
    return Message();
}
//#############################################################################
DeiHandlerSegMeals::DeiHandlerSegMeals(const ssim::SegmentInfoList& l, const boost::optional<ssim::details::CshContext>& c)
    : DeiHandler(111, l), cs(c)
{}

Message DeiHandlerSegMeals::applyInner(ssim::Route& route, const ssim::SegmentInfo& si, ssim::DefValueSetters& dvs) const
{
    for (ct::SegNum sn : ssim::details::findSegs(route, si.dep, si.arr)) {
        auto legs = ssim::getLegsRange(route.legs, sn);
        const nsi::DepArrPoints dap(legs.first->s.from, legs.second->s.to);

        auto appMls = ssim::adjustedMeals(*si.meals, dap, cs, false);
        if (!appMls) {
            return appMls.err();
        }
        route.segProps[sn].meals = *appMls;
        markValueAsExplicit(dvs, dap);
    }
    return Message();
}

Message DeiHandlerSegMeals::resetValue(ssim::Route& route, const ssim::SegmentInfo& si) const
{
    for (ct::SegNum sn : ssim::details::findSegs(route, si.dep, si.arr)) {
        auto i = route.segProps.find(sn);
        if (i != route.segProps.end() && i->second.meals) {
            i->second.meals.reset();
        }
    }
    return Message();
}

ssim::DefValueSetters DeiHandlerSegMeals::initialState(const ssim::Route& r, const ProcContext& attr) const
{
    ssim::DefValueSetters out;
    for (size_t sn = 0; sn < r.segCount(); ++sn) {
        if (ct::isLongSeg(ct::SegNum(sn))) {
            if (auto v = attr.callbacks->prepareDefaultValueSetter(dei, ssim::getSegDap(r, ct::SegNum(sn)), false)) {
                out.push_back(v);
            }
        }
    }
    return out;
}

} //details

} //ssim

