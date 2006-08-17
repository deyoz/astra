//
// C++ Interface: etick_change_status
//
// Description: Функции запроса на смену статуса
//
// Roman
//
#ifndef _ETICK_CHANGE_STATUS_H_
#define _ETICK_CHANGE_STATUS_H_
#include <list>
#include "astra_ticket.h"

namespace Ticketing
{
namespace ChangeStatus
{
    void ETChangeStatus(const TReqInfo *reqInfo, const std::list<Ticket> &lTick,
                        Ticketing::Itin* itin=NULL);
}
}
#endif /*_ETICK_CHANGE_STATUS_H_*/
