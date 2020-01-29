/*
*  C++ Implementation: CodeshareType
*
* Description: Code share types
*
*
* Author: Kovalev Roman <rom@sirena2000.ru>, (C) 2013
*
*/
#include <etick/codeshare_type.h>

const char *Ticketing::CodeshareTypeElem::ElemName = "Codeshare type";

#define ADD_ELEMENT( El, C, D )\
    addElem( VTypes,  Ticketing::CodeshareTypeElem( Ticketing::CodeshareType::El, C,C, D,D));

DESCRIBE_CODE_SET( Ticketing::CodeshareTypeElem ) {
    ADD_ELEMENT( FreeSale,         "FS",  "Free sale" );
    ADD_ELEMENT( BlockSpace,       "BS",  "Block space" );
} // end of initialization

#undef ADD_ELEMENT
