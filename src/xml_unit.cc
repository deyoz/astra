#include <vector>
#include <string>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include "xml_unit.h"
#include "basic.h"
#include "stl_utils.h"

#define NICKNAME "VLAD"
#include "serverlib/test.h"


using namespace std;
using namespace BASIC;

const char* NodeContent(xmlNodePtr node)
{
    const char *res=NULL;
  if (node!=NULL)
  {
    if (node->type==XML_TEXT_NODE) res=(char*)node->content;
    else
    {
      //поиск XML_TEXT_NODE среди потомков
      node=node->children;
      while(node!=NULL)
      {
        if (node->type==XML_TEXT_NODE)
        {
          res=(char*)node->content;
          break;
        };
        node=node->next;
      };
    };
  };
  if (res==NULL)
    return "";
  else
    return res;
};

xmlNodePtr find_node(const char* expr, xmlDocPtr doc, xmlNodePtr cur=NULL)
{
  xmlNodePtr res=NULL;
  xmlXPathContextPtr ctx=NULL;
  xmlXPathObjectPtr obj=NULL;
  if (doc!=NULL&&(ctx=xmlXPathNewContext(doc))!=NULL)
  {
    ctx->node=cur;
    obj=xmlXPathEval((xmlChar*)expr,ctx);
    if (obj!=NULL&&obj->type==XPATH_NODESET&&
        obj->nodesetval!=NULL&&obj->nodesetval->nodeNr>0&&
        obj->nodesetval->nodeTab[0]!=(void*)doc)
      res=obj->nodesetval->nodeTab[0];
  };
  if (obj!=NULL) xmlXPathFreeObject(obj);
  if (ctx!=NULL) xmlXPathFreeContext(ctx);
  return res;
};

xmlNodePtr GetNode(const char* expr, xmlDocPtr data, xmlNodePtr cur)
{
  return find_node(expr,data,cur);
};

xmlNodePtr GetNode(const char* expr, xmlNodePtr cur)
{
  if (cur==NULL)
    return find_node(expr,NULL,cur);
  else
    return find_node(expr,cur->doc,cur);
};

void GetNodes(const char* expr, xmlDocPtr data, vector<xmlNodePtr>& nodes, xmlNodePtr cur)
{
  nodes.clear();
  xmlXPathContextPtr ctx=NULL;
  xmlXPathObjectPtr obj=NULL;
  if (data!=NULL&&(ctx=xmlXPathNewContext(data))!=NULL)
  {
    ctx->node=cur;
    obj=xmlXPathEval((xmlChar*)expr,ctx);
    if (obj!=NULL&&obj->type==XPATH_NODESET&&
        obj->nodesetval!=NULL)
    {
      for(int i=0;i<obj->nodesetval->nodeNr;i++)
      {
        if (obj->nodesetval->nodeTab[i]!=(void*)data)
          nodes.push_back(obj->nodesetval->nodeTab[i]);
      };
    };
  };
  if (ctx!=NULL) xmlXPathFreeContext(ctx);
  if (obj!=NULL) xmlXPathFreeObject(obj);
  return;
};

void GetNodes(const char* expr, vector<xmlNodePtr>& nodes, xmlNodePtr cur)
{
  if (cur==NULL)
    GetNodes(expr,NULL,nodes,cur);
  else
    GetNodes(expr,cur->doc,nodes,cur);
};

xmlNodePtr NodeAsNode(const char* expr, xmlDocPtr data, xmlNodePtr cur)
{
  xmlNodePtr node;
  node=find_node(expr,data,cur);
  if (node==NULL)
    throw EXMLError(string("Node '") + expr + "' does not exists");
  return node;
};

xmlNodePtr NodeAsNode(const char* expr, xmlNodePtr cur)
{
  if (cur==NULL)
    return NodeAsNode(expr,NULL,cur);
  else
    return NodeAsNode(expr,cur->doc,cur);
};

bool NodeIsNULL(xmlNodePtr node)
{
  if (node==NULL) throw EXMLError("Node not defined (NULL)");
  return strcmp(NodeContent(node),"")==0;
};

bool NodeIsNULL(const char* expr, xmlDocPtr data, xmlNodePtr cur)
{
  xmlNodePtr node;
  node=find_node(expr,data,cur);
  if (node==NULL)
    throw EXMLError(string("Node '") + expr + "' does not exists");
  return strcmp(NodeContent(node),"")==0;
};

