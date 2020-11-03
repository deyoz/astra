#if HAVE_CONFIG_H
#endif

#include <syslog.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/uio.h>
#include <time.h>
#include <errno.h>
#include <sys/poll.h>
#include <sys/times.h>
#include <sys/time.h>
#include <assert.h>
#include <fstream>
#include <sstream>
#include <iostream>
#include <ctime>
#include <netdb.h>

#include "tclmon.h"
#include "tcl_utils.h"
#include "lwriter.h"
#include "guarantee_write.h"
#include "log_manager.h"
#include "monitor.h"
#include "monitor_ctl.h"
#include "log_queue.h"
#include "va_holder.h"
#include "dates.h"
#include "logwriter.h"
#include "ourtime.h"

#define NICKNAME "NONSTOP"
#include "test.h"

static const int YEAR_BASE = 1900;

void abortOnError();
int cutLogDefault(int);
static CutLogFunc cutLogFuncPtr = cutLogDefault;
void setCutLogger(CutLogFunc func)
{
    cutLogFuncPtr = func;
}

int cutLog(int l)
{
    return cutLogFuncPtr(l);
}
/*
static int detect_snprintf()
{
    static int flag = 0;
    if (!flag) {
        char b[10];
        int rc = snprintf(b, 2, "aaaa");
        if (rc < 0) {
            flag = -1;
        } else {
            flag = 1;
        }
    }
    return flag;

}
*/
static inline unsigned long yearSize(const int year)
{
    return (Dates::IsLeapYear(year) ? 366 : 365);
}

static const struct tm* gmtimeFromSignalHandler(const time_t* t, struct tm* const stm)
{
    static const unsigned long monthTab[2][12] = {
                { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 },
                { 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 }
    };
    static const int START_EPOCH_YEAR = 1970;
    static const unsigned long SECONDS_IN_DAY = 24L * 60L * 60L;
    unsigned long dayclock = static_cast<const unsigned long>(*t) % SECONDS_IN_DAY;
    unsigned long dayno = static_cast<const unsigned long>(*t) / SECONDS_IN_DAY;
    int year = START_EPOCH_YEAR;
    unsigned long tmp;

    stm->tm_sec = dayclock % 60;
    stm->tm_min = (dayclock % (60 * 60)) / 60;
    stm->tm_hour = dayclock / (60 * 60);
    stm->tm_wday = (dayno + 4) % 7; //day 0 was thursday
    while (dayno >= (tmp = yearSize(year))) {
        dayno -= tmp;
        ++year;
    }
    stm->tm_year = year - YEAR_BASE;
    stm->tm_yday = dayno;
    stm->tm_mon = 0;
    while (dayno >= (tmp = monthTab[Dates::IsLeapYear(year)][stm->tm_mon])) {
        dayno -= tmp;
        ++stm->tm_mon;
    }
    stm->tm_mday = dayno + 1;
    stm->tm_isdst = -1;

    return stm;
}

int cutLogDefault(int l)
{
    return CutLogHolder::Instance().cutLog(l);
}

static std::unique_ptr<char[]> construct_logmsg(const char *format, va_list ap)
{
    constexpr char const* diag = "internal error - BAD format/va_list combination - snprintf failed";
    // see `man 3 vsnprintf`, example section
    va_list aq;
    __va_copy(aq, ap);

    int size = vsnprintf(nullptr, 0, format, aq);
    va_end(aq);

    if(size < 0)
    {
        auto p = std::unique_ptr<char[]>(new char[strlen(diag)+1]);
        strcpy(p.get(), diag);
        return p;
    }

    auto p = std::unique_ptr<char[]>(new char[++size]);
    size = vsnprintf(p.get(), size, format, ap);
    if (size < 0)
        p.reset();
    return p;
}

CutLogHolder::CutLogHolder()
{
    const int ll = readIntFromTcl(current_group2() + "(LOG_LEVEL)", 20);
    const int cl = readIntFromTcl("CUTLOGGING", ll);

    curCutLog_ = std::min(cl, ll);
}

CutLogHolder& CutLogHolder::Instance()
{
    static CutLogHolder instance;

    return instance;
}


