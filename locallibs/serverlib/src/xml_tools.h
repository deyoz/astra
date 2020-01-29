#ifndef __XML_TOOLS_H__
#define __XML_TOOLS_H__

#include <libxml/tree.h>
#include <libxml/xpath.h>

#ifdef __cplusplus

class XmlDoc;

#include <string>
extern "C"
{
#endif /* __cplusplus */

/*****************************************************************************/
/***************              ���� 㧫��                      ***************/
/*****************************************************************************/

xmlNodePtr findNode(xmlNodePtr parentNode, const char *name_or_simple_path);
/* �����頥� 㪠��⥫� �� 㧥� � ������� ������, ����騩 ����� parentNode */
/* �������� �ਬ�⨢�� ���� ���� term/query/node_to_find. ���� �������     */
/* ��稭�� � �����।�⢥���� ��⮬��� parentNode.                           */
/* �� ����� �᪮�� 㧥�, �����頥� NULL.                                   */

xmlNodePtr findNodeR(xmlNodePtr parentNode, const char *name);
/* �����頥� 㪠��⥫� �� 㧥� � ������ name, ����騩 ����� parentNode.    */
/* ���� ������� ४��ᨢ��, ��稭�� � �����।�⢥���� ��⮬��� parentNode. */
/* �� ����� �᪮�� 㧥�, �����頥� NULL.                                   */

xmlNodePtr getNode(xmlNodePtr parentNode, const char *name_or_simple_path);
/* �����頥� 㪠��⥫� �� 㧥� � ������� ������, ����騩 ����� parentNode */
/* �������� �ਬ�⨢�� ���� ���� term/query/node_to_find. ���� �������     */
/* ��稭�� � �����।�⢥���� ��⮬��� parentNode.                           */
/* �� ����� �᪮�� 㧥�, ᮧ���� ���, �᫨ ����� �����/��� ����� 99,      */
/* ���� - ��ᠥ� EXCEPTIONS::Exception.                                    */

xmlNodePtr fnode(xmlNodePtr parentNode, const char *pathname);
/* �����頥� 㪠��⥫� �� 㧥� � ������� ������, ����騩 ����� parentNode */
/* �������� ���� ���� term/query/tab[4]/node_to_find. ���� ������� ��稭��  */
/* � �����।�⢥���� ��⮬��� parentNode.                                   */
/* �� ����� �᪮�� 㧥�, �����頥� NULL.                                   */

xmlNodePtr findIChild(xmlNodePtr parentNode, int ind);
/* �����頥� 㪠��⥫� �� �����।�⢥����� ��⮬�� 㧫� parentNode c ��� */
/* ������ � ������� ��ਡ�⮬ index � ���祭��� ind.                       */
/* �� ����� �᪮�� 㧥�, �����頥� NULL.                                   */

xmlNodePtr findNodeByProp(xmlNodePtr parentNode, const char *prop,
                          const char *val);
/* �����頥� 㪠��⥫� �� �����।�⢥����� ��⮬�� 㧫� parentNode c ��� */
/* ������ � ������� ��ਡ�⮬ prop � ���祭��� val.                        */
/* �� ����� �᪮�� 㧥�, �����頥� NULL.                                   */

xmlNodePtr findNamedNodeByProp(xmlNodePtr parentNode, const char *name,
                               const char *prop, const char *val);
/* �����頥� 㪠��⥫� �� �����।�⢥����� ��⮬�� 㧫� parentNode c       */
/* ������ name � ������� ��ਡ�⮬ prop � ���祭��� val.                   */
/* �� ����� �᪮�� 㧥�, �����頥� NULL.                                   */

xmlNodePtr findNamedNodeByPropR(xmlNodePtr parentNode, const char *name,
                                const char *prop, const char *val);
/* �����頥� 㪠��⥫� �� ��⮬�� (�� ���������) 㧫� parentNode c      */
/* ������ name � ������� ��ਡ�⮬ prop � ���祭��� val.                   */
/* �� ����� �᪮�� 㧥�, �����頥� NULL.                                   */

xmlNodePtr findNext(xmlNodePtr indexedNode);
/* �����頥� 㪠��⥫� �� 㧥�, �ᯮ������� �� ����� �஢�� �             */
/* indexedNode, � ����騩 ������� ��ਡ�� index � ���祭��� �� �������     */
/* ����� ���祭�� ��ਡ�� index 㧫� indexedNode.                          */
/* �� ����� �᪮�� 㧥� ��� ��ਡ�� index 㧫� indexedNode, �����頥� NULL.*/

/*****************************************************************************/
/***************        �㭪樨 ��� ����䨪�樨 ������         ***************/
/*****************************************************************************/

xmlNodePtr newChild(xmlNodePtr parentNode, const char *newNodeName);
/* ������� ���⮩ 㧥� � ������ newNodeName ����� parentNode � �����頥�   */
/* 㪠��⥫� �� ����. �����頥� NULL, �᫨ parentNode ��� newNodeName - 0.  */

xmlAttrPtr xmlSetPropInt(xmlNodePtr resNode, const char *prop_name, int value);
/* ������� (��� ������� ���祭��) ��ਡ�� prop_name 㧫� resNode, �         */
/* ⥪�⮢� ���祭���, ᮤ�ঠ騬 �������� ������ �᫠ value.            */
/* �����頥� 㪠��⥫� �� ᮮ⢥�����騩 ��ਡ��.                          */

void xmlNodeSetContentInt(xmlNodePtr resNode, int val);
/* ������� ⥪�⮢�� ���祭�� 㧫� resNode �� ��ப�, ᮤ�ঠ��� �������� */
/* ������ �᫠ val.                                                         */

void xmlClearNode(xmlNodePtr node);
/* ������ �� ᮤ�ন��� ��࠭��� ���� (unlink & free) */

/*****************************************************************************/
/***************        C-�㭪樨 ��� ����祭�� ������         ***************/
/*****************************************************************************/

// use xml_parse_memory_CP866()
xmlDocPtr make_tree(const char *str, int str_len) __attribute__ ((deprecated));
/* ������� XML-���㬥�� �� ��ப� str ������ str_len � ������頥� 㪠��⥫� */
/* �� ����. � ��砥 �訡�� XML-����� �����頥� NULL.                     */

/* �����뢠�� ��ப���� �।�⠢����� XML-��ॢ� reqDoc �� ����� *str.      */
/* � ��६����� *str_len ������ ����� ��ப�.                               */
/* ��������: ������, �뤥������ ��� ��ப�, ����室��� �᢮������ xmlFree()  */
// use xml_dump()
void make_str(xmlDocPtr reqDoc, char **str, int *str_len) __attribute__ ((deprecated));
void make_format_str(xmlDocPtr reqDoc, char **str, int *str_len) __attribute__ ((deprecated));

const char *getText(const xmlNodePtr dataNode);
/* �����頥� 㪠��⥫� �� ⥪�⮢�� ᮤ�ন��� ��ࢮ�� ���������� �         */
/* dataNode 㧫�, �᫨ �� - ⥪�⮢� �㦥���. ����, �����頥� NULL.     */

const char *getprop(const xmlNodePtr resNode, const char *prop);
/* �����頥� 㪠��⥫� �� ⥪�⮢�� ᮤ�ন��� ��ਡ�� � ������ prop 㧫�  */
/* dataNode. �� ������⢨� �᪮���� ��ਡ��, �����頥� NULL.              */

int xmlGetPropInt(const xmlNodePtr reqNode, const char *prop_name, int *res);

int istrue(const xmlNodePtr boolNode);
/* �����頥� 1, �᫨ ᮤ�ন��� boolNode - ��ப� "true", 0 - ����.        */

int isfalse(const xmlNodePtr boolNode);
/* �����頥� 1, �᫨ ᮤ�ন��� boolNode - ��ப� "false", 0 - ����.       */

int isempty(const xmlNodePtr dataNode);
/* �����頥� 1, �᫨ ⥪�⮢�� ᮤ�ন��� ��ࢮ�� ���������� � dataNode     */
/* ⥪�⮢��� �㦥����� 㧫� - ���⮥, ��� �� �� ⥪�⮢� 㧥�.           */
/* ����, �����頥� 0.                                                      */

int istext(const xmlNodePtr reqNode);
/* �����頥� 1, �᫨ ���� �������� � dataNode 㧥� - ⥪�⮢� �㦥���.*/
/* ����, �����頥� 0.                                                      */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#ifdef __cplusplus

std::string make_str(const XmlDoc& );
std::string make_format_str(const XmlDoc& );

bool xmlbool(const xmlNodePtr reqNode, bool dflt);

inline xmlNodePtr findNode(const xmlNodePtr parentNode, const std::string &path)
{
    return findNode(parentNode, path.c_str());
}

inline xmlNodePtr findNodeR(const xmlNodePtr parentNode, const std::string &name)
{
    return findNodeR(parentNode, name.c_str());
}

std::string formatXmlString(const std::string &xml_in);
/* �����頥� �ଠ�஢����� ��ப� c XML-��ॢ��, ����ᠭ�� � str         */
/* ����� ॠ�������� �१ ����஥��� ��ॢ� (�᫨ ��ப� ������ -         */
/* ���� ���� �ந�����⥫쭮��).                                         */

xmlNodePtr xmlNewBoolChild(xmlNodePtr parentNode, const char *name, bool value);
inline xmlNodePtr xmlNewBoolChild(xmlNodePtr parentNode, const std::string &name, bool value)
{
  return xmlNewBoolChild(parentNode,name.c_str(),value);
}

void xmlNewBoolProp(xmlNodePtr node, const char *name, bool value);
inline void xmlNewBoolProp(xmlNodePtr node, const std::string &name, bool value)
{
  return xmlNewBoolProp(node,name.c_str(),value);
}

/*****************************************************************************/
/***************       C++-�㭪樨 ��� ����祭�� ������        ***************/
/*****************************************************************************/

XmlDoc xml_parse_memory_CP866(const char* str, size_t len);
XmlDoc xml_parse_memory_CP866(const char* str);
XmlDoc xml_parse_memory_CP866(const std::string& str);
// ������� XML-���㬥�� �� ��ப� str
// ��ப� � ����஢�� UTF-8, � ��������� ������� UTF-8
// �� ���� १������饣� XML-��ॢ� ��४���஢��� � ��866 
//   (�᫨ � XmlContext �� ��⠭������ donotencode2UTF8)

// use xml_parse_memory_CP866()
xmlDocPtr makeXmlTreeFromStr(const std::string &str) __attribute__ ((deprecated));
/* ������� XML-���㬥�� �� ��ப� str � ������頥� 㪠��⥫� �� ����.       */
/* � ��砥 �訡�� XML-����� �����頥� NULL.                              */

std::string makeStrFromXmlTree(const xmlDocPtr reqDoc) __attribute__ ((deprecated));
std::string makeFormatStrFromXmlTree(const xmlDocPtr reqDoc) __attribute__ ((deprecated));
/* �����頥� ��ப���� �।�⠢����� XML-��ॢ� reqDoc.                     */

const char *gettext(const xmlNodePtr reqNode);
/* �����頥� 㪠��⥫� �� ⥪�⮢�� ᮤ�ন��� ��ࢮ�� ���������� �         */
/* dataNode 㧫�, �᫨ �� - ⥪�⮢� �㦥���. ����, �����頥� NULL.     */

std::string getStrFromXml(const xmlNodePtr dataNode);
/* �����頥� STL-string, ����஥��� �� ⥪�⮢�� ᮤ�ন��� 㧫� dataNode, */
/* �᫨ ���� �������� � dataNode 㧥� - ⥪�⮢� �㦥���. ����,       */
/* �����頥� ������ ��ப�.                                                 */

inline std::string getStrFromXml(const xmlNodePtr parentNode, const std::string &child_name)
{
  return getStrFromXml(child_name.empty()?parentNode:findNode(parentNode,child_name.c_str()));
}

std::string getTrimmedStrFromXml(const xmlNodePtr dataNode);
/* �����頥� STL-string, ����஥��� �� ⥪�⮢�� ᮤ�ন��� 㧫� dataNode, */
/* �᫨ ���� �������� � dataNode 㧥� - ⥪�⮢� �㦥���. ����,       */
/* �����頥� ������ ��ப�. ������ isspace-ᨬ���� �� ����� ���栬 ��ப�. */

inline std::string getTrimmedStrFromXml(const xmlNodePtr parentNode, const std::string &child_name)
{
  return getTrimmedStrFromXml(child_name.empty()?parentNode:findNode(parentNode,child_name.c_str()));
}

std::string getStrPropFromXml(const xmlNodePtr dataNode, const char *prop);
std::string getStrPropFromXml(const xmlNodePtr dataNode, const std::string &prop);
/* �����頥� STL-string, ����஥��� �� ⥪�⮢�� ᮤ�ন��� ��ਡ�� prop  */
/* 㧫� dataNode. �� ������⢨� �᪮���� ��ਡ��, �����頥� ������ ��ப�.*/

int getIntFromXml(const xmlNodePtr dataNode, int err_value=0);
inline int getIntFromXml(const xmlNodePtr parentNode, const std::string &child_name, int err_value=0)
{
  return getIntFromXml(child_name.empty()?parentNode:findNode(parentNode,child_name.c_str()),err_value);
}
int getIntPropFromXml(const xmlNodePtr dataNode, const std::string &prop, int err_value=0);

double getDblPropFromXml(const xmlNodePtr dataNode, const std::string &prop, double err_value=0.);
double getDblFromXml(const xmlNodePtr dataNode, double err_value = 0.0);


std::string getCDataFromXml(xmlNodePtr node);
inline std::string getCDataFromXml(xmlNodePtr node, const std::string &child_name)
{
  return getCDataFromXml(child_name.empty()?node:findNode(node,child_name));
}
                                   

/*****************************************************************************/
/***************        C++-�㭪樨 ��� ���᪠ ������          ***************/
/*****************************************************************************/

xmlXPathObjectPtr getNodeSetByXPath(xmlNodePtr, const std::string&); 
xmlNodePtr getFirstNodeByXPath(xmlNodePtr, const std::string&);


#endif /* __cplusplus */

#endif /* __XML_TOOLS_H__ */
