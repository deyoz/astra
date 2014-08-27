/*
*  C++ Implementation: EdifactRequest
*
* Description: Makes an EDIFACT request structure
*
*
* Author: Komtech-N <rom@sirena2000.ru>, (C) 2007
*
*/
#include "EdifactRequest.h"
#include "exceptions.h"
#include "remote_system_context.h"
#include "edi_tlg.h"
#include "tlg.h"
#include "tlg_source_edifact.h"
#include "EdiSessionTimeOut.h"
#include "AgentWaitsForRemote.h"

#include <edilib/edi_func_cpp.h>

#include <memory.h>
#include <boost/scoped_ptr.hpp>

#define NICKNAME "ROMAN"
#define NICK_TRACE ROMAN_TRACE
#include <serverlib/slogger.h>

namespace edifact
{
using namespace EXCEPTIONS;
using namespace TlgHandling;
using namespace Ticketing;

EdifactRequest::EdifactRequest(const std::string &pult, int ctxtId, edi_msg_types_t msg_type,
                               const Ticketing::RemoteSystemContext::SystemContext* sysCont)
    :edilib::EdifactRequest(msg_type), TlgOut(0), ReqCtxtId(ctxtId)
{
    setEdiSessionController(new NewAstraEdiSessWR(pult, msgHead(), sysCont));
    setEdiSessMesAttr();
}

EdifactRequest::~EdifactRequest()
{
    delete TlgOut;
}

void EdifactRequest::sendTlg()
{
    if(TlgOut)
        delete TlgOut;

    collectMessage();

    TlgOut = new TlgSourceEdifact(makeEdifactText(), ediSess()->hth());

    LogTrace(TRACE1) << *TlgOut;

    // Положить тлг в очередь на отправку
    ::sendTlg(sysCont()->routerCanonName().c_str(),
              OWN_CANON_NAME(),
              qpOutA,
              sysCont()->edifactResponseTimeOut(),
              TlgOut->text(),
              ASTRA::NoExists,
              ASTRA::NoExists);
    ediSess()->ediSession()->CommitEdiSession();

    // Записать информацию о timeout отправленной телеграммы
    edilib::EdiSessionTimeOut::add(ediSess()->edih()->msg_type,
                                   mesFuncCode(),
                                   ediSessId(),
                                   sysCont()->edifactResponseTimeOut());

     Ticketing::ConfigAgentToWait(sysCont()->ida(),
                                  ediSess()->ediSession()->pult(),
                                  ediSess()->ediSession()->ida(),
                                  reqCtxtId());

}

int EdifactRequest::reqCtxtId() const
{
    return ReqCtxtId;
}

const TlgSourceEdifact * EdifactRequest::tlgOut() const
{
    return TlgOut;
}

const Ticketing::RemoteSystemContext::SystemContext * EdifactRequest::sysCont()
{
    return dynamic_cast<NewAstraEdiSessWR*>(ediSess())->sysCont();
}

} // namespace edifact

