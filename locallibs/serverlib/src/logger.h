#ifndef _LOGGER_H_
#define _LOGGER_H_

#include "trace_signature.h"

#ifdef __cplusplus
extern "C" {
#endif

enum TRACE_CODES
{
 DEFAULT_TRACE=0,
 AIRIMP_SYSTEM_TRACE,
 AKSANDR_TRACE,
 ANDREW_TRACE,
 ANN_TRACE,
 ASM_TRACE,
 BAV_TRACE,
 CHEM_TRACE,
 DIMA_EMEL_TRACE,
 DJEK_ILYA_TRACE,
 DJEK_TRACE,
 DSB_TRACE,
 GRIG_TRACE,
 ILEJN_TRACE,
 ILYA_TRACE,
 IVAN_TRACE,
 JANE_TRACE,
 KAG_TRACE,
 KHONOV_TRACE,
 KONST_TRACE,
 KSE_TRACE,
 LANN_TRACE,
 MNICK_TRACE,
 NONSTOP_TRACE,
 OLEG_GRIG_TRACE,
 OLEG_TRACE,
 PNRNGT_TRACE,
 ROMAN_TRACE,
 SHURIK_TRACE,
 SYSTEM_TRACE,
 TATA_TRACE,
 WAITLIST_TRACE,
 YURAN_TRACE,
 ANTON_TRACE,
 DMITRYVM_TRACE,
 MAXIM_TRACE,
 GARICK_TRACE,
 HOLYBRAKE_TRACE,
 ROGER_TRACE,
 DMG_TRACE,
 ASH_TRACE,
 FOREVER_TRACE,
 DAG_TRACE,
 ALEROM_TRACE,
 VLAD_TRACE,
 FELIX_TRACE
};

int getRealTraceLev(int level);

int getTraceLev(int level, const char *nickname,const char *filename, int line);
// Returns loglevel for TRACE : getTraceLev(TRACE5) returns 12
int getTraceLevByNum(int trace_num);
// Returns loglevel for 5 : getTraceLevByNum(5)=getTraceLev(TRACE5) returns 12

void ProgError(const char *nickname, const char *filename, int line, const char *format,  ...)
#ifdef __GNUC__
__attribute__((format(printf, 4, 5)))
#endif
;


void WriteLog(const char *nickname, const char *filename, int line, const char *format,  ...)
#ifdef __GNUC__
__attribute__((format(printf, 4, 5)))
#endif
;

void ProgTrace(int Level, const char *nickname, const char *filename, int line, const char *format,  ...)
#ifdef __GNUC__
__attribute__((format(printf,5,6)))
#endif
;

void ProgTraceForce(int Level, const char *nickname, const char *filename, int line, const char *format,  ...)
#ifdef __GNUC__
__attribute__((format(printf,5,6)))
#endif
;

int logger_get_logall();
#ifndef ASTRA

#endif

int logLevelExternal();
void setLogLevelExternal(int l);

void ClearI(void);

void ReLog(int s);
void ReLogArch(void);

int was_prog_error();
void reset_prog_error();


#define STDLOG NICKNAME,__FILE__,__LINE__
#define STDLOG_VARIABLE nick, file, line

#define TRACE_VARIABLE level, STDLOG_VARIABLE

#ifndef __GNUC__
#define BIGLOG STDLOG,""
#else
#define BIGLOG STDLOG,__FUNCTION__
#define BIGLOG_SIGNATURE STDLOG_SIGNATURE, const char* func 
#define BIGLOG_VARIABLE STDLOG_VARIABLE, func
#endif
#ifdef __cplusplus

}
#ifndef __STATIST__

#include "simplelog.h"
namespace Logger 
{
 class ServerlibTracer : public Tracer 
 {
   void ProgError(const char *nickname, const char *filename, int line, const char *format ,  ...);
   void ProgTrace(int l,const char *nickname, const char *filename, int line, const char *format,  ...);
   void WriteLog(const char *nickname, const char *filename, int line, const char *format,  ...);
 };
}

namespace ServerFramework
{
class LogLevel
{
/*  Использование:
 *  {
 *    LogLevel a(<Новый уровень логов>);
 *    <Блок с измененным уровнем логов>
 *  }
 *
 *  Другой способ использования:
 *  LogLevel lvl;
 *  if (условие)
 *     lvl.setLevel(new_lvl)
 */
public:
    LogLevel()
    {
    }
    void setLevel(int lvl);

    LogLevel(int l)
    {
        setLogLevelExternal(l);
    }
    ~LogLevel()
    {
        setLogLevelExternal(-1);
    }
};
}

#endif /* __STATIST__ */

#endif /* __cplusplus */
#endif /* _LOGGER_H_ */
