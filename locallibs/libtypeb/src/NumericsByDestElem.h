//
// C++ Interface: NumericsByDestElement
//
// Description: Numerics By Destination
//
//
// Author: Kovalev Roman <rom@sirena2000.ru>, (C) 2009
//
//
#ifndef _NUMERICSBYDESTELEMENT_H_
#define _NUMERICSBYDESTELEMENT_H_
#include <string>
#include <list>
#include "typeb_message.h"
#include "tb_elements.h"

namespace typeb_parser
{
    class NumericsByDestElement : public TbElement
    {
        std::string Airport;
        std::list<int> Nums1;
        std::list<int> Nums2;

        NumericsByDestElement(const std::string &Airp, std::list<int> ns, std::list<int> ns2)
            :Airport(Airp), Nums1(ns), Nums2(ns2)
        {
        }
        public:
            static NumericsByDestElement * parse (const std::string &nums_element);
            const std::string &airport() const { return Airport; }
            // before pad str
            const std::list<int> &nums1() const { return Nums1; }
            // after pad str
            const std::list<int> &nums2() const { return Nums2; }
    };



    std::ostream & operator << (std::ostream& os, const NumericsByDestElement &nums);

}
#endif /*_NUMERICSBYDESTELEMENT_H_*/
