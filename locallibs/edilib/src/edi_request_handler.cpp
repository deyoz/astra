#include <edilib/edi_request_handler.h>
#include <edilib/edi_func_cpp.h>


namespace edilib {

EdiRequestHandler::EdiRequestHandler(_EDI_REAL_MES_STRUCT_ *mes, const edilib::EdiSessRdData *edisess)
    :PMesSelect(mes), PMesInsert(0), EdiSess(edisess),
     RespStatus(edilib::EdiRespStatus::successfully)
{
}

EdiRequestHandler::~ EdiRequestHandler()
{
}

_EDI_REAL_MES_STRUCT_ * EdiRequestHandler::pMes() const
{
    return PMesSelect;
}

_EDI_REAL_MES_STRUCT_ * EdiRequestHandler::pMesW() const
{
    return PMesInsert;
}

void EdiRequestHandler::setMesW(_EDI_REAL_MES_STRUCT_ *val)
{
    PMesInsert = val;
}

edilib::EdiSessionId_t EdiRequestHandler::ediSessId() const
{
    return EdiSess->ediSession()->ida();
}

const EdiSessRdData *EdiRequestHandler::ediSess() const
{
    return EdiSess;
}

void EdiRequestHandler::makeAnAnswerErr()
{
    SetEdiFullSegment(pMesW(), SegmElement("ERC"), ediErrorCode());

    if(!ediErrorText().empty())
    {
        SetEdiSegment(pMesW(), SegmElement("IFT"));
        PushEdiPointW(pMesW());
        SetEdiPointToSegmentW(pMesW(), "IFT");
        SetEdiFullComposite(pMesW(), CompElement("C346"),  "3");

        std::string text = ediErrorText();
        int numParts = text.length()/70 + ((text.length()%70==0)?0:1);
        for(int i = 0; i < numParts; i++)
        {
            edilib::SetEdiDataElem(pMesW(), edilib::DataElement(4440, i), text.substr(i*70, 70));
        }

        PopEdiPointW(pMesW());
    }
}

void EdiRequestHandler::makeMSG()
{
    if(mesFuncCodeAnswer().empty())
        return;

    std::ostringstream tmp;
    tmp << ":" << mesFuncCodeAnswer() << "+" << RespStatus.code();
    edilib::SetEdiFullSegment(pMesW(), edilib::SegmElement("MSG"), tmp.str());
}

void EdiRequestHandler::setRespStatus(edilib::EdiRespStatus::RespStatus_t respStatus)
{
    RespStatus = edilib::EdiRespStatus(respStatus);
}

void EdiRequestHandler::onHandlerError(const std::exception *e)
{
    setEdiErrorCode("118");
    setRespStatus(edilib::EdiRespStatus::notProcessed);
}

} // namespace edilib
