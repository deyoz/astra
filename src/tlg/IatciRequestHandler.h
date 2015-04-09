#pragma once

#include "RequestHandler.h"
#include "iatci_types.h"

namespace TlgHandling {

class IatciRequestHandler: public AstraRequestHandler
{
    std::string m_reqRef;
public:
    IatciRequestHandler(_EDI_REAL_MES_STRUCT_ *pMes,
                        const edilib::EdiSessRdData *edisess);
    virtual void makeAnAnswer();
    virtual void makeAnAnswerErr();
    virtual std::string mesFuncCode() const;
    virtual std::string respType() const = 0;    // Значение RAD:9868
    //virtual std::string requestReference() const;

    //void setRequestReference(const std::string& reqRef);

    virtual ~IatciRequestHandler() {}
};

}//namespace TlgHandling
