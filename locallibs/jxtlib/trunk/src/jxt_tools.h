#ifndef __JXT_TOOLS_H__
#define __JXT_TOOLS_H__

#include <libxml/tree.h>

#ifdef __cplusplus
#include <string>
#include <vector>
#include <map>
extern "C"
{
#endif /* __cplusplus */

xmlDocPtr newDoc();
/* Создает новый XML-документ в формате JXT с кодировкой UTF-8, корневым     */
/* тэгом term и вложенным в него тэгом answer.                               */

xmlNodePtr setElemProp(xmlNodePtr resNode, const char *tag, const char *prop,
                       const char *value);
/* Создает тэг свойств в формате JXT в узле properties, вложенном в resNode. */
/* resNode - узел, соответствующий тэгу answer;                              */
/* tag - id элемента интерфейса, или список id элементов, указанных через    */
/*       запятую;                                                            */
/* prop - имя задаваемого свойства;                                          */
/* value - новое значение для свойства.                                      */
/* Функция создает тэг idref при указании одиночного id в параметре tag, и   */
/* тэг, соответствующий имени свойства, при списочном tag.                   */

void addPeriodConstraintDDMMRR(const char *date1, const char *date2,
                               const char *tag, xmlNodePtr resNode);
/* Задает ограничения для JXT-элемента date с идентификатором tag, согласно  */
/* датам date1 и date2, заданным в формате DDMMRR. resNode должен указывать  */
/* на тэг answer.                                                            */

void addPeriodConstraintRRMMDD(const char *date1, const char *date2,
                               const char *tag, xmlNodePtr resNode);
/* Задает ограничения для JXT-элемента date с идентификатором tag, согласно  */
/* датам date1 и date2, заданным в формате RRMMDD. resNode должен указывать  */
/* на тэг answer.                                                            */

void addFrequencyConstraint(const char *freq);
/* Задает ограничения для окошка-уточнения частоты периода, согласно         */
/* переданной частоты.                                                       */

xmlNodePtr iface_C(xmlNodePtr resNode, const char *iface_id);
/* Вызывает C++-функцию iface                                                */

xmlNodePtr iface_C_nocheck(xmlNodePtr resNode, const char *iface_id);
/* Вызывает C++-функцию iface                                                */

void closeUserSession(const char *pult, xmlNodePtr resNode);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#ifdef __cplusplus
/*****************************************************************************/
/***************               C++-функции                     ***************/
/*****************************************************************************/

inline xmlNodePtr setElemProp(xmlNodePtr resNode, const char *tag,
                              const char *prop, const std::string& value)
{
  return setElemProp(resNode, tag, prop, value.c_str());
}

xmlNodePtr iface(xmlNodePtr resNode, const std::string &iface_id);
xmlNodePtr iface(xmlNodePtr resNode, const std::string &iface_id, int handle,
                 bool no_check=false);
/* Создает тэги interface и links, необходимые JXT для отрисовки интерфейса  */
/* в окне. Параметр resNode должен соответствовать тэгу answer ответа,       */
/* iface_id должен содержать id отображаемого интерфейса.                    */
/* Возвращает указатель на тэг interface.                                    */

int gotoIface(xmlNodePtr resNode, const char *iface_id, int do_not_draw=0);

xmlNodePtr insertEmptyRow(xmlNodePtr tabNode, int col_count, int row_index);
/* Добавляет в данные для JXT-таблицы, на которую указывает tabNode, строку  */
/* с индексом row_index и col_count пустыми колонками.                       */
/* Возвращает указатель на созданный узел row.                               */

long getXmlDataVer_inner(const std::string &type, const std::string &id,
                         bool no_iparts);

std::string getCachedIfaceWoIparts(const std::string &id, long answer_ver);
void mergeIpartIntoIface(xmlNodePtr ansNode, const std::string &ipart_name);
void setCachedIfaceWoIparts(xmlNodePtr resNode, const std::string &id,
                            long ver);
long getXmlDataVer(const std::string &type, const std::string &rid);
/* Возвращает актуальную (наибольшую) версию ресурса типа type с             */
/* идентификатором rid из таблицы XML_STUFF. В случае ненахождения ресурса,  */
/* возвращает -1.                                                            */

long getDataVer(const char *data_id);
/* Возвращает актуальную (наибольшую) версию данных с идентификатором id.    */
/* В случае ненахождения данных возвращает -1.                               */

void updateJxtData(const char *data_id, long term_ver, xmlNodePtr resNode);
/* Записывает в resNode информацию, необходимую для обновления данных на JXT.*/
/* data_id - идентификатор данных, term_ver - текущая версия на терминале.   */

long getNewestPluginVer(const char *id, const char *ext);
/* Возвращает актуальную версию бинарных данных с идентификатором id и       */
/* расширением ext.                                                          */

std::vector<char> readBinary(const char *id, const char *ext, long ver, int *err);

void InsertXmlData(const char *type, const char *id, const std::string &data);

std::string GetXmlData2(const char *type, const char *id, long query_ver);

std::string GetXmlData(const char *type, const char *id, long query_ver);

inline std::string GetActualXmlData(const char *type, const char *id)
{
  return GetXmlData(type,id,getXmlDataVer(type,id));
}

void markCurrWindowAsModal(xmlNodePtr resNode);

xmlDocPtr getXmlStuffDoc(const std::string &id, const std::string &type);

#endif /* __cplusplus */


#endif /* __JXT_TOOLS_H__ */
