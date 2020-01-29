#ifndef __JXT_BGND_H__
#define __JXT_BGND_H__

#include <libxml/tree.h>

/*****************************************************************************/
/*********************      C functions     **********************************/
/*****************************************************************************/

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

void addJxtBgndTaskC(const char *pult, const char *task_id, int check_timeout);
void delJxtBgndTaskC(const char *pult, const char *task_id);

void resetJxtCheckTimeoutC(const char *pult);

#ifdef __cplusplus
}
#endif /* __cplusplus */

/*****************************************************************************/
/************************   C++ functions   **********************************/
/*****************************************************************************/

#ifdef __cplusplus
#include <string>
#include <vector>
#include <map>
#include "jxt_xml_cont.h"

void resetJxtCheckTimeout(const std::string &pult);
void addJxtBgndTask(const std::string &pult, const std::string &task_id, int check_timeout);
void delJxtBgndTask(const std::string &pult, const std::string &task_id);

class JxtBgndTasks
{
  private:
    std::map<std::string, JxtProcFuncType *> tasks;
    JxtBgndTasks()
    {
      tasks.clear();
    }
  public:
    ~JxtBgndTasks()
    {
    }
    static JxtBgndTasks *getInstance(void);
    void addTask(const std::string &t_id, JxtProcFuncType *f)
    {
      tasks[t_id]=f;
    }
    JxtProcFuncType *getProc(const std::string &t_id)
    {
      std::map<std::string, JxtProcFuncType *>::iterator pos=tasks.find(t_id);
      if(pos==tasks.end())
        return 0;
      else
        return pos->second;
    }
};

class JxtBgndHandler
{
  private:
    xmlNodePtr resNode;
    std::string pult;
    struct TaskParams
    {
      std::string task_id;
      JxtProcFuncType *proc;
      int check_timeout;
      TaskParams()
      {
        proc=0;
      }
      TaskParams(const std::string &t_id, JxtProcFuncType *fun, int ct) :
        task_id(t_id), proc(fun), check_timeout(ct)
      {
      }
    };
    std::map <std::string, TaskParams> tasks;
    bool data_loaded;
    void addTask(const TaskParams &tp, bool save=true);
  public:
    explicit JxtBgndHandler(xmlNodePtr ansNode, const std::string &pul) :
                           resNode(ansNode), pult(pul)
    {
      data_loaded=false;
    }
    ~JxtBgndHandler()
    {
    }
    void loadData(); // read bgnd tasks data from DB
    void addTask(const std::string &task_id, int ct, bool no_check=false)
    {
      if(!no_check && !data_loaded)
        loadData();
      addTask(TaskParams(task_id,(JxtBgndTasks::getInstance())->getProc(task_id),ct));
      setJxtCheckTimeout();
    }
    void delTask(const std::string &task_id);
    void delAllTasks(int exp_time=0);
    void saveData(const TaskParams &tp) const;
    int getMinCheckTimeout(void);
    void setJxtCheckTimeout(int ct);
    void setJxtCheckTimeout()
    {
      setJxtCheckTimeout(getMinCheckTimeout());
    }
    void checkReady(XMLRequestCtxt *ctxt, xmlNodePtr reqNode,
                    xmlNodePtr resNode);
};

#endif /* __cplusplus */

#endif /* __JXT_BGND_H__ */
