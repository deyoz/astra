//
// C++ Interface: CategoryApElement
//
// Description: Category By Destination
//
//
// Author: Kovalev Roman <rom@sirena2000.ru>, (C) 2009
//
//
#ifndef _CATEGORYAPELEMENT_H_
#define _CATEGORYAPELEMENT_H_
#include <string>
#include <list>
#include "typeb_message.h"
#include "tb_elements.h"

namespace typeb_parser
{
    class CategoryApElement : public TbElement
    {
        std::string Airport;

        CategoryApElement(const std::string &Airp)
            :Airport(Airp)
        {
        }
        public:
            static CategoryApElement * parse (const std::string &text);
            const std::string &airport() const { return Airport; }
    };



    std::ostream & operator << (std::ostream& os, const CategoryApElement &nums);

}
#endif /*_CATEGORYAPELEMENT_H_*/
