//
// C++ Interface: TVL.7365 Processing indicator
//
// Description: edifact TVL.7365 code set
//
//
// Author: Kovalev Roman <rom@sirena2000.ru>, (C) 2010
//
//
#ifndef _EDI_PROC_IND_H_
#define _EDI_PROC_IND_H_

#include <serverlib/base_code_set.h>

namespace edilib
{

DECLARE_CODE_SET_ELEM(EdiProcIndElem)
};

DECLARE_CODE_SET(EdiProcInd, EdiProcIndElem)
// {
// dirty hack
    enum ProcInd_e
    {
        NoAction = 0,
        ActionRequired,
        ThisFlightToBeProc,
        // 1G RSS extension
        Secured, // most frequently used
        Guaranteed,
        SuperGuaranteed,
        JourneyAction,
    };
};

std::ostream  &operator << (std::ostream &s, const EdiProcInd &seq);


} /* namespace edilib */

#endif /*_EDI_PROC_IND_H_*/
