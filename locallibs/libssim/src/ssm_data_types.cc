#include <serverlib/dates.h>
#include <nsi/time_utils.h>
#include <coretypes/message_binders.h>

#include "ssm_data_types.h"
#include "ssim_utils.h"
#include "ssm_proc.h"
#include "ssim_create.h"
#include <serverlib/lngv_user.h>

#define NICKNAME "ASH"
#include <serverlib/slogger.h>

#define CALL_MSG_RET_PREF(fl, prd, expr) \
if (Message msg = (expr)) {\
    LogTrace(TRACE1) << msg;\
    std::string pref = Message(STDLOG, _("%1% %2%: ")).bind(fl).bind(prd).toString(UserLanguage::en_US());\
    std::string msgStr = pref + msg.toString(UserLanguage::en_US());\
    msg = Message(STDLOG, _("%1%")).bind(msgStr);\
    return msg;\
}

namespace ssim {

static std::string legChangeToString(const nsi::Points& points)
{
    std::string out;
    for (nsi::PointId pt : points) {
        out += nsi::Point(pt).code(ENGLISH).toUtf() + '/';
    }
    return out.substr(0, out.size() - 1);
}

LegChangeInfo::LegChangeInfo(const nsi::Points& p, const RawDeiMap& d)
    : DeiInfo(d), points(p)
{}

std::ostream& operator << (std::ostream& os, const LegChangeInfo& x)
{
    return os << legChangeToString(x.points);
}

PeriodInfo::PeriodInfo(const Period& p, const RawDeiMap& d)
    : DeiInfo(d), period(p)
{}

bool operator< (const PeriodInfo& x, const PeriodInfo& y)
{
    return x.period < y.period;
}

bool operator== (const PeriodInfo& x, const PeriodInfo& y)
{
    return x.period == y.period;
}

std::ostream& operator << (std::ostream& os, const PeriodInfo& pi)
{
    os << ssim::printDate(pi.period.start) << " "
       << ssim::printDate(pi.period.end) << " "
       << pi.period.freq.normalString();
    if (pi.period.biweekly)
        os << "/W2";
    os << static_cast<const DeiInfo &>(pi);
    return os;
}

std::ostream& operator << (std::ostream& os, const SkdPeriodInfo& si)
{
    os << ssim::printDate(si.start) << " " << ssim::printDate(si.end);
    return os;
}

RevFlightInfo::RevFlightInfo(const ct::Flight& f, const PeriodInfo& p, const RawDeiMap& d)
    : FlightInfo(f, d), pi(p)
{}

std::ostream& operator << (std::ostream& os, const ProtoEqtStuff& s)
{
    os << s.pi;
    os << std::endl << s.eqi;
    if (s.legs) {
        os << std::endl << *s.legs;
    }
    return os;
}

std::ostream & operator << (std::ostream & os, const SsmTimStuff& ts)
{
    os << ts.pi;
    for (const RoutingInfo& r : ts.legs) {
        os << std::endl << r.toString();
    }
    return os;
}
//#############################################################################
typedef std::pair<std::string, std::string> str_pair;
typedef const std::string & (*keyFunc)(const str_pair &);
typedef std::string & (*valueFunc)(str_pair &);

const std::string & cgetFirst(const str_pair & p) { return p.first; }
const std::string & cgetSecond(const str_pair & p) { return p.second; }
std::string & getFirst(str_pair & p) { return p.first; }
std::string & getSecond(str_pair & p) { return p.second; }

static void collapseContainer(std::list<str_pair>& vc, keyFunc key, valueFunc value, bool stable)
{
    for (auto ref = vc.begin(); ref != vc.end(); ++ref) {
        for (auto i = std::next(ref); i != vc.end();) {
            if (key(*i) == key(*ref)) {
                value(*ref) += value(*i);
                i = vc.erase(i);
            } else {
                ++i;
                if (stable) {
                    break;
                }
            }
        }
    }
}
//#############################################################################
std::ostream& operator<< (std::ostream& os, const CachedScdPeriods& c)
{
    for (const auto& v : c.forDeletion) {
        for (const ssim::ScdPeriod& scp : v.second) {
            os << "delete: " << scp.flight << '/' << scp.period << ' ' << (scp.adhoc ? 'A' : 'B') << std::endl;
        }
    }
    for (const MarkedScdPeriod &m : c.changes) {
        os << "act=" << m.act << " " << m.scd.flight << "/" << m.scd.period << std::endl;
    }
    return os;
}

std::map<ct::Flight, ssim::ScdPeriods> CachedScdPeriods::forSaving() const
{
    std::map<ct::Flight, ssim::ScdPeriods> ret;
    for (const MarkedScdPeriod& msp : changes) {
        if (msp.act != DELETE) {
            ret[msp.scd.flight].push_back(msp.scd);
        }
    }
    ASSERT(ret.size() < 3); //0, 1 или 2 (обрезок + новое по flt | replace freesale)
    return ret;
}
//#############################################################################
static Message handlePeriodCancel(
        CachedScdPeriods& cached, const ProcContext& attr,
        const ct::Flight& origFlt, const Period& prd,
        ssim::SsmActionType type,
        bool xasm
    )
{
    auto actualFlt = getActFlight(attr, origFlt, prd, true);
    // может быть, расписания просто нет, и удалять нечего, поэтому и flt пуст - для этого разрешаем не найти инфо в бд
    if (!actualFlt) {
        if (actualFlt.err()) {
            CALL_MSG_RET_PREF(origFlt, prd, actualFlt.err());
        }
        return Message();
    }
    // дочитываем расписания в кэш
    details::updateCachedPeriods(cached, attr, attr.timeIsLocal, *actualFlt, prd, xasm);

    if (type == ssim::SSM_CNL) {
        cached.cancelled[*actualFlt].push_back(prd);
    }

    details::DiffHandlerCnl hdl(*actualFlt, prd, xasm, attr);
    CALL_MSG_RET_PREF(origFlt, prd, hdl.handle(cached, true));
    // что-то или удаляется или нет, в любом случае - все хорошо
    return Message();
}
//#############################################################################
static ssim::SsmSubmsgPtr createProtoNew(
        ssim::SsmActionType type, const ssim::ScdPeriod& scp, const Periods& ps,
        const ssim::PubOptions& opts, bool xasm
    )
{
    ASSERT(!scp.adhoc);
    ASSERT(type == ssim::SSM_NEW || type == ssim::SSM_RPL);

    const ssim::PeriodInfo pi(scp.period);

    ssim::FlightInfo fi(scp.flight);
    ssim::SsmProtoNewStuff stuff = { pi };
    ssim::SegmentInfoList segs;
    ssim::details::setupSsimData(scp, fi, stuff.legs, segs, opts, ssim::details::getDeiSubset(type));

    std::list<SsmProtoNewStuff> pns;
    for (const Period& p : ps) {
        const Period prd = Period::normalize(p & scp.period);
        if (prd.empty()) {
            continue;
        }
        pns.push_back(stuff);
        pns.back().pi.period = prd;
    }

    if (pns.empty()) {
        return ssim::SsmSubmsgPtr();
    }

    if (type == ssim::SSM_NEW) {
        return std::make_shared<ssim::SsmNewSubmsg>(xasm, fi, pns, segs);
    }
    return std::make_shared<ssim::SsmRplSubmsg>(xasm, fi, pns, segs);
}

static ssim::SsmSubmsgPtr createEqtCon(
        ssim::SsmActionType type, const ssim::ScdPeriod& scp,
        const Periods& ps, const ssim::PubOptions& opts
    )
{
    ASSERT(!scp.adhoc);
    ASSERT(type == ssim::SSM_EQT || type == ssim::SSM_CON);

    //prepare full data set as for NEW submessage
    ssim::FlightInfo fi(scp.flight);
    ssim::LegStuffList legs;
    ssim::SegmentInfoList segs;
    ssim::details::setupSsimData(scp, fi, legs, segs, opts, ssim::details::getDeiSubset(type));

    //and reduce it
    const ssim::PeriodInfo pi(scp.period);
    std::list<ssim::ProtoEqtStuff> stuff;
    for (const ssim::LegStuff& lg : legs) {
        if (stuff.empty() || !(stuff.back().eqi == lg.eqi && stuff.back().eqi.di == lg.ri.di)) {
            stuff.push_back(ssim::ProtoEqtStuff {
                pi, lg.eqi, ssim::LegChangeInfo({lg.ri.dep, lg.ri.arr})
            });
            stuff.back().eqi.di = lg.ri.di;
            continue;
        }

        ssim::LegChangeInfo& lci = *stuff.back().legs;
        ASSERT(lci.points.back() == lg.ri.dep);
        lci.points.push_back(lg.ri.arr);
    }

    if (stuff.size() == 1) {
        stuff.front().legs = boost::none;
    }

    //split by periods
    std::list<ssim::ProtoEqtStuff> stuffExt;
    for (const Period& p : ps) {
        const Period prd = Period::normalize(p & scp.period);
        if (prd.empty()) {
            continue;
        }
        for (const ssim::ProtoEqtStuff& pes : stuff) {
            stuffExt.push_back(pes);
            stuffExt.back().pi.period = prd;
        }
    }

    if (stuffExt.empty()) {
        return ssim::SsmSubmsgPtr();
    }
    if (type == ssim::SSM_EQT) {
        return std::make_shared<ssim::SsmEqtSubmsg>(fi, stuffExt, segs);
    }
    return std::make_shared<ssim::SsmConSubmsg>(fi, stuffExt, segs);
}
//#############################################################################
SsmSubmsg::SsmSubmsg(const ssim::FlightInfo& f)
    : fi(f)
{}

SsmSubmsg::~SsmSubmsg()
{}

bool SsmSubmsg::isInverted(const ssim::ProcContext& attr) const
{
    return attr.inventoryHost && attr.ourComp != fi.flt.airline;
}

SsmProtoNewSubmsg::SsmProtoNewSubmsg(bool x, const ssim::FlightInfo & fl, const std::list<ssim::SsmProtoNewStuff> & pl, const ssim::SegmentInfoList & sl)
    : SsmSubmsg(fl), xasm(x), stuff(pl), segs(sl)
{ }

std::string SsmProtoNewSubmsg::toString() const
{
    std::stringstream os;
    os << fi << std::endl;
    // при схлопывании дублирующихся строк будем оперировать именно строками, а не структурами
    // поэтому подготовим сначала почву
    std::list<str_pair> vc; // период - плечи с экипом
    for (const ssim::SsmProtoNewStuff & s : stuff) {
        // подготовим список пар экип-плечо и схлопнем по экипам
        std::list<str_pair> tmp;
        for (const ssim::LegStuff & ls : s.legs) {
            std::stringstream s1, s2;
            s1 << ls.eqi << std::endl;
            s2 << ls.ri.toString() << std::endl;
            tmp.emplace_back(s1.str(), s2.str());
        }
        collapseContainer(tmp, cgetFirst, getSecond, true);

        // теперь лепим получившиеся пары экип+плечо(и) к периоду
        std::stringstream p, l;
        p << s.pi << std::endl;
        for (const str_pair & pr : tmp) {
            l << pr.first << pr.second;
        }
        vc.emplace_back(p.str(), l.str());
    }
    // теперь схлопнем периоды по экип+плечи
    collapseContainer(vc, cgetSecond, getFirst, false);
    // теперь собственно вывод
    for (const str_pair & v : vc) {
        os << v.first << v.second;
    }
    for (const ssim::SegmentInfo & si : segs) {
        os << si << std::endl;
    }
    return os.str();
}

Periods SsmProtoNewSubmsg::getPeriods() const
{
    Periods ret;
    for (const ssim::SsmProtoNewStuff & st : stuff) {
        ret.push_back(st.pi.period);
    }
    return ret;
}
//#############################################################################
SsmNewSubmsg::SsmNewSubmsg(bool x, const ssim::FlightInfo & fl, const std::list<ssim::SsmProtoNewStuff> & pl, const ssim::SegmentInfoList & sl)
    : SsmProtoNewSubmsg(x, fl, pl, sl)
{}

ssim::SsmActionType SsmNewSubmsg::type() const
{
    return ssim::SSM_NEW;
}

std::string SsmNewSubmsg::toString() const
{
    return "NEW" + std::string(xasm ? " XASM\n" : "\n") + SsmProtoNewSubmsg::toString();
}

Message SsmNewSubmsg::modify(ssim::CachedScdPeriods& cached, const ssim::ProcContext& attr) const
{
    // для NEW предполагается, что мы сохраняем информацию поверх пустоты
    // иначе мы в неправильном состоянии и тлг не может быть обработана
    // значит, допустимы только две ситуации:
    // либо пересечений с cached нет
    // либо закэшированны удаляемые периоды DELETED
    LogTrace(TRACE5) << __FUNCTION__ << "-new";

    for (const ssim::SsmProtoNewStuff& st : stuff) {
        ssim::details::SegmentInfoCleaner sic;

        Expected<ssim::ScdPeriods> tlgScd = ssim::makeScdPeriod(
            fi, st.pi, st.legs, segs, attr, ssim::toLocalTime, true
        );
        if (!tlgScd) {
            CALL_MSG_RET_PREF(fi.flt, st.pi.period, tlgScd.err());
        }

        ssim::ExtraSegInfos esi;
        sic.getUnhandled(esi, segs);

        for (ssim::ScdPeriod& scp : *tlgScd) {
            scp.auxData = esi;

            details::updateCachedPeriods(cached, attr, true, scp.flight, scp.period, xasm);
            details::DiffHandlerNew hdl(scp, attr);
            CALL_MSG_RET_PREF(fi.flt, st.pi.period, hdl.handle(cached));
        }
    }
    return Message();
}

SsmSubmsgPtr SsmNewSubmsg::create(const ssim::ScdPeriod& scp, const Periods& ps, const ssim::PubOptions& opts, bool xasm)
{
    return createProtoNew(ssim::SSM_NEW, scp, ps, opts, xasm);
}
//#############################################################################
SsmRplSubmsg::SsmRplSubmsg(bool x, const ssim::FlightInfo & fl, const std::list<ssim::SsmProtoNewStuff> & pl, const ssim::SegmentInfoList & sl)
    : SsmProtoNewSubmsg(x, fl, pl, sl)
{}

ssim::SsmActionType SsmRplSubmsg::type() const
{
    return ssim::SSM_RPL;
}

std::string SsmRplSubmsg::toString() const
{
    return "RPL" + std::string(xasm ? " XASM\n" : "\n") + SsmProtoNewSubmsg::toString();
}

Message SsmRplSubmsg::modify(ssim::CachedScdPeriods& cached, const ssim::ProcContext& attr) const
{
    LogTrace(TRACE5) << __FUNCTION__ << "-rpl";

    for (const ssim::SsmProtoNewStuff& st : stuff) {
        ssim::details::SegmentInfoCleaner sic;

        Expected<ssim::ScdPeriods> tlgScd = ssim::makeScdPeriod(
            fi, st.pi, st.legs, segs, attr, ssim::toLocalTime, true
        );

        if (!tlgScd) {
            CALL_MSG_RET_PREF(fi.flt, st.pi.period, tlgScd.err());
        }

        ssim::ExtraSegInfos esi;
        sic.getUnhandled(esi, segs);

        for (ssim::ScdPeriod& scp : *tlgScd) {
            scp.auxData = esi;

            details::updateCachedPeriods(cached, attr, true, scp.flight, scp.period, xasm);
            details::DiffHandlerRpl hdl(scp, attr);
            CALL_MSG_RET_PREF(fi.flt, st.pi.period, hdl.handle(cached));
        }
    }
    return Message();
}

SsmSubmsgPtr SsmRplSubmsg::create(const ssim::ScdPeriod& scp, const Periods& ps, const ssim::PubOptions& opts, bool xasm)
{
    return createProtoNew(ssim::SSM_RPL, scp, ps, opts, xasm);
}
//#############################################################################
SsmSkdSubmsg::SsmSkdSubmsg(bool b, const ssim::FlightInfo & fi, const ssim::SkdPeriodInfo & pi)
    : SsmSubmsg(fi), xasm(b), period(pi)
{}

ssim::SsmActionType SsmSkdSubmsg::type() const
{
    return ssim::SSM_SKD;
}

std::string SsmSkdSubmsg::toString() const
{
    std::stringstream os;
    os << "SKD" << (xasm ? " XASM\n" : "\n") << fi << "\n" << period << "\n";
    return os.str();
}

Periods SsmSkdSubmsg::getPeriods() const
{
    return Periods(1, Period(this->period.start, this->period.end, "1234567"));
}

Message SsmSkdSubmsg::modify(ssim::CachedScdPeriods& cached, const ssim::ProcContext& attr) const
{
    LogTrace(TRACE5) << __FUNCTION__ << "-skd";

    const Period prd(period.start, period.end);
    return handlePeriodCancel(cached, attr, fi.flt, prd, type(), xasm);
}
//#############################################################################
SsmRsdSubmsg::SsmRsdSubmsg(const ssim::FlightInfo & fi, const ssim::SkdPeriodInfo & pi)
    : SsmSubmsg(fi), period(pi)
{}

ssim::SsmActionType SsmRsdSubmsg::type() const
{
    return ssim::SSM_RSD;
}

std::string SsmRsdSubmsg::toString() const
{
    std::stringstream os;
    os << "RSD\n" << fi << std::endl << period;
    return os.str();
}

Periods SsmRsdSubmsg::getPeriods() const
{
    return Periods(1, Period(this->period.start, this->period.end, "1234567"));
}

Message SsmRsdSubmsg::modify(ssim::CachedScdPeriods&, const ssim::ProcContext&) const
{
    //TODO: добавить в callbacks sendRecap?
    // rsd не модифицирует расписания, просто публикует их
    return Message();
}
//#############################################################################
SsmCnlSubmsg::SsmCnlSubmsg(bool x, const ssim::FlightInfo & fl, const ssim::PeriodInfoList & l)
    : SsmSubmsg(fl), xasm(x), periods(l) {}

ssim::SsmActionType SsmCnlSubmsg::type() const
{
    return ssim::SSM_CNL;
}

std::string SsmCnlSubmsg::toString() const
{
    std::stringstream os;
    os << "CNL" << (xasm ? " XASM\n" : "\n");
    os << fi << std::endl;
    for (const ssim::PeriodInfo & pi : periods) {
        os << pi << std::endl;
    }
    return os.str();
}

Periods SsmCnlSubmsg::getPeriods() const
{
    Periods ret;
    for (const ssim::PeriodInfo & st : periods) {
        ret.push_back(st.period);
    }
    return ret;
}

Message SsmCnlSubmsg::modify(ssim::CachedScdPeriods& cached, const ssim::ProcContext& attr) const
{
    LogTrace(TRACE5) << __FUNCTION__ << "-cnl";

    for (const ssim::PeriodInfo& pi : periods) {
        CALL_MSG_RET(handlePeriodCancel(cached, attr, fi.flt, pi.period, type(), xasm));
    }
    return Message();
}
//#############################################################################
SsmEqtConSubmsg::SsmEqtConSubmsg(const ssim::FlightInfo & fl, const std::list<ssim::ProtoEqtStuff> & ps, const ssim::SegmentInfoList & sl)
    : SsmSubmsg(fl), stuff(ps), segs(sl)
{}

std::string SsmEqtConSubmsg::toString() const
{
    std::stringstream os;
    os << ssim::enumToStr(type()) << std::endl;
    os << fi << std::endl;

    //merge equipment/leg by periods
    //don't try to merge legs by equipment - it's more complex than string concatenation
    std::list<str_pair> vc;   //period - equipment per leg(s)
    for (const ssim::ProtoEqtStuff& s : stuff) {
        std::stringstream s1, s2;
        s1 << s.pi << std::endl;
        s2 << s.eqi << std::endl;
        if (s.legs) {
            s2 << *s.legs << std::endl;
        }
        vc.emplace_back(s1.str(), s2.str());
    }
    collapseContainer(vc, cgetFirst, getSecond, true);
    collapseContainer(vc, cgetSecond, getFirst, true);
    for (const auto& v : vc) {
        os << v.first << v.second;
    }
    for (const ssim::SegmentInfo & si : segs) {
        os << si << std::endl;
    }
    return os.str();
}

Periods SsmEqtConSubmsg::getPeriods() const
{
    Periods ret;
    for (const ssim::ProtoEqtStuff & st : stuff) {
        ret.push_back(st.pi.period);
    }
    return ret;
}

Message SsmEqtConSubmsg::modify(ssim::CachedScdPeriods& cached, const ssim::ProcContext& attr) const
{
    LogTrace(TRACE5) << __FUNCTION__ << '-' << ssim::enumToStr(type());

    for (const ssim::ProtoEqtStuff & st : stuff) {
        if (st.pi.period.empty()) {
            continue;
        }
        Expected<ct::Flight> actualFlt = getActFlight(attr, fi.flt, st.pi.period);
        if (!actualFlt) {
            CALL_MSG_RET_PREF(fi.flt, st.pi.period, actualFlt.err());
        }

        details::updateCachedPeriods(cached, attr, attr.timeIsLocal, *actualFlt, st.pi.period);
        details::DiffHandlerEqtCon hdl(
            isInverted(attr), st, segs,
            ssim::getPartnerInfo(ssim::AgreementType::CSH, { &st.pi, &fi }),
            *actualFlt, attr
        );
        CALL_MSG_RET_PREF(fi.flt, st.pi.period, hdl.handle(cached, true));
    }
    return Message();
}

SsmEqtSubmsg::SsmEqtSubmsg(const FlightInfo& fl, const std::list<ProtoEqtStuff>& ps, const SegmentInfoList& sl)
    : SsmEqtConSubmsg(fl, ps, sl)
{}

ssim::SsmActionType SsmEqtSubmsg::type() const
{
    return ssim::SSM_EQT;
}

SsmSubmsgPtr SsmEqtSubmsg::create(const ssim::ScdPeriod& scp, const Periods& ps, const ssim::PubOptions& opts)
{
    return createEqtCon(ssim::SSM_EQT, scp, ps, opts);
}

SsmConSubmsg::SsmConSubmsg(const FlightInfo& fl, const std::list<ProtoEqtStuff>& ps, const SegmentInfoList& sl)
    : SsmEqtConSubmsg(fl, ps, sl)
{}

ssim::SsmActionType SsmConSubmsg::type() const
{
    return ssim::SSM_CON;
}

SsmSubmsgPtr SsmConSubmsg::create(const ssim::ScdPeriod& scp, const Periods& ps, const ssim::PubOptions& opts)
{
    return createEqtCon(ssim::SSM_CON, scp, ps, opts);
}
//#############################################################################
static std::vector<ssim::SsmTimStuff> splitByLeaps(const ssim::SsmTimStuff& in)
{
    std::vector<ssim::Section> scs;
    for (const ssim::RoutingInfo& ri : in.legs) {
        scs.emplace_back(ri.dep, ri.arr, ri.depTime, ri.arrTime);
    }

    std::vector<ssim::SsmTimStuff> out;
    for (const Period& prd : splitUtcScdByLeaps(ScdRouteView{ in.pi.period, scs})) {
        out.push_back(in);
        out.back().pi.period = prd;
    }
    return out;
}

static std::vector<ssim::SsmTimStuff> toLocalTime(const ssim::SsmTimStuff& in)
{
    //FIXME: аналогично соотв. ф-ции для ScdPeriod
    //       напрашивается обобщение, но слишком отличаются структуры
    //       и похоже обвязка будет больше, чем сокращение от обобщения

    std::vector<ssim::SsmTimStuff> out = splitByLeaps(in);
    for (ssim::SsmTimStuff& sts : out) {
        boost::gregorian::days offset(0);
        const boost::gregorian::date baseDt = sts.pi.period.start;
        for (auto lgi = sts.legs.begin(); lgi != sts.legs.end(); ++lgi) {
            const auto tm = nsi::timeGmtToCity(boost::posix_time::ptime(baseDt, lgi->depTime), lgi->dep);
            if (lgi == sts.legs.begin()) {
                offset = tm.date() - baseDt;
            }
            const boost::posix_time::ptime anchor(baseDt + offset);
            lgi->depTime = tm - anchor;
            lgi->arrTime = nsi::timeGmtToCity(boost::posix_time::ptime(baseDt, lgi->arrTime), lgi->arr) - anchor;
        }
        sts.pi.period = sts.pi.period.shift(offset);
    }
    return out;
}

SsmTimSubmsg::SsmTimSubmsg(const ssim::FlightInfo & fl, const std::list<ssim::SsmTimStuff> & pl, const ssim::SegmentInfoList & sl)
    : SsmSubmsg(fl), stuff(pl), segs(sl) {}

ssim::SsmActionType SsmTimSubmsg::type() const
{
    return ssim::SSM_TIM;
}

std::string SsmTimSubmsg::toString() const
{
    std::stringstream os;
    os << "TIM\n";
    os << fi << std::endl;
    // при схлопывании дублирующихся строк будем оперировать именно строками, а не структурами
    // поэтому подготовим сначала почву
    std::list<str_pair> vc; // пара период - плечи
    for (const ssim::SsmTimStuff & s : stuff) {
        std::stringstream p, l;
        p << s.pi << std::endl;
        for (const ssim::RoutingInfo& ri : s.legs) {
            l << ri.toString() << std::endl;
        }
        vc.emplace_back(p.str(), l.str());
    }
    // получили набор пар период + строка с несколькими плечами
    // если эта строка одинакова для нескольких периодов, надо схлопнуть
    collapseContainer(vc, cgetSecond, getFirst, false);
    // теперь собственно вывод
    for (const str_pair & v : vc) {
        os << v.first << v.second;
    }
    for (const ssim::SegmentInfo & si : segs) {
        os << si << std::endl;
    }
    return os.str();
}

Periods SsmTimSubmsg::getPeriods() const
{
    Periods ret;
    for (const ssim::SsmTimStuff & st : stuff) {
        ret.push_back(st.pi.period);
    }
    return ret;
}

Message SsmTimSubmsg::modify(ssim::CachedScdPeriods& cached, const ssim::ProcContext& attr) const
{
    // TIM - изменение времен
    LogTrace(TRACE5) << __FUNCTION__ << "-tim";

    for (const ssim::SsmTimStuff& st : stuff) {
        Expected<ct::Flight> actualFlt = getActFlight(attr, fi.flt, st.pi.period);
        if (!actualFlt) {
            CALL_MSG_RET_PREF(fi.flt, st.pi.period, actualFlt.err());
        }

        std::vector<ssim::SsmTimStuff> sts;
        if (attr.timeIsLocal) {
            sts.push_back(st);
        } else {
            toLocalTime(st).swap(sts);
        }

        for (const ssim::SsmTimStuff& lst : sts) {
            details::updateCachedPeriods(cached, attr, true, *actualFlt, lst.pi.period);
            details::DiffHandlerTim hdl(isInverted(attr), lst, segs, *actualFlt, attr);
            CALL_MSG_RET_PREF(fi.flt, lst.pi.period, hdl.handle(cached));
        }
    }
    return Message();
}

SsmSubmsgPtr SsmTimSubmsg::create(const ssim::ScdPeriod& scp, const ssim::PubOptions& opts)
{
    ASSERT (!scp.adhoc);

    const ssim::PeriodInfo pi(scp.period);
    ssim::FlightInfo fi(scp.flight);
    ssim::LegStuffList lst;
    ssim::SegmentInfoList segs;
    ssim::details::setupSsimData(scp, fi, lst, segs, opts, ssim::details::getDeiSubset(ssim::SSM_TIM));

    ssim::RoutingInfoList ril;
    std::transform(lst.begin(), lst.end(), std::back_inserter(ril), [] (const ssim::LegStuff& x) { return x.ri; });
    return std::make_shared<ssim::SsmTimSubmsg>(
        fi, std::list<ssim::SsmTimStuff>{ ssim::SsmTimStuff { pi, ril } }, segs
    );
}
//#############################################################################
SsmRevSubmsg::SsmRevSubmsg(const ssim::RevFlightInfo & fl, const ssim::PeriodInfoList & pil)
    : SsmSubmsg(fl), flightPeriod(fl.pi), periods(pil)
{}

ssim::SsmActionType SsmRevSubmsg::type() const
{
    return ssim::SSM_REV;
}

std::string SsmRevSubmsg::toString() const
{
    std::stringstream os;
    os << "REV\n";
    os << fi << " " << flightPeriod << std::endl;
    for (const ssim::PeriodInfo & pi : periods) {
        os << pi << std::endl;
    }
    return os.str();
}

Periods SsmRevSubmsg::getPeriods() const
{
    Periods ret;
    for (const ssim::PeriodInfo & st : periods) {
        ret.push_back(st.period);
    }
    return ret;
}

Message SsmRevSubmsg::modify(ssim::CachedScdPeriods& cached, const ssim::ProcContext& attr) const
{
    LogTrace(TRACE5) << __FUNCTION__ << "-rev";

    Expected<ct::Flight> actualFlt = getActFlight(attr, fi.flt, flightPeriod.period);
    if (!actualFlt) {
        CALL_MSG_RET_PREF(fi.flt, flightPeriod.period, actualFlt.err());
    }
    // дочитываем расписания в кэш
    details::updateCachedPeriods(cached, attr, attr.timeIsLocal, *actualFlt, flightPeriod.period);
    details::DiffHandlerRev hdl(periods, *actualFlt, flightPeriod.period, attr);
    CALL_MSG_RET_PREF(fi.flt, flightPeriod.period, hdl.handle(cached, true));
    return Message();
}
//#############################################################################
SsmAdmSubmsg::SsmAdmSubmsg(const ssim::FlightInfo & fl, const ssim::PeriodInfoList & pl, const ssim::SegmentInfoList & sl)
    : SsmSubmsg(fl), periods(pl), segs(sl)
{}

ssim::SsmActionType SsmAdmSubmsg::type() const
{
    return ssim::SSM_ADM;
}

std::string SsmAdmSubmsg::toString() const
{
    std::stringstream os;
    os << "ADM\n";
    os << fi << std::endl;
    for (const ssim::PeriodInfo & pi : periods) {
        os << pi << std::endl;
    }
    for (const ssim::SegmentInfo & si : segs) {
        os << si << std::endl;
    }
    return os.str();
}

Periods SsmAdmSubmsg::getPeriods() const
{
    Periods ret;
    for (const ssim::PeriodInfo & st : periods) {
        ret.push_back(st.period);
    }
    return ret;
}

Message SsmAdmSubmsg::modify(ssim::CachedScdPeriods& cached, const ssim::ProcContext& attr) const
{
    // ADM - это просто применение dei на период
    // причем кш в ней игнорируется сейчас
    LogTrace(TRACE5) << __FUNCTION__ << "-adm";

    for (const ssim::PeriodInfo & pi : periods) {
        Expected<ct::Flight> actualFlt = getActFlight(attr, fi.flt, pi.period);
        if (!actualFlt) {
            CALL_MSG_RET_PREF(fi.flt, pi.period, actualFlt.err());
        }
        // дочитываем расписания в кэш
        details::updateCachedPeriods(cached, attr, attr.timeIsLocal, *actualFlt, pi.period);
        details::DiffHandlerAdm hdl(isInverted(attr), segs, *actualFlt, pi.period, attr);
        CALL_MSG_RET_PREF(fi.flt, pi.period, hdl.handle(cached, true));
    }
    return Message();
}
//#############################################################################
SsmFltSubmsg::SsmFltSubmsg(const ssim::FlightInfo & f1, const ssim::FlightInfo & f2, const ssim::PeriodInfoList & pil, const ssim::SegmentInfoList & sil)
    : SsmSubmsg(f1), newFi(f2), periods(pil), segs(sil)
{}

ssim::SsmActionType SsmFltSubmsg::type() const
{
    return ssim::SSM_FLT;
}

std::string SsmFltSubmsg::toString() const
{
    std::stringstream os;
    os << "FLT\n";
    os << fi << std::endl;
    for (const ssim::PeriodInfo & pi : periods) {
        os << pi << std::endl;
    }
    os << newFi << std::endl;
    for (const ssim::SegmentInfo & si : segs) {
        os << si << std::endl;
    }
    return os.str();
}

Periods SsmFltSubmsg::getPeriods() const
{
    Periods ret;
    for (const ssim::PeriodInfo & st : periods) {
        ret.push_back(st.period);
    }
    return ret;
}

Message SsmFltSubmsg::modify(ssim::CachedScdPeriods& cached, const ssim::ProcContext& attr) const
{
    LogTrace(TRACE5) << __FUNCTION__ << "-flt";

    if (fi.flt.airline != newFi.flt.airline) {
        return Message(STDLOG, _("Change of airline isn't allowed"));
    }
    for (const ssim::PeriodInfo& pi : periods) {
        if (pi.period.empty()) {
            continue;
        }
        Expected<ct::Flight> actualFlt = getActFlight(attr, fi.flt, pi.period);
        if (!actualFlt) {
            CALL_MSG_RET_PREF(fi.flt, pi.period, actualFlt.err());
        }

        details::updateCachedPeriods(cached, attr, attr.timeIsLocal, *actualFlt, pi.period);
        details::DiffHandlerFlt hdl(isInverted(attr), newFi.flt, fi.flt, segs, *actualFlt, pi.period, attr);
        CALL_MSG_RET_PREF(fi.flt, pi.period, hdl.handle(cached, true));
    }
    return Message();
}
//#############################################################################
SsmNacSubmsg::SsmNacSubmsg(const ssim::FlightInfo & fi, const std::string& ri, const ssim::PeriodInfoList & pil)
    : SsmSubmsg(fi), rejectInfo(ri), periods(pil)
{}

ssim::SsmActionType SsmNacSubmsg::type() const
{
    return ssim::SSM_NAC;
}

std::string SsmNacSubmsg::toString() const
{
    return "NAC";
}

Periods SsmNacSubmsg::getPeriods() const
{
    Periods ret;
    for (const ssim::PeriodInfo & st : periods) {
        ret.push_back(st.period);
    }
    return ret;
}

Message SsmNacSubmsg::modify(ssim::CachedScdPeriods&, const ssim::ProcContext&) const
{
    return Message(STDLOG, _("NAC received: %1%")).bind(rejectInfo);
}
//#############################################################################
// то еше убожество (как и в asm) - пропускать его никуда нельзя, потому что пакер валится
SsmAckSubmsg::SsmAckSubmsg()
    : SsmSubmsg(ssim::FlightInfo(ct::Flight(nsi::CompanyId(0), ct::FlightNum(1))))
{}

ssim::SsmActionType SsmAckSubmsg::type() const
{
    return ssim::SSM_ACK;
}

std::string SsmAckSubmsg::toString() const
{
    return "ACK";
}

Periods SsmAckSubmsg::getPeriods() const
{
    return {};
}

Message SsmAckSubmsg::modify(ssim::CachedScdPeriods&, const ssim::ProcContext&) const
{
    return Message();
}
//#############################################################################
std::string SsmStruct::toString() const
{
    std::stringstream os;
    os << head.toString();

    for (const SsmSubmsgPtr& p : specials) {
        os << p->toString() << "//\n";
    }

    for (const auto& v : submsgs) {
        for (const SsmSubmsgPtr& p : v.second) {
            os << p->toString() << "//\n";
        }
    }

    const std::string out = os.str();
    if (!specials.empty() || !submsgs.empty()) {
        return out.substr(0, out.size() - 3);
    }
    return out;
}

} //ssim
