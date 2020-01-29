#pragma once

#include <memory>

#include "oag_records.h"

namespace Oag {

class OagFilter
{
public:

    typedef std::shared_ptr<OagFilter> OagFilterPtr;

public:

    virtual ~OagFilter() {}

    virtual bool applyFilter(const CarrierRecord&) = 0;

    virtual bool checkAirline(const CarrierRecord&) = 0;
};

class OagFlightFilter : public OagFilter
{
public:

    OagFlightFilter(const std::string& airline, std::size_t flightNum = std::size_t(-1));

    virtual bool applyFilter(const CarrierRecord&);

    virtual bool checkAirline(const CarrierRecord&);

private:
    std::string airline;
    std::size_t flightNum;
};

class ServiceTypeFilter : public OagFilter
{
    std::string ignoredServices;
public:
    explicit ServiceTypeFilter(const std::string& = "F");
    virtual bool applyFilter(const CarrierRecord&);
    virtual bool checkAirline(const CarrierRecord&);
};

class OagDummyFilter : public OagFilter
{
public:

    virtual bool applyFilter(const CarrierRecord&) { return false; }
    virtual bool checkAirline(const CarrierRecord&) { return true; }
};

}
