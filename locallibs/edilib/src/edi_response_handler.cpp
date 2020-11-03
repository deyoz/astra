//
// C++ Implementation: edi_response
//
// Description: Базовый класс обработки edifact ответов
//
//
// Author: Kovalev Roman <rom@sirena2000.ru>, (C) 2008
//
//
#include "edilib/edi_response_handler.h"
#include "edilib/edi_except.h"
#include "edilib/edi_func_cpp.h"

#define NICKNAME "ROMAN"
#define NICKTRACE ROMAN_TRACE
#include <serverlib/slogger.h>

namespace edilib
{

EdiRespStatus::EdiRespStatus(RespStatus_t Stat)
    :Status(Stat)
{
    switch(Status)
    {
        case successfully:
            Code = "3";
            break;
        case notProcessed:
            Code = "7";
            break;
        case rejected:
            Code = "8";
            break;
        case partial:
            Code = "6";
            break;
    };
}

EdiRespStatus::EdiRespStatus(const std::string & resp_status)
{
    Code = resp_status;
    if(resp_status == "3") {
        // OK
        Status = successfully;
    } else if(resp_status == "6") {
        Status = partial;
    } else if(resp_status == "7") {
        Status = notProcessed;
    } else if(resp_status == "8") {
        Status = rejected;
    } else {
        Status = notProcessed;
        LogWarning(STDLOG) << "Strange edifact response status: " << resp_status <<
                              ". Will be used us notProcessed.";
    }
}

std::string EdiRespStatus::code() const
{
    return Code;
}

EdiRespStatus::RespStatus_t EdiRespStatus::status() const
{
    return Status;
}

EdiResponseHandler::EdiResponseHandler(_EDI_REAL_MES_STRUCT_ *pmes, const EdiSessRdData *EdiSess)
    :PMes(pmes), EdiSess(EdiSess), RespStatus(edilib::EdiRespStatus::successfully)
{
}

_EDI_REAL_MES_STRUCT_ * EdiResponseHandler::pMes() const
{
    return PMes;
}

EdiSessionId_t EdiResponseHandler::ediSessId() const
{
    return EdiSess->ediSession()->ida();
}

const string &EdiResponseHandler::ediErrCode() const
{
    return EdiErrCode;
}

const string &EdiResponseHandler::ediErrText() const
{
    return EdiErrText;
}

void EdiResponseHandler::setEdiErrCode(const string &val)
{
    EdiErrCode = val;
}

void EdiResponseHandler::setEdiErrText(const string &val)
{
    EdiErrText = val;
}

const EdiSessRdData *EdiResponseHandler::ediSess() const
{
    return EdiSess;
}

const EdiRespStatus &EdiResponseHandler::respStatus() const
{
    return RespStatus;
}

void EdiResponseHandler::setRespStatus(const EdiRespStatus &rstat)
{
    RespStatus = rstat;
}

const string EdiResponseHandler::funcCode() const
{
    return FuncCode;
}

void EdiResponseHandler::setFuncCode(const string &fc)
{
    FuncCode = fc;
}

void EdiResponseHandler::fillErrorDetails()
{
    if(pMes())
    {
        using namespace edilib;
        EdiErrCode = GetDBFName(pMes(), DataElement(9321), CompElement("C901"), SegmElement("ERC"));

        if(SetEdiPointToSegmentG(pMes(), SegmElement("IFT")))
        {
            for(unsigned i = 0; i < GetNumDataElem(pMes(), 4440); i++)
                EdiErrText += GetDBFName(pMes(), DataElement(4440, i), "");
        }

        ResetEdiPointG(pMes());
    }
}

void EdiResponseHandler::onHandlerError(const std::exception *)
{
}

bool EdiResponseHandler::needDetectCarf() const
{
    return false;
}

std::string EdiResponseHandler::unReference() const
{
    return ediSess()->ediSession()->ourCarf();
}

} // namespace edilib

