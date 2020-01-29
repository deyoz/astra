/*
*  C++ Implementation: RemFoidElem
*
* Description: Ремарка FOID в TypeB телеграмме
*
*
* Author: Kovalev Roman <rom@sirena2000.ru>, (C) 2007
*
*/

#include "RemFoidElem.h"

namespace typeb_parser
{

Remark * RemFoidElem::parse(const std::string & code, const std::string & rem)
{
    return new RemFoidElem(rem);
}

} // namespace typeb_parser

