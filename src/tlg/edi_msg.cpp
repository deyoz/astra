#include "edi_msg.h"

//#define REGERR(x,e,r) const ErrMsg_t EtsErr::x="EtsErr::"#x
#define REGERR(x,e,r) const Ticketing::ErrMsg_t EdiErr::x="EdiErr::"#x;\
    namespace {\
    Ticketing::ErrMsgs x (EdiErr::x, e,r);\
}

REGERR(EDI_PROC_ERR,
       "UNABLE TO PROCESS - SYSTEM ERROR",
       "…‚Ž‡ŒŽ†Ž Ž€Ž’€’œ - ‘ˆ‘’…Œ€Ÿ Ž˜ˆŠ€");

REGERR(EDI_INV_MESSAGE_F,
       "MESSAGE FUNCTION INVALID",
       "…‚…€Ÿ ”“Š–ˆŸ ‘ŽŽ™…ˆŸ");
REGERR(EDI_NS_MESSAGE_F,
       "MESSAGE FUNCTION NOT SUPPORTED",
       "”“Š–ˆŸ ‘ŽŽ™…ˆŸ … Ž„„…†ˆ‚€…’‘Ÿ");
REGERR(EDI_SYNTAX_ERR,
       "EDIFACT SYNTAX MESSAGE ERROR",
       "‘ˆ’€Š‘ˆ—…‘Š€Ÿ Ž˜ˆŠ€ ‚ ‘ŽŽ™…ˆˆ EDIFACT");

