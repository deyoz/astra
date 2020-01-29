#ifndef _EDI_LOGGER_H_
#define _EDI_LOGGER_H_

#include <stdarg.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#define EDI_LOGFILE "log_edi.log" /**/

    typedef void (*EdiError_t)(const char *nickname,
			       const char *filename,int line,
			       const char *format ,  ...)
#if (__GNUC__ > 2) /*Супер компилятор 2.95 на Дебиан - НЕ ПОНИМАЕТ __attribute__ для нефункции*/
	__attribute__((format(printf,4,5)))
#endif
	;

    extern EdiError_t EdiError;

    typedef void (*WriteEdiLog_t)(const char *nickname,
				  const char *filename,int line,
				  const char *format ,  ...)
#if  (__GNUC__ > 2)
	__attribute__((format(printf,4,5)))
#endif
	;

    extern WriteEdiLog_t WriteEdiLog;

    typedef void (*EdiTrace_t)(int Level,const char *nickname,
			       const char *filename,int line,
			       const char *format ,  ...)
#if (__GNUC__ > 2)
	__attribute__((format(printf,5,6)))
#endif
	;

    extern EdiTrace_t EdiTrace;

    /******************************************************************/
    /* Задает функции логирования                                     */
    /* 1) Ошибки 2) Срочно в номер 3) Всякие                          */
    /******************************************************************/
    void InitEdiLogger(EdiError_t pfErr, WriteEdiLog_t pfWrLog, EdiTrace_t pfTrace);


    void edi_prog_error(const char *nickname,
			const char *filename,int line,
			const char *format ,  ...);

    void edi_prog_write(const char *nickname,
			const char *filename,int line,
			const char *format ,  ...);

    void edi_prog_trace(int Level,
			const char *nickname,
			const char *filename,int line,
			const char *format ,  ...);

    int open_edi_log(const char *file);

    void close_edi_log();

    void set_edi_loglevel(int level);
#ifdef __cplusplus
}
#endif
#endif
