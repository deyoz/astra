#pragma once

#include <edilib/edi_request_handler.h>
#include <etick/etick_msg_types.h>
#include <libtlg/tlgnum.h>


struct _EDI_REAL_MES_STRUCT_;

namespace TlgHandling {

class AstraEdiRequestHandler: public edilib::EdiRequestHandler
{
public:
    AstraEdiRequestHandler(_EDI_REAL_MES_STRUCT_* pMes,
                           const edilib::EdiSessRdData *edisess);

    virtual void onParseError(const std::exception *e);
    virtual void onHandlerError(const std::exception *e);
    virtual bool needPutErrToQueue() const;

    virtual void saveErrorInfo(const Ticketing::ErrMsg_t& errCode,
                               const std::string& errText);

    virtual ~AstraEdiRequestHandler() {}

protected:
    tlgnum_t inboundTlgNum() const;
};

}//namespace TlgHandling