bool NodeIsNULL(const char* expr, xmlNodePtr cur)
{
  if (cur==NULL)
    return NodeIsNULL(expr,NULL,cur);
  else
    return NodeIsNULL(expr,cur->doc,cur);
};

const char* NodeAsString(xmlNodePtr node)
{
  if (node==NULL) throw EXMLError("Node not defined (NULL)");
  return NodeContent(node);
};

const char* NodeAsString(const char* expr, xmlDocPtr data, xmlNodePtr cur)
{
  xmlNodePtr node;
  node=find_node(expr,data,cur);
  if (node==NULL)
    throw EXMLError(string("Node '") + expr + "' does not exists");
  return NodeContent(node);
};

const char* NodeAsString(const char* expr, xmlNodePtr cur, const char *nvl)
{
    if(xmlNodePtr node = GetNode(expr, cur))
        return NodeAsString(node);
    else
        return nvl;
};

int NodeAsInteger(const char* expr, xmlNodePtr cur, int nvl)
{
    if(xmlNodePtr node = GetNode(expr, cur))
        return NodeAsInteger(node);
    else
        return nvl;
};

double NodeAsFloat(const char* expr, xmlNodePtr cur, double nvl)
{
    if(xmlNodePtr node = GetNode(expr, cur))
        return NodeAsFloat(node);
    else
        return nvl;
};

TDateTime NodeAsDateTime(const char* expr, xmlNodePtr cur, TDateTime nvl)
{
    if(xmlNodePtr node = GetNode(expr, cur))
        return NodeAsDateTime(node);
    else
        return nvl;
};

const char* NodeAsString(const char* expr, xmlNodePtr cur)
{
  if (cur==NULL)
    return NodeAsString(expr,NULL,cur);
  else
    return NodeAsString(expr,cur->doc,cur);
};

int NodeAsInteger(xmlNodePtr node)
{
  int Value;
  if (node==NULL) throw EXMLError("Node not defined (NULL)");
  if ( StrToInt( NodeContent(node), Value ) == EOF )
    throw EXMLError("Cannot convert node to an Integer");
  return Value;
};

int NodeAsInteger(const char* expr, xmlDocPtr data, xmlNodePtr cur)
{
  int Value;
  xmlNodePtr node;
  node=find_node(expr,data,cur);
  if (node==NULL)
    throw EXMLError(string("Node '") + expr + "' does not exists");
  if ( StrToInt( NodeContent(node), Value ) == EOF )
    throw EXMLError(string("Cannot convert node '") + expr + "' to an Integer");
  return Value;
};

int NodeAsInteger(const char* expr, xmlNodePtr cur)
{
  if (cur==NULL)
    return NodeAsInteger(expr,NULL,cur);
  else
    return NodeAsInteger(expr,cur->doc,cur);
};

double NodeAsFloat(xmlNodePtr node)
{
  double Value;
  if (node==NULL) throw EXMLError("Node not defined (NULL)");
  if ( StrToFloat( NodeContent(node), Value ) == EOF )
    throw EXMLError("Cannot convert node to an Float");
  return Value;
};

double NodeAsFloat(const char* expr, xmlDocPtr data, xmlNodePtr cur)
{
  double Value;
  xmlNodePtr node;
  node=find_node(expr,data,cur);
  if (node==NULL)
    throw EXMLError(string("Node '") + expr + "' does not exists");
  if ( StrToFloat( NodeContent(node), Value ) == EOF )
    throw EXMLError(string("Cannot convert node '") + expr + "' to an Float");
  return Value;
};

double NodeAsFloat(const char* expr, xmlNodePtr cur)
{
  if (cur==NULL)
    return NodeAsFloat(expr,NULL,cur);
  else
    return NodeAsFloat(expr,cur->doc,cur);
};

TDateTime NodeAsDateTime(xmlNodePtr node)
{
    return NodeAsDateTime(node, (char*)ServerFormatDateTimeAsString);
}

TDateTime NodeAsDateTime(xmlNodePtr node, const char* format)
{
  TDateTime Value;
  if (node==NULL) throw EXMLError("Node not defined (NULL)");
  if ( StrToDateTime( NodeContent(node), format, Value ) == EOF )
    throw EXMLError("Cannot convert node to an DateTime");
  return Value;
};

