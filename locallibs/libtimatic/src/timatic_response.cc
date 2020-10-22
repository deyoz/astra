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

std::string DocumentParagraphContent::toStr(Optional<xmlNodePtr> resNode, Optional<const std::vector<Node> &> node_list) const
{
    XmlDoc resDoc;
    if(not resNode) {
        resDoc = xMakeDoc("<res/>");
        resNode = resDoc.get()->children;
        node_list = items;
    }
    for(const auto &i: node_list.get()) {
        if(i.nodeType == XML_TEXT_NODE)
            xmlNodeAddContent(resNode.get(), (const xmlChar*)i.nodeContent.c_str());
        else {
            xmlNodePtr curNode = xmlNewChild(resNode.get(), nullptr, i.nodeName.c_str(), nullptr);
            for(const auto &prop: i.props)
                xmlNewProp(curNode, (const xmlChar *)prop.first.c_str(), (const xmlChar *)prop.second.c_str());
            toStr(curNode, i.items);
        }
    }
    std::string result;
    if(resDoc) {
        xmlBufferPtr buf = xmlBufferCreate();
        try {
            xmlNodePtr curNode = resDoc.get()->children->children;
            while(curNode) {
                xmlNodeDump(buf, resDoc.get(), curNode, 0, 0);
                curNode = curNode->next;
            }
            result = (char *)buf->content;
            xmlBufferFree(buf);
        } catch(...) {
            xmlBufferFree(buf);
        }
    }
    return result;
}

void DocumentParagraphContent::fromXML(xmlNodePtr node, Optional<std::vector<Node> &> node_list)
{
    if(not node_list) {
        items.clear();
        node_list = items;
    }
    for (xmlNodePtr child2 = node->children; child2; child2 = child2->next) {
        node_list->emplace_back();
        auto &node = node_list->back();
        node.nodeName = reinterpret_cast<const char *>(child2->name);
        if(child2->content)
            node.nodeContent = (const char *)child2->content;
        node.nodeType = child2->type;
        xmlAttrPtr pr = child2->properties;
        while(pr) {
            node.props.push_back(std::make_pair((const char *)pr->name, (const char *)pr->children->content));
            pr = pr->next;
        }
        fromXML(child2, node.items);
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
            paragraphText_.fromXML(child);



// In this case vector will be empty
// <paragraphText>
//   <p>Visa required.</p>
// </paragraphText>

// Those examples will be handled incorrectly too
//
// <paragraphText>Visitors not holding return/onward tickets could be refused entry. <notesReference paragraphId="43399"/></paragraphText>
//
// <paragraphText>Free import by persons of 18 years and older:<br/>1. 400 cigarettes or 200 cigarillos or 100 cigars or 500 grams of tobacco products if only one type of tobacco products is imported (otherwise only half of the quantities allowed);<br/>2. only for persons of 21 years and older: alcoholic beverages: 3 liters;<br/>3. a reasonable quantity of perfume for personal use;<br/>4. goods up to an amount of EUR 10,000.- for personal use only;<br/>5. caviar (factory packed) max. 250 grams per person. </paragraphText>



            /*
            for (xmlNodePtr child2 = child->children; child2; child2 = child2->next) {
                if (xCmpNames(child2, "text") && child2->content) {
                    std::string value(reinterpret_cast<const char *>(child2->content));
                    if (!value.empty())
                        paragraphText_.emplace_back(StrUtils::StringTrim(value));
                }
            }
            */
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
            sufficientDocumentation_ = getSufficientDocumentation(xGetStr(child));
        else if (xCmpNames(child, "message"))
            message_ = MessageType(child);
        else if (xCmpNames(child, "sectionInformation"))
            sectionInformation_.emplace_back(child);
    }
}

//-----------------------------------------------

DocumentCheckResp::DocumentCheckResp(const xmlNodePtr node)
    : sufficientDocumentation_(SufficientDocumentation::No)
{
    for (xmlNodePtr child = node->children; child; child = child->next) {
        if (xCmpNames(child, "sufficientDocumentation"))
            sufficientDocumentation_ = getSufficientDocumentation(xGetStr(child));
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
            for (xmlNodePtr child2 = child->children; child2; child2 = child2->next) {
                if (xCmpNames(child2, "parameter"))
                    parameterList_.emplace_back(child2);
            }
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
            visaResults_ = VisaResults(child);
    }
}


} // Timatic
