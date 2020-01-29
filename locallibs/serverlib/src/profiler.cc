#if HAVE_CONFIG_H
#endif

/**
  * profiling futures
  * based on google profiler
*/
#include <dlfcn.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include "tcl_utils.h"

#include <boost/optional.hpp>

#include "profiler.h"

#define NICKNAME "ROMAN"
#include "slogger.h"


namespace {
const char * dirname = "profiles";

static void *profiler_lib = 0;

typedef int  (*ProfilerStart_t) (const char *);
typedef void (*ProfilerStop_t)  ();
typedef void (*ProfilerRegisterThread_t) ();
typedef void (*ProfilerFlush_t) ();
typedef void (*ProfilerEnable_t) ();
typedef void (*ProfilerDisable_t) ();
typedef void (*ProfilerGetCurrentState_t) (struct ProfilerState* state);

static ProfilerStart_t          ProfilerStart = 0;
static ProfilerStop_t           ProfilerStop  = 0;
static ProfilerRegisterThread_t ProfilerRegisterThread = 0;
static ProfilerFlush_t          ProfilerFlush = 0;
static ProfilerEnable_t         ProfilerEnable = 0;
static ProfilerDisable_t        ProfilerDisable = 0;
static ProfilerGetCurrentState_t ProfilerGetCurrentState = 0;

const char * get_prof_libname()
{
    const char * def_lib = "libprofiler.so";
    static std::string libname = "";

    if(libname.empty()) {
        libname = readStringFromTcl("GOOGLE_PROFILER_LIBNAME", def_lib);
    }

    return libname.c_str();
}
/*
void * load_symbol(void * lib, const char *symb_name)
{
    void * symbol = dlsym(lib, symb_name);
    const char *dlsym_error = dlerror();
    if (dlsym_error) {
        LogWarning(STDLOG) << "Cannot load symbol '" << symb_name << "': " << dlsym_error;
        return 0;
    }

    return symbol;
}
*/
int unload_profiler(void ** profiler_lib)
{
    if(!*profiler_lib)
        return 0;

    // close the library
    LogTrace(TRACE1) << "Closing library " << get_prof_libname() << " ...";
    dlclose(*profiler_lib);

    *profiler_lib = 0;
    ProfilerStart = 0;
    ProfilerStop  = 0;
    ProfilerRegisterThread = 0;
    ProfilerFlush = 0;
    ProfilerEnable= 0;
    ProfilerDisable = 0;
    ProfilerGetCurrentState = 0;

    return 0;
}

int load_profiler(void ** profiler_lib)
{
    LogTrace(TRACE1) << "Loading " << get_prof_libname();

    *profiler_lib = dlopen(get_prof_libname(), RTLD_LAZY);

    if (!*profiler_lib) {
        LogWarning(STDLOG) << "Cannot open library: " << dlerror();
        return -1;
    }

    // load the symbol
    LogTrace(TRACE3) << "Loading symbols ...";

    // reset errors
    dlerror();
    ProfilerStart = (ProfilerStart_t) dlsym(*profiler_lib, "ProfilerStart");
    ProfilerStop  = (ProfilerStop_t)  dlsym(*profiler_lib, "ProfilerStop");
    ProfilerRegisterThread = (ProfilerRegisterThread_t) dlsym(*profiler_lib, "ProfilerRegisterThread");
    ProfilerFlush = (ProfilerFlush_t) dlsym(*profiler_lib, "ProfilerFlush");
    ProfilerEnable= (ProfilerEnable_t) dlsym(*profiler_lib, "ProfilerEnable");
    ProfilerDisable = (ProfilerDisable_t) dlsym(*profiler_lib, "ProfilerDisable");
    ProfilerGetCurrentState = (ProfilerGetCurrentState_t) dlsym(*profiler_lib, "ProfilerGetCurrentState");

    if(!ProfilerStart
            || !ProfilerStop
            || !ProfilerRegisterThread
            || !ProfilerFlush
            || !ProfilerEnable
            || !ProfilerDisable
            || !ProfilerGetCurrentState)
    {
        unload_profiler(profiler_lib);
        return -1;
    }

    return 0;
}

void mkdir_prof()
{
    mkdir(dirname, 0755);
}

const std::string mk_filename(const char *process_name)
{
    char prof_filename[100] = "";
    char datetime[30] = "";
    time_t t;
    struct tm *tmp;

    mkdir_prof();

    t = time(NULL);
    tmp = localtime(&t);

    strftime(datetime, sizeof(datetime), "%y%m%d-%H%M%S", tmp);
    snprintf(prof_filename, sizeof(prof_filename), "%s/%.30s_%s_%u.prof",
             dirname, process_name, datetime, getpid());

    return prof_filename;
}

int load_profiler()
{
    if(!profiler_lib && load_profiler(&profiler_lib))
        return -1; // load was not succeed.

    return 0; // OK
}

} // namespace {}

