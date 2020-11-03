#ifndef _JXTLIB_XML_LIB_CPP_H_
#define _JXTLIB_XML_LIB_CPP_H_

#include <serverlib/xmllibcpp.h>
#include <serverlib/int_parameters.h>

xmlNodePtr xmlNewTextChild(xmlNodePtr node, xmlNsPtr ns, const char *name, char value);
xmlNodePtr xmlNewTextChild(xmlNodePtr node, xmlNsPtr ns, const char *name, int value);
xmlNodePtr xmlNewTextChild(xmlNodePtr node, xmlNsPtr ns, const char *name, long value);
xmlNodePtr xmlNewTextChild(xmlNodePtr node, xmlNsPtr ns, const char *name, long long value);
xmlNodePtr xmlNewTextChild(xmlNodePtr node, xmlNsPtr ns, const char *name, unsigned value);
xmlNodePtr xmlNewTextChild(xmlNodePtr node, xmlNsPtr ns, const char *name, unsigned long value);
xmlNodePtr xmlNewTextChild(xmlNodePtr node, xmlNsPtr ns, const char *name, unsigned long long value);
xmlNodePtr xmlNewTextChild(xmlNodePtr node, xmlNsPtr ns, const char *name, double value);
xmlNodePtr xmlNewTextChild(xmlNodePtr node, xmlNsPtr ns, const char *name, float value);

template<typename traits_t, typename base_t>
xmlNodePtr xmlNewTextChild(xmlNodePtr node, xmlNsPtr ns, const char *name, ParInt::BaseIntParam<traits_t,base_t> val)
{
    if(val)
        return xmlNewTextChild(node, ns, name, val.get());
    else
        return xmlNewTextChild(node, ns, name, nullptr);
}

void xmlNodeSetContent(xmlNodePtr cur, char c);

#endif
