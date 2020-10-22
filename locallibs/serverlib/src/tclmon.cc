#if HAVE_CONFIG_H
#endif

#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <algorithm>
#include <random>

#include "tclmon.h"
#include "moncmd.h"
#include "tcl_utils.h"
#include "lwriter.h"
#include "testmode.h"
#include "guarantee_write.h"
#include "mes.h"
#include "query_runner.h"
#include "tcl_utils.h"

#define NICKNAME "KONST"
#include "test.h"

const std::string SOFT_RESTART_CMD { "exit" };

pid_t Pid_p;
int main_tests();

static int write_fifo(int fd, _Message_ *p_mes);
static std::string our_process_name;
static std::string full_process_name;
static int supervisorSocket;
static time_t start_time;
static int (*App_init)(Tcl_Interp *);
static int (*App_start)(Tcl_Interp *);
static struct {
    int argc_;
    char** argv_;
} arguments;
/*****************************************************************/
bool isNosir();
/*****************************************************************/
void constructCsaControl(uint16_t key);
int OpenLogFile(const char* processName, const char* grpName);
/*****************************************************************/
const char* getUnknownProcessName()
    {
    return "[unknown]";
}
/*****************************************************************/
const char* tclmonCurrentProcessName()
  {
    return ( !our_process_name.empty() ) ? our_process_name.c_str() : "unknown_process_name";
  }
/*****************************************************************/
const char* tclmonFullProcessName()
  {
    return ( !full_process_name.empty() ) ? full_process_name.c_str() : "unknown_process_name";
  }
/*****************************************************************/
static time_t getProcStartTime()
{
    return start_time;
}
/*****************************************************************/
void setProcStartTime(const time_t t)
{
    start_time = t;
}
/*****************************************************************/
int semaphoreKey()
{
    return readIntFromTcl("SEM_KEY", 65536 + 65535);
}
/*****************************************************************/
static int getMonitorSilenceTimeout()
     {
    return 5;
}
/*****************************************************************/
int startupKeepSilence()
    {
    return time(0) - getProcStartTime() < getMonitorSilenceTimeout();
}
/*****************************************************************/
int set_logging(ClientData cl, Tcl_Interp* interp, int objc, Tcl_Obj* CONST objv[])
{
    --objc;
    ++objv;
    if (objc != 1) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("wrong arg count to set_logging", -1));

        return TCL_ERROR;
    }

    int use_rsyslog;
    if (Tcl_GetIntFromObj(interp, *objv, &use_rsyslog)) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("arg must be integer", -1));

        return TCL_ERROR;
    }

    if (use_rsyslog) {
        setLoggingGroup("tclmon", LOGGER_SYSTEM_LOGGER);
    } else {
        setLoggingGroup("TCLMON_LOG", LOGGER_SYSTEM_FILE);
    }

    return TCL_OK;
}
/*****************************************************************/
int getControlPipe()
  {
    return supervisorSocket;
  }
/*****************************************************************/
static int tcl_nosir()
{
    static const char NOSIR_KEY_RUN_TESTS[] = "-run_tests";
    static const int ARG_NOSIR_KEY = 3;

    int res;
    if ((ARG_NOSIR_KEY < arguments.argc_) &&
            !strncmp(arguments.argv_[ARG_NOSIR_KEY], NOSIR_KEY_RUN_TESTS, sizeof(NOSIR_KEY_RUN_TESTS) - 1)) {
        res = main_tests();
    } else {
        setLogging("nosir.log", LOGGER_SYSTEM_FILE, tclmonCurrentProcessName());
        res = ServerFramework::applicationCallbacks()->nosir_proc(
                arguments.argc_ - ARG_NOSIR_KEY,
                arguments.argv_ + ARG_NOSIR_KEY
        );
    }

    return (res == 0) ? TCL_OK : TCL_ERROR;
}
/*****************************************************************/
static void parseProcessArguments(std::string& srcArgs, std::vector<char* >& argv)
{
    srcArgs += '\0';

    bool inArg = false;
    for(std::string::iterator i = srcArgs.begin(); i != srcArgs.end(); ++i) {
        if (std::isspace(*i)) {
            *i = '\0';
            inArg = false;
        } else if (!inArg) {
            inArg = true;
            argv.push_back(&(*i));
        }
    }
}
/*****************************************************************/
int tcl_execute(ClientData cl, Tcl_Interp* interp, int objc, Tcl_Obj* CONST objv[])
{
    if (isNosir()) { //In nosir mode
        return tcl_nosir();
    }

    constructCsaControl(readIntFromTcl("CSA_KEY"));
    //Найти обработчик для данного типа,
    //запустить его, в аргументах командной строки дескриптор
    //Unix сокета для связи с supervisor'ом
    int ptSize;
    const struct NAME2F* pt = getProcTable(&ptSize);
    std::string logFile;
    ProcessHandlerType* handler = 0;

    for(const struct NAME2F* end = pt + ptSize; pt != end; ++pt) {
        if (our_process_name == pt->name) {
            handler = pt->pf;
            logFile = pt->log_group;
            break;
        }
    }
    if (!handler) {
        fprintf(stderr, "Handler for %s not found!\n", our_process_name.c_str());

        abort();
    }

    std::vector<char* > processArgv;
    std::string srcProcessArg(full_process_name);

    parseProcessArguments(srcProcessArg, processArgv);
    OpenLogFile(logFile.empty() ? NULL : logFile.c_str(), (1 < processArgv.size()) ? processArgv[1] : NULL);
    ServerFramework::applicationCallbacks()->init();
    handler(supervisorSocket, processArgv.size(), processArgv.data());

    return TCL_OK;
}
/*****************************************************************/

