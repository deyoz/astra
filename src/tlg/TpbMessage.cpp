#include "TpbMessage.h"
#include "tlg.h"
#include "tlg_source_typeb.h"

#include <sstream>
#include <boost/scoped_ptr.hpp>

#define NICKNAME "ANTON"
#define NICK_TRACE ANTON_TRACE
#include <serverlib/slogger.h>


namespace airimp {

using TlgHandling::TlgSourceTypeB;


TpbMessage::TpbMessage(const std::string& msg,
                       const Ticketing::RemoteSystemContext::SystemContext* sysCont)
    : Msg(msg), SysCont(sysCont)
{
    Header = makeHeader();
}

void TpbMessage::send()
{
    boost::scoped_ptr<TlgSourceTypeB> TlgOut(new TlgSourceTypeB(fullMsg()));
    TlgOut->setToRot(sysCont()->routerCanonName());
    TlgOut->setFromRot(OWN_CANON_NAME());

    LogTrace(TRACE1) << *TlgOut;

    // В очередь на отправку
    sendTpbTlg(*TlgOut);
}

const Ticketing::RemoteSystemContext::SystemContext* TpbMessage::sysCont() const
{
    return SysCont;
}

std::string TpbMessage::fullMsg() const
{
    return Header + Msg;
}

std::string TpbMessage::makeHeader()
{
    std::stringstream header;
    header << sysCont()->remoteAddrAirimp() << "\n";
    header << "." << sysCont()->ourAddrAirimp() << "\n";
    return header.str();
}

}//namespace airimp
