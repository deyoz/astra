#pragma once

#include <vector>
#include <string>
#include <map>

#include <boost/optional/optional.hpp>

namespace Oag {

struct FlightLegRecord;

struct CarrierRecord
{
    enum TimeMode { UNKNOWN_TIME_MODE = 0, UTC_TIME_MODE, LOCAL_TIME_MODE };
    typedef std::vector<FlightLegRecord> FlightLegs;

    TimeMode timeMode;
    std::string airline;
    boost::optional<std::string> season;

    std::string schedulePeriodFrom;
    std::string schedulePeriodTo;

    std::string creationDate;
    std::string dataTitle;
    std::string releaseDate;
    char scheduleStatus;

    boost::optional<std::string> flightServiceInfo;
    boost::optional<std::string> eTicketInfo;
    char secureFlight;

    std::string creationTime;
    std::size_t serialNum;

    FlightLegs flightLegs;
};

struct DepArrPoints
{
    std::string dep;
    std::string arr;

    DepArrPoints(const std::string& i_dep, const std::string& i_arr)
        : dep(i_dep), arr(i_arr)
    {}
};

struct SegmentData
{
    char serviceType;
    DepArrPoints dap;
    std::string data;

    SegmentData(char i_serviceType, const DepArrPoints& i_dap, const std::string& i_data)
        : serviceType(i_serviceType), dap(i_dap), data(i_data)
    {}
};

struct FlightLegRecord
{
    boost::optional<std::string> opSuffix;
    std::string airline;
    std::size_t flightNum;

    std::size_t itinVarId;
    std::size_t legSeqNum;
    char serviceType;

    std::string opPeriodFrom;
    std::string opPeriodTo;
    std::string opDays;
    bool biweekly;

    std::string departureStation;
    std::string schPassDepTime;
    std::string schAirDepTime;
    std::string timeVariationDep;
    boost::optional<std::string> arrPassTerminal;

    std::string arrivalStation;
    std::string schAirArrTime;
    std::string schPassArrTime;
    std::string timeVariationArr;
    boost::optional<std::string> depPassTerminal;

    std::string airType;

    boost::optional<std::string> passReservBookDes;
    boost::optional<std::string> passReservBookModif;

    std::string mealServNote;
    boost::optional<std::string> opAirDesignators;
    boost::optional<std::string> minIntConTime;
    char secureFlight;
    std::string aircraftOwner;

    boost::optional<char> airDiscloseCsh;

    std::string traffRestrCode;
    bool traffRestrOverflow;
    boost::optional<std::string> airConfig;

    boost::optional<char> depDateVar;
    boost::optional<char> arrDateVar;

    std::size_t serialNum;

    std::map<std::size_t, std::vector<SegmentData>> segData; // dei code + segment data
};

struct TrailerRecord
{
    std::string airline;
    std::string releaseDate;

    std::size_t serialNumCheckRef;
    char code;
    std::size_t serialNum;
};

}
