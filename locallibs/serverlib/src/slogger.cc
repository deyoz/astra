/*
*  C++ Implementation: slogger
*
* Description: Потоковый логгировщег
*
*
* Author: Kovalev Roman <rom@sirena2000.ru>, (C) 2006
*
*/
#include "helpcpp.h"
#include "string_cast.h"
#include "lwriter.h"
#include <boost/date_time/gregorian/gregorian_types.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include "dates.h"
#include "tcl_utils.h"
#define NICKNAME "ROMAN"
#include "slogger.h"
#include "logger.h"
#include "log_manager.h"

namespace {
   const std::string tabulation("\t\t");
   const std::string colon(":");
}


StreamLogger::StreamLogger(int level, const char * nick, const char * file, unsigned line)
    :Level(level), Nick(nick), File(file), Line(line),
    CutLog( !LogManager::Instance().isWriteLog(level) ),Stream(0)
{
}

bool StreamLogger::writeCallStack(int reset)
{
    static int f=-1;
    if(reset!=-2)
    {
        f=reset;
    }
    if(f==-1){
        f=getVariableStaticBool("SERVERLIB_STORE_CALLSTACK", &f,0);
    }
    return f!=0;
}
void StreamLogger::flush()
{

    if (writeCallStack()) {
        std::string const &logstr = stream().str(); 
        HelpCpp::CallStackRegisterManager::getInstance()
        .log(tabulation+ Nick+colon+File+colon+HelpCpp::string_cast(Line)+colon+logstr);
    }
    if (CutLog)
        return;
    {
     std::string const &logstr = stream().str();
     write_log_message_str(Level, Nick, File, Line, logstr.c_str());
    }
}

StreamLogger & StreamLogger::setf(std::ios::fmtflags fmtfl)
{   
    if (CutLog)  return *this;
    stream().setf(fmtfl);
    return *this;
}

StreamLogger & StreamLogger::setf(std::ios::fmtflags fmtfl, std::ios::fmtflags mask)
{
    if (CutLog)  return *this;
    stream().setf(fmtfl, mask);
    return *this;
}

std::ostringstream & StreamLogger::stream()
{
    if (!Stream) {
        Stream = new std::ostringstream();
        Stream->setf(std::ios_base::boolalpha);
    }
    return *Stream;
}

StreamLogger::~StreamLogger()
{
    flush();
    delete this->Stream;
}

StreamLogger::StreamLogger(const StreamLogger &sl):Level(sl.Level), Nick(sl.Nick),
                                                   File(sl.File), Line(sl.Line),
                                                   CutLog(sl.CutLog), Stream(0)
{
}

StreamLogger & StreamLogger::operator<< (const boost::gregorian::date& x) {
    return operator<< <boost::gregorian::date> (x);
}

StreamLogger & StreamLogger::operator<< (const boost::posix_time::ptime& x) {
    return operator<< <boost::posix_time::ptime> (x);
}

StreamLogger & StreamLogger::operator<< (const boost::posix_time::time_duration& x) {
    return operator<< <boost::posix_time::time_duration> (x);
}

StreamLogger & StreamLogger::operator<< (std::basic_ostream<char>& (*func)(std::basic_ostream<char>&)) {
    return operator<< <> (func);
}

StreamLogger & StreamLogger::operator<< (std::ios_base& (*func)(std::ios_base&)) {
    return operator<< <> (func);
}

StreamLogger & StreamLogger::operator<< (std::basic_ios<char>& (*func)(std::basic_ios<char>&)) {
    return operator<< <> (func);
}

StreamLogger & StreamLogger::operator<< (char const* v) {
    return operator<< <> (v ? v : "(nil)");
}

#ifdef XP_TESTING
#include <sstream>
#include <iomanip>
#include "tcl_utils.h"
#include "xp_test_utils.h"

#include "checkunit.h"

namespace {

int cutLogTest(int l)
{
  return 2 < l;
}

struct TLogTest
{
  int i;
};

static int LogCounter=0;
std::ostream & operator << (std::ostream& os, const TLogTest &k)
{
  os<<(LogCounter++);
  return os;
}

struct Foo
{
    Foo() {
        vec_ = std::vector<int>{1, 2, 3};
    }
    ~Foo() {
        LogTrace(TRACE0) << "~Foo";
    }
    const std::vector<int>& vec() const {
        return vec_;
    }
    std::vector<int> vec_;
};


START_TEST(slogger)
{
  std::ostringstream os;
  os<<TLogTest(); // 0
  os<<TLogTest(); // 1
  fail_unless(os.str()=="01","invalid test object");

  setCutLogger(cutLogTest); // Cutlog=2
  LogTrace(TRACE5)<<TLogTest(); // --
  os<<TLogTest(); // 2
  fail_unless(os.str()=="012",(os.str()+": invalid operator << call").c_str());
  LogTrace(TRACE1)<<TLogTest(); // 3
  os<<TLogTest(); // 4
  fail_unless(os.str()=="0124",(os.str()+": invalid operator << call").c_str());

  LogTrace(TRACE0) << LogCont("-", Foo().vec());
  std::map<int, std::string> vals;
  vals[0] = "zero";
  vals[1] = "one";
  vals[5] = "five";
  LogTrace(TRACE0) << LogCont("-", vals, [](LogStream& s, const auto& p) { s << '[' << p.first << '|' << p.second << ']';});
                 
                
  // Compile test:
  LogTrace(TRACE0)<<1<<std::endl;
  LogTrace(TRACE0)<<1<<std::hex;
  LogTrace(TRACE0)<<1<<std::uppercase;
  LogTrace(TRACE0)<<1<<std::setw(10);
  LogTrace(TRACE0)<<1<<std::setw(10)<<std::right;

  LogTrace(TRACE0)<<1<<std::endl;
  LogTrace(TRACE0)<<1<<std::hex<<99;
  LogTrace(TRACE0)<<1<<std::uppercase<<"asdasd";
  LogTrace(TRACE0)<<1<<std::setw(10)<<99;
  LogTrace(TRACE0)<<1<<std::setw(10)<<std::right<<99;

  LogTrace(TRACE0)<<std::endl;
  LogTrace(TRACE0)<<std::hex<<99;
  LogTrace(TRACE0)<<std::uppercase<<"asdasd";
  LogTrace(TRACE0)<<std::setw(10)<<99;
  LogTrace(TRACE0)<<std::setw(10)<<std::right<<99;
  
}
END_TEST;

#define SUITENAME "Serverlib"

TCASEREGISTER(0, 0)
{
    ADD_TEST(slogger);
}TCASEFINISH

} // namespace
#endif /* XP_TESTING */
