#ifndef _SERVERLIB_EXCEPTION_H_
#define _SERVERLIB_EXCEPTION_H_ 

#include <unistd.h>
#include <cstdlib>
#include <stdexcept>
#include <string>

#include "logger.h"
#include "tcl_utils.h"
#include "lngv.h"
#include "encstring.h"

namespace ServerFramework
{
struct NickFileLine
{
    const char* nick;
    const char* file;
    int line;
};

class Exception
    : public std::runtime_error
{
public:
    Exception(const std::string& msg, bool backtraceOff = false);
    Exception(const char* nick, const char* file, int line, const char* func, const std::string& msg, bool backtraceOff = false);
    
    virtual ~Exception() throw();
    
    void error(const char* nick, const char* file, int line) const;
    void print(int logLevel, const char* nick, const char* file, int line) const;
    const char* place() const;
    const char* stack() const;
    const char* backtrace() const;
private:
    std::string _place; 
    std::string _backtrace;
    std::string _stack;
};

struct IUserMsgException {
    virtual ~IUserMsgException() {}
    virtual std::string usermsg_what() const = 0; // description for developers
    virtual std::string usermsg_token() const = 0; // short msg id string, e.g. "ERR_PROG"
    virtual EncString usermsg_text(Language lang) const = 0; // user-frienly description
};

class AssertionFailed: public Exception
{
public:
    AssertionFailed(const char* nick, const char* file, long int line, const std::string& expr);
    virtual ~AssertionFailed() throw() {}
};

class xml_exception : public Exception
{
  public:
  xml_exception (const std::string &nick_, const std::string &fl_, int ln_, const std::string &msg);
  xml_exception (const std::string &msg);
};

class xml_custom_exception : public xml_exception
{
  public:
  xml_custom_exception (const std::string &nick_, const std::string &fl_, int ln_)
    :xml_exception(nick_,fl_,ln_,std::string())
  {
  }
};
}
namespace comtech = ServerFramework;

#define SIRENA_ABORT_OPT(subsystem) {\
    if (getVariableStaticBool(("SIRENA_ABORT(" + std::string(subsystem) + ")").c_str(), 0, 0)) { \
        ProgError("SYSTEM", __FILE__, __LINE__, "ASSERTION FAILED 'SIRENA_ABORT_OPT'"); \
        sleep(10); \
        abort(); \
    } \
}
namespace ServerFramework{
void  assertionFailed0Impl(char const* const nick, const char* const file, long int line, 
                                    const char* const what);

void assertionFailed1Impl(const char* const subsystemName, char const* const nick,
                                    const char* const file, long int line, const char* const what);
} // ServerFramework

// usage:
// ASSERT(expr);
// or
// ASSERT(expr, "subsystem_name");

#define _ASSERT_0(expr) ((expr) \
        ? ((void)(0)) \
        : comtech::assertionFailed0Impl(NICKNAME, __FILE__, __LINE__, #expr));

#define _ASSERT_1(expr, subsystemName) ((expr) \
        ? ((void)(0)) \
        : comtech::assertionFailed1Impl(subsystemName, NICKNAME, __FILE__, __LINE__, #expr));

#define _GET_3RD_ARG(arg1, arg2, arg3, ...) arg3
#define _ASSERT_MACRO_CHOOSER(...) \
        _GET_3RD_ARG(__VA_ARGS__, _ASSERT_1, _ASSERT_0, )

#define ASSERT(...) _ASSERT_MACRO_CHOOSER (__VA_ARGS__)(__VA_ARGS__)

#define HEADER_ASSERT(expr) {\
    if (!(expr)) \
        comtech::assertionFailed0Impl("ASSERT", __FILE__, __LINE__, #expr); }

#endif //_SERVERLIB_EXCEPTION_H_
