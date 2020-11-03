#pragma once

#include "oag_records.h"

#include <string>
#include <set>
#include <memory>

namespace Oag {

class OagRecordsHandler
{
public:

    typedef std::shared_ptr<OagRecordsHandler> OagRecordsHandlerPtr;

public:

    OagRecordsHandler(const std::string& i_outFile) : outFile(i_outFile) {}

    virtual ~OagRecordsHandler() {}

    void handleRecords(const CarrierRecord&);

    void handleCarrierProcBegin(const std::string&);
    void handleCarrierProcEnd(const std::string&, const CarrierRecord&);

private:
    virtual void carrierProcBegin(const std::string&, std::ostream&) = 0;
    virtual void carrierProcEnd(const std::string&, const CarrierRecord&, std::ostream&) = 0;

    virtual std::string MakeHeading(const CarrierRecord&) = 0;
    virtual std::string MakeActionInfo(const CarrierRecord&) = 0;
    virtual std::string MakeFlightInfo(const CarrierRecord&) = 0;
    virtual std::string MakePeriodInfo(const CarrierRecord&) = 0;
    virtual std::string MakeRoutingInfo(const CarrierRecord&) = 0;
    virtual std::string MakeSegmentInfo(const CarrierRecord&) = 0;

    const std::string outFile;
};


class HeaderComposer
{
    std::string reciever;
    std::string sender;
    std::string center;
public:
    explicit HeaderComposer(const std::string& ctr);
    HeaderComposer(const std::string& rcv, const std::string& snd);
    std::string compose(const std::string& airline) const;
};

struct SsmGenerationOptions
{
    bool forceInfiniteValidity; //for SKD
    bool cancelMissingFlights;  //generate CNL for flights that not mentioned in import file
    std::function< std::set<std::string> (const std::set<std::string>&) > findMissingFlights;
};

class OagSsmCreator : public OagRecordsHandler
{
public:
    OagSsmCreator(const HeaderComposer&, const SsmGenerationOptions&, const std::string& outFile);

private:
    virtual void carrierProcBegin(const std::string&, std::ostream&);
    virtual void carrierProcEnd(const std::string&, const CarrierRecord&, std::ostream&);

    virtual std::string MakeHeading(const CarrierRecord&);
    virtual std::string MakeActionInfo(const CarrierRecord&);
    virtual std::string MakeFlightInfo(const CarrierRecord&);
    virtual std::string MakePeriodInfo(const CarrierRecord&);
    virtual std::string MakeRoutingInfo(const CarrierRecord&);
    virtual std::string MakeSegmentInfo(const CarrierRecord&);

    std::string makeEquipmentInfo(const FlightLegRecord&) const;

    struct SaleConfRecord {
        std::string subcls;
        std::string airConf;
    };

    typedef std::map<std::string, SaleConfRecord> SaleMap;
    SaleMap saleConfs;

    HeaderComposer headerComposer;
    SsmGenerationOptions genOpts;
    std::string flight;

    std::string airline;
    std::set<std::string> flights;
};

}
