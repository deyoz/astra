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
/***************              Поиск узлов                      ***************/
/*****************************************************************************/

xmlNodePtr findNode(xmlNodePtr parentNode, const char *name_or_simple_path);
/* Возвращает указатель на узел с заданным именем, лежащий внутри parentNode */
/* Понимает примитивный путь вида term/query/node_to_find. Поиск ведется     */
/* начиная с непосредственных потомков parentNode.                           */
/* Не найдя искомый узел, возвращает NULL.                                   */

xmlNodePtr findNodeR(xmlNodePtr parentNode, const char *name);
/* Возвращает указатель на узел с именем name, лежащий внутри parentNode.    */
/* Поиск ведется рекурсивно, начиная с непосредственных потомков parentNode. */
/* Не найдя искомый узел, возвращает NULL.                                   */

xmlNodePtr getNode(xmlNodePtr parentNode, const char *name_or_simple_path);
/* Возвращает указатель на узел с заданным именем, лежащий внутри parentNode */
/* Понимает примитивный путь вида term/query/node_to_find. Поиск ведется     */
/* начиная с непосредственных потомков parentNode.                           */
/* Не найдя искомый узел, создает его, если длина имени/пути меньше 99,      */
/* иначе - бросает EXCEPTIONS::Exception.                                    */

xmlNodePtr fnode(xmlNodePtr parentNode, const char *pathname);
/* Возвращает указатель на узел с заданным именем, лежащий внутри parentNode */
/* Понимает путь вида term/query/tab[4]/node_to_find. Поиск ведется начиная  */
/* с непосредственных потомков parentNode.                                   */
/* Не найдя искомый узел, возвращает NULL.                                   */

xmlNodePtr findIChild(xmlNodePtr parentNode, int ind);
/* Возвращает указатель на непосредственного потомка узла parentNode c любым */
/* именем и заданным атрибутом index со значением ind.                       */
/* Не найдя искомый узел, возвращает NULL.                                   */

xmlNodePtr findNodeByProp(xmlNodePtr parentNode, const char *prop,
                          const char *val);
/* Возвращает указатель на непосредственного потомка узла parentNode c любым */
/* именем и заданным атрибутом prop со значением val.                        */
/* Не найдя искомый узел, возвращает NULL.                                   */

xmlNodePtr findNamedNodeByProp(xmlNodePtr parentNode, const char *name,
                               const char *prop, const char *val);
/* Возвращает указатель на непосредственного потомка узла parentNode c       */
/* именем name и заданным атрибутом prop со значением val.                   */
/* Не найдя искомый узел, возвращает NULL.                                   */

xmlNodePtr findNamedNodeByPropR(xmlNodePtr parentNode, const char *name,
                                const char *prop, const char *val);
/* Возвращает указатель на потомка (любой вложености) узла parentNode c      */
/* именем name и заданным атрибутом prop со значением val.                   */
/* Не найдя искомый узел, возвращает NULL.                                   */

xmlNodePtr findNext(xmlNodePtr indexedNode);
/* Возвращает указатель на узел, расположенный на одном уровне с             */
/* indexedNode, и имеющий заданный атрибут index со значением на единицу     */
/* больше значения атрибута index узла indexedNode.                          */
/* Не найдя искомый узел или атрибут index узла indexedNode, возвращает NULL.*/

/*****************************************************************************/
/***************        Функции для модификации данных         ***************/
/*****************************************************************************/

xmlNodePtr newChild(xmlNodePtr parentNode, const char *newNodeName);
/* Создает пустой узел с именем newNodeName внутри parentNode и возвращает   */
/* указатель на него. Возвращает NULL, если parentNode или newNodeName - 0.  */

xmlAttrPtr xmlSetPropInt(xmlNodePtr resNode, const char *prop_name, int value);
/* Создает (или изменяет значение) атрибут prop_name узла resNode, с         */
/* текстовым значением, содержащим десятичную запись числа value.            */
/* Возвращает указатель на соответствующий атрибут.                          */

void xmlNodeSetContentInt(xmlNodePtr resNode, int val);
/* Изменяет текстовое значение узла resNode на строку, содержащую десятичную */
/* запись числа val.                                                         */

void xmlClearNode(xmlNodePtr node);
/* Удаляет все содержимое выбранной ноды (unlink & free) */

/*****************************************************************************/
/***************        C-Функции для получения данных         ***************/
/*****************************************************************************/

// use xml_parse_memory_CP866()
xmlDocPtr make_tree(const char *str, int str_len) __attribute__ ((deprecated));
/* Создает XML-документ из строки str длиной str_len и воозвращает указатель */
/* на него. В случае ошибки XML-парсера возвращает NULL.                     */

