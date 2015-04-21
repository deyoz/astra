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
    };
}//namespace Ticketing

#undef DEFERR
#undef DEFMSG
