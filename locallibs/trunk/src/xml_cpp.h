#ifndef _JXTLIB_XML_CPP_H__
#define _JXTLIB_XML_CPP_H__

#include <serverlib/xml_cpp.h>

void XmlDocTrace(int Level, const char *nickname, const char *filename, int line, const XmlDoc& doc);
xmlNodePtr xmlDocGetRootElement(const XmlDoc& );


#endif // _JXTLIB_XML_CPP_H__
