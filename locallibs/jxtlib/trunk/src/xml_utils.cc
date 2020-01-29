/* pragma cplusplus */
#ifdef HAVE_MPATROL
#include <mpatrol.h>
#endif
#include <string>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <tclmon/lwriter.h>
//#include "setup.h"
#define NICKNAME "ILYA"
#define NICKTRACE ILYA_TRACE
#include <serverlib/test.h>
//#include "jxt_tools.h"
#include "xml_utils.h"
#include "xml_stuff.h"
#include "xml_cpp.h"
#include "xmllibcpp.h"
#include <serverlib/log_manager.h>

bool is_CutLog(int level)
{
    return !LogManager::Instance().isWriteLog(level);
}

void XmlNodeTrace(int Level, const char *nickname, const char *filename,
                  int line, const xmlNodePtr m, int format)
{
  if (is_CutLog(Level))
    return;

  ProgTrace(Level,nickname,filename,line,"node=%p doc=%p",m,
    m!=NULL?m->doc:NULL);
  if (m!=NULL)
  {
    ProgTrace(Level,nickname,filename,line, "%s", xml_node_dump(m, format).c_str());
  }
  xmlAttrPtr a=m==NULL?NULL:m->properties;
  while(a!=NULL)
  {
    ProgTrace(Level,nickname,filename,line,"a=%p name=%p '%s'",a,a->name, a->name);

    for(xmlNodePtr cont=a->children;cont!=NULL;cont=cont->next)
    {
      ProgTrace(Level,nickname,filename,line,
                "   cont=%p cont->name='%s' cont->content=%p '%s'",
                cont,cont->name,cont->content,cont->content);
    }
    a=a->next;
  }
}

void XmlDocTrace(int Level, const char *nickname, const char *filename, int line, const XmlDoc& doc)
{
    XmlDocTrace(Level, nickname, filename, line, doc.get());
}

void XmlDocTrace(int Level, const char *nickname, const char *filename,
                 int line, const xmlDocPtr doc)
{
  if (is_CutLog(Level))
    return;

  ProgTrace(Level,nickname,filename,line,"doc=%p",doc);

  xmlNodePtr n=NULL;
  if(doc!=NULL)
    n=xmlDocGetRootElement(doc);
  XmlNodeTrace(Level,nickname,filename,line,n);
}

xmlNodePtr xmlDocGetRootElement(const XmlDoc& x)
{
    return x ? xmlDocGetRootElement(x.get()) : nullptr;
}

xmlNodePtr xmlNewTextChild(xmlNodePtr node, xmlNsPtr ns, const char *name, char value)
{
    xmlNodePtr n = xmlNewTextChild(node, ns, name, nullptr);
    if(n)
        xmlNodeSetContentLen(n, &value, 1);
    return n;
}
xmlNodePtr xmlNewTextChild(xmlNodePtr node, xmlNsPtr ns, const char *name, int value) { return xmlNewTextChild(node, ns, name, std::to_string(value)); }
xmlNodePtr xmlNewTextChild(xmlNodePtr node, xmlNsPtr ns, const char *name, long value) { return xmlNewTextChild(node, ns, name, std::to_string(value)); }
xmlNodePtr xmlNewTextChild(xmlNodePtr node, xmlNsPtr ns, const char *name, long long value) { return xmlNewTextChild(node, ns, name, std::to_string(value)); }
xmlNodePtr xmlNewTextChild(xmlNodePtr node, xmlNsPtr ns, const char *name, unsigned value) { return xmlNewTextChild(node, ns, name, std::to_string(value)); }
xmlNodePtr xmlNewTextChild(xmlNodePtr node, xmlNsPtr ns, const char *name, unsigned long value) { return xmlNewTextChild(node, ns, name, std::to_string(value)); }
xmlNodePtr xmlNewTextChild(xmlNodePtr node, xmlNsPtr ns, const char *name, unsigned long long value) { return xmlNewTextChild(node, ns, name, std::to_string(value)); }
xmlNodePtr xmlNewTextChild(xmlNodePtr node, xmlNsPtr ns, const char *name, float value) { return xmlNewTextChild(node, ns, name, std::to_string(value)); }
xmlNodePtr xmlNewTextChild(xmlNodePtr node, xmlNsPtr ns, const char *name, double value) { return xmlNewTextChild(node, ns, name, std::to_string(value)); }

void xmlNodeSetContent(xmlNodePtr cur, char c)
{
    xmlNodeSetContentLen(cur, &c, 1);
}
