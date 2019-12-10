#pragma once

#include "timatic_base.h"

#include <boost/date_time.hpp>

namespace Timatic {

enum class RequestType {
    Base,
    // processLogin
    CheckName,
    Login,
    // processRequest
    Document,
    Param,
    ParamValues,
    Country,
    Visa,
    //
    CountryInformation,
    News,
    Notification,
    Terms,
    Download,
    Cli,
    CliLogger,
    CliKeyword,
};

class Request {
public:
    virtual ~Request() = default;
    virtual void validate() const;
    std::string content(const Session &) const;
    RequestType type() const { return type_; }

    int64_t requestID() const { return requestID_; }
    const Optional<std::string> &subRequestID() const { return subRequestID_; }
    const Optional<std::string> &customerCode() const { return customerCode_; }

    void requestID(const int64_t val) { requestID_ = val; }
    void subRequestID(const std::string &val) { subRequestID_ = val; }
    void customerCode(const std::string &val) { customerCode_ = val; }

protected:
    Request(RequestType type);
    virtual std::string payload(const Session &session) const;
    virtual void fill(xmlNodePtr node) const;

private:
    RequestType type_;
    int64_t requestID_;
    Optional<std::string> subRequestID_;
    Optional<std::string> customerCode_;
};

//-----------------------------------------------

class CheckNameReq final : public Request {
public:
    CheckNameReq();
    void validate() const override;

    const std::string &username() const { return username_; }
    const std::string &subUsername() const { return subUsername_; }

    void username(const std::string &val) { username_ = val; }
    void subUsername(const std::string &val) { subUsername_ = val; }

protected:
    std::string payload(const Session &session) const override;
    void fill(xmlNodePtr node) const override;

private:
    std::string username_;
    std::string subUsername_;
};

//-----------------------------------------------

class LoginReq final : public Request {
public:
    LoginReq();
    void validate() const override;

    const std::string &loginInfo() const { return loginInfo_; }
    void loginInfo(const std::string &val) { loginInfo_ = val; }

protected:
    std::string payload(const Session &session) const override;
    void fill(xmlNodePtr node) const override;

private:
    std::string loginInfo_;
};

//-----------------------------------------------

/*
 * The less data provided, the lengthier the information returned,
 * and it may not be possible to filter irrelevant information.
 *
 * In practice, the following fields are crucial for receiving a clean Yes/No response:
 * - Destination
 * - Duration of Stay
 * - Nationality
 * - Document Type
 * - Document Expiry Date
 * - Document Group
 * - Arrival Date
 * - Visa
 * - Ticket
 *
 */

class DocumentReq final : public Request {
public:
    DocumentReq();
    void validate() const override;

    // setters
    // mandatory (must be set)
    DataSection section() const { return section_; }
    const std::string &destinationCode() const { return destinationCode_; }
    const std::string &nationalityCode() const { return nationalityCode_; }
    const std::string &documentType() const { return documentType_; }
    const std::string &documentGroup() const  { return documentGroup_; }
    // mandatory2 (may be absent)
    const Optional<boost::posix_time::ptime> &arrivalDateGMT() const { return arrivalDateGMT_; }
    const Optional<std::string> &carrierCode() const { return carrierCode_; }
    const Optional<std::string> &ticket() const { return ticket_; }
    const Optional<std::string> &passportSeries() const { return passportSeries_; }
    const std::vector<TransitCountry> &transitCountry() const { return transitCountry_; }
    const Optional<StayDuration> &stayDuration() const { return stayDuration_; }
    const Optional<Visa> &visa() const { return visa_; }
    const Optional<VisaData> &visaData() const { return visaData_; }
    const Optional<std::string> &residencyDocument() const { return residencyDocument_; }
    // optional
    const Optional<boost::posix_time::ptime> &departDateGMT() const { return departDateGMT_; }
    const Optional<std::string> &departAirportCode() const { return departAirportCode_; }
    const Optional<std::string> &issueCountryCode() const { return issueCountryCode_; }
    const Optional<boost::posix_time::ptime> &issueDateGMT() const { return issueDateGMT_; }
    const Optional<DocumentFeature> &documentFeature() const { return documentFeature_; }
    const Optional<std::string> &documentLanguage() const { return documentLanguage_; }
    const Optional<StayType> &stayType() const { return stayType_; }
    const Optional<Gender> &gender() const { return gender_; }
    const Optional<std::string> &birthCountryCode() const { return birthCountryCode_; }
    const Optional<boost::posix_time::ptime> &birthDateGMT() const { return birthDateGMT_; }
    const Optional<std::string> &residentCountryCode() const { return residentCountryCode_; }
    const Optional<std::string> &secondaryDocumentType() const { return secondaryDocumentType_; }
    const std::vector<std::string> &countriesVisited() const { return countriesVisited_; }