class LocalLogger
{
public:
    enum LogLevel {Error, Warning, Info, Debug};
};
static int syslogLevels[4] = {LOG_ERR, LOG_WARNING, LOG_INFO, LOG_DEBUG};

LogWriter::LogWriter(const std::string& appname)
        : appname_(appname), pid_(getpid())
    {}
const char* LogWriter::logHeadFromSignalHandler(const int level, const char* nickname, const char* filename,
                int line, int* const headSize)
{
        static const int BUF_SIZE = 512;
        static char buf[BUF_SIZE];
        struct timeval now;
        struct tm stm;

        (void)gettimeofday(&now, 0);
        (void)gmtimeFromSignalHandler(&now.tv_sec, &stm);
        *headSize = snprintf(
                buf,
                BUF_SIZE,
                "%.4i%02i%02iT%02i%02i%02i.%06lu+00 %02d %.200s %07d |TIME IN GMT| >>>>> %.16s:%.64s:%d ",
                stm.tm_year + YEAR_BASE,
                stm.tm_mon + 1,
                stm.tm_mday,
                stm.tm_hour,
                stm.tm_min,
                stm.tm_sec,
                now.tv_usec,
                level,
                get_log_head(-10,false),
                pid_,
                nickname,
                filename,
                line
        );
        if (*headSize < 0) {
            *headSize = 0;
            *buf = '\0';
        }

        return buf;
}

static std::unique_ptr<LogWriter> lw;

static void reopenLogWriter()
{
    if (lw) {
        lw->reopen();
    }
}
static void (*reopenLogFunc)(void) = reopenLogWriter;

static int reg_log_reopen = 0;
void regLogReopen(int s)
{
    reg_log_reopen = 1;
}

void reopenLog()
{
    if (getVariableStaticBool("REOPENLOGS", NULL, 0)
            || reg_log_reopen) {
        reopenLogFunc();
    }
    reg_log_reopen = 0;
}

void reopenWriter(LogWriter& writer)
{
    writer.reopen();
}

void setLogReopen(void (*f)(void))
{
    reopenLogFunc = f;
}

void flushLog()
{
    if (lw) {
        lw->flush();
    }
}

void closeLog()
{
    if (lw) {
        lw->close();
        lw.reset();
    }
}

class LogConsole
    : public LogWriter
{
public:
    LogConsole(const std::string& appname)
        : LogWriter(appname)
    {}
    virtual ~LogConsole() {}
    virtual void write(int level, const char* head, const char* msg) {
        std::cerr << appname_ << " " << get_log_head(level,false)
            << " " << head << msg << std::endl;
    }
    virtual void writeFromSignalHandler(const int level, const char* nickname, const char* filename,
                int line, const char* msg, const int msg_size) {
        int headSize;
        std::cerr << logHeadFromSignalHandler(level, nickname, filename, line, &headSize) << msg << std::endl;
    }
    virtual void reopen() {
    }
    virtual void flush() {
    }
    virtual void close() {
    }
};

static bool flushLogFile()
{
    bool flushlog = true;
    char const* ptr = getenv("NOFLUSH_LOG");
    if (ptr) {
        if (atoi(ptr) == 1) {
            flushlog = false;
        }
    }
    return flushlog;
}

