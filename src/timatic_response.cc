#include "timatic_response.h"
#include "xml_unit.h"
#include "stl_utils.h"

#define NICKNAME "DENIS"
#include "serverlib/slogger.h"

using namespace Timatic;
using namespace std;
using namespace EXCEPTIONS;

void ParagraphTextToXML(xmlNodePtr node, const vector<DocumentParagraphContent::Node> &items)
{
    for(const auto &i: items) {
        if(i.nodeName == "text") {
//            NodeAddContent(node, i.nodeContent.c_str()); // эта ф-я портит кавычки в тексте
            xmlNodeAddContent(node, (const xmlChar *) i.nodeContent.c_str());
        } else if(i.props.empty()) {
            xmlNodePtr curNode = NewTextChild(node, i.nodeName.c_str(), nullptr);
            ParagraphTextToXML(curNode, i.items);
        }
    }
}

void ParagraphToXML(xmlNodePtr tableNode, const DocumentParagraphSection &dps, int padding)
{
    xmlNodePtr trNode = NewTextChild(tableNode, "tr");
    xmlNodePtr tdNode = NewTextChild(trNode, "td");
    ParagraphTextToXML(tdNode, dps.paragraphText().items);
    SetProp(tdNode, "style", "padding-left: " + IntToString(padding));
    for(const auto &i: dps.documentChildParagraph())
        ParagraphToXML(tableNode, i, 300);
}

void subsectionToXML(xmlNodePtr tableNode, const DocumentSubSection &dss)
{
    xmlNodePtr trNode = NewTextChild(tableNode, "tr");
    xmlNodePtr tdNode = NewTextChild(trNode, "td", dss.subsectionName());
    SetProp(tdNode, "style", "font-weight: bold; padding-left: 200");
    for(const auto &i: dss.documentParagraph())
        ParagraphToXML(tableNode, i, 250);
}

void SectionToXML(xmlNodePtr tableNode, const string &countryName, const DocumentSection &ds)
{
    xmlNodePtr trNode = NewTextChild(tableNode, "tr");
    xmlNodePtr tdNode = NewTextChild(trNode, "td", countryName + " - Destination " + ds.sectionName());
    SetProp(tdNode, "style", "font-size: large; font-weight: bold; padding-left: 100");
    for(const auto &i: ds.documentParagraph())
        ParagraphToXML(tableNode, i, 250);
    for(const auto &i: ds.subsectionInformation())
        subsectionToXML(tableNode, i);

}

void toXML(xmlNodePtr bodyNode, const DocumentResp &resp)
{
    const DocumentCheckResp &dcr = *resp.documentCheckResp();
    const auto &dci = dcr.documentCountryInfo();
    NewTextChild(bodyNode, "p");
    xmlNodePtr tableNode = NewTextChild(bodyNode, "table");
    SetProp(tableNode, "class", "default_color");
    SetProp(tableNode, "cellspacing", 0);
    SetProp(tableNode, "border", 0);
    SetProp(tableNode, "cellpadding", 2);
    SetProp(tableNode, "width", 700);
    if(dci[0].message()) {
        xmlNodePtr trNode = NewTextChild(tableNode, "tr");
        NewTextChild(trNode, "td", dci[0].message()->text);
    }
    if(dci[0].sectionInformation().empty()) {
        xmlNodePtr trNode = NewTextChild(tableNode, "tr");
        NewTextChild(trNode, "td", "Section Information is empty");
    } else
        for(const auto &ds: dci[0].sectionInformation())
            SectionToXML(tableNode, dci[0].countryName(), ds);
}

