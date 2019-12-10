#pragma once

#include <boost/date_time.hpp>
#include <boost/optional.hpp>
#include <string>


typedef struct _xmlDoc xmlDoc;
typedef xmlDoc *xmlDocPtr;

typedef struct _xmlNode xmlNode;
typedef xmlNode *xmlNodePtr;

typedef struct _xmlNs xmlNs;
typedef xmlNs *xmlNsPtr;


namespace Timatic {

template <class T>
using Optional = boost::optional<T>;

//-----------------------------------------------

class ErrorType {
public:
    ErrorType(const xmlNodePtr node);
    std::string message;
    Optional<std::string> code;
};

//-----------------------------------------------

class MessageType {
public:
    MessageType(const xmlNodePtr node);
    std::string text;
    Optional<std::string> code;
};

//-----------------------------------------------

class ParameterType {
public:
    ParameterType(const xmlNodePtr node);
    std::string parameterName;
    bool mandatory;
};

//-----------------------------------------------

class ParamValue {
public:
    ParamValue(const xmlNodePtr node);
    std::string name;
    std::string displayName;
    Optional<std::string> code;
};

//-----------------------------------------------

class CityDetail {
public:
    CityDetail(const xmlNodePtr node);
    std::string cityName;
    std::string cityCode;
};

//-----------------------------------------------

class AlternateDetail {
public:
    AlternateDetail(const xmlNodePtr node);
    std::string countryName;
    std::string countryCode;
};

//-----------------------------------------------

class VisaType {
public:
    VisaType(const xmlNodePtr node);
    std::string visaType;
    std::string issuedBy;
    std::string applicableFor;
    Optional<boost::posix_time::ptime> expiryDateGMT;
};

//-----------------------------------------------

class StayDuration {
public:
    void validate() const;
    void fill(xmlNodePtr node) const;

    Optional<int> days() const { return days_; }
    Optional<int> hours() const { return hours_; }

    void days(const int val) { days_ = val; }
    void hours(const int val) { hours_ = val; }

private:
    Optional<int> days_;
    Optional<int> hours_;
};

//-----------------------------------------------

enum class DataSection {
    PassportVisaHealth,
    PassportVisa,
    Passport,
    Health,
    All
};

//-----------------------------------------------

enum class DocumentFeature {
    Biometric,
    DigitalPhoto,
    MachineReadableDocument
};

//-----------------------------------------------

enum class StayType {
    Vacation,
    Business,
    Duty
};

//-----------------------------------------------

enum class Gender {
    Male,
    Female
};

//-----------------------------------------------

enum class Visa {
    NoVisa,
    ValidVisa
};

//-----------------------------------------------

enum class Sufficient {
    No,
    Conditional,
    Yes
};

//-----------------------------------------------

enum class ParagraphType {
    Information,
    Restriction,
    Exception,
    Requirement,
    Recommendation,
    Applicable
};

//-----------------------------------------------

enum class ParameterName {
    DocumentType,
    Gender,
    DataSection,
    StayType,
    Country,
    Section,
    DocumentGroup,
    Visa,
    ResidencyDocument,
    Ticket,
    DocumentFeature
};

//-----------------------------------------------

enum class GroupName {
    Organisation,
    Infected
};

//-----------------------------------------------

class VisaData {
public:
    void validate() const;
    void fill(xmlNodePtr node) const;

    const std::string &visaType() const { return visaType_; }
    const std::string &issuedBy() const { return issuedBy_; }
    const boost::posix_time::ptime &expiryDateGMT() const { return expiryDateGMT_; }

    void visaType(const std::string &val) { visaType_ = val; }
    void issuedBy(const std::string &val) { issuedBy_ = val; }
    void expiryDateGMT(const boost::posix_time::ptime &val) { expiryDateGMT_ = val; }

private:
    std::string visaType_;
    std::string issuedBy_;
    boost::posix_time::ptime expiryDateGMT_;
};

//-----------------------------------------------

class TransitCountry {
public:
    void validate() const;
    void fill(xmlNodePtr node) const;

    const std::string &airportCode() const { return airportCode_; }
    const Optional<boost::posix_time::ptime> &arrivalTimestamp() const { return arrivalTimestamp_; }
    const Optional<boost::posix_time::ptime> &departTimestamp() const { return departTimestamp_; }
    const Optional<Visa> &visa() const { return visa_; }
    const Optional<VisaData> &visaData() const { return visaData_; }
    const Optional<std::string> &ticket() const { return ticket_; }

    void airportCode(const std::string &val) { airportCode_ = val; }
    void arrivalTimestamp(const boost::posix_time::ptime &val) { arrivalTimestamp_ = val; }
    void departTimestamp(const boost::posix_time::ptime &val) { departTimestamp_ = val; }
    void visa(const Visa &val) { visa_ = val; }
    void visaData(const VisaData &val) { visaData_ = val; }
    void ticket(const std::string &val) { ticket_ = val; }

private:
    std::string airportCode_;
    Optional<boost::posix_time::ptime> arrivalTimestamp_;
    Optional<boost::posix_time::ptime> departTimestamp_;
    Optional<Visa> visa_;
    Optional<VisaData> visaData_;
    Optional<std::string> ticket_;
};

//-----------------------------------------------

struct Config {
    Config(const std::string &username = "",
           const std::string &subUsername = "",
           const std::string &password = "",
           const std::string &host = "",
           const int port = 443);
    std::string username;
    std::string subUsername;
    std::string password;
    std::string host;
    int port;
};

struct Session {
    std::string sessionID;
    std::string jsessionID;
    int expires = 0;
};

//-----------------------------------------------

std::string toString(const DataSection &val);
std::string toString(const DocumentFeature &val);
std::string toString(const StayType &val);
std::string toString(const Gender &val);
std::string toString(const Visa &val);
std::string toString(const boost::posix_time::ptime &val);
std::string toString(const ParameterName &val);
std::string toString(const GroupName &val);

//-----------------------------------------------

DataSection getDataSection(const std::string &val);
Sufficient getSufficient(const std::string &val);
ParagraphType getParagraphType(const std::string &val);
boost::posix_time::ptime getPTime(const std::string &val);

} // Timatic
