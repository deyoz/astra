#pragma once

#include "ResponseHandler.h"
#include "iatci_types.h"
#include "edi_elements.h"


namespace TlgHandling
{

class IatciResponseHandler: public AstraEdiResponseHandler
{
protected:
    std::list<iatci::Result> m_lRes;

public:
    IatciResponseHandler(_EDI_REAL_MES_STRUCT_ *PMes,
                         const edilib::EdiSessRdData *edisess);

    virtual iatci::Result::Action_e action() const = 0;
    virtual void fillFuncCodeRespStatus();
    virtual void fillErrorDetails();
    virtual void parse();
    virtual void handle();
    virtual void onTimeOut();
    virtual void onCONTRL() {}

    virtual ~IatciResponseHandler() {}
};

}//namespace TlgHandling
