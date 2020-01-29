/*
*  C++ Implementation: typeb_parse_exceptions
*
* Description: exceptions at time of parsing
*
*
* Author: Kovalev Roman <rom@sirena2000.ru>, (C) 2006
*
*/
#include <boost/format.hpp>
#include "typeb_parse_exceptions.h"

namespace typeb_parser
{
using boost::format;
using std::string;
using namespace Ticketing;

std::string typeb_parse_except::errText() const
{
    if(pLocalize)
        return pLocalize->str();
    else
        return "";
}

typeb_parse_except & typeb_parse_except::operator <<(const std::string & p)
{
    *pLocalize << p;
    return *this;
}

typeb_parse_except & typeb_parse_except::operator <<(const char * p)
{
    *pLocalize << p;
    return *this;
}

}

