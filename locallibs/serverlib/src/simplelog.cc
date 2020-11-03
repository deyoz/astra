#if HAVE_CONFIG_H
#endif

#include <stdarg.h>
#include <stdio.h>
#include <unistd.h>
#include "simplelog.h"
namespace Logger {
    void Tracer::ProgTrace(int Level,const char *nickname, 
        const char *filename,int line,
        const char *format ,  ...)
    {
        va_list ap;
        va_start(ap,format);
        fprintf(stderr,"[%d]: %s %s:%d: ",getpid(),nickname,filename,line);
        vfprintf(stderr,format,ap);
        fputc('\n',stderr);
        va_end(ap);
    }
    void Tracer::ProgError(const char *nickname, 
        const char *filename,int line,
        const char *format ,  ...) 
    {
        va_list ap;
        va_start(ap,format);
        fprintf(stderr,"[%d]:>>>>> %s %s:%d: ",getpid(),nickname,filename,line);
        vfprintf(stderr,format,ap);
        fputc('\n',stderr);
        va_end(ap);
    }
    void Tracer::WriteLog(const char *nickname, 
        const char *filename,int line,
        const char *format ,  ...) 
    {
        va_list ap;
        va_start(ap,format);
        fprintf(stderr,"[%d]:%s %s:%d: ",getpid(),nickname,filename,line);
        vfprintf(stderr,format,ap);
        fputc('\n',stderr);
        va_end(ap);
    }
    int Tracer::getLogLevel()
    {
        return 1000;
    }
    Tracer & getTracer(Tracer *t)
    {
        static Tracer *tracer;
        if(t){
            delete tracer;
            tracer=t;
        }
        if(!tracer)
            tracer=new Tracer();
        return *tracer;
    }
    void setTracer( Tracer *t)
    {
        getTracer(t);
    }

}

