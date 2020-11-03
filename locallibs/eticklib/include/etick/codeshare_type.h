//
// C++ Interface: CodeshareType
//
// Description: Code share types
//
//
// Author: Kovalev Roman <rom@sirena2000.ru>, (C) 2013
//
//
#ifndef _CODESHARE_TYPE_H_
#define _CODESHARE_TYPE_H_

#include <serverlib/base_code_set.h>


namespace Ticketing
{

DECLARE_CODE_SET_ELEM( CodeshareTypeElem )

};

DECLARE_CODE_SET( CodeshareType, CodeshareTypeElem )
    enum CodeshareType_e
    {
        FreeSale,        // FS
        BlockSpace,      // BS
    };
};

} // namespace Ticketing

#endif /*_CODESHARE_TYPE_H_*/
