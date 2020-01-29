#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <map>
#include <memory>

#include "supervisor.h"
#include "monitor_ctl.h"
#include "moncmd.h"
#include "lwriter.h"
#include "log_queue.h"
#include "testmode.h"
#include "monitor.h"
#include "string_cast.h"
#include "helpcpp.h"

#define NICKNAME "KONST"
#include "slogger.h"

extern int stopped_by_signal;

void reset_potok_head(void);
int get_que_num_by_grp_name_proc(char *grp_name);
void add_zapr_to_potok_proc(int n_zapr, int type_zapr, int grp_zapr);
void get_potok_all_proc(char *potok_str, int potok_size);
void reset_potok_proc(void);
char *get_potok_by_que_name_proc(char *que_name);
void finish (int);
void constructCsaControl(_CsaControl_* rawCsa);
void fill_set_flag(_Message_ *p_mes, int pid_p, int TO_restart, int n_zapr, int type_zapr, const char* const subgroup);

namespace Supervisor {

class CsaHolder
    : private comtech::noncopyable
{
public:
    explicit CsaHolder(int key);

    ~CsaHolder();

    _CsaControl_* csa() {
        return csa_;
    }

private:
    friend int exit_main_proc(Tcl_Interp*, bool);

    void destroy() {
        if(0 < id_) {
            shmdt(csa_);
            csa_ = 0;
            shmctl(id_, IPC_RMID, 0);
            id_ = -1;
        }
    }

private:
    _CsaControl_* csa_;
    int id_;
};

class SemHolder
    : private comtech::noncopyable
{
public:
    explicit SemHolder(const int key) {
        const int flag = 0777;

        id_ = semget(key, 1, flag | IPC_CREAT | IPC_EXCL);
        if (id_ < 0) {
            id_ = semget(key, 1, flag);
            if (id_ < 0) {
                throw std::runtime_error("Can't get semaphore for write context size");
            }
        }

        const short initValue[1] = {1};
        semctl(id_, 1, SETALL, initValue);
    }

    ~SemHolder() {
        destroy();
    }

private:
    friend int exit_main_proc(Tcl_Interp*, bool);

    void destroy() {
        if (0 < id_) {
            semctl(id_, 0, IPC_RMID);
            id_ = -1;
        }
    }

private:
    int id_;
};
/*****************************************************************/
static const int NPROC = 500;

typedef ProcS ProcessTable[NPROC];

static std::unique_ptr<CsaHolder> csaHolder;
static std::unique_ptr<SemHolder> semHolder;
static ProcessTable ProcTab;
static std::map<int, time_t> finalProcess;
static bool applyingConfigDiff;
static int MaxFdBeforeTclMain = -1;
static int batch_killing;
static void (*before_exit_main_proc)(void);
static char **Argv;
static int  Argc;
static int (*App_init)(Tcl_Interp *);
static int (*App_start)(Tcl_Interp *);
pid_t Pid_p;
static int TclmonTestingMode;
static std::set<int> restartedPids;

_Fields_ f_action[N_ACTION]=
{
 {A_NONE          ,"N"},/*None*/
 {A_CREATE        ,"C"},/*Create*/
 {A_RESTART       ,"R"},/*Restart*/
 {A_STOP          ,"S"},/*Stop*/
 {A_WORK          ,"W"},/*Work*/
 {A_FINAL         ,"F"}, /*Final state*/
 {A_FORCED_RESTART,"FR"} /*Forced restart*/
};
/*****************************************************************/
CsaHolder::CsaHolder(int key)
{
    static const int getFlag = 00600;
    int atFlag = 0;

    id_ = shmget(key, sizeof(_CsaControl_), getFlag | IPC_CREAT | IPC_EXCL);
    if(id_ < 0) {
        id_ = shmget(key, sizeof(_CsaControl_), getFlag);
        if(id_ < 0) {
            throw std::runtime_error("Can't create shared memory segment");
        }
    }
    csa_ = static_cast<_CsaControl_*>(shmat(id_, 0, atFlag));
    if(reinterpret_cast<void*>(-1) == csa_) {
        shmctl(id_, IPC_RMID, 0);
        throw std::runtime_error("Can't create shared memory segment");
    }

    memset(reinterpret_cast<char*>(csa_), 0, sizeof(_CsaControl_));
    constructCsaControl(csa_);
}

CsaHolder::~CsaHolder()
{
    destroy();
}
/*****************************************************************/
class FinalProcessCompare
{
public:
    typedef std::pair<int, time_t> first_argument_type;
    typedef int second_argument_type;
    typedef bool result_type;


public:
    bool operator()(const first_argument_type& lhs, const second_argument_type& rhs) const {
        return rhs == lhs.second;
    }
};
/*****************************************************************/
static void put_message_to_que( _Message_ &p_mes, ProcS &pr );
static const char* read_cur_req(Tcl_Interp* interp, int pid);
static int batchProcKilling();
static int set_stop_pid_process(Tcl_Interp *interp, ProcS &pr);
static void write_needset_flag_tclmon(Tcl_Interp *interp, ProcS &p_proctab_to);
static void write_set_flag_tclmon(Tcl_Interp *interp, int pid_p, int TO_restart, 
                           int n_zapr, int type_zapr);
static int lock_startup( ClientData cl, Tcl_Interp *interp, 
                         int objc,Tcl_Obj* CONST objv[]);
static int watch( Tcl_Interp* interp);
static int tcl_set_testing_mode( ClientData cl, Tcl_Interp *interp, 
                         int objc,Tcl_Obj* CONST objv[]);
static int stop_obr_tcl(ClientData cl,Tcl_Interp *interp,int objc,
        Tcl_Obj* CONST objv[]);
static int tcl_process_final(ClientData cl, Tcl_Interp* interp, int objc, Tcl_Obj* CONST objv[]);
static int restart_obr_tcl(ClientData cl, Tcl_Interp* interp,int objc, Tcl_Obj* CONST objv[]);
static int command_obr_tcl(ClientData cl, Tcl_Interp* interp,int objc, Tcl_Obj* CONST objv[]);
static int add_zapr_to_potok_tcl(ClientData cl, Tcl_Interp* interp, int objc, Tcl_Obj* CONST objv[]);
static int get_potok_by_que_name_tcl(ClientData cl, Tcl_Interp* interp, int objc, Tcl_Obj* CONST objv[]);
static int get_potok_tcl(ClientData cl, Tcl_Interp *interp, int objc, Tcl_Obj* CONST objv[]);
static int get_que_num_by_grp_name_tcl(ClientData cl, Tcl_Interp* interp, int objc, Tcl_Obj* CONST objv[]);
static int show_bufs_tcl(ClientData cl, Tcl_Interp* interp, int objc, Tcl_Obj* CONST objv[]);
static int kill_proc (Tcl_Interp *interp , ProcS &pr , int s , int action, time_t kill_time, time_t delay);
static int kill_proc_restart_stop(Tcl_Interp *interp , ProcS &pr , int s , int action, time_t kill_time, time_t delay);
static void delete_proc(Tcl_Interp *interp, ProcS &pr);
int exit_main_proc(Tcl_Interp *interp, bool restart);
static void obr_cmd_set_flag(Tcl_Interp *interp, char *str_cmd, int len_cmd );
static void obr_cmd_command(Tcl_Interp *interp, ProcS &p_proctab, _MesBl_ *p_mbl );
static int start_new(int argc, const char* objv[], Tcl_Interp* interp, const GroupDescription::PROCESS_LEVEL pl);
static int stop_old(int handlersCount, const std::string& startLine, Tcl_Interp* interp);
static int restartHandlers(Tcl_Interp* interp);
/*****************************************************************/
static const int FD_TABLE_SIZE = 512;
static int fdtoclose[FD_TABLE_SIZE]={
-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
};
static int fdtoshutdown[FD_TABLE_SIZE]={
-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
};

static void addToCloseFd(int fd,int shut)
{
  for(int i=0;i<FD_TABLE_SIZE;i++){
    if(fdtoclose[i]==-1){
      fdtoclose[i]=fd;
      break;
    }
  }
  if(shut){
    for(int i=0;i<FD_TABLE_SIZE;i++){
      if(fdtoshutdown[i]==-1){
        fdtoshutdown[i]=fd;
        break;
      }
    }
  }
}

static void closeAndShut(void)
{
  for(int i=0;i<FD_TABLE_SIZE;i++){
    if(fdtoshutdown[i]!=-1){
      shutdown(fdtoshutdown[i],2);
      fdtoshutdown[i]=-1;
    }
  }
  for(int i=0;i<FD_TABLE_SIZE;i++){
    if(fdtoclose[i]!=-1){
      close(fdtoclose[i]);
      fdtoclose[i]=-1;
    }
  }
}

static int fdtocloseTclMain[FD_TABLE_SIZE]={
-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
};

static void addToCloseFdTclMain(int fd)
{
  for(int i=0;i<FD_TABLE_SIZE;i++){
    if(fdtocloseTclMain[i]==-1){
      fdtocloseTclMain[i]=fd;
      break;
    }
  }
}
static void closeAndShutTclMain(void)
{
  for(int i=0;i<FD_TABLE_SIZE;i++){
    if(fdtocloseTclMain[i]!=-1){
      close(fdtocloseTclMain[i]);
      fdtocloseTclMain[i]=-1;
    }
  }
}
/*****************************************************************/
class RunTimeChecker
{
    static const int CANNOT_GET = 10000000;

    std::string fileName_;

public:
    static RunTimeChecker& Instance();

    ///Before call this method must be called getLastRunTime(...) where set filename
    int markRunTime();

    int getLastRunTime(const char* file);

    static int tclGetLastRunTime(ClientData cl, Tcl_Interp* interp, int objc, Tcl_Obj* CONST objv[]);

private:
    explicit RunTimeChecker(): fileName_()
    {
    }

    //no copy
    RunTimeChecker(const RunTimeChecker&);

