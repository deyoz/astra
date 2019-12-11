#pragma once

#include "timatic_base.h"

#include <vector>

namespace Timatic {

enum class ResponseType {
    Base,
    Error,
    // processLoginResponse
    CheckName,
    Login,
    // processRequestResponse
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
    ParagraphReport,
    TimBookExtract,
    TimDownload,
};

class Response {
public:
    static ResponseType ctype() { return ResponseType::Base; }
    static const char  *cname() { return "response"; }

    virtual ~Response() = default;
    ResponseType type() const { return type_; }
    int64_t requestID() const { return requestID_; }
    const Optional<std::string> &subRequestID() const { return subRequestID_; }
    const Optional<std::string> &customerCode() const { return customerCode_; }
    const std::vector<ErrorType> &errors() const { return errors_; }

protected:
    Response(ResponseType type);
    Response(ResponseType type, const xmlNodePtr node);
    ResponseType type_;
    int64_t requestID_;
    Optional<std::string> subRequestID_;
    Optional<std::string> customerCode_;
    std::vector<ErrorType> errors_;
};

//-----------------------------------------------

class ErrorResp final : public Response {
public:
    static ResponseType ctype() { return ResponseType::Error; }
    static const char  *cname() { return "errorResponse"; }

    ErrorResp(const xmlNodePtr node);
};

//-----------------------------------------------

class CheckNameResp final : public Response {
public:
    static ResponseType ctype() { return ResponseType::CheckName; }
    static const char  *cname() { return "checkNameResponse"; }

    CheckNameResp(const xmlNodePtr node);
    const std::string &randomNumber() const { return randomNumber_; }
    const std::string &sessionID() const { return sessionID_; }

private:
    std::string randomNumber_;
    std::string sessionID_;
};

//-----------------------------------------------

class LoginResp final : public Response {
public:
    static ResponseType ctype() { return ResponseType::Login; }
    static const char  *cname() { return "loginResponse"; }

    LoginResp(const xmlNodePtr node);
    bool successfulLogin() const { return successfulLogin_; }

private:
    bool successfulLogin_;
};

//-----------------------------------------------

class DocumentParagraphSection {
public:
    DocumentParagraphSection(const xmlNodePtr node);
    int paragraphID() const { return paragraphID_; }
    ParagraphType paragraphType() const { return paragraphType_; }
    const std::vector<std::string> &paragraphText() const { return paragraphText_; }
    const std::vector<DocumentParagraphSection> documentChildParagraph() const { return documentChildParagraph_; }

private:
    int paragraphID_;
    ParagraphType paragraphType_;
    std::vector<std::string> paragraphText_;
    std::vector<DocumentParagraphSection> documentChildParagraph_;
};

//-----------------------------------------------

class DocumentSubSection {
public:
    DocumentSubSection(const xmlNodePtr node);
    const std::string subsectionName() const { return subsectionName_; }
    const std::vector<DocumentParagraphSection> &documentParagraph() const { return documentParagraph_; }

private:
    std::string subsectionName_;
    std::vector<DocumentParagraphSection> documentParagraph_;
};

//-----------------------------------------------

class DocumentSection {
public:
    DocumentSection(const xmlNodePtr node);
    const std::string &sectionName() const { return sectionName_; }
    const std::vector<DocumentParagraphSection> &documentParagraph() const { return documentParagraph_; }
    const std::vector<DocumentSubSection> &subsectionInformation() const { return subsectionInformation_; }

private:
    std::string sectionName_;
    std::vector<DocumentParagraphSection> documentParagraph_;
    std::vector<DocumentSubSection> subsectionInformation_;
};

//-----------------------------------------------

class DocumentCountryInfo {
public:
    DocumentCountryInfo(const xmlNodePtr node);
    const std::string &countryCode() const { return countryCode_; }
    const std::string &countryName() const { return countryName_; }
    const Optional<std::string> &commonTransit() const { return commonTransit_; }
    const Optional<SufficientDocumentation> &sufficientDocumentation() const { return sufficientDocumentation_; }
    const Optional<MessageType> &message() const { return message_; }
    const std::vector<DocumentSection> &sectionInformation() const { return sectionInformation_; }

private:
    std::string countryCode_;
    std::string countryName_;
    Optional<std::string> commonTransit_;
    Optional<SufficientDocumentation> sufficientDocumentation_;
    Optional<MessageType> message_;
    std::vector<DocumentSection> sectionInformation_;
};

//-----------------------------------------------

class DocumentCheckResp {
public:
    DocumentCheckResp(const xmlNodePtr node);
    SufficientDocumentation sufficientDocumentation() const { return sufficientDocumentation_; }
    const std::vector<DocumentCountryInfo> &documentCountryInfo() const { return documentCountryInfo_; }

private:
    SufficientDocumentation sufficientDocumentation_;
    std::vector<DocumentCountryInfo> documentCountryInfo_;
};

//-----------------------------------------------

class DocumentResp final : public Response {
public:
    static ResponseType ctype() { return ResponseType::Document; }
    static const char  *cname() { return "documentResponse"; }

