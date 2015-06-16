#include "IatciSeatmapRequestHandler.h"
#include "view_edi_elements.h"

#include <edilib/edi_func_cpp.h>

#include <boost/foreach.hpp>

#define NICKNAME "ANTON"
#define NICK_TRACE ANTON_TRACE
#include <serverlib/slogger.h>


namespace TlgHandling {

using namespace edifact;
using namespace edilib;


IatciSeatmapRequestHandler::IatciSeatmapRequestHandler(_EDI_REAL_MES_STRUCT_ *pMes,
                                                       const edilib::EdiSessRdData *edisess)
    : IatciRequestHandler(pMes, edisess)
{
}

void IatciSeatmapRequestHandler::makeAnAnswer()
{
    int curSg1 = 0;
    BOOST_FOREACH(const iatci::Result& res, m_lRes)
    {
        PushEdiPointW(pMesW());
        SetEdiSegGr(pMesW(), SegGrElement(1, curSg1));
        SetEdiPointToSegGrW(pMesW(), SegGrElement(1, curSg1), "SegGr1(flg) not found");

        viewFdrElement(pMesW(), res.flight());
        viewRadElement(pMesW(), respType(), res.statusAsString());
        if(res.cascadeDetails())
            viewChdElement(pMesW(), *res.cascadeDetails());

        ASSERT(res.seatmap());
        if(res.seatmap()->seatRequestDetails()) {
            viewSrpElement(pMesW(), *res.seatmap()->seatRequestDetails());
        }

        int curCbd = 0;
        BOOST_FOREACH(const iatci::CabinDetails& cabin, res.seatmap()->lCabinDetails()) {
            viewCbdElement(pMesW(), cabin, curCbd++);
        }

        int curRod = 0;
        BOOST_FOREACH(const iatci::RowDetails& row, res.seatmap()->lRowDetails()) {
            viewRodElement(pMesW(), row, curRod++);
        }

        curSg1++;

        PopEdiPointW(pMesW());
    }
}

}//namespace TlgHandling
