#include <string>
#include <serverlib/str_utils.h>
#include <serverlib/helpcpp.h>
#include <serverlib/cursctl.h>
#include <tclmon/tcl_utils.h>

#define NICKNAME "ILYA"
#include <serverlib/test.h>
#include "jxt_xml_cont.h"
#include "xml_msg.h"
#include "xml_tools.h"
#include "jxt_cont.h"
#include "gettext.h"
#include "jxt_handle.h"
#include "JxtInterface.h"
#include "jxtlib_db_callbacks.h"

#include "jxtlib.h"
#include "jxt_tools.h"
#include "cont_tools.h"
#include "xmllibcpp.h"
#include "xml_stuff.h"
#include "xml_cpp.h"

using namespace std;
using namespace jxtlib;
using namespace JxtHandles;

void UpdateXmlData_inner(XMLRequestCtxt *ctxt, xmlNodePtr reqNode,
                         xmlNodePtr resNode, const string &type,
                         const string &id)
{
  ProgTrace(TRACE2,"UpdateXmlData_inner (id='%s', type='%s')",id.c_str(),
            type.c_str());

  if(type!="interface" && type!="ipart" && type!="ppart" && type!="type")
  {
    ProgTrace(TRACE1,"Unknown type: <%s>",type.c_str());
    throw jxtlib_exception(STDLOG,"Некорректный формат запроса!");
  }

  xmlNodePtr node=reqNode->children;
  if(xmlStrcmp(node->name,"update")!=0 && xmlStrcmp(node->name,"get")!=0)
    throw jxtlib_exception(STDLOG,"Neither <update> nor <get> found!");

  if(id.empty() || id.size()>50)
  {
    ProgTrace(TRACE1,"id='%s'",id.c_str());
    throw jxtlib_exception(STDLOG,"Неверный формат запроса");
  }

  long terminal_ver=atoiNVL(getprop(node,"ver"),0);
  long server_ver=getXmlDataVer_inner(type,id,false);
  string xml;
  string cached_iface; // we'll try to find cached iface with iparts in it
  long answer_ver=server_ver;
  bool no_iparts_mode=false;

  if(type=="interface")
  {
    no_iparts_mode=true;
    answer_ver=getXmlDataVer_inner(type,id,true); // версия для ответа
    cached_iface=getCachedIfaceWoIparts(id,answer_ver);
  }

  /* we get data from DB only if server has newer version than terminal does */
  if(cached_iface.empty())
  {
    if(answer_ver>terminal_ver)
      xml.assign(GetXmlData(type.c_str(),id.c_str(),server_ver));
    else throw jxtlib_exception(STDLOG,
                         "Server has no newer version than terminal does!");
  }
  else
    xml.assign(cached_iface);

  ctxt->set_donotencode2UTF8(1);
  auto newDoc = xml_parse_memory(xml);
  if(!newDoc)
    throw jxtlib_exception(STDLOG, "Запрошенный документ поврежден и не может быть скачан");

  xmlUnlinkNode(resNode);
  xmlFreeNode(resNode);
  xmlNodePtr newAnsNode=newDoc->children->children;
  xmlUnlinkNode(newAnsNode);
  xmlAddChild(ctxt->resDoc->children,newAnsNode);
  resNode=ctxt->resDoc->children->children;

  if(!no_iparts_mode) // type!="interface"
    return;
  if(!cached_iface.empty()) // cached iface containing iparts already created
  {
    ProgTrace(TRACE1,"Got cached iface '%s'",id.c_str());
    return;
  }

  auto iparts = JxtlibDbCallbacks::instance()->getIparts(id);
  for(auto &&ipart_name: iparts)
  {
    ProgTrace(TRACE5,"insert ipart '%s' into iface '%s'",ipart_name.c_str(),
              id.c_str());
    mergeIpartIntoIface(resNode,ipart_name);
  }
  xmlSetProp(resNode->children,"ver",HelpCpp::string_cast(answer_ver));
  setCachedIfaceWoIparts(resNode->children,id,answer_ver);
}

