#include "IatciRequestHandler.h"
#include "view_edi_elements.h"
#include "remote_system_context.h"
#include "edi_msg.h"
#include "iatci_api.h"

#include <serverlib/str_utils.h>
#include <edilib/edi_func_cpp.h>
#include <etick/exceptions.h>

#define NICKNAME "ANTON"
#define NICK_TRACE ANTON_TRACE
#include <serverlib/slogger.h>


namespace TlgHandling {

using namespace edifact;
using namespace edilib;
using namespace Ticketing;
using namespace Ticketing::TickExceptions;
using namespace Ticketing::RemoteSystemContext;


IatciRequestHandler::IatciRequestHandler(_EDI_REAL_MES_STRUCT_ *pMes,
                                         const edilib::EdiSessRdData *edisess)
    : AstraRequestHandler(pMes, edisess), m_ediErrorLevel("1")
{
}

std::string IatciRequestHandler::mesFuncCode() const
{
    return ""; // No MSG segment at IATCI
}

void IatciRequestHandler::loadDeferredData()
{
    m_lRes = iatci::loadDeferredCkiData(SystemContext::Instance(STDLOG).inbTlgInfo().tlgNum());
    if(m_lRes.empty()) {
        throw tick_soft_except(STDLOG, AstraErr::EDI_PROC_ERR);
    } else if(m_lRes.size() == 1 && m_lRes.front().status() == iatci::Result::Failed) {
        // что-то пошло не так - скорее всего, случился таймаут
        boost::optional<iatci::ErrorDetails> err = m_lRes.front().errorDetails();
        if(err) {
            throw tick_soft_except(STDLOG, err->errCode(), err->errDesc().c_str());
        } else {
            LogError(STDLOG) << "Warning: error detected but error information not found!";
        }
    }
}

void IatciRequestHandler::saveErrorInfo(const Ticketing::ErrMsg_t& errCode,
                                        const std::string& errText)
{
    setEdiErrorCode(StrUtils::ToUpper(getErdErrByInner(errCode)));
    setEdiErrorText(StrUtils::ToUpper(errText));
}

void IatciRequestHandler::setEdiErrorLevel(const std::string& errLevel)
{
    m_ediErrorLevel = errLevel;
}

const std::string& IatciRequestHandler::ediErrorLevel() const
{
    return m_ediErrorLevel;
}

}//namespace TlgHandling