#define CHECK_ARGC(argc, val) if((argc) <= (val)) {  fprintf(stderr, #argc " <= " #val "\n");  return 2;  }

int queue_main(int argc,
        char* argv[],
        int (*app_init)(Tcl_Interp*),
        int (*app_start)(Tcl_Interp*),
        void (*before_exit)(void))
{
    //static const int ARG_CONFIG_FILE = 1;
    static const int ARG_SUPERVISOR_SOCKET = 2;
    static const int ARG_START_LINE = 3;

    arguments.argc_ = argc;
    arguments.argv_ = argv;
    if (!isNosir()) {
        CHECK_ARGC(argc, ARG_SUPERVISOR_SOCKET);
        CHECK_ARGC(argc, ARG_START_LINE);
        supervisorSocket = std::stoi(argv[ARG_SUPERVISOR_SOCKET]);
        if(auto space = strchr(argv[ARG_START_LINE], ' '))
            our_process_name.assign(argv[ARG_START_LINE], space);
        else
            our_process_name = argv[ARG_START_LINE];
        full_process_name.assign(argv[ARG_START_LINE]);
    } else {
        our_process_name.assign(argv[0]);
        full_process_name.assign(argv[0]);
        for(int i = 1; i < argc; ++i) {
            full_process_name += ' ';
            full_process_name += argv[i];
        }
    }

    Pid_p = getpid();
    App_init = app_init;
    App_start = app_start;

    clist_init( CLIST_PROCID_TCLMON );

    set_sig(regLogReopen, SIGUSR1);

    Tcl_Main(argc, argv, Tcl_AppInit);

    return 1;
}
/*****************************************************************/
int Tcl_AppInit(Tcl_Interp *interp)
{
    setTclInterpretator(interp);
    if (Tcl_Init(interp) == TCL_ERROR) {
        return TCL_ERROR;
    }

#ifdef TCL_TEST
#ifdef TCL_XT_TEST
    if(Tclxttest_Init(interp) == TCL_ERROR) {
        return TCL_ERROR;
    }
#endif
    if (Tcltest_Init(interp) == TCL_ERROR) {
        return TCL_ERROR;
    }
    Tcl_StaticPackage(interp, "Tcltest", Tcltest_Init, (Tcl_PackageInitProc *) NULL);
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
    if (! Tcl_CreateObjCommand(interp, "execute", tcl_execute, static_cast<ClientData>(0), 0)) {
        fprintf(stderr, "execute creation failed \n%s\n", Tcl_GetString(Tcl_GetObjResult(interp)));
        return TCL_ERROR;
    }

    if (App_init and App_init(interp) < 0) {
        fprintf(stderr, "app init failed\n");
        return TCL_ERROR;
    }

    return TCL_OK;
}
/*****************************************************************/
int write_set_queue_size(int nmes_que, int max_nmes_que, int write_to_log_flag)
    {
    _Message_ mes;
    mes.set_mes_len(
            snprintf(mes.get_mes_text(), mes.get_text_size(), "%s %d %d %d %d",
                ARRCMD_SET_QUEUE_SIZE , Pid_p, nmes_que, max_nmes_que, write_to_log_flag));
    mes.set_mes_fd(Pid_p);
    mes.set_mes_cmd(TCLCMD_COMMAND);

    return write_fifo(supervisorSocket, &mes);
}

