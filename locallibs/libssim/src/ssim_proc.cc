#include <nsi/message_binders.h>
#include "ssim_proc.h"
#include "ssim_utils.h"
#include "callbacks.h"

#define NICKNAME "ASH"
#include <serverlib/slogger.h>

namespace ssim {

boost::optional<PartnerInfo> getPartnerInfo(AgreementType agr, const std::vector<const DeiInfo*>& infos)
{
    //берем требуемую информацию о партнере, считая что на входе упорядоченный по возрастанию
    //уровня список элементов, где он может быть указан
    for (const DeiInfo* di : infos) {
        auto i = di->partners.find(agr);
        if (i == di->partners.end()) {
            continue;
        }
        //можно, конечно, пройтись дальше и ругаться на несоответсвия, но зачем...
        return i->second;
    }
    return boost::none;
}

static boost::optional<ssim::Franchise> findOprDisclosure(
        const SegmentInfoList& segs, const nsi::PointId& dep, const nsi::PointId& arr
    )
{
    ssim::details::SegmentInfoCleaner::markAsHandled(ct::DeiCode(127));

    std::vector<SegmentInfoList::const_iterator> infos;
    for (auto si = segs.begin(); si != segs.end(); ++si) {
        if (si->deiType.get() != 127) {
            continue;
        }
        if ((si->dep == dep || si->dep.get() == 0) && (si->arr == arr || si->arr.get() == 0)) {
            infos.push_back(si);
        }
    }

    if (infos.empty()) {
        return boost::none;
    }

    std::sort(infos.begin(), infos.end(),
        [] (SegmentInfoList::const_iterator x, SegmentInfoList::const_iterator y) {
            return (x->dep != y->dep) ? x->dep < y->dep : x->arr < y->arr;
        }
    );
    return infos.back()->oprDisclosure;
}

static Message applyAgreements(
        const FlightInfo& fi, const PeriodInfo& pi, const RoutingInfoList& legs,
        const SegmentInfoList& segs, const ProcContext& attr, ssim::ScdPeriod& scd
    )
{
    LogTrace(TRACE5) << __FUNCTION__;

    PeriodicRoutes routes{ PeriodicRoute(scd.period, scd.route) };
    //для начала DEI 10/50 - ни от чего не зависят, ничего не хотят
    CALL_MSG_RET(ssim::details::DeiHandlerMktLegDup(segs).apply(routes, attr));
    CALL_MSG_RET(ssim::details::DeiHandlerOprLegDup(segs).apply(routes, attr));

    //теперь надо разобраться с DEI 2/9/127
    //при этом надо учитывать, что DEI 2/127 для маркетинга может трактоваться как wet lease
    //при наличии в DEI 50 иной компании

    ASSERT(1 == routes.size());
    std::swap(scd.route, routes.front().second);
    ssim::Route& route = scd.route;

    const bool wasOprLegDup = (route.legs.end() != std::find_if(route.legs.begin(), route.legs.end(),
        [] (const ssim::Leg& x) { return static_cast<bool>(x.oprFlt); }
    ));

    for (const RoutingInfo& ri : legs) {
        auto lgi = std::find_if(route.legs.begin(), route.legs.end(),
            [&ri] (const ssim::Leg& x) { return x.s.from == ri.dep && x.s.to == ri.arr; }
        );
        if (lgi == route.legs.end()) {
            continue;
        }

        //DEI 2
        auto oprDis = getPartnerInfo(AgreementType::CSH, { &ri, &pi, &fi });
        //DEI 9
        auto wetDis = getPartnerInfo(AgreementType::FRANCH, { &ri, &pi, &fi });

        if (!oprDis && !wetDis) {
            //нет никаких disclosure
            continue;
        }

        if (oprDis && wetDis) {
            //не должно быть одновременно DEI 2 и DEI 9
            //они используют одно DEI 127 для уточнения и делят 1 поле в SSIM (chapter 7)
            return Message(STDLOG, _("Incorrect use of DEIs 2 and 9"));
        }

        const PartnerInfo& partner = (oprDis ? *oprDis : *wetDis);

        boost::optional<Franchise> oprDisclosure;
        if (!partner.company) {
            //надо искать DEI 127 для уточнения
            if (!(oprDisclosure = findOprDisclosure(segs, ri.dep, ri.arr))) {
                return Message(STDLOG, _("No description of X on leg %1%%2%"))
                    .bind(ssim::pointCode(ri.dep)).bind(ssim::pointCode(ri.arr));
            }
        } else {
            oprDisclosure = Franchise { partner.company };
        }

        bool wetLeaseDisclosure = static_cast<bool>(wetDis);
        if (oprDis) {
            const nsi::CompanyId* airline = (oprDis->company
                ? &oprDis->company.get()
                : (oprDisclosure && oprDisclosure->code ? &oprDisclosure->code.get() : nullptr )
            );

            if (!lgi->oprFlt) {
                //есть DEI 2, но нет DEI 50 - весьма странно
                //однако же по неизвестным причинам мы трактуем 2/[Airline] как кодшер с тем же номером рейса
                //но только если не было ни одного DEI 50 на других плечах
                if (airline) {
                    if (!wasOprLegDup) {
                        lgi->oprFlt = ct::Flight(*airline, scd.flight.number, scd.flight.suffix);
                    }
                } else {
                    //ну а если даже компании нет, считаем это лизингом
                    wetLeaseDisclosure = true;
                }
            } else if (!airline || *airline != lgi->oprFlt->airline) {
                //компания не указана или не совпадает с компанией из DEI 50
                //значит маркетинг сообщает о лизинге
                wetLeaseDisclosure = true;
            }
        }

        if (wetLeaseDisclosure) {
            lgi->franchise = oprDisclosure;
        }
    }
    return Message();
}
//#############################################################################
static bool haveMealsContinuation(
        nsi::PointId dep, nsi::PointId arr, const ssim::SegmentInfoList& sis
    )
{
    for (const ssim::SegmentInfo& si : sis) {
        if (si.deiType.get() != 109) {
            continue;
        }
        if ((si.dep.get() == 0 || si.dep == dep) && (si.arr.get() == 0 || si.arr == arr)) {
            return true;
        }
    }
    return false;
}

static Message applyMeals(
        ssim::PeriodicRoutes& route, const ssim::RoutingInfoList& ris, const ssim::SegmentInfoList& sis,
        const ProcContext& attr,
        const boost::optional<ssim::details::CshContext>& cs,
        const boost::optional<ssim::details::DefaultRequisites>& withDefaults
    )
{
    //т.к. в общем случае применение meal services требует знания порядка подклассов
    //подразумеваем, что к моменту попадания сюда все, что касалось prbd уже заполнено

    //Meal service note (DEI 7) может применяться на ACV, если не указаны PRBD
    //про остальные (DEI 109, 111) ничего не написано, полагаем, что для них подобный финт не применим

    //сначала пройдемся по DEI 7
    for (const ssim::RoutingInfo& ri : ris) {
        if (ri.di.find(ct::DeiCode(7)) == ri.di.end()) {
            continue;
        }

        if (ri.mealsContinuationExpected) {
            if (!haveMealsContinuation(ri.dep, ri.arr, sis)) {
                return Message(STDLOG, _("Missing expected DEI 109 on %1%%2%"))
                    .bind(pointCode(ri.dep)).bind(pointCode(ri.arr));
            }
            continue;
        }

        const auto lns = ssim::details::findLegs(route.front().second.legs, ri.dep, ri.arr);
        if (lns.size() != 1) {
            LogError(STDLOG) << "Missing leg for DEI 7: " << ri.dep << '-' << ri.arr;
            continue;
        }

        for (PeriodicRoute& pr : route) {
            ssim::Leg& lg = pr.second.legs.at(lns.front());

            CALL_EXP_RET(appMls, adjustedMeals(*ri.meals, lg.s.dap(), cs, lg.subclOrder.rbds().empty()));
            lg.meals = *appMls;
        }
    }
    CALL_MSG_RET(ssim::details::DeiHandlerLegMeals(sis, cs).apply(route, attr, withDefaults));
    CALL_MSG_RET(ssim::details::DeiHandlerSegMeals(sis, cs).apply(route, attr, withDefaults));
    return Message();
}
//#############################################################################
std::vector<nsi::DepArrPoints> getAffectedLegs(const nsi::Points& lci, const ssim::Route& route, bool is_ssm)
{
    ASSERT(lci.size() > 1);

    //SSIM: The notification of intermediate stations is optional for SSM messages in Chapter 4
    const nsi::PointId bpt = lci.front();
    const nsi::PointId ept = lci.back();

    auto blg = std::find_if(route.legs.begin(), route.legs.end(),
        [&bpt] (const ssim::Leg& x) { return x.s.from == bpt; }
    );
    if (blg == route.legs.end()) {
        return {};
    }

    auto elg = std::find_if(route.legs.begin(), route.legs.end(),
        [&ept] (const ssim::Leg& x) { return x.s.to == ept; }
    );
    if (elg == route.legs.end()) {
        return {};
    }

    std::vector<nsi::DepArrPoints> out;
    nsi::Points pts = { blg->s.from };
    for (auto i = blg; i != std::next(elg); ++i) {
        pts.push_back(i->s.to);
        out.push_back(i->s.dap());
    }

    if (lci == pts || (is_ssm && lci.size() == 2)) {
        return out;
    }
    return {};
}

using PeriodicRoutesWithCsh = std::vector< std::pair< ssim::PeriodicRoute, boost::optional<ssim::CshSettings> > >;

static PeriodicRoutesWithCsh splitByCsh(const ssim::PeriodicRoutes& in, const ssim::PeriodicCshs& cshs)
{
    PeriodicRoutesWithCsh out;

    if (cshs.empty()) {
        for (const ssim::PeriodicRoute& pr : in) {
            out.emplace_back(pr, boost::none);
        }
        return out;
    }

    Periods rps;
    std::transform(in.begin(), in.end(), std::back_inserter(rps),
        [] (const ssim::PeriodicRoute& x) { return x.first; }
    );

    Periods cps;
    std::transform(cshs.begin(), cshs.end(), std::back_inserter(cps),
        [] (const ssim::PeriodicCsh& x) { return x.first; }
    );

    for (const Period& p : Period::normalize(Period::intersect(rps, cps))) {
        auto pf = [&p] (const auto& x) { return !(x.first & p).empty(); };
        auto i = std::find_if(in.begin(), in.end(), pf);
        auto j = std::find_if(cshs.begin(), cshs.end(), pf);
        ASSERT(i != in.end() && j != cshs.end());

        const ssim::CshSettings& cs = j->second;
        out.emplace_back(ssim::PeriodicRoute { p, i->second }, cs);

        //setup (missing) operating reference if host supports it
        if (cs.oprFlt) {
            for (ssim::Leg& lg : out.back().first.second.legs) {
                ASSERT(!lg.oprFlt || lg.oprFlt == cs.oprFlt);
                lg.oprFlt = cs.oprFlt;
            }
        }
    }

    for (const Period& p : Period::normalize(Period::difference(rps, cps))) {
        auto i = std::find_if(in.begin(), in.end(), [&p] (const auto& x) { return !(x.first & p).empty(); });
        ASSERT(i != in.end());
        out.emplace_back(ssim::PeriodicRoute { p, i->second }, boost::none);
    }

    return out;
}
//#############################################################################
namespace route_makers {

Expected<ssim::Route> createNew(const ssim::LegStuffList& lsl, const ssim::SegmentInfoList& sl)
{
    ssim::Route route;

    ssim::Legs& lgs = route.legs;
    for (const ssim::LegStuff& ls : lsl) {
        lgs.emplace_back(
            Section(ls.ri.dep, ls.ri.arr, ls.ri.depTime, ls.ri.arrTime),
            ls.eqi.serviceType, ls.eqi.tts,
            ct::RbdLayout::partialCreate(ls.eqi.config)
        );
    }
    LogTrace(TRACE5) << route;
    return route;
}

Expected<ssim::PeriodicRoutes> applyTim(
        const ssim::ScdPeriod& ref,
        const ssim::RoutingInfoList& rl,
        const ssim::SegmentInfoList& sl,
        const ProcContext& attr, bool invertedScd
    )
{
    if (rl.size() != ref.route.legs.size()) {
        return Message(STDLOG, _("TIM isn't allowed to change route points"));
    }

    ssim::Route route(ref.route);
    for (size_t i = 0; i < rl.size(); ++i) {
        const ssim::RoutingInfo& ri = rl[i];
        ssim::Leg& lg = route.legs[i];

        if (lg.s.from != ri.dep || lg.s.to != ri.arr) {
            return Message(STDLOG, _("Change of ports on leg #%1%")).bind(i + 1);
        }

        lg.s.dep = ri.depTime;
        lg.s.arr = ri.arrTime;
    }

    CALL_EXP_RET(ecs, attr.callbacks->cshSettingsByScd(ref));

    PeriodicRoutes routes;
    const ssim::details::DefaultRequisites wd = { ref.flight, ref.period };
    for (const auto& cpr : splitByCsh( { ssim::PeriodicRoute(ref.period, route) }, *ecs)) {
        PeriodicRoutes prs = { cpr.first };

        const auto cs = ssim::details::CshContext::create(cpr.second, invertedScd);

        CALL_MSG_RET(ssim::details::DeiHandlerArrTerm(sl).apply(prs, attr));
        CALL_MSG_RET(ssim::details::DeiHandlerDepTerm(sl).apply(prs, attr));
        CALL_MSG_RET(ssim::details::DeiHandlerEt(sl).apply(prs, attr));

        CALL_MSG_RET(applyMeals(prs, rl, sl, attr, cs, wd));
        routes.insert(routes.end(), prs.begin(), prs.end());
    }

    return routes;
}

Expected<ssim::PeriodicRoutes> applyEqtCon(
        const ssim::ScdPeriod& ref,
        const ssim::EquipmentInfo& eqi,
        const ssim::SegmentInfoList& sl,
        const boost::optional<nsi::Points>& legs,
        const ProcContext& attr,
        const boost::optional<PartnerInfo>& oprDis, bool invertedScd
    )
{
    boost::optional< std::vector< nsi::DepArrPoints > > affected;
    if (legs) {
        affected = ssim::getAffectedLegs(*legs, ref.route, true);
        if (affected->empty()) {
            return Message(STDLOG, _("Cannot resolve Leg Change Identifier"));
        }
    }

    ssim::Route route(ref.route);
    for (ssim::Leg& l : route.legs) {
        if (affected && affected->end() == std::find(affected->begin(), affected->end(), l.s.dap())) {
            continue;
        }

        l.serviceType = eqi.serviceType;
        l.aircraftType = eqi.tts;
        l.subclOrder = ct::RbdLayout::partialCreate(eqi.config);
    }

    CALL_EXP_RET(ecs, ssim::details::getCshContextEqtCon(ref, attr, invertedScd, oprDis));

    const ssim::details::DefaultRequisites wd = { ref.flight, ref.period };

    ssim::PeriodicRoutes routes;
    for (const auto& cpr : ssim::splitByCsh( { ssim::PeriodicRoute(ref.period, route) }, *ecs )) {
        ssim::PeriodicRoutes prs = { cpr.first };
        const auto cs = ssim::details::CshContext::create(cpr.second, invertedScd);
        CALL_MSG_RET(correctLegOrders(prs, cs))
        CALL_MSG_RET(ssim::details::DeiHandlerSegorder(sl, cs).apply(prs, attr, wd));
        routes.insert(routes.end(), prs.begin(), prs.end());
    }
    return routes;
}

Expected<ssim::PeriodicRoutes> applyAdm(
        const ssim::ScdPeriod& ref,
        const ssim::SegmentInfoList& slist,
        const ProcContext& attr, bool invertedScd
    )
{
    const ssim::details::DefaultRequisites wd = { ref.flight, ref.period };

    CALL_EXP_RET(ecs, attr.callbacks->cshSettingsByScd(ref));

    ssim::PeriodicRoutes routes;
    for (const auto& cpr : splitByCsh({ ssim::PeriodicRoute(ref.period, ref.route) }, *ecs)) {
        PeriodicRoutes prs = { cpr.first };

        const auto cs = ssim::details::CshContext::create(cpr.second, invertedScd);

        CALL_MSG_RET(ssim::details::DeiHandlerTrafRestr(slist).apply(prs, attr, wd));
        CALL_MSG_RET(ssim::details::DeiHandlerArrTerm(slist).apply(prs, attr, wd));
        CALL_MSG_RET(ssim::details::DeiHandlerDepTerm(slist).apply(prs, attr, wd));
        CALL_MSG_RET(ssim::details::DeiHandlerEt(slist).apply(prs, attr, wd));
        CALL_MSG_RET(ssim::details::DeiHandlerSf(slist).apply(prs, attr, wd));
        CALL_MSG_RET(ssim::details::DeiHandlerSegorder(slist, cs).apply(prs, attr, wd));
        CALL_MSG_RET(ssim::details::DeiHandlerLegMeals(slist, cs).apply(prs, attr, wd));
        CALL_MSG_RET(ssim::details::DeiHandlerSegMeals(slist, cs).apply(prs, attr, wd));

        routes.insert(routes.end(), prs.begin(), prs.end());
    }
    return routes;
}

} //route_makers
//#############################################################################
static Message truncateByMkt(ssim::ScdPeriod& scd, const nsi::CompanyId& mkt)
{
    //remove redundant marketings and legs without suitable marketings
    size_t lastMktLeg = 0, ln = 0;
    ssim::Legs mktLegs;
    for (ssim::Leg& lg : scd.route.legs) {
        lg.manFlts.erase(
            std::remove_if(lg.manFlts.begin(), lg.manFlts.end(),
                [&mkt] (const ct::Flight& x) { return x.airline != mkt; }
            ),
            lg.manFlts.end()
        );
        if (lg.manFlts.size() > 1) {
            return Message(STDLOG, _("Ambiguous marketing leg duplication set on %1%%2%"))
                .bind(ssim::pointCode(lg.s.from)).bind(ssim::pointCode(lg.s.to));
        }
        if (!lg.manFlts.empty()) {
            if (!mktLegs.empty()) {
                if (mktLegs.back().manFlts.front() != lg.manFlts.front()) {
                    return Message(STDLOG, _("Different marketing leg duplications"));
                }
                if (ln - lastMktLeg != 1) {
                    return Message(STDLOG, _("Invalid marketing route"));
                }
            }
            mktLegs.push_back(lg);
            lastMktLeg = ln;
            ++ln;
        }
    }

    scd.route.legs.swap(mktLegs);
    if (scd.route.legs.empty()) {
        return Message(STDLOG, _("No information about marketing partner"));
    }

    const ct::Flight oprFlt = scd.flight;
    scd.flight = scd.route.legs.front().manFlts.front();
    for (ssim::Leg& lg : scd.route.legs) {
        lg.manFlts.clear();
        lg.oprFlt = oprFlt;
    }

    return Message();
}

static Message inverseOperatingScd(ssim::ScdPeriod& scd, nsi::CompanyId mkt)
{
    //schedule builded by SSM/ASM in operating requisites

    const Legs& lgs = scd.route.legs;

    //there should be no operating leg indentification (DEI 50)
    if (std::any_of(lgs.begin(), lgs.end(), [] (const Leg& x) { return static_cast<bool>(x.oprFlt); } )) {
        return Message(STDLOG, _("Incorrect duplicate leg cross reference"));
    }

    CALL_MSG_RET(truncateByMkt(scd, mkt));

    if (scd.flight.airline != mkt) {
        return Message(STDLOG, _("Airline %1% misfits system profile")).bind(scd.flight.airline);
    }
    return Message();
}

static Expected<ct::RbdLayout> filteredMktOrder(const ct::RbdLayout& ord, const ssim::RbdMappings& mapping)
{
    LogTrace(TRACE5) << __FUNCTION__;

    if (mapping.empty()) {
        LogTrace(TRACE1) << "No mapping found for rbds filtering";
        return Message();
    }

    ct::Rbds rs;
    ct::Cabins cabs;
    ct::rbd_layout_map rm;
    for (ct::Rbd r : ord.rbds()) {
        auto ri = std::find_if(mapping.begin(), mapping.end(), [r] (const ssim::RbdMapping& mp) { return mp.manRbd == r; } );
        if (ri != mapping.end()) {
            rs.push_back(r);
            rm[ri->manCabin].push_back(r);
            if (cabs.end() == std::find(cabs.begin(), cabs.end(), ri->manCabin)) {
                cabs.push_back(ri->manCabin);
            }
        }
    }

    //подправляем конфиг
    ct::CabsConfig fcs, ocs = ord.config();
    for (ct::Cabin c : cabs) {
        auto ci = std::find_if(ocs.begin(), ocs.end(), [c] (const ct::CabsConfig::value_type& cc) { return cc.first == c; });
        if (ci != ocs.end()) {
            fcs.emplace_back(c, ci->second);
        } else {
            fcs.emplace_back(c, ct::IntOpt());
        }
    }

    boost::optional<ct::RbdLayout> out = ct::RbdLayout::create(rm, rs, fcs);
    if (!out) {
        return Message(STDLOG, _("Couldn't filter order %1% by mapping")).bind(ord.toString());
    }

    LogTrace(TRACE1) << "config filtered from [" << ord.toString() << "] to [" << out->toString() << "]";
    return *out;
}

static Expected<ct::RbdLayout> translateOprOrder(const ct::RbdLayout& org, const ssim::RbdMappings& mapping)
{
    LogTrace(TRACE5) << __FUNCTION__;

    if (mapping.empty()) {
        return Message(STDLOG, _("No mapping found for operator's rbds translation"));
    }

    //собственно порядок
    ct::Rbds mrs;
    ct::Cabins cabs;
    ct::rbd_layout_map rlm;
    for (ct::Rbd r : org.rbds()) {
        for (const ssim::RbdMapping& rm : mapping) {
            if (rm.opRbd == r) {
                rlm[rm.manCabin].push_back(rm.manRbd);
                if (cabs.end() == std::find(cabs.begin(), cabs.end(), rm.manCabin)) {
                    cabs.push_back(rm.manCabin);
                }
                mrs.push_back(rm.manRbd);
            }
        }
    }
    LogTrace(TRACE5) << "remapped order " << ct::rbdsCode(mrs) << " with cabins " << ct::cabinsCode(cabs);

    //дополнительно смотрим на конфигурацию
    ct::CabsConfig mcs, ocs = org.config();
    for (ct::Cabin c : cabs) {
        const ct::Cabin oc = *ssim::details::remapManToOpr(c, mapping);

        auto oci = std::find_if(ocs.begin(), ocs.end(),  [oc] (const ct::CabsConfig::value_type& cc) { return cc.first == oc; } );
        if (oci != ocs.end()) {
            mcs.emplace_back(c, oci->second);
        } else {
            mcs.emplace_back(c, ct::IntOpt());
        }
    }

    boost::optional<ct::RbdLayout> out = ct::RbdLayout::create(rlm, mrs, mcs);
    if (!out) {
        return Message(STDLOG, _("Couldn't translate operating order %1% into marketing's")).bind(org.toString());
    }

    LogTrace(TRACE1) << "order translated from [" << org.toString() << "] to [" << out->toString() << "]";
    return *out;
}

Message correctLegOrders(ssim::PeriodicRoutes& route, const boost::optional<ssim::details::CshContext>& cs)
{
    if (!cs) {
        return Message();
    }

    for (ssim::PeriodicRoute& proute : route) {
        for (ssim::Leg& lg : proute.second.legs) {
            const auto mapping = cs->getMapping(lg.s.dap());

            if (cs->inverse) {
                CALL_EXP_RET(ord, translateOprOrder(lg.subclOrder, mapping));
                lg.subclOrder = *ord;
            } else {
                CALL_EXP_RET(ord, filteredMktOrder(lg.subclOrder, mapping));
                lg.subclOrder = *ord;
            }
        }
    }

    return Message();
}

static Message applyEssentials(
        ssim::ScdPeriods& periods,
        const ssim::FlightInfo& fi, const ssim::PeriodInfo& per,
        const ssim::LegStuffList& legs, const ssim::SegmentInfoList& segs,
        const ProcContext& attr
    )
{
    ssim::ScdPeriods newPeriods;
    for (ssim::ScdPeriod pr : periods) {
        //apply codeshare and franchise agreements
        ssim::RoutingInfoList rl;
        std::transform(legs.begin(), legs.end(), std::back_inserter(rl), [] (const ssim::LegStuff& x) { return x.ri; } );
        CALL_MSG_RET(applyAgreements(fi, per, rl, segs, attr, pr));

        //-------------------------------------------------------------------------
        const bool inversed = (attr.inventoryHost && attr.ourComp != pr.flight.airline);
        if (inversed) {
            CALL_MSG_RET(inverseOperatingScd(pr, attr.ourComp));
        }

        CALL_EXP_RET(ecs, attr.callbacks->cshSettingsByTlg(attr.ourComp, pr));

        for (const auto& cpr : splitByCsh( { ssim::PeriodicRoute { pr.period, pr.route } }, *ecs)) {
            const auto cs = ssim::details::CshContext::create(cpr.second, inversed);
            if (!cs) {
                if (attr.inventoryHost && !attr.sloader) {
                    return Message(STDLOG, _("Unexpected absence of code-sharing reference"));
                }
            }

            const ssim::details::DefaultRequisites wd = { pr.flight, pr.period };
            ssim::PeriodicRoutes routes { cpr.first };
            CALL_MSG_RET(ssim::details::DeiHandlerTrafRestr(segs).apply(routes, attr, wd));
            CALL_MSG_RET(ssim::details::DeiHandlerArrTerm(segs).apply(routes, attr, wd));
            CALL_MSG_RET(ssim::details::DeiHandlerDepTerm(segs).apply(routes, attr, wd));
            CALL_MSG_RET(ssim::details::DeiHandlerEt(segs).apply(routes, attr, wd));
            CALL_MSG_RET(ssim::details::DeiHandlerSf(segs).apply(routes, attr, wd));

            CALL_MSG_RET(correctLegOrders(routes, cs))
            CALL_MSG_RET(ssim::details::DeiHandlerSegorder(segs, cs).apply(routes, attr, wd));
            CALL_MSG_RET(ssim::details::DeiHandlerInflightServices(segs).apply(routes, attr, wd));
            CALL_MSG_RET(applyMeals(routes, rl, segs, attr, cs, wd));

            for (const ssim::PeriodicRoute& proute : routes) {
                newPeriods.push_back(pr);
                ssim::ScdPeriod& s = newPeriods.back();
                s.period = proute.first;
                s.route = proute.second;
                LogTrace(TRACE5) << s;
            }
        }
    }
    periods.swap(newPeriods);
    return Message();
}

Expected<ssim::ScdPeriods> makeScdPeriod(
        const ssim::FlightInfo& fi, const ssim::PeriodInfo& per,
        const ssim::LegStuffList& legs, const ssim::SegmentInfoList& segs,
        const ProcContext& attr, const timeModeConvert& tmConv,
        bool ssm
    )
{
    const ct::Flight flt = fi.flt;
    const Period prd = Period::normalize(per.period);

    auto route = ssim::route_makers::createNew(legs, segs);
    if (!route) {
        return route.err();
    }
    // сдвиг в начале маршрута больше 24 часов - нет чтоб даты нормально указывать...
    // NOTE проверка не в построении маршрута, поскольку для asm это нормально(?) - для рейда на 27 прислать время 281000
    if (ssm && route->legs.front().s.dep.hours() >= 24) {
        return Message(STDLOG, _("Date shift in first point of route is not allowed"));
    }

    ssim::ScdPeriod pr(flt);
    pr.period = prd;
    pr.route = *route;
    pr.adhoc = (!ssm);

    if (attr.timeIsLocal) {
        ssim::ScdPeriods scps { pr };
        CALL_MSG_RET(applyEssentials(scps, fi, per, legs, segs, attr));
        return scps;
    }

    ssim::ScdPeriods out = tmConv(pr);
    CALL_MSG_RET(applyEssentials(out, fi, per, legs, segs, attr));
    return out;
}

namespace details {

Expected<ssim::PeriodicCshs> getCshContextEqtCon(
        const ssim::ScdPeriod& scp, const ProcContext& attr,
        bool invertedScd, const boost::optional<PartnerInfo>& oprDis
    )
{
    if (attr.inventoryHost) {
        const boost::optional<ct::Flight> oprFlt = ssim::operatingFlight(scp.route);
        if (oprDis) {
            // некий 2/ указан
            if (invertedScd) {
                //в реквизитах оператора никаких 2 быть не может
                return Message(STDLOG, _("Wrong operating company"));
            }

            if (!oprFlt) {
                //ничего не знаем про кодшер
                return Message(STDLOG, _("Wrong operating company"));
            }

            //в реквизитах маркетинга - должны совпадать с уже установленным оператором
            if (oprDis->company != oprFlt->airline) {
                return Message(STDLOG, _("Wrong operating company"));
            }
        }
        return attr.callbacks->cshSettingsByScd(scp);
    }
    return PeriodicCshs();
}

} //details

} //ssim
