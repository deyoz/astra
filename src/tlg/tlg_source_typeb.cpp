#include "tlg_source_typeb.h"
#include "tlg_source_edifact.h"

#define NICKNAME "ANTON"
#define NICKTRACE ANTON_TRACE
#include <serverlib/slogger.h>


namespace TlgHandling {

TlgSourceTypeB::TlgSourceTypeB(const TlgSource& src)
    : TlgSourceTypified(src)
{}

void TlgSourceTypeB::write()
{
    LogTrace(TRACE1) << "TlgSourceTypeB write";
    TlgSourceTypified::writeToDb(*this);
}

bool TlgSourceTypeB::isItYours(const std::string &txt)
{
    // всё, что не edifact, считаем typeb
    return !TlgSourceEdifact::isItYours(txt);
}

std::ostream& operator<<(std::ostream& os, const TlgSourceTypeB& tlg)
{
    os << tlg.text();
    return os;
}

}//namespace TlgHandling
