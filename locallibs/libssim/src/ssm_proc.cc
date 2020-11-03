#include <serverlib/dates.h>
#include <serverlib/period_joiner.h>

#include "ssm_proc.h"
#include "ssim_utils.h"
#include "callbacks.h"

#define NICKNAME "ASH"
#include <serverlib/slogger.h>

namespace ssim { namespace details {

enum ScdPeriodType {
    Scd_Type_Any = 0,
    Scd_Type_Basic,
    Scd_Type_Adhoc
};

static ssim::ScdPeriods filterByType(const ssim::ScdPeriods& scds, ScdPeriodType type)
{
    if (type == Scd_Type_Any) {
        return scds;
    }

    const bool adhocs = (type == Scd_Type_Adhoc);
    ssim::ScdPeriods result;
    result.reserve(scds.size());
    for (const auto& s : scds) {
        if (s.adhoc == adhocs) {
            result.push_back(s);
        }
    }
    return result;
}
//#############################################################################
void updateCachedPeriods(
        CachedScdPeriods& ms, const ProcContext& attr, const bool timeIsLocal,
        const ct::Flight& flt, const Period& prd, bool xasm
    )
{
    // сначала определяем, покрывает ли текущий период из телеграммы набор модификаций уже сделанный по тлг (MarkedScdPeriods)
    // если нет, и разница между ними не пустая, то нужно дочитать информацию из базы
    LogTrace(TRACE5) << __FUNCTION__ << " " << flt.toString() << " " << prd << " " << xasm;

    Periods uncovered(1, prd);
    for (const MarkedScdPeriod& msp : ms.changes) {
        // из-за replaceForFreeSale & ssm-flt это возможно и нормально, когда
        // удаляется период одного рейса и сохраняется период другого
        if (flt != msp.scd.flight) {
            continue;
        }
        uncovered = Period::difference(uncovered, Periods(1, msp.scd.period.shift(ssim::getScdOffset(msp.scd, timeIsLocal))));
    }
    // получили чистую нормализованную разницу между новым и старым без пустых периодов
    // для них надо искать информацию из базы
    LogTrace(TRACE5) << uncovered.size() << " uncovered periods in marked vector by period " << prd;

    if (uncovered.empty() && xasm) {
        //в случае с xasm все равно надо еще почитать, но интересовать будут только adhoc
        LogTrace(TRACE5) << "search for adhoc in " << prd;

        const ssim::ScdPeriods clean = filterByType(
            ssim::details::getSchedulesUsingTimeMode(flt, prd, attr, timeIsLocal),
            Scd_Type_Adhoc
        );

        for (const ssim::ScdPeriod& scd : clean) {
            if (ms.forDeletion[flt].insert(scd).second) {
                LogTrace(TRACE1) << "append additional adhoc " << flt << ' ' << scd.period << " for deletion";
            }
        }
    }

    for (const Period & unc : uncovered) {
        LogTrace(TRACE5) << "search scd for " << unc;
        const ssim::ScdPeriods clean = filterByType(
            ssim::details::getSchedulesUsingTimeMode(flt, unc, attr, timeIsLocal),
            (xasm ? Scd_Type_Any : Scd_Type_Basic)
        );
        LogTrace(TRACE1) << "read " << clean.size() << " additional periods after filtering by xasm=" << xasm;
        for (const ssim::ScdPeriod& scd : clean) {
            // эти периоды будут затронуты => их безусловно надо будет удалить чтобы писать с чистого листа
            if (!ms.forDeletion[flt].insert(scd).second) {
                LogTrace(TRACE5) << scd.period << "; adhoc = " << scd.adhoc << " already marked for deletion";
                continue;
            }

            // xasm означает, что обрабатывается skd|new|rpl xasm
            // нерегулярные расписания банально удаляются без всяких минусов
            // и пересечений (которые важны для регулярных)
            if (scd.adhoc && xasm) {
                continue;
            }
            // для работы с регулярным расписанием сохраняем в кэше период, который
            // потом будем пересекать-вычитать и проч
            ms.changes.emplace_back(MarkedScdPeriod { CLEAN, scd });
        }
    }
    LogTrace(TRACE5) << ms;
}
//#############################################################################
// вспомогательная структура для modify'ев -- блок из пересечения и разницы между периодами расписания
struct ScdDiffHelper
{
    MarkedScdPeriods untouched; // msp - period
    MarkedScdPeriods inter;     // msp & period
    MarkedScdPeriods diffFlt;

