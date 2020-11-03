//
// C++ Interface: FUNCTIONAL SERVICE ELEMENT DEFINITIONS
//
// Description: Functional Service Element
//
//
// Author: Kovalev Roman <rom@sirena2000.ru>, (C) 2011
//
//
#ifndef _FSE_TYPES_H_
#define _FSE_TYPES_H_

#include <serverlib/base_code_set.h>


namespace edilib
{

DECLARE_CODE_SET_ELEM( EdiFseElem )

};

DECLARE_CODE_SET( EdiFse, EdiFseElem )
    enum EdiFse_e
    {
        Availability,        // AV1
        SeatMap,             // SM1
        TravelerReservation1,// TR1
        TravelerReservation2,// TR2
        ElectronicTicketing, // ETK1
        PnrDispClaimSync,    // CSP1
        InformationalData1,  // INF1
        InformationalData2,  // INF2,
        FreqTraveler1,       // FTV1
        FreqTraveler2        // FTV2
    };
};

} // namespace edilib

#endif /*_FSE_TYPES_H_*/
