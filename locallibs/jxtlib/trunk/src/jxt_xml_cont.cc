#ifdef HAVE_MPATROL
#include <mpatrol.h>
#endif
#include <string.h>

#define NICKNAME "ILYA"
#define NICKTRACE ILYA_TRACE
#include <serverlib/test.h>

#include <libxml/tree.h>
#include "xml_stuff.h"
#include "jxt_stuff.h"
#include "xml_tools.h"
#include "xml_msg.h"
#include "jxt_tools.h"
#include "jxt_cont.h"
#include "cont_tools.h"
#include "jxt_handle.h"
#include "jxt_xml_cont.h"
#include "jxtlib.h"
#include <serverlib/str_utils.h>
#include "xmllibcpp.h"

using namespace std;
using namespace jxtlib;
using namespace JxtContext;

int getErrorMessageId()
{
	return loclib::LocaleLib::Instance()->GetCallbacks()->getErrorMessageId();
}
int getMessageId()
{
	return loclib::LocaleLib::Instance()->GetCallbacks()->getMessageId();
}
int getUnknownMessageId()
{
	return loclib::LocaleLib::Instance()->GetCallbacks()->getUnknownMessageId();
}

XMLRequestCtxt *XMLRequestCtxt::Instance(bool reset)
{
  static XMLRequestCtxt *instance_=0;
  if(reset)
  {
    delete instance_;
    instance_=0;
  }
  if(!instance_)
    instance_=new XMLRequestCtxt();
  return instance_;
}


XMLRequestCtxt::XMLRequestCtxt()
{
  QueryHandle=AnswerHandle=0;
  Query="";
  opr="";
  pult="";
  resDoc=NULL;
  reqDoc=NULL;
  HandlerInd=-1;
  _lang=-1;
  do_not_encode2utf8 = 0;
}

void printDoc(xmlNodePtr node)
{
  while(node)
  {
    if(node)
    {
      if(istext(node))
        ProgTrace(TRACE1,"<%s>%s</%s>",node->name,getText(node),node->name);
      else
        ProgTrace(TRACE1,"<%s/>",node->name);
    }
    if(!isempty(node))
      printDoc(node->children);
    node=node->next;
  }
}

XMLRequestCtxt::~XMLRequestCtxt()
{
  xmlFreeDoc(resDoc);
  xmlFreeDoc(reqDoc);
}

void XMLRequestCtxt::Init(const std::string &Query, const std::string &pult_, const std::string &opr_)
{
  resDoc=reqDoc=NULL;

  pult = pult_;
  opr  = opr_;

  xmlKeepBlanksDefault(0);
  if((reqDoc=xmlParseMemory(Query.c_str(),Query.length()))==NULL)
  {
    ProgError(STDLOG,"Cannot parse input string!");
    throw jxtlib_exception(STDLOG,"Неверный формат запроса");
  }
  if(xml_decode_nodelist(reqDoc->children))
    throw jxtlib_exception(STDLOG,"Ошибка перекодировки ответа");
  if(!reqDoc || !reqDoc->children || xmlStrcmp(reqDoc->children->name,"term"))
    throw jxtlib_exception(STDLOG,"Неверный формат запроса");
  xmlNodePtr reqNode=reqDoc->children->children;
  if(!reqNode || xmlStrcmp(reqNode->name,"query"))
    throw jxtlib_exception(STDLOG,"Неверный формат запроса");

  int handle=atoiNVL(getprop(reqNode,"handle"),-100);
  if(handle==-100)
  {
    ProgTrace(TRACE1,"Отсутствует атрибут handle, считаем handle=0");
    handle=0;
    //throw jxtlib_exception(STDLOG,"Неверный формат запроса");
  }
  // Посмотрим на язык запроса
  static ILanguage iLanguage=ILanguage::getILanguage();
  auto buf=getprop(reqNode,"lang");
  char _lang[3];
  if(!buf || strlen(buf)!=2)
  {
    // по умолчанию - русский
    ProgTrace(TRACE1,"Wrong or missing attribute: lang='%s'",buf);
    strcpy(_lang,"RU");
  }
  else
  {
    strcpy(_lang,buf);
    UpperCase(_lang);
  }
  setLang(iLanguage.getIda(_lang));

  QueryHandle=AnswerHandle=handle;

  JxtContHandler *jch=getJxtContHandler();
  JxtCont *jc_sys=jch->sysContext();
  jc_sys->write("REQ_HANDLE",handle);
  jc_sys->write("HANDLE",handle);
  jc_sys->write("JXT_BUILD",getStrPropFromXml(reqNode,"build"));
  /*JxtCont *jc=*/ jch->getContext(handle);
  jch->setCurrentContext(handle);

  ProgTrace(TRACE1,"reqNode->name='%s'",reqNode->name);
  buf=getprop(reqNode,"id");
  //int a=0;
  if(buf)
  {
    jc_sys->write("CUR_IFACE",buf);
  }
  else
  {
    jc_sys->write("CUR_IFACE","");
  }
  /**
  * С версией интерфейса мы в данный момент ничего не делаем
  */
  /*
  buf=getprop(reqNode,"ver");
  if(buf)
    jc->write("IFACE_VER",buf);
  */

  resDoc=newDoc();

  if(!(reqNode=reqNode->children))
  {
    ProgError(STDLOG,"Отсутствует командный тэг");
    throw jxtlib_exception(STDLOG,"Неверный формат запроса");
  }
  HandlerInd=0;
}

