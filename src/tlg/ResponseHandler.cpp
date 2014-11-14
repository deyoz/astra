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


AstraEdiResponseHandler::AstraEdiResponseHandler(_EDI_REAL_MES_STRUCT_ * pmes,
                                         const edilib::EdiSessRdData *edisess)
    : EdiResponseHandler(pmes, edisess)
{
}

void AstraEdiResponseHandler::setRemoteResultStatus()
{
    tst();
    if(!remoteResults())
        return;

    using namespace edifact;
    RemoteStatus::Status_t stat = RemoteStatus::Success;
    switch(respStatus().status())
    {
        case EdiRespStatus::successfully:
            stat = RemoteStatus::Success;
            break;
        case EdiRespStatus::unsuccessfully:
        case EdiRespStatus::partial:
            stat = RemoteStatus::CommonError;
            break;
    }
    remoteResults()->setStatus(stat);
}

void AstraEdiResponseHandler::readRemoteResults()
{
    tst();
    RemoteResults = edifact::RemoteResults::readDb(ediSessId());

    if(RemoteResults)
    {
        tst();
        if(ediErrCode().empty())
            RemoteResults->setStatus(edifact::RemoteStatus::Success);
        else
        {
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
    LogTrace(TRACE3) << "destruct ~AstraEdiResponseHandler";
    if(RemoteResults)
    {
        LogTrace(TRACE3) << "next MeetAgentExpectations";
        Ticketing::MeetAgentExpectations(*RemoteResults);
    }
}

void AstraEdiResponseHandler::fillFuncCodeRespStatus()
{
    LogTrace(TRACE3) << "fillFuncCodeRespStatus called";
    readRemoteResults();
    if(pMes())
    {
        // Может быть 0 при обработке time out
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
    tst();
    if(pMes())
    {
        tst();
        edilib::EdiPointHolder ph(pMes());
        edilib::SetEdiPointToSegGrG(pMes(), 1);
        edilib::EdiResponseHandler::fillErrorDetails();
    }
}

}// namespace TlgHandling