class LogFile
    : public LogWriter
{
    int forge_log_head1 = 0;
public:
    LogFile(const std::string& appname, const std::string& filename)
        : LogWriter(appname), filename_(filename), flushLog_(flushLogFile())
    {
        open();
        if(auto e = getenv("CRISP_LOG_HEAD")) {
            if(strcmp(e,"1")==0 or strcmp(e,"YES")==0)
                forge_log_head1 = -11;
            if(strcmp(e,"2")==0 or strcmp(e,"NIL")==0)
                forge_log_head1 = -12;
        }
    }
    virtual ~LogFile() {}
    virtual void write(int level, const char* head2, const char* msg)
    {
        const bool had_cc = LogCensor::apply(const_cast<char*>(msg));

        const char* head1 = get_log_head(forge_log_head1 ? forge_log_head1 : level, had_cc);
        const size_t h1_len = strlen(head1);
        size_t h2_len = strlen(head2);
        const char* p1 = msg;
        const char* p2 = strchr(p1, '\n');

        for(unsigned i=0; true; i++)
        {
            const int part_len = (p2 ? p2 - p1 : strlen(p1));
            ofs_.rdbuf()->sputn(head1, h1_len);
            ofs_.rdbuf()->sputn(head2, h2_len);
            ofs_.rdbuf()->sputn(p1, part_len);
            ofs_.rdbuf()->sputc('\n');
            if (!p2 || !*p2) {
                break;
            }
            p1 = p2 + 1;
            p2 = strchr(p1, '\n');
            h2_len = 0;
        }
        if (flushLog_) {
            flush();
        }
    }
    virtual void writeFromSignalHandler(const int level, const char* nickname, const char* filename,
                int line, const char* msg, const int msg_size) {
        int headSize;
        const char* head = logHeadFromSignalHandler(level, nickname, filename, line, &headSize);
        ofs_.rdbuf()->sputn(head, headSize);
        ofs_.rdbuf()->sputn(msg, strlen(msg));
        ofs_.rdbuf()->sputc('\n');
        flush();
    }
    virtual void reopen() {
        close();
        open();
    }
    virtual void flush() {
        ofs_.flush();
    }
    virtual void close() {
        ofs_.close();
    }
private:
    void open() {
        ofs_.open(filename_.c_str(), std::ios_base::out | std::ios_base::app);
        if (!ofs_) {
            write_log_message_stderr(STDLOG, "open log file failed: %s", filename_.c_str());
            Abort(1);
        }
        ofs_.flush();
    }
    const std::string filename_;
    const bool flushLog_;
    std::ofstream ofs_;
};

static void write_iovec(struct iovec* v, size_t v_sz, int sock, int totallen)
{
    clock_t t1 = clock();
    const int sz = writev(sock, v, v_sz);
    clock_t t2 = clock();
    if (t1 > t2) {
        t2 -= t2 + CLOCKS_PER_SEC;
        t1 -= t2 + CLOCKS_PER_SEC;
    }
    t2 -= t1;
    t2 *= 10;
    t2 /= CLOCKS_PER_SEC;
    t2 *= 100;
    if (t2 >100) {
        write_log_message_stderr(STDLOG, "write wait %d msec , %d bytes", t2, sz);
    }
    if (sz != totallen) {
        write_log_message_stderr(STDLOG, "write %d to %d totalllen=%d: %s",
                sz, sock, totallen, strerror(errno));
        if ((writev(2/*stderr*/, v, v_sz)) != totallen) {
            write_log_message_stderr(STDLOG, "write %d to %d totalllen=%d: %s",
                    sz, 2, totallen, strerror(errno));
        }
    }
}

