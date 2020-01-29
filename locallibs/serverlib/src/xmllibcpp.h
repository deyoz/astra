#ifndef _XMLLIBCPP_H_
#define _XMLLIBCPP_H_
#include <sstream>
#include <string>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>

namespace boost {  namespace posix_time {  class ptime;  }  }
namespace boost {  namespace gregorian {  class date;  }  }

inline xmlDocPtr xmlNewDoc(const char* ver) {
    return xmlNewDoc(reinterpret_cast<const xmlChar*>(ver));
}

inline xmlNodePtr xmlNewDocNode(xmlDocPtr doc, xmlNsPtr ns, const char* name, const char* content) {
    return xmlNewDocNode(doc, ns, reinterpret_cast<const xmlChar*>(name), reinterpret_cast<const xmlChar*>(content));
}

inline int xmlStrcmp(const std::string &a,const std::string &b){
	return xmlStrcmp(reinterpret_cast<const xmlChar*>(a.c_str()),reinterpret_cast<const xmlChar*>(b.c_str()));
}
inline int xmlStrcmp(const xmlChar* a,const std::string &b){
	return xmlStrcmp(a,reinterpret_cast<const xmlChar*>(b.c_str()));
}
inline int xmlStrcmp(const std::string &a,const xmlChar *b){
	return xmlStrcmp(reinterpret_cast<const xmlChar*>(a.c_str()),b);
}

inline xmlNodePtr xmlNewChild (xmlNodePtr parent, xmlNsPtr ns, const std::string &name, const std::string &content){
	return xmlNewChild(parent,ns,reinterpret_cast<const xmlChar*>(name.c_str()),reinterpret_cast<const xmlChar*>(content.c_str()));
}

inline xmlNodePtr xmlNewChild (xmlNodePtr parent, xmlNsPtr ns, const std::string &name, const xmlChar *content){
	return xmlNewChild(parent,ns,reinterpret_cast<const xmlChar*>(name.c_str()),content);
}

inline xmlNodePtr xmlNewChild (xmlNodePtr parent, xmlNsPtr ns, const std::string &name){
	return xmlNewChild(parent,ns,reinterpret_cast<const xmlChar*>(name.c_str()),nullptr);
}

inline xmlChar* xmlStrdup(const std::string &cur){
	return xmlStrdup(reinterpret_cast<const xmlChar*>(cur.c_str()));
}
/*
template<typename T>
void xmlNodeSetContent(xmlNodePtr node, T&& value) {
  std::ostringstream buf;
  buf << value;
  xmlNodeSetContent(node, reinterpret_cast<const xmlChar*>(buf.str().c_str()));
}
*/
inline void xmlNodeSetContent(xmlNodePtr cur, const std::string &s)
{
    return xmlNodeSetContentLen(cur,reinterpret_cast<const xmlChar*>(s.c_str()), s.size());
}

inline void xmlNodeSetContent(xmlNodePtr cur, const char* s)
{
    return xmlNodeSetContent(cur, reinterpret_cast<const xmlChar*>(s));
}

inline void xmlNodeSetContent(xmlNodePtr cur, int s)
{
    return xmlNodeSetContent(cur, std::to_string(s));
}

inline void xmlNodeSetContentLen(xmlNodePtr node, const char* str, int len)
{
  return xmlNodeSetContentLen(node,reinterpret_cast<const xmlChar*>(str), len);
}

inline void xmlNodeSetContentLen(xmlNodePtr node, const std::string &str, int len)
{
  return xmlNodeSetContentLen(node,reinterpret_cast<const xmlChar*>(str.c_str()), len);
}

inline void xmlNodeSetName(xmlNodePtr node, const std::string &str)
{
  return xmlNodeSetName(node,reinterpret_cast<const xmlChar*>(str.c_str()));
}

xmlAttrPtr xmlSetPropNoEncCheck(xmlNodePtr node, const char *name, const char *value);


