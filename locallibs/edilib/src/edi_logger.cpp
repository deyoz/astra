#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include "edi_logger.h"
#define NICKNAME "ROMAN"
#define NICKTRACE ROMAN_TRACE
#include "edi_test.h"

char logbuff[2000];

static int stat_log_level=99;

EdiError_t EdiError = edi_prog_error;
WriteEdiLog_t WriteEdiLog = edi_prog_write;
EdiTrace_t EdiTrace = edi_prog_trace;

FILE *f_edi_log=NULL;

static const char*get_time_str()
{
    static const char *blank="";
    static char tmstr[100];
    struct tm *ptm;
    time_t now = time(NULL);

    if(now==((time_t) -1)){
        return blank;
    }

    ptm = localtime(&now);
    if(ptm==NULL){
        return blank;
    }

    strftime(tmstr, sizeof(tmstr)-1, "%d.%b.%y %T", ptm);

    return tmstr;
}

static void edi_log_write(int Level,
			  const char *nickname,
			  const char *filename,int line,
			  const char *format ,  va_list ap)
{
    int len1,len2;
    len1=snprintf(logbuff,sizeof(logbuff),"%s %02d %s%s:%s:%d ",
		  get_time_str(), Level, Level<0?">>>>>":(Level==0?"#####":""),
		  nickname, filename,line);

    if(len1<0||len1>sizeof(logbuff)){
        return;
    }

    len2=vsnprintf(logbuff+len1, sizeof(logbuff)-len1, format, ap);
    
    if(len2>sizeof(logbuff)-len1 || len2<0){
        len2=sizeof(logbuff)-2;
	logbuff[len2]='\0';
    }
    len2=len2+len1;

    while(logbuff[len2-1]=='\n'){
	logbuff[--len2]='\0';
    }
    logbuff[len2++]='\n';
    logbuff[len2]='\0';

    fwrite(logbuff, sizeof(char), len2, f_edi_log?f_edi_log:stdout);
}


void edi_prog_error(const char *nickname,
		    const char *filename,int line,
		    const char *format ,  ...)
{
    va_list ap;

    va_start(ap, format);

    edi_log_write(-1,nickname,filename,line,format,ap);

    va_end(ap);
}

void edi_prog_write(const char *nickname,
		    const char *filename,int line,
		    const char *format ,  ...)
{
    va_list ap;

    va_start(ap, format);

    edi_log_write(0,nickname,filename,line,format,ap);

    va_end(ap);
}

void edi_prog_trace(int Level,
		    const char *nickname,
		    const char *filename,int line,
		    const char *format ,  ...)
{
    va_list ap;

    if(stat_log_level<Level){
        return;
    }

    va_start(ap, format);

    edi_log_write(Level,nickname,filename,line,format,ap);

    va_end(ap);
}

int open_edi_log(const char *file)
{
    if(f_edi_log && f_edi_log!=stdout){
        fclose(f_edi_log);
    }
    f_edi_log = fopen(file, "a+");
    if(!f_edi_log){
	fprintf(stderr, "Failed to open %s:%s. Use default output",file,strerror(errno));
	f_edi_log=stdout;
        return -1;
    }

    return 0;
}

void close_edi_log(void)
{
    if(f_edi_log && f_edi_log!=stdout){
        fclose(f_edi_log);
    }
}

void set_edi_loglevel(int level)
{
    stat_log_level=level;
}

void InitEdiLogger(EdiError_t pfErr, WriteEdiLog_t pfWrLog, EdiTrace_t pfTrace)
{
    if(pfErr)
	EdiError = pfErr;
    if(pfWrLog)
	WriteEdiLog = pfWrLog;
    if(pfTrace)
	EdiTrace = pfTrace;
}
