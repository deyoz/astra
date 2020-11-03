#include "timatic_xml.h"

#include <serverlib/exception.h>
#define NICKNAME "TIMATIC"
#include <serverlib/slogger.h>
#include <serverlib/xml_stuff.h>
#include <serverlib/xml_tools.h>

namespace Timatic {

const xmlChar *xCharCast(const char *str)
{
    return reinterpret_cast<const xmlChar *>(str);
}

const xmlChar *xCharCast(const std::string &str)
{
    return xCharCast(str.c_str());
}

//-----------------------------------------------

bool xCmpNames(const xmlNodePtr node, const char *name)
{
    return xmlStrcmp(node->name, xCharCast(name)) == 0;
}

bool xCmpNames(const xmlNodePtr node, const std::string &name)
{
    return xCmpNames(node, name.c_str());
}

//-----------------------------------------------

std::string xGetStr(const xmlNodePtr node)
{
    return getStrFromXml(node);
}

int xGetInt(const xmlNodePtr node)
{
    return getIntFromXml(node);
}

bool xGetBool(const xmlNodePtr node)
{
    return xmlbool(node, false);
}

//-----------------------------------------------

std::string xGetStrProp(const xmlNodePtr node, const char *prop)
{
    return getStrPropFromXml(node, prop);
}

int xGetIntProp(const xmlNodePtr node, const char *prop, int errValue)
{
    return getIntPropFromXml(node, prop, errValue);
}

//-----------------------------------------------

xmlNodePtr xFindNode(const xmlNodePtr node, const char *name)
{
    return findNode(node, name);
}

xmlNodePtr xFindNodeR(const xmlNodePtr node, const char *name)
{
    return findNodeR(node, name);
}

//-----------------------------------------------

XmlDoc xMakeDoc(const std::string &str)
{
    xmlDocPtr doc = xmlParseMemory(str.data(), str.size());
    if (!doc) {
        LogTrace(TRACE1) << __FUNCTION__ << ": str=" << str;
        throw ServerFramework::Exception(STDLOG, __FUNCTION__, "doc is null");
    }
    return XmlDoc::impropriate(doc);
}

std::string xDumpDoc(const XmlDoc &doc, bool indent)
{
    return xml_dump(doc, indent);
}


} // Timatic
