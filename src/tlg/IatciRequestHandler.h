#pragma once

#include "RequestHandler.h"
#include "iatci_types.h"

#include <edilib/EdiSessionId_t.h>

#include <boost/optional.hpp>

namespace TlgHandling {

class IatciRequestHandler: public AstraRequestHandler
{
    std::string m_reqRef;
    std::string m_ediErrorLevel;

protected:
    std::list<iatci::dcrcka::Result> m_lRes;

public:
    IatciRequestHandler(_EDI_REAL_MES_STRUCT_ *pMes,
                        const edilib::EdiSessRdData *edisess);

    virtual std::string mesFuncCode() const;
    virtual bool fullAnswer() const;
    virtual void handle();
    virtual void makeAnAnswer();
    virtual void makeAnAnswerErr();

    virtual std::string respType() const = 0;    // Значение RAD:9868

    virtual ~IatciRequestHandler() {}

protected:
    virtual const iatci::IBaseParams* paramsNew() const = 0;

    virtual iatci::dcrcka::Result handleRequest() const = 0;
    virtual edilib::EdiSessionId_t sendCascadeRequest() const = 0;
    virtual void saveErrorInfo(const Ticketing::ErrMsg_t& errCode,
                               const std::string& errText);

    void setEdiErrorLevel(const std::string& errLevel);
    const std::string& ediErrorLevel() const;

    bool postponeHandling() const;

private:
    virtual void loadDeferredData();
};

}//namespace TlgHandling
