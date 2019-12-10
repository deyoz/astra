#include "timatic_exception.h"

#include <serverlib/helpcpp.h>
#include <serverlib/va_holder.h>

namespace Timatic {

Error::Error(BIGLOG_SIGNATURE)
    : ServerFramework::Exception("", false), nick_(nick), file_(file), line_(line), func_(func)
{
}

Error::Error(BIGLOG_SIGNATURE, const char *fmt, ...)
    : Error(BIGLOG_VARIABLE)
{
    VA_HOLDER(ap, fmt);
    msg_ = HelpCpp::vsprintf_string_ap(fmt, ap);
}

//-----------------------------------------------

SessionError::SessionError(BIGLOG_SIGNATURE, const char *fmt, ...)
    : Error(BIGLOG_VARIABLE)
{
    VA_HOLDER(ap, fmt);
    msg_ = HelpCpp::vsprintf_string_ap(fmt, ap);
}

//-----------------------------------------------

ValidateError::ValidateError(BIGLOG_SIGNATURE, const char *fmt, ...)
    : Error(BIGLOG_VARIABLE)
{
    VA_HOLDER(ap, fmt);
    msg_ = HelpCpp::vsprintf_string_ap(fmt, ap);
}

} // Timatic
