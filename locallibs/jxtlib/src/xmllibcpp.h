#ifndef _JXTLIB_XML_LIB_CPP_H_
#define _JXTLIB_XML_LIB_CPP_H_

#include <serverlib/xmllibcpp.h>

xmlNodePtr xmlNewTextChild(xmlNodePtr node, xmlNsPtr ns, const char *name, char value);
xmlNodePtr xmlNewTextChild(xmlNodePtr node, xmlNsPtr ns, const char *name, int value);
xmlNodePtr xmlNewTextChild(xmlNodePtr node, xmlNsPtr ns, const char *name, long value);
xmlNodePtr xmlNewTextChild(xmlNodePtr node, xmlNsPtr ns, const char *name, long long value);
xmlNodePtr xmlNewTextChild(xmlNodePtr node, xmlNsPtr ns, const char *name, unsigned value);
xmlNodePtr xmlNewTextChild(xmlNodePtr node, xmlNsPtr ns, const char *name, unsigned long value);
xmlNodePtr xmlNewTextChild(xmlNodePtr node, xmlNsPtr ns, const char *name, unsigned long long value);
xmlNodePtr xmlNewTextChild(xmlNodePtr node, xmlNsPtr ns, const char *name, double value);
xmlNodePtr xmlNewTextChild(xmlNodePtr node, xmlNsPtr ns, const char *name, float value);

void xmlNodeSetContent(xmlNodePtr cur, char c);

#endif
