/*
*  C++ Implementation: tlg_source_edifact
*
* Description: tlg_source: Специфика EDIFACT телеграмм
*
*/
#include "tlg_source_edifact.h"
#include "edi_tlg.h"

#include <edilib/edi_user_func.h>
#include <libtlg/telegrams.h>
#include <serverlib/cursctl.h>

#define NICKNAME "ROMAN"
#define NICKTRACE ROMAN_TRACE
#include <serverlib/slogger.h>


namespace TlgHandling
{
using namespace OciCpp;
using namespace std;

bool TlgSourceEdifact::isItYours(const std::string & txt)
{
    if(IsEdifactText(txt.c_str(), txt.size()))
    {
        LogTrace(TRACE3) << "It's EDIFACT!";
        return true;
    }
    else
    {
        return false;
    }
}

void TlgSourceEdifact::write()
{
    TlgSourceTypified::writeToDb(*this);
    if(!h2h())
    {
        LogTrace(TRACE3) << "No h2h";
    }
    else
    {
        if (telegrams::callbacks()->writeHthInfo(tlgNum(), *H2h)) {
            LogError(STDLOG) << "writeHthInfo failed: " << tlgNum();
        }
    }
}

void TlgSourceEdifact::readH2H()
{
    if(!tlgNum().num.valid())
    {
        tst();
        return;
    }

    hth::HthInfo tmp;
    if (telegrams::callbacks()->readHthInfo(tlgNum(), tmp) < 0)
    {
        LogTrace(TRACE3) << "Edifact without H2H";
        H2h.reset();
    }
    else
    {
        H2h.reset( new hth::HthInfo(tmp) );
    }
}

const std::string &TlgSourceEdifact::text2view() const
{
    tst();
    char *buff=0;
    size_t len;
    if(Text2View.empty() && !text().empty())
    {
        tst();
        if(!ChangeEdiCharSet(text().c_str(), text().length(), "SIRE", &buff, &len) && buff)
        {
            tst();
            Text2View = std::string(buff,len);
            free(buff);
        }
        else
        {
            tst();
            Text2View = text();
        }
    }

    tst();
    return Text2View;
}

void TlgSourceEdifact::setTlgSubtype(const std::string & stype)
{
    throw EXCEPTIONS::Exception("Not implemented yet");
}

bool TlgHandling::TlgSourceEdifact::operator ==(const TlgSourceEdifact & t) const
{
    if(text() == t.text() &&
       (h2h()?1:0) == (t.h2h()?1:0) &&
       (h2h()?(*h2h() == *t.h2h()):1))
    {
        return true;
    }
    else
    {
        LogTrace(TRACE3) << "tlgs are differ";
        return false;
    }
}

std::ostream & operator <<(std::ostream & os, const TlgSourceEdifact & tlg)
{
    os << dynamic_cast <const TlgSourceTypified &> (tlg);
    os << "h2h: " << (tlg.h2h()?"":"No h2h");
    if(tlg.h2h())
    {
        os << *tlg.h2h();
    }

    return os;
}

}
