#ifndef XML_UNITH
#define XML_UNITH

#include <vector>
#include <string>
#include <libxml/parser.h>
#include "basic.h"
#include "exceptions.h"

class EXMLError: public EXCEPTIONS::Exception
{
  public:
	EXMLError(const char *format, ...): EXCEPTIONS::Exception("") {
    	va_list ap;
    	va_start(ap, format);
    	vsnprintf(Message, sizeof(Message), format, ap);
        Message[sizeof(Message)-1]=0;
    	va_end(ap);
    };
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
BASIC::TDateTime NodeAsDateTime(xmlNodePtr node, char* format);
BASIC::TDateTime NodeAsDateTime(char* expr, xmlDocPtr data, char* format, xmlNodePtr cur=NULL);
BASIC::TDateTime NodeAsDateTime(char* expr, char* format, xmlNodePtr cur);

xmlNodePtr NewTextChild(xmlNodePtr parent, const char *name, const char *content = NULL);
xmlNodePtr NewTextChild(xmlNodePtr parent, const char *name, const std::string content);
xmlNodePtr NewTextChild(xmlNodePtr parent, const char *name, const int content);
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
