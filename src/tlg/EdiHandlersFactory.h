//
// C++ Interface: EdiHandlersFactory
//
// Description: edifact tlg handlers factory
//
//
// Author: Kovalev Roman <rom@sirena2000.ru>, (C) 2008
//
//
#pragma once

#include "ResponseHandler.h"
#include "RequestHandler.h"

#include <edilib/edi_astra_msg_types.h>

class AstraEdiSessRD;

namespace Ticketing
{
    TlgHandling::AstraEdiResponseHandler* EdiResHandlersFactory(EDI_REAL_MES_STRUCT *pMes,
                                                                edi_msg_types_t msgid,
                                                                const std::string &func_code,
                                                                const edilib::EdiSessRdData *sessionHandler);

    TlgHandling::AstraEdiRequestHandler* EdiReqHandlersFactory(EDI_REAL_MES_STRUCT *pMes,
                                                               edi_msg_types_t msgid,
                                                               const std::string &func_code,
                                                               const edilib::EdiSessRdData *sessionHandler);

}//namespace Ticketing