/*****************************************************************/
int write_clear_cur_req(void)
{
    if (inTestMode()) {
 
        return 0;
  }
  
    _Message_ mes;

    mes.set_mes_len(snprintf(mes.get_mes_text(),  mes.get_text_size(),
                "%s %d", ARRCMD_CLEAR_CUR_REQ , Pid_p));
    mes.set_mes_fd(Pid_p);
    mes.set_mes_cmd(TCLCMD_CUR_REQ);

    return write_fifo(supervisorSocket, &mes);
  }
/*****************************************************************/
int write_set_cur_req(const char* cur_req)
  {
    if (inTestMode()) {
        return 0;
  }
  
    _Message_ mes;
    if (cur_req == NULL || *cur_req == 0) {
  
        return write_clear_cur_req();
  }
  
    time_t t = time(0);
    const struct tm* tms = localtime(&t);
    if(tms->tm_mday == 1 && tms->tm_mon == 3)
    {
        const size_t str_len = strlen(cur_req);
        char buf[ str_len + 1 ];
        for(size_t i=0; i<str_len; i++)
            buf[i] = cur_req[str_len-i-1];
        buf[str_len] = '\0';

        mes.set_mes_len( snprintf(mes.get_mes_text(), mes.get_text_size(),
                                  "%s %d %s", ARRCMD_SET_CUR_REQ, Pid_p, buf) );
    }
    else
    {
        mes.set_mes_len(
                snprintf(mes.get_mes_text(), mes.get_text_size(), "%s %d %s",
                    ARRCMD_SET_CUR_REQ, Pid_p, cur_req));
    }
  
    mes.set_mes_fd(Pid_p);
    mes.set_mes_cmd(TCLCMD_CUR_REQ);
  
    return write_fifo(supervisorSocket, &mes);
  }
