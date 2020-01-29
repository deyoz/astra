#if HAVE_CONFIG_H
#endif /* HAVE_CONFIG_H */
#include <string>
#include <cstring>
#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#define NICKNAME "ILYA"
#define NICKTRACE ILYA_TRACE
#include "test.h"
#include "xml_tools.h"
#include "xml_stuff.h"
#include "exception.h"
#include "xmllibcpp.h"
#include "xml_context.h"
#include "xml_cpp.h"
#include "isdigit.h"
#include "str_utils.h"

using namespace ServerFramework;


struct XmlCharHolder
{
  xmlChar *mem;

  XmlCharHolder(xmlChar *_mem = nullptr) : mem(_mem) {}
  ~XmlCharHolder()
  {
    if(mem != nullptr)
      xmlFree(mem);
  }
};


extern "C" const char *getText(const xmlNodePtr reqNode)
{
  if(!reqNode || !reqNode->children || reqNode->children->type!=XML_TEXT_NODE)
    return nullptr;
  return reinterpret_cast<const char*>(reqNode->children->content);
}

const char *gettext(const xmlNodePtr reqNode)
{
  return getText(reqNode);
}

extern "C" int istrue(const xmlNodePtr reqNode)
{
  if(!reqNode || !reqNode->children || reqNode->children->type!=XML_TEXT_NODE)
    return 0;
  if(xmlStrcmp(reqNode->children->content,"true")==0)
    return 1;
  return 0;
}

extern "C" int isfalse(const xmlNodePtr reqNode)
{
  if(!reqNode || !reqNode->children || reqNode->children->type!=XML_TEXT_NODE)
    return 0;
  if(xmlStrcmp(reqNode->children->content,"false")==0)
    return 1;
  return 0;
}

bool xmlbool(const xmlNodePtr reqNode, bool dflt)
{
  if(istrue(reqNode))
      return true;
  if(isfalse(reqNode))
      return false;
  return dflt;
}

extern "C" int isempty(const xmlNodePtr reqNode)
{
  if(!reqNode)
    return 1;
  if(reqNode->children==NULL || (reqNode->children->type==XML_TEXT_NODE && 
    (reqNode->children->content==NULL || reqNode->children->content[0]=='\0') 
     && reqNode->children->children==NULL))
    return 1;
  return 0;
}

extern "C" int istext(const xmlNodePtr reqNode)
{
  if(!reqNode || !reqNode->children)
    return 0;
  if(reqNode->children->type==XML_TEXT_NODE && reqNode->children->children==NULL)
    return 1;
  return 0;
}

extern "C" xmlAttrPtr xmlSetPropInt(xmlNodePtr resNode, const char *prop_name, int value)
{
    return xmlSetProp(resNode, prop_name, value);
}

extern "C" void xmlClearNode(xmlNodePtr node)
{
    if (!node)
        return;
    ProgTrace(TRACE5, "clearing node: %s", node->name);
//     currNode = NULL;
    while (xmlNodePtr currNode = node->children)
    {
        xmlUnlinkNode(currNode);
        xmlFreeNode(currNode);
    }
}

extern "C" int xmlGetPropInt(xmlNodePtr reqNode, const char *prop_name, int *res)
{
    auto tmp = getprop(reqNode,prop_name);
  if(!tmp)
    return 1;
  *res=atoi(tmp);
  return 0;
}

extern "C" void xmlNodeSetContentInt(xmlNodePtr resNode, int val)
{
  char str[20];
  sprintf(str,"%i",val);
  return xmlNodeSetContent(resNode,str);
}

extern "C" const char *getprop(const xmlNodePtr resNode, const char *prop)
{
  xmlAttrPtr pr;
  if(!resNode || !(pr=resNode->properties))
    return NULL;
  while(pr!=NULL)
  {
    if(xmlStrcmp(pr->name, prop)==0)
      return reinterpret_cast<const char*>(pr->children->content);
    pr=pr->next;
  }
  return NULL;
}

std::string getStrFromXml(const xmlNodePtr node)
{
  if(auto tmp = getText(node))
      return tmp;
  return std::string();
}

std::string getTrimmedStrFromXml(const xmlNodePtr node)
{
  return StrUtils::StringTrim(getStrFromXml(node));
}

std::string getCDataFromXml(xmlNodePtr node)
{
  if(!node || !node->children || node->children->type!=XML_CDATA_SECTION_NODE || !node->children->content)
    return std::string();
  return std::string((const char *)node->children->content);

}