TDateTime NodeAsDateTime(const char* expr, xmlDocPtr data, const char* format, xmlNodePtr cur)
{
  TDateTime Value;
  xmlNodePtr node;
  node=find_node(expr,data,cur);
  if (node==NULL)
    throw EXMLError(string("Node '") + expr + "' does not exists");
ProgTrace( TRACE5, "expr=%s, format=%s, nodecont=%s", expr, format, NodeContent(node) );
  if ( StrToDateTime( NodeContent(node), format, Value ) == EOF )
    throw EXMLError(string("Cannot convert node '") + expr + "' to an DateTime");
  return Value;
};

TDateTime NodeAsDateTime(const char* expr, const char* format, xmlNodePtr cur)
{
  if (cur==NULL)
    return NodeAsDateTime(expr,NULL,format,cur);
  else
    return NodeAsDateTime(expr,cur->doc,format,cur);
};

TDateTime NodeAsDateTime(const char* expr, xmlNodePtr cur)
{
  return NodeAsDateTime( expr, (char*)ServerFormatDateTimeAsString, cur );
}

xmlNodePtr GetNodeFast(const char *expr, xmlNodePtr &node)
{
  if (node==NULL) return NULL;
  xmlNodePtr node2=node;
  while (node2!=NULL)
  {
    if (strcmp((char*)node2->name,expr)==0)
    {
      if (node2->next!=NULL) node=node2->next;
      return node2;
    };
    node2=node2->prev;
  };
  node2=node->next;
  while (node2!=NULL)
  {
    if (strcmp((char*)node2->name,expr)==0)
    {
      if (node2->next!=NULL) node=node2->next;
      return node2;
    };
    node2=node2->next;
  };
  return NULL;
}

xmlNodePtr NodeAsNodeFast(const char *expr, xmlNodePtr &node)
{
  xmlNodePtr node2=GetNodeFast(expr,node);
  if (node2==NULL)
    throw EXMLError(string("Node '") + expr + "' does not exists");
  return node2;
}

bool NodeIsNULLFast(const char *expr, xmlNodePtr &node)
{
  return NodeIsNULL(NodeAsNodeFast(expr,node));
}

const char* NodeAsStringFast(const char *expr, xmlNodePtr &node)
{
  return NodeAsString(NodeAsNodeFast(expr,node));
}

int NodeAsIntegerFast(const char *expr, xmlNodePtr &node)
{
  return NodeAsInteger(NodeAsNodeFast(expr,node));
}

double NodeAsFloatFast(const char *expr, xmlNodePtr &node)
{
  return NodeAsFloat(NodeAsNodeFast(expr,node));
}

BASIC::TDateTime NodeAsDateTimeFast(const char *expr, const char *format, xmlNodePtr &node)
{
  return NodeAsDateTime(NodeAsNodeFast(expr,node),format);
}

BASIC::TDateTime NodeAsDateTimeFast(const char *expr, xmlNodePtr &node)
{
  return NodeAsDateTime(NodeAsNodeFast(expr,node),(char*)ServerFormatDateTimeAsString);
}

bool NodeIsNULLFast(const char *expr, xmlNodePtr &node, bool nvl)
{
    if(GetNodeFast(expr, node) == NULL)
        return nvl;
    else
        return NodeIsNULL(NodeAsNodeFast(expr,node));
}

const char* NodeAsStringFast(const char *expr, xmlNodePtr &node, const char* nvl)
{
    if(GetNodeFast(expr, node) == NULL)
        return nvl;
    else
        return NodeAsString(NodeAsNodeFast(expr,node));
}

int NodeAsIntegerFast(const char *expr, xmlNodePtr &node, int nvl)
{
    if(GetNodeFast(expr, node) == NULL)
        return nvl;
    else
        return NodeAsInteger(NodeAsNodeFast(expr,node));
}

double NodeAsFloatFast(const char *expr, xmlNodePtr &node, double nvl)
{
    if(GetNodeFast(expr, node) == NULL)
        return nvl;
    else
        return NodeAsFloat(NodeAsNodeFast(expr,node));
}