    ScdDiffHelper(
            const MarkedScdPeriods& marked,
            const ct::Flight& flt, const Period& p,
            bool timeIsLocal
        )
    {
        // из-за fs & flt здесь хранятся разные (возможно) номера рейсов
        // все, что не совпадает с переданным параметром, трогать не надо
        MarkedScdPeriods msps;
        for (const MarkedScdPeriod& msp : marked) {
            (msp.scd.flight == flt ? msps : diffFlt).push_back(msp);
        }

        // разница msps - p == untouched
        for (const MarkedScdPeriod& msp : msps) {
            const boost::gregorian::days offset = getScdOffset(msp.scd, timeIsLocal);

            // старое-незатронутое сохраняем как есть, обновив периоды
            for (const Period& dp : join(Period::normalize(msp.scd.period.shift(offset) - p), false)) {
                untouched.push_back(msp);
                MarkedScdPeriod& m = untouched.back();
                m.scd.period = dp.shift(-offset);
                LogTrace(TRACE5) << "add untouched period " << m.act << " " << m.scd.period;
            }

            // пересечение
            const Period ip = (msp.scd.period.shift(offset) & p);
            if (!ip.empty()) {
                inter.push_back(msp);
                inter.back().scd.period = Period::normalize(ip.shift(-offset));
            }
        }
    }
};

static Message assumeScdExists(const MarkedScdPeriods& msps)
{
    if (std::none_of(msps.begin(), msps.end(), [] (const MarkedScdPeriod& x) { return x.act != DELETE; })) {
        LogTrace(TRACE1) << "nothing to replace, all is deleted";
        return Message(STDLOG, _("No schedules found"));
    }
    return Message();
}
//#############################################################################
DiffHandler::DiffHandler(const ct::Flight& f, const Period& p, const ProcContext& a)
    : flt(f), per(p), attr(a) {}

DiffHandler::~DiffHandler()
{}

Message DiffHandler::handleEmpty(MarkedScdPeriods&)
{
    return Message(STDLOG, _("Periods're not found"));
}

Message DiffHandler::handleOld(MarkedScdPeriods& res, const MarkedScdPeriods& untouched)
{
    res.insert(res.end(), untouched.begin(), untouched.end());
    return Message();
}

Message DiffHandler::handleNew(MarkedScdPeriods&)
{
    return Message();
}

Message DiffHandler::handle(CachedScdPeriods& cached, bool useTimeMode)
{
    if (cached.forDeletion.size() > 1) {
        // собираемся удалить более одного рейса (не периода, а именно рейса)
        return Message(STDLOG, _("Several flights in ssm : not supported"));
    }
    MarkedScdPeriods result;
    if (cached.changes.empty()) {
        CALL_MSG_RET(this->handleEmpty(result));
    } else {
        ScdDiffHelper diff(cached.changes, flt, per, useTimeMode ? attr.timeIsLocal : true);
        // сохраняем информацию по рейсам, которых мы сейчас не касаемся
        result.insert(result.end(), diff.diffFlt.begin(), diff.diffFlt.end());
        CALL_MSG_RET(this->handleOld(result, diff.untouched));
        CALL_MSG_RET(this->handleInter(result, diff.inter));
        CALL_MSG_RET(this->handleNew(result));
    }
    cached.changes.swap(result);
    if (cached.forSaving().size() > 2) {
        // обрезки удаляемого и новое дают 2 рейса. но не больше
        return Message(STDLOG, _("Several flights in ssm : not supported"));
    }
    LogTrace(TRACE5) << cached;
    return Message();
}
//#############################################################################
DiffHandlerNew::DiffHandlerNew(const ssim::ScdPeriod& tlg, const ProcContext& a)
    : DiffHandler(tlg.flight, tlg.period, a), tlgScd(tlg)
{}

Message DiffHandlerNew::handleEmpty(MarkedScdPeriods& res)
{
    LogTrace(TRACE5) << "adding new parts:" << INSERT << " " << tlgScd.flight << "/" << tlgScd.period;
    res.push_back(MarkedScdPeriod { INSERT, tlgScd });
    return Message();
}

// что делать с кусками, которые пересекаются с периодом
Message DiffHandlerNew::handleInter(MarkedScdPeriods&, const MarkedScdPeriods& msps)
{
    for (const MarkedScdPeriod& msp : msps) {
        if (msp.act != DELETE) {
            LogTrace(TRACE1) << "overlap: " << msp.scd.period << " of type " << msp.act;
            return Message(STDLOG, _("Periods' overlap %1%")).bind(Dates::ddmmrr(msp.scd.period.start));
        }
        // поскольку мы в пересечении, то по определению есть новый период
        // значит он будет добавлен и синхронизация на этом пересечении будет проведена
        // единственная разница - тип будет insert, а не update, но на данный момент это
        // не имеет значения
    }
    return Message();
}

// что делать с кусками периода, не покрывающими найденное расписание
// по умолчанию - ничего (для NEW: сохранить, основная задача)
Message DiffHandlerNew::handleNew(MarkedScdPeriods &res)
{
    LogTrace(TRACE5) << "adding new parts:" << INSERT << " " << tlgScd.flight << "/" << tlgScd.period;
    res.push_back(MarkedScdPeriod { INSERT, tlgScd });
    return Message();
}
//#############################################################################
DiffHandlerRpl::DiffHandlerRpl(const ssim::ScdPeriod& tlg, const ProcContext& a)
    : DiffHandler(tlg.flight, tlg.period, a), tlgScd(tlg)
{}

Message DiffHandlerRpl::handleEmpty(MarkedScdPeriods& res)
{
    LogTrace(TRACE5) << "adding new parts:" << UPDATE << " " << tlgScd.flight << "/" << tlgScd.period;
    res.push_back(MarkedScdPeriod { UPDATE, tlgScd });
    return Message();
}

// в силу каких то причин rpl не отваливалась, если ничего не было найдено
// такое поведение нужно сохранить
// так что фактически rpl обрабатывается как new, которое уничтожает попутно все перекрытия периодов
Message DiffHandlerRpl::handleInter(MarkedScdPeriods&, const MarkedScdPeriods&)
{
    return Message();
}

Message DiffHandlerRpl::handleNew(MarkedScdPeriods &res)
{
    LogTrace(TRACE5) << "adding new parts:" << UPDATE << " " << tlgScd.flight << "/" << tlgScd.period;
    res.push_back(MarkedScdPeriod { UPDATE, tlgScd });
    return Message();
}
//#############################################################################
DiffHandlerCnl::DiffHandlerCnl(const ct::Flight& f, const Period& p, bool x, const ProcContext& a)
    : DiffHandler(f, p, a), xasm(x)
{}

Message DiffHandlerCnl::handleEmpty(MarkedScdPeriods&)
{
    return Message();
}

Message DiffHandlerCnl::handleInter(MarkedScdPeriods& res, const MarkedScdPeriods& msps)
{
    for (const MarkedScdPeriod & msp : msps) {
        LogTrace(TRACE5) << "adding parts:" << DELETE << " " << msp.scd.flight << "/" << msp.scd.period;
        res.emplace_back(MarkedScdPeriod { DELETE, msp.scd });
    }
    return Message();
}

Message DiffHandlerCnl::handleNew(MarkedScdPeriods&)
{
    return Message();
}
//#############################################################################
DiffHandlerTim::DiffHandlerTim(bool inv, const ssim::SsmTimStuff& s, const ssim::SegmentInfoList& sl, const ct::Flight& f, const ProcContext& a)
    : DiffHandler(f, s.pi.period, a), stuff(s), slist(sl), invertedScd(inv)
{}

Message DiffHandlerTim::handleInterInner(MarkedScdPeriods& res, const MarkedScdPeriod& msp)
{
    if (msp.act == DELETE) {
        res.push_back(msp);
        return Message();
    }

    ssim::details::SegmentInfoCleaner sic;
    const ssim::ScdPeriod& inter = msp.scd;

    CALL_EXP_RET(r, ssim::route_makers::applyTim(inter, stuff.legs, slist, attr, invertedScd));

    for (const ssim::PeriodicRoute& proute : *r) {
        ssim::ScdPeriod scp(inter);
        scp.period = proute.first;
        scp.route = proute.second;
        sic.getUnhandled(scp.auxData, slist);

        LogTrace(TRACE5) << "adding intersected part:" << UPDATE << " " << scp.flight << "/" << scp.period;
        res.push_back(MarkedScdPeriod { UPDATE, scp });
    }
    return Message();
}

Message DiffHandlerTim::handleInter(MarkedScdPeriods& res, const MarkedScdPeriods& msps)
{
    CALL_MSG_RET(assumeScdExists(msps));
    for (const MarkedScdPeriod& msp : msps) {
        CALL_MSG_RET(handleInterInner(res, msp));
    }
    return Message();
}
//#############################################################################
DiffHandlerEqtCon::DiffHandlerEqtCon(
        bool inv, const ssim::ProtoEqtStuff& ps, const ssim::SegmentInfoList& s,
        const boost::optional<PartnerInfo>& opr,
        const ct::Flight& f, const ProcContext& a
    )
    : DiffHandler(f, ps.pi.period, a), pes(ps), slist(s), oprDis(opr), invertedScd(inv)
{}

Message DiffHandlerEqtCon::handleInterInner(MarkedScdPeriods& res, const MarkedScdPeriod& msp)
{
    if (msp.act == DELETE) {
        res.push_back(msp);
        return Message();
    }

    const ssim::ScdPeriod& inter = msp.scd;

    ssim::details::SegmentInfoCleaner sic;

    const boost::optional<nsi::Points> legs = (
        pes.legs ? boost::make_optional(pes.legs->points)
                 : boost::none
    );

    CALL_EXP_RET(r, ssim::route_makers::applyEqtCon(inter, pes.eqi, slist, legs, attr, oprDis, invertedScd));

    for (const ssim::PeriodicRoute& proute : *r) {
        ssim::ScdPeriod scp(inter);
        scp.period = proute.first;
        scp.route = proute.second;
        sic.getUnhandled(scp.auxData, slist);

        LogTrace(TRACE5) << "modifed scd:" << scp.route;
        res.push_back(MarkedScdPeriod { UPDATE, scp });
    }
    return Message();
}

Message DiffHandlerEqtCon::handleInter(MarkedScdPeriods& res, const MarkedScdPeriods& msps)
{
    CALL_MSG_RET(assumeScdExists(msps));
    for (const MarkedScdPeriod& msp : msps) {
        CALL_MSG_RET(handleInterInner(res, msp));
    }
    return Message();
}
//#############################################################################
DiffHandlerFlt::DiffHandlerFlt(
        bool inv,
        const ct::Flight& nf, const ct::Flight& of, const ssim::SegmentInfoList& s,
        const ct::Flight& f, const Period& p, const ProcContext& a
    )
    : DiffHandler(f, p, a), newFlt(nf), oldFlt(of), slist(s), invertedScd(inv)
{}

static bool is_matched(const ssim::Leg& lg, const ssim::SegmentInfo& seg)
{
    return (lg.s.from == seg.dep || seg.dep.get() == 0)
        && (lg.s.to == seg.arr || seg.arr.get() == 0);
}

static bool conflictingEntries(const ssim::Leg& lg, const ssim::SegmentInfo& seg)
{
    if (!seg.oprFl) {
        return false;
    }
    LogTrace(TRACE5) << __FUNCTION__ << " " << (static_cast<bool>(lg.oprFlt) ? lg.oprFlt->toTlgStringUtf() : "n/a");
    LogTrace(TRACE5) << __FUNCTION__ << " " << seg.oprFl->toTlgStringUtf();

    return is_matched(lg, seg) && (lg.oprFlt && lg.oprFlt != seg.oprFl);
}

static Message ssmFltCheckCsh(
        bool invertedScd, const ssim::ScdPeriod& scp,
        const ct::Flight& tflt, const ssim::SegmentInfoList& segs
    )
{
    if (!invertedScd) {
        // нужно убедиться, что кш в тлг (если есть) соответствует расписанию
        // никаких подмен, все как есть, так и проверяем
        for (const ssim::Leg& lg : scp.route.legs) {
            if (std::any_of(segs.begin(), segs.end(), std::bind(conflictingEntries, lg, std::placeholders::_1))) {
                return Message(STDLOG, _("Conflicting operators"));
            }
        }
    } else {
        if (std::any_of(segs.begin(), segs.end(), [] (const ssim::SegmentInfo& x) { return static_cast<bool>(x.oprFl); } )) {
            return Message(STDLOG, _("Incorrect duplicate leg cross reference"));
        }

        const bool hasMkts = std::any_of(segs.begin(), segs.end(), [] (const ssim::SegmentInfo& x) {
            return !x.manFls.empty();
        });

        for (const ssim::Leg& lg : scp.route.legs) {
            if (hasMkts) {
                auto si = std::find_if(segs.begin(), segs.end(), [&lg, flt=scp.flight] (const ssim::SegmentInfo& x) {
                    return is_matched(lg, x)
                        && std::find(x.manFls.begin(), x.manFls.end(), flt) != x.manFls.end();
                });
                if (si == segs.end()) {
                    return Message(STDLOG, _("Different marketing flights"));
                }
            }
            if (lg.oprFlt != tflt) {
                return Message(STDLOG, _("Different operating flights"));
            }
        }
    }
    return Message();
}

Message DiffHandlerFlt::handleInterInner(MarkedScdPeriods& res, const MarkedScdPeriod& msp)
{
    if (msp.act == DELETE) {
        res.push_back(msp);
        return Message();
    }

    ssim::ScdPeriod inter(msp.scd);
    CALL_MSG_RET(ssmFltCheckCsh(invertedScd, inter, oldFlt, slist));

    if (!invertedScd) {
        inter.flight.number = newFlt.number;
        inter.flight.suffix = newFlt.suffix;
        LogTrace(TRACE1) << "number changed to " << inter.flight;
    } else {
        for (ssim::Leg& lg : inter.route.legs) {
            lg.oprFlt->number = newFlt.number;
            lg.oprFlt->suffix = newFlt.suffix;
            LogTrace(TRACE1) << "opr number changed to " << *lg.oprFlt;
        }
    }

    res.push_back(MarkedScdPeriod { UPDATE, inter });
    return Message();
}

Message DiffHandlerFlt::handleInter(MarkedScdPeriods& res, const MarkedScdPeriods& msps)
{
    CALL_MSG_RET(assumeScdExists(msps));

    for (const MarkedScdPeriod& msp : msps) {
        CALL_MSG_RET(handleInterInner(res, msp));
    }
    return Message();
}
//#############################################################################
DiffHandlerAdm::DiffHandlerAdm(
        bool inv, const ssim::SegmentInfoList& sl, const ct::Flight& f, const Period& p, const ProcContext& a
    )
    : DiffHandler(f, p, a), invertedScd(inv), slist(sl)
{}

Message DiffHandlerAdm::handleInterInner(MarkedScdPeriods& res, const MarkedScdPeriod& msp)
{
    if (msp.act == DELETE) {
        res.push_back(msp);
        return Message();
    }

    ssim::details::SegmentInfoCleaner sic;

    const ssim::ScdPeriod& inter = msp.scd;

    CALL_EXP_RET(r, ssim::route_makers::applyAdm(inter, slist, attr, invertedScd));

    for (const ssim::PeriodicRoute& proute : *r) {
        ssim::ScdPeriod scp(inter);
        scp.period = proute.first;
        scp.route = proute.second;
        sic.getUnhandled(scp.auxData, slist);

        LogTrace(TRACE5) << "adding intersected part:" << UPDATE << " " << scp.flight << "/" << scp.period;
        res.push_back(MarkedScdPeriod { UPDATE, scp });
    }
    return Message();
}

Message DiffHandlerAdm::handleInter(MarkedScdPeriods& res, const MarkedScdPeriods& msps)
{
    CALL_MSG_RET(assumeScdExists(msps));

    for (const MarkedScdPeriod& msp : msps) {
        CALL_MSG_RET(handleInterInner(res, msp));
    }
    return Message();
}
//#############################################################################
DiffHandlerRev::DiffHandlerRev(const ssim::PeriodInfoList& ps, const ct::Flight& f, const Period& p, const ProcContext& a)
    : DiffHandler(f, p, a), plist(ps) {}

Message DiffHandlerRev::handleInter(MarkedScdPeriods& res, const MarkedScdPeriods& msps)
{
    auto mit = std::find_if(msps.begin(), msps.end(), [] (const MarkedScdPeriod& x) {
        return x.act != DELETE;
    });
    if (mit == msps.end()) {
        // что мы собрались заменять, если все удалено?
        return Message(STDLOG, _("Periods're not found"));
    }

    base = *mit;
    // rev - это просто изменение периода (дат) с возможным тиражированием
    // особенность в том, что если применяется к сильно дробленому расписанию (>1 периода накрывается)
    // то не очень понятно (в общем случае), как собственно эти периоды тиражировать
    // поэтому эта ситуация требует отдельного внимания
    // т.е. если уже что-то где-то встречали, то маршрут НЕ должен меняться
    for (size_t i = 1; i < msps.size(); ++i) {
        if (msps[i].scd.route.legs != msps[i-1].scd.route.legs) {
            return Message(STDLOG, _("Craft/rbds/route are changing on period"));
        }
    }
    // информацию об удаляемых периодах нужно оставить, поскольку мы заменяем то, с чем мы пересеклись
    // на что-то абсолютно (в общем случае) из других мест
    std::copy_if(msps.begin(), msps.end(), std::back_inserter(res),
        [] (const MarkedScdPeriod& x) { return x.act == DELETE; }
    );
    return Message();
}

Message DiffHandlerRev::handleNew(MarkedScdPeriods& res)
{
    for (const ssim::PeriodInfo& pi : plist) {
        MarkedScdPeriod tmp(*base);
        tmp.scd.period = Period::normalize(pi.period.shift(-getScdOffset(base->scd, attr.timeIsLocal)));
        tmp.act = INSERT;
        res.push_back(tmp);
    }
    return Message();
}


} } //ssim::details