void UpdateXmlData(XMLRequestCtxt *ctxt, xmlNodePtr reqNode,
                   xmlNodePtr resNode)
{
  ProgTrace(TRACE1,"UpdateXmlData");

  if(isempty(reqNode) || !reqNode->name)
    throw jxtlib_exception(STDLOG,"Неверный формат запроса!");

  string type(reinterpret_cast<const char*>(reqNode->name));
  string id=getStrPropFromXml(reqNode->children,"id");
  return UpdateXmlData_inner(ctxt,reqNode,resNode,type,id);
}

void quit_window(XMLRequestCtxt *ctxt, xmlNodePtr reqNode,
                 xmlNodePtr resNode)
{
  closeWindow(resNode);
  /*
  xmlNodePtr comNode,killNode;

  comNode=getNode(resNode,"command");
  killNode=getNode(comNode,"kill");
  xmlSetProp(killNode,"handle",ctxt->GetQueryHandle());
  closeCurrentJxtHandle();
  xmlSetProp(resNode,"handle",getCurrJxtHandle());
  */
}

void close_handle(XMLRequestCtxt *ctxt, xmlNodePtr reqNode,
                             xmlNodePtr resNode)
/* Два варианта запроса: */
/* 1. <query handle="n"> <close/> </query> */
/* 2. <query handle="0">     */
/*      <close>              */
/*        <handle>k</handle> */
/*        ...                */
/*      </close>             */
/*    <query>                */
{
  ProgTrace(TRACE5,"closeNode->name='%s'",reqNode->name);
  if(isempty(reqNode)) /* 1st type of query */
  {
    closeHandle(ctxt->pult.c_str()); /* closes current handle */
  }
  else /* 2nd type of query */
  {
    tst();
    xmlNodePtr node=reqNode->children;
    while(node!=NULL)
    {
      if(xmlStrcmp(node->name,"handle")!=0)
      {
        ProgTrace(TRACE1,"Wrong tag name found: <%s>",node->name);
        throw jxtlib_exception(STDLOG,"Неверный формат запроса");
      }
      auto buf=gettext(node);
      if(!buf || atoiNVL(buf,-100)==-100)
      {
        ProgTrace(TRACE1,"Tag <%s> content is invalid !!!",node->name);
        throw jxtlib_exception(STDLOG,"Неверный формат запроса");
      }
      closeHandleByNum(ctxt->pult.c_str(),atoi(buf));
      node=node->next;
    }
  }
  // Если закрыли последнее окно, откроем окно со списком задач
  if(getNumberOfHandles(getXmlCtxt()->pult.c_str())==0)
		JxtInterfaceMng::Instance()->OnEvent("", "init_main", ctxt, reqNode, resNode);
//		JxtInterfaceMng::OnEvent("", "init_main", ctxt, reqNode, resNode);
  else
  {
    // Если закрытое окно - не последнее, пришлем терминалу весточку, чтобы
    // ему было приятнее
    addXmlMessage("");
  }
  return;
}

xmlNodePtr closeWindow(xmlNodePtr resNode)
{
  xmlNodePtr killNode=NULL;

  killNode=newChild(getNode(resNode,"command"),"kill");
  xmlSetProp(killNode,"handle",getCurrJxtHandle());

  closeCurrentJxtHandle();
  return killNode;
}

void UpdateData(XMLRequestCtxt *ctxt, xmlNodePtr reqNode,
                           xmlNodePtr resNode)
{
  ProgTrace(TRACE2,"UpdateData");

  xmlNodePtr updNode=reqNode->children;
  string id=getStrPropFromXml(updNode,"id");
  long term_ver=atoiNVL(getprop(updNode,"ver"),0);

  updateJxtData(id.c_str(),term_ver,resNode);
}