    // getters
    // mandatory
    void section(const DataSection val) { section_ = val; }
    void destinationCode(const std::string &val) { destinationCode_ = val; }
    void nationalityCode(const std::string &val) { nationalityCode_ = val; }
    void documentType(const std::string &val) { documentType_ = val; }
    void documentGroup(const std::string &val) { documentGroup_ = val; }
    // mandatory2
    void arrivalDateGMT(const boost::posix_time::ptime &val) { arrivalDateGMT_ = val; }
    void carrierCode(const std::string &val) { carrierCode_ = val; }
    void ticket(const std::string &val) { ticket_ = val; }
    void passportSeries(const std::string &val) { passportSeries_ = val; }
    void transitCountry(const TransitCountry &val) { transitCountry_.emplace_back(val); }
    void stayDuration(StayDuration val) { stayDuration_ = val; }
    void visa(const Visa val) { visa_ = val; }
    void visaData(const VisaData &val) { visaData_ = val; }
    void residencyDocument(const std::string &val) { residencyDocument_ = val; }
    // optional
    void departDateGMT(const boost::posix_time::ptime &val) { departDateGMT_ = val; }
    void departAirportCode(const std::string &val) { departAirportCode_ = val; }
    void issueCountryCode(const std::string &val) { issueCountryCode_ = val; }
    void issueDateGMT(const boost::posix_time::ptime &val) { issueDateGMT_ = val; }
    void documentFeature(const DocumentFeature &val) { documentFeature_ = val; }
    void documentLanguage(const std::string &val) { documentLanguage_ = val; }
    void stayType(const StayType &val) { stayType_ = val; }
    void gender(const Gender &val) { gender_ = val; }
    void birthCountryCode(const std::string &val) { birthCountryCode_ = val; }
    void birthDateGMT(const boost::posix_time::ptime &val) { birthDateGMT_ = val; }
    void residentCountryCode(const std::string &val) { residentCountryCode_ = val; }
    void secondaryDocumentType(const std::string &val) { secondaryDocumentType_ = val; }
    void countriesVisited(const std::string &val) { countriesVisited_.emplace_back(val); }

protected:
    std::string payload(const Session &session) const override;
    void fill(xmlNodePtr node) const override;

private:
    // mandatory
    DataSection section_;
    std::string destinationCode_;
    std::string nationalityCode_;
    std::string documentType_;
    std::string documentGroup_;
    // mandatory2
    Optional<boost::posix_time::ptime> arrivalDateGMT_;
    Optional<std::string> carrierCode_;
    Optional<std::string> ticket_;
    Optional<std::string> passportSeries_;
    std::vector<TransitCountry> transitCountry_;
    Optional<StayDuration> stayDuration_;
    Optional<Visa> visa_;
    Optional<VisaData> visaData_;
    Optional<std::string> residencyDocument_;
    // optional
    Optional<boost::posix_time::ptime> departDateGMT_;
    Optional<std::string> departAirportCode_;
    Optional<std::string> issueCountryCode_;
    Optional<boost::posix_time::ptime> issueDateGMT_;
    Optional<DocumentFeature> documentFeature_;
    Optional<std::string> documentLanguage_;
    Optional<StayType> stayType_;
    Optional<Gender> gender_;
    Optional<std::string> birthCountryCode_;
    Optional<boost::posix_time::ptime> birthDateGMT_;
    Optional<std::string> residentCountryCode_;
    Optional<std::string> secondaryDocumentType_;
    std::vector<std::string> countriesVisited_;
};

//-----------------------------------------------

class ParamReq final : public Request {
public:
    ParamReq();
    void validate() const override;

