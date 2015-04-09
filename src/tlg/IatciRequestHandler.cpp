#include "IatciRequestHandler.h"

#define NICKNAME "ANTON"
#define NICK_TRACE ANTON_TRACE
#include <serverlib/slogger.h>


namespace TlgHandling {

IatciRequestHandler::IatciRequestHandler(_EDI_REAL_MES_STRUCT_ *pMes,
                                         const edilib::EdiSessRdData *edisess)
    : AstraRequestHandler(pMes, edisess)
{
}

void IatciRequestHandler::makeAnAnswer()
{
    // TODO
}

void IatciRequestHandler::makeAnAnswerErr()
{
    // TODO
}

std::string IatciRequestHandler::mesFuncCode() const
{
    return ""; // No MSG segment at IATCI
}

/*std::string IatciRequestHandler::requestReference() const
{
    ASSERT(!m_reqRef.empty());
    return m_reqRef;
}


void IatciRequestHandler::setRequestReference(const std::string& reqRef)
{
    m_reqRef = reqRef;
}*/

}//namespace TlgHandling
