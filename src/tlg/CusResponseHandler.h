#pragma once

#include "ResponseHandler.h"
#include <boost/shared_ptr.hpp>

namespace edifact {
struct Cusres;
}//namespace edifact


namespace TlgHandling
{

class CusResponseHandler: public AstraEdiResponseHandler
{
    boost::shared_ptr<edifact::Cusres> m_data;

public:
    CusResponseHandler(_EDI_REAL_MES_STRUCT_ *PMes,
                       const edilib::EdiSessRdData *edisess);

    virtual void fillFuncCodeRespStatus() {}
    virtual void fillErrorDetails() {}
    virtual void parse();
    virtual void handle();
    virtual void onTimeOut() {}
    virtual void onCONTRL() {}

    virtual ~CusResponseHandler() {}
};

}//namespace TlgHandling
