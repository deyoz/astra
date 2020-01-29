#ifdef HAVE_MPATROL
#include <mpatrol.h>
#endif
#define NICKNAME "ILYA"
#define NICKTRACE ILYA_TRACE
#include <serverlib/test.h>

#include <string.h>
#include <stdarg.h>
#include <libxml/tree.h>
#include <string>
#include "xmllibcpp.h"
#include "xml_msg.h"
#include "xml_tools.h"
#include "jxt_xml_cont.h"
#include "lngv.h"
#include "jxtlib.h"
#include "gettext.h"
#include "jxt_sys_reqs.h"
#include "jxt_tools.h"
#include "cont_tools.h"

using namespace std;
using namespace loclib;

void addXmlMessageBox(xmlNodePtr resNode, std::string msg)
// creates unique <messageBox> node in resNode's child <command>
{
  xmlNodePtr comNode=getNode(resNode,"command");
  xmlNodePtr msgBoxNode=getNode(comNode,"messageBox");
  if(msgBoxNode)
  {
    xmlUnlinkNode(msgBoxNode);
    xmlFreeNode(msgBoxNode);
  }
  xmlNewTextChild(comNode,NULL,"messageBox",msg);
}

void addXmlMessageBoxFmt_v(xmlNodePtr resNode, const char *fmt, va_list ap)
{
  char tmp[MAX_MSGBOX_CONTENT_LEN];
  int written=vsnprintf(tmp,MAX_MSGBOX_CONTENT_LEN-1,fmt,ap);
  if(written>MAX_MSGBOX_CONTENT_LEN-1 || written<1)
    ProgTrace(TRACE2,"written=%i, fmt='%s'",written,fmt);
  addXmlMessageBox(resNode,tmp);
}

void addXmlMessageBoxFmt(xmlNodePtr resNode, const char *fmt, ...)
{
  va_list ap;
  va_start(ap,fmt);
  addXmlMessageBoxFmt_v(resNode,fmt,ap);
  va_end(ap);
}

extern "C" void addCXmlMessageBoxFmt(xmlNodePtr resNode, const char *fmt, ...)
{
  va_list ap;
  va_start(ap,fmt);
  addXmlMessageBoxFmt_v(resNode,fmt,ap);
  va_end(ap);
}

int getCurrLang()
{
  return LocaleLib::Instance()->GetCallbacks()->getCurrLang();
}

std::string _getTextByNum(unsigned code, va_list ap, int lang)
{
  const char *str=LocaleLib::Instance()->GetCallbacks()
                           ->get_msg_by_num(code,lang);
  if(!str)
      str=(lang==ENGLISH) ? "MESSAGE NUMBER OUT OF RANGE" :
                            "НОМЕР СООБЩЕНИЯ НЕ ИЗВЕСТЕН СИСТЕМЕ";
  char s[1000];
  vsprintf(s,str,ap);
  return string(s);
}

std::string getTextByNum(unsigned code, ...)
{
  int lang=getCurrLang();
  va_list ap;
  va_start(ap,code);
  string str=_getTextByNum(code,ap,lang);
  va_end(ap);
  return str;
}

std::string getTextByNumL(unsigned code, int lang, ...)
{
  va_list ap;
  va_start(ap,lang);
  string str=_getTextByNum(code,ap,lang);
  va_end(ap);
  return str;
}

void addXmlMessage(const char *msg)
{
  xmlNewTextChild(getNode(getResNode(),"command"),NULL,"message",
                  getLocalizedText(msg));
}

xmlNodePtr create_error_msg(xmlNodePtr resNode, const char *fmt, ...)
/* Эта функция формирует строку-сообщение из fmt и следующих за fmt */
/* параметров и записывает ее в тэг <error>, который помещает в тэг */
/* <command>. Если тэга <command> не существует, функция создает его */
/* в тэге, в котором содержится resNode (на одном уровне с ним). */
/* ЗАМЕЧАНИЕ: для нормальной обработки сообщения терминалом необходимо, */
/* чтобы resNode соответствовал тэгу <answer>. */
{
  va_list ap;
  char *res=NULL;
  int res_len=100;

  if((res=(char *)malloc(res_len))==NULL)
  {
    ProgError(STDLOG,"Cannot allocate memory!!!");
    return NULL;
  }

  while(1)
  {
    int written;

    va_start(ap,fmt);
    written=vsnprintf(res,res_len,fmt,ap);
    va_end(ap);

    if(written>-1 && written<res_len)
      break;
    if(written>-1)
      res_len=written+1;
    else
      res_len*=2;

    if((res=(char *)realloc(res,res_len))==NULL)
    {
      ProgError(STDLOG,"Cannot allocate memory!!!");
      return NULL;
    }
  }
  xmlNodePtr errNode=getNode(getNode(resNode,"command"),"error");
  xmlNodeSetContent(errNode,res);
  free(res);
  return errNode;
}