    DocumentResp(const xmlNodePtr node);
    const Optional<DocumentCheckResp> &documentCheckResp() const { return documentCheckResp_; }

private:
    Optional<DocumentCheckResp> documentCheckResp_;
};

//-----------------------------------------------

class ParamResults {
public:
    ParamResults(const xmlNodePtr node);
    const std::string &countryCode() const { return countryCode_; }
    DataSection section() const { return section_; }
    const std::vector<ParameterType> &parameterList() const { return parameterList_; }

private:
    std::string countryCode_;
    DataSection section_;
    std::vector<ParameterType> parameterList_;
};

//-----------------------------------------------

class ParamResp final : public Response {
public:
    static ResponseType ctype() { return ResponseType::Param; }
    static const char  *cname() { return "paramResponse"; }

    ParamResp(const xmlNodePtr node);
    const std::vector<ParamResults> &paramResults() const { return paramResults_; }

private:
    std::vector<ParamResults> paramResults_;
};

//-----------------------------------------------

class ParamValuesResp final : public Response {
public:
    static ResponseType ctype() { return ResponseType::ParamValues; }
    static const char  *cname() { return "paramValuesResponse"; }

    ParamValuesResp(const xmlNodePtr node);
    const std::vector<ParamValue> &paramValues() const { return paramValues_; }

private:
    std::vector<ParamValue> paramValues_;
};

//-----------------------------------------------

class CountryDetail {
public:
    CountryDetail(const xmlNodePtr node);
    const std::string &countryName() const { return countryName_; }
    const std::string &countryCode() const { return countryCode_; }
    const std::vector<AlternateDetail> &alternateCountries() const { return alternateCountries_; }
    const std::vector<CityDetail> &cities() const { return cities_; }

private:
    std::string countryName_;
    std::string countryCode_;
    std::vector<AlternateDetail> alternateCountries_;
    std::vector<CityDetail> cities_;
};

//-----------------------------------------------

class GroupDetail {
public:
    GroupDetail(const xmlNodePtr node);
    const std::string &groupName() const { return groupName_; }
    const std::string &groupCode() const { return groupCode_; }
    const std::string &groupDescription() const { return groupDescription_; }
    const std::vector<CountryDetail> &countries() const { return countries_; }

private:
    std::string groupName_;
    std::string groupCode_;
    std::string groupDescription_;
    std::vector<CountryDetail> countries_;
};

//-----------------------------------------------

class CountryResp final : public Response {
public:
    static ResponseType ctype() { return ResponseType::Country; }
    static const char  *cname() { return "countryResponse"; }

    CountryResp(const xmlNodePtr node);
    const std::vector<GroupDetail> &groups() const { return groups_; }
    const std::vector<CountryDetail> &countries() const { return countries_; }

private:
    std::vector<GroupDetail> groups_;
    std::vector<CountryDetail> countries_;
};

//-----------------------------------------------

class VisaResults {
public:
    VisaResults(const xmlNodePtr node);
    const std::string &countryName() const { return countryName_; }
    const std::string &countryCode() const { return countryCode_; }
    const std::vector<VisaType> &countryVisaList() const { return countryVisaList_; }
    const std::vector<VisaType> &groupVisaList() const { return groupVisaList_; }

private:
    std::string countryName_;
    std::string countryCode_;
    std::vector<VisaType> countryVisaList_;
    std::vector<VisaType> groupVisaList_;
};

//-----------------------------------------------

class VisaResp final : public Response {
public:
    static ResponseType ctype() { return ResponseType::Visa; }
    static const char  *cname() { return "visaResponse"; }

    VisaResp(const xmlNodePtr node);
    const Optional<VisaResults> &visaResults() const { return visaResults_; }

private:
    Optional<VisaResults> visaResults_;
};

} // Timatic