int getIntFromXml(const xmlNodePtr dataNode, int err_value)
{
  return atoiNVL(getText(dataNode),err_value);
}

int getIntPropFromXml(const xmlNodePtr dataNode, const std::string &prop, int err_value)
{
  return atoiNVL(getprop(dataNode,prop.c_str()),err_value);
}

double strToDbl(const std::string& input, double err_value) {
  char *endptr=0;
  double res=strtod(input.c_str(),&endptr);
  if(endptr!=0 && (*endptr)!='\0')
    return err_value;
  return res;
}

double getDblPropFromXml(const xmlNodePtr dataNode, const std::string &prop, double err_value)
{
  std::string p = getStrPropFromXml(dataNode,prop);
  return (p.empty() ? err_value : strToDbl(p, err_value));
}

double getDblFromXml(const xmlNodePtr dataNode, double err_value) {
  std::string p = getStrFromXml(dataNode);
  return (p.empty() ? err_value : strToDbl(p, err_value));
}

std::string getStrPropFromXml(const xmlNodePtr node, const char *prop)
{
  const char *tmp=getprop(node,prop);
  return tmp?tmp:"";
}

std::string getStrPropFromXml(const xmlNodePtr node, const std::string &prop)
{
  const char *tmp=getprop(node, prop.c_str());
  return tmp ? tmp : "";
}

extern "C" xmlNodePtr newChild(xmlNodePtr resNode, const char *name)
{
  if(!resNode || !name)
    return NULL;
  xmlNodePtr retNode=xmlNewChild(resNode, NULL, name, NULL);
  if(!retNode)
  {
    ProgTrace(TRACE1, "newChild failed on resNode->name='%s', name='%s'", resNode->name,name);
    throw xml_exception(STDLOG, "Ошибка программы!");
  }
  return retNode;
}

xmlDocPtr make_tree(const char *str, int str_len)
{
  xmlDocPtr resDoc=NULL;

  xmlKeepBlanksDefault(0);
#ifdef XML_DEBUG
  if(str_len<2500)
    ProgTrace(TRACE1,"str='%s', str_len=%i, strlen(str)=%i",
      str,str_len,strlen(str));
#endif
  resDoc=xmlParseMemory(str,str_len);
  if(!resDoc)
    ProgTrace(TRACE1,"Error parsing string");
  else
  {
    XMLContext *ctxt=getXmlContext();
    if((!ctxt || !ctxt->donotencode2UTF8()) &&
       xml_decode_nodelist(resDoc->children))
      throw xml_exception(STDLOG,"Ошибка перекодировки xml-документа!");
  }
  return resDoc;
}

xmlDocPtr makeXmlTreeFromStr(const std::string &str)
{
  return make_tree(str.c_str(), str.size());
}

XmlDoc xml_parse_memory_CP866(const char* q, size_t len)
{
  XmlDoc resDoc=xml_parse_memory(q, len);
  if(!resDoc)
  {
    ProgTrace(TRACE1,"Error parsing string");
  }
  else
  {
    XMLContext *ctxt=getXmlContext();
    if((!ctxt || !ctxt->donotencode2UTF8()) && 
      xml_decode_nodelist(resDoc->children))
    {
      throw xml_exception(STDLOG,"Ошибка перекодировки xml-документа!");
    }
  }
  return resDoc;
}

XmlDoc xml_parse_memory_CP866(const char* q)
{
  return xml_parse_memory_CP866(q, strlen(q));
}

XmlDoc xml_parse_memory_CP866(const std::string& q)
{
  return xml_parse_memory_CP866(q.data(), q.size());
}


void make_str(xmlDocPtr reqDoc, char **str, int *str_len)
{
  if(xml_encode_nodelist(reqDoc->children))
    throw xml_exception(STDLOG,"Ошибка перекодировки xml-документа!");
  xmlFree(const_cast<xmlChar *>(reqDoc->encoding));
  reqDoc->encoding=xmlStrdup("UTF-8");
  xmlDocDumpMemory(reqDoc,(xmlChar **)str,str_len);
  if(xml_decode_nodelist(reqDoc->children))
    throw xml_exception(STDLOG,"Ошибка перекодировки xml-документа!");
}

std::string make_str(const XmlDoc& doc)
{
    return CP866toUTF8(xml_dump(doc, false));
}

std::string make_format_str(const XmlDoc& doc)
{
    return CP866toUTF8(xml_dump(doc, true));
}