void createErrorDoc( const  char * nick, const char * file , int line , std::string&& msg, int how)
{
  if(auto ctx = getXmlCtxt())
      ctx->addBottomMessage(BottomMessage(nick?nick:"", file?file:"", line, std::move(msg), how));
}

void createErrorDoc( const  char * nick,
        const char * file , int line ,const char *msg, int how)
{
  if(getXmlCtxt())
  {
    getXmlCtxt()->addBottomMessage(BottomMessage(nick?nick:"",file?file:"",
                                                 line,msg,how));
  }
}

void createMsgDoc(const char *msg)
{
  return createErrorDoc(STDLOG,msg,loclib::LocaleLib::Instance()->GetCallbacks()->getMessageId());
}

int ErrTagSetProps(xmlNodePtr resNode, const std::string& iface_id, const char *tag)
{
  if(!iface_id.empty() && tag && strlen(tag)>0)
  {
    const char *ptr=NULL;
    xmlNodePtr propNode,node;

    iface(resNode, iface_id);
    propNode=getNode(resNode,"properties");

    if((ptr=strchr(tag,','))==NULL)
    {
      node=newChild(propNode,"idref");
      xmlSetProp(node,"error","true");
      xmlSetProp(node,"focus","true");
    }
    else
    {
      if(ptr-tag<100)
      {
        node=newChild(propNode,"idref");
        xmlSetProp(node,"focus","true");
        xmlSetProp(node,"name",std::string(tag, ptr-tag));
        xmlSetProp(node,"save","false");
      }
      node=newChild(propNode,"error");
      xmlNodeSetContent(node,"true");
    }
    xmlSetProp(node,"name",tag);
    xmlSetProp(node,"save","false");
    return 0;
  }
  else
    return 1;
}

void create_xmlerr(const char *tag, const char *fmt, va_list ap)
{
  if(!fmt)
  {
    ProgTrace(TRACE1,"Invalid parameters!");
    return;
  }

  std::string iface_id = readSysContextNVL("CUR_IFACE","");
  int req_handle=readSysContextInt("REQ_HANDLE",0);

  char msg[1000] = {};
  vsnprintf(msg,sizeof(msg)-1,fmt,ap);

  createErrorDoc(STDLOG,msg,loclib::LocaleLib::Instance()->GetCallbacks()->getErrorMessageId());
  xmlDocPtr resDoc = getResDoc();
  if(resDoc && !iface_id.empty())
    ErrTagSetProps(resDoc->children->children,iface_id,tag);
  xmlSetProp(getResNode(),"handle",req_handle);
}

namespace
{
    void clearAnswerNode()
    {
        xmlNodePtr resNode = xmlDocGetRootElement(getXmlCtxt()->resDoc);
//        ProgTrace(TRACE5, "resNode: [%s]", resNode->name);
        xmlClearNode(findNodeR(resNode, "answer"));
    }
}

void throw_xmlerr(const char *tag, const char *fmt, ...)
{
    //  clearing answer node
    clearAnswerNode();
    va_list ap;
    va_start(ap,fmt);
    create_xmlerr(tag,getLocalizedText(fmt),ap);
    va_end(ap);
    throw jxtlib::jxtlib_custom_exception(STDLOG);
}

void throw_xmlerr_no_loc(const char *tag, const char *fmt, ...)
{
  va_list ap;
  va_start(ap,fmt);
  create_xmlerr(tag,fmt,ap);
  va_end(ap);
  throw jxtlib::jxtlib_custom_exception(STDLOG);
}

extern "C" int xmlerr(int *err, const char *tag, const char *fmt, ...)
{
  va_list ap;
  va_start(ap,fmt);
  create_xmlerr(tag,fmt,ap);
  if(err)
    *err=0xffffffff;
  va_end(ap);

  return 0;
}

void throw_xmlerr_inner(const char *tag, unsigned err, va_list ap)
{
  int lang=getCurrLang();
  const char *str=LocaleLib::Instance()->GetCallbacks()
                           ->get_msg_by_num(err,lang);

  if(!str)
    str=(lang==ENGLISH) ? "MESSAGE NUMBER OUT OF RANGE" :
                          "НОМЕР ОШИБКИ НЕ ИЗВЕСТЕН СИСТЕМЕ";

  create_xmlerr(tag,str,ap);
}

