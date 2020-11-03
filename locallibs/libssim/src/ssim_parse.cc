#include <boost/lexical_cast.hpp>

#include <serverlib/dates.h>
#include <serverlib/algo.h>
#include <serverlib/str_utils.h>

#include <typeb/AnyStringElem.h>
#include <typeb/ReferenceElem.h>
#include <typeb/SsmElem.h>
#include <typeb/AsmTypes.h>

#include "ssim_parse.h"
#include "ssim_data_types.h"
#include "ssim_utils.h"
#include "callbacks.h"

#define NICKNAME "ASH"
#include <serverlib/slogger.h>

namespace {

static std::pair<int, std::string> prepareDei(const std::string& s)
{
    LogTrace(TRACE5) << s;
    size_t pos = s.find("/");
    std::string numStr = s.substr(0, pos);
    std::string dei;
    if (pos != std::string::npos) {
        //it's possible in segment string to have dei in form '555' for example
        //so 'dei' string may be empty.
        dei = s.substr(pos + 1);
    }
    int num = 0;
    try {
        num = boost::lexical_cast<int>(numStr);
    } catch (const boost::bad_lexical_cast & e) {
        LogError(STDLOG) << "bad lexical cast " << s;
        return std::make_pair(-1, "");
    }
    return std::make_pair(num, dei);
}


static boost::posix_time::time_duration timeFromRouting(
        const std::string & dt, const std::string & t, const std::string & var
    )
{
    LogTrace(TRACE5) << __FUNCTION__ << " " << dt << " " << t << " " << var;
    std::string time(t);
    assert (dt.empty() || var.empty());

    std::string shift(var);
    if (!shift.empty() && shift.at(0) == 'M') {
        shift[0] = '-';
    }
    try {
        int dateShift = 0;
        if (!shift.empty()) {
            dateShift = boost::lexical_cast<int>(shift);
        }
        if (!dt.empty()) {
            dateShift = boost::lexical_cast<int>(dt); //здесь полагается, что отсчитываем о 0 а не от даты рейса
        }
        int hours = boost::lexical_cast<int>(time.substr(0,2));
        int mins = boost::lexical_cast<int>(time.substr(2));
        LogTrace(TRACE5) << "dateShift=" << dateShift << " hours=" << hours << " mins=" << mins;
        return boost::posix_time::time_duration(hours, mins, 0, 0) + boost::posix_time::hours(24 * dateShift);
    } catch (const boost::bad_lexical_cast&) {
        return boost::posix_time::not_a_date_time;
    }
}

} //anonymous

