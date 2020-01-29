//#ifdef __cplusplus
//extern "C" {
//#endif
#ifndef _LWRITER_H_
#define _LWRITER_H_

#include <stdarg.h>
#include <memory>

class LogWriter;

class CutLogHolder
{
public:
    static CutLogHolder& Instance();

    bool cutLog(int level){
        return curCutLog_ < level;
    }

    int logLevel()const {
        return curCutLog_;
    }

    void setLogLevel(int newLogLevel){
        curCutLog_ = newLogLevel;
    }

private:
    explicit CutLogHolder();

    //no copy
    CutLogHolder(const CutLogHolder&);
    CutLogHolder& operator=(const CutLogHolder&);

private:
    int curCutLog_;
};

class CutLogLevel
{
    const int restore_value;
  public:
    CutLogLevel()
        : restore_value(CutLogHolder::Instance().logLevel())
    {
    }
    explicit CutLogLevel(int new_cut_log_level)
        : restore_value(CutLogHolder::Instance().logLevel())
    {
        CutLogHolder::Instance().setLogLevel(new_cut_log_level);
    }
    void reset(const int ll) {
        CutLogHolder::Instance().setLogLevel(ll);
    }
    ~CutLogLevel()
    {
        CutLogHolder::Instance().setLogLevel(restore_value);
    }
};

typedef int(*CutLogFunc)(int);
void setCutLogger(CutLogFunc func);
int cutLog(int l);

enum LOGGER_SYSTEM
{
    LOGGER_SYSTEM_CONSOLE,
    LOGGER_SYSTEM_FILE,
    LOGGER_SYSTEM_LOGGER,
    LOGGER_SYSTEM_RSYSLOG
};
void setLoggingGroup(const char* var, LOGGER_SYSTEM type);
std::unique_ptr<LogWriter> makeLogger(LOGGER_SYSTEM type, const char* file, const char* logGrpName);
void setLogging(const char* file, LOGGER_SYSTEM type, const char* logGrpName);
void reopenWriter(LogWriter& writer);
void regLogReopen(int s);

const char *get_log_head(int log, bool add_cc_mark);

void write_log_message_str(int level, const char* nickname, const char* filename, int line,
        const char* msg);
void write_log_to_writer(LogWriter* const writer, int level, const char* head, const char* msg);
void write_log_message(int level, const char* nickname, const char* filename, int line,
        const char* format, va_list ap);
//The following function use internal static allocated buffers.
//Designed for write log from signal handler (signal handler execution not interrupted other signal)
void write_log_from_signal_handler(int level, const char* nickname, const char* filename,
                int line, const char* msg, const int msg_size);
void write_log_message_stderr(const char* nickname, const char* filename, int line,
        const char* format, ...);

void setLogReopen(void (*f)(void));
void reopenLog();
void closeLog();
void flushLog();

#endif
//#ifdef __cplusplus
//}
//#endif
