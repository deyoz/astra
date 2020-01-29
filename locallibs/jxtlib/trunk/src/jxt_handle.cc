#ifdef HAVE_MPATROL
#include <mpatrol.h>
#endif

//#include <libxml/tree.h>
#include <string>

#include <serverlib/string_cast.h>

#define NICKNAME "ILYA"
#define NICKTRACE ILYA_TRACE
#include <serverlib/test.h>

#include "jxt_cont.h"
#include "jxt_handle.h"

using namespace std;
using namespace JxtContext;

ContRec::ContRec(const std::string &_name, const std::string &_val, int _len)
{
  name=_name;
  if(_len<0)
    value=_val;
  else
    value=_val.substr(0,_len);
}
ContRec::ContRec(const std::string &_name, int _val):
   name(_name),value(HelpCpp::string_cast(_val))
{}

namespace JxtHandles
{
  void closeCurrentJxtHandle()
  {
    JxtCont *jc=getJxtContHandler()->currContext();
    int par_handle=jc->readInt("PARENT_HANDLE");
    // delete current context and set it's parent current
    getJxtContHandler()->deleteContext(jc->getHandle(),par_handle);
    // mark new current handle as current in syscontext
    getJxtContHandler()->sysContext()->write("HANDLE",par_handle);
  }

  void closeJxtHandleByNum(int handle)
  {
    JxtCont *jc=getJxtContHandler()->getContext(handle);
    int par_handle=jc->readInt("PARENT_HANDLE");
    // delete current context and set it's parent current
    getJxtContHandler()->deleteContext(jc->getHandle(),par_handle);
    // mark new current handle as current in syscontext
    getJxtContHandler()->sysContext()->write("HANDLE",par_handle);
  }

  int getFreeJxtHandle()
  {
    // Не делаем контекст текущим, просто возвращаем его номер
    return getJxtContHandler()->
             getFreeContext(JxtContHandler::JUST_CREATE)->getHandle();
  }
  void createNewJxtHandle(int handle)
  {
    JxtContHandler *jch=getJxtContHandler();
    JxtCont *jc_sys=jch->sysContext();
    int current_is_modal=jch->currContext()->readInt("IS_MODAL");
    int old_handle=jc_sys->readInt("HANDLE");
    jc_sys->write("HANDLE",handle);
    jch->setCurrentContext(handle)->write("PARENT_HANDLE",old_handle)
       ->write("IS_MODAL",current_is_modal);
  }

  void createNewJxtHandle()
  {
    JxtContHandler *jch=getJxtContHandler();
    JxtCont *jc_sys=jch->sysContext();
    int old_handle=jc_sys->readInt("HANDLE");
    int current_is_modal=jch->currContext()->readInt("IS_MODAL");
    jc_sys->write("HANDLE",jch->getFreeContext(JxtContHandler::SET_CURRENT)
                              ->write("PARENT_HANDLE",old_handle)
                              ->write("IS_MODAL",current_is_modal)
                              ->getHandle());
  }

  void duplicateJxtHandle()
  {
    JxtContHandler *jch=getJxtContHandler();
    JxtCont *jc_old=jch->currContext();
    int old_handle=jc_old->getHandle();
    JxtCont *jc_new=jch->getFreeContext(JxtContHandler::SET_CURRENT);
    jc_old->readAll();
    jc_old->copyData(jc_new);
    jc_new->write("PARENT_HANDLE",old_handle);
  }

  int numberOfOpenJxtHandles()
  {
    return getJxtContHandler()->getNumberOfContexts()-1;
  }

  int getJxtHandleNumByIface(const std::string &iface_id)
  {
    JxtContHandler *jch=getJxtContHandler();
    vector<int> contexts_vec;
    jch->getNumberOfContexts(contexts_vec);
    for(int i=jch->getMaxContextsNum();i>=1;--i)
    {
      if(contexts_vec[i]!=0) // there is such handle opened
      {
        if(jch->getContext(i)->read("IFACE")==iface_id)
          return i;
      }
    }
    return -1;
  }

  void getJxtHandleByIface(const std::string &iface_id)
  {
    int matching_handle=JxtHandles::getJxtHandleNumByIface(iface_id);
    if(matching_handle<=0)
      JxtHandles::createNewJxtHandle();
    else
      JxtHandles::createNewJxtHandle(matching_handle);
  }

  int findJxtHandleByParams(HandleParams *hp)
  {
    vector<ContRec> &data=hp->getData();
    int data_size=data.size();
    JxtContHandler *jch=getJxtContHandler();
    vector<int> contexts_vec;
    jch->getNumberOfContexts(contexts_vec);
    for(int i=jch->getMaxContextsNum();i>=1;--i)
    {
      if(contexts_vec[i]==0) // no such handle opened
        continue;
      JxtCont *jc=jch->getContext(i);
      int j;
      for(j=0;j<data_size;j++)
      {
        if(jc->read(data[j].getName())!=data[j].getValue())
          break;
      }
      if(j==data_size) // found exactly what we wanted
        return i;
    }
    return -1;
  }

  int getJxtHandleByParams(HandleParams *hp)
  {
    int matching_handle=JxtHandles::findJxtHandleByParams(hp);
    if(matching_handle<=0)
    {
      JxtHandles::createNewJxtHandle();
      matching_handle=0;
    }
    else
      JxtHandles::createNewJxtHandle(matching_handle);
    return matching_handle;
  }

  int getCurrJxtHandle()
  {
    return getJxtContHandler()->currContext()->getHandle();
  }

} // namespace JxtHandles

extern "C" void closeHandle(const char *pult)
{
  ProgTrace(TRACE2,"closeHandle: pult='%s'",pult);
  JxtHandles::closeCurrentJxtHandle();
}

extern "C" void closeHandleByNum(const char *pult, int num)
{
  ProgTrace(TRACE2,"closeHandleByNum: pult='%s', num=%i",pult,num);
  JxtHandles::closeJxtHandleByNum(num);
}

extern "C" int getFreeHandle(const char *pult)
{
  ProgTrace(TRACE2,"getFreeHandle '%s'",pult);
  return JxtHandles::getFreeJxtHandle();
}

extern "C" int createHandleByNum(const char *pult, int num)
{
  ProgTrace(TRACE2,"createHandleByNum(%i)",num);
  JxtHandles::createNewJxtHandle(num);
  return 0;
}

extern "C" int _createNewHandle(const char *file, int line, const char *pult)
{
  ProgTrace(TRACE2,"createHandle: pult='%s'",pult);
  ProgTrace(TRACE2,"  called from %s:%i",file,line);
  JxtHandles::createNewJxtHandle();
  return 0;
}

extern "C" int DuplicateHandle()
{
  ProgTrace(TRACE2,"DuplicateHandle");
  JxtHandles::duplicateJxtHandle();
  return 0;
}

extern "C" const char *getCurrHandle(const char *pult)
{
  static string ch=string();
  ch=HelpCpp::string_cast(getJxtContHandler()->currContext()->getHandle());
  return ch.c_str();
}

extern "C" int getHandleByIface(const char *iface)
{
  JxtHandles::getJxtHandleByIface(iface);
  return getJxtContHandler()->currContext()->getHandle();
}
