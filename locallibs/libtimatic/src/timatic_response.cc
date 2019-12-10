#include "timatic_response.h"
#include "timatic_xml.h"

#include <serverlib/str_utils.h>
#include <serverlib/xmllibcpp.h>

namespace Timatic {

Response::Response(ResponseType type)
    : type_(type), requestID_(-1)
{
}

Response::Response(ResponseType type, const xmlNodePtr node)
    : Response(type)
{
    for (xmlNodePtr child = node->children; child; child = child->next) {
        if (xCmpNames(child, "requestID"))
            requestID_ = xGetInt(child);
        else if (xCmpNames(child, "subRequestID"))
            subRequestID_ = xGetStr(child);
        else if (xCmpNames(child, "customerCode"))
            customerCode_ = xGetStr(child);
        else if (xCmpNames(child, "error"))
            errors_.emplace_back(child);
    }
}

//-----------------------------------------------

ErrorResp::ErrorResp(const xmlNodePtr node)
    : Response(ctype(), node)
{
}

//-----------------------------------------------

CheckNameResp::CheckNameResp(const xmlNodePtr node)
    : Response(ctype(), node)
{
    for (xmlNodePtr child = node->children; child; child = child->next) {
        if (xCmpNames(child, "randomNumber"))
            randomNumber_ = xGetStr(child);
        else if (xCmpNames(child, "sessionID"))
            sessionID_ = xGetStr(child);
    }
}

//-----------------------------------------------

LoginResp::LoginResp(const xmlNodePtr node)
    : Response(ctype(), node), successfulLogin_(false)
{
    for (xmlNodePtr child = node->children; child; child = child->next) {
        if (xCmpNames(child, "successfulLogin"))
            successfulLogin_ = xGetBool(child);
    }
}

//-----------------------------------------------

DocumentParagraphSection::DocumentParagraphSection(const xmlNodePtr node)
    : paragraphID_(-1)
{
    paragraphID_ = xGetIntProp(node, "paragraphId", -1);

    for (xmlNodePtr child = node->children; child; child = child->next) {
        if (xCmpNames(child, "paragraphType")) {
            paragraphType_ = getParagraphType(xGetStr(child));
        } else if (xCmpNames(child, "paragraphText")) {
            for (xmlNodePtr child2 = child->children; child2; child2 = child2->next) {
                if (xCmpNames(child2, "text") && child2->content) {
                    std::string value(reinterpret_cast<const char *>(child2->content));
                    if (!value.empty())
                        paragraphText_.emplace_back(StrUtils::StringTrim(value));
                }
            }
        } else if (xCmpNames(child, "documentChildParagraph")) {
            documentChildParagraph_.emplace_back(child);
        }
    }
}

//-----------------------------------------------

DocumentSubSection::DocumentSubSection(const xmlNodePtr node)
{
    for (xmlNodePtr child = node->children; child; child = child->next) {
        if (xCmpNames(child, "subsectionName"))
            subsectionName_ = xGetStr(child);
        else if (xCmpNames(child, "documentParagraph"))
            documentParagraph_.emplace_back(child);
    }
}

//-----------------------------------------------

DocumentSection::DocumentSection(const xmlNodePtr node)
{
    for (xmlNodePtr child = node->children; child; child = child->next) {
        if (xCmpNames(child, "sectionName"))
            sectionName_ = xGetStr(child);
        else if (xCmpNames(child, "documentParagraph"))
            documentParagraph_.emplace_back(child);
        else if (xCmpNames(child, "subsectionInformation"))
            subsectionInformation_.emplace_back(child);
    }
}

//-----------------------------------------------

DocumentCountryInfo::DocumentCountryInfo(const xmlNodePtr node)
{
    for (xmlNodePtr child = node->children; child; child = child->next) {
        if (xCmpNames(child, "countryCode"))
            countryCode_ = xGetStr(child);
        else if (xCmpNames(child, "countryName"))
            countryName_ = xGetStr(child);
        else if (xCmpNames(child, "commonTransit"))
            commonTransit_ = xGetStr(child);
        else if (xCmpNames(child, "sufficientDocumentation"))
            sufficientDocumentation_ = getSufficient(xGetStr(child));
        else if (xCmpNames(child, "message"))
            message_ = MessageType(child);
        else if (xCmpNames(child, "sectionInformation"))
            sectionInformation_.emplace_back(child);
    }
}

//-----------------------------------------------

DocumentCheckResp::DocumentCheckResp(const xmlNodePtr node)
    : sufficientDocumentation_(Sufficient::No)
{
    for (xmlNodePtr child = node->children; child; child = child->next) {
        if (xCmpNames(child, "sufficientDocumentation"))
            sufficientDocumentation_ = getSufficient(xGetStr(child));
         else if (xCmpNames(child, "documentCountryInformation"))
            documentCountryInfo_.emplace_back(child);
    }
}

//-----------------------------------------------

DocumentResp::DocumentResp(const xmlNodePtr node)
    : Response(ctype(), node)
{
    for (xmlNodePtr child = node->children; child; child = child->next) {
        if (xCmpNames(child, "documentCheckResponse"))
            documentCheckResp_ = DocumentCheckResp(child);
    }
}

//-----------------------------------------------

ParamResults::ParamResults(const xmlNodePtr node)
{
    for (xmlNodePtr child = node->children; child; child = child->next) {
        if (xCmpNames(child, "countryCode")) {
            countryCode_ = xGetStr(child);
        } else if (xCmpNames(child, "section")) {
            section_ = getDataSection(xGetStr(child));
        } else if (xCmpNames(child, "parameterList")) {
            for (xmlNodePtr child2 = child->children; child2; child2 = child2->next)
                parameterList_.emplace_back(child2);
        }
    }
}

//-----------------------------------------------

ParamResp::ParamResp(const xmlNodePtr node)
    : Response(ctype(), node)
{
    for (xmlNodePtr child = node->children; child; child = child->next) {
        if (xCmpNames(child, "paramResults"))
            paramResults_.emplace_back(child);
    }
}

//-----------------------------------------------

ParamValuesResp::ParamValuesResp(const xmlNodePtr node)
    : Response(ctype(), node)
{
    for (xmlNodePtr child = node->children; child; child = child->next) {
        if (xCmpNames(child, "values")) {
            for (xmlNodePtr child2 = child->children; child2; child2 = child2->next){
                if (xCmpNames(child2, "value"))
                    paramValues_.emplace_back(child2);
            }
        }
    }
}

//-----------------------------------------------

CountryDetail::CountryDetail(const xmlNodePtr node)
{
    for (xmlNodePtr child = node->children; child; child = child->next) {
        if (xCmpNames(child, "countryName")) {
            countryName_ = xGetStr(child);
        } else if (xCmpNames(child, "countryCode")) {
            countryCode_ = xGetStr(child);
        } else if (xCmpNames(child, "alternateCountries")) {
            for (xmlNodePtr child2 = child->children; child2; child2 = child2->next){
                if (xCmpNames(child2, "alternateCountry"))
                    alternateCountries_.emplace_back(child2);
            }
        } else if (xCmpNames(child, "cities")) {
            for (xmlNodePtr child2 = child->children; child2; child2 = child2->next){
                if (xCmpNames(child2, "city"))
                    cities_.emplace_back(child2);
            }
        }
    }
}

//-----------------------------------------------

GroupDetail::GroupDetail(const xmlNodePtr node)
{
    for (xmlNodePtr child = node->children; child; child = child->next) {
        if (xCmpNames(child, "groupCode")) {
            groupCode_ = xGetStr(child);
        } else if (xCmpNames(child, "groupName")) {
            groupName_ = xGetStr(child);
        } else if (xCmpNames(child, "groupDescription")) {
            groupDescription_ = xGetStr(child);
        } else if (xCmpNames(child, "countries")) {
            for (xmlNodePtr child2 = child->children; child2; child2 = child2->next){
                if (xCmpNames(child2, "country"))
                    countries_.emplace_back(child2);
            }
        }
    }
}

//-----------------------------------------------

CountryResp::CountryResp(const xmlNodePtr node)
    : Response(ctype(), node)
{
    for (xmlNodePtr child = node->children; child; child = child->next) {
        if (xCmpNames(child, "groups")) {
            for (xmlNodePtr child2 = child->children; child2; child2 = child2->next){
                if (xCmpNames(child2, "group"))
                    groups_.emplace_back(child2);
            }
        } else if (xCmpNames(child, "countries")) {
            for (xmlNodePtr child2 = child->children; child2; child2 = child2->next){
                if (xCmpNames(child2, "country"))
                    countries_.emplace_back(child2);
            }
        }
    }
}

//-----------------------------------------------

VisaResults::VisaResults(const xmlNodePtr node)
{
    for (xmlNodePtr child = node->children; child; child = child->next) {
        if (xCmpNames(child, "countryName")) {
            countryName_ = xGetStr(child);
        } else if (xCmpNames(child, "countryCode")) {
            countryCode_ = xGetStr(child);
        } else if (xCmpNames(child, "countryVisaList")) {
            for (xmlNodePtr child2 = child->children; child2; child2 = child2->next){
                if (xCmpNames(child2, "visaType"))
                    countryVisaList_.emplace_back(child2);
            }
        } else if (xCmpNames(child, "groupVisaList")) {
            for (xmlNodePtr child2 = child->children; child2; child2 = child2->next){
                if (xCmpNames(child2, "visaType"))
                    groupVisaList_.emplace_back(child2);
            }
        }
    }
}

//-----------------------------------------------

VisaResp::VisaResp(const xmlNodePtr node)
    : Response(ctype(), node)
{
    for (xmlNodePtr child = node->children; child; child = child->next) {
        if (xCmpNames(child, "visaResults"))
            visaResults_.emplace_back(child);
    }
}


} // Timatic
