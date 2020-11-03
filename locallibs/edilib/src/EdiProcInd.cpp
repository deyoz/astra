/*
*  C++ Implementation: 1050 Sequence number
*
* Description: edifact TVL.7365 code set
*
*
* Author: Kovalev Roman <rom@sirena2000.ru>, (C) 2010
*
*/
#include "EdiProcInd.h"

const char *edilib::EdiProcIndElem::ElemName = "Edi Processing Indicator";

#define ADD_EMEMENT(El, C, D)\
    addElem( VTypes,  EdiProcIndElem(edilib::EdiProcInd::El,   C,C, D,D));
DESCRIBE_CODE_SET(edilib::EdiProcIndElem)
{
    using namespace edilib;
    ADD_EMEMENT( NoAction,       "N", "No action required" );
    ADD_EMEMENT( ActionRequired, "P", "Action required" );
    ADD_EMEMENT( ThisFlightToBeProc, "TF", "This flight only to be processed" );

    ADD_EMEMENT( Secured,        "O", "Secured" );
    ADD_EMEMENT( Guaranteed,     "G", "Guaranteed" );
    ADD_EMEMENT( SuperGuaranteed,"S", "Super Guaranteed" );
    ADD_EMEMENT( JourneyAction,  "J", "Action based on journey")
}