void like_throw_xmlerr(const char *tag, unsigned err, ...)
{
  va_list ap;
  va_start(ap,err);

  throw_xmlerr_inner(tag,err,ap);

  va_end(ap);
}

void throw_xmlerr(const char *tag, unsigned err, ...)
{
    clearAnswerNode();

  va_list ap;
  va_start(ap,err);

  throw_xmlerr_inner(tag,err,ap);

  va_end(ap);
  throw jxtlib::jxtlib_custom_exception(STDLOG);
}

void throw_xmlerr(const unsigned char *tag, unsigned err, ...)
{
    clearAnswerNode();

  va_list ap;
  va_start(ap,err);

  throw_xmlerr_inner((const char *)tag,err,ap);

  va_end(ap);
  throw jxtlib::jxtlib_custom_exception(STDLOG);
}

namespace jxtlib
{

E_throw_xmlerr::E_throw_xmlerr(const char* tag, const std::string &msg)
    : E_throw_xmlerr(tag, msg.c_str()) {}

E_throw_xmlerr::E_throw_xmlerr(const std::string& tag, const char* msg)
    : E_throw_xmlerr(tag.c_str(), msg) {}

E_throw_xmlerr::E_throw_xmlerr(const std::string& tag, const std::string &msg)
    : E_throw_xmlerr(tag.c_str(), msg.c_str()) {}

E_throw_xmlerr::E_throw_xmlerr(const xmlChar* tag, const char* msg)
    : E_throw_xmlerr(reinterpret_cast<const char*>(tag), msg) {}

E_throw_xmlerr::E_throw_xmlerr(const char *tag, const char *fmt, ...) : 
 jxtlib::jxtlib_custom_exception(STDLOG)
{
    //  clearing answer node
    clearAnswerNode();
    va_list ap;
    va_start(ap,fmt);
    create_xmlerr(tag,getLocalizedText(fmt),ap);
    va_end(ap);
}

E_throw_xmlerr::E_throw_xmlerr(const char *tag, const unsigned char *fmt, ...) : 
 jxtlib::jxtlib_custom_exception(STDLOG)
{
    //  clearing answer node
    clearAnswerNode();
    va_list ap;
    va_start(ap,fmt);
    create_xmlerr(tag,getLocalizedText((const char *)fmt),ap);
    va_end(ap);
}

E_throw_xmlerr::E_throw_xmlerr(const char *tag, unsigned err, ...) : 
 jxtlib::jxtlib_custom_exception(STDLOG)
{
    clearAnswerNode();

  va_list ap;
  va_start(ap,err);

  throw_xmlerr_inner(tag,err,ap);

  va_end(ap);
}

E_throw_xmlerr::E_throw_xmlerr(const unsigned char *tag, unsigned err, ...) : 
 jxtlib::jxtlib_custom_exception(STDLOG)
{
    clearAnswerNode();

  va_list ap;
  va_start(ap,err);

  throw_xmlerr_inner((const char *)tag,err,ap);

  va_end(ap);
}

} // jxtlib

void throw_ind_xmlerr(const char *tag, const char *ind,
                                 const char *fmt, ...)
{
  if(!tag || !ind)
    throw jxtlib::jxtlib_exception(STDLOG,"Ошибка программы!");
  va_list ap;
  va_start(ap,fmt);
  std::string idref_name=std::string(tag)+"["+ind+"]";
  create_xmlerr(idref_name.c_str(),getLocalizedText(fmt),ap);
  va_end(ap);
  throw jxtlib::jxtlib_custom_exception(STDLOG);
}

void throw_iind_xmlerr(const char *tag, int ind,
                                 const char *fmt, ...)
{
  if(!tag)
    throw jxtlib::jxtlib_exception(STDLOG,"Ошибка программы!");
  va_list ap;
  va_start(ap,fmt);
  char iind[25];
  sprintf(iind,"[%i]",ind);
  std::string idref_name=std::string(tag)+iind;
  create_xmlerr(idref_name.c_str(),getLocalizedText(fmt),ap);
  va_end(ap);
  throw jxtlib::jxtlib_custom_exception(STDLOG);
}