namespace ssim { namespace details {

bool prepareRawDeiMap(ssim::RawDeiMap& map, const std::vector<std::string>& v)
{
    for (const std::string & s :v) {
        std::pair<int, std::string> tmp = prepareDei(s);
        if (tmp.first < 0) {
            return false;
        }
        map.emplace(ct::DeiCode(tmp.first), tmp.second);
    }
    LogTrace(TRACE5) << "Dei sz = " << map.size();
    return true;
}

Expected<nsi::Points> parseLegChange(const std::string& s, unsigned line)
{
    nsi::Points pts;
    for (const std::string& str : StrUtils::split_string< std::vector<std::string> >(s, '/', StrUtils::KeepEmptyTokens::True)) {
        const EncString pt = EncString::from866(str);

        if (!nsi::Point::find(pt)) {
            return Message(STDLOG, _("#%1% Unrecognized station %2%")).bind(line).bind(str);
        }
        pts.emplace_back(nsi::Point(pt).id());
    }
    if (pts.size() < 2) {
        return Message(STDLOG, _("%1% Invalid leg change indentifier")).bind(line);
    }

    assert(pts.size() > 1);
    return pts;
}

Message fillHead(const typeb_parser::TypeBMessage& msg, ssim::MsgHead& msgh)
{
    const typeb_parser::AddrHead& head = msg.addrs();
    msgh.sender = head.sender();
    msgh.receiver = head.receiver();
    msgh.msgId = head.msgId();

    //time string
    const typeb_parser::TbElement * curTe = msg.findp("Time");
    if (curTe->elemAsList().total() == 0) {
        LogTrace(TRACE5) << "No time string in tlg, use UTC";
        msgh.timeIsLocal = false;
    } else {
        const auto& tme = curTe->elemAsGroup().byNum(0).cast<const typeb_parser::AnyStringElem>(STDLOG);
        msgh.timeIsLocal = (tme.toString() == "LT");
    }

    //ref string
    curTe = msg.findp("Ref");
    if (curTe->elemAsList().total() == 0) {
        LogTrace(TRACE5) << "No reference string in tlg";
    } else {
        const auto& rfe = curTe->elemAsGroup().byNum(0).cast<const typeb_parser::ReferenceElem>(STDLOG);
        const boost::gregorian::date dt = Dates::DateFromDDMON(
            rfe.date(), Dates::GuessYear_Current(), Dates::currentDate()
        );
        if (dt.is_not_a_date()) {
            return Message(STDLOG, _("Invalid date in header"));
        }

        msgh.ref = ssim::ReferenceInfo {
            dt,
            rfe.gid(),
            rfe.isLast(),
            rfe.mid(),
            rfe.creatorRef()
        };
    }
    return Message();
}

Expected<ssim::FlightInfo> makeInfoFromFlight(const typeb_parser::TbElement & elem)
{
    const typeb_parser::SsmFlightElem& e = elem.cast<const typeb_parser::SsmFlightElem>(STDLOG);
    LogTrace(TRACE5) << e;
    boost::optional<ct::Flight> flt = ct::Flight::fromStr(e.flight());
    if (!flt) {
        return Message(STDLOG, _("#%1% Incorrect flight %2%")).bind(e.line()).bind(e.flight());
    }

    LogTrace(TRACE5) << "FlightElem transformed";

    ssim::RawDeiMap map;
    if (!prepareRawDeiMap(map, e.dei())) {
        return Message(STDLOG, _("#%1% Incorrect dei on flight %2%")).bind(e.line()).bind(e.flight());
    }
    ssim::FlightInfo fi(*flt, map);
    if (!fi.fillPartnerInfo()) {
        return Message(STDLOG, _("#%1% Incorrect code-sharing on flight %2%")).bind(e.line()).bind(e.flight());
    }
    return fi;
}

static boost::optional<ssim::Franchise> getOprDisclosure(const std::string& deiCnt)
{
    const size_t pos = deiCnt.find("/");

    const EncString compCode = EncString::from866(deiCnt.substr(0, pos));
    std::string compName;
    if (pos != std::string::npos) {
        compName = deiCnt.substr(pos + 1);
    }

    LogTrace(TRACE5) << __FUNCTION__ << " descr:[" << compCode << "] [" << compName << "]";

    if (!compCode.empty()) {
        if (!nsi::Company::find(compCode)) {
            return boost::none;
        }
        return ssim::Franchise { nsi::Company(compCode).id(), compName };
    }
    if (compName.empty()) {
        return boost::none;
    }
    return ssim::Franchise { boost::none, compName };
}

Expected<ssim::SegmentInfo> makeInfoFromSeg(const typeb_parser::TbElement& elem, bool nil_value_allowed)
{
    const typeb_parser::SsmSegmentElem & e = elem.cast<const typeb_parser::SsmSegmentElem>(STDLOG);
    LogTrace(TRACE5) << e;
    EncString city1(EncString::from866(e.seg().substr(0,3)));
    EncString city2(EncString::from866(e.seg().substr(3,3)));
    boost::optional<nsi::PointId> p1, p2;
    if (city1.toUtf() == "QQQ") {
        //any point
        p1 = nsi::PointId(0);
    } else {
        if(!nsi::Point::find(city1)) {
            return Message(STDLOG, _("#%1% Incorrect departure point on segment %2%")).bind(e.line()).bind(e.seg());
        }
        p1 = nsi::Point(city1).id();
    }
    if (city2.toUtf() == "QQQ") {
        p2 = nsi::PointId(0);
    } else {
        if(!nsi::Point::find(city2)) {
            return Message(STDLOG, _("#%1% Incorrect arrival point on segment %2%")).bind(e.line()).bind(e.seg());
        }
        p2 = nsi::Point(city2).id();
    }
    std::pair<int, std::string> pr;
    if (!e.dei().empty()) {
        pr = prepareDei(e.dei());
    }
    if (pr.first < 0) {
        return Message(STDLOG, _("#%1% Incorrect dei %2%")).bind(e.line()).bind(e.dei());
    }
    LogTrace(TRACE5) << "SegmentElem transformed";
    ssim::SegmentInfo si(*p1, *p2, ct::DeiCode(pr.first), pr.second);

    //NIL возможен лишь для ADM, для остальных можно было бы ругаться
    //но нет смысла в лишней ругани, если придет NIL для DEI, которое мы даже не поддерживаем
    //поэтому если сообщение не подразумевает NIL, считаем его просто значением и отваливаемся на валидациях контента
    if (nil_value_allowed && si.dei == "NIL") {
        si.resetValue = true;
        return si;
    }

    // traffic restriction
    if (si.deiType.get() == 8) {
        if (!(si.trs = ssim::parseTrNote(si.dei))) {
            return Message(STDLOG, _("#%1% Incorrect traffic restriction %2%")).bind(e.line()).bind(si.dei);
        }
    }
    // et
    if (si.deiType.get() == 505) {
        // ожидаются две буквы ET or EN
        if (si.dei == "ET")
            si.et = true;
        else if (si.dei == "EN")
            si.et = false;
        else {
            return Message(STDLOG, _("#%1% Incorrect eticket information %2%")).bind(e.line()).bind(si.dei);
        }
    }

    if (si.deiType.get() == 504) {
        if (si.dei == "S") {
            si.secureFlight = true;
        } else {
            return Message(STDLOG, _("#%1% Incorrect secure flight indicator %2%")).bind(e.line()).bind(si.dei);
        }
    }

    // csh-opr
    if (si.deiType.get() == 50) {
        if (!(si.oprFl = ct::Flight::fromStr(pr.second))) {
            return Message(STDLOG, _("#%1% Incorrect code-sharing on segment %2%")).bind(e.line()).bind(e.seg());
        }
    }
    // csh-mkt
    if (si.deiType.get() == 10) {
        for (const std::string& str : StrUtils::split_string< std::vector<std::string> >(StrUtils::delSpaces(pr.second), '/', StrUtils::KeepEmptyTokens::True)) {
            boost::optional<ct::Flight> fl = ct::Flight::fromStr(str);
            if (!fl) {
                return Message(STDLOG, _("#%1% Incorrect code-sharing %2% on segment %3%")).bind(e.line()).bind(str).bind(e.seg());
            }
            si.manFls.push_back(*fl);
        }
    }

    //109 - Meal service note exceeding maximum length
    //111 - Meal service segment override
    if (si.deiType.get() == 109 || si.deiType.get() == 111) {
        if (!(si.meals = ssim::getMealsMap(si.dei))) {
            return Message(STDLOG, _("#%1% Incorrect meal service on segment %2%")).bind(e.line()).bind(e.seg());
        }
    }

    //127 - Operating airline disclosure
    if (si.deiType.get() == 127) {
        if (!(si.oprDisclosure = getOprDisclosure(si.dei))) {
            return Message(STDLOG, _("#%1% Incorrect airline disclosure on segment %2%")).bind(e.line()).bind(e.seg());
        }
    }

    return si;
}

template <typename T> Expected<ssim::RoutingInfo> makeInfoFromRouting(const T& e)
{
    const EncString city1(EncString::from866(e.dep()));
    const EncString city2(EncString::from866(e.arr()));
    // мы хотим порты
    if(!nsi::Point::find(city1)) {
        return Message(STDLOG, _("#%1% Unrecognized departure point %2%")).bind(e.line()).bind(e.dep());
    }
    nsi::PointId p1(nsi::Point(city1).id());

    if(!nsi::Point::find(city2)) {
        return Message(STDLOG, _("#%1% Unrecognized arrival point %2%")).bind(e.line()).bind(e.arr());
    }
    nsi::PointId p2(nsi::Point(city2).id());
    ssim::RawDeiMap map;
    if (!prepareRawDeiMap(map, e.dei())) {
        return Message(STDLOG, _("#%1% Incorrect dei on leg %2%%3%")).bind(e.line()).bind(e.dep()).bind(e.arr());
    }
    // особый случай - отправление в полночь должно быть указано как 0000
    // прилет в полночь -- как 2400 соответственно
    if (e.airSTD() == "2400" || e.airSTA() == "0000") {
        return Message(STDLOG, _("#%1% Incorrect format of midnight")).bind(e.line());
    }
    ssim::RoutingInfo ri(p1, boost::posix_time::time_duration(), e.pasSTD(), p2, boost::posix_time::time_duration(), e.pasSTA(), map);
    // времена заполняются позже
    if (!ri.fillPartnerInfo()) {
        return Message(STDLOG, _("#%1% Incorrect code-sharing on leg %2%%3%")).bind(e.line()).bind(e.dep()).bind(e.arr());
    }
    if (!ri.fillMeals()) {
        return Message(STDLOG, _("#%1% Incorrect meal service on leg %2%%3%")).bind(e.line()).bind(e.dep()).bind(e.arr());
    }
    return ri;
}

Expected<ssim::RoutingInfo> makeInfoFromSsmRouting(const typeb_parser::TbElement& elem)
{
    const typeb_parser::SsmRoutingElem& e = elem.cast<const typeb_parser::SsmRoutingElem>(STDLOG);
    Expected<ssim::RoutingInfo> opt = makeInfoFromRouting<typeb_parser::SsmRoutingElem>(e);
    if (!opt.valid()) {
        return opt;
    }
    boost::posix_time::time_duration depTime = timeFromRouting("", e.airSTD(), e.varSTD());
    if (depTime.is_special()) {
        return Message(STDLOG, _("#%1% Incorrect time of departure %2%/%3%")).bind(e.line()).bind(e.airSTD()).bind(e.varSTD());
    }
    boost::posix_time::time_duration arrTime = timeFromRouting("", e.airSTA(), e.varSTA());
    if (arrTime.is_special()) {
        return Message(STDLOG, _("#%1% Incorrect time of arrival %2%/%3%")).bind(e.airSTA()).bind(e.varSTA());
    }
    opt->depTime = depTime;
    opt->arrTime = arrTime;
    return *opt;
}

Expected<ssim::RoutingInfo> makeInfoFromAsmRouting(const typeb_parser::TbElement & elem)
{
    const typeb_parser::AsmRoutingElem& e = elem.cast<const typeb_parser::AsmRoutingElem>(STDLOG);
    Expected<ssim::RoutingInfo> opt = makeInfoFromRouting<typeb_parser::AsmRoutingElem>(e);
    LogTrace(TRACE5) << e;
    // есть особенность - если пришедшее время имеет вид 221203б то 22 влияет на результат,
    // но мы не знаем, какова дата рейда (известно только снаружи). поэтому считаем, что точка
    // отсчета - нулевое число
    if (!opt.valid()) {
        return opt;
    }
    boost::posix_time::time_duration depTime = timeFromRouting(e.dateD(), e.airSTD(), "");
    if (depTime.is_special()) {
        return Message(STDLOG, _("#%1% Incorrect time of departure %2%/%3%")).bind(e.line()).bind(e.airSTD()).bind(e.dateD());
    }
    boost::posix_time::time_duration arrTime = timeFromRouting(e.dateA(), e.airSTA(), "");
    if (arrTime.is_special()) {
        return Message(STDLOG, _("#%1% Incorrect time of arrival %2%/%3%")).bind(e.line()).bind(e.airSTA()).bind(e.dateA());
    }
    opt->depTime = depTime;
    opt->arrTime = arrTime;
    LogTrace(TRACE5) << opt->toString();
    return *opt;
}

Expected<ssim::EquipmentInfo> makeInfoFromEquip(const typeb_parser::TbElement& elem)
{
    const typeb_parser::SsmEquipmentElem & e = elem.cast<const typeb_parser::SsmEquipmentElem>(STDLOG);
    LogTrace(TRACE5) << e;
    //we don't have information about company here. so we can't create ct::RbdLayout
    //it'll be created later - on processing stage
    LogTrace(TRACE5) << "EquipmentElem transformed";

    ssim::RawDeiMap map;
    if (!prepareRawDeiMap(map, e.dei())) {
        return Message(STDLOG, _("#%1% Incorrect dei in equipment string")).bind(e.line());
    }

    const EncString craftType(EncString::from866(e.craftType()));
    if (!nsi::AircraftType::find(craftType)) {
        return Message(STDLOG, _("#%1% Unknown plane type %2%")).bind(e.line()).bind(craftType.toUtf());
    }

    EncString st(EncString::fromUtf(std::string(1, e.serviceType())));
    if (!nsi::ServiceType::find(st)) {
        LogTrace(TRACE1) << "Unknown service type: " << e.serviceType();
        st = EncString::fromUtf("J");
    }

    boost::optional<ct::RbdsConfig> cfg = ct::getRbdsConfig(e.aircraftConfig());
    if (!cfg) {
        return Message(STDLOG, _("#%1% Wrong subclass order")).bind(e.line());
    }

    ssim::EquipmentInfo ei(
        nsi::ServiceType(st).id(),
        nsi::AircraftType(craftType).id(),
        *cfg,
        map
    );
    if (!ei.fillPartnerInfo()) {
        return Message(STDLOG, _("#%1% Incorrect code-sharing in equipment string")).bind(e.line());
    }
    if (ei.partners.find(ssim::AgreementType::CSH) != ei.partners.end()) {
        return Message(STDLOG, _("#%1% Code-sharing in this string is not allowed")).bind(e.line());
    }
    return ei;
}

//#############################################################################
struct DeiSegKey
{
    nsi::PointId dep, arr;
    ct::DeiCode deiType;

