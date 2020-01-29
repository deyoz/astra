#ifdef HAVE_MPATROL
#include <mpatrol.h>
#endif
#include <string>
#include <libxml/tree.h>
#define NICKNAME "ILYA"
#define NICKTRACE ILYA_TRACE
#include <serverlib/test.h>
#include <serverlib/cursctl.h>
#include "xml_tools.h"
#include "xmllibcpp.h"
#include "jxt_bgnd.h"
#include "jxtlib.h"

using namespace std;
using namespace OciCpp;
namespace
{
class JxtBgndTasksException : public jxtlib::jxtlib_exception
{
  public:
    JxtBgndTasksException(const std::string &msg) : jxtlib::jxtlib_exception(msg) {}
};
}


void resetJxtCheckTimeout(const std::string &pult)
{
  JxtBgndHandler(getResNode(),pult).delAllTasks();
}

void addJxtBgndTask(const std::string &pult, const std::string &task_id,
                    int check_timeout)
{
  JxtBgndHandler(getResNode(),pult).addTask(task_id,check_timeout);
}

void delJxtBgndTask(const std::string &pult, const std::string &task_id)
{
  JxtBgndHandler(getResNode(),pult).delTask(task_id);
}

extern "C" void resetJxtCheckTimeoutC(const char *pult)
{
  if(!pult)
  {
    ProgError(STDLOG,"resetJxtCheckTimeout(NULL)");
    throw JxtBgndTasksException("resetJxtCheckTimeout(NULL)");
  }
  resetJxtCheckTimeout(pult);
}

extern "C" void addJxtBgndTaskC(const char *pult, const char *task_id, int check_timeout)
{
  if(!pult)
  {
    ProgError(STDLOG,"resetJxtCheckTimeout(NULL)");
    throw JxtBgndTasksException("resetJxtCheckTimeout(NULL)");
  }
  if(!task_id)
  {
    ProgError(STDLOG,"addJxtBgndTaskC(NULL)");
    throw JxtBgndTasksException("addJxtBgndTaskC(NULL)");
  }
  addJxtBgndTask(pult,task_id,check_timeout);
}

extern "C" void delJxtBgndTaskC(const char *pult, const char *task_id)
{
  if(!pult)
  {
    ProgError(STDLOG,"resetJxtCheckTimeout(NULL)");
    throw JxtBgndTasksException("resetJxtCheckTimeout(NULL)");
  }
  if(!task_id)
  {
    ProgError(STDLOG,"delJxtBgndTaskC(NULL)");
    throw JxtBgndTasksException("delJxtBgndTaskC(NULL)");
  }
  delJxtBgndTask(pult,task_id);
}

JxtBgndTasks *JxtBgndTasks::getInstance(void)
{
  static JxtBgndTasks *instance_=0;
  if(!instance_)
    instance_=new JxtBgndTasks;
  return instance_;
}

void JxtBgndHandler::loadData()
{
  ProgTrace(TRACE2,"loadData");
  tasks.clear();
  TaskParams tp;
  CursCtl c=make_curs("SELECT CHECK_TIME, TASK_ID FROM JXT_BGND WHERE TERM=:V1");
  c.bind(":V1",pult).def(tp.check_timeout).def(tp.task_id).exec();
  while(!c.fen())
  {
    tp.proc=JxtBgndTasks::getInstance()->getProc(tp.task_id);
    addTask(tp,false);
  }
  data_loaded=true;
}

void JxtBgndHandler::saveData(const TaskParams &tp) const
{
  ProgTrace(TRACE2,"saveData");
  CursCtl c=make_curs("DELETE FROM JXT_BGND WHERE TERM=:V1 AND TASK_ID=:V2");
  c.bind(":V1",pult).bind(":V2",tp.task_id).exec();

  CursCtl c2=make_curs("INSERT INTO JXT_BGND (CHECK_TIME,START_TIME,TASK_ID,"
    "TERM) VALUES (:ct,SYSDATE,:t_id,:pul)");
  c2.bind(":ct",tp.check_timeout).bind(":t_id",tp.task_id).bind(":pul",pult).
     exec();
}

void JxtBgndHandler::delTask(const std::string &task_id)
{
  ProgTrace(TRACE2,"delTask(%s)",task_id.c_str());
  CursCtl c=make_curs("DELETE FROM JXT_BGND WHERE TERM=:V1 AND TASK_ID=:V2");
  c.bind(":V1",pult).bind(":V2",task_id).exec();

  tasks.clear();
  data_loaded=false;
  loadData();
  setJxtCheckTimeout();
}

void JxtBgndHandler::delAllTasks(int exp_time)
{
  ProgTrace(TRACE2,"delAllTasks(%i)",exp_time);
  CursCtl c=make_curs("DELETE FROM JXT_BGND WHERE TERM=:V1 AND "
                      "START_TIME<SYSDATE-:exp_time/(24*60)");
  c.bind(":V1",pult).bind(":exp_time",exp_time).exec();
  tasks.clear();
  data_loaded=false;
  setJxtCheckTimeout();
}

void JxtBgndHandler::addTask(const TaskParams &tp, bool save)
{
  ProgTrace(TRACE2,"addTask");
  if(tp.check_timeout>99)
  {
    ProgError(STDLOG,"Error: tp.check_timeout=%i",tp.check_timeout);
    throw JxtBgndTasksException("Background task check timeout is too large");
  }
  if(!tp.proc)
  {
    ProgError(STDLOG,"Error: tp.proc is NULL for task_id='%s'",tp.task_id.c_str());
        throw JxtBgndTasksException("Adding task with unknown id");
      }
      tasks[tp.task_id]=tp;
      if(save)
        saveData(tp);
    }

    void JxtBgndHandler::setJxtCheckTimeout(int ct)
    {
      xmlSetProp(getNode(getNode(resNode,"command"),"set_update"),"timeout",ct<0?0:ct);
    }

    void JxtBgndHandler::checkReady(XMLRequestCtxt *ctxt, xmlNodePtr reqNode,
                                    xmlNodePtr resNode)
    {
      ProgTrace(TRACE2,"checkReady: tasks.size()=%zd", tasks.size());
      if(!data_loaded)
        loadData();
      for(std::map <std::string, TaskParams>::iterator i=tasks.begin();i!=tasks.end();++i)
      {
        if(i->second.proc)
        {
          i->second.proc(ctxt,reqNode,resNode);
        }
      }
    }

    int JxtBgndHandler::getMinCheckTimeout(void)
    {
      if(!data_loaded)
        loadData();
      int min_to=999;
      for(std::map <std::string, TaskParams>::iterator i=tasks.begin();i!=tasks.end();++i)
      {
        if(min_to>i->second.check_timeout)
          min_to=i->second.check_timeout;
      }
      return tasks.empty()?0:min_to;
    }

