/*	2006 by Roman Kovalev 	*/
/*	roman@pike.dev.sirena2000.ru		*/

#include "etick/etick_localization.h"
#include "etick/etick_msg.h"

namespace Ticketing
{

using boost::format;
using std::string;

const std::string EtickLocalization::Localize(std::string ErrCode, Language l)
{
    return ErrMsgs::Localize(ErrCode, l);
}

EtickLocalization::operator std::string &()
{
    if(pFrmt)
    {
        ErrText = boost::str( *pFrmt );
        pFrmt.reset();
    }
    return ErrText;
}

std::string EtickLocalization::str()
{
    return *this;
}

}
