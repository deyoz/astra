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
#include "edi_tlg.h"

// emd handlers
#include "EmdDispResponseHandler.h"
#include "EmdSysUpdateResponseHandler.h"
// iatci handlers
#include "IatciCkiResponseHandler.h"
#include "IatciCkuResponseHandler.h"
#include "IatciCkxResponseHandler.h"

#define NICKNAME "ROMAN"
#define NICKTRACE ROMAN_TRACE
#include <serverlib/slogger.h>

namespace Ticketing
{

TlgHandling::AstraEdiResponseHandler* EdiResHandlersFactory(edi_msg_types_t msgid,
                                                            const std::string &func_code,
                                                            boost::shared_ptr<AstraEdiSessRD> psess)
{
    switch (msgid)
    {
        case TKCRES:
        {
            if(func_code == "791") // 791
            {
                return new TlgHandling::EmdDispResponseHandler(0, psess.get());
            }
            else if(func_code == "794") // 794
            {
                return new TlgHandling::EmdSysUpdateResponseHandler(0, psess.get());
            }
        }
        case DCRCKA:
        {
            if(func_code == "I") // CKI
            {
                return new TlgHandling::IatciCkiResponseHandler(0, psess.get());
            }
            else if(func_code == "U") // CKU
            {
                return new TlgHandling::IatciCkuResponseHandler(0, psess.get());
            }
            else if(func_code == "X") // CKX
            {
                return new TlgHandling::IatciCkxResponseHandler(0, psess.get());
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