inline xmlAttrPtr xmlSetProp(xmlNodePtr node, const char* name, const char *value) {
  return xmlSetPropNoEncCheck(node, name, value);
}
inline xmlAttrPtr xmlSetProp(xmlNodePtr node, const char* name, const std::string & value) {
  return xmlSetPropNoEncCheck(node, name, value.c_str());
}

#define DISABLE_SPECIALISATION(T, TYPE) static_assert(not std::is_same<T,TYPE>::value, "disabled specialisation for " #TYPE )

template<class T> xmlAttrPtr xmlSetProp(xmlNodePtr node, const char* name, T&& value)
{
    DISABLE_SPECIALISATION(T, boost::gregorian::date);
    DISABLE_SPECIALISATION(T, boost::posix_time::ptime);
    std::ostringstream buf;
    buf << value;
    return xmlSetPropNoEncCheck(node, name, buf.str().c_str());
}

xmlAttrPtr xmlSetProp_ddmmyyyy(xmlNodePtr node, const char *name, const boost::gregorian::date& d);
xmlAttrPtr xmlSetProp_hh24mi_ddmmyyyy(xmlNodePtr node, const char *name, const boost::posix_time::ptime& t);

template<typename T>
inline xmlAttrPtr xmlSetProp(xmlNodePtr node, const std::string &name, T&& value) {
  return xmlSetProp(node, name.c_str(), value);
}
/*
template<typename T> xmlNodePtr xmlNewTextChild(xmlNodePtr node, xmlNsPtr ns, const char *name, T&& value)
{
    DISABLE_SPECIALISATION(T, boost::gregorian::date);
    DISABLE_SPECIALISATION(T, boost::posix_time::ptime);
    std::ostringstream buf;
    buf << value;
    return xmlNewTextChild(node, ns, reinterpret_cast<const xmlChar*>(name), reinterpret_cast<const xmlChar*>(buf.str().c_str()));
}
*/
inline xmlNodePtr xmlNewTextChild(xmlNodePtr node, xmlNsPtr ns, const char *name, const char * value) {
  return xmlNewTextChild(node, ns, reinterpret_cast<const xmlChar*>(name), reinterpret_cast<const xmlChar*>(value));
}

inline xmlNodePtr xmlNewTextChild(xmlNodePtr node, xmlNsPtr ns , const char *name , const std::string &value) {
 return xmlNewTextChild(node, ns, reinterpret_cast<const xmlChar*>(name), reinterpret_cast<const xmlChar*>(value.c_str()));
}

inline xmlNodePtr xmlNewTextChild(xmlNodePtr node, xmlNsPtr ns, const std::string &name, const std::string& value) {
  return xmlNewTextChild(node, ns, name.c_str(), value.c_str());
}

xmlNodePtr xmlNewTextChild_ddmmyyyy(xmlNodePtr node, xmlNsPtr ns, const char *name, const boost::gregorian::date& d);
xmlNodePtr xmlNewTextChild_hh24mi_ddmmyyyy(xmlNodePtr node, xmlNsPtr ns, const char *name, const boost::posix_time::ptime& t);

/*
template<typename T>
xmlNodePtr xmlNewTextChild(xmlNodePtr node, xmlNsPtr ns, const std::string &name, T&& value) {
  return xmlNewTextChild(node, ns, name.c_str(), value);
}
*/
inline xmlAttrPtr xmlHasProp(xmlNodePtr node, const char *name) {
  return xmlHasProp(node, reinterpret_cast<const xmlChar*>(name));
}

inline xmlNsPtr xmlNewNs(xmlNodePtr node, const char *href, const char *prefix) {
  return xmlNewNs(node, reinterpret_cast<const xmlChar*>(href), reinterpret_cast<const xmlChar*>(prefix));
}

inline xmlNsPtr xmlNewNs(xmlNodePtr node, const std::string &href, const std::string &prefix) {
  return xmlNewNs(node, href.c_str(), prefix.c_str());
}

#endif