class LogSocket
    : public LogWriter
{
public:
    LogSocket(const std::string& appname, int sockfd)
        : LogWriter(appname), sockfd_(sockfd), saved_fd_(INIT_SOCK_VALUE)
    {}
    virtual ~LogSocket() {}
    virtual void reopen() {
        sockfd_ = checkSocket(sockfd_);
    }
    virtual void flush() {
    }
    virtual void close() {
        ::shutdown(sockfd_, SHUT_RDWR);
        ::close(sockfd_);
    }
    virtual int connectSocket() = 0;
    virtual void writeFromSignalHandler(const int level, const char* nickname, const char* filename,
                int line, const char* msg, const int msg_size) {
        struct pollfd p;

        p.fd = sockfd_;
        p.events = POLLOUT;
        if (poll(&p, 1, 0) < 0) {
            return;
        } else if ((p.revents & POLLHUP) || (p.revents & POLLERR) || (p.revents & POLLNVAL)) {
            return;
        } else if (p.revents & POLLOUT) {
            static const int VEC_COUNT = 3;
            struct iovec v[VEC_COUNT] = {};
            int logHeadSize;

            v[0].iov_base = const_cast<char*>(logHeadFromSignalHandler(level, nickname, filename, line, &logHeadSize));
            v[0].iov_len = logHeadSize;
            v[1].iov_base = const_cast<char*>(msg);
            v[1].iov_len = msg_size;
            v[2].iov_base = const_cast<char*>("\n");
            v[2].iov_len = 1;
            writev(sockfd_, v, VEC_COUNT);
        }
    }
    void writeMsg(int level, const char* head1, const char* head2, const char* msg) {
        static bool insideLogWrite = false;
        if (insideLogWrite) {
            LogConsole(appname_).write(-1, head2, msg);
            return;
        }
        insideLogWrite = true;
        const int STEP = 5;
        struct iovec v[4 * STEP] = {};
        const int h1_len = strlen(head1);
        int h2_len = strlen(head2);
        sockfd_ = checkSocket(sockfd_);
        int totallen = 0;
        size_t i = 0;
        size_t step_num = 0;
        const char* p1 = msg;
        const char* p2 = strchr(p1, '\n');
        while (1) {
            static const char errorPattern[] = ">>>>>";

            const int part_len = (p2 ? p2 - p1 : strlen(p1));
            if (-1 == level) {
                const char* err = static_cast<const char*>(memchr(head2, '>', h2_len));
                if (err
                        && (err < (head2 + h2_len - (sizeof(errorPattern) - 2)))
                        && !strncmp(err, errorPattern, sizeof(errorPattern) - 1)) {
                    static std::string errorBuf;

                    makeErrorMessage(head1, h1_len, head2, h2_len, p1, part_len, errorBuf);
                    monitorControl::is_errors_control(errorBuf.c_str(), errorBuf.size());
                }
            }
            step_num = i % STEP;
            v[step_num * 4 + 0].iov_base = const_cast<char*>(head1);
            v[step_num * 4 + 0].iov_len = h1_len;
            v[step_num * 4 + 1].iov_base = const_cast<char*>(head2);
            v[step_num * 4 + 1].iov_len = h2_len;
            v[step_num * 4 + 2].iov_base = const_cast<char*>(p1);
            v[step_num * 4 + 2].iov_len = part_len;
            v[step_num * 4 + 3].iov_base = const_cast<char*>("\n");
            v[step_num * 4 + 3].iov_len = 1;
            totallen += (h1_len + h2_len + part_len + 1);

            if (step_num == STEP - 1) {
                write_iovec(v, STEP * 4, sockfd_, totallen);
                totallen = 0;
            }
            ++i;
            h2_len = 0;
            if (!p2 || !*p2) {
                break;
            }
            p1 = p2 + 1;
            p2 = strchr(p1, '\n');
        }
        step_num = i % STEP;
        if (step_num) {
            write_iovec(v, step_num * 4, sockfd_, totallen);
        }
        insideLogWrite = false;
    }
protected:
    static const int INIT_SOCK_VALUE = -1000;

    virtual void makeErrorMessage(const char* head1, const int h1_len, const char* head2, const int h2_len,
                                  const char* msg, const int msg_len, std::string& buf) = 0;

    int checkSocket(int s) {
        struct pollfd P;
        int n;
        if (INIT_SOCK_VALUE == saved_fd_) {
            saved_fd_ = s;
        }
        if (s != saved_fd_) {
            write_log_message_stderr(STDLOG, "logger descriptor changed from %d to %d", saved_fd_, s);
        }
        P.fd = s;
        P.events = POLLOUT;
        while (1) {
            n = poll(&P, 1, 500);
            if (n == 0) {
                write_log_message_stderr(STDLOG, "logger timeout");
                break;
            } else if (n < 0) {
                write_log_message_stderr(STDLOG, "poll failed:%s", strerror(errno));
                continue;
            }
            int need_reconnect = 0;
            if (P.revents & POLLHUP) {
                write_log_message_stderr(STDLOG, "POLLHUP on logger");
                need_reconnect = 1;
            }
            if (P.revents & POLLERR) {
                write_log_message_stderr(STDLOG, "POLLERR on logger");
                need_reconnect = 1;
            }
            if ((P.revents & POLLOUT) == 0) {
                write_log_message_stderr(STDLOG, "no POLLOUT on logger");
                need_reconnect = 1;
            }
            if (need_reconnect) {
                close();
                s = P.fd = reconnectSocket(5);
                saved_fd_ = s;
            }
            break;
        }
        if (s < 0) {
            write_log_message_stderr(STDLOG, "failed to reconnect to logger process after failure");
            Abort(5);
        }
        return s;
    }
    int reconnectSocket(int count) {
        int s = connectSocket();
        if (s < 0) {
            write_log_message_stderr(STDLOG, "reconnectSocket failed.");
            Abort(1);
        }
        return s;
    }
    int sockfd_;
    int saved_fd_;
};

