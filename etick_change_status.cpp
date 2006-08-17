//
// C++ Implementation: etick_change_status
//
// Description: Функции запроса на смену статуса
//
// Roman

#define NICKNAME "ROMAN"
#include "test.h"
#include "etick_change_status.h"
#include "tlg/edi_tlg.h"
namespace Ticketing
{
namespace ChangeStatus
{
    void ETChangeStatus(const TReqInfo *reqInfo, const std::list<Ticket> &lTick,
                        Ticketing::Itin* itin)
    {
        OrigOfRequest org(*reqInfo);
        ProgTrace(TRACE2,"request for change of status from:");
        org.Trace(TRACE2);
        Ticket::Trace(TRACE2, lTick);
        if(itin)
        {
            itin->Trace(TRACE2);
        }

        ChngStatData chngData(org,lTick,itin);
        SendEdiTlgTKCREQ_ChangeStat(chngData);
    }
}
}
