#ifndef __LOGWRITER_H_
#define __LOGWRITER_H_

#include <string>

class LogWriter
{
  public:
    LogWriter(const std::string& appname);
    virtual ~LogWriter() {}
    virtual void write(int level, const char* head, const char* msg) = 0;
    virtual void writeFromSignalHandler(const int level, const char* nickname, const char* filename,
                int line, const char* msg, const int msg_size) = 0;
    virtual const char* logHeadFromSignalHandler(const int level, const char* nickname, const char* filename,
                int line, int* const headSize);
    virtual void reopen() = 0;
    virtual void flush() = 0;
    virtual void close() = 0;
protected:
    std::string appname_;
    pid_t pid_;
};

#endif // __LOGWRITER_H_
