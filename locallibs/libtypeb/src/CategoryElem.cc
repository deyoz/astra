/*
*  C++ Implementation: CategoryElement
*
* Description: Category NOSHO, GOSHO....
*
*
* Author: Kovalev Roman <rom@sirena2000.ru>, (C) 2009
*
*/
#include <boost/regex.hpp>
#include <boost/lexical_cast.hpp>
#include "CategoryElem.h"
#include "typeb_msg.h"
#define NICKNAME "ROMAN"
#define NICKTRACE ROMAN_TRACE
#include <serverlib/slogger.h>

namespace typeb_parser
{
using namespace std;
using namespace boost;

CategoryElement *CategoryElement::parse(const std::string & text)
{
    LogTrace(TRACE3) << "Start parsing CategoryElement";
    smatch what;
    if (!regex_match(text, what, regex("^([A-Z€-Ÿð]{5})\\s*(\\d{1,3})([A-ZA-Ÿð]{1})$")) || what.size() != 4)
    {
        throw typeb_parse_except(STDLOG, TBMsg::INV_CATEGORY_ELEM, "Invalid data");
    }


    LogTrace(TRACE3) << "Done parsing CategoryElement";
    return new CategoryElement(what[1],
                               boost::lexical_cast<unsigned>(what[2]),
                               what[3].str()[0]);
}

std::ostream & operator <<(std::ostream & os, const CategoryElement & cat)
{
    os << cat.catName() << ":" << cat.num() << ":" << cat.subClsCode();
    return os;
}

}
