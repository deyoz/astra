#include "IatciSmfResponseHandler.h"

#define NICKNAME "ANTON"
#define NICK_TRACE ANTON_TRACE
#include <serverlib/slogger.h>


namespace TlgHandling {

IatciSmfResponseHandler::IatciSmfResponseHandler(_EDI_REAL_MES_STRUCT_ *pMes,
                                                 const edilib::EdiSessRdData *edisess)
    : IatciSeatmapResponseHandler(pMes, edisess)
{
}

iatci::dcrcka::Result::Action_e IatciSmfResponseHandler::action() const
{
    return iatci::dcrcka::Result::Seatmap;
}

}//namespace TlgHandling
