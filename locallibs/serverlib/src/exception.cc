//#ifndef SERVERLIB_NO_GLIBC_EXCEPTION_BACKTRACE
//#define SERVERLIB_NO_GLIBC_EXCEPTION_BACKTRACE
//#endif
//#define SERVERLIB_NO_LIBCXXABI_EXCEPTION_BACKTRACE
// for backtrace
#include <cxxabi.h>
#ifndef SERVERLIB_NO_GLIBC_EXCEPTION_BACKTRACE
  #include <execinfo.h>
#else
#ifndef SERVERLIB_NO_LIBCXXABI_EXCEPTION_BACKTRACE
  #include <libunwind.h>
#endif // SERVERLIB_NO_LIBCXXABI_EXCEPTION_BACKTRACE
#endif // SERVERLIB_NO_GLIBC_EXCEPTION_BACKTRACE
#include <string.h>

#include <boost/format.hpp>
#include <boost/algorithm/string/join.hpp>

#include "tcl_utils.h"
#include "helpcpp.h"
#include "exception.h"
#include "base_code_set.h"

#define NICKNAME "NONSTOP"
#include "slogger.h"

#include "logger.h"

using std::string;
using std::stringstream;
using std::runtime_error;
using std::list;

std::string addr2line(const std::string& addr);
namespace
{
string prepareMeta(BIGLOG_SIGNATURE)
{
    stringstream str;
    str << nick << ":" << file << ":" << line << " " << func;
    return str.str();
}

void prepareBacktrace(list<string>& traceList)
{
#ifndef SERVERLIB_NO_GLIBC_EXCEPTION_BACKTRACE
    const int btSz = 100;
    void* addrlist[btSz + 1] = {0};

    int addrlen = ::backtrace(addrlist, btSz);
    if (addrlen == 0) {
        ProgError(STDLOG, "prepare_backtrace failed: possibly corrupt stack");
        return;
    }
    char** symbollist = backtrace_symbols(addrlist, addrlen);

    for (int i = 1; i < addrlen; ++i) {
        char *begin_name = 0, *begin_offset = 0, *end_offset = 0;

        // find parentheses and +address offset surrounding the mangled name:
        // ./module(function+0x15c) [0x8048a6d]
        for (char *p = symbollist[i]; *p; ++p) {
            if (*p == '(')
                begin_name = p;
            else if (*p == '+')
                begin_offset = p;
            else if (*p == ')' && begin_offset) {
                end_offset = p;
                break;
            }
        }
        
        string traceline;
        if (begin_name && begin_offset && end_offset && begin_name < begin_offset) {
            *begin_name++ = '\0';
            *begin_offset++ = '\0';
            *end_offset = '\0';

            int status = 0;
            auto funcname = std::unique_ptr<char, decltype(free)*>(abi::__cxa_demangle(begin_name, nullptr, nullptr, &status), free);
            if (status == 0)
            {
#ifdef SERVERLIB_ADDR2LINE
            string addr = addr2line((boost::format("%p") % addrlist[i]).str());
            traceline = (boost::format("%s : %s+%s") % addr % funcname.get() % begin_offset).str();
#else
            traceline = (boost::format("%s : %s+%s") % addrlist[i] % funcname.get() % begin_offset).str();
#endif // SERVERLIB_ADDR2LINE
            traceList.push_back(traceline);
            }
            else
                traceList.push_back((boost::format("%s : __inlined__function_?__+%s") % addrlist[i] % begin_offset).str());
        } else {
            traceline = (boost::format("%p %s") % addrlist[i] % symbollist[i]).str();
            traceList.push_back(traceline);
        }
    }

    free(symbollist);
#else
#ifndef SERVERLIB_NO_LIBCXXABI_EXCEPTION_BACKTRACE
    unw_cursor_t cursor;
    unw_context_t context;
    unw_getcontext(&context);
    unw_init_local(&cursor, &context);

    while (unw_step(&cursor) > 0)
    {
        unw_word_t offset, pc;
        unw_get_reg(&cursor, UNW_REG_IP, &pc);
        if (pc == 0)
            break;
        char sym[256];
        if (unw_get_proc_name(&cursor, sym, sizeof(sym), &offset) == 0)
        {
            char* nameptr = sym;
            int status;
            auto funcname = std::unique_ptr<char, decltype(free)*>(abi::__cxa_demangle(sym, nullptr, nullptr, &status), free);
            if (status == 0) {
                nameptr = funcname.get();
            }
            traceList.push_back((boost::format("0x%lx : %s+0x%lx") % pc % nameptr % offset).str());
        }
        else
            traceList.push_back((boost::format("0x%lx :  -- error: unable to obtain symbol name") % pc).str());
    }
#else
    traceList.emplace_back("SERVERLIB_NO_..._EXCEPTION_BACKTRACE defined");
#endif
#endif // SERVERLIB_NO_GLIBC_EXCEPTION_BACKTRACE
}

string getBacktraceString(bool backtraceOff)
{
    if (backtraceOff)
        return "";
    static int f=-1;
    if (!getVariableStaticBool("SERVERLIB_STORE_BACKTRACE", &f, 0) ) {
        return "";
    }
    list<string> traceList;
    prepareBacktrace(traceList);

    return boost::algorithm::join(traceList, "\n");
}

}

