#include <iomanip>

#include <serverlib/dates.h>

#include "ssim_data_types.h"
#include "ssim_enums.h"
#include "ssim_utils.h"

#define NICKNAME "ASH"
#include <serverlib/slogger.h>

namespace ssim {

ENUM_NAMES_BEGIN(AsmActionType)
    ( ASM_NEW, "NEW" )
    ( ASM_CNL, "CNL" )
    ( ASM_RIN, "RIN" )
    ( ASM_RPL, "RPL" )
    ( ASM_ACK, "ACK" )
    ( ASM_ADM, "ADM" )
    ( ASM_CON, "CON" )
    ( ASM_EQT, "EQT" )
    ( ASM_FLT, "FLT" )
    ( ASM_NAC, "NAC" )
    ( ASM_RRT, "RRT" )
    ( ASM_TIM, "TIM" )
ENUM_NAMES_END(AsmActionType)

ENUM_NAMES_BEGIN(SsmActionType)
    ( SSM_NEW, "NEW" )
    ( SSM_CNL, "CNL" )
    ( SSM_RPL, "RPL" )
    ( SSM_SKD, "SKD" )
    ( SSM_ACK, "ACK" )
    ( SSM_ADM, "ADM" )
    ( SSM_CON, "CON" )
    ( SSM_EQT, "EQT" )
    ( SSM_FLT, "FLT" )
    ( SSM_NAC, "NAC" )
    ( SSM_REV, "REV" )
    ( SSM_RSD, "RSD" )
    ( SSM_TIM, "TIM" )
ENUM_NAMES_END(SsmActionType)

const std::vector<AgreementDesc>& agreementDescs()
{
    static std::vector<AgreementDesc> descs({
        { AgreementType::CSH,     ct::DeiCode(2), ct::DeiCode(50) },
        { AgreementType::FRANCH,  ct::DeiCode(9), ct::DeiCode(127) },
        { AgreementType::AIRCR,   ct::DeiCode(3), ct::DeiCode(113) },
        { AgreementType::COCKPIT, ct::DeiCode(4), ct::DeiCode(114) },
        { AgreementType::CREW,    ct::DeiCode(5), ct::DeiCode(115) }
    });
    return descs;
}

boost::optional<AgreementType> agrTypeByDei(const ct::DeiCode& c)
{
    for (const auto& a : agreementDescs()) {
        if (a.code == c) {
            return a.tp;
        }
    }
    return boost::none;
}

std::ostream& operator << (std::ostream& os, const ReferenceInfo & ri)
{
    os << Dates::ddmon(ri.date, ENGLISH) << std::setw(5) << std::setfill('0') << ri.gid;
    os << (ri.isLast ? "E" : "C") << std::setw(3) << std::setfill('0') << ri.mid << ri.creatorRef;
    return os;
}

std::ostream& operator << (std::ostream& os, const RawDeiMap& di)
{
    for (const auto& v : di) {
        os << ' ' << v.first << '/' << v.second;
    }
    return os;
}

std::ostream& operator << (std::ostream& os, const DeiInfo& d)
{
    std::set<ct::DeiCode> agrs;
    for (const auto& v : d.partners) {
        for (const AgreementDesc& ad : agreementDescs()) {
            if (ad.tp == v.first) {
                os << ' ' << ad.code << '/' << (v.second.company ? nsi::Company(*v.second.company).code(ENGLISH).toUtf() : "X");
                agrs.emplace(ad.code);
            }
        }
    }

    for (const auto& v : d.di) {
        if (agrs.find(v.first) != agrs.end()) {
            continue;
        }
        os << ' ' << v.first << '/' << v.second;
    }
    return os;
}

std::ostream& operator << (std::ostream & os, const FlightInfo& fi)
{
    os << fi.flt.toTlgStringUtf();
    os << static_cast<const DeiInfo &>(fi);
    return os;
}

std::ostream & operator << (std::ostream & os, const EquipmentInfo & ei)
{
    os << nsi::ServiceType(ei.serviceType).code(ENGLISH) << ' ';
    os << nsi::AircraftType(ei.tts).code(ENGLISH) << ' ';
    os << ct::toString(ei.config, ENGLISH);
    os << static_cast<const DeiInfo &>(ei);
    return os;
}

bool operator==(const EquipmentInfo& x, const EquipmentInfo& y)
{
    return x.serviceType == y.serviceType
        && x.tts == y.tts
        && x.config == y.config;
}

std::ostream & operator << (std::ostream & os, const SegmentInfo& si)
{
    os << ssim::pointCode(si.dep) << ssim::pointCode(si.arr) << " " << si.deiType << "/" << si.dei;
    return os;
}

DeiInfo::DeiInfo(const RawDeiMap& v)
    : di(v), mealsContinuationExpected(false)
{}

std::string DeiInfo::dei(ct::DeiCode i) const
{
    auto it = this->di.find(i);
    if (it != this->di.end()) {
        return it->second;
    }
    return std::string();
}

bool DeiInfo::fillPartnerInfo()
{
    std::map<AgreementType, std::list<PartnerInfo> > ret;
    for (const auto& p : this->di) {
        boost::optional<AgreementType> tp = agrTypeByDei(p.first);
        if (!tp) {
            //not agreement
            continue;
        }
        LogTrace(TRACE5) << "Check for company " << p.second;
        if (p.second == "X") {
            ret[*tp].push_back(PartnerInfo {});
        } else {
            const EncString c(EncString::from866(p.second));
            if (!nsi::Company::find(c)) {
                return false;
            }
            ret[*tp].push_back(PartnerInfo { nsi::Company(c).id() });
        }
    }

    for (const auto& v : ret) {
        if (v.second.size() > 1) {
            LogTrace(TRACE1) << "More than one agreement " << static_cast<int>(v.first) << " dei in flight element";
            return false;
        }
        this->partners[v.first] = v.second.front();
    }
    return true;
}

bool DeiInfo::fillMeals()
{
    auto mi = di.find(ct::DeiCode(7));
    if (mi == di.end()) {
        return true;
    }

    const std::string& cnt = mi->second;
    if (cnt == "XX") {
        mealsContinuationExpected = true;
        return true;
    }
    if ((meals = ssim::getMealsMap(cnt))) {
        return true;
    }
    return false;
}

FlightInfo::FlightInfo(const ct::Flight& f, const RawDeiMap& m)
    : DeiInfo(m), flt(f)
{}

EquipmentInfo::EquipmentInfo(nsi::ServiceTypeId s, nsi::AircraftTypeId a, const ct::RbdsConfig& c, const RawDeiMap& m)
    : DeiInfo(m), serviceType(s), tts(a), config(c)
{}

RoutingInfo::RoutingInfo(
        nsi::PointId d, const boost::posix_time::time_duration& tm1, const std::string& psd,
        nsi::PointId a, const boost::posix_time::time_duration& tm2, const std::string& psa,
        const RawDeiMap& ds
    )
    : DeiInfo(ds), dep(d), depTime(tm1), pasSTD(psd), arr(a), arrTime(tm2), pasSTA(psa)
{}

bool operator==(const RoutingInfo& x, const RoutingInfo& y)
{
    return x.dep == y.dep
        && x.depTime == y.depTime
        && x.pasSTD == y.pasSTD
        && x.arr == y.arr
        && x.arrTime == y.arrTime
        && x.pasSTA == y.pasSTA;
}

static void timeToString(std::ostream & os, const std::string & time, int shift, bool ssm, const boost::gregorian::date & dt)
{
    if (ssm) { // SSM, сдвиг даты через слэш
        os << time;
        if (shift < 0)
            os << "/M" << (-1) * shift;
        else if (shift > 0)
            os << "/" << shift;
    } else { // ASM, сдвиг даты - число перед временем
        if (!dt.is_not_a_date())
            os << Dates::ddmmrr(dt + boost::gregorian::days(shift)).substr(0,2);
        os << time;
    }
}

std::string RoutingInfo::toString(bool ssm, const boost::gregorian::date& dt) const
{
    std::stringstream os;
    std::string dep = nsi::Point(this->dep).code(ENGLISH).toUtf();
    std::string arr = nsi::Point(this->arr).code(ENGLISH).toUtf();
    os << dep;
    int shift = Dates::daysOffset(this->depTime).days();
    std::string time = Dates::hh24mi(this->depTime - boost::posix_time::hours(shift * 24), false).substr(0,4);
    timeToString(os, time, shift, ssm, dt);
    if (!this->pasSTD.empty()) {
        if (ssm && shift == 0) // SSM без сдвига даты, нужно вывести, что он отсутствует
            os << "/0";
        os << "/" << this->pasSTD;
    }
    os << " " << arr;
    shift = Dates::daysOffset(this->arrTime).days();
    time = Dates::hh24mi(this->arrTime - boost::posix_time::hours(shift * 24), false).substr(0,4);
    // тонкость -- полночь во времени отправления это 2400
    if (time == "0000") {
        time = "2400";
        shift--;
    }
    timeToString(os, time, shift, ssm, dt);
    if (!this->pasSTA.empty()) {
        if (ssm && shift == 0) // SSM без сдвига даты, нужно вывести, что он отсутствует
            os << "/0";
        os << "/" << this->pasSTA;
    }
    os << static_cast<const DeiInfo &>(*this);
    return os.str();
}

SegmentInfo::SegmentInfo(nsi::PointId p1, nsi::PointId p2, ct::DeiCode d, const std::string& t)
    : dep(p1), arr(p2), deiType(d), dei(t), resetValue(false)
{ }

std::string MsgHead::toString(bool ssm) const
{
    std::stringstream os;
    for (const std::string& s : receiver) {
        os << s;
        if (s != receiver.back()) {
            os << " ";
        }
    }
    os << "\n." << sender << " ";
    os << msgId << std::endl;
    os << (ssm ? "SSM\n" : "ASM\n");
    if (timeIsLocal) {
        os << "LT\n";
    }
    if (ssm && ref) {
        // TODO сиреновский копипаст, вообще по ssim можно и в asm
        os << *ref << std::endl;
    }
    return os.str();
}

} //ssim
