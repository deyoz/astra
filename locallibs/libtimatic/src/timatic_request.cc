#include "timatic_request.h"
#include "timatic_exception.h"
#include "timatic_xml.h"

#define NICKNAME "TIMATIC"
#include <serverlib/slogger.h>
#include <serverlib/str_utils.h>

namespace Timatic {

static const std::string ENVELOPE =
        "<?xml version='1.0' encoding='UTF-8'?>"
        "<env:Envelope xmlns:env='http://schemas.xmlsoap.org/soap/envelope' "
                      "xmlns:xsd='http://www.w3.org/2001/XMLSchema' "
                      "xmlns:xsi='http://www.w3.org/2001/XMLSchema-instance'>"
          "<env:Header>"
          "</env:Header>"
          "<env:Body>"
          "</env:Body>"
        "</env:Envelope>";

static void writeSession(XmlDoc doc, const Session &session)
{
    if (session.sessionID.empty())
        return;

    auto rootNode = xmlDocGetRootElement(doc.get());
    auto headNode = xFindNode(rootNode, "Header");

    auto sessNode = xmlNewChild(headNode, nullptr, "sessionID", session.sessionID);
    if (!sessNode)
        throw Error(BIGLOG, "sessNode is null");
    xmlSetNs(sessNode, xmlNewNs(sessNode, "http://www.opentravel.org/OTA/2003/05/beta", "m"));

    auto envNs = xmlSearchNs(doc.get(), nullptr, (const xmlChar *)"env");
    xmlSetNsProp(sessNode, envNs, (const xmlChar *)"actor", (const xmlChar *)"http://schemas.xmlsoap.org/soap/actor/next");
    xmlSetNsProp(sessNode, envNs, (const xmlChar *)"mustUnderstand", (const xmlChar *)"0");
}

static xmlNodePtr getWorkNode(XmlDoc doc, const char *parentName, const char *childName)
{
    auto rootNode = xmlDocGetRootElement(doc.get());
    auto bodyNode = xFindNode(rootNode, "Body");

    auto procNode = xmlNewChild(bodyNode, nullptr, parentName, nullptr);
    if (!procNode)
        throw Error(BIGLOG, "procNode is null, parentName=%s", parentName);
    xmlSetNs(procNode, xmlNewNs(procNode, "http://www.opentravel.org/OTA/2003/05/beta", "m"));

    auto workNode = xmlNewChild(procNode, nullptr, childName, nullptr);
    if (!workNode)
        throw Error(BIGLOG, "workNode is null, childName=%s", childName);
    xmlSetNs(workNode, nullptr);
    return workNode;
}

//-----------------------------------------------

Request::Request(RequestType type)
    : type_(type), requestID_(-1)
{
}

void Request::validate() const
{
    if (requestID_ < 0 || requestID_ > 9999999999)
        throw ValidateError(BIGLOG, "requestID=%ld", requestID_);

    if (subRequestID_ && subRequestID_->size() > 35)
        throw ValidateError(BIGLOG, "subRequestID=%s", subRequestID_->c_str());

    if (customerCode_ && customerCode_->size() > 10)
        throw ValidateError(BIGLOG, "customerCode=%s", customerCode_->c_str());
}

std::string Request::content(const Session &session) const
{
    validate();
    return StrUtils::rtrim(payload(session));
}

std::string Request::payload(const Session &) const
{
    return {};
}

void Request::fill(xmlNodePtr node) const
{
    xmlNewChild(node, nullptr, "requestID", std::to_string(requestID_));

    if (subRequestID_)
        xmlNewChild(node, nullptr, "subRequestID", *subRequestID_);

    if (customerCode_)
        xmlNewChild(node, nullptr, "customerCode", *customerCode_);
}

//-----------------------------------------------

CheckNameReq::CheckNameReq()
    : Request(RequestType::CheckName)
{
}

void CheckNameReq::validate() const
{
    Request::validate();

    if (username_.empty())
        throw ValidateError(BIGLOG, "username=%s", username_.c_str());

    if (subUsername_.empty())
        throw ValidateError(BIGLOG, "subUsername=%s", subUsername_.c_str());
}

std::string CheckNameReq::payload(const Session &session) const
{
    XmlDoc doc = xMakeDoc(ENVELOPE);
    auto workNode = getWorkNode(doc, "processLogin", "checkNameRequest");
    if (workNode)
        fill(workNode);
    return xDumpDoc(doc);
}

void CheckNameReq::fill(xmlNodePtr node) const
{
    Request::fill(node);
    xmlNewChild(node, nullptr, "username", username_);
    xmlNewChild(node,  nullptr, "subUsername", subUsername_);
}

//-----------------------------------------------

LoginReq::LoginReq()
    : Request(RequestType::Login)
{
}

void LoginReq::validate() const
{
    Request::validate();

    if (loginInfo_.empty())
        throw ServerFramework::Exception(STDLOG, __FUNCTION__, "loginInfo=" + loginInfo_);
}

std::string LoginReq::payload(const Session &session) const
{
    XmlDoc doc = xMakeDoc(ENVELOPE);
    writeSession(doc, session);
    auto workNode = getWorkNode(doc, "processLogin", "loginRequest");
    if (workNode)
        fill(workNode);
    return xDumpDoc(doc);
}

void LoginReq::fill(xmlNodePtr node) const
{
    Request::fill(node);
    xmlNewChild(node, nullptr, "loginInfo", loginInfo_);
}

//-----------------------------------------------

DocumentReq::DocumentReq()
    : Request(RequestType::Document), section_(DataSection::PassportVisa)
{
}

void DocumentReq::validate() const
{
    Request::validate();

    //
    // mandatory
    if (section_ < DataSection::PassportVisaHealth || section_ > DataSection::All)
        throw ValidateError(BIGLOG, "section=%d", (int)section_);

    if (destinationCode_.size() < 2 || destinationCode_.size() > 3)
        throw ValidateError(BIGLOG, "destinationCode=%s", destinationCode_.c_str());

    if (nationalityCode_.size() != 2)
        throw ValidateError(BIGLOG, "nationalityCode=%s", nationalityCode_.c_str());

    if (documentType_.size() > 100)
        throw ValidateError(BIGLOG, "documentType=%s", documentType_.c_str());

    //
    // mandatory2
    if (arrivalDateGMT_ && arrivalDateGMT_->is_special())
        throw ValidateError(BIGLOG, "arrivalDateGMT is special");

    if (carrierCode_ && carrierCode_->size() > 10)
        throw ValidateError(BIGLOG, "carrierCode=%s", carrierCode_->c_str());

    if (ticket_ && (ticket_->size() < 1 || ticket_->size() > 300))
        throw ValidateError(BIGLOG, "ticket=%s", ticket_->c_str());

    if (passportSeries_ && passportSeries_->empty())
        throw ValidateError(BIGLOG, "passportSeries=%s", passportSeries_->c_str());

    if (transitCountry_.size() > 5)
        throw ValidateError(BIGLOG, "transitCountry.size=%ld", transitCountry_.size());

    for (const auto &tc : transitCountry_)
        tc.validate();

    if (stayDuration_)
        stayDuration_->validate();

    if (visa_ && (visa_ < Visa::NoVisa || visa_ > Visa::ValidVisa))
        throw ValidateError(BIGLOG, "visa=%d", (int)*visa_);

    if (visaData_)
        visaData_->validate();

    if (residencyDocument_ && residencyDocument_->empty())
        throw ValidateError(BIGLOG, "residencyDocument=%s", residencyDocument_->c_str());

    //
    // optional
    if (departDateGMT_ && departDateGMT_->is_special())
        throw ValidateError(BIGLOG, "departDateGMT is special");

    if (departAirportCode_ && (departAirportCode_->size() < 2 || departAirportCode_->size() > 3))
        throw ValidateError(BIGLOG, "departAirportCode=%s", departAirportCode_->c_str());

    if (issueCountryCode_ && issueCountryCode_->size() != 2)
        throw ValidateError(BIGLOG, "issueCountryCode=%s", issueCountryCode_->c_str());

    if (issueDateGMT_ && issueDateGMT_->is_special())
        throw ValidateError(BIGLOG, "issueDateGMT is special");

    if (documentFeature_ && (documentFeature_ < DocumentFeature::Biometric || documentFeature_ > DocumentFeature::MachineReadableDocument))
        throw ValidateError(BIGLOG, "documentFeature=%d", (int)*documentFeature_);

    if (documentLanguage_ && documentLanguage_->size() != 2)
        throw ValidateError(BIGLOG, "documentLanguage=%s", documentLanguage_->c_str());

    if (stayType_ && (stayType_ < StayType::Vacation || stayType_ > StayType::Duty))
        throw ValidateError(BIGLOG, "stayType=%d", (int)*stayType_);

    if (gender_ && (gender_ != Gender::Male || gender_ != Gender::Female))
        throw ValidateError(BIGLOG, "gender=%d", (int)*gender_);

    if (birthCountryCode_ && birthCountryCode_->size() != 2)
        throw ValidateError(BIGLOG, "birthCountryCode=%s", birthCountryCode_->c_str());

    if (birthDateGMT_ && birthDateGMT_->is_special())
        throw ValidateError(BIGLOG, "birthDateGMT is special");

    if (residentCountryCode_ && residentCountryCode_->size() != 2)
        throw ValidateError(BIGLOG, "residentCountryCode=%s", residentCountryCode_->c_str());

    if (secondaryDocumentType_ && secondaryDocumentType_->size() > 100)
        throw ValidateError(BIGLOG, "secondaryDocumentType=%s", secondaryDocumentType_->c_str());

    for (const auto &cv : countriesVisited_) {
        if (cv.size() != 2)
            throw ValidateError(BIGLOG, "countriesVisited=%s", cv.c_str());
    }
}

std::string DocumentReq::payload(const Session &session) const
{
    XmlDoc doc = xMakeDoc(ENVELOPE);
    writeSession(doc, session);
    auto workNode = getWorkNode(doc, "processRequest", "documentRequest");
    if (workNode)
        fill(workNode);
    return xDumpDoc(doc);
}

void DocumentReq::fill(xmlNodePtr node) const
{
    Request::fill(node);

    //
    // mandatory
    xmlNewChild(node, nullptr, "section", toString(section_));

    if (!destinationCode_.empty())
        xmlNewChild(node, nullptr, "destinationCode", destinationCode_);

    if (!nationalityCode_.empty())
        xmlNewChild(node, nullptr, "nationalityCode", nationalityCode_);

    if (!documentType_.empty())
        xmlNewChild(node, nullptr, "documentType", documentType_);

    if (!documentGroup_.empty())
        xmlNewChild(node, nullptr, "documentGroup", documentGroup_);

    //
    // mandatory2
    if (arrivalDateGMT_)
        xmlNewChild(node, nullptr, "arrivalDate", toString(*arrivalDateGMT_));

    if (carrierCode_)
        xmlNewChild(node, nullptr, "carrierCode", *carrierCode_);

    if (ticket_)
        xmlNewChild(node, nullptr, "ticket", *ticket_);

    if (passportSeries_)
        xmlNewChild(node, nullptr, "passportSeries", *passportSeries_);

    for (const auto &tc : transitCountry_)
        tc.fill(node);

    if (stayDuration_)
        stayDuration_->fill(node);

    if (visa_)
        xmlNewChild(node, nullptr, "visa", toString(*visa_));

    if (visaData_)
        visaData_->fill(node);

    if (residencyDocument_)
        xmlNewChild(node, nullptr, "residencyDocument", *residencyDocument_);

    //
    // optional
    if (departDateGMT_)
        xmlNewChild(node, nullptr, "departDate", toString(*departDateGMT_));

    if (departAirportCode_)
        xmlNewChild(node, nullptr, "departAirportCode", *departAirportCode_);

    if (issueCountryCode_)
        xmlNewChild(node, nullptr, "issueCountryCode", *issueCountryCode_);

    if (issueDateGMT_)
        xmlNewChild(node, nullptr, "issueDate", toString(*issueDateGMT_));

    if (documentFeature_)
        xmlNewChild(node, nullptr, "documentFeature", toString(*documentFeature_));

    if (stayType_)
        xmlNewChild(node, nullptr, "stayType", toString(*stayType_));

    if (gender_)
        xmlNewChild(node, nullptr, "gender", toString(*gender_));

    if (birthCountryCode_)
        xmlNewChild(node, nullptr, "birthCountryCode", *birthCountryCode_);

    if (birthDateGMT_)
        xmlNewChild(node, nullptr, "birthDate", toString(*birthDateGMT_));

    if (residentCountryCode_)
        xmlNewChild(node, nullptr, "residentCountryCode", *residentCountryCode_);

    if (secondaryDocumentType_)
        xmlNewChild(node, nullptr, "secondaryDocumentType", *secondaryDocumentType_);

    for (const auto &cv : countriesVisited_)
        xmlNewChild(node, nullptr, "countriesVisited", cv);
}

//-----------------------------------------------

ParamReq::ParamReq()
    : Request(RequestType::Param), section_(DataSection::PassportVisaHealth)
{
}

void ParamReq::validate() const
{
    Request::validate();

    if (countryCode_.size() != 2)
        throw ValidateError(BIGLOG, "countryCode=%s", countryCode_.c_str());

    if (section_ < DataSection::PassportVisaHealth || section_ > DataSection::All)
        throw ValidateError(BIGLOG, "section=%d", (int)section_);

    if (transitCountry_.size() > 5)
        throw ValidateError(BIGLOG, "transitCountry.size=%zu", transitCountry_.size());

    for (const auto &tc : transitCountry_) {
        if (tc.size() < 2 || tc.size() > 3)
            throw ValidateError(BIGLOG, "transitCountry=%s", tc.c_str());
    }

    if (queryDateGMT_ && queryDateGMT_->is_special())
        throw ValidateError(BIGLOG, "queryDateGMT is special");
}

std::string ParamReq::payload(const Session &session) const
{
    XmlDoc doc = xMakeDoc(ENVELOPE);
    writeSession(doc, session);
    auto workNode = getWorkNode(doc, "processRequest", "paramRequest");
    if (workNode)
        fill(workNode);
    return xDumpDoc(doc);
}

void ParamReq::fill(xmlNodePtr node) const
{
    Request::fill(node);

    xmlNewChild(node, nullptr, "countryCode", countryCode_);
    xmlNewChild(node, nullptr, "section", toString(section_));

    for (const auto &tc : transitCountry_)
        xmlNewChild(node, nullptr, "transitCountry", tc);

    if (queryDateGMT_)
        xmlNewChild(node, nullptr, "queryDateGMT", toString(*queryDateGMT_));
}

//-----------------------------------------------

ParamValuesReq::ParamValuesReq()
    : Request(RequestType::ParamValues), parameterName_(ParameterName::DocumentType)
{
}

void ParamValuesReq::validate() const
{
    if (parameterName_ < ParameterName::DocumentType || parameterName_ > ParameterName::DocumentFeature)
        throw ValidateError(BIGLOG, "parameterName=%d", (int)parameterName_);

    if (section_ && section_->size() > 100)
        throw ValidateError(BIGLOG, "section=%s", section_->c_str());

    if (queryDateGMT_ && queryDateGMT_->is_special())
        throw ValidateError(BIGLOG, "queryDateGMT is special");
}

std::string ParamValuesReq::payload(const Session &session) const
{
    XmlDoc doc = xMakeDoc(ENVELOPE);
    writeSession(doc, session);
    auto workNode = getWorkNode(doc, "processRequest", "paramValuesRequest");
    if (workNode)
        fill(workNode);
    return xDumpDoc(doc);
}

void ParamValuesReq::fill(xmlNodePtr node) const
{
    Request::fill(node);

    xmlNewChild(node, nullptr, "parameterName", toString(parameterName_));

    if (section_)
        xmlNewChild(node, nullptr, "section", *section_);

    if (queryDateGMT_)
        xmlNewChild(node, nullptr, "queryDateGMT", toString(*queryDateGMT_));
}

//-----------------------------------------------

CountryReq::CountryReq()
    : Request(RequestType::Country)
{
}

void CountryReq::validate() const
{
    if (countryCode_ && countryCode_->size() != 2)
        throw ValidateError(BIGLOG, "countryCode=%s", countryCode_->c_str());

    if (countryName_ && countryName_->size() > 100)
        throw ValidateError(BIGLOG, "countryName=%s", countryName_->c_str());

    if (cityCode_ && cityCode_->size() != 3)
        throw ValidateError(BIGLOG, "cityCode=%s", cityCode_->c_str());

    if (cityName_ && cityName_->size() > 100)
        throw ValidateError(BIGLOG, "cityName=%s", cityName_->c_str());

    if (groupCode_ && groupCode_->size() > 4)
        throw ValidateError(BIGLOG, "groupCode=%s", groupCode_->c_str());

    if (groupName_ && (groupName_ != GroupName::Organisation || groupName_ != GroupName::Infected))
        throw ValidateError(BIGLOG, "groupName=%d", (int)*groupName_);
}

std::string CountryReq::payload(const Session &session) const
{
    XmlDoc doc = xMakeDoc(ENVELOPE);
    writeSession(doc, session);
    auto workNode = getWorkNode(doc, "processRequest", "countryRequest");
    if (workNode)
        fill(workNode);
    return xDumpDoc(doc);
}

void CountryReq::fill(xmlNodePtr node) const
{
    Request::fill(node);

    if (countryCode_)
        xmlNewChild(node, nullptr, "countryCode", *countryCode_);

    if (countryName_)
        xmlNewChild(node, nullptr, "countryName", *countryName_);

    if (cityCode_)
        xmlNewChild(node, nullptr, "cityCode", *cityCode_);

    if (cityName_)
        xmlNewChild(node, nullptr, "cityName", *cityName_);

    if (groupCode_)
        xmlNewChild(node, nullptr, "groupCode", *groupCode_);

    if (groupName_)
        xmlNewChild(node, nullptr, "groupName", toString(*groupName_));
}

//-----------------------------------------------

VisaReq::VisaReq()
    : Request(RequestType::Visa)
{
}

void VisaReq::validate() const
{
    if (countryCode_ && countryCode_->size() > 2)
        throw ValidateError(BIGLOG, "countryCode=%s", countryCode_->c_str());
}

std::string VisaReq::payload(const Session &session) const
{
    XmlDoc doc = xMakeDoc(ENVELOPE);
    writeSession(doc, session);
    auto workNode = getWorkNode(doc, "processRequest", "visaRequest");
    if (workNode)
        fill(workNode);
    return xDumpDoc(doc);
}

void VisaReq::fill(xmlNodePtr node) const
{
    Request::fill(node);

    if (countryCode_)
        xmlNewChild(node, nullptr, "countryCode", *countryCode_);
}

} // Timatic
