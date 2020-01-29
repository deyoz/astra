/*
*  C++ Implementation: CategoryApElement
*
* Description: Category By Destination
*
*
* Author: Kovalev Roman <rom@sirena2000.ru>, (C) 2009
*
*/
#include "CategoryApElem.h"
#include "typeb_msg.h"
#define NICKNAME "ROMAN"
#define NICKTRACE ROMAN_TRACE
#include <serverlib/slogger.h>

namespace typeb_parser
{
using namespace std;

CategoryApElement *CategoryApElement::parse(const std::string & text)
{
    std::string Airport;

    if(text.length() != 4)
    {
        throw typeb_parse_except(STDLOG, TBMsg::INV_CATEGORY_AP_ELEM,
                                 "CategoryApElement element is invalid!");
    }

    return new CategoryApElement(text.substr(1));
}

std::ostream & operator <<(std::ostream & os, const CategoryApElement & cat)
{
    os << cat.airport();
    return os;
}

}
