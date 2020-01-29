#ifndef PROFILER_H
#define PROFILER_H

void startstop_profiling();
void start_profiling(const char *process_name = "proc");
void stop_profiling();


struct ProfilerState {
  int    enabled;             /* Is profiling currently enabled? */
  time_t start_time;          /* If enabled, when was profiling started? */
  char   profile_name[1024];  /* Name of profile file being written, or '\0' */
  int    samples_gathered;    /* Number of samples gatheered to far (or 0) */
};

/**
  * handle cmd from sirena tcl core.
  * can be looks like: 'Profiling: cmd=enable cpu_frequency=10000 process_name=obrzap_grp1_Txt request_text=1ŒŽ‚.*'
  * request_text parameter matters only for text handlers. For airimp it could be message text
  * how to use: from Monitor. 'Send cmd to process'->'type a message like above'
*/
void handle_profiling_cmd(const char* buff, size_t buff_sz);

bool isProfilingCmd(const char* buff, size_t buff_sz);

#endif // PROFILER_H