    RunTimeChecker& operator=(const RunTimeChecker&);
};

RunTimeChecker& RunTimeChecker::Instance()
{
    static RunTimeChecker instance;

    return instance;
}

int RunTimeChecker::markRunTime()
{
    static time_t t = -1;
    time_t cur;

    cur = time(0);
    if (t == (time_t)-1 || cur - t > 60) {
        t = cur;
        if (write_number_to_file(fileName_.c_str(), cur)<0) {

            return -1;
        }
    }

    return 0;
}

int RunTimeChecker::getLastRunTime(const char *file)
{
    static const char* errStr = "A T T E N T I O N !!\n"
                                "В Н И М А Н И Е !!\n"
                                "check system time!!!\n"
                                "%s\n"
                                "Проверьте что текущее время на этом компьютере установлено\n"
                                "правильно.\n"
                                "После установки правильного времени - сотрите файл %s\n"
                                "и запустите систему снова\n";

    long deftime = time(0);
    long ret = get_number_from_file(file, deftime, CANNOT_GET);

    fileName_ = file;
    if (ret == CANNOT_GET) {
        fprintf(stderr, errStr, "Нет возможности определить, когда в последний\n"
                                "раз запускалась система.", file);

        return -1;
    }

    ret = deftime - ret;
    if (ret < 0) {
        ret = -ret;
    }
    if (ret > 100000) {
        fprintf(stderr, errStr, "Возможно в компьютере установлено\n"
                                "неправильное время.", file);

        return -1;

    }

    return 0;
}

int RunTimeChecker::tclGetLastRunTime(ClientData cl, Tcl_Interp *interp, int objc, Tcl_Obj* CONST objv[])
{
    if (objc == 2) {
        if (RunTimeChecker::Instance().getLastRunTime(Tcl_GetString(objv[1]))<0) {
            Tcl_SetResult(interp, const_cast<char*>("check system time"), TCL_STATIC);

            return TCL_ERROR;
        }
    } else {
        Tcl_SetResult(interp, const_cast<char*>("needs filename"), TCL_STATIC);

        return TCL_ERROR;
    }

    return TCL_OK;
}
/*****************************************************************/
static int addProcess(Supervisor::GroupDescription::PROCESS_LEVEL pl, Tcl_Interp* interp, int objc, Tcl_Obj* CONST objv[])
{
    static const int ARG_HANDLERS_COUNT = 1;
    static const int ARG_START_LINE = 2;
    static const int ARG_PRIORITY = 3;
    int handlersCount;
    int priority;

    if (TCL_OK != Tcl_GetIntFromObj(interp, objv[ARG_HANDLERS_COUNT], &handlersCount)) {
        Tcl_AppendResult(interp, ". Error parsing handlers count!", NULL);

        return TCL_ERROR;

    }
    if ((Supervisor::GroupDescription::LEVEL_A == pl) && (1 != handlersCount)) {
        Tcl_ResetResult(interp);
        Tcl_AppendResult(interp, "For dispatcher handlers count must be 1.", NULL);

        return TCL_ERROR;

    }
    if (TCL_OK != Tcl_GetIntFromObj(interp, objv[ARG_PRIORITY], &priority)) {
         Tcl_AppendResult(interp, ". Error parsing group priority!", NULL);

        return TCL_ERROR;
    }

    Supervisor::StartParam::Instance().add(
            Supervisor::GroupDescription(
                handlersCount,
                Tcl_GetString(objv[ARG_START_LINE]),
                priority,
                pl
            )
    );

    return TCL_OK;
}
/*****************************************************************/
static int tcl_create_proc_grp(ClientData cl, Tcl_Interp *interp, int objc,Tcl_Obj* CONST objv[])
{
    return addProcess(Supervisor::GroupDescription::LEVEL_C, interp, objc, objv);
}
/*****************************************************************/
static int tcl_create_dispatcher(ClientData cl, Tcl_Interp* interp, int objc, Tcl_Obj* CONST objv[])
{
    return addProcess(Supervisor::GroupDescription::LEVEL_A, interp, objc, objv);
}
/*****************************************************************/
static int tcl_create_process(ClientData cl, Tcl_Interp* interp, int objc, Tcl_Obj* CONST objv[])
{
    return addProcess(Supervisor::GroupDescription::LEVEL_OTHER, interp, objc, objv);
}
/*****************************************************************/
static int tcl_create_profile_process(ClientData cl, Tcl_Interp* interp, int objc, Tcl_Obj* CONST objv[])
{
    return addProcess(Supervisor::GroupDescription::LEVEL_PROFILE, interp, objc, objv);
}
/*****************************************************************/
static int tcl_read_config(ClientData cl, Tcl_Interp* interp, int objc, Tcl_Obj* CONST objv[])
{
    if (applyingConfigDiff) {
        Tcl_ResetResult(interp);
        Tcl_AppendResult(interp, "Read config failed. Previous configuration changes not full applyed!");

        return TCL_ERROR;
    }

    StartParam::Instance().resetConfig();
    if (TCL_OK != Tcl_EvalFile(getTclInterpretator(), Argv[1])) {
        LogTrace(TRACE0) << "Can't Tcl_EvalFile(..., " << Argv[1] << "): " << Tcl_GetStringResult(interp);
        Tcl_ResetResult(interp);
        Tcl_AppendResult(interp, "Read config failed. Configuration no changed!");

        return TCL_ERROR;
    }

    return StartParam::Instance().applyDiff(start_new, stop_old);
}
/*****************************************************************/
int tcl_monitor_timeout_reset(ClientData cl, Tcl_Interp* interp, int objc, Tcl_Obj* CONST objv[])
{
    Tcl_ResetResult(interp);
    if (3 == objc) {
        int ppid;
        if (TCL_OK != Tcl_GetIntFromObj(interp, objv[1], &ppid)) {
            ProgTrace(TRACE1, "Invalid pid: %s", Tcl_GetString(objv[1]));
            Tcl_AppendResult(interp, "Invalid pid.", nullptr);
            return TCL_ERROR;
        }

        int newTimeout;
        if (TCL_OK != Tcl_GetIntFromObj(interp, objv[2], &newTimeout)) {
            ProgTrace(TRACE1, "Invalid timeout: %s", Tcl_GetString(objv[2]));
            Tcl_AppendResult(interp, "Invalid timeout value.", nullptr);
            return TCL_ERROR;
        }

        if (newTimeout < 0) {
            ProgTrace(TRACE1, "Invalid timeout: %d", newTimeout);
            Tcl_AppendResult(interp, "Invalid timeout value.", nullptr);
            return TCL_ERROR;
        }

        write_set_flag_tclmon(interp, ppid, newTimeout, 0, QUEPOT_ZAPR);

        ProgTrace(TRACE1, "New timeout value %d has been set.", newTimeout);

        return TCL_OK;
    }

    Tcl_AppendResult(interp, "Wrong arguments count, must be - monitor_timeout_reset <new timeout value>", nullptr);

    return TCL_ERROR;
}
/*****************************************************************/
int tcl_init(Tcl_Interp* interp)
{
    if (TCL_OK != Tcl_SetSystemEncoding(interp, "cp866")) {
        fprintf(stderr, "%s\n", Tcl_GetString(Tcl_GetObjResult(interp)));

        return -1;
    }
    if (!Tcl_CreateObjCommand(interp, "create_proc_grp", tcl_create_proc_grp, nullptr, 0)) {
        fprintf(stderr, "%s\n", Tcl_GetString(Tcl_GetObjResult(interp)));

        return -1;
    }
    if (!Tcl_CreateObjCommand(interp, "create_dispatcher", tcl_create_dispatcher, nullptr, 0)) {
        fprintf(stderr, "%s\n", Tcl_GetString(Tcl_GetObjResult(interp)));

        return -1;
    }
    if (!Tcl_CreateObjCommand(interp, "create_process", tcl_create_process, nullptr, 0)) {
        fprintf(stderr, "%s\n", Tcl_GetString(Tcl_GetObjResult(interp)));

        return -1;
    }
    if (!Tcl_CreateObjCommand(interp, "create_profile_process", tcl_create_profile_process, nullptr, 0)) {
        fprintf(stderr, "%s\n", Tcl_GetString(Tcl_GetObjResult(interp)));

        return -1;
    }
    if (!Tcl_CreateObjCommand(interp, "read_config", tcl_read_config, nullptr, 0)) {
        fprintf(stderr, "%s\n", Tcl_GetString(Tcl_GetObjResult(interp)));

        return -1;
    }
    if (!Tcl_CreateObjCommand(interp, "last_run_time", RunTimeChecker::tclGetLastRunTime, nullptr, 0)) {
        fprintf(stderr, "%s\n", Tcl_GetString(Tcl_GetObjResult(interp)));

        return -1;
    }
    if (!Tcl_CreateObjCommand(interp, "process_final", tcl_process_final, nullptr, 0)) {
        fprintf(stderr, "%s\n", Tcl_GetString(Tcl_GetObjResult(interp)));

        return -1;
    }
    if ( !Tcl_CreateObjCommand(interp, "monitor_timeout_reset", tcl_monitor_timeout_reset, nullptr, 0) ) {
        fprintf( stderr, "%s\n", Tcl_GetString(Tcl_GetObjResult(interp)) );

        return -1;
    }

    return 0;
}
/*****************************************************************/
int tcl_start(Tcl_Interp *interp)
{
    return Supervisor::StartParam::Instance().start(start_new);
}
/*****************************************************************/
void on_exit()
{
}
/*****************************************************************/
int run(int argc, char **argv)
{
    static const int MAX_ARGC = 1024 - 1;
    char tcl_mode_yes[] = "TCL_MODE=YES";

    check_risc_order();
    if (MAX_ARGC < argc) {
        exit(1);
    }

    if ( (1 < argc) && strstr(argv[1],".tcl") ) {
        if (!inTestMode()) {
            fprintf(stderr,"start in TCL_MODE\n");
        }
        putenv(tcl_mode_yes);
    } else {
        puts("Second argument must be config_file_name.tcl for supervisor");

        exit(1);
    }

    char* nullTerminateArgv[argc + 1];
    for(int i = 0; i < argc; ++i) {
        nullTerminateArgv[i] = argv[i];
    }
    nullTerminateArgv[argc] = 0;
    Logger::setTracer(new Logger::ServerlibTracer());

    return watch_main(argc, nullTerminateArgv, tcl_init, tcl_start, on_exit);
}
/*****************************************************************/
// returns non zero if killing in progress. stop_obr_all
static int batchProcKilling()
{
    return batch_killing;
}
/*****************************************************************/
static char* get_action_by_num(int num)
{
    for (int i = 0; i < N_ACTION; i++) {
        if (f_action[i].f_num == num) {
            return f_action[i].f_name;
        }
    }
    return f_action[0].f_name;
}
/*****************************************************************/
static const char* find_proc_name_by_no(int proc_num)
{
    static char p_name[100];
    char str_no[100];

    sprintf(str_no, "%d", proc_num);
    Tcl_Obj* p_obj = Tcl_NewObj();
    if (p_obj == NULL) {
        ProgError(STDLOG, ">>>>1Tcl_NewObj return NULL\n");
        return getUnknownProcessName();
    }
    Tcl_AppendStringsToObj(p_obj, ARRCMD_GET_NAME_ARR_NO, " ", str_no, NULL);
    if (TCL_OK != Tcl_EvalObjEx(getTclInterpretator(), p_obj, TCL_EVAL_GLOBAL)) {
        ProgError(STDLOG, "Tcl_EvalObjEx(%s) Error\n%s\n",
                ARRCMD_GET_NAME_ARR_NO, Tcl_GetStringResult(getTclInterpretator()));
        return getUnknownProcessName();
    }
    strncpy(p_name, Tcl_GetStringResult(getTclInterpretator()), sizeof(p_name) - 1);
    p_name[sizeof(p_name) - 1] = 0;
    return p_name;
}
/*****************************************************************/
static const char* find_proc_name_by_pid(int pid)
{
    for (int i = 0; i < NPROC; i++) {
        if (ProcTab[i].ppid == pid) {
            return find_proc_name_by_no(ProcTab[i].proc_num);
        }
    }
    return getUnknownProcessName();
}
/*****************************************************************/
static ProcS* find_proctab_by_pid(int pid)
{
    for (int i = 0; i < NPROC; i++) {
        if (ProcTab[i].ppid == pid) {
            return &(ProcTab[i]);
        }
    }
    return NULL;
}
/*********************************************/
static void set_stop_child(Tcl_Interp* interp, int pid)
{
    for (int i = 0; i < NPROC; i++) {
        ProcS& pr = ProcTab[i];
        if (pr.ppid == pid) {
            set_stop_pid_process(interp, pr);
            if (pr.action == A_RESTART) {
            } else if (A_FORCED_RESTART == pr.action) {
                Tcl_Obj* obj = Tcl_NewObj();
                if (obj) {
                    static const int BUF_SIZE = 64;
                    char buf[BUF_SIZE];

                    snprintf(buf, BUF_SIZE, "%d", pr.proc_num);
                    Tcl_AppendStringsToObj(obj, ARRCMD_RESET_N_START_ARR_NO,
                        " ", buf, 0);
                    if (TCL_OK != Tcl_EvalObjEx(interp, obj, TCL_EVAL_GLOBAL)) {
                        ProgError(STDLOG, "Tcl_EvalObjEx(%s) Error\n%s\n",
                            ARRCMD_RESET_N_START_ARR_NO, Tcl_GetStringResult(interp));
                    }
                } else {
                    LogError(STDLOG) << "Tcl_NewObj() failed";
                }
                kill_proc(interp , pr , 0, A_RESTART, 0, 0);
            }else if (pr.action == A_STOP) {
                delete_proc(interp, pr);
            } else if (pr.action == A_WORK) {
                if (ISSET_BIT(pr.proc_flag, PROC_FL_SPEC)) {
                    kill_proc(interp , pr , 0, A_STOP, 0, 0);
                    delete_proc(interp, pr);
                } else {
                    kill_proc(interp , pr , 0, A_RESTART, 0, 0);
                }
            } else if (A_FINAL == pr.action) {
                    delete_proc(interp, pr);
            }
            return;
        }
    }
}
/*****************************************************************/
static void tclmon_err_log_to_monitor(const char* nickname, const char* f_name, int f_line, char* res_str, bool use_censor = true)
{
    const bool had_cc = use_censor ? LogCensor::apply(res_str) : false;
    const auto str_logg = HelpCpp::vsprintf_string("%s>>>>>%s:%s:%d: TCLMON: %s", get_log_head(-1,had_cc), nickname, f_name, f_line, res_str);
    monitorControl::is_errors_control(str_logg.data(), str_logg.size());
}
/*****************************************************************/
static void waitchld(int s)
{
}
/*****************************************************************/
static void waitchld_do(Tcl_Interp* interp, int* need_stop_proc_obr)
{
    char str_res[1000];
    int pid;
    do {
        char flag_print = 0;
        int status;

        pid = waitpid(-1, &status, WNOHANG | WUNTRACED);
        if (pid > 0) {
            const char* p_name = find_proc_name_by_pid(pid);
            if (WIFSIGNALED(status)) {
                flag_print = 1;
                if (need_stop_proc_obr != NULL && WTERMSIG(status) == SIGXFSZ) {
                    *need_stop_proc_obr = 1;
                }
                snprintf(str_res, sizeof(str_res), "%s(pid=%d) exited: uncaught signal #%d (%s)",
                        p_name, pid, WTERMSIG(status), read_cur_req(interp, pid));
                set_stop_child(interp, pid);
                ProgTrace(TRACE1, "%s", str_res);
                if (!(startupKeepSilence() && strcmp(p_name, getUnknownProcessName()) == 0) && !batchProcKilling())
                { tclmon_err_log_to_monitor(STDLOG, str_res); }
            }
            if (WIFEXITED(status)) {
                set_stop_child(interp, pid);
                flag_print = 1;
                snprintf(str_res, sizeof(str_res), "Process %s(pid=%d) exited with exit code %d",
                        p_name, pid, WEXITSTATUS(status));
                ProgTrace(TRACE1, "%s", str_res);
                if (!(startupKeepSilence() && strcmp(p_name, getUnknownProcessName()) == 0) && !batchProcKilling()) {
                    const std::set<int>::iterator i = restartedPids.find(pid);

                    if (restartedPids.end() != i) {
                        restartedPids.erase(i);
                    } else {
                        tclmon_err_log_to_monitor(STDLOG, str_res, false);
                    }
                }
            }
            if (WIFSTOPPED(status)) {
                flag_print = 1;
                snprintf(str_res, sizeof(str_res), "Process %s(pid=%d) stopped by signal #%d",
                        p_name, pid, WSTOPSIG(status));
                ProgTrace(TRACE1, "%s", str_res);
                if (!(startupKeepSilence() && strcmp(p_name, getUnknownProcessName()) == 0) && !batchProcKilling())
                { tclmon_err_log_to_monitor(STDLOG, str_res, false); }
            }
            if (flag_print == 0) {
                ProgTrace(TRACE1, "child %s pid=%d", p_name, pid);
            }
        }
    } while (pid > 0);
}
/*****************************************************************/
static int find_free(void)
{
    for (int i = 0; i < NPROC; i++) {
        if (ProcTab[i].action == A_NONE) {
            return i;
        }
    }
    return -1;
}
/*****************************************************************/
static int set_stop_pid_process(Tcl_Interp* interp, ProcS& pr)
{
    char str_no[100];

    Tcl_Obj* script_obj = Tcl_NewObj();
    pr.ppid = -1;
    pr.kill_time = 0;
    if (script_obj == NULL) {
        ProgError(STDLOG, ">>>>7Tcl_NewObj return NULL\n");
        return -1;
    }
    sprintf(str_no, "%d", pr.proc_num);
    Tcl_AppendStringsToObj(script_obj, ARRCMD_SET_STOP_ARR_NO,
            " ", str_no, " ", NULL);
    if (TCL_OK != Tcl_EvalObjEx(interp, script_obj, TCL_EVAL_GLOBAL)) {
        ProgError(STDLOG, "Tcl_EvalObjEx(%s) Error\n%s\n",
                ARRCMD_SET_STOP_ARR_NO, Tcl_GetStringResult(interp));
    }
    return 0;
}
/*****************************************************************/
static int set_action_process(Tcl_Interp* interp, ProcS& pr, int action)
{
    char str_no[100];
    Tcl_Obj* script_obj = Tcl_NewObj();
    if (script_obj == NULL) {
        ProgError(STDLOG, ">>>>7Tcl_NewObj return NULL\n");
        return -1;
    }
    pr.action = action;
    sprintf(str_no, "%d", pr.proc_num);
    Tcl_AppendStringsToObj(script_obj, ARRCMD_SET_ACTION_ARR_NO,
            " ", str_no, " ", get_action_by_num(action), NULL);
    if (TCL_OK != Tcl_EvalObjEx(interp, script_obj, TCL_EVAL_GLOBAL)) {
        ProgError(STDLOG, "Tcl_EvalObjEx(%s) Error\n%s\n",
                ARRCMD_SET_ACTION_ARR_NO, Tcl_GetStringResult(interp));
    }
    return 0;
}
/*****************************************************************/
static void delete_proc(Tcl_Interp* interp, ProcS& pr)
{
    char str_no[100];

    if (pr.fd >= 0) {
        close(pr.fd);
        pr.fd = -1;
    }
    pr.ppid = -1;
    pr.action = A_NONE;
    clear_Qmbl(pr.proc_que_out);
    reset_mbl(pr.p_mbl);
    sprintf(str_no, " %d", pr.proc_num);

    Tcl_Obj* script_obj = Tcl_NewObj();
    if (script_obj == NULL) {
        ProgError(STDLOG, ">>>>2Tcl_NewObj return NULL\n");
        return;
    }
    Tcl_AppendStringsToObj(script_obj, ARRCMD_DELETE_ARR_NO, str_no, NULL);
    if (TCL_OK != Tcl_EvalObjEx(interp, script_obj, TCL_EVAL_GLOBAL)) {
        ProgError(STDLOG, "Tcl_EvalObjEx(%s) Error\n%s\n",
                ARRCMD_DELETE_ARR_NO, Tcl_GetStringResult(interp));
    }
}
/*****************************************************************/
static int Tcl_AppInit(Tcl_Interp* interp)
{
    if (MaxFdBeforeTclMain > 0) {
        for (int i = MaxFdBeforeTclMain + 1; i < FD_TABLE_SIZE; ++i) {
            if (fcntl(i, F_GETFL) != -1) {
                addToCloseFd(i, 0);
                addToCloseFdTclMain(i);
            }
        }
    }

    setTclInterpretator(interp);
    if (Tcl_Init(interp) == TCL_ERROR) {
        return TCL_ERROR;
    }

    reset_potok_head();
#ifdef TCL_TEST
#ifdef TCL_XT_TEST
    if (Tclxttest_Init(interp) == TCL_ERROR) {
        return TCL_ERROR;
    }
#endif
    if (Tcltest_Init(interp) == TCL_ERROR) {
        return TCL_ERROR;
    }
    Tcl_StaticPackage(interp, "Tcltest", Tcltest_Init, (Tcl_PackageInitProc*) NULL);
    if (TclObjTest_Init(interp) == TCL_ERROR) {
        return TCL_ERROR;
    }
#ifdef TCL_THREADS
    if (TclThread_Init(interp) == TCL_ERROR) {
        return TCL_ERROR;
    }
#endif
    if (Procbodytest_Init(interp) == TCL_ERROR) {
        return TCL_ERROR;
    }
    Tcl_StaticPackage(interp, "procbodytest", Procbodytest_Init, Procbodytest_SafeInit);
#endif /* TCL_TEST */

    /*
     * Call the init procedures for included packages.  Each call should
     * look like this:
     *
     * if (Mod_Init(interp) == TCL_ERROR) {
     *     return TCL_ERROR;
     * }
     *
     * where "Mod" is the name of the module.
     */

    /*
     * Call Tcl_CreateCommand for application-specific commands, if
     * they weren't already created by the init procedures called above.
     */

    Tcl_CreateObjCommand(interp, "set_logging", set_logging,
            (ClientData)0, 0);

    Tcl_CreateObjCommand(interp, MONCMD_PROC_STOP, stop_obr_tcl,
            (ClientData)0, 0);

    Tcl_CreateObjCommand(interp, MONCMD_PROC_RESTART, restart_obr_tcl,
            (ClientData)0, 0);

    Tcl_CreateObjCommand(interp, MONCMD_PROC_CMD, command_obr_tcl,
            (ClientData)0, 0);

    Tcl_CreateObjCommand(interp, "add_zapr_to_potok", add_zapr_to_potok_tcl,
            (ClientData)0, 0);

    Tcl_CreateObjCommand(interp, "get_potok", get_potok_tcl,
            (ClientData)0, 0);

    Tcl_CreateObjCommand(interp, "get_que_num_by_grp_name", get_que_num_by_grp_name_tcl,
            (ClientData)0, 0);

    Tcl_CreateObjCommand(interp, "get_potok_by_que_name", get_potok_by_que_name_tcl,
            (ClientData)0, 0);

    Tcl_CreateObjCommand(interp, "show_bufs_proc", show_bufs_tcl,
            (ClientData)0, 0);

    if (! Tcl_CreateObjCommand(interp, "set_testing_mode", tcl_set_testing_mode,
                (ClientData)0, 0)) {
        fprintf(stderr, "set_testing_mode creation failed \n%s\n",
                Tcl_GetString(Tcl_GetObjResult(interp)));
        return 1;
    }

    if (! Tcl_CreateObjCommand(interp, "lock_startup", lock_startup,
                (ClientData)0, 0)) {
        fprintf(stderr, "lock_startup creation failed \n%s\n",
                Tcl_GetString(Tcl_GetObjResult(interp)));
        return 1;
    }

    if (App_init) {
        if (App_init(interp) < 0) {
            fprintf(stderr, "app init failed\n");

            return TCL_ERROR;
        }
    }

    return TCL_OK;
}
/*****************************************************************/
int watch_main(int argc,
        char* argv[],
        int (*app_init)(Tcl_Interp*),
        int (*app_start)(Tcl_Interp*),
        void (*before_exit)(void))
{
    before_exit_main_proc = before_exit;
    App_init = app_init;
    App_start = app_start;
    Argv = argv;
    Argc = argc;

    clist_init( CLIST_PROCID_SUPERVISOR );

    set_sig(waitchld,SIGCLD);
    set_sig(waitchld,SIGCHLD);
    set_sig(regLogReopen,SIGUSR1);

    for(int i = 0; i < NPROC; ++i) {
        ProcTab[i].proc_num = i + 1;
    }

    // Химия для предотвращения роста file descriptors при перезапуске сирены:
    // Перед вызовом Tcl_Main() определяем максимальный открытый дескриптор и
    // записываем его номер в MaxFdBeforeTclMain.
    // После выполнения Tcl_Main() в функции Tcl_AppInit() определяем дескрипторы,
    // открытые в Tcl_Main() и добавляем их в fdtoclose ф-цией addToCloseFd()
    for (int  i_fd = 0; i_fd < FD_TABLE_SIZE; ++i_fd) {
        if (fcntl(i_fd, F_GETFL) != -1) {
            MaxFdBeforeTclMain = i_fd;
        }
    }
    //This call inner init tcl library path. This need for call *Encoding family Tcl API. Hack.
    //This initialisation performed in Tcl_Main()
    Tcl_FindExecutable(argv[0]);
    if (TCL_OK != Supervisor::Tcl_AppInit(Tcl_CreateInterp())) {
        fprintf(stderr, "%s\n", "Call Tcl_AppInit() failed");

        return 1;
    }
    if (TCL_OK != Tcl_EvalFile(getTclInterpretator(), argv[1])) {
        fprintf(stderr, "%s%s\n", "Read config failed: ", Tcl_GetStringResult(getTclInterpretator()));

        return 1;
    }
    csaHolder.reset(new CsaHolder(readIntFromTcl("CSA_KEY")));
    semHolder.reset(new SemHolder(readIntFromTcl("SEM_KEY", SEMAPHORE_KEY)));

    watch(getTclInterpretator());

    return 1;
}
/*****************************************************************/
static int creat_proc(ProcS& pr, Tcl_Interp* interp, const char* olist, const GroupDescription::PROCESS_LEVEL pl)
{
    Tcl_Obj* script_obj = Tcl_NewObj();
    if (script_obj == NULL) {
        ProgError(STDLOG, ">>>>3Tcl_NewObj return NULL\n");

        return -1;
    }

    pr.action = A_CREATE;
    pr.level = pl;
    if( (pr.p_mbl = new_mbl()) == NULL ) {
        ProgError(STDLOG, "Error alloc mbl\n");
        return -1;
    }

    char str_no[100] = {};
    sprintf(str_no, "%d", pr.proc_num);
    Tcl_AppendStringsToObj(script_obj, ARRCMD_CREATE_ARR_NO, " ",
            str_no, " ", get_action_by_num(pr.action),
            " [ list ", olist, " ] ",
            NULL);
    if (TCL_OK != Tcl_EvalObjEx(interp, script_obj, TCL_EVAL_GLOBAL)) {
        fprintf(stderr, "Tcl_EvalObjEx(%s) Error\n%s\n",
                ARRCMD_CREATE_ARR_NO, Tcl_GetStringResult(interp));
    }

    return 0;
}
/*****************************************************************/
static void close_all_desc(void)
{
    for (int i = 0; i < NPROC; i++) {
        ProcS& s_pr = ProcTab[i];
        if (s_pr.action != A_NONE) {
            close(s_pr.fd);
        }
    }
}
/*****************************************************************/
static short getProcFlag(const std::string& procName)
{
    if ("monitor" == procName) {
        return PROC_FL_MAIN;
    } else if ("leva_udp" == procName) {
        return PROC_FL_DISPATCHER;
    } else if ("leva" == procName) {
        return PROC_FL_DISPATCHER;
    } else if ("levh" == procName) {
        return PROC_FL_DISPATCHER;
    } else if ("levhssl" == procName) {
        return PROC_FL_DISPATCHER;
    } else if ("shmsrv" == procName) {
        return PROC_FL_DISPATCHER;
    } else if ("xpr_disp" == procName) {
        return PROC_FL_DISPATCHER;
    } else if ("logger" == procName) {
        return PROC_FL_DISPATCHER;
    } else if ("httpsrv" == procName) {
        return PROC_FL_DISPATCHER;
    }

    return PROC_FL_NONE;
}
/*****************************************************************/
static pid_t start_proc(ProcS& pr, Tcl_Interp* interp)
{
    static const int BUF_SIZE = 128;
    static const char NEW_PROC_ARGC = 8;

    static const std::string execFileConfig(readStringFromTcl("EXEC_FILE_CONFIG"));

    char str_pid[BUF_SIZE];
    char str_no[BUF_SIZE];
    int pp[2];

    if (socketpair(AF_UNIX , SOCK_STREAM , 0 , pp) < 0) {
        ProgError(STDLOG, ">>>>Error while calling socketpair(pp) in start_proc(Err=%d)\n", errno);

        return -1;
    }
    snprintf(str_no, 128, "%d", pr.proc_num);

    Tcl_Obj* p_obj = Tcl_NewObj();

    if (NULL == p_obj) {
        ProgError(STDLOG, ">>>>4Tcl_NewObj return NULL\n");

        return -1;
    }
    Tcl_AppendStringsToObj(p_obj, ARRCMD_GET_CMDS_ARR_NO, " ", str_no, NULL);

    int Ret = Tcl_EvalObjEx(interp, p_obj, TCL_EVAL_GLOBAL);
    Tcl_Obj* t_obj = Tcl_GetObjResult(interp);
    if (TCL_OK != Ret) {
        ProgError(STDLOG, "Tcl_EvalObj(%s) Error\n%s\n",
                ARRCMD_GET_CMDS_ARR_NO, Tcl_GetString(t_obj));

        return -1;
    }

    const std::string startLine(Tcl_GetString(t_obj));
    const std::string processName(startLine.substr(0, startLine.find(' ')));

    fflush(0);
    int pid = fork();
    if (pid < 0) {
        ProgError(STDLOG, "fork failed(Err=%d)\n", errno);
        Tcl_DecrRefCount(t_obj);
        close(pp[0]);
        close(pp[1]);

        return -1;
    }
    if (0 == pid) { //Потомок
        closeLog();
        close(pp[0]);
        close_all_desc();

        char* new_proc_argv[NEW_PROC_ARGC];
        std::string sock_desc(HelpCpp::string_cast(pp[1]));

        new_proc_argv[0] = const_cast<char*>(GroupDescription::execFileName(pr.level).c_str()); //Имя исполняемого файла
        new_proc_argv[1] = const_cast<char*>(execFileConfig.c_str()); //Имя конфигурационного файла (.tcl)
        new_proc_argv[2] = const_cast<char*>(sock_desc.c_str()); //Дескриптор локального сокета для связи с монитором
        new_proc_argv[3] = const_cast<char*>(startLine.c_str()); //Строка запуска
        new_proc_argv[4] = 0;
        if (execv(new_proc_argv[0], new_proc_argv)) {
            ProgError(STDLOG, "Process %s %s %s", Argv[0], (Argc > 1) ? Argv[1] : "", Tcl_GetString(t_obj));
            perror("execv failed");

            exit(1);
        }
    } else { // Родитель
        ProgTrace(TRACE1, "Process %s(pid=%d) started (fd_in=%d) (fd_out=%d)",
                processName.c_str(), pid, pp[0], pp[0]);
        pr.ppid = pid;
        setProcStartTime(time(0));
        close(pp[1]);

        pr.fd = pp[0];  /* и чтение и запись */
        /*        pr.time = time (0);*/
        pr.kill_time = 0;
        pr.delay = 0;
        pr.action = A_WORK;
        pr.proc_flag = getProcFlag(processName);

        sprintf(str_no, "%d", pr.proc_num);
        sprintf(str_pid, "%d", pr.ppid);

        Tcl_Obj* script_obj = Tcl_NewObj();
        if (NULL == script_obj) {
            ProgError(STDLOG, ">>>>6Tcl_NewObj return NULL\n");

            return -1;
        }

        Tcl_AppendStringsToObj(script_obj, "set_start_arr_no ",
                str_no, " ", processName.c_str(), " ", str_pid, " ", get_action_by_num(pr.action), NULL);
        if (TCL_OK != Tcl_EvalObjEx(interp, script_obj, TCL_EVAL_GLOBAL)) {
            ProgError(STDLOG, "Tcl_EvalObjEx Error:\n%s\n", Tcl_GetStringResult(interp));

            return -1;
        }
        if (fcntl(pp[0], F_SETFL, O_NONBLOCK) < 0) {
            ProgError(STDLOG, "fcntl pp[0] error. %s\n", strerror(errno));
        }
    }

    return pid;
}
/*****************************************************************/
static int kill_proc_restart_stop(Tcl_Interp* interp , ProcS& pr , int s, int action,
        time_t kill_time, time_t delay)
{
    int cur_action = action;
    if (cur_action == A_RESTART && ISSET_BIT(pr.proc_flag, PROC_FL_SPEC)) {
        cur_action = A_STOP;
    }
    return kill_proc(interp, pr, s, cur_action, kill_time, delay);
}
/*****************************************************************/
static int kill_proc(Tcl_Interp* interp , ProcS& pr , int s, int action,
        time_t kill_time, time_t delay)
{
    if (pr.fd >= 0) {
        close(pr.fd);
        pr.fd = -1;
    }
    if (kill_time != 0) {
        pr.kill_time = kill_time;
    }
    if (delay != 0) {
        pr.delay = delay;
    }
    clear_Qmbl(pr.proc_que_out);
    reset_mbl(pr.p_mbl);
    /*
       if(pr.ppid==-1)
       {
       delete_proc(interp,pr);
       }
       else
     */
    {
        if (pr.ppid > 0 && s > 0) {
            int ret = kill(pr.ppid, s);
            if (ret == -1) {
                if (errno == ESRCH) {
                    set_stop_pid_process(interp, pr);
                }
            }
        }
        set_action_process(interp, pr, action);
    }
    return 0;
}
/*****************************************************************/
static int start_new(int argc, const char* objv[], Tcl_Interp* interp, GroupDescription::PROCESS_LEVEL pl)
{
    if (argc != 1) {
        Tcl_ResetResult(interp);
        Tcl_AppendResult(interp, "error in args", NULL);

        return TCL_ERROR;
    }

    int i = find_free();
    if (i < 0) {
        Tcl_ResetResult(interp);
        Tcl_AppendResult(interp, "Too many processes", NULL);

        return TCL_ERROR;
    }

    ProcS& p_proctab = ProcTab[i];
    if (creat_proc(p_proctab, interp, objv[0], pl) < 0) {
        delete_proc(interp, p_proctab);

        return TCL_ERROR;
    }
    if (start_proc(p_proctab, interp) < 0) {
        delete_proc(interp, p_proctab);

        return TCL_ERROR;
    }

    return TCL_OK;
}
/*****************************************************************/
static int stop_old(int handlersCount, const std::string& startLine, Tcl_Interp* interp)
{
    static const char* ERROR_MESSAGE = "Can't stoped processes";
    static const int ERROR_MESSAGE_LEN = strlen(ERROR_MESSAGE);
    Tcl_Obj* cmd = Tcl_NewStringObj(ARRCMD_FIND_PROC_FOR_STOP, -1);

    if (!cmd) {
        LogTrace(TRACE0) << "Call Tcl_NewStringObj() fail";
        monitorControl::is_errors_control(ERROR_MESSAGE, ERROR_MESSAGE_LEN);

        return -1;
    }
    Tcl_AppendStringsToObj(cmd, " ", HelpCpp::string_cast(handlersCount).c_str(), " [ list ", startLine.c_str()," ] ", 0);
    if (Tcl_EvalObjEx(interp, cmd, TCL_EVAL_GLOBAL)) {
        LogTrace(TRACE0) << "Can't evaluate " << ARRCMD_FIND_PROC_FOR_STOP << ": " << Tcl_GetStringResult(interp);
        monitorControl::is_errors_control(ERROR_MESSAGE, ERROR_MESSAGE_LEN);

        return -1;
    }

    TclObjHolder result(Tcl_GetObjResult(interp));
    Tcl_Obj* processNumObj;
    int prNum;
    _Message_ mes;
    mes.set_mes_text("exit", 4);
    mes.set_mes_cmd(TCLCMD_PROCCMD);

    for(int i = 0; i < handlersCount; ++i) {
        if (TCL_OK != Tcl_ListObjIndex(interp, result.get(), i, &processNumObj)) {
            LogTrace(TRACE0) << "Call Tcl_ListObjIndex(...) fail";
            monitorControl::is_errors_control(ERROR_MESSAGE, ERROR_MESSAGE_LEN);

            return -1;
        }
        if (TCL_OK != Tcl_GetIntFromObj(interp, processNumObj, &prNum)) {
            LogTrace(TRACE0) << "Call Tcl_GetIntFromObj(...) fail";
            monitorControl::is_errors_control(ERROR_MESSAGE, ERROR_MESSAGE_LEN);

            return -1;
        }

        set_action_process(interp, ProcTab[prNum - 1], A_FINAL);
        put_message_to_que(mes, ProcTab[prNum - 1]);
    }
    applyingConfigDiff = false;

    return 0;
}
/*****************************************************************/
static int stop_proc_obr(Tcl_Interp* interp)
{
    int n_pids = 0;
    for (int i = 0; i < NPROC; i++) {
        ProcS& pr = ProcTab[i];
        if (pr.action != A_NONE && ISNSET_BIT(pr.proc_flag , PROC_FL_MAIN)) {
            kill_proc(interp, pr, SIGTERM, A_STOP, time(0) + 5, 0);
            n_pids++;
        }
    }
    if (n_pids > 0) {
        batch_killing = 1;
    }

    return n_pids;
}
/*****************************************************************/
static int stop_obr_tcl(ClientData cl, Tcl_Interp* interp, int objc,
        Tcl_Obj* CONST objv[])
{
    char str_pid[100];
    int n_pids = 0;

    objc--;
    objv++;

    if (objc < 1) {
        Tcl_ResetResult(interp);
        Tcl_AppendResult(interp, "error in args", NULL);
        return TCL_ERROR;
    }

    char* str = Tcl_GetString(objv[0]);
    if (strcmp(str, MONCMD_PROC_STOP_PARALL) == 0) {
        exit_main_proc(interp, false);
        exit(0);
    }
    if (strcmp(str, MONCMD_PROC_STOP_PARSEL) == 0) {
        n_pids = stop_proc_obr(interp);
        Tcl_ResetResult(interp);
        snprintf(str_pid, sizeof(str_pid), "Stopped %d processes", n_pids);

        time_t ct = time(0);
        struct tm* tm_p = localtime(&ct);
        strftime(str_pid + strlen(str_pid), 30, " at %T", tm_p);

        monitorControl::is_errors_control(str_pid, strlen(str_pid));
        Tcl_AppendResult(interp, str_pid, NULL);
        return TCL_OK;
    }

    Tcl_Obj* ptr_res = Tcl_NewObj();
    if (ptr_res == NULL) {
        ProgError(STDLOG, ">>>>8Tcl_NewObj return NULL\n");
        return TCL_ERROR;
    }
    for (int j = 1; j < objc; j++) {
        int pid;
        if (TCL_OK != Tcl_GetIntFromObj(interp, objv[j], &pid)) {
            continue;
        }
        for (int i = 0; i < NPROC; i++) {
            ProcS& pr = ProcTab[i];
            if (pr.ppid == pid) {
                const char* p_name = find_proc_name_by_no(pr.proc_num);
                if (strcmp(p_name, str) == 0) {
                    kill_proc(interp , pr, SIGTERM, A_STOP, time(0) + 5, 0);
                    sprintf(str_pid, "%d ", pid);
                    Tcl_AppendStringsToObj(ptr_res, str_pid, NULL);
                    n_pids++;
                    break;
                }
            }
        }
    }

    if (n_pids == 0) {
        Tcl_ResetResult(interp);
        Tcl_AppendResult(interp, "No need processes for stop", NULL);
        return TCL_ERROR;
    }
    Tcl_SetObjResult(interp, ptr_res);
    return TCL_OK;
}
/*****************************************************************/
static int restart_obr_tcl(ClientData cl, Tcl_Interp* interp, int objc,
        Tcl_Obj* CONST objv[])
{
    const char* p_name;
    char str_pid[100];
    int n_pids = 0;

    objc--;
    objv++;

    if (objc < 1) {
        Tcl_ResetResult(interp);
        Tcl_AppendResult(interp, "error in args", NULL);
        return TCL_ERROR;
    }

    char* str = Tcl_GetString(objv[0]);

    if (strcmp(str, MONCMD_PROC_RESTART_PARALL) == 0) {
        char msg[100] = "Shutdown all processes for restart at ";
        time_t ct = time(0);
        struct tm* tm_p = localtime(&ct);
        strftime(msg + strlen(msg), 30, "%T", tm_p);
        monitorControl::is_errors_control(msg, strlen(msg));
        exit_main_proc(interp, true);
    }
    if (!strcmp(str, MONCMD_PROC_RESTART_HANDLERS)) {
        static const int BUF_SIZE = 128;
        char msg[BUF_SIZE] = "Shutdown all handlers for restart at ";
        static const int msgHeadLen = strlen(msg);

        time_t ct = time(0);
        struct tm* tm_p = localtime(&ct);

        strftime(msg + msgHeadLen, BUF_SIZE - msgHeadLen - 1, "%T", tm_p);
        monitorControl::is_errors_control(msg, strlen(msg));

        restartHandlers(interp);
        return TCL_OK;
    }

    Tcl_Obj* ptr_res = Tcl_NewObj();
    if (ptr_res == NULL) {
        ProgError(STDLOG, ">>>>9Tcl_NewObj return NULL\n");
        return TCL_ERROR;
    }
    for (int j = 1; j < objc; j++) {
        int pid;
        if (TCL_OK != Tcl_GetIntFromObj(interp, objv[j], &pid)) {
            continue;
        }
        for (int i = 0; i < NPROC; i++) {
            ProcS& pr = ProcTab[i];
            if (pr.ppid == pid) {
                p_name = find_proc_name_by_no(pr.proc_num);
                if (strcmp(p_name, str) == 0) {
                    if (pr.ppid > 0) {
                        kill_proc_restart_stop(interp , pr , SIGTERM, A_RESTART, time(0) + 10, 0);
                        sprintf(str_pid, "%d ", pid);
                        Tcl_AppendStringsToObj(ptr_res, str_pid, NULL);
                        n_pids++;
                    }
                    break;
                }
            }
        }
    }
    if (n_pids == 0) {
        Tcl_ResetResult(interp);
        Tcl_AppendResult(interp, "No need processes for restart", NULL);
        return TCL_ERROR;
    }

    Tcl_SetObjResult(interp, ptr_res);
    return TCL_OK;
}
/*****************************************************************/
static int tcl_process_final(ClientData cl, Tcl_Interp* interp, int objc, Tcl_Obj* CONST objv[])
{
    int arrProcNum;
    if ((1 != objc) || (TCL_OK != Tcl_GetIntFromObj(interp, objv[0], &arrProcNum))) {
        LogTrace(TRACE0) << __FUNCTION__ << " Can't get process number for finalize";

        return TCL_ERROR;
    }

    kill_proc(interp, ProcTab[arrProcNum - 1], SIGKILL, A_STOP, 0, 0);
    delete_proc(interp, ProcTab[arrProcNum - 1]);

    return TCL_OK;
}
/*****************************************************************/
static int get_que_num_by_grp_name_tcl(ClientData cl, Tcl_Interp* interp, int objc,
        Tcl_Obj* CONST objv[])
{
    char str_grp_num[20];

    objc--;
    objv++;
    if (objc != 1) {
        Tcl_ResetResult(interp);
        Tcl_AppendResult(interp, "error in args", NULL);

        return TCL_ERROR;
    }

    char* str_grp_name = Tcl_GetString(objv[0]);

    Tcl_Obj* ptr_res = Tcl_NewObj();
    if (ptr_res == NULL) {
        ProgError(STDLOG, ">>>>22Tcl_NewObj return NULL\n");

        return TCL_ERROR;
    }

    snprintf(str_grp_num, sizeof(str_grp_num), "%d",
            get_que_num_by_grp_name_proc(str_grp_name));

    Tcl_AppendStringsToObj(ptr_res, str_grp_num, NULL);
    Tcl_SetObjResult(interp, ptr_res);

    return TCL_OK;
}
/*****************************************************************/
static int add_zapr_to_potok_tcl(ClientData cl, Tcl_Interp* interp, int objc,
        Tcl_Obj* CONST objv[])
{
    int n_zapr = 1;
    int type_zapr = QUEPOT_ZAPR;
    int grp_zapr = 0;

    objc--;
    objv++;

    if (objc >= 1) {
        if (Tcl_GetIntFromObj(interp, objv[0], &n_zapr) != TCL_OK) {
            n_zapr = 1;
        }
    }

    if (objc >= 2) {
        if (Tcl_GetIntFromObj(interp, objv[1], &type_zapr) != TCL_OK) {
            type_zapr = QUEPOT_ZAPR;
        }
    }

    if (objc >= 3) {
        if (Tcl_GetIntFromObj(interp, objv[2], &grp_zapr) != TCL_OK) {
            grp_zapr = 0;
        }
    }
    add_zapr_to_potok_proc(n_zapr, type_zapr, grp_zapr);
    return TCL_OK;
}
/*****************************************************************/
static int get_potok_tcl(ClientData cl, Tcl_Interp* interp, int objc,
        Tcl_Obj* CONST objv[])
{
    char potok_str[2000];
    get_potok_all_proc(potok_str, sizeof(potok_str));
    Tcl_ResetResult(interp);
    Tcl_AppendResult(interp, potok_str, NULL);
    return TCL_OK;
}
/*****************************************************************/
static int get_potok_by_que_name_tcl(ClientData cl, Tcl_Interp* interp, int objc,
        Tcl_Obj* CONST objv[])
{
    objc--;
    objv++;

    /*????*/
    if (objc < 1) {
        Tcl_ResetResult(interp);
        Tcl_AppendResult(interp, "error in args", NULL);

        return TCL_ERROR;
    }
    char* grp_name = Tcl_GetString(objv[0]);
    char* str_res = get_potok_by_que_name_proc(grp_name);

    Tcl_ResetResult(interp);
    Tcl_AppendResult(interp, str_res, NULL);

    return TCL_OK;
}
/*****************************************************************/
static int show_bufs_tcl(ClientData cl, Tcl_Interp* interp, int objc,
        Tcl_Obj* CONST objv[])
{
    char str_res[2000];

    str_res[0] = 0;
    show_buf_counts(str_res, sizeof(str_res));
    Tcl_ResetResult(interp);
    Tcl_AppendResult(interp, str_res, NULL);

    return TCL_OK;
}
/*****************************************************************/
int test_set_flag_tclmon(Tcl_Interp* interp)
{
    static time_t time1_setfl = 0;

    if (time1_setfl == 0) {
        time1_setfl = time(NULL);
    }

    time_t time2 = time(NULL);
    if (SETFLAG_TO > 0) {
        if (difftime(time2, time1_setfl) > SETFLAG_TO) {
            time1_setfl = time2;
            write_set_flag_tclmon(interp, Pid_p, -1, 0, QUEPOT_ZAPR);
        }
    }

    return 1;
}
/*****************************************************************/
const char* read_cur_req(Tcl_Interp* interp, int pid)
{
    static char req[200];
    char cmd[200];

    if (inTestMode()) {

        return "";
    }

    req[0] = '\0';
    snprintf(cmd, sizeof(cmd), "%s %d", ARRCMD_GET_CUR_REQ, pid);

    Tcl_Obj* pobj = Tcl_NewStringObj(cmd, -1);
    if (pobj == NULL) {
        ProgError(STDLOG, ">>>>10Tcl_NewStringObj return NULL\n");

        return req;
    }

    int Ret = Tcl_EvalObjEx(interp, pobj, TCL_EVAL_GLOBAL);
    const char* res_ptr = Tcl_GetStringResult(interp);
    if (Ret != TCL_OK) {
        ProgError(STDLOG, "GetRequest failed:%s\n", res_ptr);

        return req;
    }
    if (res_ptr != NULL) {
        strncpy(req, res_ptr, sizeof(req));
    }

    return req;
}
/*****************************************************************/
static void write_set_flag_tclmon(Tcl_Interp* interp, int pid_p, int TO_restart, int n_zapr, int type_zapr)
{
    _Message_ mes;
    fill_set_flag(&mes, pid_p, TO_restart, n_zapr, type_zapr, "{}");
    obr_cmd_set_flag(interp, mes.get_mes_text(), mes.get_mes_len());
}
/*****************************************************************/
void test_restart_proc(Tcl_Interp* interp)
{
    static time_t time1_rest = 0;

    if (time1_rest == 0) {
        time1_rest = time(NULL);
    }

    time_t time2 = time(NULL);
    if (TO_TEST_RESTART_PROC <= 0) {
        return;
    }
    if (difftime(time2, time1_rest) <= TO_TEST_RESTART_PROC) {
        return;
    }
    time1_rest = time2;

    Tcl_Obj* pobj = Tcl_NewStringObj(ARRCMD_TEST_RESTART_PROC, -1);
    if (pobj == NULL) {
        ProgError(STDLOG, ">>>>10Tcl_NewStringObj return NULL\n");
        return;
    }

    int Ret = Tcl_EvalObjEx(interp, pobj, TCL_EVAL_GLOBAL);
    const char* res_ptr = Tcl_GetStringResult(interp);
    if (Ret != TCL_OK) {
        ProgError(STDLOG, "TestRestart failed:%s\n", res_ptr);
        return;
    }
    if (res_ptr != NULL) {
        if (strlen(res_ptr) > 0) {
            char str_res[1000];
            snprintf(str_res, sizeof(str_res),
                    "Restart processes for timeout:%s", res_ptr);
            ProgError(STDLOG, "TCLMON: %s\n", str_res);
            tclmon_err_log_to_monitor(STDLOG, str_res, false);
        }
    }
}
/*****************************************************************/
void test_quefull_proc(Tcl_Interp* interp)
{
    static time_t time1_rest = 0;

    if (time1_rest == 0) {
        time1_rest = time(NULL);
    }

    time_t time2 = time(NULL);

    if (TO_TEST_QUEFULL_PROC <= 0) {

        return;
    }
    if (difftime(time2, time1_rest) <= TO_TEST_QUEFULL_PROC) {

        return;
    }

    time1_rest = time2;
    Tcl_Obj* pobj = Tcl_NewStringObj(ARRCMD_GET_QUEUE_SIZE_FOR_LOG, -1);
    if (pobj == NULL) {
        ProgError(STDLOG, ">>>>105Tcl_NewStringObj return NULL\n");

        return;
    }

    int Ret = Tcl_EvalObjEx(interp, pobj, TCL_EVAL_GLOBAL);
    const char* res_ptr = Tcl_GetStringResult(interp);
    if (Ret != TCL_OK) {
        ProgError(STDLOG, "TestQueFull failed:%s\n", res_ptr);

        return;
    }
    if (res_ptr != NULL) {
        if (strlen(res_ptr) > 0) {
            ProgError(STDLOG, "TCLMON: %s\n", res_ptr);
            tclmon_err_log_to_monitor(STDLOG, const_cast<char*>(res_ptr), false);
        }
    }
}
/*****************************************************************/
void test_not_asked_proc(Tcl_Interp* interp)
{
    static time_t time1_rest = 0;

    if (time1_rest == 0) {
        time1_rest = time(NULL);
    }

    time_t time2 = time(NULL);

    if (TO_TEST_NOT_ASKED_PROC <= 0) {

        return;
    }
    if (difftime(time2, time1_rest) <= TO_TEST_NOT_ASKED_PROC) {

        return;
    }
    time1_rest = time2;

    Tcl_Obj* pobj_cmd = Tcl_NewStringObj(ARRCMD_TEST_NOT_ASKED_PROC, -1);
    if (pobj_cmd == NULL) {
        ProgError(STDLOG, ">>>>10Tcl_NewStringObj return NULL\n");

        return;
    }

    int Ret = Tcl_EvalObjEx(interp, pobj_cmd, TCL_EVAL_GLOBAL);
    if (Ret != TCL_OK) {
        ProgError(STDLOG, "TestNotAsked failed:%s\n", Tcl_GetStringResult(interp));

        return;
    }
    Tcl_Obj* pobj_res = Tcl_GetObjResult(interp);
    if (pobj_res == NULL) {

        return;
    }
    Tcl_IncrRefCount(pobj_res);

    int objc;
    Ret = Tcl_ListObjLength(interp, pobj_res, &objc);
    if (Ret != TCL_OK ||
            objc <= 0) {
        Tcl_DecrRefCount(pobj_res);

        return;
    }

    for (int i = 0; i < objc; i++) {
        Tcl_Obj* to;
        if (Tcl_ListObjIndex(interp, pobj_res, i, &to) != TCL_OK) {

            continue;
        }
        if (to == NULL) {

            continue;
        }

        int proc_pid;
        if (TCL_OK != Tcl_GetIntFromObj(interp, to, &proc_pid)) {

            continue;
        }

        ProcS* p_pr = find_proctab_by_pid(proc_pid);
        if (p_pr == NULL) {

            continue;
        }
        ProcS& p_proctab = *p_pr;
        if (ISNSET_BIT(p_proctab.proc_flag, PROC_FL_KILLNOTASKED)) {

            continue;
        }

        ProgError(STDLOG, "Process pid=%d not asked, send needset_flag\n", proc_pid);
        write_needset_flag_tclmon(interp, p_proctab);
        write_set_flag_tclmon(interp, proc_pid, 10, 0, QUEPOT_ZAPR);
    }
    Tcl_DecrRefCount(pobj_res);
}
/*****************************************************************/
static void write_needset_flag_tclmon(Tcl_Interp* interp, ProcS& p_proctab_to)
{
    _Message_ s_mes_send;

    s_mes_send.set_mes_cmd(TCLCMD_NEEDSET_FLAG);
    put_message_to_que(s_mes_send, p_proctab_to);
}
/*****************************************************************/
static int set_start_mainproc(Tcl_Interp* interp)
{
    char str_pid[100];
    char str_no[100];

    Pid_p = getpid();

    Tcl_Obj* script_obj = Tcl_NewObj();
    if (script_obj == NULL) {

        return TCL_ERROR;
    }

    sprintf(str_no, "%d", 0);
    sprintf(str_pid, "%d", Pid_p);
    Tcl_AppendStringsToObj(script_obj, ARRCMD_CREATE_ARR_NO, " ",
            str_no, " ", get_action_by_num(A_CREATE), " ",
            " [ list ", Tcl_Concat(Argc, Argv), " ] ", NULL);
    if (TCL_OK != Tcl_EvalObjEx(interp, script_obj, TCL_EVAL_GLOBAL)) {
        ProgError(STDLOG, "Tcl_EvalObjEx(%s) Error\n%s\n",
                ARRCMD_CREATE_ARR_NO, Tcl_GetStringResult(interp));

        return TCL_ERROR;
    }

    script_obj = Tcl_NewObj();
    if (script_obj == NULL) {

        return TCL_ERROR;
    }
    Tcl_AppendStringsToObj(script_obj, "set_start_arr_no ",
            str_no, " ", "tclmon", " ", str_pid, " ", get_action_by_num(A_WORK), NULL);
    if (TCL_OK != Tcl_EvalObjEx(interp, script_obj, TCL_EVAL_GLOBAL)) {
        ProgError(STDLOG, "Tcl_EvalObjEx Error:\n%s\n",
                Tcl_GetStringResult(interp));

        return TCL_ERROR ;
    }

    return TCL_OK;
}
/*****************************************************************/
static int lock_startup(ClientData cl, Tcl_Interp* interp,
        int objc, Tcl_Obj* CONST objv[])
{
    struct flock lock;
    objc--;
    objv++;

    if (objc < 1) {
        Tcl_ResetResult(interp);
        Tcl_AppendResult(interp, "error in args", NULL);
        return TCL_ERROR;
    }

    char* filename = Tcl_GetString(objv[0]);
    int fd = open(filename, O_WRONLY);
    if (fd < 0) {
        Tcl_ResetResult(interp);
        Tcl_AppendResult(interp, "cannot open file ", filename, " ", strerror(errno), NULL);
        return TCL_ERROR;
    }

    addToCloseFd(fd, 0);
    lock.l_type = F_WRLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;

    int res = fcntl(fd, F_SETLK, &lock);
    if (res < 0) {
        Tcl_ResetResult(interp);
        if (errno == EAGAIN || errno == EACCES) {
            Tcl_AppendResult(interp, "cannot put lock on ", filename, " ",
                    "another process is running", NULL);
            res = fcntl(fd, F_GETLK, &lock);
            if (res == 0) {
                char tmp[50];
                sprintf(tmp, " with pid %d", lock.l_pid);
                Tcl_AppendResult(interp, tmp, 0);
            }
        } else {
            Tcl_AppendResult(interp, "cannot put lock", filename, " ", strerror(errno), NULL);
        }
        return TCL_ERROR;
    }
    return TCL_OK;
}
/*****************************************************************/
int get_n_testing_processes()
{
    int n = 0;
    for (int i = 0; i < NPROC; i++) {
        ProcS& s_pr = ProcTab[i];
        if (s_pr.action != A_NONE && ISSET_BIT(s_pr.proc_flag, PROC_FL_SPEC)) {
            n++;
        }
    }
    return n;
}
/*****************************************************************/
static int tcl_set_testing_mode(ClientData cl, Tcl_Interp* interp, int objc,
        Tcl_Obj* CONST objv[])
{
    int mode_tmp = 0;

    objc--;
    objv++;
    if (objc >= 1) {
        if (TCL_OK != Tcl_GetIntFromObj(interp, *objv, &mode_tmp)) {
            Tcl_SetObjResult(interp,
                    Tcl_NewStringObj("set_testing_mode: need integer arg", -1));
            return TCL_OK;
        }
    }
    if (mode_tmp < 0) {
        mode_tmp = 0;
    }
    if (mode_tmp != TclmonTestingMode) {
        TclmonTestingMode = mode_tmp;
    }
    return TCL_OK;
}
/*****************************************************************/
int getStopAllObrOnSig25(Tcl_Interp* interp)
{
    static int fl = -1;

    if (fl == -1) {
        fl = 0;
        Tcl_Obj* ptr = Tcl_GetVar2Ex(getTclInterpretator(), "STOPOBR_WHEN25SIG", NULL, TCL_GLOBAL_ONLY);
        if (ptr) {
            if (strstr("#yes#Yes#YES#ON#on#On#true#True#TRUE#1#", Tcl_GetString(ptr))) {
                fl = 1;
            }
        }
    }
    return fl;
}
/*****************************************************************/
int getExtendDiagnosticLog()
{
    static int fl = -1;

    if (fl == -1) {
        fl = 0;
        Tcl_Obj* ptr = Tcl_GetVar2Ex(getTclInterpretator(), "EXTEND_DIAGNOSTIC_LOG", NULL, TCL_GLOBAL_ONLY);
        if (ptr) {
            if (strstr("#yes#Yes#YES#ON#on#On#true#True#TRUE#1#", Tcl_GetString(ptr))) {
                fl = 1;
            }
        }
    }

    return fl;
}
/*****************************************************************/
static int watch(Tcl_Interp* interp)
{
    int need_stop_proc_obr = 0;

    if (App_start) {
        int App_start_res = App_start(interp);
        switch (App_start_res) {
        case TCL_OK:
            break;
        case TCL_RETURN:
            return TCL_OK;
        default:
            return TCL_ERROR;
        }
    }

    set_start_mainproc(interp);
    reset_potok_proc();

    struct pollfd* descr_tab = (struct pollfd*) calloc(2 * NPROC, sizeof descr_tab[0]);
    int* ij = (int*)calloc(2 * NPROC, sizeof ij[0]);
    if (!descr_tab || !ij) {
        ProgError(STDLOG, "calloc failed\n");
        free(descr_tab);
        free(ij);
        return 1;
    }

    if(set_sig (finish, SIGTERM) < 0 or set_sig (finish, SIGINT) < 0)
        return TCL_ERROR;

    while (1) {
        if (stopped_by_signal) {
            return exit_main_proc(interp, false);
        }
        if (TclmonTestingMode == 1) {
            if (get_n_testing_processes() == 0) {
                return exit_main_proc(interp, false);
            }
        }
        if (need_stop_proc_obr != 0) {
            if (getStopAllObrOnSig25(interp) == 1) {
                int n_pids = stop_proc_obr(interp);
                ProgError(STDLOG, "%d obr processes stopped, because 'File size limit exceeded'\n", n_pids);
            }
            need_stop_proc_obr = 0;
        }
        int j = 0;
        for (int i = 0; i < NPROC; i++) {
            ProcS& s_pr = ProcTab[i];
            if ((s_pr.action == A_WORK) || (A_FORCED_RESTART == s_pr.action) || (A_FINAL == s_pr.action)) {
                descr_tab[j].fd = s_pr.fd;
                descr_tab[j].events = POLLIN;
                if( ! s_pr.proc_que_out.is_empty() ) {
                    descr_tab[j].events |= POLLOUT;
                }
                ij[j++] = i;
            }
        }
        int ndescr = j;
        if (RunTimeChecker::Instance().markRunTime()) {
            ProgError(STDLOG, "mark_run_time return error");
        }

        int pres = poll(descr_tab, ndescr, 15000);
        if (pres < 0) {
            if (errno != EINTR) {
                free(descr_tab);
                free(ij);
                return 1;
            }
        } else {
            reopenLog();
            for (int i = 0; (i < ndescr) && (pres > 0); ++i) {
                ProcS& p_proctab = ProcTab[ij[i]];
                if (p_proctab.fd == -1) {
                    ProgError(STDLOG, "%d fd=%d proc was killed", i, p_proctab.fd);
                    continue;
                }
                short revents = descr_tab[i].revents;
                if (revents) {
                    pres--;
                    if (revents & (POLLERR | POLLNVAL)) {
                        descr_tab[i].fd = -1;
                        if (A_WORK == p_proctab.action) {
                            kill_proc_restart_stop(interp , p_proctab, SIGTERM, A_RESTART, time(0) + 10, 0);
                        }
                        continue;
                    } else if (revents & (POLLHUP)) {
                        descr_tab[i].fd = -1;
                        if (A_WORK == p_proctab.action) {
                            kill_proc_restart_stop(interp , p_proctab, SIGTERM, A_RESTART, time(0) + 10, time(0) + 2);
                        }
                        continue;
                    }
                    if (revents & POLLIN) {
                        _MesBl_ *p_mbl = p_proctab.p_mbl;
                        if (!p_mbl) {
                            ProgError(STDLOG, "%d POLLIN on %d - empty message", i, p_proctab.fd);
                            break;
                        }
                        int Ret = recv_mbl(descr_tab[i].fd, p_mbl);
                        if (Ret < 0) {
                            descr_tab[i].fd = -1;
                            kill_proc_restart_stop(interp , p_proctab, SIGTERM, A_RESTART, time(0) + 10, time(0) + 2);
                            break;
                        } else if (Ret > 0) {
                            switch (p_mbl->get_mes_cmd()) {
                            case TCLCMD_SET_FLAG:
                                obr_cmd_set_flag(interp, p_mbl->get_mes_text(), p_mbl->get_mes_len());
                                break;
                            case TCLCMD_SHOW:
                            case TCLCMD_COMMAND:
                            case TCLCMD_CUR_REQ:
                                obr_cmd_command(interp, p_proctab, p_mbl);
                                break;
                            default: {
                                LogError(STDLOG)<<"ERROR: receive mes from pipe with wrong command ("<< p_mbl->get_mes_cmd()<<") from process proc_num="<<p_proctab.proc_num<<" with pid="<<p_proctab.ppid;
                                kill_proc_restart_stop(interp , p_proctab, SIGTERM, A_RESTART, time(0) + 10, time(0) + 2);
                            }
                                break;
                            }
                            reset_mbl(p_mbl);
                        }
                    }
                    if (revents & POLLOUT) {
                        _MesBl_ *p_mbl = p_proctab.proc_que_out.get_first();
                        if (!p_mbl) {
                            ProgError(STDLOG, "%d POLLOUT on %d - empty message", i, p_proctab.fd);
                            break;
                        }
                        int Ret = send_mbl(descr_tab[i].fd, p_mbl);
                        if (Ret < 0) {
                            free_mbl(p_mbl);
                            descr_tab[i].fd = -1;
                            kill_proc_restart_stop(interp , p_proctab, SIGTERM, A_RESTART, time(0) + 10, time(0) + 2);
                            break;
                        } else if (Ret == 0) {
                            p_proctab.proc_que_out.put_first(p_mbl);
                        } else if (Ret > 0) {
                            free_mbl(p_mbl);
                        }
                    }
                }
            }                       /* for */
        }

        waitchld_do(interp, &need_stop_proc_obr);

        test_set_flag_tclmon(interp);
        test_restart_proc(interp);
        test_quefull_proc(interp);
        test_not_asked_proc(interp);

        for (int i = 0; i < NPROC; i++) {
            ProcS& pr = ProcTab[i];
            if (pr.action == A_RESTART || pr.action == A_STOP) {
                if (pr.ppid != -1) {
                    if ((time(0) - pr.kill_time) > 0) {
                        pr.kill_time = time(0) + 5;
                        int ret = kill(pr.ppid, SIGKILL);
                        if (ret == -1) {
                            if (errno == ESRCH) {
                                set_stop_pid_process(interp, pr);
                            }
                        }
                    }
                }

                if (pr.ppid == -1) {
                    if ((time(0) - pr.delay) > 0) {
                        if (pr.action == A_RESTART) {
                            const char* p_name = find_proc_name_by_no(pr.proc_num);
                            if (start_proc(pr, interp) < 0) {
                                ProgError(STDLOG, "failed to restart %s\n", p_name);
                            }
                        }
                    }
                }
            }
        }
    }
    free(descr_tab);
    free(ij);
}
/*****************************************************************/
void send_mbl_result_by_TclResult(Tcl_Interp* interp, ProcS& p_proctab,
        _MesBl_ *p_mbl, int tcl_ret)
{
    _Message_ &s_message = p_mbl->get_message();
    s_message.set_mes_len(0);

    const char* res_ptr = Tcl_GetStringResult(interp);
    int len = res_ptr ? strlen(res_ptr) : 0;
    if (len > 0) {
        int real_len = s_message.set_mes_text__rest_to_tail(res_ptr, len);
        if (real_len != len) {
            // Не смогли полностью уместить необходимый текст, выведем сообщение об ошибке.
            std::string s_ret = "Answer is too long: " + HelpCpp::string_cast(len) + " bytes";
            s_message.set_mes_text__rest_to_tail(s_ret.c_str(), s_ret.length());
            tcl_ret = TCL_ERROR;
        }
    }

    s_message.set_mes_ret((tcl_ret == TCL_OK) ? TCL_OK : TCL_ERROR);
    put_message_to_que(s_message, p_proctab);
}
/*****************************************************************/
static void obr_cmd_set_flag(Tcl_Interp* interp, char* str_cmd, int len_cmd)
{
    Tcl_Obj* pobj = Tcl_NewStringObj(str_cmd, len_cmd);
    if (pobj == NULL) {
        ProgError(STDLOG, ">>>>11Tcl_NewStringObj return NULL\n");
    } else {
        int ret = Tcl_EvalObjEx(interp, pobj, TCL_EVAL_GLOBAL);
        if (ret != TCL_OK) {
            ProgError(STDLOG, "set_flag error:%s", Tcl_GetStringResult(interp));
        }
    }
}
/*****************************************************************/
static void obr_cmd_command(Tcl_Interp* interp, ProcS& p_proctab, _MesBl_ *p_mbl)
{
    Tcl_Obj* pobj = Tcl_NewStringObj(p_mbl->get_mes_text(), p_mbl->get_mes_len());
    if (pobj == NULL) {
        ProgError(STDLOG, ">>>>12Tcl_NewStringObj return NULL\n");
    } else {
        int ret = Tcl_EvalObjEx(interp, pobj, TCL_EVAL_GLOBAL);
        if (p_mbl->get_mes_cmd() == TCLCMD_COMMAND) {
            send_mbl_result_by_TclResult(interp, p_proctab, p_mbl, ret);
        }
    }
}
/*****************************************************************/
static int command_obr_tcl(ClientData cl, Tcl_Interp* interp, int objc,
        Tcl_Obj* CONST objv[])
{
    if (objc < 4) {
        Tcl_ResetResult(interp);
        Tcl_AppendResult(interp, "Wrong number of arguments", NULL);
        return TCL_ERROR;
    }

    objc--;
    objv++;

    char* str_proc_name = Tcl_GetString(objv[0]);

    objc--;
    objv++;

    int proc_pid;
    if (TCL_OK != Tcl_GetIntFromObj(interp, objv[0], &proc_pid)) {
        Tcl_ResetResult(interp);
        Tcl_AppendResult(interp, "Wrong proc_pid", NULL);
        return TCL_ERROR;
    }

    ProcS* p_proctab_to = NULL;
    for (int i = 0; i < NPROC; i++) {
        if (ProcTab[i].ppid == proc_pid) {
            const char* p_name = find_proc_name_by_no(ProcTab[i].proc_num);
            if (strcmp(p_name, str_proc_name) != 0) {
                Tcl_ResetResult(interp);
                Tcl_AppendResult(interp, "error in args: wrong proc_pid for proc_name", NULL);
                return TCL_ERROR;
            }
            p_proctab_to = &(ProcTab[i]);
            break;
        }
    }
    if (p_proctab_to == NULL) {
        Tcl_ResetResult(interp);
        Tcl_AppendResult(interp, "error in args: proc_pid not found", NULL);
        return TCL_ERROR;
    }

    objc--;
    objv++;

    Tcl_Obj* cmd_obj = Tcl_ConcatObj(objc, objv);
    if (cmd_obj == NULL) {
        ProgError(STDLOG, ">>>>23Tcl_NewStringObj return NULL\n");
        Tcl_ResetResult(interp);
        Tcl_AppendResult(interp, "Not enough memory for obj", NULL);
        return TCL_ERROR;
    }

    char* str_cmd = Tcl_GetString(cmd_obj);
    int len = strlen(str_cmd);

    _Message_ s_mes_send;
    s_mes_send.set_mes_text(str_cmd, len);
    s_mes_send.set_mes_cmd(TCLCMD_PROCCMD);
    put_message_to_que(s_mes_send, *p_proctab_to);

    Tcl_ResetResult(interp);
    Tcl_AppendResult(interp, "OK", NULL);
    return TCL_OK;
}
/*****************************************************************/
static void put_message_to_que(_Message_ &s_message, ProcS& pr)
{
    _MesBl_ *p_mbl = new_mbl();
    if (p_mbl == NULL) {
        return;
    }
    p_mbl->get_message()._copy(s_message);

    p_mbl->prepare_to_send();

    if( ! pr.proc_que_out.put_last(p_mbl) ) {
        free_mbl(p_mbl);
    }

}
/*****************************************************************/
/* Проверяет количество неостановленных процессов*/
int check_stop_processes(void)
{
    int n = 0;

    for (int i = 0; i < NPROC; i++) {
        if (ProcTab[i].ppid != -1) {
            n++;
        }
    }
    return n;
}
/*****************************************************************/
int stop_all_processes(Tcl_Interp* interp, int sig, int to_wait)
{
    for (int i_pr = 0; i_pr < NPROC; i_pr++) {
        ProcS& pr = ProcTab[i_pr];
        if (pr.action != A_NONE) {
            kill_proc(interp, pr, sig, A_STOP, time(0) + 5, 0);
        }
    }

    for (int i_to = 0; i_to < to_wait; i_to++) {
        int cur_need_stop_proc_obr = 0;
        waitchld_do(interp, &cur_need_stop_proc_obr);
        if (check_stop_processes() == 0) {
            return 0;
        }
        sleep(1);
    }

    int Ret = check_stop_processes();
    return Ret;
}
/*****************************************************************/
/* flag = 0 - exit */
/*        1 - exec */
int exit_main_proc(Tcl_Interp* interp, bool restart)
{
    LogTrace(TRACE1) << "Exit_main_proc";

    int Ret = stop_all_processes(interp, SIGTERM, 5);
    if (Ret != 0) {
        stop_all_processes(interp, SIGKILL, 2);
    }

    clist_uninit();
    ProgError(STDLOG, "Before_Exit_main_proc");
    if (before_exit_main_proc) {
        before_exit_main_proc();
    }
    closeAndShut();
    closeLog();
    monitorControl::close_control_server();

    if (not inTestMode())
        for (int i = 0; i < FD_TABLE_SIZE; i++) {
            if (fcntl(i, F_GETFL) != -1) {
                fprintf(stderr, "%i IS OPEN\n", i);
            }
        }

    csaHolder->destroy();
    semHolder->destroy();
    if (restart == 1) {
        closeAndShutTclMain();
        execv(Argv[0], Argv);
        fprintf(stderr, "Execv error (errno=%d)\n", errno);
        exit(1);
    } else {
        exit(0);
    }
}
/*********************************************************/
static int restartHandlers(Tcl_Interp* interp)
{
    LogTrace(TRACE1) << "restartHandlers(...)";

    int restartedCount = 0;
    const ProcS* end = ProcTab + NPROC;
    for(ProcS* pr = ProcTab; pr != end; ++pr) {
        if ((A_WORK == pr->action)
                && (ISNSET_BIT(pr->proc_flag, PROC_FL_DISPATCHER))
                && (ISNSET_BIT(pr->proc_flag, PROC_FL_MAIN))) {
            _Message_ mes;
            mes.set_mes_text("exit", 4);
            mes.set_mes_cmd(TCLCMD_PROCCMD);
            put_message_to_que(mes, *pr);
            set_action_process(interp, *pr, A_FORCED_RESTART);
            ++restartedCount;
            restartedPids.insert(pr->ppid);
        }
    }

    return restartedCount;
}
/*********************************************************/

} //namespace Supervisor
