#ifndef __XML_MSG_H__
#define __XML_MSG_H__

#define MAX_MSGBOX_CONTENT_LEN 1000

#include <stdarg.h>
#include <libxml/tree.h>
#include "lngv.h"
#include "gettext.h"

#ifdef __cplusplus
#include <string>

void addXmlMessageBox(xmlNodePtr resNode, std::string msg);
void addXmlMessageBoxFmt(xmlNodePtr resNode, const char *fmt, ...);

int isErrorAnswer();
void convertError2Message();

void throw_iind_xmlerr(const char *tag, int ind, const char *fmt, ...);
void throw_iind_xmlerr(const char *tag, int ind, unsigned err, ...);
void throw_ind_xmlerr(const char *tag, const char *ind, const char *fmt, ...);
void throw_ind_xmlerr(const char *tag, const char *ind, unsigned err, ...);
/* Creates errorDoc for tag[ind] */
/* CAUTION! This function never returns! */

void throw_tree_xmlerr(const char *tag, const char *termId,
                       const char *fmt, ...);
void throw_tree_xmlerr(const char *tag, const char *termId,
                       unsigned err, ...);
/* Creates errorDoc for tag{termId} */
/* CAUTION! This function never returns! */

void like_throw_xmlerr(const char *tag, unsigned err, ...);
void throw_xmlerr(const char *tag, const char *fmt, ...);
void throw_xmlerr(const char *tag, unsigned err, ...);
void throw_xmlerr(const unsigned char *tag, unsigned err, ...);
inline void throw_xmlerr(const std::string &tag, const std::string &msg)
{
  return throw_xmlerr(tag.c_str(),msg.c_str());
}
void throw_xmlerr_no_loc(const char *tag, const char *fmt, ...);
/* ������� CustomException ᮣ��᭮ �ந����쭮� ��ப� fmt ... */
/* CAUTION! These functions never return! */

extern "C"
{
#endif /* __cplusplus */

void addXmlMessage(const char *msg);
/* Adds <command><message>...</message></command> to AnswerDoc which gets by */
/* getAnswerDoc() function */

xmlNodePtr create_error_msg(xmlNodePtr resNode, const char *fmt, ...);
/* �� �㭪�� �ନ��� ��ப�-ᮮ�饭�� �� fmt � ᫥����� �� fmt */
/* ��ࠬ��஢ � �����뢠�� �� � �� <error>, ����� ����頥� � �� */
/* <command>. �᫨ �� <command> �� �������, �㭪�� ᮧ���� ��� */
/* � ��, ᮮ⢥�����饬 resNode. */
/* ���������: ��� ��ଠ�쭮� ��ࠡ�⪨ ᮮ�饭�� �ନ����� ����室���, */
/* �⮡� resNode ᮮ⢥��⢮��� ��� <answer>. */

//int err_msg_s(char * s, unsigned code, ...);
void addCXmlMessageBoxFmt(xmlNodePtr resNode, const char *fmt, ...);

int xmlerr(int *err, const char *tag, const char *fmt, ...);

#ifdef __cplusplus
}
std::string _getTextByNum(unsigned code, va_list ap, int language);
inline std::string _getTextByNum(unsigned code, va_list ap)
{
  return _getTextByNum(code,ap,getCurrLang());
}
std::string getTextByNum(unsigned code, ...);
std::string getTextByNumL(unsigned code, int language, ...);

xmlDocPtr createExceptionDoc(const std::string &msg, int handle,
                             const char *description=NULL);

int ErrTagSetProps(xmlNodePtr resNode, const std::string& iface_id, const char *tag);
void createErrorDoc( const  char * nick, const char * file , int line , std::string&& msg, int how);
void createErrorDoc(const char *nick, const char * file, int line, const char *msg, int how);
void createMsgDoc(const char *msg);

#endif /* __cplusplus */
#endif /* __XML_MSG_H__ */
