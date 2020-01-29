#include <boost/bind.hpp>
#include <boost/function.hpp>

#include "ocilocal.h"
#include "cursctl.h"
#include "oci_err.h"
#include "oci_selector_char.h"

#define NICKNAME "NONSTOP"
#include "test.h"

namespace OciCpp {

void DbmsOutput::enable() 
{
    if( this->enabled )
        return;
    int size = 40000;
    make_curs("begin dbms_output.enable(:sz); end;").bindOut(":sz", size).exec();
    this->enabled = true;
}

void DbmsOutput::disable()
{
    if( !this->enabled )
        return;
    make_curs("begin dbms_output.disable; end;").exec();
    this->enabled = false;
}

DbmsOutput::DbmsOutput()
{
    this->enabled = false;
    enable();
}

DbmsOutput::~DbmsOutput()
{
    if( this->enabled ) {
        disable();
    }
}

static void handleBuffTrace(const char (&buf)[2000], bool needPrint, int level, STDLOG_SIGNATURE)
{
    if( needPrint ) {
        ProgTrace(level, STDLOG, "<%s>", buf);
    }
}

static void handleBuffAppendStr(const char (&buf)[2000], std::string& str)
{
    str += std::string(buf, 2000);
}

static void handleDbmsOutput(boost::function<void(const char (&)[2000])> handler)
{
    char buf[2000]; 
    buf[0] = 0;
    int stat = 0;
    OciCpp::CursCtl c = make_curs("begin dbms_output.get_line(:str, :n); end;");
    c.unstb();
    c.bindOut(":str", buf);
    c.bindOut(":n", stat);
    c.noThrowError(CERR_NULL);
    c.exec();
    while( !stat ) {
        handler(buf);
        c.exec();
    }
}

void DbmsOutput::getLines(bool needPrint, int level, STDLOG_SIGNATURE) 
{
    if (!this->enabled)
        return;
    handleDbmsOutput(boost::bind(&handleBuffTrace, _1, needPrint, level, nick, file, line));
}

void DbmsOutput::clear()
{
    getLines(false, 0, STDLOG); //last 2 are ignored
}

void DbmsOutput::print(int level, STDLOG_SIGNATURE)
{
    getLines(true, level, STDLOG);
}

std::string DbmsOutput::str() const
{
    std::string res;
    if (this->enabled)
        handleDbmsOutput(boost::bind(&handleBuffAppendStr, _1, boost::ref(res)));
    return res;
}

}//OciCpp