namespace ServerFramework
{

void  assertionFailed0Impl(char const* const nick, const char* const file, long int line, 
                                    const char* const what)
{
    SIRENA_ABORT_OPT(nick);
    throw comtech::AssertionFailed(nick, file, line, what);
}

void assertionFailed1Impl(const char* const subsystemName, char const* const nick,
                                    const char* const file, long int line, const char* const what)
{
    SIRENA_ABORT_OPT(subsystemName);
    throw AssertionFailed(nick, file, line, what);
}

Exception::Exception(const string& msg, bool backtraceOff)
    : runtime_error(msg), 
    _backtrace(getBacktraceString(backtraceOff)), 
    _stack(backtraceOff ? "" : HelpCpp::CallStackRegisterManager::getInstance().dump())
{}

Exception::Exception(BIGLOG_SIGNATURE, const std::string& msg, bool backtraceOff)
    : runtime_error(prepareMeta(nick, file, line, func) + (msg.empty() or msg[0] != ' ' ? " " : "") + msg),
    _place(prepareMeta(nick, file, line, func)),
    _backtrace(getBacktraceString(backtraceOff)),
    _stack(backtraceOff ? "" : HelpCpp::CallStackRegisterManager::getInstance().dump())
{}

Exception::~Exception() throw()
{}

const char* Exception::place() const
{
    return _place.c_str();
}

const char* Exception::stack() const
{
    return _stack.c_str();
}

const char* Exception::backtrace() const
{
    return _backtrace.c_str();
}

void Exception::print(int level, STDLOG_SIGNATURE) const
{
    ProgTrace(level, nick, file, line, "Exception from(%s)\nbacktrace: %s\nstack: %s", 
            place(), backtrace(), stack());
}

void Exception::error(STDLOG_SIGNATURE) const
{
    ProgError(nick, file, line, "Exception: %s", what());
    print(0, nick, file, line);
}

/*******************************************************************************
 * AssertionFailed
 ******************************************************************************/

AssertionFailed::AssertionFailed(const char* nick,
                                 const char* file,
                                 int long line,
                                 const std::string& expr):
    Exception(nick, file, line, "", expr)
{
}

xml_exception::xml_exception (const std::string &nick_, const std::string &fl_,
     int ln_, const std::string &msg) : Exception(nick_.c_str(), fl_.c_str(), ln_, "", msg)
{
  ProgTrace(getRealTraceLev(1),
    nick_.c_str(), fl_.c_str(), ln_, "thrown jxtlib_exception: '%s'", this->what());
}

xml_exception::xml_exception (const std::string &msg)
     : Exception(msg)
{
}

} // ServerFramework

NoSuchCode::NoSuchCode(const char* n, const char* f, int l, const std::string &wht) : Exception(wht)
{
    LogWarning(n, f, l) << wht;
}

#ifdef XP_TESTING
#include "memmap.h"
#include "xp_test_utils.h"
#include "checkunit.h"
#include "string_cast.h"
#include "str_utils.h"

