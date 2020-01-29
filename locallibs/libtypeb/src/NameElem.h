//
// C++ Interface: name_element
//
// Description: Елемент имен в typeB телеграмме
//
//
// Author: Kovalev Roman <rom@sirena2000.ru>, (C) 2006
//
//
#ifndef _NAMEELEM_H_
#define _NAMEELEM_H_
#include <string>
#include <list>
#include "typeb_message.h"
#include "tb_elements.h"

namespace typeb_parser
{
class NameElem : public TbElement
{
    std::list<std::string> Names;
    std::string SurName;
    unsigned Nseats;

    NameElem(unsigned nseats, std::string SName, std::list<std::string> ns)
    :Names(ns), SurName(SName),Nseats(nseats)
    {
    }
public:
    typedef std::list<std::string> Names_t;


    static NameElem * parse (const std::string &name_element);
    const Names_t &names() const { return Names; }
    unsigned nseats() const { return Nseats; }
    const std::string &surName() const { return SurName; }
    const std::string &passName() const{ return names().front(); }
};



std::ostream & operator << (std::ostream& os, const NameElem &name);

}
#endif /*_NAME_ELEM_H_*/
