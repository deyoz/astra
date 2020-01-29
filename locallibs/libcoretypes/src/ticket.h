#ifndef CORETYPES_TICKET_H
#define CORETYPES_TICKET_H

#include <string>

#include <serverlib/rip.h>
#include <serverlib/rip_validators.h>

#include <serverlib/encstring.h>

#define TICKET_SERIES_LEN 6
#define TICKET_NUMBER_LEN 15
#define ACCOUNT_CODE_LEN 3


namespace ct
{

bool isValidTicketSer(const EncString& str);
bool isValidTicketNum(const EncString& str);
bool isValidAccountCode(const EncString& str);

DECL_RIP2(TicketSer, EncString, ct::isValidTicketSer);
DECL_RIP2(TicketNum, EncString, ct::isValidTicketNum);
DECL_RIP2(AccountCode, EncString, ct::isValidAccountCode);
DECL_RIP_RANGED(Coupon, unsigned, 1, 4);

} // ct

#endif /* CORETYPES_TICKET_H */

