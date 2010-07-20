/*
*  C++ Implementation: EdiHandlersFactory
*
* Description: edifact tlg handlers factory
*
*
* Author: Kovalev Roman <rom@sirena2000.ru>, (C) 2008
*
*/
#include "EdiHandlersFactory.h"

#define NICKNAME "ROMAN"
#define NICKTRACE ROMAN_TRACE
#include <serverlib/slogger.h>

namespace Ticketing
{

edifact::EdifactResponse * EdiResHandlersFactory(edi_msg_types_t msgid,
                                            const std::string &func_code,
                                            boost::shared_ptr<edifact::EdiSessRD> psess)
{
    switch (msgid)
    {
        case TKCRES:
        {
            if(func_code == CosRequest::MsgCode()) // 142
            {
                return new HandleTypeACosResponse(0, psess);
            }
            else if(func_code == RemoteDisplayRequest::MsgCode()) // 131
            {
                return new HandleTypeADispResponse(0, psess);
            }
        }
        default:;
    }

    LogError(STDLOG) <<
            "There is no factory for message " << msgid <<
            "; function code is " << func_code;
    return 0;
}

}
