#include "astra_msg.h"

#define REGERR(x,e,r) ADDERR(AstraErr,x,e,r)
#define REGMSG(x,e,r) ADDMSG(AstraMsg,x,e,r)

namespace Ticketing
{

REGERR(TIMEOUT_ON_HOST_3,
       "Timeout occured on host 3",
       "’ ©¬ ãâ");
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

}//namespace Ticketing
