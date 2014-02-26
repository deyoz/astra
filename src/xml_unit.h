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
    EXMLError(const std::string &msg):Exception(msg) {};
};

xmlNodePtr GetNode(const char* expr, xmlDocPtr data, xmlNodePtr cur=NULL);
xmlNodePtr GetNode(const char* expr, xmlNodePtr cur);
void GetNodes(const char* expr, xmlDocPtr data, std::vector<xmlNodePtr>& nodes, xmlNodePtr cur=NULL);
void GetNodes(const char* expr, std::vector<xmlNodePtr>& nodes, xmlNodePtr cur);
xmlNodePtr NodeAsNode(const char* expr, xmlDocPtr data, xmlNodePtr cur=NULL);
xmlNodePtr NodeAsNode(const char* expr, xmlNodePtr cur);
bool NodeIsNULL(xmlNodePtr node);
bool NodeIsNULL(const char* expr, xmlDocPtr data, xmlNodePtr cur=NULL);
bool NodeIsNULL(const char* expr, xmlNodePtr cur);
const char* NodeAsString(xmlNodePtr node);
const char* NodeAsString(const char* expr, xmlDocPtr data, xmlNodePtr cur=NULL);
const char* NodeAsString(const char* expr, xmlNodePtr cur);
int NodeAsInteger(xmlNodePtr node);
int NodeAsInteger(const char* expr, xmlDocPtr data, xmlNodePtr cur=NULL);
int NodeAsInteger(const char* expr, xmlNodePtr cur);
double NodeAsFloat(xmlNodePtr node);
double NodeAsFloat(const char* expr, xmlDocPtr data, xmlNodePtr cur=NULL);
double NodeAsFloat(const char* expr, xmlNodePtr cur);
BASIC::TDateTime NodeAsDateTime(xmlNodePtr node);
BASIC::TDateTime NodeAsDateTime(xmlNodePtr node, const char* format);
BASIC::TDateTime NodeAsDateTime(const char* expr, xmlDocPtr data, const char* format, xmlNodePtr cur=NULL);
BASIC::TDateTime NodeAsDateTime(const char* expr, const char* format, xmlNodePtr cur);
BASIC::TDateTime NodeAsDateTime(const char* expr, xmlNodePtr cur);
//оптимизированные по быстродействию функции
//node - любой брат(сосед) искомого
xmlNodePtr GetNodeFast(const char *expr, xmlNodePtr &node);
xmlNodePtr NodeAsNodeFast(const char *expr, xmlNodePtr &node);
bool NodeIsNULLFast(const char *expr, xmlNodePtr &node);
const char* NodeAsStringFast(const char *expr, xmlNodePtr &node);
int NodeAsIntegerFast(const char *expr, xmlNodePtr &node);
double NodeAsFloatFast(const char *expr, xmlNodePtr &node);
BASIC::TDateTime NodeAsDateTimeFast(const char *expr, const char *format, xmlNodePtr &node);
BASIC::TDateTime NodeAsDateTimeFast(const char *expr, xmlNodePtr &node);

// Если узел не найден, возвращается nvl
bool NodeIsNULLFast(const char *expr, xmlNodePtr &node, bool nvl);
const char* NodeAsStringFast(const char *expr, xmlNodePtr &node, const char* nvl);
int NodeAsIntegerFast(const char *expr, xmlNodePtr &node, int nvl);
double NodeAsFloatFast(const char *expr, xmlNodePtr &node, double nvl);
BASIC::TDateTime NodeAsDateTimeFast(const char *expr, xmlNodePtr &node, BASIC::TDateTime nvl);

const char* NodeAsString(const char* expr, xmlNodePtr cur, const char *nvl);
int NodeAsInteger(const char* expr, xmlNodePtr cur, int nvl);
double NodeAsFloat(const char* expr, xmlNodePtr cur, double nvl);
BASIC::TDateTime NodeAsDateTime(const char* expr, xmlNodePtr cur, BASIC::TDateTime nvl);

// Если content == nvl, узел не создается
xmlNodePtr NewTextChild(xmlNodePtr parent, const char *name, const std::string content, const std::string nvl);
xmlNodePtr NewTextChild(xmlNodePtr parent, const char *name, const int content, const int nvl);
xmlNodePtr NewTextChild(xmlNodePtr parent, const char *name, const double content, const double nvl);

void NodeSetContent(xmlNodePtr cur, const char* content);
void NodeSetContent(xmlNodePtr cur, const std::string content);
void NodeSetContent(xmlNodePtr cur, const int content);
xmlNodePtr NewTextChild(xmlNodePtr parent, const char *name, const char *content = NULL);
xmlNodePtr NewTextChild(xmlNodePtr parent, const std::string &name, const std::string &content);
xmlNodePtr NewTextChild(xmlNodePtr parent, const char *name, const std::string content);
xmlNodePtr NewTextChild(xmlNodePtr parent, const char *name, const int content);
xmlNodePtr NewTextChild(xmlNodePtr parent, const char *name, const double content);
xmlNodePtr ReplaceTextChild(xmlNodePtr parent, const char *name, const char *content = NULL);
xmlNodePtr ReplaceTextChild(xmlNodePtr parent, const char *name, const std::string content);
xmlNodePtr ReplaceTextChild(xmlNodePtr parent, const char *name, const int content);
xmlAttrPtr SetProp(xmlNodePtr node, const char *name, const char *value = NULL);
xmlAttrPtr SetProp(xmlNodePtr node, const char *name, const std::string value);
xmlAttrPtr SetProp(xmlNodePtr node, const char *name, const int value);
xmlNodePtr CopyNodeList(xmlNodePtr dest, xmlNodePtr src); //Функция с ошибкой libxml2! Не изменяет указатели xmlDocPtr
xmlNodePtr CopyNode(xmlNodePtr dest, xmlNodePtr src, bool recursive=true);

void SetXMLDocEncoding(xmlDocPtr doc, const char *encoding);
xmlDocPtr CreateXMLDoc(const char *root);
xmlDocPtr TextToXMLTree( const std::string& str );
std::string XMLTreeToText( xmlDocPtr doc);
std::string GetXMLDocText( xmlDocPtr doc);

class xmlDocPtrCover
{
  public:
    xmlDocPtr docPtr;
    xmlDocPtrCover(xmlDocPtr doc);
    ~xmlDocPtrCover();
};

class XMLDoc
{
  private:
    boost::shared_ptr<xmlDocPtrCover> docPtrCoverPtr;
  public:
    XMLDoc();
    XMLDoc(const char *root);
    XMLDoc(const std::string &text);
    ~XMLDoc();
    xmlDocPtr docPtr() const;
    void set(const char *root);
    void set(const std::string &text);
};

bool ValidXMLChar( const char c );
bool ValidXMLString( const std::string& str );

#endif