int XMLRequestCtxt::isResDocEmpty()
{
  if(!resDoc || !resDoc->children || !resDoc->children->children ||
     isempty(resDoc->children->children))
    return 1;
  return 0;
}

void XMLRequestCtxt::setNewOpr(string opr_code)
{
  opr=opr_code;
}

extern "C" xmlDocPtr getResDoc()
{
  XMLRequestCtxt *ctxt=getXmlCtxt();
  if(!ctxt || !ctxt->resDoc || !ctxt->resDoc->children ||
     !ctxt->resDoc->children->children)
    throw jxtlib_exception(STDLOG,"Ошибка программы!");
  return ctxt->resDoc;
}

extern "C" xmlNodePtr getResNode()
{
  XMLRequestCtxt *ctxt=getXmlCtxt();
  if(!ctxt || !ctxt->resDoc || !ctxt->resDoc->children ||
     !ctxt->resDoc->children->children)
    throw jxtlib_exception(STDLOG,"Ошибка программы!");
  return ctxt->resDoc->children->children;
}

extern "C" void setResDoc(xmlDocPtr newResDoc)
{
  XMLRequestCtxt *ctxt=getXmlCtxt();
  if(!ctxt)
    throw jxtlib_exception(STDLOG,"Ошибка программы!");
  xmlFreeDoc(ctxt->resDoc);
  ctxt->resDoc=newResDoc;
  return;
}

extern "C" xmlDocPtr getReqDoc()
{
  XMLRequestCtxt *ctxt=getXmlCtxt();
  if(!ctxt || !ctxt->reqDoc)
    throw jxtlib_exception(STDLOG,"Ошибка программы!");
  return ctxt->reqDoc;
}

void XmlCtxtHook_inner()
{
  XMLRequestCtxt &xmlRC=*getXmlCtxt();
  if(!xmlRC.resDoc || !xmlRC.resDoc->children ||
     !xmlRC.resDoc->children->children)
  {
    //ProgTrace(TRACE1,"resDoc's spoiled!!!");
    xmlFreeDoc(xmlRC.resDoc);  // it may be created so - free
    xmlRC.resDoc=createExceptionDoc("Ошибка программы!",0);
  }
  else
  {
    // if no 'handle' attribute was set in handler, we set it here
    xmlNodePtr resNode;
    auto tmp=getprop(resNode=xmlRC.resDoc->children->children,"handle");
    //ProgTrace(TRACE1,"getcurhan='%s'",getCurrHandle(xmlRC.pult.c_str()));
    if(!tmp)
      xmlSetProp(resNode,"handle",getCurrHandle(xmlRC.pult.c_str()));
  }
}

void XmlCtxtHook()
{
  ProgTrace(TRACE2,"XmlCtxtHook");
  XmlCtxtHook_inner();
  SaveContextsHook();
}

void XmlCtxtHookNoWrite()
{
  ProgTrace(TRACE2,"XmlCtxtHookNoWrite");
  XmlCtxtHook_inner();
  JxtContext::JxtContHolder::Instance()->reset();
}

void addXmlBM(XMLRequestCtxt &xmlRC)
{
  if(xmlRC.lastBottomMessage())
  {
    xmlNodePtr resNode=xmlRC.resDoc->children->children;
    string msg=xmlRC.lastBottomMessage()->getMsg();
    int is_err=xmlRC.lastBottomMessage()->isError();
    xmlNodePtr errNode=findNode(findNode(resNode,"command"), is_err?"error":"message");
    auto tmp=getText(errNode);
    if(!tmp || msg!=tmp)
    {
      tst();
      if(errNode && (!tmp || tmp[0]=='\0'))
        xmlNodeSetContent(errNode,msg);
      else
        xmlNewTextChild(getNode(resNode,"command"),NULL,
                        is_err?"error":"message",msg);
    }
  }
}

XMLRequestCtxt *getXmlCtxt_(const char *file, int line)
{
  ProgTrace(TRACE5,"getXmlCtxt() called from %s:%i",file,line);
  return jxtlib::JXTLib::Instance()->GetCallbacks()->GetXmlRequestCtxt();
//  return XMLRequestCtxt::Instance();
}