void start_profiling(const char *process_name)
{
    if(load_profiler())
        return; // load was not succeed.

    const std::string file_name = mk_filename(process_name);
    ProfilerStart(file_name.c_str());
    ProfilerRegisterThread();

    LogTrace(TRACE1) << "Profiler started. file_name: " << file_name;
}

void stop_profiling()
{
    if(profiler_lib) {
        ProfilerStop();
        unload_profiler(&profiler_lib);
        LogTrace(TRACE1) << "Profiler stoped.";
    }
}

void startstop_profiling()
{
    if(profiler_lib == 0)
        start_profiling();
    else
        stop_profiling();
}

bool isProfilingCmd(const char* buff, size_t buff_sz)
{
    if(buff_sz > 10 && !strncmp(buff, "Profiling:", 10)) {
        return true;
    } else {
        return false;
    }
}

struct ProfilingCmdParams {
    std::string process_name;
    std::string cpu_frequency;
    std::string request_text;

    // if set to any value (including 0 or the empty string), use ITIMER_REAL instead of ITIMER_PROF to gather profiles. In general,
    // ITIMER_REAL is not as accurate as ITIMER_PROF, and also interacts badly with use of alarm(),
    // so prefer ITIMER_PROF unless you have a reason prefer ITIMER_REAL.
    std::string realtime;

    enum cmd_kind_t {
        cmd_enable,
        cmd_disable,
        cmd_status,
        cmd_flush
    };
    cmd_kind_t cmd_kind;

    static std::string getValue(const char * key, const std::string &cmd, const char *_default) {
        std::string::size_type pos1 = cmd.find(key);
        std::string::size_type pos2 = std::string::npos;
        if(pos1 != std::string::npos) {
            pos2 = cmd.find(' ', pos1);
            return cmd.substr(pos1 + strlen(key), pos2 == std::string::npos ? pos2 : pos2 - pos1 - strlen(key));
        } else {
            return _default;
        }
    }

    static boost::optional<ProfilingCmdParams> parse(const std::string &cmd) {

        ProfilingCmdParams params;
        
        const std::string cmd_kind_str = getValue("cmd=",  cmd, "");
        if(cmd_kind_str == "enable") {
            params.cmd_kind = cmd_enable;
        } else if(cmd_kind_str == "disable") {
            params.cmd_kind = cmd_disable;
        } else if(cmd_kind_str == "status") {
            params.cmd_kind = cmd_status;
        } else if(cmd_kind_str == "flush") {
            params.cmd_kind = cmd_flush;
        } else {
            return boost::optional<ProfilingCmdParams>();
        }

        params.process_name = getValue("process_name=",  cmd, "proc");

        if(params.cmd_kind == cmd_enable) {
            params.cpu_frequency= getValue("cpu_frequency=", cmd, "100");
            params.request_text = getValue("request_text=",  cmd, "");
            params.realtime     = getValue("realtime=", cmd, "");
        }
        return boost::optional<ProfilingCmdParams>(params);
    }
};

