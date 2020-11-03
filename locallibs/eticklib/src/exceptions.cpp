/*	2006 by Roman Kovalev 	*/
/*	roman@pike.dev.sirena2000.ru		*/
#include "etick/exceptions.h"
#include "etick/et_utils.h"
#include "etick/etick_localization.h"

#define NICKNAME "ROMAN"
#include <serverlib/slogger.h>

namespace Ticketing
{
namespace TickExceptions
{

tick_exception & tick_exception::operator << (const std::string &p)
{
    ErrTextParams.push_back(p);
    return *this;
}

tick_soft_except & tick_soft_except::operator << (const std::string &p)
{
    tick_exception::operator << (p);
    return *this;
}

tick_fatal_except & tick_fatal_except::operator << (const std::string &p)
{
    tick_exception::operator << (p);
    return *this;
}

tick_exception::tick_exception(const char *nickname, const char *file, int line,
        const ErrMsg_t & err, short fatal)
    : comtech::Exception(""), What(""), ErrCode(err)
{
    if(fatal){
        LogError(nickname, file, line) << what();
    }
}

tick_exception::tick_exception(const char *nickname, const char *file, int line,
        const ErrMsg_t & err, short fatal, const char *str)
    : comtech::Exception(str),What(str), ErrCode(err)
{
    if(fatal){
        LogError(nickname, file, line) << what();
    } else {
        LogWarning(nickname, file, line) << what();
    }
}

tick_exception::tick_exception(const char *nickname, const char *file, int line,
        const ErrMsg_t & err, short fatal, const std::string &str)
    : comtech::Exception(str),What(str), ErrCode(err)
{
    if(fatal){
        LogError(nickname, file, line) << what();
    } else {
        LogWarning(nickname, file, line) << what();
    }
}

std::string tick_exception::errText(Language l) const
{
    LogTrace(TRACE3) << "tick_exception::errText called!";
    LogTrace(TRACE3) << "ErrTextParams.size = " << ErrTextParams.size();
    EtickLocalization lclz (ErrCode, l);
    for(std::list<std::string>::const_iterator iter = ErrTextParams.begin();
       iter != ErrTextParams.end(); iter ++)
    {
        lclz << *iter;
    }
    return lclz.str();
}

tick_fatal_except::tick_fatal_except(const char *nickname, const char *file,
                                     int line, const ErrMsg_t &err,
                                     const char *format,...) throw ()
    :tick_exception(nickname,file,line,err,0)
{
    va_list ap;
    va_start(ap,format);

    try{
        setMessage(EtUtils::PrintApFormat(format, ap));
        ProgError(nickname, file, line, "%s", what());
    }
    catch(const std::bad_alloc& e){
        ProgError(STDLOG, "%s", e.what());
        setMessage(e.what());
        va_end(ap);
    }
    catch(...)
    {
        va_end(ap);
        throw;
    }

    va_end(ap);
}
// tick_fatal_except::tick_fatal_except(const char *nickname, const char *file,
//                                      int line, const char *format,...) throw ()
//     :tick_exception(nickname,file,line,EtErr::ProgErr,"",0)
// {
//     va_list ap;
//     va_start(ap,format);
//
//     try{
//         setMessage(EtUtils::PrintApFormat(format, ap));
//         ProgError(nickname, file, line, what());
//     }
//     catch(std::bad_alloc &e){
//         ProgError(STDLOG, e.what());
//         setMessage(e.what());
//         va_end(ap);
//     }
//     catch(...){
//         va_end(ap);
//         throw;
//     }
//
//     va_end(ap);
// }
//

tick_soft_except::tick_soft_except(const char *nickname, const char *file,
                                   int line, const ErrMsg_t & err, const char *format,...)throw()
    :tick_exception(nickname,file,line,err,0)
{
    va_list ap;
    va_start(ap,format);

    try{
        setMessage(EtUtils::PrintApFormat(format, ap));
        WriteLog(nickname, file, line, "%s:%s", err.c_str(), What.c_str());
    }
    catch(const std::bad_alloc& e){
        ProgError(STDLOG, "%s", e.what());
        setMessage(e.what());
        va_end(ap);
    }
    catch(...)
    {
        va_end(ap);
        throw;
    }

    va_end(ap);
}

tick_soft_except::tick_soft_except(const char *nickname, const char *file, int line,
                                   const ErrMsg_t &err) throw()
    :tick_exception(nickname, file, line, err, 0)
{
    WriteLog(nickname, file, line, "%s", err.c_str());
}

}// tick_exceptions
}//Ticketing
