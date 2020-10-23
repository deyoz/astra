#include "base_callbacks.h"

#define NICKNAME "VLAD"
#include "serverlib/slogger.h"

void CallbacksExceptionFilter(STDLOG_SIGNATURE)
{
    try {
        throw;
    } catch(const std::exception &E) {
        LogError(STDLOG_VARIABLE) << __FUNCTION__ << ": something wrong: " << E.what();
    } catch(...) {
        LogError(STDLOG_VARIABLE) << __FUNCTION__ << ": something wrong";
    }
}

void CallbacksExceptionFilter(TRACE_SIGNATURE)
{
    CallbacksExceptionFilter(STDLOG_VARIABLE);
}