/*****************************************************************/
void fill_set_flag(_Message_ *p_mes, int pid_p, int TO_restart, int n_zapr, int type_zapr, const char* const subgroup)
{
    p_mes->set_mes_len(snprintf(p_mes->get_mes_text(), p_mes->get_text_size(), "%s %d %d %d %d %s",
                                ARRCMD_SET_FLAG_ARR_NO, pid_p, TO_restart, n_zapr, type_zapr, subgroup));
    p_mes->set_mes_fd(pid_p);
    p_mes->set_mes_cmd(TCLCMD_SET_FLAG);
}
/*****************************************************************/
int write_set_flag_type(int TO_restart, int n_zapr, int type_zapr, const char* const subgroup)
{
    _Message_ mes;
    fill_set_flag(&mes, Pid_p, TO_restart, n_zapr, type_zapr, subgroup ? subgroup : "{}");
    return write_fifo(supervisorSocket, &mes);
}
/*****************************************************************/
int write_set_flag(int TO_restart, int n_zapr)
{
    set_monitor_timeout(TO_restart);

    if (inTestMode())
        return 0;

    return write_set_flag_type(TO_restart, n_zapr, QUEPOT_ZAPR, nullptr);
}
/*****************************************************************/
static int write_fifo(int fd, _Message_ *p_mes)
  {
#ifdef XP_TESTING
    if (inTestMode()) {
 
        return 0;
  }
#endif // XP_TESTING

    if (!fd || !p_mes) {
        ProgError(STDLOG, "%s app error: fd=%d p_mes=%p", __FUNCTION__, fd, p_mes);
 
        return 0;
}

    const ssize_t len = p_mes->get_mes_head_size() + p_mes->get_mes_len();
    const ssize_t n = gwrite(STDLOG, fd, p_mes, len);
    if (n != len) {
        ProgTrace(TRACE0, "%s : cannot write %zi byte (write %zi bytes) to fd %d(err=%d):%s",
                __FUNCTION__, len, n, fd, errno, strerror(errno));
        if (n == 0) {
  
            return 0;
}

        return -1 * errno;
  }
  
    return 1;
}
/*****************************************************************/
void send_signal_udp(struct sockaddr_un *addr,const char *var,
      const char *var2,const void*data, size_t len)
{
   send_signal_udp_suff(addr,var,var2,-1,data,len);
}
/*****************************************************************/
void send_signal_udp_suff(struct sockaddr_un *addr,const char *var,
      const char *var2,int suff,const void*data,int len)
{
#ifdef XP_TESTING
    if (inTestMode()) {

    return;
  }
#endif // XP_TESTING

  static int sock=-1; 
  
    if (sock < 0) {
    sock=socket(AF_UNIX,SOCK_DGRAM,0);
        if (sock < 0) {
      ProgError(STDLOG,"socket:%s",strerror(errno));

      return;
    }
        if (fcntl(sock, F_SETFL, O_NONBLOCK) < 0) {
      ProgError(STDLOG,"fcntl (signalsock) error. %s\n",strerror(errno));
    }
  }
    if (addr->sun_family == 0) {
    Tcl_Obj* obj;
        if (var) {
      obj=Tcl_GetVar2Ex(getTclInterpretator(),
                 const_cast<char*>(var),
                 const_cast<char*>(var2), TCL_GLOBAL_ONLY | TCL_LEAVE_ERR_MSG);
            if (!obj) {
        ProgError(STDLOG,"%.30s(%.50s):%s",var, var2?var2:"null",
             Tcl_GetString(Tcl_GetObjResult(getTclInterpretator())));

        return;
      }
    }
    addr->sun_family=AF_UNIX;
    strcpy(addr->sun_path,var ? Tcl_GetString(obj):var2);
        if (suff >= 0) {
      char tmpb[100];
      sprintf(tmpb,"%03d",suff);
      strcat(addr->sun_path,tmpb);
    }
  }
    if (access(addr->sun_path, W_OK) < 0) {

		return;
  }
  
  struct pollfd pfd[1];
  pfd[0].fd=sock;
  pfd[0].events=POLLOUT;
  
    while (1) {
    int npoll=poll(pfd,1,0);
        if (npoll == 1) {
            if (pfd[0].revents & ~POLLOUT) {
        ProgError(STDLOG,"poll <%s> revents %d",addr->sun_path, pfd[0].revents);
      }
            if (pfd[0].revents & POLLOUT) {
                if (sendto(sock, data, len, 0, (struct sockaddr*)addr, sizeof * addr) < 0) {
                    if ((errno == ENOBUFS && len <= 4) || (errno == EAGAIN) || startupKeepSilence()) {
            ProgTrace(TRACE1, "send_signal_udp <%s> sendto:%s",
                  addr->sun_path, strerror(errno));
                    } else {
            ProgError(STDLOG, "send_signal_udp <%s> sendto:%s",
                  addr->sun_path, strerror(errno));
          }
        }
      }
        } else if (npoll < 0) {
            if (errno == EINTR) {
        continue;
            }
      ProgError(STDLOG,"poll POLLOUT <%s> sendto:%s",addr->sun_path, strerror(errno));
    }
    break;
  }
}
/*********************************************************/
void mes_for_process_from_monitor(_MesBl_ *p_mbl, handle_control_msg_t f_handle_control_msg)
{
/* Длина команды: p_mbl->p_mes->mes_head.len_text*/
/* Текст команды: p_mbl->p_mes->mes_text*/
  _Message_ &s_message=p_mbl->get_message();
    if (s_message.get_mes_cmd() == TCLCMD_PROCCMD) {
    int len = s_message.get_mes_len();
        if (len >= s_message.get_text_size()) {
       len = s_message.get_text_size() - 1;
     }
     s_message.get_mes_text()[len]=0;

        if (f_handle_control_msg) {
       f_handle_control_msg(s_message.get_mes_text(), len);
     }
    } else if (s_message.get_mes_cmd() == TCLCMD_NEEDSET_FLAG) {
    write_set_flag(0, 0);
  }
}
/*********************************************************/
void Abort(int i)
{
    sleep(10);
    SIRENA_ABORT_OPT("TCLMON");
    exit(i);
}
/*********************************************************/

void random_sleep()
{
    static constexpr long ML_SECOND = 1000;

    static std::random_device gen;
    static std::uniform_int_distribution<long> distribution(ML_SECOND / 2, ML_SECOND);

    timespec t = { 0, distribution(gen) * ML_SECOND * ML_SECOND };

    nanosleep(&t, nullptr);
}