/* Записывает строковое представление XML-дерева reqDoc по адресу *str.      */
/* В переменную *str_len пишется длина строки.                               */
/* ВНИМАНИЕ: память, выделенную под строку, необходимо освободить xmlFree()  */
// use xml_dump()
void make_str(xmlDocPtr reqDoc, char **str, int *str_len) __attribute__ ((deprecated));
void make_format_str(xmlDocPtr reqDoc, char **str, int *str_len) __attribute__ ((deprecated));

const char *getText(const xmlNodePtr dataNode);
/* Возвращает указатель на текстовое содержимое первого вложенного в         */
/* dataNode узла, если он - текстовый служебный. Иначе, возвращает NULL.     */

const char *getprop(const xmlNodePtr resNode, const char *prop);
/* Возвращает указатель на текстовое содержимое атрибута с именем prop узла  */
/* dataNode. При отсутствии искомого атрибута, возвращает NULL.              */

int xmlGetPropInt(const xmlNodePtr reqNode, const char *prop_name, int *res);

int istrue(const xmlNodePtr boolNode);
/* Возвращает 1, если содержимое boolNode - строка "true", 0 - иначе.        */

int isfalse(const xmlNodePtr boolNode);
/* Возвращает 1, если содержимое boolNode - строка "false", 0 - иначе.       */

int isempty(const xmlNodePtr dataNode);
/* Возвращает 1, если текстовое содержимое первого вложенного в dataNode     */
/* текстового служебного узла - пустое, или это не текстовый узел.           */
/* Иначе, возвращает 0.                                                      */

int istext(const xmlNodePtr reqNode);
/* Возвращает 1, если первый вложенный в dataNode узел - текстовый служебный.*/
/* Иначе, возвращает 0.                                                      */

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
/* Возвращает форматированную строку c XML-деревом, записанным в str         */
/* Сейчас реализовано через построение дерева (если строка большая -         */
/* будут потери производительности).                                         */

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
/***************       C++-Функции для получения данных        ***************/
/*****************************************************************************/

XmlDoc xml_parse_memory_CP866(const char* str, size_t len);
XmlDoc xml_parse_memory_CP866(const char* str);
XmlDoc xml_parse_memory_CP866(const std::string& str);
// Создает XML-документ из строки str
// Строка в кодировке UTF-8, в заголовке значится UTF-8
// Все ноды результирующего XML-дерева перекодированы в СР866 
//   (если в XmlContext не установлено donotencode2UTF8)

// use xml_parse_memory_CP866()
xmlDocPtr makeXmlTreeFromStr(const std::string &str) __attribute__ ((deprecated));
/* Создает XML-документ из строки str и воозвращает указатель на него.       */
/* В случае ошибки XML-парсера возвращает NULL.                              */

std::string makeStrFromXmlTree(const xmlDocPtr reqDoc) __attribute__ ((deprecated));
std::string makeFormatStrFromXmlTree(const xmlDocPtr reqDoc) __attribute__ ((deprecated));
/* Возвращает строковое представление XML-дерева reqDoc.                     */

const char *gettext(const xmlNodePtr reqNode);
/* Возвращает указатель на текстовое содержимое первого вложенного в         */
/* dataNode узла, если он - текстовый служебный. Иначе, возвращает NULL.     */

std::string getStrFromXml(const xmlNodePtr dataNode);
/* Возвращает STL-string, построенный на текстовом содержимом узла dataNode, */
/* если первый вложенный в dataNode узел - текстовый служебный. Иначе,       */
/* возвращает пустую строку.                                                 */

inline std::string getStrFromXml(const xmlNodePtr parentNode, const std::string &child_name)
{
  return getStrFromXml(child_name.empty()?parentNode:findNode(parentNode,child_name.c_str()));
}

std::string getTrimmedStrFromXml(const xmlNodePtr dataNode);
/* Возвращает STL-string, построенный на текстовом содержимом узла dataNode, */
/* если первый вложенный в dataNode узел - текстовый служебный. Иначе,       */
/* возвращает пустую строку. Удаляет isspace-символы по обоим концам строки. */

inline std::string getTrimmedStrFromXml(const xmlNodePtr parentNode, const std::string &child_name)
{
  return getTrimmedStrFromXml(child_name.empty()?parentNode:findNode(parentNode,child_name.c_str()));
}

std::string getStrPropFromXml(const xmlNodePtr dataNode, const char *prop);
std::string getStrPropFromXml(const xmlNodePtr dataNode, const std::string &prop);
/* Возвращает STL-string, построенный на текстовом содержимом атрибута prop  */
/* узла dataNode. При отсутствии искомого атрибута, возвращает пустую строку.*/

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
/***************        C++-Функции для поиска данных          ***************/
/*****************************************************************************/

xmlXPathObjectPtr getNodeSetByXPath(xmlNodePtr, const std::string&); 
xmlNodePtr getFirstNodeByXPath(xmlNodePtr, const std::string&);


#endif /* __cplusplus */

#endif /* __XML_TOOLS_H__ */
