#ifndef _TEST_H
#define _TEST_H
#ifndef TEST
    #define TEST 1
#endif
#ifndef NICKNAME

#if TEST == 1

    #define tst() printf("%d %s\n",__LINE__,__FILE__)
    #define PUTS(x) puts((x))
    #define PRINTF printf
	#define FPRINTF fprintf
	#define FPUTS fputs
	#define FFLUSH(x) fflush((x))

#else

    #error no test
    #define tst(x)
    #define PUTS(x)
    #define PRINTF
	#define FPRINTF
	#define FPUTS
	#define fflush(x)

#endif
#ifdef PROHIBIT_UNDEFINED_NICKNAME
#error "NICKNAME undefined. DO NOT WRITE CODE IN HEADERS!"
#endif
#else


#include "logger.h" /* (C) Copyright Ilja Golstein forever*/

#define GLUE_TWO_ARGS(x,y) GLUE_TWO_ARGS2(x,y)
#define GLUE_TWO_ARGS2(x,y) x##y
#define BackTrace HelpCpp::RegStack GLUE_TWO_ARGS(reg_stack_object,__LINE__) (BIGLOG)

#define TRACE0  getRealTraceLev(0),STDLOG
#define TRACE1  getRealTraceLev(1),STDLOG
#define TRACE2  getRealTraceLev(12),STDLOG
#define TRACE3  getRealTraceLev(12),STDLOG
#define TRACE4  getRealTraceLev(12),STDLOG
#define TRACE5  getRealTraceLev(12),STDLOG
#define TRACE6  getRealTraceLev(12),STDLOG
#define TRACE7  1024,STDLOG


#define FuncIn(fn) const char * func_name = #fn; \
                   ProgTrace(TRACE2,"Enter %s", func_name)
#define FuncOut(fn) ProgTrace(TRACE2,"Leave " #fn)
#define RETURN(x) return ((ProgTrace(TRACE2,"Leave %s", func_name)),(x));
#define NVL(x,y) ((x)==NULL?(y):(x))
#define SNULL(x) NVL(x,"(null)")

#define tst() ProgTrace(TRACE3," ")
#define TST() ProgError(STDLOG," ")

#ifdef XP_TESTING
void allowProgError();
void denyProgError();
#endif
#ifdef __cplusplus

#include <string>

namespace OciCpp {
class DbmsOutput {
    bool enabled;
    void getLines(bool, int, STDLOG_SIGNATURE);
public:
    DbmsOutput();
    ~DbmsOutput();
    void enable();
    void disable();
    void print(int, STDLOG_SIGNATURE);
    void clear();
    std::string str() const;
};
}
#endif
#endif

/*
    Использование:

    ProgError(STDLOG,"text %s %d .." ,....) - Для ошибок

    WriteLog(STDLOG,"text %s %d .." ,....) - Вывод обязательной
    информации, не связанной с ошибками

    ProgTrace(TRACE3,"text %s %d .." ,....) - Для отладочной  информации

    TRACE1 - очень важная информация

    TRACE2 - вход/выход в "большую" функцию,
    используется  в FuncIn()/FuncOut()

    TRACE3 - вывод отладочной информации с параметрами
    т.е.  race=%s итд

    TRACE4 - метки типа "пришли сюда", используется
    tst();

    TRACE5 - используется функциями общего назначения (today_plus),
    для печати своих параметров, итд

*/

#endif