void handle_profiling_cmd_status(const ProfilingCmdParams &cmd_params, const ProfilerState &state)
{
    char status_str[2048];
    if(state.enabled) {
        time_t total_time = time(0) - state.start_time;
        if(!state.start_time) {
            total_time = 0;
        }
        snprintf(status_str, sizeof(status_str),
                 "Профилирование запущено на процессе %s(pid:%u);\n"
                 "Имя файла: %.100s;\n"
                 "Успели начпокать сэмплов: %d за %02ldм:%02ldс\n",
                 cmd_params.process_name.c_str(), getpid(),
                 state.profile_name,
                 state.samples_gathered, total_time / 60, total_time % 60);
    } else {
        snprintf(status_str, sizeof(status_str), 
                 "Профилирование выключено. %s(pid:%u)\n",
                 cmd_params.process_name.c_str(), getpid());
    }
    
    fprintf(stderr, "%s", status_str);
}

void handle_profiling_cmd_flush(const ProfilingCmdParams &cmd_params, const ProfilerState &state)
{
    if(state.enabled && ProfilerFlush) {
        ProfilerFlush();
    }
}

void handle_profiling_cmd_enable(const ProfilingCmdParams &cmd_params, const ProfilerState &state)
{
    if(state.enabled)
        return;

    setenv("CPUPROFILE_FREQUENCY", cmd_params.cpu_frequency.c_str(), 1);
    if(!cmd_params.realtime.empty())
        setenv("CPUPROFILE_REALTIME", cmd_params.realtime.c_str(), 1);
    start_profiling(cmd_params.process_name.c_str());
    if(!cmd_params.request_text.empty())
        // Enable должен будет подать код, который и будет следить за regex
        ProfilerDisable();
}

void handle_profiling_cmd_disable(const ProfilingCmdParams &cmd_params, const ProfilerState &state)
{
    unsetenv("CPUPROFILE_FREQUENCY");
    unsetenv("CPUPROFILE_REALTIME");

    stop_profiling();
}

void handle_profiling_cmd(const ProfilingCmdParams &cmd_params)
{
    if(load_profiler()) {
        return;
    }

    ProfilerState state = {};
    ProfilerGetCurrentState(&state);

    fprintf(stderr, "cmd: %d", cmd_params.cmd_kind);

    switch(cmd_params.cmd_kind) {
    case ProfilingCmdParams::cmd_enable:
        handle_profiling_cmd_enable(cmd_params, state);
        break;
    case ProfilingCmdParams::cmd_disable:
        handle_profiling_cmd_disable(cmd_params, state);
        break;
    case ProfilingCmdParams::cmd_status:
        handle_profiling_cmd_status(cmd_params, state);
        break;
    case ProfilingCmdParams::cmd_flush:
        handle_profiling_cmd_flush(cmd_params, state);
        break;
    }
}

void handle_profiling_cmd(const char* buff, size_t buff_sz)
{
    if(buff_sz <= 10)
        return;
    const std::string cmd(buff + 10, buff_sz);

    boost::optional<ProfilingCmdParams> cmdParams = ProfilingCmdParams::parse(cmd);

    if(cmdParams) {
        handle_profiling_cmd(*cmdParams);
    }
}


#ifdef XP_TESTING
#include "xp_test_utils.h"
#include "checkunit.h"

namespace {

START_TEST(profiler_parse_cmd)
{
    boost::optional<ProfilingCmdParams> pcp_ =
            ProfilingCmdParams::parse("Profiling: cmd=enable process_name=zhopa cpu_frequency=1099 request_text=1MOW.*");

    fail_unless(pcp_.is_initialized() == true);
    ProfilingCmdParams pcp = pcp_.get();
    fail_unless(pcp.cmd_kind == ProfilingCmdParams::cmd_enable);
    fail_unless(pcp.cpu_frequency == "1099");
    fail_unless(pcp.process_name == "zhopa");
    fail_unless(pcp.request_text == "1MOW.*");

    pcp_ = ProfilingCmdParams::parse("Profiling: cmd=disable");
    fail_unless(pcp_.is_initialized() == true);
    pcp = pcp_.get();
    fail_unless(pcp.cmd_kind == ProfilingCmdParams::cmd_disable);
}
END_TEST;

#define SUITENAME "Serverlib"
TCASEREGISTER(0, 0)
{
    ADD_TEST(profiler_parse_cmd);
}
TCASEFINISH;
} // namespace
#endif /* XP_TESTING */
