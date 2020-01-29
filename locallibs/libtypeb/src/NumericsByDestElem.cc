/*
*  C++ Implementation: NumericsByDestElement
*
* Description: Numerics By Destination
*
*
* Author: Kovalev Roman <rom@sirena2000.ru>, (C) 2009
*
*/
#include <list>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/regex.hpp>
#include <boost/lexical_cast.hpp>
#include <serverlib/str_utils.h>

#include "NumericsByDestElem.h"
#include "typeb_msg.h"
#define NICKNAME "ROMAN"
#define NICKTRACE ROMAN_TRACE
#include <serverlib/slogger.h>

namespace typeb_parser
{
using namespace std;
using namespace boost;

NumericsByDestElement *NumericsByDestElement::parse(const std::string & nums_element)
{
    if(nums_element.empty() || nums_element.length() < 4)
    {
        throw typeb_parse_except(STDLOG, TBMsg::INV_NUMS_BY_DEST_ELEM,
                                 "Numerics by destination element is empty!");
    }

    std::string Airport = nums_element.substr(0,3);
    if(!regex_match(Airport, regex("^([A-Z€-Ÿð]{3})$"), match_any))
    {
        throw typeb_parse_except(STDLOG, TBMsg::INV_NUMS_BY_DEST_ELEM, "Invalid data");
    }

    std::string numsstr = StrUtils::trim(nums_element.substr(3));

    std::string numsstr1;
    std::string numsstr2;

    size_t pos = numsstr.find("PAD");
    if(pos != std::string::npos)
    {
        numsstr1 = numsstr.substr(0, pos);
        numsstr2 = numsstr.substr(pos + 3);
    }
    else
    {
        numsstr1 = numsstr;
    }

    numsstr1 = StrUtils::delSpaces(numsstr1);
    numsstr2 = StrUtils::delSpaces(numsstr2);

    list<string> numslist1, numslist2;
    
    split(numslist1, numsstr1, algorithm::is_any_of("/"));
    split(numslist2, numsstr2, algorithm::is_any_of("/"));

    std::list<int> Nums1;
    std::list<int> Nums2;
    try
    {
        for(list<string>::const_iterator iter = numslist1.begin();
            iter != numslist1.end(); iter++)
        {
            if(iter->empty())
                continue;
            Nums1.push_back(boost::lexical_cast<unsigned>(*iter));
        }

        for(list<string>::const_iterator iter = numslist2.begin();
            iter != numslist2.end(); iter++)
        {
            if(iter->empty())
                continue;
            Nums2.push_back(boost::lexical_cast<unsigned>(*iter));
        }
    }
    catch(const bad_lexical_cast &e)
    {
        LogTrace(TRACE1) << "|" << numsstr1 << "|PAD|" << numsstr2 << "|: lexical_cast failed...somewhere";
        throw typeb_parse_except(STDLOG, TBMsg::INV_NUMS_BY_DEST_ELEM, "Invalid data");
    }
    return new NumericsByDestElement(Airport, Nums1, Nums2);
}

std::ostream & operator <<(std::ostream & os, const NumericsByDestElement & nums_element)
{
    os << nums_element.airport();
    for( std::list<int>::const_iterator i = nums_element.nums1().begin();
         i != nums_element.nums1().end(); i++)
    {
        os << "/" << (*i);
    }
    os << "|PAD|";
    for( std::list<int>::const_iterator i = nums_element.nums2().begin();
         i != nums_element.nums2().end(); i++)
    {
        os << "/" << (*i);
    }
    return os;
}

}