static int initSocket(const std::string& filename)
{
    static unsigned tryCount;
    static struct sockaddr_un addr;

    int s = socket(addr.sun_family = AF_UNIX, SOCK_STREAM, 0);
    if (s < 0) {
        write_log_message_stderr(STDLOG, "socket failed:%s", strerror(errno));
        Abort(1);
    }
    if (filename.size() >= sizeof addr.sun_path) {
        write_log_message_stderr(STDLOG, "too long file name %.120s", filename.c_str());
        Abort(1);
    }
    strcpy(addr.sun_path, filename.c_str());

    tryCount = 5 + 1;
    while (--tryCount && (connect(s, (struct sockaddr*)&addr, sizeof addr) < 0)) {
        write_log_message_stderr(STDLOG, "%s: attempt connect to <%s> failed: (%d) - %s.", __FUNCTION__,
                                         filename.c_str(), errno, strerror(errno));
        write_log_message_stderr(STDLOG, "%d attempt(s) left.", tryCount - 1);

        random_sleep();
    }

    if (!tryCount) {
        write_log_message_stderr(STDLOG, "cannot connect to logger socket <%s>: %s",
                filename.c_str(), strerror(errno));
        close(s);
        Abort(1);
    }

    return s;
}
static int initSocket(const std::string& host, const std::string& port)
{
    int s = socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0) {
        write_log_message_stderr(STDLOG, "socket failed:%s", strerror(errno));
        Abort(1);
    }
    const hostent* server = gethostbyname(host.c_str());
    if (server == NULL) {
        write_log_message_stderr(STDLOG, "gethostbyname failed: %.120s", host.c_str());
        Abort(1);
    }
    sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_addr = *((struct in_addr *)server->h_addr);
    addr.sin_port = htons(atoi(port.c_str()));
    if (connect(s, (sockaddr*)&addr, sizeof(sockaddr)) < 0) {
        write_log_message_stderr(STDLOG, "cannot connect to logger socket <%s:%s>: %s",
                host.c_str(), port.c_str(), strerror(errno));
        close(s);
        Abort(1);
    }
    return s;
}
class LogLogger
    : public LogSocket
{
public:
    LogLogger(const std::string& appname, const std::string& filename)
        : LogSocket(appname, initSocket(filename)), filename_(filename)
    {}
    virtual ~LogLogger() {}
    virtual void write(int level, const char* head, const char* msg) {
        const bool had_cc = LogCensor::apply(const_cast<char*>(msg));
        writeMsg(level, get_log_head(level,had_cc), head, msg);
    }
    virtual int connectSocket() {
        return initSocket(filename_);
    }
private:
    virtual void makeErrorMessage(const char* head1, const int h1_len, const char* head2, const int h2_len,
                                  const char* msg, const int msg_len, std::string& buf)
    {
        buf.clear();
        buf.assign(head1, head1 + h1_len);
        buf.append(head2, h2_len);
        buf.append(msg, msg_len);
    }

private:
    const std::string filename_;
};

