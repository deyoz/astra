#ifndef __XSLT_TOOLS_
#define __XSLT_TOOLS_

#include <libxml/tree.h>
xmlDocPtr applyXSLT(xmlDocPtr xmlDoc, xmlDocPtr xslDoc, const char **params); 
xmlDocPtr applyXSLT(xmlDocPtr xmlDoc, const char* xsl, const char **params); 
xmlDocPtr applyXSLT(const char* xml, const char* xsl, const char **params); 
#endif 
