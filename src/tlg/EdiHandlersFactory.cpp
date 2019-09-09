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
#include "EtRacResponseHandler.h"

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

// request handlers
#include "IatciCkiRequestHandler.h"
#include "IatciCkuRequestHandler.h"
#include "IatciCkxRequestHandler.h"
#include "IatciBprRequestHandler.h"
#include "IatciPlfRequestHandler.h"
#include "IatciSmfRequestHandler.h"

// control method
#include "UacRequestHandler.h"
#include "EtCosRequestHandler.h"

// apis
#include "CusResponseHandler.h"
#include "CusRequestHandler.h"


#include <edilib/edi_astra_msg_types.h>

#define NICKNAME "ROMAN"
#define NICKTRACE ROMAN_TRACE
#include <serverlib/slogger.h>

namespace Ticketing
{

#define __DECLARE_HANDLER__(handler, msg__, func_code__) \
        if(msg__ == msgid && (func_code == func_code__ || !*func_code__))\
        {\
            return new handler(pMes, sessionHandler);\
        }

TlgHandling::AstraEdiResponseHandler* EdiResHandlersFactory(EDI_REAL_MES_STRUCT *pMes,
                                                            edi_msg_types_t msgid,
                                                            const std::string &func_code,
                                                            const edilib::EdiSessRdData *sessionHandler)
{
    // здесь будут регистрироваться обработчики edifact-ответов
    using namespace TlgHandling;

    // et
    __DECLARE_HANDLER__(EtDispResponseHandler,       TKCRES, "131");
    __DECLARE_HANDLER__(EtCosResponseHandler,        TKCRES, "142");
    // control method
    __DECLARE_HANDLER__(EtRacResponseHandler,        TKCRES, "734")
    __DECLARE_HANDLER__(EtRacResponseHandler,        TKCRES, "751")
    // emd
    __DECLARE_HANDLER__(EmdDispResponseHandler,      TKCRES, "791");
    __DECLARE_HANDLER__(EmdCosResponseHandler,       TKCRES, "793");
    __DECLARE_HANDLER__(EmdSysUpdateResponseHandler, TKCRES, "794");
    // iatci
    __DECLARE_HANDLER__(IatciCkiResponseHandler,     DCRCKA, "I");
    __DECLARE_HANDLER__(IatciCkuResponseHandler,     DCRCKA, "U");
    __DECLARE_HANDLER__(IatciCkxResponseHandler,     DCRCKA, "X");
    __DECLARE_HANDLER__(IatciBprResponseHandler,     DCRCKA, "B");
    __DECLARE_HANDLER__(IatciPlfResponseHandler,     DCRCKA, "P");
    __DECLARE_HANDLER__(IatciSmfResponseHandler,     DCRSMF, "S");
    // iapi
    __DECLARE_HANDLER__(CusResponseHandler,          CUSRES, "962");

    LogError(STDLOG) <<
            "There is no factory for message " << msgid <<
            "; function code is " << func_code;
    return 0;
}

TlgHandling::AstraEdiRequestHandler* EdiReqHandlersFactory(EDI_REAL_MES_STRUCT *pMes,
                                                           edi_msg_types_t msgid,
                                                           const std::string &func_code,
                                                           const edilib::EdiSessRdData *sessionHandler)
{
    // здесь будут регистрироваться обработчики edifact-запросов
    using namespace TlgHandling;
    // iatci
    __DECLARE_HANDLER__(IatciCkiRequestHandler,     DCQCKI, "");
    __DECLARE_HANDLER__(IatciCkuRequestHandler,     DCQCKU, "");
    __DECLARE_HANDLER__(IatciCkxRequestHandler,     DCQCKX, "");
    __DECLARE_HANDLER__(IatciBprRequestHandler,     DCQBPR, "");
    __DECLARE_HANDLER__(IatciPlfRequestHandler,     DCQPLF, "");
    __DECLARE_HANDLER__(IatciSmfRequestHandler,     DCQSMF, "");
    // control method
    __DECLARE_HANDLER__(UacRequestHandler,          TKCUAC, "733");
    __DECLARE_HANDLER__(CosRequestHandler,          TKCREQ, "142");
    // iapi
    __DECLARE_HANDLER__(CusRequestHandler,          CUSUMS, "132");

    LogError(STDLOG) <<
            "There is no factory for message " << msgid <<
            "; function code is " << func_code;
    return 0;
}

#undef __DECLARE_HANDLER__

}//namespace Ticketing
