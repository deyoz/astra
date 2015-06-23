#pragma once

#include <string>
#include "etick/etick_msg.h"

#define DEFERR(x) static const Ticketing::ErrMsg_t x
#define DEFMSG(x) static const Ticketing::EtsMsg_t x

#define ADDERR(s,x,e,r) const Ticketing::ErrMsg_t s::x=#s"::"#x;\
    namespace {\
    Ticketing::ErrMsgs x (s::x, e,r);\
}
#define ADDMSG(s,x,e,r) const Ticketing::AstraMsg_t s::x=#s"::"#x;\
    namespace {\
    Ticketing::ErrMsgs x (s::x, e,r);\
}


namespace Ticketing
{
    typedef ErrMsg_t AstraMsg_t;

    struct AstraErr {
        DEFERR(TIMEOUT_ON_HOST_3);
        DEFERR(EDI_PROC_ERR);
        DEFERR(EDI_INV_MESSAGE_F);
        DEFERR(EDI_NS_MESSAGE_F);
        DEFERR(EDI_SYNTAX_ERR);
        DEFERR(PAX_SURNAME_NF);
        DEFERR(INV_FLIGHT_DATE);
        DEFERR(TOO_MANY_PAX_WITH_SAME_SURNAME);
        DEFERR(FLIGHT_NOT_FOR_THROUGH_CHECK_IN);
        DEFERR(PAX_ALREADY_CHECKED_IN);
        DEFERR(BAGGAGE_WEIGHT_REQUIRED);
        DEFERR(NO_SEAT_SELCTN_ON_FLIGHT);
        DEFERR(TOO_MANY_PAXES);
        DEFERR(TOO_MANY_INFANTS);
        DEFERR(SMOKING_ZONE_UNAVAILABLE);
        DEFERR(NON_SMOKING_ZONE_UNAVAILABLE);
        DEFERR(PAX_SURNAME_NOT_CHECKED_IN);
        DEFERR(CHECK_IN_SEPARATELY);
        DEFERR(UPDATE_SEPARATELY);
        DEFERR(CASCADED_QUERY_TIMEOUT);
        DEFERR(ID_CARD_REQUIRED);
    };
}//namespace Ticketing

#undef DEFERR
#undef DEFMSG
