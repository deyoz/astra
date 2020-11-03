#ifndef __SUPERVISOR_H__
 #define __SUPERVISOR_H__


#include "tclmon.h"
#include "mes.h"
#include "startparam.h"

namespace Supervisor {

struct ProcS
{
    int proc_num;
    pid_t ppid;
    int fd;
    /*time_t time;*/
    short proc_flag; /* see PROC_FL_... */
    time_t kill_time;
    time_t delay;
    int action;
    int cutlog;
    QueMbl proc_que_out;
    _MesBl_* p_mbl;
    GroupDescription::PROCESS_LEVEL level;

    ProcS():proc_num(0), ppid(-1), action(A_NONE), proc_que_out(0), p_mbl(0), level(GroupDescription::LEVEL_OTHER)
    {
        fd = -1;
    }

    ProcS(int processNum):proc_num(processNum), ppid(-1), action(A_NONE), proc_que_out(0), p_mbl(0), level(GroupDescription::LEVEL_OTHER)
    {
        fd = -1;
    }

private:
    //no copy
    ProcS(const ProcS&);
    ProcS& operator=(const ProcS&);
};

class TclObjHolder
{
public:
    explicit TclObjHolder(Tcl_Obj* o) :
        obj_(o)
    {
        Tcl_IncrRefCount(obj_);
    }

    ~TclObjHolder() {
        Tcl_DecrRefCount(obj_);
    }

    Tcl_Obj* get() const {
        return obj_;
    }

private:
    Tcl_Obj* obj_;
};

int watch_main (int argc, char *argv[], int (*app_init)(Tcl_Interp *),
    int (*app_start)(Tcl_Interp *), void (*before_exit)(void) );

int getExtendDiagnosticLog();

int run(int argc, char **argv);

} //namespace Supervisor

#endif /*  __SUPERVISOR_H__*/
