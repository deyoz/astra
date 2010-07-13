/*
*  C++ Implementation: EdifactRequest
*
* Description: Makes an EDIFACT request structure
*
*
* Author: Komtech-N <rom@sirena2000.ru>, (C) 2007
*
*/
#include <memory.h>
#include <boost/scoped_ptr.hpp>

#include "EdifactRequest.h"
#include "edilib/edi_func_cpp.h"
#include "exceptions.h"
#include "EdiSessionAstra.h"
#define ETICK_CHECK_PRIVATE
// #include "remote_system_context.h"
// #include "tlg_source_edifact.h"
// #include "obr_tlg_queue.h"
// #include "EdiSessionTimeOut.h"
// #include "AgentWaitsForRemote.h"

#define NICKNAME "ROMAN"
#define NICK_TRACE ROMAN_TRACE
#include "serverlib/slogger.h"

namespace edifact
{
using namespace EXCEPTIONS;

EdifactRequest::EdifactRequest(const std::string &pult,
                               const RemoteSystemContext::SystemContext *SCont,
                               edi_msg_types_t msg_type)
    :edilib::EdifactRequest(msg_type), TlgOut(0)
{
    setEdiSessionController(new AstraEdiSessWR(pult));
}

EdifactRequest::~EdifactRequest()
{
    delete TlgOut;
}

const RemoteSystemContext::SystemContext * EdifactRequest::sysCont()
{
    return 0;//dynamic_cast<AstraEdiSessWR *>(ediSess())->sysCont();
}

void EdifactRequest::sendTlg()
{
    if(TlgOut)
        delete TlgOut;

//     LogTrace(TRACE2) <<
//             "RouterId: " << sysCont()->router() <<
//             "; Router H2h: " << (ediSess()->hth()?"yes":"NO");

//     TlgOut = new TlgSourceEdifact(makeEdifactText(), ediSess()->hth());

//     TlgOut->setToRot( sysCont()->router() );

//     LogTrace(TRACE1) << *TlgOut;

//     TlgHandling::TlgQueue::putTlg2OutQueue(*TlgOut);
    ediSess()->ediSession()->CommitEdiSession();
    // Записать информацию о timeout отправленной телеграммы
//     EdiSessionTimeOut::add(ediSess()->edih()->msg_type,
//                            MsgFuncCode,
//                            ediSessId(),
//                            sysCont()->edifactResponseTimeOut());

//     Ticketing::ConfigAgentToWait(sysCont()->ida(),
//                                  Ticketing::EdiSessionId_t(ediSess()->ediSession()->ida()));
}

const TlgHandling::TlgSourceEdifact * EdifactRequest::tlgOut() const
{
    return TlgOut;
}

} // namespace edifact

