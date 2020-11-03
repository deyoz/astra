#include <serverlib/dates.h>
#include <nsi/time_utils.h>
#include <coretypes/message_binders.h>

#include "asm_data_types.h"
#include "ssm_data_types.h"
#include "ssim_proc.h"
#include "dei_handlers.h"
#include "callbacks.h"
#include "ssim_utils.h"
#include "ssim_create.h"

#define NICKNAME "ASH"
#include <serverlib/slogger.h>

namespace {

static Expected<ssim::ScdPeriods> getSchedulesUsingTimeMode(
        const ct::Flight& flt, const boost::gregorian::date& dt,
        const ssim::ProcContext& attr
    )
{
    const ssim::ScdPeriods scps = ssim::details::getSchedulesUsingTimeMode(flt, Period(dt, dt), attr, attr.timeIsLocal);
    if (scps.empty()) {
        return scps;
    }

    ssim::ScdPeriods bases, adhocs;
    for (const ssim::ScdPeriod& scp : scps) {
        (scp.adhoc ? adhocs : bases).push_back(scp);
    }
    if (bases.size() > 1 || adhocs.size() > 1) {
        return Message(STDLOG, _("Cannot choose schedule for replacement %1% %2%"))
            .bind(flt).bind(dt);
    }
    return ssim::ScdPeriods { (adhocs.empty() ? bases : adhocs).front() };
}

static ssim::ScdPeriod& addRef(ssim::ScdPeriods& res, const Period& p, bool timeIsLocal)
{
    ASSERT(res.size() == 1);
    if (!res.front().adhoc) {
        const boost::gregorian::days offset(
            timeIsLocal ? 0 : ssim::getUtcOffset(res.front()).days()
        );

        ssim::ScdPeriod adh(res.front());
        adh.period = p.shift(-offset);
        adh.adhoc = true;
        res.push_back(adh);
    }
    return res.back();
}

static ssim::ScdPeriods convertToLocalAsm(const ssim::ScdPeriod& scp, const boost::gregorian::date& refDt)
{
    ssim::ScdPeriod out = scp;

    const boost::posix_time::ptime anchor(refDt);
    for (ssim::Leg& lg : out.route.legs) {
        lg.s.dep = nsi::timeGmtToCity(boost::posix_time::ptime(out.period.start, lg.s.dep), lg.s.from) - anchor;
        lg.s.arr = nsi::timeGmtToCity(boost::posix_time::ptime(out.period.start, lg.s.arr), lg.s.to) - anchor;
    }
    out.period = Period(refDt, refDt, Freq(refDt, refDt));
    return { out };
}

static std::string cnlRinString(const ssim::FlightIdInfo& fid)
{
    std::stringstream str;
    str << fid;
    return str.str();
}

static std::string fltAdmString(const ssim::FlightIdInfo& fid, const ssim::SegmentInfoList& sl)
{
    std::stringstream str;
    str << fid << "\n";
    for (const ssim::SegmentInfo& si :sl) {
        str << si << "\n";
    }
    return str.str();
}

static bool needDate(const ssim::RoutingInfoList& rl)
{
    for (const ssim::RoutingInfo & ri :rl) {
        int ds = Dates::daysOffset(ri.depTime).days(),
            as = Dates::daysOffset(ri.arrTime).days();
        if (ds != 0 || as != 0)
            return true;
    }
    return false;
}


} //anonymous