    DeiSegKey(nsi::PointId d, nsi::PointId a, ct::DeiCode dei)
        : dep(d), arr(a), deiType(dei)
    {}
};

static bool operator< (const DeiSegKey& x, const DeiSegKey& y)
{
    if (x.deiType != y.deiType) {
        return x.deiType < y.deiType;
    }
    if (x.dep != y.dep) {
        return x.dep < y.dep;
    }
    return x.arr < y.arr;
}

static boost::optional<ssim::SegmentInfo> mergeMeals_(const ssim::SegmentInfoList& lst)
{
    ASSERT(lst.size() > 1);
    ASSERT(lst.front().deiType.get() == 109 || lst.front().deiType.get() == 111);

    ssim::MealServiceInfo out;
    for (const ssim::SegmentInfo& si : lst) {
        if (!out.groupMeals.empty()) {
            return boost::none;
        }

        for (const auto& v : si.meals->rbdMeals) {
            if (out.rbdMeals.find(v.first) != out.rbdMeals.end()) {
                return boost::none;
            }
            out.rbdMeals.emplace(v.first, v.second);
        }
        out.groupMeals = si.meals->groupMeals;
    }
    ssim::SegmentInfo osg = lst.front();
    osg.meals = out;
    return osg;
}

static Expected<ssim::SegmentInfo> mergeMeals(const ssim::SegmentInfoList& lst)
{
    if (auto out = mergeMeals_(lst)) {
        return *out;
    }
    const ssim::SegmentInfo& si = lst.front();
    return Message(STDLOG, _("Incorrect meal service %1%%2% %3%"))
        .bind(ssim::pointCode(si.dep)).bind(ssim::pointCode(si.arr)).bind(si.deiType.get());
}

static Expected<ssim::SegmentInfo> mergeRestrictions(const ssim::SegmentInfoList& lst)
{
    ASSERT(lst.size() > 1);

    std::vector<ssim::Restrictions> trs;
    for (const auto& si : lst) {
        if (si.trs) {
            trs.emplace_back(*si.trs);
        }
    }
    if (auto tr_out = ssim::mergeTrNotes(trs)) {
        ssim::SegmentInfo si = lst.front();
        si.trs = tr_out;
        return si;
    }

    const ssim::SegmentInfo& si = lst.front();
    return Message(STDLOG, _("Incorrect traffic restrictions at %1%%2%"))
        .bind(ssim::pointCode(si.dep)).bind(ssim::pointCode(si.arr));
}

static Expected<ssim::SegmentInfo> mergeCsh(const ssim::SegmentInfoList& lst)
{
    ASSERT(lst.size() > 1);
    ASSERT(lst.front().deiType.get() == 10);

    ssim::SegmentInfo out(lst.front());
    for (auto si = std::next(lst.begin()), se = lst.end(); si != se; ++si) {
        out.manFls.insert(out.manFls.end(), si->manFls.begin(), si->manFls.end());
        out.dei += std::string(out.dei.empty() ? 0 : 1, '/') + si->dei;
    }
    return out;
}

static Message mergeSegmentInfos(ssim::SegmentInfoList& segsInfo)
{
    const std::map<ct::DeiCode, Expected<ssim::SegmentInfo> (*) (const ssim::SegmentInfoList&)> deisToMerge = {
        { ct::DeiCode(109), mergeMeals },
        { ct::DeiCode(111), mergeMeals },
        { ct::DeiCode(8), mergeRestrictions },
        { ct::DeiCode(10), mergeCsh }
    };

    ssim::SegmentInfoList out;
    std::map<DeiSegKey, ssim::SegmentInfoList> dmp;
    for (const auto& si : segsInfo) {
        if (!algo::contains(deisToMerge, si.deiType)) {
            out.push_back(si);
            continue;
        }
        dmp[DeiSegKey(si.dep, si.arr, si.deiType)].push_back(si);
    }

    for (const auto& v : dmp) {
        if (v.second.size() < 2) {
            out.push_back(v.second.front());
            continue;
        }

        CALL_EXP_RET(msi, deisToMerge.at(v.first.deiType)((v.second)));
        out.push_back(*msi);
    }

    segsInfo.swap(out);

    return Message();
}
//#############################################################################
bool isSegDeiAllowed_EqtCon(ct::DeiCode d)
{
    const int dei = d.get();
    if (dei >= 101 && dei <= 108) {
        return true;
    }
    if (dei >= 113 && dei <= 115) {
        return true;
    }
    return (dei == 127 || dei >= 800);
}

bool isSegDeiAllowed_Flt(ct::DeiCode d)
{
    const int dei = d.get();
    return (dei == 10 || dei == 50 || dei == 122 || dei >= 800);
}

bool isSegDeiAllowed_Tim(ct::DeiCode d)
{
    const int dei = d.get();
    return (dei == 97 || dei == 109 || dei == 111 || dei >= 800);
}
//#############################################################################
Message separateSegsAndSuppl(
        const typeb_parser::TbGroupElement& grp, size_t index, ssim::SegmentInfoList& res,
        bool nil_value_allowed, supported_dei_func is_dei_allowed
    )
{
    res.clear();
    if (grp.total() > index) { //additional info
        std::string nm = grp.byNum(index).templ()->accessName();
        if (nm == "Segment") {
            const auto& lst = grp.byNum(index).elemAsList();
            for (auto i = lst.begin(), e = lst.end(); i != e; ++i) {
                CALL_EXP_RET(si, makeInfoFromSeg(**i, nil_value_allowed));

                if (is_dei_allowed && !is_dei_allowed(si->deiType)) {
                    return Message(STDLOG, _("DEI %1% isn't supported in segment string for this type of tlg")).bind(si->deiType.get());
                }

                res.push_back(*si);
            }
        } // else - its suppl info, nevermind
    }

    //now merge separated lines (e.g. 109, 111)
    return mergeSegmentInfos(res);
}

void reportParsedFlight(const ct::Flight& flt, ssim::ParseRequisitesCollector* pc)
{
    if (pc) {
        pc->appendFlight(flt);
    }
}

void reportParsedPeriod(const ct::Flight& flt, const Period& prd, ssim::ParseRequisitesCollector* pc)
{
    if (pc) {
        pc->appendPeriod(flt, prd);
    }
}

} } //ssim::details