std::string makeStrFromXmlTree(const xmlDocPtr reqDoc)
{
  XmlCharHolder xmlBuffHolder;
  int buffSize=0;
  make_str(reqDoc, reinterpret_cast<char**>(&xmlBuffHolder.mem), &buffSize);
  return std::string(reinterpret_cast<char*>(xmlBuffHolder.mem), buffSize);
}

void make_format_str(xmlDocPtr reqDoc, char **str, int *str_len)
{
  if(xml_encode_nodelist(reqDoc->children))
    throw xml_exception(STDLOG,"Ошибка перекодировки xml-документа!");
  xmlFree(const_cast<xmlChar *>(reqDoc->encoding));
  reqDoc->encoding=xmlStrdup("UTF-8");
  xmlDocDumpFormatMemory(reqDoc,(xmlChar **)str,str_len,1);
  if(xml_decode_nodelist(reqDoc->children))
    throw xml_exception(STDLOG,"Ошибка перекодировки xml-документа!");
}

std::string makeFormatStrFromXmlTree(const xmlDocPtr reqDoc)
{
  XmlCharHolder xmlBuffHolder;
  int buffSize=0;
  make_format_str(reqDoc, reinterpret_cast<char**>(&xmlBuffHolder.mem),
    &buffSize);
  return std::string(reinterpret_cast<char*>(xmlBuffHolder.mem), buffSize);
}

std::string formatXmlString(const std::string &xml_in)
{
  xmlKeepBlanksDefault(0);
  xmlDocPtr doc=xmlParseMemory(xml_in.c_str(), xml_in.size());
  std::string xml_out;
  if(!doc)
  {
    ProgTrace(TRACE1,"Error parsing string");
    xml_out=xml_in; // просто скопируем входную строку
  }
  else
  {
    char *str=0;
    int str_len=0;
    xmlFree(const_cast<xmlChar *>(doc->encoding));
    doc->encoding=xmlStrdup("UTF-8");
    xmlDocDumpFormatMemory(doc,(xmlChar **)&str, &str_len, 1);
    xmlFreeDoc(doc);
    xml_out=std::string(str, str_len);
    xmlFree(str);
  }
  return xml_out;
}

xmlNodePtr findNode(const xmlNodePtr resNode, const char *name)
{
  xmlNodePtr retNode=NULL;
  const char *ptr=strchr(name,'/'), *brk;
  if(!resNode || isempty(resNode))
    return NULL;

  if(!ptr)
    brk=const_cast<char *>(name)+strlen(name);
  else
    brk=ptr;

  retNode=resNode->children;
  while(retNode)
  {
    if(strncmp(reinterpret_cast<const char*>(retNode->name),name,brk-name)==0 &&
       (int)strlen(reinterpret_cast<const char*>(retNode->name))==(brk-name))
    {
      if(!ptr)
        return retNode;
      return findNode(retNode,ptr+1);
    }
    retNode=retNode->next;
  }
  return NULL;
}

xmlNodePtr findNodeR(const xmlNodePtr reqNode, const char *tagname)
{
  xmlNodePtr node=reqNode;
  while(node!=NULL && tagname!=NULL)
  {
    if(xmlStrcmp(node->name,tagname)==0)
      return node;
    if(!isempty(node))
    {
      xmlNodePtr snode=findNodeR(node->children,tagname);
      if(snode)
        return snode;
    }
    node=node->next;
  }
  return NULL;
}

xmlNodePtr getNode(const xmlNodePtr resNode, const char *name)
{
  xmlNodePtr retNode=NULL;
  const char *ptr=strchr(name,'/'), *brk;
  char iface[100]="";

  if(resNode && !isempty(resNode))
  {
    if(!ptr)
      brk=name+strlen(name);
    else
      brk=ptr;

    retNode=resNode->children;
    while(retNode)
    {
      if((int)strlen(reinterpret_cast<const char*>(retNode->name))==(brk-name) &&
         strncmp(reinterpret_cast<const char*>(retNode->name),name,brk-name)==0)
      {
        if(!ptr)
          return retNode;
        return getNode(resNode->children,ptr+1);
      }
      /*ProgTrace(TRACE1,"<%s>,%i",retNode->name,brk-name);*/
      retNode=retNode->next;
    }
  }

  if(ptr)
  {
    if(ptr-name>99)
    {
      ProgError(STDLOG,"Too long tag name - cannot create :<%.*s...>",99,name);
      throw xml_exception(STDLOG,"getNode() failed to create node");
    }
    strncpy(iface,name,ptr-name);
    retNode=newChild(resNode,iface);
  }
  else
  {
    if(!name)
      return resNode;
     retNode=newChild(resNode,name);
  }
  if(ptr)
    return getNode(retNode,ptr+1);
  return retNode;
}

