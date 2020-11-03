/*
*  C++ Implementation: FUNCTIONAL SERVICE ELEMENT DEFINITIONS
*
* Description: Functional Service Element
*
*
* Author: Kovalev Roman <rom@sirena2000.ru>, (C) 2011
*
*/
#include "edilib/fse_types.h"

const char *edilib::EdiFseElem::ElemName = "Functional Service Elements Set";

#define ADD_ELEMENT( El, C, D )\
    addElem( VTypes,  edilib::EdiFseElem( edilib::EdiFse::El, C,C, D,D));

DESCRIBE_CODE_SET( edilib::EdiFseElem ) {
    ADD_ELEMENT( Availability,         "AV1",  "Daily Fligh tAvailability" );
    ADD_ELEMENT( SeatMap,              "SM1",  "Seat Map" );
    ADD_ELEMENT( TravelerReservation1, "TR1",  "Traveler Reservation. Long Version"  );
    ADD_ELEMENT( TravelerReservation2, "TR2",  "Traveler Reservation. Short Version" );
    ADD_ELEMENT( ElectronicTicketing,  "ETK1", "Electronic Ticketing" );
    ADD_ELEMENT( PnrDispClaimSync,     "CSP1", "PNR Display, Claim, Service, and Synchronization" );
    ADD_ELEMENT( InformationalData1,   "INF1", "Informational data. The request will be one-off" );
    ADD_ELEMENT( InformationalData2,   "INF2", "Informational data. The request will be conversational" );
    ADD_ELEMENT( FreqTraveler1,        "FTV1", "Frequent Traveler Verification. The request will be one-off" );
    ADD_ELEMENT( FreqTraveler2,        "FTV2", "Frequent Traveler Verification. The request will be conversational" );
} // end of initialization

#undef ADD_ELEMENT
