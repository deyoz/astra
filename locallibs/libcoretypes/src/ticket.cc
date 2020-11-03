#include "ticket.h"

namespace ct
{

bool isValidTicketSer(const EncString& str)
{
    const EncString::base_type::size_type size = str.to866().size();
    return (0 < size) && (size < (TICKET_SERIES_LEN + 1));
}

bool isValidTicketNum(const EncString& str)
{
    const EncString::base_type::size_type size = str.to866().size();
    return (0 < size) && (size < (TICKET_NUMBER_LEN + 1));
}

bool isValidAccountCode(const EncString& str)
{
    const EncString::base_type::size_type size = str.to866().size();
    return (0 < size) && (size < (ACCOUNT_CODE_LEN + 1));
}

} // namespace ct
