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
/* Создает CustomException согласно произвольной строке fmt ... */
/* CAUTION! These functions never return! */

extern "C"
{
#endif /* __cplusplus */

void addXmlMessage(const char *msg);
/* Adds <command><message>...</message></command> to AnswerDoc which gets by */
/* getAnswerDoc() function */

xmlNodePtr create_error_msg(xmlNodePtr resNode, const char *fmt, ...);
/* Эта функция формирует строку-сообщение из fmt и следующих за fmt */
/* параметров и записывает ее в тэг <error>, который помещает в тэг */
/* <command>. Если тэга <command> не существует, функция создает его */
/* в тэге, соответствующем resNode. */
/* ЗАМЕЧАНИЕ: для нормальной обработки сообщения терминалом необходимо, */
/* чтобы resNode соответствовал тэгу <answer>. */

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
