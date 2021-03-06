#include "ResponseHandler.h"
#include "AgentWaitsForRemote.h"

#include <edilib/edi_types.h>
#include <edilib/edi_func_cpp.h>

#define NICKNAME "ANTON"
#define NICKTRACE ANTON_TRACE
#include <serverlib/slogger.h>


namespace TlgHandling
{

using namespace edilib;


AstraEdiResponseHandler::AstraEdiResponseHandler(_EDI_REAL_MES_STRUCT_ * pMes,
                                         const edilib::EdiSessRdData *edisess)
    : EdiResponseHandler(pMes, edisess)
{
}

void AstraEdiResponseHandler::setRemoteResultStatus()
{
    if(!remoteResults())
        return;

    using namespace edifact;
    RemoteStatus::Status_t stat = RemoteStatus::Success;
    switch(respStatus().status())
    {
        case EdiRespStatus::successfully:
            stat = RemoteStatus::Success;
            break;
        case EdiRespStatus::partial:
        case EdiRespStatus::notProcessed:
        case EdiRespStatus::rejected:
            stat = RemoteStatus::CommonError;
            break;
    }
    remoteResults()->setStatus(stat);
}

void AstraEdiResponseHandler::readRemoteResults()
{
    RemoteResults = edifact::RemoteResults::readDb(ediSessId());

    if(RemoteResults)
    {
        if(ediErrCode().empty()) {
            RemoteResults->setStatus(edifact::RemoteStatus::Success);
        } else {
            RemoteResults->setStatus(edifact::RemoteStatus::CommonError);
            RemoteResults->setEdiErrCode(ediErrCode());
            RemoteResults->setRemark(ediErrText());
        }
    }
}

edifact::pRemoteResults AstraEdiResponseHandler::remoteResults() const
{
    return RemoteResults;
}

AstraEdiResponseHandler::~AstraEdiResponseHandler()
{
  //????墠?뢠?? ????? ?᪫?祭?? ? ???????????, ????? abort ????????, ??? ???????⨬?
  try
  {
    if(RemoteResults)
    {
        LogTrace(TRACE3) << "next MeetAgentExpectations";
        Ticketing::MeetAgentExpectations(*RemoteResults);
    }
  }
  catch(std::exception &e)
  {
    LogError(STDLOG) << __FUNCTION__ << ": " << e.what();
  }
  catch(...)
  {
    LogError(STDLOG) << __FUNCTION__ << ": unknown error";
  }
}

void AstraEdiResponseHandler::fillFuncCodeRespStatus()
{
    readRemoteResults();
    if(pMes())
    {
        // ????? ???? 0 ??? ??ࠡ?⪥ time out
        setFuncCode(
                GetDBFName(pMes(), DataElement(1225), "AstraErr::EDI_INV_MESSAGE_F",
                           CompElement("C302"), SegmElement("MSG")) );
        std::string resp_status =
                GetDBFName(pMes(), DataElement(4343), SegmElement("MSG"),
                           "AstraErr::EDI_INV_MESSAGE_F");

        setRespStatus(EdiRespStatus(resp_status));
        setRemoteResultStatus();
    }
}

void AstraEdiResponseHandler::fillErrorDetails()
{
    if(pMes())
    {
        edilib::EdiPointHolder ph(pMes());
        edilib::SetEdiPointToSegGrG(pMes(), 1);
        edilib::EdiResponseHandler::fillErrorDetails();
    }
}

}// namespace TlgHandling
