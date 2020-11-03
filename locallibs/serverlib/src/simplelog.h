#ifndef _SIMPLELOG_H_
#define _SIMPLELOG_H_
namespace Logger 
{
    class Tracer
    {
        public:
        virtual ~Tracer() {};
        virtual void ProgTrace(int Level, const char *nickname, 
        const char *filename, int line,
        const char *format,  ...)
#if 0
#ifdef __GNUC__
    __attribute__((format(printf,5,6)))
#endif
#endif
    ;
    virtual void ProgError(const char *nickname, 
        const char *filename, int line,
        const char *format,  ...)
#if 0
#ifdef __GNUC__
    __attribute__((format(printf, 4, 5)))
#endif
#endif
    ;
    virtual void WriteLog(const char *nickname, 
        const char *filename,int line,
        const char *format ,  ...)
#if 0
#ifdef __GNUC__
    __attribute__((format(printf,4,5)))
#endif
#endif
    ;
    virtual int getLogLevel();
    };
    Tracer & getTracer(Tracer *t = 0);
    void setTracer( Tracer *t);

}

#endif
