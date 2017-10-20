#pragma once

#include "RequestHandler.h"


namespace TlgHandling {

class UacRequestHandler: public AstraEdiRequestHandler
{
public:
    UacRequestHandler(_EDI_REAL_MES_STRUCT_ *pMes,
                      const edilib::EdiSessRdData *edisess);

    virtual std::string mesFuncCode() const;
    virtual bool fullAnswer() const;
    virtual void parse() {}
    virtual void handle();
    virtual void makeAnAnswer() {}

    virtual ~UacRequestHandler() {}
};

}//namespace TlgHandling
