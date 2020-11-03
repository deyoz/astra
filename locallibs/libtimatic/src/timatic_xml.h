#pragma once

#include <serverlib/xmllibcpp.h>
#include <serverlib/xml_cpp.h>

namespace Timatic {

const xmlChar *xCharCast(const char *str);
const xmlChar *xCharCast(const std::string &str);

bool xCmpNames(const xmlNodePtr node, const char *name);
bool xCmpNames(const xmlNodePtr node, const std::string &name);

std::string xGetStr(const xmlNodePtr node);
int xGetInt(const xmlNodePtr node);
bool xGetBool(const xmlNodePtr node);

std::string xGetStrProp(const xmlNodePtr node, const char *prop);
int xGetIntProp(const xmlNodePtr node, const char *prop, int errValue = -1);

xmlNodePtr xFindNode(const xmlNodePtr node, const char *name);
xmlNodePtr xFindNodeR(const xmlNodePtr node, const char *name);

XmlDoc xMakeDoc(const std::string &str);
std::string xDumpDoc(const XmlDoc& doc, bool indent = false);

} // Timatic
