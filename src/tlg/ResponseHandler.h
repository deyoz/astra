#pragma once

#include "remote_results.h"

#include <edilib/edi_response_handler.h>
#include <edilib/edi_session.h>

struct _EDI_REAL_MES_STRUCT_;

namespace TlgHandling
{
/**
 * @class AstraEdiResponseHandler
 * @brief Базовый класс обработки edifact ответов
*/
class AstraEdiResponseHandler : public edilib::EdiResponseHandler
{
    edifact::pRemoteResults RemoteResults;
protected:
    AstraEdiResponseHandler(_EDI_REAL_MES_STRUCT_* pMes,
                            const edilib::EdiSessRdData *edisess);
    void setRemoteResultStatus();
public:
    void readRemoteResults();

    edifact::pRemoteResults remoteResults() const;

    virtual void fillFuncCodeRespStatus();
    virtual void fillErrorDetails();

    virtual ~AstraEdiResponseHandler();
};

}// namespace TlgHandling
