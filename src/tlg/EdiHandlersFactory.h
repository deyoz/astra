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

#include <edilib/edi_astra_msg_types.h>
#include "ResponseHandler.h"

class AstraEdiSessRD;

namespace Ticketing
{
    TlgHandling::AstraEdiResponseHandler* EdiResHandlersFactory(edi_msg_types_t msgid,
                                                                const std::string &func_code,
                                                                boost::shared_ptr<AstraEdiSessRD> edisess);

}//namespace Ticketing

