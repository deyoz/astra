#ifndef XML_UNITH
#define XML_UNITH

#include <vector>
#include <string>
#include <libxml/parser.h>
#include "setup.h"
#include "basic.h"
#include "exceptions.h"

class EXMLError: public EXCEPTIONS::Exception
{
  public:
    EXMLError(const std::string &msg):Exception(msg) {};	
};

xmlNodePtr GetNode(char* expr, xmlDocPtr data, xmlNodePtr cur=NULL);
xmlNodePtr GetNode(char* expr, xmlNodePtr cur);
void GetNodes(char* expr, xmlDocPtr data, std::vector<xmlNodePtr>& nodes, xmlNodePtr cur=NULL);
void GetNodes(char* expr, std::vector<xmlNodePtr>& nodes, xmlNodePtr cur);
xmlNodePtr NodeAsNode(char* expr, xmlDocPtr data, xmlNodePtr cur=NULL);
xmlNodePtr NodeAsNode(char* expr, xmlNodePtr cur);
bool NodeIsNULL(xmlNodePtr node);
bool NodeIsNULL(char* expr, xmlDocPtr data, xmlNodePtr cur=NULL);
bool NodeIsNULL(char* expr, xmlNodePtr cur);
char* NodeAsString(xmlNodePtr node);
char* NodeAsString(char* expr, xmlDocPtr data, xmlNodePtr cur=NULL);
char* NodeAsString(char* expr, xmlNodePtr cur);
int NodeAsInteger(xmlNodePtr node);
int NodeAsInteger(char* expr, xmlDocPtr data, xmlNodePtr cur=NULL);
int NodeAsInteger(char* expr, xmlNodePtr cur);
double NodeAsFloat(xmlNodePtr node);
double NodeAsFloat(char* expr, xmlDocPtr data, xmlNodePtr cur=NULL);
double NodeAsFloat(char* expr, xmlNodePtr cur);
BASIC::TDateTime NodeAsDateTime(xmlNodePtr node);
BASIC::TDateTime NodeAsDateTime(xmlNodePtr node, char* format);
BASIC::TDateTime NodeAsDateTime(char* expr, xmlDocPtr data, char* format, xmlNodePtr cur=NULL);
BASIC::TDateTime NodeAsDateTime(char* expr, char* format, xmlNodePtr cur);
BASIC::TDateTime NodeAsDateTime(char* expr, xmlNodePtr cur);
//оптимизированные по быстродействию функции
//node - любой брат(сосед) искомого 
xmlNodePtr GetNodeFast(char *expr, xmlNodePtr &node);
xmlNodePtr NodeAsNodeFast(char *expr, xmlNodePtr &node);
bool NodeIsNULLFast(char *expr, xmlNodePtr &node);
char* NodeAsStringFast(char *expr, xmlNodePtr &node);
int NodeAsIntegerFast(char *expr, xmlNodePtr &node);
double NodeAsFloatFast(char *expr, xmlNodePtr &node);
BASIC::TDateTime NodeAsDateTimeFast(char *expr, char *format, xmlNodePtr &node);
BASIC::TDateTime NodeAsDateTimeFast(char *expr, xmlNodePtr &node);

void NodeSetContent(xmlNodePtr cur, const char* content);
void NodeSetContent(xmlNodePtr cur, const std::string content);
void NodeSetContent(xmlNodePtr cur, const int content);
xmlNodePtr NewTextChild(xmlNodePtr parent, const char *name, const char *content = NULL);
xmlNodePtr NewTextChild(xmlNodePtr parent, const char *name, const std::string content);
xmlNodePtr NewTextChild(xmlNodePtr parent, const char *name, const int content);
xmlNodePtr NewTextChild(xmlNodePtr parent, const char *name, const double content);
xmlNodePtr ReplaceTextChild(xmlNodePtr parent, const char *name, const char *content = NULL);
xmlNodePtr ReplaceTextChild(xmlNodePtr parent, const char *name, const std::string content);
xmlNodePtr ReplaceTextChild(xmlNodePtr parent, const char *name, const int content);
xmlAttrPtr SetProp(xmlNodePtr node, const char *name, const char *value = NULL);
xmlAttrPtr SetProp(xmlNodePtr node, const char *name, const std::string value);
xmlAttrPtr SetProp(xmlNodePtr node, const char *name, const int value);
xmlNodePtr CopyNodeList(xmlNodePtr dest, xmlNodePtr src);
xmlNodePtr CopyNode(xmlNodePtr dest, xmlNodePtr src, bool recursive=true);

std::string GetXMLDocText( xmlDocPtr doc);

#endif
