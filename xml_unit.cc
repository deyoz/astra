#include <vector>
#include <string>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include "xml_unit.h"
#include "basic.h"
#include "stl_utils.h"

using namespace std;
using namespace BASIC;

char* NodeContent(xmlNodePtr node)
{
  char *res=NULL;
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

xmlNodePtr find_node(char* expr, xmlDocPtr doc, xmlNodePtr cur=NULL)
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

xmlNodePtr GetNode(char* expr, xmlDocPtr data, xmlNodePtr cur)
{
  return find_node(expr,data,cur);
};

xmlNodePtr GetNode(char* expr, xmlNodePtr cur)
{
  if (cur==NULL)
    return find_node(expr,NULL,cur);
  else
    return find_node(expr,cur->doc,cur);
};

void GetNodes(char* expr, xmlDocPtr data, vector<xmlNodePtr>& nodes, xmlNodePtr cur)
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

void GetNodes(char* expr, vector<xmlNodePtr>& nodes, xmlNodePtr cur)
{
  if (cur==NULL)
    GetNodes(expr,NULL,nodes,cur);
  else
    GetNodes(expr,cur->doc,nodes,cur);
};

xmlNodePtr NodeAsNode(char* expr, xmlDocPtr data, xmlNodePtr cur)
{
  xmlNodePtr node;
  node=find_node(expr,data,cur);
  if (node==NULL)
    throw EXMLError("Node '%s' does not exists",expr);
  return node;
};

xmlNodePtr NodeAsNode(char* expr, xmlNodePtr cur)
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

bool NodeIsNULL(char* expr, xmlDocPtr data, xmlNodePtr cur)
{
  xmlNodePtr node;
  node=find_node(expr,data,cur);
  if (node==NULL)
    throw EXMLError("Node '%s' does not exists",expr);
  return strcmp(NodeContent(node),"")==0;
};

bool NodeIsNull(char* expr, xmlNodePtr cur)
{
  if (cur==NULL)
    return NodeIsNULL(expr,NULL,cur);
  else
    return NodeIsNULL(expr,cur->doc,cur);
};

char* NodeAsString(xmlNodePtr node)
{
  if (node==NULL) throw EXMLError("Node not defined (NULL)");
  return NodeContent(node);
};

char* NodeAsString(char* expr, xmlDocPtr data, xmlNodePtr cur)
{
  xmlNodePtr node;
  node=find_node(expr,data,cur);
  if (node==NULL)
    throw EXMLError("Node '%s' does not exists",expr);
  return NodeContent(node);
};

char* NodeAsString(char* expr, xmlNodePtr cur)
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

int NodeAsInteger(char* expr, xmlDocPtr data, xmlNodePtr cur)
{
  int Value;
  xmlNodePtr node;
  node=find_node(expr,data,cur);
  if (node==NULL)
    throw EXMLError("Node '%s' does not exists",expr);
  if ( StrToInt( NodeContent(node), Value ) == EOF )
    throw EXMLError("Cannot convert node '%s' to an Integer",expr);
  return Value;
};

int NodeAsInteger(char* expr, xmlNodePtr cur)
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

double NodeAsFloat(char* expr, xmlDocPtr data, xmlNodePtr cur)
{
  double Value;
  xmlNodePtr node;
  node=find_node(expr,data,cur);
  if (node==NULL)
    throw EXMLError("Node '%s' does not exists",expr);
  if ( StrToFloat( NodeContent(node), Value ) == EOF )
    throw EXMLError("Cannot convert node '%s' to an Float",expr);
  return Value;
};

double NodeAsFloat(char* expr, xmlNodePtr cur)
{
  if (cur==NULL)
    return NodeAsFloat(expr,NULL,cur);
  else
    return NodeAsFloat(expr,cur->doc,cur);
};

TDateTime NodeAsDateTime(xmlNodePtr node, char* format)
{
  TDateTime Value;
  if (node==NULL) throw EXMLError("Node not defined (NULL)");
  if ( StrToDateTime( NodeContent(node), format, Value ) == EOF )
    throw EXMLError("Cannot convert node to an DateTime");
  return Value;
};

TDateTime NodeAsDateTime(char* expr, xmlDocPtr data, char* format, xmlNodePtr cur)
{
  TDateTime Value;
  xmlNodePtr node;
  node=find_node(expr,data,cur);
  if (node==NULL)
    throw EXMLError("Node '%s' does not exists",expr);
  if ( StrToDateTime( NodeContent(node), format, Value ) == EOF )
    throw EXMLError("Cannot convert node '%s' to an DateTime",expr);
  return Value;
};

TDateTime NodeAsDateTime(char* expr, char* format, xmlNodePtr cur)
{
  if (cur==NULL)
    return NodeAsDateTime(expr,NULL,format,cur);
  else
    return NodeAsDateTime(expr,cur->doc,format,cur);
};

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
    xmlNodeSetContent(node,BAD_CAST content);
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

string GetXMLDocText( xmlDocPtr doc)
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
