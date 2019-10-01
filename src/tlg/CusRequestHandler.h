#pragma once

#include "RequestHandler.h"
#include <boost/shared_ptr.hpp>

namespace edifact {
struct Cusres;
}//namespace edifact


namespace TlgHandling {

class CusRequestHandler: public AstraEdiRequestHandler
{
    boost::shared_ptr<edifact::Cusres> m_data;
    
public:
    CusRequestHandler(_EDI_REAL_MES_STRUCT_ *pMes,
                      const edilib::EdiSessRdData *edisess);

    virtual std::string mesFuncCode() const;
    virtual bool fullAnswer() const;
    virtual void parse();
    virtual void handle();
    virtual void makeAnAnswer();
    virtual void makeAnAnswerErr();

    virtual ~CusRequestHandler() {}
};

}//namespace TlgHandling
