/*
*  C++ Implementation: name_element
*
* Description: Елемент имен в typeB телеграмме
*
*
* Author: Kovalev Roman <rom@sirena2000.ru>, (C) 2006
*
*/
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/regex.hpp>
#include <boost/lexical_cast.hpp>

#include "NameElem.h"
#include "typeb_msg.h"
#include <serverlib/isdigit.h>
#define NICKNAME "ROMAN"
#define NICKTRACE ROMAN_TRACE
#include <serverlib/slogger.h>

namespace typeb_parser
{
using namespace std;
using namespace boost;

NameElem *NameElem::parse(const std::string & name_element)
{
    Names_t Names;
    split(Names, name_element, algorithm::is_any_of("/"));
    if(Names.empty())
    {
        throw typeb_parse_except(STDLOG, TBMsg::INV_NAME_ELEMENT,
                                 "Name element is empty!");
    }

    const string &surname = Names.front();
    const string::size_type nondigit_pos = surname.find_first_not_of("0123456789");

    unsigned nseats = 0;
    try
    {
        nseats = lexical_cast<unsigned>(surname.substr(0, nondigit_pos ));
    }
    catch(const boost::bad_lexical_cast &e)
    {
        throw typeb_parse_except(STDLOG, TBMsg::INV_NAME_ELEMENT,
                                 "Invalid seats number");
    }

    Names.front() = nondigit_pos == std::string::npos ? "" : surname.substr(nondigit_pos);

    for(Names_t::const_iterator i= Names.begin(); i!=Names.end(); i++){
        LogTrace(TRACE3) << "Name element: {" << *i << "}";
        if (!regex_match(*i, regex("^[A-ZА-ЯЁ ]{1,60}$"), match_any))
        {
            throw typeb_parse_except(STDLOG, TBMsg::WRONG_CHARS_IN_NAME);
        }
    }

    string SurName = Names.front();
    Names.pop_front();

    if(nseats != Names.size() && nseats>1)
    {
        throw typeb_parse_except(STDLOG, TBMsg::INV_NAME_ELEMENT,
                                 "Nseats and number of names mismatch");
    }

    return new NameElem(nseats,SurName, Names);
}

std::ostream & operator <<(std::ostream & os, const NameElem & name)
{
    os << name.nseats() << name.surName();
    for( NameElem::Names_t::const_iterator i=name.names().begin();
         i!=name.names().end(); i++)
    {
        os << "/" << (*i);
    }
    return os;
}

}