xmlNodePtr fnode(const xmlNodePtr node, const char *pathname)
{
    return find_node(node, pathname);
}

xmlNodePtr findNext(const xmlNodePtr node)
{
  xmlNodePtr n;
  int ind = 0;

  if (!node || !node->parent)
  {
    return NULL;
  }
  auto buf = getprop(node, "index");
  if (!buf)
  {
    return NULL;
  }
  ind=atoi(buf)+1;
  n=(node->parent)->children;
  while(n!=NULL)
  {
    buf = getprop(n, "index");
    if (buf && atoi(buf)==ind)
    {
      return n;
    }
    n=n->next;
  }

  return NULL;
}

xmlNodePtr findIChild(const xmlNodePtr reqNode, int ind)
{
  xmlNodePtr node;

  if(isempty(reqNode))
    return NULL;
  node=reqNode->children;
  while(node!=NULL)
  {
    auto buf=getprop(node,"index");
    if(buf && atoiNVL(buf,-1)==ind)
      return node;
    node=node->next;
  }
  return NULL;
}

xmlNodePtr findNodeByProp(const xmlNodePtr reqNode, const char *prop,
                          const char *val)
{
  if(!reqNode || isempty(reqNode))
    return NULL;
  xmlNodePtr node=reqNode->children;
  while(node!=NULL)
  {
    auto tmp=getprop(node,prop);
    if(tmp!=NULL && xmlStrcmp(tmp,val)==0)
      return node;
    if(!isempty(node))
    {
      xmlNodePtr cnode=findNodeByProp(node->children,prop,val);
      if(cnode)
        return cnode;
    }
    node=node->next;
  }
  return node;
}

xmlNodePtr findNamedNodeByProp(const xmlNodePtr reqNode, const char *name,
                               const char *prop, const char *val)
{
  if(!reqNode)
    return NULL;
  xmlNodePtr node=reqNode->children;
  while(node!=NULL)
  {
    auto tmp=getprop(node,prop);
    if(xmlStrcmp(node->name,name)==0 && tmp!=NULL && strcmp(tmp,val)==0)
      return node;
    if(0 && !isempty(node))
    {
      xmlNodePtr cnode=findNamedNodeByProp(node->children,name,prop,val);
      if(cnode)
        return cnode;
    }
    node=node->next;
  }
  return node;
}

xmlNodePtr findNamedNodeByPropR(const xmlNodePtr reqNode, const char *name,
                                const char *prop, const char *val)
{
  if(!reqNode)
    return NULL;
  xmlNodePtr node=reqNode->children;
  while(node!=NULL)
  {
    auto tmp=getprop(node,prop);
    if(xmlStrcmp(node->name,name)==0 && tmp!=NULL && strcmp(tmp,val)==0)
      return node;
    if(!isempty(node))
    {
      xmlNodePtr cnode=findNamedNodeByPropR(node,name,prop,val);
      if(cnode)
        return cnode;
    }
    node=node->next;
  }
  return node;
}

xmlNodePtr xmlNewBoolChild(xmlNodePtr parentNode, const char *name, bool value)
{
  return xmlNewTextChild(parentNode,0,name,value?"true":"false");
}

void xmlNewBoolProp(xmlNodePtr node, const char *name, bool value)
{
  xmlSetProp(node,name,value?"true":"false");
}

#ifdef XP_TESTING

#include "xp_test_utils.h"
#include "checkunit.h"

void init_xml_tools_tests()
{
}

START_TEST(TestFindNode)
{
  xmlDocPtr fareDoc = xmlNewDoc(reinterpret_cast<const xmlChar*>("1.0"));
  fareDoc->children = xmlNewDocNode(fareDoc, NULL, reinterpret_cast<const xmlChar*>("root"), NULL);
  fareDoc->encoding = xmlStrdup("UTF-8");
  xmlNodePtr fareNode = fareDoc->children;
  xmlNewTextChild(fareNode, NULL, "test1", ""); 
  //xmlNodePtr subNode = newChild(fareNode, "test2");
  if (!findNode(fareNode, "test1"))
  {
     fail_unless(0,"FindNode BUGGGG");
  }
  if (findNode(fareNode, "test2"))
  {
     fail_unless(0,"FindNode empty node");
  }
  xmlFreeDoc(fareDoc);
}
END_TEST

