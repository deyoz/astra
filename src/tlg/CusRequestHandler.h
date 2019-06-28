#pragma once

#include "RequestHandler.h"


namespace TlgHandling {

class CusRequestHandler: public AstraEdiRequestHandler
{
public:
    CusRequestHandler(_EDI_REAL_MES_STRUCT_ *pMes,
                      const edilib::EdiSessRdData *edisess);

    virtual std::string mesFuncCode() const;
    virtual bool fullAnswer() const;
    virtual void parse();
    virtual void handle() {}
    virtual void makeAnAnswer() {}

    virtual ~CusRequestHandler() {}
};

}//namespace TlgHandling
