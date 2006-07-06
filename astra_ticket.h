#include "etick/ticket.h"

class OrigOfRequest : public Ticketing::BaseOrigOfRequest
{
public:
    OrigOfRequest()
        :BaseOrigOfRequest("1H","MOW","12345678","01MOW","MOW",'N',"ŒŽ‚‚‹€","1455")
    {
    }
};