    const std::string &countryCode() const { return countryCode_; }
    DataSection section() const { return section_; }
    const std::vector<std::string> &transitCountry() const { return transitCountry_; }
    const Optional<boost::posix_time::ptime> &queryDateGMT() const { return queryDateGMT_; }

    void countryCode(const std::string &val) { countryCode_ = val; }
    void section(const DataSection val) { section_ = val; }
    void transitCountry(const std::string &val) { transitCountry_.emplace_back(val); }
    void queryDateGMT(const boost::posix_time::ptime &val) { queryDateGMT_ = val; }

protected:
    std::string payload(const Session &session) const override;
    void fill(xmlNodePtr node) const override;

private:
    std::string countryCode_;
    DataSection section_;
    std::vector<std::string> transitCountry_;
    Optional<boost::posix_time::ptime> queryDateGMT_;
};

//-----------------------------------------------

class ParamValuesReq final : public Request {
public:
    ParamValuesReq();
    void validate() const override;

    ParameterName parameterName() const { return parameterName_; }
    const Optional<std::string> section() const { return section_; }
    const Optional<boost::posix_time::ptime> &queryDateGMT() const { return queryDateGMT_; }

    void parameterName(const ParameterName val) { parameterName_ = val; }
    void section(const std::string &val) { section_ = val; }
    void queryDateGMT(const boost::posix_time::ptime &val) { queryDateGMT_ = val; }

protected:
    std::string payload(const Session &session) const override;
    void fill(xmlNodePtr node) const override;

private:
    ParameterName parameterName_;
    Optional<std::string> section_;
    Optional<boost::posix_time::ptime> queryDateGMT_;
};

//-----------------------------------------------

class CountryReq final : public Request {
public:
    CountryReq();
    void validate() const override;

    const Optional<std::string> &countryCode() const { return countryCode_; }
    const Optional<std::string> &countryName() const { return countryName_; }
    const Optional<std::string> &cityCode() const { return cityCode_; }
    const Optional<std::string> &cityName() const { return cityName_; }
    const Optional<std::string> &groupCode() const { return groupCode_; }
    const Optional<GroupName> &groupName() const { return groupName_; }

    void countryCode(const std::string &val) { countryCode_ = val; }
    void countryName(const std::string &val) { countryName_ = val; }
    void cityCode(const std::string &val) { cityCode_ = val; }
    void cityName(const std::string &val) { cityName_ = val; }
    void groupCode(const std::string &val) { groupCode_ = val; }
    void groupName(const GroupName val) { groupName_ = val; }

protected:
    std::string payload(const Session &session) const override;
    void fill(xmlNodePtr node) const override;

private:
    Optional<std::string> countryCode_;
    Optional<std::string> countryName_;
    Optional<std::string> cityCode_;
    Optional<std::string> cityName_;
    Optional<std::string> groupCode_;
    Optional<GroupName> groupName_;
};

//-----------------------------------------------

class VisaReq final : public Request {
public:
    VisaReq();
    void validate() const override;

    const Optional<std::string> &countryCode() const { return countryCode_; }
    void countryCode(const std::string &val) { countryCode_ = val; }

protected:
    std::string payload(const Session &session) const override;
    void fill(xmlNodePtr node) const override;

private:
    Optional<std::string> countryCode_;
};

} // Timatic
