//
// C++ Interface: EdiHandlersFactory
//
// Description: edifact tlg handlers factory
//
//
// Author: Kovalev Roman <rom@sirena2000.ru>, (C) 2008
//
//
#ifndef _EDIHANDLERSFACTORY_H_
#define _EDIHANDLERSFACTORY_H_

#include <edilib/edi_tick_msg_types.h>
#include "EdifactResponse.h"

namespace Ticketing
{
    edifact::EdifactResponse * EdiResHandlersFactory(edi_msg_types_t msgid, const std::string &func_code,
            boost::shared_ptr<edifact::EdiSessRD> edisess);
}

#endif /*_EDIHANDLERSFACTORY_H_*/
