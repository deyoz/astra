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
    setEdiSessionController(new AstraEdiSessWR(pult, msgHead(), sysCont));
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

    updateMesHead();
    setEdiSessMesAttr();
    collectMessage();

    TlgOut = new TlgSourceEdifact(makeEdifactText(), ediSess()->hth());
    TlgOut->setToRot(sysCont()->routerCanonName());
    TlgOut->setFromRot(OWN_CANON_NAME());

    LogTrace(TRACE1) << *TlgOut;

    // В очередь на отправку
    sendEdiTlg(*TlgOut,
               kickInfo().background_mode()?qpOutAStepByStep:qpOutA,
               sysCont()->edifactResponseTimeOut());

    ediSess()->ediSession()->CommitEdiSession();

    if(needSaveEdiSessionContext()) {
        //запишем контексты
        AstraContext::SetContext("EDI_SESSION",
                                 ediSessId().get(),
                                 context());
    }

    // Записать информацию о timeout отправленной телеграммы
    if(!kickInfo().background_mode()) {
        LogTrace(TRACE1) << "Save EdiSessionTimeOut for session " << ediSessId();
        edilib::EdiSessionTimeOut::add(ediSess()->edih()->msg_type,
                                       funcCode(),
                                       ediSessId(),
                                       sysCont()->edifactResponseTimeOut());
    } else {
        LogTrace(TRACE1) << "No need to save EdiSessionTimeOut in background_mode";
    }

    if(needRemoteResults()) {
        RemoteResults::add(kickInfo().msgId,
                           kickInfo().desk,
                           ediSess()->ediSession()->ida(),
                           sysCont()->ida());
    }

    if(needConfigAgentToWait()) {
        Ticketing::ConfigAgentToWait(sysCont()->ida(),
                                     ediSess()->ediSession()->pult(),
                                     ediSess()->ediSession()->ida(),
                                     kickInfo());
    }
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
    return dynamic_cast<AstraEdiSessWR*>(ediSess())->sysCont();
}

} // namespace edifact
