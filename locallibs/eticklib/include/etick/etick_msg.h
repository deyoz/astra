/*	2006 by Roman Kovalev 	*/
/*	roman@pike.dev.sirena2000.ru		*/
#ifndef _ETICK_ERR_MSG_H_
#define _ETICK_ERR_MSG_H_

#include <string>
#include <map>
#include "etick/lang.h"
#include "etick/etick_msg_types.h"

namespace Ticketing
{
    struct EtErr{
        REGERR(ProgErr);
        REGERR(WRONG_CC);
        REGERR(INV_COUPON_STATUS);
        REGERR(INV_ITIN_STATUS);
        REGERR(INV_RBD);
        REGERR(INV_DATE);
        REGERR(INV_TIME);
        REGERR(INV_TICK_ACT);
        REGERR(INV_CPN_ACT);
        REGERR(INV_FOP_ACT);
        REGERR(INV_TAX_AMOUNT);
        REGERR(MISS_MONETARY_INF);
        REGERR(INV_PASS_TYPE);
        REGERR(INV_IFT_QUALIFIER);
        REGERR(INV_FOID);
        REGERR(INV_COUPON_NUM);
        REGERR(ETS_INV_LUGGAGE);
    };
}// namespace Ticketing

#undef REGERR
#endif /*_ETICK_ERR_MSG_H_*/