void toXML(xmlNodePtr bodyNode, const ParamValuesResp &resp)
{
    NewTextChild(bodyNode, "p");
    NewTextChild(bodyNode, "p");

    xmlNodePtr tableNode = NewTextChild(bodyNode, "table");
    SetProp(tableNode, "class", "default_color");
    SetProp(tableNode, "cellspacing", 2);
    SetProp(tableNode, "border", 1);
    SetProp(tableNode, "cellpadding", 5);
    SetProp(tableNode, "width", 600);
    xmlNodePtr trNode = NewTextChild(tableNode, "tr");
    NewTextChild(trNode, "th", "name");
    NewTextChild(trNode, "th", "displayName");
    NewTextChild(trNode, "th", "code");

    for(const auto &pr: resp.paramValues()) {
        trNode = NewTextChild(tableNode, "tr");
        NewTextChild(trNode, "td", pr.name);
        NewTextChild(trNode, "td", pr.displayName);
        NewTextChild(trNode, "td", pr.code ? pr.code.get() : " "); // nbsp рабочий вариант \xa0
    }
}

void toXML(xmlNodePtr bodyNode, const ParamResp &resp)
{
    NewTextChild(bodyNode, "p");
    NewTextChild(bodyNode, "p");

    for(const auto &pr: resp.paramResults()) {
        xmlNodePtr tableNode = NewTextChild(bodyNode, "table");
        SetProp(tableNode, "class", "default_color");
        SetProp(tableNode, "cellspacing", 2);
        SetProp(tableNode, "border", 1);
        SetProp(tableNode, "cellpadding", 5);
        SetProp(tableNode, "width", 600);
        xmlNodePtr trNode = NewTextChild(tableNode, "tr");
        NewTextChild(trNode, "th", "Parameter Name");
        NewTextChild(trNode, "th", "Mandatory");

        for(const auto &pl: pr.parameterList()) {
            trNode = NewTextChild(tableNode, "tr");
            SetProp(trNode, "style", (string)"color:" + (pl.mandatory ? "red" : "green"));
            NewTextChild(trNode, "td", pl.parameterName);
            ostringstream s;
            s << boolalpha << pl.mandatory;
            NewTextChild(trNode, "td", s.str());
        }
    }
}

void toXML(xmlNodePtr bodyNode, const std::vector<ErrorType> &errors)
{
    NewTextChild(bodyNode, "p");
    xmlNodePtr divNode = NewTextChild(bodyNode, "div");
    SetProp(divNode, "class", "red_color");
    xmlNodePtr pNode = NewTextChild(divNode, "p", "ERROR");
    SetProp(pNode, "style", "font-size: large; font-weight: bold; color: red");

    xmlNodePtr tableNode = NewTextChild(divNode, "table");
    SetProp(tableNode, "cellspacing", 2);
    SetProp(tableNode, "border", 1);
    SetProp(tableNode, "cellpadding", 5);
    SetProp(tableNode, "width", 600);
    SetProp(tableNode, "style", "color: red");
    xmlNodePtr trNode = NewTextChild(tableNode, "tr");
    NewTextChild(trNode, "th", "Code");
    NewTextChild(trNode, "th", "Message");
    for(const auto &err: errors) {
        trNode = NewTextChild(tableNode, "tr");
        NewTextChild(trNode, "td", (err.code ? err.code.get() : " ")); // здесь не пробел, а nbsp \xa0
        NewTextChild(trNode, "td", err.message);
    }
}

template <typename T>
bool responseIs(const HttpData &httpData)
{
    return dynamic_cast<const T*>(httpData.response().get()) != nullptr;
};

void TimaticResponseToXML(xmlNodePtr bodyNode, const Timatic::HttpData &httpData)
{
    if(not httpData.response())
        throw Exception("%s: response is null", __func__);
    if(not httpData.response()->errors().empty()) {
        toXML(bodyNode, httpData.response()->errors());
    } else {
        if(responseIs<DocumentResp>(httpData))
            toXML(bodyNode, httpData.get<DocumentResp>());
        else if(responseIs<ParamResp>(httpData))
            toXML(bodyNode, httpData.get<ParamResp>());
        else if(responseIs<ParamValuesResp>(httpData))
            toXML(bodyNode, httpData.get<ParamValuesResp>());
        else
            throw Exception("%s: %s", __func__, "unknown response");
    }
}
