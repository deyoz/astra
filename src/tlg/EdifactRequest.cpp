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
#include "remote_results.h"
#include "edi_tlg.h"
#include "tlg.h"
#include "tlg_source_edifact.h"
#include "EdiSessionTimeOut.h"
#include "AgentWaitsForRemote.h"
#include "astra_context.h"
#include "astra_consts.h"

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

EdifactRequest::EdifactRequest(const std::string &pult,
                               const std::string& ctxt,
                               const edifact::KickInfo &v_kickInfo,
                               edi_msg_types_t msg_type,
                               const Ticketing::RemoteSystemContext::SystemContext* sysCont)
    :edilib::EdifactRequest(msg_type), TlgOut(0),
     ediSessCtxt(ctxt), m_kickInfo(v_kickInfo), SysCont(sysCont)
{
    setEdiSessionController(new NewAstraEdiSessWR(pult, msgHead(), sysCont));
    setEdiSessMesAttr();
}

EdifactRequest::~EdifactRequest()
{
    delete TlgOut;
    delete SysCont;
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
              qpOutA, //!!!здесь доделать step by step, если kickInfo.background_mode
              sysCont()->edifactResponseTimeOut(),
              TlgOut->text(),
              ASTRA::NoExists,
              ASTRA::NoExists);
    ediSess()->ediSession()->CommitEdiSession();

    //запишем контексты
    AstraContext::SetContext("EDI_SESSION",
                             ediSessId().get(),
                             context());


    // Записать информацию о timeout отправленной телеграммы
    edilib::EdiSessionTimeOut::add(ediSess()->edih()->msg_type,
                                   funcCode(),
                                   ediSessId(),
                                   sysCont()->edifactResponseTimeOut());

    RemoteResults::add(kickInfo().msgId,
                       kickInfo().desk,
                       ediSess()->ediSession()->ida(),
                       sysCont()->ida());

    Ticketing::ConfigAgentToWait(sysCont()->ida(),
                                 ediSess()->ediSession()->pult(),
                                 ediSess()->ediSession()->ida(),
                                 kickInfo());

}

std::string EdifactRequest::funcCode() const
{
    return mesFuncCode();
}

const TlgSourceEdifact * EdifactRequest::tlgOut() const
{
    return TlgOut;
}

const Ticketing::RemoteSystemContext::SystemContext * EdifactRequest::sysCont()
{
    return dynamic_cast<NewAstraEdiSessWR*>(ediSess())->sysCont();
}

edilib::EdiSessionId_t EdifactRequest::ediSessionId() const
{
    const edilib::EdiSessWrData *const ediSessWrData = ediSess();
    const edilib::EdiSession *const ediSession = ediSessWrData ? ediSessWrData->ediSession() : nullptr;
    return ediSession ? ediSession->ida() : edilib::EdiSessionId_t();
}

} // namespace edifact

