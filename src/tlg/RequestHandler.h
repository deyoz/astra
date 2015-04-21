#pragma once

#include <edilib/edi_request_handler.h>
#include <etick/etick_msg_types.h>


struct _EDI_REAL_MES_STRUCT_;

namespace TlgHandling {

class AstraRequestHandler: public edilib::EdiRequestHandler
{
public:
    AstraRequestHandler(_EDI_REAL_MES_STRUCT_* pMes,
                        const edilib::EdiSessRdData *edisess);

    virtual void onParseError(const std::exception *e);
    virtual void onHandlerError(const std::exception *e);
    virtual bool needPutErrToQueue() const;

    virtual void saveErrorInfo(const Ticketing::ErrMsg_t &errCode,
                               const std::string& errText) {}

    virtual ~AstraRequestHandler() {}
};

}//namespace TlgHandling