using namespace ServerFramework;

int meta_exceptions_secret_func1(const char* msg)
{
    throw Exception(BIGLOG, msg);
}

void meta_exceptions_secret_func2(const string& msg)
{
    meta_exceptions_secret_func1(msg.c_str());
}

static void throwException(size_t level)
{

    if(level){
        MarkStack;
        LogTrace(1000,STDLOG)<< string(50,'x')+"level "<<level;
        throwException( level - 1 );
    }else{
        throw comtech::Exception(BIGLOG, "some exception message" );
    }
    
}

static void wrapException(size_t depth)
{
    throwException(depth);
}

static void memory_check(size_t sz)
{
    const size_t RECURSION_DEPTH = 100;

    for(size_t i = 0; i < sz; ++i) {
        try{
            wrapException( RECURSION_DEPTH );  // throw Exception("some exception message");
       }catch(Exception& e){
             e.print(0,STDLOG);
        }
    }
    size_t mem1 = size_of_allocated_mem();
    for(int i=0;i<10;++i){
        for(size_t i = 0; i < sz; ++i) {
            try{
                wrapException( RECURSION_DEPTH );  // throw Exception("some exception message");
           }catch(Exception& e){
                 e.print(0,STDLOG);
            }
        }
        size_t mem2 = size_of_allocated_mem();
        ProgTrace(TRACE5, "%s %zu cycles %zd KB lost", __FUNCTION__, sz, mem1 - mem2);
        fail_unless(mem2 < mem1+100, "Memory sz=%zu leak %zd KB", sz, mem1 - mem2);
    }
//    ProgTrace(TRACE5, "%s %zd cycles %zd KB lost", __FUNCTION__, sz, static_cast<size_t>(labs(mem1 - mem2)));
//    fail_unless(mem1 < mem2+10, "Memory sz=%zd leak %zd KB", sz, static_cast<size_t>(labs(mem1 - mem2)));
}

namespace {

START_TEST(check_meta)
{
    try {
        meta_exceptions_secret_func2("test exception");
        fail_unless(0,"must throw Exception");
    } catch (const Exception& e) {
        ProgTrace(TRACE5, "%s", e.stack());
        ProgTrace(TRACE5, "-------------------------");
        ProgTrace(TRACE5, "%s", e.backtrace());
    }
}
END_TEST

START_TEST(message_number)
{
    MarkStack;
    HelpCpp::CallStackRegisterManager& rm = HelpCpp::CallStackRegisterManager::getInstance();
    const int SKIP_LINES=100;
    for(size_t i = 1; i <= SKIP_LINES+HelpCpp::CallStackRegisterManager::pointMsgMaxNumber; i++)
        rm.log(HelpCpp::string_cast(i).c_str());
    std::stringstream str;
    str << rm.dump();
    unsigned count = 0;
    std::string tmp;
    while(getline(str, tmp)){    
        LogTrace(TRACE1) << "line is " << tmp;
        if(tmp.find("line is")!=std::string::npos ){
            unsigned i = StrUtils::atoiTrim(tmp.substr(tmp.length() - 2, 2));
            fail_unless( i>SKIP_LINES, 
                    "wrong number %u met, expected %u", i, count + 10);
        }
        count++;
    }
    //... and one'll be "END OF STACK TRACE", thus count - 1
   
    fail_unless(count  <= HelpCpp::CallStackRegisterManager::pointMsgMaxNumber
            +2, // + 2 = header and footer
            "number of messages %u exceeded the limit", count  );
}
END_TEST

START_TEST(exception_memcheck)
{
    memory_check(100);
}
END_TEST

void start_tests()
{
    setTclVar( "SERVERLIB_STORE_BACKTRACE", "1" );
    setTclVar( "SERVERLIB_STORE_CALLSTACK", "1" );
    StreamLogger::writeCallStack(-1);
}
#define SUITENAME "Serverlib"
TCASEREGISTER(start_tests, 0)
{
    SET_TIMEOUT(300);
    ADD_TEST(check_meta);
    ADD_TEST(message_number);
    ADD_TEST(exception_memcheck);
}
TCASEFINISH
}

#endif /* XP_TESTING */