namespace ssim {

static ssim::AsmSubmsgPtr createProtoNew(ssim::AsmActionType type, const ssim::ScdPeriod& scp, const ssim::PubOptions& opts)
{
    ASSERT(type == ssim::ASM_NEW || type == ssim::ASM_RPL);

    ssim::FlightIdInfo fi = { ct::FlightDate { scp.flight, scp.period.start }, boost::none };
    ssim::LegStuffList legs;
    ssim::SegmentInfoList segs;
    ssim::details::setupSsimData(scp, fi, legs, segs, opts, ssim::details::getDeiSubset(type));

    if (type == ssim::ASM_NEW) {
        return std::make_shared<ssim::AsmNewSubmsg>(fi, legs, segs);
    }
    return std::make_shared<ssim::AsmRplSubmsg>(fi, legs, segs);
}

static void setupSegmentInformation(
        ssim::SegmentInfoList& sil,
        const ssim::Route& route, const std::vector<ct::LegNum>& lns,
        const ssim::SegmentInfoList& all
    )
{
    for (const ssim::SegmentInfo& si : all) {
        for (ct::SegNum sn : ssim::details::findSegs(route, si.dep, si.arr)) {
            const std::vector<ct::LegNum> lgs = ct::legsBySeg(sn);
            const bool cross = std::any_of(lns.begin(), lns.end(), [&lgs] (ct::LegNum x) {
                return lgs.end() != std::find(lgs.begin(), lgs.end(), x);
            });
            if (cross) {
                sil.push_back(si);
                break;
            }
        }
    }
}

static std::vector<ssim::AsmSubmsgPtr> createEqtCon(
        ssim::AsmActionType type, const ssim::ScdPeriod& scp, const ssim::PubOptions& opts
    )
{
    ASSERT(type == ssim::ASM_EQT || type == ssim::ASM_CON);

    //prepare full data set as for NEW submessage
    ssim::FlightIdInfo fi = { ct::FlightDate { scp.flight, scp.period.start }, boost::none };
    ssim::LegStuffList legs;
    ssim::SegmentInfoList segs;
    ssim::details::setupSsimData(scp, fi, legs, segs, opts, ssim::details::getDeiSubset(type));

    //and transform it
    //FLCI in ASM is part of Flight information and therefore part of the identifier
    //so if equpment information is different we should generate several ASM

    enum { FLI_ = 0, EQI_, LGS_ };
    std::vector< std::tuple<ssim::FlightIdInfo, ssim::EquipmentInfo, std::vector<ct::LegNum> > > grps;

    unsigned ln = 0;
    for (const ssim::LegStuff& lg : legs) {
        ++ln;
        if (grps.empty() || !(std::get<EQI_>(grps.back()) == lg.eqi && std::get<EQI_>(grps.back()).di == lg.ri.di)) {
            grps.emplace_back(fi, lg.eqi, std::vector<ct::LegNum> { ct::LegNum(ln - 1) });

            auto& v = grps.back();
            std::get<FLI_>(v).points = nsi::Points {lg.ri.dep, lg.ri.arr};
            std::get<EQI_>(v).di = lg.ri.di;
            continue;
        }

        auto& v = grps.back();

        ASSERT(std::get<FLI_>(v).points->back() == lg.ri.dep);
        std::get<FLI_>(v).points->push_back(lg.ri.arr);
        std::get<LGS_>(v).push_back(ct::LegNum(ln - 1));
    }

    if (grps.size() == 1) {
        //entire route - skip FLCI
        std::get<FLI_>(grps.front()).points = boost::none;
    }

    std::vector<ssim::AsmSubmsgPtr> out;
    for (const auto& grp : grps) {
        ssim::SegmentInfoList grpSegs;
        setupSegmentInformation(grpSegs, scp.route, std::get<LGS_>(grp), segs);

        if (type == ssim::ASM_EQT) {
            out.push_back(std::make_shared<ssim::AsmEqtSubmsg>(std::get<FLI_>(grp), std::get<EQI_>(grp), grpSegs));
        } else {
            out.push_back(std::make_shared<ssim::AsmConSubmsg>(std::get<FLI_>(grp), std::get<EQI_>(grp), grpSegs));
        }
    }
    return out;
}

FlightIdInfo::FlightIdInfo(
        const ct::FlightDate& f,
        const boost::optional<ct::FlightDate>& nf, const ssim::RawDeiMap& ds
    )
    : DeiInfo(ds), fld(f), newFld(nf)
{}

std::ostream& operator << (std::ostream& os, const FlightIdInfo& fi)
{
    os << fi.fld.flt.toTlgStringUtf() << "/" << Dates::ddmonrr(fi.fld.dt, ENGLISH);
    if (fi.points) {
        os << " " << ssim::LegChangeInfo(*fi.points);
    }
    if (fi.newFld) {
        os << " " << fi.newFld->flt.toTlgStringUtf() << "/" << Dates::ddmonrr(fi.newFld->dt, ENGLISH);
    }
    os << static_cast<const ssim::DeiInfo&>(fi);
    return os;
}

//#############################################################################
AsmSubmsg::AsmSubmsg(const FlightIdInfo& f)
    : fid(f)
{}

AsmSubmsg::AsmSubmsg(const ct::Flight& f)
    : fid(ct::FlightDate { f, boost::gregorian::date() }, boost::none)
{}

AsmSubmsg::~AsmSubmsg()
{}

Period AsmSubmsg::period() const
{
    return Period::normalize(Period(fid.fld.dt, fid.fld.dt));
}

bool AsmSubmsg::isInverted(const ProcContext& attr) const
{
    return attr.inventoryHost && attr.ourComp != fid.fld.flt.airline;
}

Expected<ssim::ScdPeriods> AsmSubmsg::getOldScd(const ProcContext& attr) const
{
    // в общем случае определяем маркетинговый рейс (из тлг или по базе), не найти нельзя
    // затем расписание - тоже должно существовать
    Expected<ct::Flight> act = ssim::getActFlight(attr, this->fid.fld.flt, this->period());
    if (!act) {
        return act.err();
    }
    auto scps = getSchedulesUsingTimeMode(*act, this->fid.fld.dt, attr);
    if (!scps) {
        return scps.err();
    }
    if (scps->empty()) {
        return Message(STDLOG, _("Schedule is not found %1% %2%"))
            .bind(fid.fld.flt).bind(fid.fld.dt);
    }
    return *scps;
}
//#############################################################################
AsmProtoNewSubmsg::AsmProtoNewSubmsg(const FlightIdInfo& fid, const ssim::LegStuffList& l, const ssim::SegmentInfoList& s)
    : AsmSubmsg(fid), stuff(l), segs(s)
{}

std::string AsmProtoNewSubmsg::toString() const
{
    std::stringstream res;
    res << ssim::enumToStr(type()) << std::endl;
    res << fid << std::endl;

    // если где-либо есть сдвиг даты, мы протащим в вывод ssim::RoutingInfo дату рейда
    boost::gregorian::date dt;
    for (const ssim::LegStuff& ls :stuff) {
        if (Dates::daysOffset(ls.ri.depTime).days() != 0 || Dates::daysOffset(ls.ri.arrTime).days() != 0) {
            dt = this->fid.fld.dt;
            break;
        }
    }

    std::string prevEq = "";
    for (const ssim::LegStuff& ls :stuff) {
        std::string eq = HelpCpp::string_cast(ls.eqi);
        if (eq != prevEq) {
            res << eq << std::endl;
            prevEq = eq;
        }
        res << ls.ri.toString(false, dt) << std::endl;
    }

    for (const ssim::SegmentInfo & si :segs) {
        res << si << std::endl;
    }
    return res.str();
}
//#############################################################################
static Expected<ct::Flight> getOurFlight(
        const ProcContext& attr, const FlightIdInfo& fid,
        const LegStuffList& legs, const SegmentInfoList& segs
    )
{
    ct::Flight act = fid.fld.flt;

    if (attr.inventoryHost && attr.ourComp != fid.fld.flt.airline) {
        //NEW/RPL подразумевает полный набор данных
        //опираться на существующие расписания для определения нашего рейса нельзя
        //надо достать его из тлг

        std::set<ct::Flight> mkts;
        for (const ssim::LegStuff& lg : legs) {
            const nsi::DepArrPoints dap(lg.ri.dep, lg.ri.arr);

            auto si = std::find_if(segs.begin(), segs.end(), [&dap] (const ssim::SegmentInfo& si) {
                return (si.dep.get() == 0 || si.dep == dap.dep)
                    && (si.arr.get() == 0 || si.arr == dap.arr)
                    && (si.deiType.get() == 10);
            });

            if (si != segs.end()) {
                for (const ct::Flight& f : si->manFls) {
                    if (f.airline == attr.ourComp) {
                        mkts.emplace(f);
                    }
                }
            }
        }
        if (mkts.empty()) {
            return Message(STDLOG, _("No information about marketing partner"));
        }
        if (mkts.size() != 1) {
            return Message(STDLOG, _("Different marketing leg duplications"));
        }
        act = *mkts.begin();
    }
    return act;
}
//#############################################################################
AsmNewSubmsg::AsmNewSubmsg(const FlightIdInfo& fid, const ssim::LegStuffList& l, const ssim::SegmentInfoList& s)
    : AsmProtoNewSubmsg(fid, l, s)
{}

ssim::AsmActionType AsmNewSubmsg::type() const
{
    return ssim::ASM_NEW;
}

Expected<ssim::ScdPeriods> AsmNewSubmsg::getOldScd(const ProcContext& attr) const
{
    CALL_EXP_RET(act, getOurFlight(attr, fid, stuff, segs));

    auto scps = getSchedulesUsingTimeMode(*act, this->fid.fld.dt, attr);
    if (!scps) {
        return scps.err();
    }
    // если что-то нашли, то это обязательно отмененный Adhoc
    if (!scps->empty() && !(scps->front().adhoc && scps->front().cancelled)) {
        return Message(STDLOG, _("Schedule already exists"));
    }
    return *scps;
}

Message AsmNewSubmsg::modifyScd(ssim::ScdPeriods& res, const ProcContext& attr) const
{
    const ssim::PeriodInfo pi(this->period());

    ssim::details::SegmentInfoCleaner sic;

    if (res.empty()) {
        //никаких расписаний еще не было
        Expected<ssim::ScdPeriods> scd = ssim::makeScdPeriod(
            fid.fld.flt, pi, stuff, segs, attr, ssim::toLocalTime, false
        );
        if (!scd) {
            return scd.err();
        }
        ASSERT(scd->size() == 1);

        ssim::ScdPeriod& scp = scd->front();
        sic.getUnhandled(scp.auxData, segs);
        res.push_back(scp);
    } else {
        //был отмененный adhoc на этом месте (раньше проверили), заменяем его
        ssim::ScdPeriod& scp = addRef(res, pi.period, attr.timeIsLocal);
        ASSERT(scp.adhoc && scp.cancelled);
        Expected<ssim::ScdPeriods> scd = ssim::makeScdPeriod(
            fid.fld.flt, pi, stuff, segs, attr,
            std::bind(convertToLocalAsm, std::placeholders::_1, std::cref(scp.period.start)),
            false
        );
        if (!scd) {
            return scd.err();
        }
        ASSERT(scd->size() == 1);
        scp = scd->front();
        sic.getUnhandled(scp.auxData, segs);
    }
    return Message();
}

AsmSubmsgPtr AsmNewSubmsg::create(const ssim::ScdPeriod& scp, const ssim::PubOptions& opts)
{
    return createProtoNew(ssim::ASM_NEW, scp, opts);
}
//#############################################################################
AsmRplSubmsg::AsmRplSubmsg(const FlightIdInfo& fid, const ssim::LegStuffList& l, const ssim::SegmentInfoList& s)
    : AsmProtoNewSubmsg(fid, l, s)
{}

ssim::AsmActionType AsmRplSubmsg::type() const
{
    return ssim::ASM_RPL;
}

Expected<ssim::ScdPeriods> AsmRplSubmsg::getOldScd(const ProcContext& attr) const
{
    CALL_EXP_RET(act, getOurFlight(attr, fid, stuff, segs));

    auto scps = getSchedulesUsingTimeMode(*act, this->fid.fld.dt, attr);
    if (!scps) {
        return scps.err();
    }

    if (scps->empty()) {
        return Message(STDLOG, _("Schedule is not found %1% %2%"))
            .bind(fid.fld.flt).bind(fid.fld.dt);
    }
    return *scps;
}

Message AsmRplSubmsg::modifyScd(ssim::ScdPeriods& res, const ProcContext& attr) const
{
    if (res.empty()) {
        return Message(STDLOG, _("Schedule is not found %1% %2%"))
            .bind(fid.fld.flt).bind(fid.fld.dt);
    }

    ssim::details::SegmentInfoCleaner sic;

    ssim::ScdPeriod& scp = addRef(res, this->period(), attr.timeIsLocal);
    if (scp.cancelled) {
        return Message(STDLOG, _("Flight %1% doesn't operate on %2%"))
            .bind(scp.flight).bind(this->fid.fld.dt);
    }

    Expected<ssim::ScdPeriods> rpl = ssim::makeScdPeriod(
        fid.fld.flt, ssim::PeriodInfo(this->period()), stuff, segs, attr,
        std::bind(convertToLocalAsm, std::placeholders::_1, std::cref(scp.period.start)),
        false
    );
    if (!rpl) {
        return rpl.err();
    }
    ASSERT(rpl->size() == 1);
    scp = rpl->front();
    sic.getUnhandled(scp.auxData, segs);
    return Message();
}

AsmSubmsgPtr AsmRplSubmsg::create(const ssim::ScdPeriod& scp, const ssim::PubOptions& opts)
{
    return createProtoNew(ssim::ASM_RPL, scp, opts);
}
//#############################################################################
AsmCnlSubmsg::AsmCnlSubmsg(const FlightIdInfo& fid)
    : AsmSubmsg(fid)
{}

ssim::AsmActionType AsmCnlSubmsg::type() const
{
    return ssim::ASM_CNL;
}

std::string AsmCnlSubmsg::toString() const
{
    return "CNL\n" + cnlRinString(this->fid) + "\n";
}

Expected<ssim::ScdPeriods> AsmCnlSubmsg::getOldScd(const ProcContext& attr) const
{
    // для CNL можно и ничего не найти
    Expected<ct::Flight> act = ssim::getActFlight(attr, this->fid.fld.flt, this->period(), true);
    if (!act) {
        if (act.err()) {
            return act.err();
        }
        return ssim::ScdPeriods();
    }

    auto scps = getSchedulesUsingTimeMode(*act, this->fid.fld.dt, attr);
    if (!scps) {
        return scps.err();
    }
    if (scps->empty()) {
        LogTrace(TRACE1) << "scd for " << *act << " " << fid.fld.dt << " not found in cnl";
    }
    return *scps;
}

Message AsmCnlSubmsg::modifyScd(ssim::ScdPeriods& res, const ProcContext& attr) const
{
    if (res.empty()) {
        return Message(); // нечего отменять - это не ошибка, по крайней мере в сирене так
    }

    ssim::ScdPeriod& scd = addRef(res, this->period(), attr.timeIsLocal);
    if (scd.cancelled) {
        return Message(STDLOG, _("Flight %1% doesn't operate on %2%"))
            .bind(scd.flight).bind(fid.fld.dt);
    }
    scd.cancelled = true;
    return Message();
}
//#############################################################################
AsmRinSubmsg::AsmRinSubmsg(const FlightIdInfo& fid)
    : AsmSubmsg(fid)
{}

ssim::AsmActionType AsmRinSubmsg::type() const
{
    return ssim::ASM_RIN;
}

std::string AsmRinSubmsg::toString() const
{
    return "RIN\n" + cnlRinString(this->fid) + "\n";
}

Message AsmRinSubmsg::modifyScd(ssim::ScdPeriods& res, const ProcContext&) const
{
    if (res.empty()) {
        return Message(STDLOG, _("Schedule is not found %1% %2%"))
            .bind(fid.fld.flt).bind(fid.fld.dt);
    }
    if (!res.front().adhoc || !res.front().cancelled) {
        return Message(STDLOG, _("Can reinstate only cancelled adhoc"));
    }
    res.front().cancelled = false;
    return Message();
}
//#############################################################################
AsmFltSubmsg::AsmFltSubmsg(const FlightIdInfo& fid, const ssim::SegmentInfoList& sl)
    : AsmSubmsg(fid), segs(sl)
{}

ssim::AsmActionType AsmFltSubmsg::type() const
{
    return ssim::ASM_FLT;
}

std::string AsmFltSubmsg::toString() const
{
    return "FLT\n" + fltAdmString(this->fid, this->segs);
}

Message AsmFltSubmsg::modifyScd(ssim::ScdPeriods& res, const ProcContext& attr) const
{
    if (res.empty()) {
        return Message(STDLOG, _("Schedule is not found %1% %2%"))
            .bind(fid.fld.flt).bind(fid.fld.dt);
    }

    ssim::ScdPeriod& scd = addRef(res, this->period(), attr.timeIsLocal);
    if (fid.fld.flt.airline != fid.newFld->flt.airline) {
        return Message(STDLOG, _("Change of airline isn't allowed"));
    }

    if (isInverted(attr)) {
        return Message(STDLOG, _("Tlg for marketing company awaited"));
    }
    scd.flight = fid.newFld->flt;
    return Message();
}
//#############################################################################
AsmAdmSubmsg::AsmAdmSubmsg(const FlightIdInfo& fid, const ssim::SegmentInfoList& sl)
    : AsmSubmsg(fid), segs(sl)
{}

ssim::AsmActionType AsmAdmSubmsg::type() const
{
    return ssim::ASM_ADM;
}

std::string AsmAdmSubmsg::toString() const
{
    return "ADM\n" + fltAdmString(this->fid, this->segs);
}

Message AsmAdmSubmsg::modifyScd(ssim::ScdPeriods& res, const ProcContext& attr) const
{
    if (res.empty()) {
        return Message(STDLOG, _("Schedule is not found %1% %2%"))
            .bind(fid.fld.flt).bind(fid.fld.dt);
    }

    ssim::details::SegmentInfoCleaner sic;

    ssim::ScdPeriod& scd = addRef(res, this->period(), attr.timeIsLocal);

    CALL_EXP_RET(prs, ssim::route_makers::applyAdm(scd, segs, attr, isInverted(attr)));

    ASSERT(prs->size() == 1 && prs->front().first == scd.period);

    scd.route = prs->front().second;
    sic.getUnhandled(scd.auxData, segs);

    return Message();
}
//#############################################################################
AsmTimSubmsg::AsmTimSubmsg(const FlightIdInfo& fid, const ssim::RoutingInfoList& r, const ssim::SegmentInfoList& s)
    : AsmSubmsg(fid), legs(r), segs(s)
{}

ssim::AsmActionType AsmTimSubmsg::type() const
{
    return ssim::ASM_TIM;
}

std::string AsmTimSubmsg::toString() const
{
    std::stringstream res;
    res << "TIM\n";
    res << fid << "\n";
    const bool nd = needDate(legs);
    for (const ssim::RoutingInfo& ri : legs) {
        res << ri.toString(false, nd ? fid.fld.dt : boost::gregorian::date()) << "\n";
    }
    for (const ssim::SegmentInfo& si : segs) {
        res << si << "\n";
    }
    return res.str();
}

Message AsmTimSubmsg::modifyScd(ssim::ScdPeriods& res, const ProcContext& attr) const
{
    if (res.empty()) {
        return Message(STDLOG, _("Schedule is not found %1% %2%"))
            .bind(fid.fld.flt).bind(fid.fld.dt);
    }

    ssim::ScdPeriod& scd = addRef(res, this->period(), attr.timeIsLocal);
    if (scd.cancelled) {
        return Message(STDLOG, _("Flight %1% doesn't operate on %2%"))
            .bind(scd.flight).bind(fid.fld.dt);
    }

    ssim::RoutingInfoList rls = legs;
    if (!attr.timeIsLocal) {
        const boost::gregorian::date baseDt = this->period().start;
        const boost::posix_time::ptime anchor(scd.period.start);
        for (ssim::RoutingInfo& ri : rls) {
            ri.depTime = nsi::timeGmtToCity(boost::posix_time::ptime(baseDt, ri.depTime), ri.dep) - anchor;
            ri.arrTime = nsi::timeGmtToCity(boost::posix_time::ptime(baseDt, ri.arrTime), ri.arr) - anchor;
        }
    }

    ssim::details::SegmentInfoCleaner sic;

    CALL_EXP_RET(prs, ssim::route_makers::applyTim(scd, rls, segs, attr, isInverted(attr)));
    ASSERT(prs->size() == 1 && prs->front().first == scd.period);

    scd.route = prs->front().second;
    sic.getUnhandled(scd.auxData, segs);

    return Message();
}

AsmSubmsgPtr AsmTimSubmsg::create(const ssim::ScdPeriod& scp, const ssim::PubOptions& opts)
{
    ssim::FlightIdInfo fi = { ct::FlightDate { scp.flight, scp.period.start }, boost::none};
    ssim::RoutingInfoList legs;
    ssim::SegmentInfoList segs;

    ssim::LegStuffList lst;
    ssim::details::setupSsimData(scp, fi, lst, segs, opts, ssim::details::getDeiSubset(ssim::ASM_TIM));
    std::transform(lst.begin(), lst.end(), std::back_inserter(legs), [] (const ssim::LegStuff& x) { return x.ri; });
    return std::make_shared<ssim::AsmTimSubmsg>(fi, legs, segs);
}
//#############################################################################
AsmRrtSubmsg::AsmRrtSubmsg(const FlightIdInfo& fid, const boost::optional<ssim::EquipmentInfo>& eo, const ssim::RoutingInfoList& l, const ssim::SegmentInfoList& s)
    : AsmSubmsg(fid), equip(eo), legs(l), segs(s)
{}

ssim::AsmActionType AsmRrtSubmsg::type() const
{
    return ssim::ASM_RRT;
}

std::string AsmRrtSubmsg::toString() const
{
    std::stringstream res;
    res << "RRT\n";
    res << fid << "\n";
    if (equip) {
        res << *equip << "\n";
    }
    const bool nd = needDate(legs);
    for (const ssim::RoutingInfo& ri : legs) {
        res << ri.toString(false, nd ? fid.fld.dt : boost::gregorian::date()) << "\n";
    }
    for (const ssim::SegmentInfo& si : segs) {
        res << si << "\n";
    }
    return res.str();
}

Message AsmRrtSubmsg::modifyScd(ssim::ScdPeriods&, const ProcContext&) const
{
    //TODO: RRT весьма специфический тип, который скорее всего никогда приходил
    //      как оно должно быть вообразить проблематично
    //      в сирене был набросок обработки ее, но лучше поставим ее в очередь,
    //      чем молча обработаем неправильно

    return Message(STDLOG, _("Message type is yet to be supported"));
}
//#############################################################################
AsmProtoEqtSubmsg::AsmProtoEqtSubmsg(const FlightIdInfo& fid, const ssim::EquipmentInfo& eq, const ssim::SegmentInfoList& s)
    : AsmSubmsg(fid), equip(eq), segs(s)
{}

std::string AsmProtoEqtSubmsg::toString() const
{
    std::stringstream res;
    res << ssim::enumToStr(type()) << std::endl;
    res << fid << "\n";
    res << equip << "\n";
    for (const ssim::SegmentInfo& si : segs) {
        res << si << "\n";
    }
    return res.str();
}

Message AsmProtoEqtSubmsg::modifyScd(ssim::ScdPeriods& res, const ProcContext& attr) const
{
    if (res.empty()) {
        return Message(STDLOG, _("Schedule is not found %1% %2%"))
            .bind(fid.fld.flt).bind(fid.fld.dt);
    }

    ssim::details::SegmentInfoCleaner sic;
    ssim::ScdPeriod& scd = addRef(res, this->period(), attr.timeIsLocal);
    if (scd.cancelled) {
        return Message(STDLOG, _("Flight %1% doesn't operate on %2%"))
            .bind(scd.flight).bind(fid.fld.dt);
    }

    const auto oprDis = ssim::getPartnerInfo(ssim::AgreementType::CSH, { &fid });

    CALL_EXP_RET(routes, ssim::route_makers::applyEqtCon(scd, equip, segs, fid.points, attr, oprDis, isInverted(attr)));
    ASSERT(routes->size() == 1 && routes->front().first == scd.period);

    scd.route = routes->front().second;
    sic.getUnhandled(scd.auxData, segs);

    return Message();
}
//#############################################################################
AsmEqtSubmsg::AsmEqtSubmsg(const FlightIdInfo& fid, const ssim::EquipmentInfo& eq, const ssim::SegmentInfoList& s)
    : AsmProtoEqtSubmsg(fid, eq, s)
{}

ssim::AsmActionType AsmEqtSubmsg::type() const
{
    return ssim::ASM_EQT;
}

std::vector<AsmSubmsgPtr> AsmEqtSubmsg::create(const ssim::ScdPeriod& scp, const ssim::PubOptions& opts)
{
    return createEqtCon(ssim::ASM_EQT, scp, opts);

}
//#############################################################################
AsmConSubmsg::AsmConSubmsg(const FlightIdInfo& fid, const ssim::EquipmentInfo& eq, const ssim::SegmentInfoList& s)
    : AsmProtoEqtSubmsg(fid, eq, s)
{}

ssim::AsmActionType AsmConSubmsg::type() const
{
    return ssim::ASM_CON;
}

std::vector<AsmSubmsgPtr> AsmConSubmsg::create(const ssim::ScdPeriod& scp, const ssim::PubOptions& opts)
{
    return createEqtCon(ssim::ASM_CON, scp, opts);
}
//#############################################################################
AsmAckSubmsg::AsmAckSubmsg()
    : AsmSubmsg(ct::Flight(nsi::CompanyId(0), ct::FlightNum(1)))
{}

ssim::AsmActionType AsmAckSubmsg::type() const
{
    return ssim::ASM_ACK;
}

std::string AsmAckSubmsg::toString() const
{
    return "ACK";
}

Expected<ssim::ScdPeriods> AsmAckSubmsg::getOldScd(const ProcContext&) const
{
    return ssim::ScdPeriods();
}

Message AsmAckSubmsg::modifyScd(ssim::ScdPeriods&, const ProcContext&) const
{
    return Message();
}
//#############################################################################
AsmNacSubmsg::AsmNacSubmsg(const FlightIdInfo& fid, const std::string& ri)
    : AsmSubmsg(fid), rejectInfo(ri)
{}

ssim::AsmActionType AsmNacSubmsg::type() const
{
    return ssim::ASM_NAC;
}

std::string AsmNacSubmsg::toString() const
{
    return "NAC";
}

Expected<ssim::ScdPeriods> AsmNacSubmsg::getOldScd(const ProcContext&) const
{
    return ssim::ScdPeriods();
}

Message AsmNacSubmsg::modifyScd(ssim::ScdPeriods&, const ProcContext&) const
{
    return Message(STDLOG, _("NAC received: %1%")).bind(rejectInfo);
}
//#############################################################################
std::string AsmStruct::toString() const
{
    std::stringstream os;
    os << head.toString(false);

    for (const AsmSubmsgPtr& p : specials) {
        os << p->toString() << "//\n";
    }

    for (const AsmSubmsgPtr& p : submsgs) {
        os << p->toString() << "//\n";
    }

    const std::string out = os.str();
    if (!specials.empty() || !submsgs.empty()) {
        return out.substr(0, out.size() - 3);
    }
    return out;
}

} //ssim
