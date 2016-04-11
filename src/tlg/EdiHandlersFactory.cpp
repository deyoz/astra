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

// et handlers
#include "EtDispResponseHandler.h"
#include "EtCosResponseHandler.h"
// emd handlers
#include "EmdDispResponseHandler.h"
#include "EmdCosResponseHandler.h"
#include "EmdSysUpdateResponseHandler.h"
// iatci handlers
#include "IatciCkiResponseHandler.h"
#include "IatciCkuResponseHandler.h"
#include "IatciCkxResponseHandler.h"
#include "IatciBprResponseHandler.h"
#include "IatciPlfResponseHandler.h"
#include "IatciSmfResponseHandler.h"

#define NICKNAME "ROMAN"
#define NICKTRACE ROMAN_TRACE
#include <serverlib/slogger.h>

namespace Ticketing
{

#define __DECLARE_HANDLER__(handler, msg__, func_code__) \
        if(msg__ == msgid && (func_code == func_code__ || !*func_code__))\
        {\
            return new handler(0, psess.get());\
        }

TlgHandling::AstraEdiResponseHandler* EdiResHandlersFactory(edi_msg_types_t msgid,
                                                            const std::string &func_code,
                                                            boost::shared_ptr<AstraEdiSessRD> psess)
{
    using namespace TlgHandling;

    // ET
    __DECLARE_HANDLER__(EtDispResponseHandler,       TKCRES, "131");
    __DECLARE_HANDLER__(EtCosResponseHandler,        TKCRES, "142");
    // EMD
    __DECLARE_HANDLER__(EmdDispResponseHandler,      TKCRES, "791");
    __DECLARE_HANDLER__(EmdCosResponseHandler,       TKCRES, "793");
    __DECLARE_HANDLER__(EmdSysUpdateResponseHandler, TKCRES, "794");
    // IATCI
    __DECLARE_HANDLER__(IatciCkiResponseHandler,     DCRCKA, "I");
    __DECLARE_HANDLER__(IatciCkuResponseHandler,     DCRCKA, "U");
    __DECLARE_HANDLER__(IatciCkxResponseHandler,     DCRCKA, "X");
    __DECLARE_HANDLER__(IatciBprResponseHandler,     DCRCKA, "B");
    __DECLARE_HANDLER__(IatciPlfResponseHandler,     DCRCKA, "P");
    __DECLARE_HANDLER__(IatciPlfResponseHandler,     DCRSMF, "S");

    LogError(STDLOG) <<
            "There is no factory for message " << msgid <<
            "; function code is " << func_code;
    return 0;
}

#undef __DECLARE_HANDLER__

}//namespace Ticketing
