/* pragma cplusplus */
#ifdef HAVE_MPATROL
#include <mpatrol.h>
#endif
#if HAVE_CONFIG_H
#endif /* HAVE_CONFIG_H */

#define NICKNAME "YURAN"
#define NICKTRACE YURAN_TRACE

#include <sstream>
#include <string>
#include <cstring>
#include <vector>

#include <serverlib/logger.h>
#include <serverlib/test.h>

#include <libxslt/xslt.h>
#include <libxslt/xsltInternals.h>
#include <libxslt/transform.h>
#include <libxslt/xsltutils.h>
#include <libxslt/xsltutils.h>

#include "xml_cpp.h"

xmlDocPtr applyXSLT(xmlDocPtr xmlDoc, const char* xsl, const char **params)
{
  if (!xsl || !xmlDoc)
  {
     return NULL;
  }
  struct GlobalsHolder {
      GlobalsHolder() {
          xmlSubstituteEntitiesDefault(1);
          xmlLoadExtDtdDefaultValue = 1;
      }
      ~GlobalsHolder() {
          xsltCleanupGlobals();
      }
  } globals_holder;
  auto xslDoc = xmlParseMemory(xsl, strlen(xsl));
  if (!xslDoc)
  {
    return NULL;
  }
  auto cur = xsltParseStylesheetDoc(xslDoc);
  auto res = xsltApplyStylesheet(cur, xmlDoc, params);
  xsltFreeStylesheet(cur);
  return res;
}

xmlDocPtr applyXSLT(const char* xml, const char* xsl, const char **params) 
{
  if (!xml)
  {
     return NULL;
  }
  auto xmlDoc = xml_parse_memory(xml);
  return applyXSLT(xmlDoc.get(), xsl, params);
}


#ifdef XP_TESTING
#include "serverlib/xp_test_utils.h"
#include "serverlib/checkunit.h"

#define SUITENAME "XsltTools"

void xslt_tools_setup(void)
{
}

void xslt_tools_teardown(void)
{
}


START_TEST(TestXSLTtoText)
{
  char style[]=
      "<xsl:stylesheet version='1.0' xmlns:xsl='http://www.w3.org/1999/XSL/Transform'>"
      "<xsl:output method='text'/><xsl:template match='/'> "
      " <xsl:text>Hello</xsl:text> "
      " <xsl:text>World</xsl:text> "
    "</xsl:template></xsl:stylesheet> "; 
  char xml[]="<?xml version='1.0' encoding='UTF-8'?><hi></hi>";
  const char *params[] = {"page", "0", NULL};
  if (xmlDocPtr res = applyXSLT(xml, style, params)) 
  { 
    //XmlNodeTrace(TRACE1, res->children);
    xmlFreeDoc(res);
  } 
  else 
  {
    fail_unless(0, "XSLT TEXT not work !!!" );
  }

}
END_TEST
START_TEST(TestXSLTStyle)
{
  char style[]=
      "<xsl:stylesheet version='1.0' xmlns:xsl='http://www.w3.org/1999/XSL/Transform'>"
      "<xsl:template match='/'> "
      "  <hello>Hello</hello> "
      "  <tex><xsl:value-of select='$page'/></tex>"
    "</xsl:template></xsl:stylesheet> "; 
  char xml[]="<?xml version='1.0' encoding='UTF-8'?><hi></hi>";
  char name[] = "page"; 
  char value[] = "1"; 
  const char *params[16 + 1];
  int nbparams = 2;
  params[0] = name; 
  params[1] = value; 
  xsltStylesheetPtr cur = NULL;
  params[nbparams] = NULL;
  xmlSubstituteEntitiesDefault(1);
  xmlLoadExtDtdDefaultValue = 1;
  auto xslDoc = xml_parse_memory(style, strlen(style));
  auto xmlDoc = xml_parse_memory(xml, strlen(xml));
  cur = xsltParseStylesheetDoc(xslDoc.get());
  auto res = XmlDoc::impropriate(xsltApplyStylesheet(cur, xmlDoc.get(), params));

  //xmlSaveFile("/home/yuran/xmltest/result.xml" , res);
  fail_if(not res, "XSLT does not work !!!");
  //xsltFreeStylesheet(cur);

  xsltCleanupGlobals();
  xmlCleanupParser();
}
END_TEST

TCASEREGISTER(xslt_tools_setup, xslt_tools_teardown)
 ADD_TEST(TestXSLTStyle);
 ADD_TEST(TestXSLTtoText);
TCASEFINISH


#endif
