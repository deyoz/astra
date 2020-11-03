#include "json2xml.h"

#include "message.h"
#include "json_spirit.h"
#include "xml_doc_holder.h"
#include "xmllibcpp.h"
#include "xml_tools.h"
#include "lngv_user.h"

#define NICKNAME "NONSTOP"
#include "slogger.h"

static void convert(xmlNodePtr xmlNode, const std::string& name, const json_spirit::mValue& v);

static void convert(xmlNodePtr xmlNode, int v)
{
    xmlNodeSetContent(xmlNode, HelpCpp::string_cast(v));
}

static void convert(xmlNodePtr xmlNode, double v)
{
    xmlNodeSetContent(xmlNode, HelpCpp::string_cast(v));
}

static void convert(xmlNodePtr xmlNode, bool v)
{
    xmlNodeSetContent(xmlNode, v ? "true" : "false");
}

static void convert(xmlNodePtr xmlNode, const std::string& v)
{
    xmlNodeSetContent(xmlNode, v);
}

static void convert(xmlNodePtr xmlNode, const json_spirit::mObject& o)
{
    for (const auto& kv : o) {
        convert(xmlNode, kv.first, kv.second);
    }
}

static void convert(xmlNodePtr xmlNode, const json_spirit::mArray& a)
{
    for (size_t i = 0, sz = a.size(); i < sz; ++i) {
        xmlNodePtr n = newChild(xmlNode, "item");
        xmlSetPropInt(n, "n", i);
        convert(n, "", a[i]);
    }
}

static void convert(xmlNodePtr xmlNode, const std::string& name, const json_spirit::mValue& v)
{
    xmlNodePtr n = name.empty() ? xmlNode : newChild(xmlNode, name.c_str());
    switch (v.type()) {
    case json_spirit::null_type: {
        break;
    }
    case json_spirit::obj_type: {
        convert(n, v.get_obj());
        break;
    }
    case json_spirit::array_type: {
        convert(n, v.get_array());
        break;
    }
    case json_spirit::str_type: {
        convert(n, v.get_str());
        break;
    }
    case json_spirit::bool_type: {
        convert(n, v.get_bool());
        break;
    }
    case json_spirit::int_type: {
        convert(n, v.get_int());
        break;
    }
    case json_spirit::real_type: {
        convert(n, v.get_real());
        break;
    }
    }
}

namespace HelpCpp
{

Message json2xml(xmlNodePtr xmlNode, const json_spirit::mValue& v)
{
    LogTrace(TRACE5) << v.type();
    convert(xmlNode, "", v);
    return Message();
}

Message json2xml(std::string& s, const std::string& rootName, const json_spirit::mValue& v)
{
    xmlDocPtr doc = xmlNewDoc(BAD_CAST const_cast<char*>("1.0"));
    xmlDocSetRootElement(doc, xmlNewNode(NULL, BAD_CAST const_cast<char*>(rootName.c_str())));
    XmlDocHolder xmlDoc(doc);
    if (Message m = HelpCpp::json2xml(xmlDoc.getRoot(), v)) {
        return m;
    }
    const char* xmlBuff;
    int buffersize = 0;
    xmlDocDumpMemory(doc, (xmlChar**)&xmlBuff, &buffersize);
    s = xmlBuff;
    xmlFree((xmlChar*)const_cast<char*>(xmlBuff));
    return Message();
}

} // HelpCpp

#ifdef XP_TESTING
#include "checkunit.h"

void init_json2xml_tests()
{
}

namespace {

struct Json2XmlChecks
{
    const char* jsonStr;
    const char* xmlStr;
} checks[] = {
    {"{}", "<object/>"},
    {R"({"str":"value"})", "<object><str>value</str></object>"},
    {R"({"int":123})", "<object><int>123</int></object>"},
    {R"({"real":123.456})", "<object><real>123.456</real></object>"},
    {R"({"bool":false})", "<object><bool>false</bool></object>"},
    {R"({"obj":{"key":"value"}})", "<object><obj><key>value</key></obj></object>"},
    {R"({"emptyObj":{}})", "<object><emptyObj/></object>"},
    {R"({"arr":[1,"str"]})", R"(<object><arr><item n="0">1</item><item n="1">str</item></arr></object>)"},
    {R"({"emptyArr":[]})", "<object><emptyArr/></object>"},
};

START_TEST(check_json2xml)
{
    for (const auto& c : checks) {
        json_spirit::mValue v;
        if (!json_spirit::read(c.jsonStr, v)) {
            fail_if(1, "failed to parse JSON: [%s]", c.jsonStr);
        }
        xmlDocPtr doc = xmlNewDoc(BAD_CAST const_cast<char*>("1.0"));
        xmlDocSetRootElement(doc, xmlNewNode(NULL, BAD_CAST const_cast<char*>("object")));
        XmlDocHolder xmlDoc(doc);
        if (Message m = HelpCpp::json2xml(xmlDoc.getRoot(), v)) {
            fail_if(1, "json2xml failed: %s", m.toString(UserLanguage::en_US()).c_str());
        }
        const char* xmlBuff;
        int buffersize = 0;
        xmlDocDumpMemory(doc, (xmlChar**)&xmlBuff, &buffersize);
        ProgTrace(TRACE5, "jsonStr [%s]", c.jsonStr);
        ProgTrace(TRACE5, "xmlBuff [%s]", xmlBuff);
        const std::string reply(xmlBuff);
        const std::string expectedReply(std::string(R"(<?xml version="1.0"?>)") + '\n' + c.xmlStr + '\n');
        xmlFree((xmlChar*)const_cast<char*>(xmlBuff));
        CHECK_EQUAL_STRINGS(reply, expectedReply);
    }
} END_TEST

#define SUITENAME "HelpCpp"
TCASEREGISTER(0,0)
{
    ADD_TEST(check_json2xml);
}
TCASEFINISH

} // namespace
#endif //XP_TESTING
