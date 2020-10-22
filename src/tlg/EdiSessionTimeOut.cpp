/*
*  C++ Implementation: EdiSessionTimeOut
*
* Description:
*
*
* Author: Kovalev Roman <rom@sirena2000.ru>, (C) 2008
*
*/

#include "EdiSessionTimeOut.h"
#include "EdiHandlersFactory.h"
#include "AgentWaitsForRemote.h"
#include "ResponseHandler.h"
#include "remote_results.h"
#include "edi_tlg.h"

#include <edilib/edi_func_cpp.h>
#include <edilib/edi_session.h>
#include <serverlib/ocilocal.h>

#include <boost/scoped_ptr.hpp>

#define NICKNAME "ROMAN"
#define NICKTRACE ROMAN_TRACE
#include <serverlib/slogger.h>

namespace edifact
{
using namespace edilib;

namespace
{

inline static boost::shared_ptr<AstraEdiSessRD> getSess(EdiSessionId_t id)
{
    boost::shared_ptr<AstraEdiSessRD> psess (new AstraEdiSessRD());
    psess->loadEdiSession(id);
    return psess;
}

}//namespace


void HandleEdiSessTimeOut(const EdiSessionTimeOut & to)
{
    using namespace Ticketing;

    boost::shared_ptr<AstraEdiSessRD> psess = getSess(to.ediSessionId());

    boost::scoped_ptr<TlgHandling::AstraEdiResponseHandler> handler
            (Ticketing::EdiResHandlersFactory(0, /*pMes*/
                                              to.answerMsgType(),
                                              to.funcCode(),
                                              psess.get()));

    if(handler)
    {
        handler->readRemoteResults();
        if(handler->remoteResults())
            handler->remoteResults()->setStatus(RemoteStatus::Timeout);
        handler->onTimeOut();
    }
    else
    {
        LogTrace(TRACE1) << "Nothing to do";
    }
}

//void HandleEdiSessCONTRL(EdiSessionId_t Id)
//{
//    using namespace Ticketing;

//    EdiSessionTimeOut to = EdiSessionTimeOut::readById(Id);
//    boost::shared_ptr<edifact::AstraEdiSessRD> psess = getSess(Id);
//    boost::scoped_ptr<TlgHandling::EdifactResponse> handler
//            (Ticketing::EdiResHandlersFactory(to.answerMsgType(), to.funcCode(), psess));

//    if(!handler)
//    {
//        LogTrace(TRACE1) << "Nothing to do";
//    }
//    else
//    {
//        handler->onCONTRL();
//    }

//    if(handler->remoteResults())
//        handler->remoteResults()->setStatus(RemoteStatus::Contrl);
//}

}//namespace edifact