class LogRsyslog
    : public LogSocket
{
public:
    LogRsyslog(const std::string& appname, const std::string& host, const std::string& port)
        : LogSocket(appname, initSocket(host, port)), host_(host), port_(port)
    {
        time_t now;
        (void)std::time(&now);
        //man 2 strftime(...): %z The +hhmm or -hhmm numeric timezone (that is, the hour and minute offset from UTC).
        //Change to +hh:mm or -hh:mm, RFC 3339 and RFC 5424
        size_t len = strftime(timeOffsetBuf_, TIME_OFFSET_BUF_SIZE, "%z", std::localtime(&now));
        timeOffsetBuf_[len] = timeOffsetBuf_[len - 1];
        timeOffsetBuf_[len - 1] = timeOffsetBuf_[len - 2];
        timeOffsetBuf_[len - 2] = ':';
        timeOffsetBuf_[len + 1] = '\0';
    }
    virtual ~LogRsyslog() {}
    virtual int connectSocket() {
        return initSocket(host_, port_);
    }
    virtual const char* logHeadFromSignalHandler(const int level, const char* nickname, const char* filename,
                int line, int* const headSize) {
        static const int BUF_SIZE = 1024;
        static char buf[BUF_SIZE];
        struct timeval now;
        struct tm stm;

        (void)gettimeofday(&now, 0);
        (void)gmtimeFromSignalHandler(&now.tv_sec, &stm);
        *headSize = snprintf(
                buf,
                BUF_SIZE, "<%d>1 %.4i-%02i-%02iT%02i:%02i:%02i.%06lu+00:00  %.200s %d  %s |TIME IN GMT| >>>>> %s:%s:%d ",
                LOG_MAKEPRI(LOG_USER, syslogLevels[(level < 0) ?  LocalLogger::Error
                        : level == 0 ? LocalLogger::Warning
                        : level == 1 ? LocalLogger::Info
                        : LocalLogger::Debug]),
                stm.tm_year + YEAR_BASE,
                stm.tm_mon + 1,
                stm.tm_mday,
                stm.tm_hour,
                stm.tm_min,
                stm.tm_sec,
                now.tv_usec,
                appname_.c_str(),
                pid_,
                get_log_head(-10,false),
                nickname,
                filename,
                line);
        if (*headSize < 0) {
            *headSize = 0;
            *buf = '\0';
        }

        return buf;
    }
    virtual void write(int level, const char* head, const char* msg) {
        const LocalLogger::LogLevel ll(level < 0 ? LocalLogger::Error
                : level == 0 ? LocalLogger::Warning
                : level == 1 ? LocalLogger::Info
                : LocalLogger::Debug);

        static const int LOG_HEAD_BUF_SIZE = 1024;
        char buf[LOG_HEAD_BUF_SIZE] = {};
        const bool had_cc = LogCensor::apply(const_cast<char*>(msg));
        /*auto len = */syslogHead(buf, sizeof(buf), ll, had_cc);
        writeMsg(level, buf, head, msg);
    }
private:
    size_t syslogHead(char* buf, size_t len, LocalLogger::LogLevel ll, bool add_cc_mark) const {
        static const int DATE_TIME_BUF_SIZE = 128;
        char dateTimeBuf[DATE_TIME_BUF_SIZE] = {};
        struct timeval now;

        (void)gettimeofday(&now, 0);
        //Дата в формате RFC 3339 с ограничениями RFC 5424 (see TIMESTAMP).
        std::strftime(dateTimeBuf, sizeof(dateTimeBuf), "%Y-%m-%dT%H:%M:%S", std::localtime(&now.tv_sec));
        return snprintf(buf, len, "<%d>1 %s.%06lu%s  %.200s %d  %s ",
                LOG_MAKEPRI(LOG_USER, syslogLevels[ll]),
                dateTimeBuf,
                now.tv_usec,
                timeOffsetBuf_,
                appname_.c_str(),
                pid_,
                get_log_head(-10,add_cc_mark)
        );
    }

    virtual void makeErrorMessage(const char* head1, const int h1_len, const char* head2, const int h2_len,
                                  const char* msg, const int msg_len, std::string& buf)
    {
        const char* space = static_cast<const char*>(memchr(head1, ' ', h1_len));

        buf.clear();
        buf.assign(space ? space + 1 : head1, head1 + h1_len);
        buf.append(head2, h2_len);
        buf.append(msg, msg_len);
    }

private:
    static const int TIME_OFFSET_BUF_SIZE = 16;
    const std::string host_, port_;
    char timeOffsetBuf_[TIME_OFFSET_BUF_SIZE];
};

const char *get_log_head(int l, bool cc)
{
    return user_log_head(l,cc);
}

