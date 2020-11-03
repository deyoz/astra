#pragma once

#include <string>
#include <map>
#include <list>

#include "coretypes/flight.h"
#include "coretypes/route.h"
#include "coretypes/rbdorder.h"

#include "meal_service.h"
#include "traffic_restriction.h"
#include "ssim_schedule.h"

namespace ssim {

class SsimTlgCallbacks;

struct ProcContext
{
    nsi::CompanyId ourComp;
    bool sloader;
    bool timeIsLocal;
    bool inventoryHost;
    std::unique_ptr<SsimTlgCallbacks> callbacks;
};

using RawDeiMap = std::map<ct::DeiCode, std::string>;

enum class AgreementType {
    CSH, FRANCH, AIRCR, COCKPIT, CREW
};

struct AgreementDesc
{
    AgreementType tp;
    ct::DeiCode code; //agreement indicator
    ct::DeiCode desc; //agreement describer (in case of X)
};

const std::vector<AgreementDesc>& agreementDescs();
boost::optional<AgreementType> agrTypeByDei(const ct::DeiCode&);

struct PartnerInfo
{
    boost::optional<nsi::CompanyId> company; //empty value means X
};

struct DeiInfo
{
    RawDeiMap di;

    std::map<AgreementType, PartnerInfo> partners;
    boost::optional<ssim::MealServiceInfo> meals; //can be filled only for routing info
    bool mealsContinuationExpected;         //it meens 7/XX

    DeiInfo(const RawDeiMap&);

    std::string dei(ct::DeiCode) const;
    bool fillPartnerInfo();
    bool fillMeals();
};
std::ostream& operator << (std::ostream&, const DeiInfo&);

struct FlightInfo : public DeiInfo
{
    ct::Flight flt;

    FlightInfo(const ct::Flight&, const RawDeiMap& = RawDeiMap());
};
std::ostream& operator<< (std::ostream&, const FlightInfo&);

struct EquipmentInfo : public DeiInfo
{
    nsi::ServiceTypeId serviceType;
    nsi::AircraftTypeId tts;
    ct::RbdsConfig config;

    EquipmentInfo(nsi::ServiceTypeId, nsi::AircraftTypeId, const ct::RbdsConfig&, const RawDeiMap& = RawDeiMap());
};
std::ostream& operator<< (std::ostream&, const EquipmentInfo&);
bool operator==(const EquipmentInfo&, const EquipmentInfo&);

struct RoutingInfo : public DeiInfo
{
    nsi::PointId dep;
    boost::posix_time::time_duration depTime;
    std::string pasSTD;
    nsi::PointId arr;
    boost::posix_time::time_duration arrTime;
    std::string pasSTA;

    RoutingInfo(
        nsi::PointId d, const boost::posix_time::time_duration& tm1, const std::string& psd,
        nsi::PointId a, const boost::posix_time::time_duration& tm2, const std::string& psa,
        const RawDeiMap& = RawDeiMap()
    );

    std::string toString(bool ssm = true, const boost::gregorian::date& = boost::gregorian::date()) const;
};
bool operator==(const RoutingInfo&, const RoutingInfo&);

struct SegmentInfo
{
    //raw data
    nsi::PointId dep;
    nsi::PointId arr;
    ct::DeiCode deiType;
    std::string dei;

    //parsed data
    bool resetValue;    //NIL

    boost::optional<ct::Flight> oprFl;
    std::vector<ct::Flight> manFls;
    boost::optional<ssim::Restrictions> trs;
    boost::optional<bool> et;
    boost::optional<bool> secureFlight;
    boost::optional<ssim::MealServiceInfo> meals;
    boost::optional<ssim::Franchise> oprDisclosure;

    SegmentInfo(nsi::PointId p1, nsi::PointId p2, ct::DeiCode, const std::string&);
};
std::ostream& operator<< (std::ostream&, const SegmentInfo&);

struct ReferenceInfo
{
    boost::gregorian::date date;
    unsigned int gid;
    bool isLast;
    unsigned int mid;
    std::string creatorRef;
};
std::ostream& operator<< (std::ostream&, const ReferenceInfo&);

struct LegStuff
{
    RoutingInfo ri;
    EquipmentInfo eqi;
};

struct MsgHead
{
    std::string sender;
    std::list<std::string> receiver;
    std::string msgId;
    bool timeIsLocal;
    boost::optional<ReferenceInfo> ref;

    //FIXME: store message type?
    std::string toString(bool ssm = true) const;
};

using RoutingInfoList = std::vector<RoutingInfo>;
using SegmentInfoList = std::vector<SegmentInfo>;
using LegStuffList    = std::vector<LegStuff>;

struct PubOptions
{
    bool latinOnly;
};

} //ssim
