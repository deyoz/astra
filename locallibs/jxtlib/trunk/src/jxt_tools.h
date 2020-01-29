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
/* ������� ���� XML-���㬥�� � �ଠ� JXT � ����஢��� UTF-8, ��୥��     */
/* ��� term � �������� � ���� ��� answer.                               */

xmlNodePtr setElemProp(xmlNodePtr resNode, const char *tag, const char *prop,
                       const char *value);
/* ������� �� ᢮��� � �ଠ� JXT � 㧫� properties, ��������� � resNode. */
/* resNode - 㧥�, ᮮ⢥�����騩 ��� answer;                              */
/* tag - id ����� ����䥩�, ��� ᯨ᮪ id ����⮢, 㪠������ �१    */
/*       �������;                                                            */
/* prop - ��� ����������� ᢮��⢠;                                          */
/* value - ����� ���祭�� ��� ᢮��⢠.                                      */
/* �㭪�� ᮧ���� �� idref �� 㪠����� �����筮�� id � ��ࠬ��� tag, �   */
/* ��, ᮮ⢥�����騩 ����� ᢮��⢠, �� ᯨ�筮� tag.                   */

void addPeriodConstraintDDMMRR(const char *date1, const char *date2,
                               const char *tag, xmlNodePtr resNode);
/* ������ ��࠭�祭�� ��� JXT-����� date � �����䨪��஬ tag, ᮣ��᭮  */
/* ��⠬ date1 � date2, ������� � �ଠ� DDMMRR. resNode ������ 㪠�뢠��  */
/* �� �� answer.                                                            */

void addPeriodConstraintRRMMDD(const char *date1, const char *date2,
                               const char *tag, xmlNodePtr resNode);
/* ������ ��࠭�祭�� ��� JXT-����� date � �����䨪��஬ tag, ᮣ��᭮  */
/* ��⠬ date1 � date2, ������� � �ଠ� RRMMDD. resNode ������ 㪠�뢠��  */
/* �� �� answer.                                                            */

void addFrequencyConstraint(const char *freq);
/* ������ ��࠭�祭�� ��� ���誠-��筥��� ����� ��ਮ��, ᮣ��᭮         */
/* ��।����� �����.                                                       */

xmlNodePtr iface_C(xmlNodePtr resNode, const char *iface_id);
/* ��뢠�� C++-�㭪�� iface                                                */

xmlNodePtr iface_C_nocheck(xmlNodePtr resNode, const char *iface_id);
/* ��뢠�� C++-�㭪�� iface                                                */

void closeUserSession(const char *pult, xmlNodePtr resNode);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#ifdef __cplusplus
/*****************************************************************************/
/***************               C++-�㭪樨                     ***************/
/*****************************************************************************/

inline xmlNodePtr setElemProp(xmlNodePtr resNode, const char *tag,
                              const char *prop, const std::string& value)
{
  return setElemProp(resNode, tag, prop, value.c_str());
}

xmlNodePtr iface(xmlNodePtr resNode, const std::string &iface_id);
xmlNodePtr iface(xmlNodePtr resNode, const std::string &iface_id, int handle,
                 bool no_check=false);
/* ������� �� interface � links, ����室��� JXT ��� ���ᮢ�� ����䥩�  */
/* � ����. ��ࠬ��� resNode ������ ᮮ⢥��⢮���� ��� answer �⢥�,       */
/* iface_id ������ ᮤ�ঠ�� id �⮡ࠦ������ ����䥩�.                    */
/* �����頥� 㪠��⥫� �� �� interface.                                    */

int gotoIface(xmlNodePtr resNode, const char *iface_id, int do_not_draw=0);

xmlNodePtr insertEmptyRow(xmlNodePtr tabNode, int col_count, int row_index);
/* �������� � ����� ��� JXT-⠡����, �� ������ 㪠�뢠�� tabNode, ��ப�  */
/* � �����ᮬ row_index � col_count ����묨 ���������.                       */
/* �����頥� 㪠��⥫� �� ᮧ����� 㧥� row.                               */

long getXmlDataVer_inner(const std::string &type, const std::string &id,
                         bool no_iparts);

std::string getCachedIfaceWoIparts(const std::string &id, long answer_ver);
void mergeIpartIntoIface(xmlNodePtr ansNode, const std::string &ipart_name);
void setCachedIfaceWoIparts(xmlNodePtr resNode, const std::string &id,
                            long ver);
long getXmlDataVer(const std::string &type, const std::string &rid);
/* �����頥� ���㠫��� (����������) ����� ����� ⨯� type �             */
/* �����䨪��஬ rid �� ⠡���� XML_STUFF. � ��砥 ����宦����� �����,  */
/* �����頥� -1.                                                            */

long getDataVer(const char *data_id);
/* �����頥� ���㠫��� (����������) ����� ������ � �����䨪��஬ id.    */
/* � ��砥 ����宦����� ������ �����頥� -1.                               */

void updateJxtData(const char *data_id, long term_ver, xmlNodePtr resNode);
/* �����뢠�� � resNode ���ଠ��, ����室���� ��� ���������� ������ �� JXT.*/
/* data_id - �����䨪��� ������, term_ver - ⥪��� ����� �� �ନ����.   */

long getNewestPluginVer(const char *id, const char *ext);
/* �����頥� ���㠫��� ����� ������� ������ � �����䨪��஬ id �       */
/* ���७��� ext.                                                          */

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
