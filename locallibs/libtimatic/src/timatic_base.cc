#include "timatic_base.h"
#include "timatic_exception.h"
#include "timatic_xml.h"

#include <serverlib/dates.h>
#define NICKNAME "TIMATIC"
#include <serverlib/slogger.h>
#include <serverlib/str_utils.h>

namespace Timatic {

ErrorType::ErrorType(const xmlNodePtr node)
{
    for (xmlNodePtr child = node->children; child; child = child->next) {
        if (xCmpNames(child, "message"))
            message = xGetStr(child);
        else if (xCmpNames(child, "code"))
            code = xGetStr(child);
    }
}

//-----------------------------------------------

MessageType::MessageType(const xmlNodePtr node)
{
    for (xmlNodePtr child = node->children; child; child = child->next) {
        if (xCmpNames(child, "text"))
            text = xGetStr(child);
        else if (xCmpNames(child, "code"))
            code = xGetStr(child);
    }
}

//-----------------------------------------------

ParameterType::ParameterType(const xmlNodePtr node)
    : mandatory(false)
{
    for (xmlNodePtr child = node->children; child; child = child->next) {
        if (xCmpNames(child, "parameterName"))
            parameterName = xGetStr(child);
        else if (xCmpNames(child, "mandatory"))
            mandatory = xGetBool(child);
    }
}

//-----------------------------------------------

ParamValue::ParamValue(const xmlNodePtr node)
{
    for (xmlNodePtr child = node->children; child; child = child->next) {
        if (xCmpNames(child, "name"))
            name = xGetStr(child);
        else if (xCmpNames(child, "displayName"))
            displayName = xGetStr(child);
        else if (xCmpNames(child, "code"))
            code = xGetStr(child);
    }
}

//----------------------------------------------

CityDetail::CityDetail(const xmlNodePtr node)
{
    for (xmlNodePtr child = node->children; child; child = child->next) {
        if (xCmpNames(child, "cityName"))
            cityName = xGetStr(child);
        else if (xCmpNames(child, "cityCode"))
            cityCode = xGetStr(child);
    }
}

//----------------------------------------------

AlternateDetail::AlternateDetail(const xmlNodePtr node)
{
    for (xmlNodePtr child = node->children; child; child = child->next) {
        if (xCmpNames(child, "countryName"))
            countryName = xGetStr(child);
        else if (xCmpNames(child, "countryCode"))
            countryCode = xGetStr(child);
    }
}

//----------------------------------------------

VisaType::VisaType(const xmlNodePtr node)
{
    for (xmlNodePtr child = node->children; child; child = child->next) {
        if (xCmpNames(child, "visaType"))
            visaType = xGetStr(child);
        else if (xCmpNames(child, "issuedBy"))
            issuedBy = xGetStr(child);
        else if (xCmpNames(child, "applicableFor"))
            applicableFor = xGetStr(child);
        else if (xCmpNames(child, "expiryDate"))
            expiryDateGMT = getPTime(xGetStr(child));
    }
}

//----------------------------------------------

void StayDuration::validate() const
{
    if (days_ && *days_ < 0)
        throw ValidateError(BIGLOG, "days=%d", *days_);

    if (hours_ && *hours_ < 0)
        throw ValidateError(BIGLOG, "hours=%d", *hours_);
}

void StayDuration::fill(xmlNodePtr node) const
{
    auto child = xmlNewChild(node, nullptr, "stayDuration", nullptr);

    if (days_)
        xmlNewChild(child, nullptr, "days", std::to_string(*days_));

    if (hours_)
        xmlNewChild(child, nullptr, "hours", std::to_string(*hours_));
}

//-----------------------------------------------

void VisaData::validate() const
{
    if (visaType_.size() < 1 || visaType_.size() > 10)
        throw ValidateError(BIGLOG, "visaType=%s", visaType_.c_str());

    if (issuedBy_.size() < 2 || issuedBy_.size() > 4)
        throw ValidateError(BIGLOG, "issuedBy=%s", issuedBy_.c_str());

    if (expiryDateGMT_.is_special())
        throw ValidateError(BIGLOG, "expiryDateGMT is special");
}

void VisaData::fill(xmlNodePtr node) const
{
    auto child = xmlNewChild(node, nullptr, "visaData", nullptr);
    xmlNewChild(child, nullptr, "visaType", visaType_);
    xmlNewChild(child, nullptr, "issuedBy", issuedBy_);
    xmlNewChild(child, nullptr, "expiryDate", toString(expiryDateGMT_));
}

//-----------------------------------------------

void TransitCountry::validate() const
{
    if (airportCode_.size() < 2 || airportCode_.size() > 3)
        throw ValidateError(BIGLOG, "airportCode=%s", airportCode_.c_str());

    if (arrivalTimestamp_ && arrivalTimestamp_->is_special())
        throw ValidateError(BIGLOG, "arrivalTimestamp is special");

    if (departTimestamp_ && departTimestamp_->is_special())
        throw ValidateError(BIGLOG, "departTimestamp is special");

    if (visa_ && (visa_ < Visa::NoVisa || visa_ > Visa::ValidVisa))
        throw ValidateError(BIGLOG, "visa=%d", (int)*visa_);

    if (visaData_)
        visaData_->validate();
}

void TransitCountry::fill(xmlNodePtr node) const
{
    auto child = xmlNewChild(node, nullptr, "transitCountry", nullptr);
    xmlNewChild(child, nullptr, "airportCode", airportCode_);

    if (arrivalTimestamp_)
        xmlNewChild(child, nullptr, "arrivalTimestamp", toString(*arrivalTimestamp_));

    if (departTimestamp_)
        xmlNewChild(child, nullptr, "departTimestamp", toString(*departTimestamp_));

    if (visa_)
        xmlNewChild(child, nullptr, "visa", toString(*visa_));

    if (visaData_)
        visaData_->fill(child);
}

//-----------------------------------------------

Config::Config(const std::string &username, const std::string &subUsername, const std::string &password, const std::string &host, const int port)
    : username(username),
      subUsername(subUsername),
      password(password),
      host(host),
      port(port)
{
}

//-----------------------------------------------


std::string toString(const DataSection &val)
{
    switch (val) {
    case DataSection::PassportVisaHealth: return "Passport,Visa,Health";
    case DataSection::PassportVisa: return "Passport,Visa";
    case DataSection::Passport: return "Passport";
    case DataSection::Health: return "Health";
    case DataSection::All: return "All";
    }
    return {};
}

std::string toString(const DocumentFeature &val)
{
    switch (val) {
    case DocumentFeature::Biometric: return "Biometric";
    case DocumentFeature::DigitalPhoto: return "DigitalPhoto";
    case DocumentFeature::MachineReadableDocument: return "MRD";
    }
    return {};
}

std::string toString(const StayType &val)
{
    switch (val) {
    case StayType::Vacation: return "Vacation";
    case StayType::Business: return "Business";
    case StayType::Duty: return "Duty";
    }
    return {};
}

std::string toString(const Gender &val)
{
    switch (val) {
    case Gender::Male: return "M";
    case Gender::Female: return "F";
    }
    return {};
}

std::string toString(const Visa &val)
{
    switch (val) {
    case Visa::NoVisa: return "NoVisa";
    case Visa::ValidVisa: return "ValidVisa";
    }
    return {};
}

std::string toString(const boost::posix_time::ptime &val)
{
    if (!val.is_special()) {
        // YYYY-MM-DDTHH:MM:SS (19 symbols)
        const auto value = boost::posix_time::to_iso_extended_string(val);
        if (value.size() >= 19)
            return value.substr(0, 19);
    }
    return {};
}

std::string toString(const ParameterName &val)
{
    switch (val) {
    case ParameterName::DocumentType: return "documentType";
    case ParameterName::Gender: return "gender";
    case ParameterName::DataSection: return "dataSection";
    case ParameterName::StayType: return "stayType";
    case ParameterName::Country: return "country";
    case ParameterName::Section: return "section";
    case ParameterName::DocumentGroup: return "documentGroup";
    case ParameterName::Visa: return "visa";
    case ParameterName::ResidencyDocument: return "residencyDocument";
    case ParameterName::Ticket: return "ticket";
    case ParameterName::DocumentFeature: return "documentFeature";
    }
    return {};
}

std::string toString(const GroupName &val)
{
    switch (val) {
    case GroupName::Organisation: return "Organisation";
    case GroupName::Infected: return "Infected";
    }
    return {};
}

//-----------------------------------------------

DataSection getDataSection(const std::string &val)
{
    if (val == "Passport,Visa,Health" || val == "passport,visa,health")
        return DataSection::PassportVisaHealth;

    if (val == "Passport,Visa" || val == "passport,visa")
        return DataSection::PassportVisa;

    if (val == "Passport" || val == "passport")
        return DataSection::Passport;

    if (val == "Health" || val == "health")
        return DataSection::Health;

    if (val == "All" || val == "all")
        return DataSection::All;

    return DataSection::All;
}

SufficientDocumentation getSufficientDocumentation(const std::string &val)
{
    if (val == "Yes" || val == "yes")
        return SufficientDocumentation::Yes;
    else if (val == "Conditional" || val == "conditional")
        return SufficientDocumentation::Conditional;
    else if (val == "No" || val == "no")
        return SufficientDocumentation::No;

    return SufficientDocumentation::No;
}

ParagraphType getParagraphType(const std::string &val)
{
    if (val == "Information" || val == "information")
        return ParagraphType::Information;

    if (val == "Restriction" || val == "restriction")
        return ParagraphType::Restriction;

    if (val == "Exception" || val == "exception")
        return ParagraphType::Exception;

    if (val == "Requirement" || val == "requirement")
        return ParagraphType::Requirement;

    if (val == "Recommendation" || val == "recommendation")
        return ParagraphType::Recommendation;

    if (val == "Applicable" || val == "applicable")
        return ParagraphType::Applicable;

    return ParagraphType::Information;
}

//-----------------------------------------------

boost::posix_time::ptime getPTime(const std::string &val) try
{
    static constexpr size_t datePartSize = 10;

    if (val.empty())
        return boost::posix_time::ptime();

    std::string dateStr;
    std::string timeTzStr;
    boost::date_time::split(val, 'T', dateStr, timeTzStr);
    if (timeTzStr.empty()) {
        if (val.size() > datePartSize) {
            dateStr = val.substr(0, datePartSize);
            timeTzStr = StrUtils::replaceSubstrCopy(val.substr(datePartSize), " ", "");
        } else {
            return boost::posix_time::ptime();
        }
    }

    const boost::posix_time::ptime::date_type date = boost::date_time::parse_date<boost::posix_time::ptime::date_type>(dateStr);
    boost::char_separator<char> sep("-+Z", "-+Z", boost::keep_empty_tokens);
    boost::tokenizer<boost::char_separator<char>> tokens(timeTzStr, sep);
    const std::vector<std::string> arr(tokens.begin(), tokens.end());

    if (arr.empty())
        return boost::posix_time::ptime();

    boost::posix_time::ptime p(date, boost::date_time::parse_delimited_time_duration<boost::posix_time::time_duration>(arr[0]));

    // Due to the Timatic Application servers being hosted in London all dates and times are GMT.
    // The offset should be used if local times are required.
    if (arr.size() == 1)
        return p;

    if (arr.size() != 3)
        return boost::posix_time::ptime();

    if (arr[1] == "+")
        p -= boost::date_time::parse_delimited_time_duration<boost::posix_time::time_duration>(arr[2]);
    else if (arr[1] == "-")
        p += boost::date_time::parse_delimited_time_duration<boost::posix_time::time_duration>(arr[2]);

    return p;

} catch (const std::exception &) {
    return boost::posix_time::ptime();
}

/*
// TODO: need Boost 1.63
boost::posix_time::ptime getPTime(const std::string &val)
{
    if (!val.empty())
        return boost::posix_time::from_iso_extended_string(val);
    return boost::posix_time::not_a_date_time;
}
*/

} // Timatic