TDateTime NodeAsDateTimeFast(const char *expr, xmlNodePtr &node, TDateTime nvl)
{
    if(GetNodeFast(expr, node) == NULL)
        return nvl;
    else
        return NodeAsDateTime(NodeAsNodeFast(expr,node),(char*)ServerFormatDateTimeAsString);
}

xmlNodePtr NewTextChild(xmlNodePtr parent, const string &name, const string &content)
{
    return NewTextChild(parent, name.c_str(), content);
}

xmlNodePtr NewTextChild(xmlNodePtr parent, const char *name, const char *content)
{
  if (name==NULL) return NULL;
  if (content!=NULL&&*content==0) content=NULL;
  return xmlNewTextChild(parent,NULL,BAD_CAST name,BAD_CAST content);
};

xmlNodePtr NewTextChild(xmlNodePtr parent, const char *name, const string content)
{
  return NewTextChild(parent, name, content.c_str());
};

xmlNodePtr NewTextChild(xmlNodePtr parent, const char *name, const int content)
{
  return NewTextChild(parent, name, IntToString(content).c_str());
};

xmlNodePtr NewTextChild(xmlNodePtr parent, const char *name, const double content)
{
  return NewTextChild(parent, name, FloatToString(content).c_str());
};

xmlNodePtr NewTextChild(xmlNodePtr parent, const char *name, const string content, const string nvl)
{
    if(content != nvl)
      return NewTextChild(parent, name, content.c_str());
    else
      return NULL;
};

xmlNodePtr NewTextChild(xmlNodePtr parent, const char *name, const int content, const int nvl)
{
  return NewTextChild(parent, name, IntToString(content), IntToString(nvl));
};

xmlNodePtr NewTextChild(xmlNodePtr parent, const char *name, const double content, const double nvl)
{
  return NewTextChild(parent, name, FloatToString(content), FloatToString(nvl));
};


xmlNodePtr ReplaceTextChild(xmlNodePtr parent, const char *name, const char *content)
{
  xmlNodePtr node;
  if (name==NULL) return NULL;
  node=parent->children;
  while(node!=NULL)
  {
    if (node->name!=NULL&&strcmp((char*)node->name,name)==0) break;
    node=node->next;
  };
  if (content!=NULL&&*content==0) content=NULL;
  if (node!=NULL)
  {
    xmlChar* tmp=xmlEncodeSpecialChars(node->doc,BAD_CAST content);
    xmlNodeSetContent(node,tmp);
    if (tmp!=NULL) xmlFree(tmp);
  }
  else
    node=xmlNewTextChild(parent,NULL,BAD_CAST name,BAD_CAST content);
  return node;
};

xmlNodePtr ReplaceTextChild(xmlNodePtr parent, const char *name, const string content)
{
  return ReplaceTextChild(parent, name, content.c_str());
};

xmlNodePtr ReplaceTextChild(xmlNodePtr parent, const char *name, const int content)
{
  return ReplaceTextChild(parent, name, IntToString(content).c_str());
};

xmlAttrPtr SetProp(xmlNodePtr node, const char *name, const char *value)
{
  if (name==NULL) return NULL;
  if (value==NULL) value="";
  return xmlSetProp(node,BAD_CAST name,BAD_CAST value);
};

xmlAttrPtr SetProp(xmlNodePtr node, const char *name, const string value)
{
  return SetProp(node, name, value.c_str());
};

xmlAttrPtr SetProp(xmlNodePtr node, const char *name, const int value)
{
  return SetProp(node, name, IntToString(value).c_str());
};

//Внимание! Функция с ошибкой libxml2! Не изменяет указатели xmlDocPtr
xmlNodePtr CopyNodeList(xmlNodePtr dest, xmlNodePtr src)
{
  xmlNodePtr node,res;
  if (src==NULL||dest==NULL) return NULL;
  node=xmlCopyNodeList(src->children);
  if (node==NULL) return NULL;
  res=xmlAddChildList(dest,node);
  if (res==NULL) xmlFreeNode(node);
  return res;
};

xmlNodePtr CopyNode(xmlNodePtr dest, xmlNodePtr src, bool recursive)
{
  xmlNodePtr node,res;
  if (src==NULL||dest==NULL) return NULL;
  node=xmlCopyNode(src, (int)recursive);
  if (node==NULL) return NULL;
  res=xmlAddChild(dest,node);
  if (res==NULL) xmlFreeNode(node);
  return res;
};