START_TEST(XPath_check)
{
   const char * tdesc = "<?xml version='1.0' encoding='UTF-8'?>"
                "<request><awk/><race/></request>";
   const char * bdesc = 
    "<?xml version='1.0' encoding='UTF-8'?> "
        " <sirena> <query> "    
        " <pricing from='mikhail' more_tracing='true' return_request='true'>      "
        " <segment> <departure>MOW</departure> <arrival>LED</arrival> <date>30.08.06</date>  <class>Y</class>  </segment>      "
        " <segment> <departure>LED</departure> <arrival>MOW</arrival> <date>01.09.06</date>  <class>Y</class>  </segment>      "
        " <passenger> <code>AAA</code> <count>1</count>  </passenger>      "
        "  <request_params> <min_results>5</min_results>  <mix_ac>falsei</mix_ac> <tick_ser>D</tick_ser> </request_params>      "
        " <answer_params>        "
        "  <return_stats>true</return_stats>  <regroup>truei</regroup>        "
        "  <split_companies>truei</split_companies>        <show_regroup_io_matching>truei</show_regroup_io_matching>        "
        "  <show_io_matching>truei</show_io_matching>        <show_flighttime>truei</show_flighttime>        "
        "  <show_baseclass>truei</show_baseclass>     <show_varianttotal>true</show_varianttotal>        "
        "  <show_reg_latin>truei</show_reg_latin>    "
        " </answer_params> "    
        "</pricing>  </query></sirena> " ; 
   
   xmlDocPtr doc = make_tree(tdesc, strlen(tdesc));  
   xmlDocPtr doc2 = make_tree(bdesc, strlen(bdesc)); 
   xmlNodePtr node = find_node(doc->children, "/request/awk"); 
   if (!node) 
   { 
     fail_unless(0,"node not found");
   }
   else 
   {
     ProgTrace(TRACE1, "found %s", node->name);
   }
   xmlNodePtr node2 = doc->children; 
   ProgTrace(TRACE1, "current name is %s", node2->name);
   node = find_node(node2, "awk"); 
   if (!node)
   {
     fail_unless(0,"node not found2"); 
   } 
   else 
   {
     ProgTrace(TRACE1, "found %s", node->name); 
   }
   node = find_node(doc->children, "//hz"); 
   if (node) 
   {
     fail_unless(0,"shit happend - hz found !!!!"); 
   }
   else 
   {
     ProgTrace(TRACE1, "kk hz not found");
   }
   node = find_node(doc2->children, "//tick_ser");
   if (!node) 
   {
     fail_unless(0,"pricing not found"); 
   }
   else 
   {
     ProgTrace(TRACE1, "kk found %s", node->name); 
   }
   //XmlNodeTrace(TRACE1, doc->children);
   xmlFreeDoc(doc2);
   xmlFreeDoc(doc);
}
END_TEST

START_TEST(make_str)
{
    {
    constexpr char const * const xml_utf8 = "<?xml version='1.0' encoding='UTF-8'?><request><awk/></request>";
    auto doc = XmlDoc::impropriate(make_tree(xml_utf8, strlen(xml_utf8)));
    fail_if(not doc);
    char* str;
    int len;
    make_str(doc.get(), &str, &len);
    fail_if(make_str(doc).compare(0,len,str) != 0);
    xmlFree(str);
    }
    {
    constexpr char const * const xml_cp866 = "<?xml version='1.0' encoding='CP866'?><request><awk>ЮТ</awk></request>";
    auto doc = XmlDoc::impropriate(make_tree(xml_cp866, strlen(xml_cp866)));
    fail_if(not doc);
    char* str;
    int len;
    make_str(doc.get(), &str, &len);
    fail_if(make_str(doc).compare(0,len,str) != 0);
    xmlFree(str);
    }
    if(false){
    constexpr char const * const xml_fake_utf8 = "<?xml version='1.0' encoding='UTF-8'?><request><awk>ЮТ</awk></request>";
    auto doc = XmlDoc::impropriate(make_tree(xml_fake_utf8, strlen(xml_fake_utf8)));
    fail_if(not doc);
    char* str;
    int len;
    make_str(doc.get(), &str, &len);
    fail_if(make_str(doc).compare(0,len,str) != 0);
    xmlFree(str);
    }
}
END_TEST

#define SUITENAME "XmlTools"
TCASEREGISTER(0, 0)
 ADD_TEST(XPath_check);
 ADD_TEST(TestFindNode);
 ADD_TEST(make_str)
TCASEFINISH

#endif

