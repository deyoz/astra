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
//��⨬���஢���� �� ����த���⢨� �㭪樨
//node - �� ���(�ᥤ) �᪮����
xmlNodePtr GetNodeFast(char *expr, xmlNodePtr &node);
xmlNodePtr NodeAsNodeFast(char *expr, xmlNodePtr &node);
bool NodeIsNULLFast(char *expr, xmlNodePtr &node);
char* NodeAsStringFast(char *expr, xmlNodePtr &node);
int NodeAsIntegerFast(char *expr, xmlNodePtr &node);
double NodeAsFloatFast(char *expr, xmlNodePtr &node);
BASIC::TDateTime NodeAsDateTimeFast(char *expr, char *format, xmlNodePtr &node);
BASIC::TDateTime NodeAsDateTimeFast(char *expr, xmlNodePtr &node);

// �᫨ 㧥� �� ������, �����頥��� nvl
bool NodeIsNULLFast(char *expr, xmlNodePtr &node, bool nvl);
char* NodeAsStringFast(char *expr, xmlNodePtr &node, char* nvl);
int NodeAsIntegerFast(char *expr, xmlNodePtr &node, int nvl);
double NodeAsFloatFast(char *expr, xmlNodePtr &node, double nvl);
BASIC::TDateTime NodeAsDateTimeFast(char *expr, xmlNodePtr &node, BASIC::TDateTime nvl);

char* NodeAsString(char* expr, xmlNodePtr cur, char *nvl);
int NodeAsInteger(char* expr, xmlNodePtr cur, int nvl);
double NodeAsFloat(char* expr, xmlNodePtr cur, double nvl);
BASIC::TDateTime NodeAsDateTime(char* expr, xmlNodePtr cur, BASIC::TDateTime nvl);

// �᫨ content == nvl, 㧥� �� ᮧ������
xmlNodePtr NewTextChild(xmlNodePtr parent, const char *name, const std::string content, const std::string nvl);
xmlNodePtr NewTextChild(xmlNodePtr parent, const char *name, const int content, const int nvl);
xmlNodePtr NewTextChild(xmlNodePtr parent, const char *name, const double content, const double nvl);

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
int PropAsInteger(char* expr, xmlNodePtr cur);
xmlAttrPtr SetProp(xmlNodePtr node, const char *name, const char *value = NULL);
xmlAttrPtr SetProp(xmlNodePtr node, const char *name, const std::string value);
xmlAttrPtr SetProp(xmlNodePtr node, const char *name, const int value);
xmlNodePtr CopyNodeList(xmlNodePtr dest, xmlNodePtr src); //�㭪�� � �訡��� libxml2! �� ������� 㪠��⥫� xmlDocPtr
xmlNodePtr CopyNode(xmlNodePtr dest, xmlNodePtr src, bool recursive=true);

xmlDocPtr CreateXMLDoc(const char *encoding, const char *root);
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
    XMLDoc(const char *encoding, const char *root);
    XMLDoc(const std::string &text);
    ~XMLDoc();
    xmlDocPtr docPtr() const;
    void set(const char *encoding, const char *root);
    void set(const std::string &text);
};

bool ValidXMLString( const std::string& str );

#endif