void throw_ind_xmlerr(const char *tag, const char *ind, unsigned err, ...)
{
  if(!tag || !ind)
    throw jxtlib::jxtlib_exception(STDLOG,"Ошибка программы!");
  va_list ap;
  va_start(ap,err);
  int lang=getCurrLang();
  const char *str=LocaleLib::Instance()->GetCallbacks()
                           ->get_msg_by_num(err,lang);

  if(!str)
    str=(lang==ENGLISH) ? "MESSAGE NUMBER OUT OF RANGE" :
                          "НОМЕР ОШИБКИ НЕ ИЗВЕСТЕН СИСТЕМЕ";

  std::string idref_name=std::string(tag)+"["+ind+"]";
  create_xmlerr(idref_name.c_str(),str,ap);
  va_end(ap);
  throw jxtlib::jxtlib_custom_exception(STDLOG);
}

void throw_iind_xmlerr(const char *tag, int ind, unsigned err, ...)
{
  if(!tag)
    throw jxtlib::jxtlib_exception(STDLOG,"Ошибка программы!");
  va_list ap;
  va_start(ap,err);
  char iind[25];
  sprintf(iind,"[%i]",ind);
  int lang=getCurrLang();

  std::string idref_name=std::string(tag)+iind;

  const char *str=LocaleLib::Instance()->GetCallbacks()
                           ->get_msg_by_num(err,lang);

  if(!str)
    str=(lang==ENGLISH) ? "MESSAGE NUMBER OUT OF RANGE" :
                          "НОМЕР ОШИБКИ НЕ ИЗВЕСТЕН СИСТЕМЕ";

  create_xmlerr(idref_name.c_str(),str,ap);
  va_end(ap);
  throw jxtlib::jxtlib_custom_exception(STDLOG);
}

void throw_tree_xmlerr(const char *tag, const char *termId,
                                  const char *fmt, ...)
{
  if(!tag || !termId)
    throw jxtlib::jxtlib_exception(STDLOG,"Ошибка программы!");
  va_list ap;
  va_start(ap,fmt);
  create_xmlerr(tag,getLocalizedText(fmt),ap);
  va_end(ap);
  throw jxtlib::jxtlib_custom_exception(STDLOG);
}

void throw_tree_xmlerr(const char *tag, const char *termId, unsigned err, ...)
{
  if(!tag || !termId)
    throw jxtlib::jxtlib_exception(STDLOG,"Ошибка программы!");
  va_list ap;
  va_start(ap,err);
  int lang=getCurrLang();

  const char *str=LocaleLib::Instance()->GetCallbacks()
                           ->get_msg_by_num(err,lang);

  if(!str)
    str=(lang==ENGLISH) ? "MESSAGE NUMBER OUT OF RANGE" :
                          "НОМЕР ОШИБКИ НЕ ИЗВЕСТЕН СИСТЕМЕ";


  //std::string idref_name=std::string(tag)+"{"+termId+"}";
  create_xmlerr(/*idref_name.c_str()*/tag,str,ap);
  va_end(ap);
  throw jxtlib::jxtlib_custom_exception(STDLOG);
}

int isErrorAnswer()
{
  xmlDocPtr resDoc=getResDoc();
  if(!resDoc || !resDoc->children || !resDoc->children->children)
    return 0;

  xmlNodePtr resNode=resDoc->children->children;
  if(isempty(resNode))
    return 0;
  resNode=resNode->children;
  if(xmlStrcmp(resNode->name,"command")!=0 || resNode->next!=NULL ||
     isempty(resNode))
    return 0;
  resNode=resNode->children;
  if(xmlStrcmp(resNode->name,"error")!=0 || resNode->next!=NULL)
    return 0;
  return 1;
}

void convertError2Message()
{
  xmlNodePtr resNode=findNodeR(getResDoc()->children,"error");
  if(!resNode)
    return;
  xmlNodeSetName(resNode,"message");
}

xmlDocPtr createExceptionDoc(const string &msg, int handle,
                             const char *description)
{
  xmlDocPtr resDoc;
  if((resDoc=newDoc())!=NULL)
  {
    xmlNodePtr errNode=create_error_msg(resDoc->children->children,"%s",
                                        msg.c_str());
    xmlSetProp(errNode,"product","Сирена");
    xmlSetPropInt(resDoc->children->children,"handle",handle);
    if(errNode && description)
      xmlNewTextChild(errNode,NULL,"description",description);
    XMLRequestCtxt *ctxt=getXmlCtxt();
    if(ctxt)
      ctxt->set_donotencode2UTF8(0);
  }
  else
    ProgError(STDLOG,"Cannot create new xml document!!!");
  return resDoc;
}