xmlDocPtr CreateXMLDoc(const char *encoding, const char *root)
{
  xmlDocPtr resDoc=xmlNewDoc(BAD_CAST "1.0");
  if(resDoc==NULL) return NULL;

  xmlFree(const_cast<xmlChar *>(resDoc->encoding));
  resDoc->encoding=xmlStrdup(BAD_CAST encoding);
  if (resDoc->encoding==NULL)
  {
    xmlFreeDoc(resDoc);
    return NULL;
  };
  xmlNodePtr rootNode=xmlNewDocNode(resDoc,NULL,BAD_CAST root,NULL);
  if (rootNode==NULL)
  {
    xmlFreeDoc(resDoc);
    return NULL;
  };
  xmlDocSetRootElement(resDoc,rootNode);
  return resDoc;
};

xmlDocPtr TextToXMLTree( const string& str )
{
  xmlDocPtr res=NULL;
  xmlKeepBlanksDefault(0);
  res=xmlParseMemory(str.c_str(),str.size());
  return res;
};

string XMLTreeToText( xmlDocPtr doc)
{
  char *data2=NULL;
  int datalen=0;
  string res;
  try
  {
    xmlDocDumpFormatMemory(doc,(unsigned char**)&data2,&datalen,1);
    res=data2;
    if (data2!=NULL) xmlFree(data2);
  }
  catch(...)
  {
    if (data2!=NULL) xmlFree(data2);
    throw;
  };
  return res;
};

string GetXMLDocText( xmlDocPtr doc)
{
  return XMLTreeToText(doc);
};

void NodeSetContent(xmlNodePtr cur, const int content)
{
    NodeSetContent(cur, IntToString(content));
}

void NodeSetContent(xmlNodePtr cur, const char* content)
{
    if (cur==NULL) throw EXMLError("Node not defined (NULL)");
    if (content!=NULL&&*content==0) content=NULL;
    xmlChar* tmp=xmlEncodeSpecialChars(cur->doc,BAD_CAST content);
    xmlNodeSetContent(cur,tmp);
    if (tmp!=NULL) xmlFree(tmp);
}

void NodeSetContent(xmlNodePtr cur, const string content)
{
    NodeSetContent(cur,content.c_str());
}

xmlDocPtrCover::xmlDocPtrCover(xmlDocPtr doc)
{
  docPtr=doc;
  /*if (docPtr!=NULL)
    ProgTrace(TRACE5,"xmlDocPtrCover: docPtr=%p",docPtr);*/
};

xmlDocPtrCover::~xmlDocPtrCover()
{
  if (docPtr!=NULL)
  {
    //ProgTrace(TRACE5,"xmlDocPtrCover: xmlFreeDoc(%p)",docPtr);
    xmlFreeDoc(docPtr);
  };
};

XMLDoc::XMLDoc()
{
  docPtrCoverPtr.reset(new xmlDocPtrCover(NULL));
};

XMLDoc::XMLDoc(const char *encoding, const char *root)
{
  set(encoding,root);
};

XMLDoc::XMLDoc(const std::string &text)
{
  set(text);
};

XMLDoc::~XMLDoc()
{
  //
};

xmlDocPtr XMLDoc::docPtr() const
{
  return docPtrCoverPtr->docPtr;
};

void XMLDoc::set(const char *encoding, const char *root)
{
  docPtrCoverPtr.reset(new xmlDocPtrCover(CreateXMLDoc(encoding,root)));
};

void XMLDoc::set(const std::string &text)
{
  if (!text.empty())
    docPtrCoverPtr.reset(new xmlDocPtrCover(TextToXMLTree(text)));
  else
    docPtrCoverPtr.reset(new xmlDocPtrCover(NULL));
};

bool ValidXMLChar( const char c )
{
  return !(c>=0x00 && c<0x20 && c!=0x09 && c!=0x0A && c!=0x0D);
};

bool ValidXMLString( const std::string& str )
{
  for(string::const_iterator i=str.begin(); i!=str.end(); i++)
  {
    if (!ValidXMLChar(*i)) return false;
  };
  return true;
};