static std::string get_log_head2(int level, const char* nickname, const char* filename, int line)
{
    char head[256] = {};
    snprintf(head, sizeof(head), "%s%s:%s:%d ",
            ((-1 == level) ? ">>>>>" : (level <= 0) ? "#####" : ""),
            nickname, filename, line);
    return head;
}

void write_log_to_writer(LogWriter* const writer, int level, const char* head, const char* msg)
{
    if ( !LogManager::Instance().isWriteLog(level) ) {
        return;
    }
    if (!writer) {
        write_log_message_stderr(STDLOG, "log writer not found: %s%s", head, msg);
        return;
    }
    writer->write(level, head, msg);
}

void write_log_message_str(int level, const char* nickname, const char* filename, int line,
                const char* msg)
{
    write_log_to_writer(lw.get(), level, get_log_head2(level, nickname, filename, line).c_str(), msg);

#ifdef XP_TESTING
    if (level < 0 ) {
        abortOnError();
    }
#endif // XP_TESTING
}

//See comment for this function in header file
void write_log_from_signal_handler(int level, const char* nickname, const char* filename,
                int line, const char* msg, const int msg_size)
{
    if (!lw || !msg_size) {
        fprintf(stderr, "%s\n", msg);
        return;
    }

    lw->writeFromSignalHandler(level, nickname, filename, line, msg, msg_size);
}

void write_log_message_stderr(const char* nickname, const char* filename, int line,
        const char* format, ...)
{
    VA_HOLDER(ap, format)
    auto msg = construct_logmsg(format, ap);
    LogConsole(tclmonCurrentProcessName()).write(1, get_log_head2(1, nickname, filename, line).c_str(), msg.get());
}

void write_log_message(int level, const char* nickname, const char* filename, int line,
        const char* format, va_list ap)
{
    auto msg = construct_logmsg(format, ap);
    write_log_message_str(level, nickname, filename, line, msg.get());
}

std::unique_ptr<LogWriter> makeLogger(LOGGER_SYSTEM type, const char* file, const char* logGrpName)
{
    switch (type) {
        case LOGGER_SYSTEM_CONSOLE:
            return std::make_unique<LogConsole>(logGrpName);

        case LOGGER_SYSTEM_FILE:
            return std::make_unique<LogFile>(logGrpName, file);

        case LOGGER_SYSTEM_LOGGER:
            return std::make_unique<LogLogger>(logGrpName, file);

        case LOGGER_SYSTEM_RSYSLOG:
            if(auto pos = strchr(file, ':')) {
                const size_t sz = pos - file;
                return std::make_unique<LogRsyslog>(logGrpName, std::string(file, sz).c_str(), pos + 1);
            } else {
                write_log_message_stderr(STDLOG, "wrong logger socket: %s", file);
                Abort(1);
                break;
            }
        default:
            write_log_message_stderr(STDLOG, "wrong LOGGER_SYSTEM: %d", type);
            Abort(1);
            break;
    }
    return std::unique_ptr<LogWriter>();
}

void setLogging(const char* file, LOGGER_SYSTEM type, const char* logGrpName)
{
    lw = makeLogger(type, file, logGrpName);
}

void setLoggingGroup(const char* var, LOGGER_SYSTEM type)
{
    if ( (LOGGER_SYSTEM_LOGGER == type) && (readIntFromTcl("USE_RSYSLOG", 0) != 0) ) {
        std::string addr(readStringFromTcl("RSYSLOG_HOST"));

        addr.append(1, ':');
        addr.append(readStringFromTcl("RSYSLOG_PORT"));
        setLogging(addr.c_str(), LOGGER_SYSTEM_RSYSLOG, var);

        return;
    }

    const char* var2 = ( (type == LOGGER_SYSTEM_LOGGER) ? "SOCKET" : NULL);
    Tcl_Obj* o = Tcl_GetVar2Ex(getTclInterpretator(), var, var2, TCL_GLOBAL_ONLY);

    if (!o) {
        write_log_message_stderr(STDLOG, "cannot find %s(%s)", var, var2);
        Abort(1);
    }
    setLogging(Tcl_GetString(o), type, var);
}

