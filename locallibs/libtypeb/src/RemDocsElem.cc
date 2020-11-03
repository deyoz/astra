/*
*  C++ Implementation: RemDocsElem
*
* Description: Ремарка DOCS TypeB телеграмме
*
*
* Author: Kovalev Roman <rom@sirena2000.ru>, (C) 2010
*
*/

#include "RemDocsElem.h"
#include "typeb_msg.h"
// #include ""

#define NICKNAME "ROMAN"
#define NICKTRACE ROMAN_TRACE
#include <serverlib/slogger.h>

namespace typeb_parser
{

Remark * RemDocsElem::parse(const std::string & code, const std::string & rem)
{
    tst();
    return new RemDocsElem(code, rem);
}

} // namespace typeb_parser

