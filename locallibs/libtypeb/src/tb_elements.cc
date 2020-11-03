/*
*  C++ Implementation: tb_elements
*
* Description: элементы typeb сообщения
*
*
* Author: Kovalev Roman <rom@sirena2000.ru>, (C) 2006
*
*/
#include <list>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/lexical_cast.hpp>
#include "tb_elements.h"
#include "typeb_template.h"
#include "typeb_parse_exceptions.h"
#include "typeb_cast.h"
#define NICKNAME "ROMAN"
#define NICKTRACE ROMAN_TRACE
#include <serverlib/slogger.h>

namespace typeb_parser
{
using namespace std;
using namespace boost;

std::string TbElement::name() const
{
    if( templ() )
    {
        return templ()->name();
    } else {
        return "Unknown element";
    }
}

unsigned TbElement::line() const
{
    return Line;
}

std::string::size_type TbElement::position() const
{
    return Position;
}

unsigned TbElement::length() const
{
    return Length;
}

inline void printPref(std::ostream& os, int level)
{
    for(int i=0;i<level;i++)
        os << "  ";
}

void TbElement::print(std::ostream& os, int level) const
{
    printPref(os, level);
    os << name() << " [" << templ()->accessName() << "]: ";
    if(elemType() == TbSimpleElem)
    {
        if(!lineSrc().empty()) {
            os << lineSrc();
        }
    }
    else if(elemType() == TbListElem)
    {
        elemAsList().print(os, level);
    }
    else
    {
        elemAsGroup().print(os, level);
    }
    os << "\n";
}

void TbListElement::print(std::ostream& os, int level) const
{
    size_t num = 0;
    level++;
    for(TbListElement::const_iterator iter = begin(); iter != end(); iter ++, num ++)
    {
        printPref(os, level);
        os << "[" << num << "] ";
        if((*iter)->elemType() == TbSimpleElem)
        {
            (*iter)->print(os, 0);
        }
        else if((*iter)->elemType() == TbListElem)
        {
            (*iter)->elemAsList().print(os, level);
        }
        else
        {
            (*iter)->elemAsGroup().print(os, level);
        }
    }
}

void TbGroupElement::print(std::ostream& os, int level) const
{
    size_t num = 0;
    level++;
    for(TbListElement::const_iterator iter = begin(); iter != end(); iter ++, num ++)
    {
        if((*iter)->elemType() == TbSimpleElem)
        {
            (*iter)->print(os, level);
        }
        else if((*iter)->elemType() == TbListElem)
        {
            (*iter)->elemAsList().print(os, level);
        }
        else
        {
            (*iter)->elemAsGroup().print(os, level);
        }
    }
}

std::string TbListElement::name() const
{
    static std::string s("List of ");
    if( templ() )
    {
        return s + templ()->name();
    } else
    {
        return s + "Unknown elements";
    }
}

std::string TbGroupElement::name() const
{
    static std::string s("Group of ");
    if( templ() )
    {
        return s + templ()->name();
    }
    else
    {
        return s + "Unknown elements";
    }
}


/// FIND functions
std::list< std::string > TbGroupElement::split_spath(const std::string & spath)
{
    list< string > lpath;
    boost::split(lpath, spath, boost::algorithm::is_any_of("/"));

    if(!lpath.empty() && lpath.back().empty())
        lpath.pop_back();

    if(lpath.empty())
    {
        throw typeb_except(STDLOG, "Empty element search path!");
    }
    return lpath;
}

class findelem
{
    string AccName;
public:
    findelem(const string &accName)
    :AccName(accName){}
    bool operator() (const TbElement::ptr_t &elem)
    {
/*        LogTrace(TRACE3) << "elem->templ()->accessName()={" <<
                elem->templ()->accessName() << "}";*/
        if(AccName == elem->templ()->accessName())
        {
            return true;
        }
        else
        {
            return false;
        }
    }
};

const TbElement * TbGroupElement::findp(std::list< std::string > &lpath) const
{
//     LogTrace(TRACE3) << "Curr search element: {" << lpath.front() << "}";
    if(lpath.front().empty())
    {
        lpath.pop_front();
        return findp(lpath);
    }
    TbListElement::const_iterator i = std::find_if( begin(), end(), findelem(lpath.front()));

    if(i == end())
    {
        return 0;
    } else {
        if(lpath.size()>1)
        {
            const TbGroupElement *gr = i->get()->elemAsGroupp();
            if(!gr)
            {
                return 0;
            }
            lpath.pop_front();
            return gr->findp(lpath);
        }
        else
        {
            return i->get();
        }
    }
}

const TbElement * TbGroupElement::findp(const std::string & path) const
{
    std::list< std::string > lpath = split_spath(path);
    return findp(lpath);
}

const TbElement & TbGroupElement::find(const std::string & path) const
{
    const TbElement *elem = findp(path);
    if(elem)
    {
        return *elem;
    }
    else
    {
        throw typeb_except(STDLOG,string("Element ")+path+" not found");
    }
}

const TbListElement * TbGroupElement::find_listp(const std::string & path) const
{
    const TbElement *elem = findp(path);
    if(elem)
    {
        return elem->elemAsListp();
    }
    else
    {
        return 0;
    }
}

const TbListElement & TbGroupElement::find_list(const std::string & path) const
{
    const TbElement *elem = findp(path);
    if(elem)
    {
        return elem->elemAsList();
    }
    else
    {
        throw typeb_except(STDLOG,string("Element ")+path+" not found");
    }
}

const TbGroupElement * TbGroupElement::find_grpp(const std::string & path) const
{
    const TbElement *elem = findp(path);
    if(elem)
    {
        return elem->elemAsGroupp();
    }
    else
    {
        return 0;
    }
}

const TbGroupElement & TbGroupElement::find_grp(const std::string & path) const
{
    const TbElement *elem = findp(path);
    if(elem)
    {
        return elem->elemAsGroup();
    }
    else
    {
        throw typeb_except(STDLOG,string("Element ")+path+" not found");
    }
}

const TbElement & TbListElement::front() const
{
    return *frontp();
}

const TbElement * TbListElement::frontp() const
{
    return Elements.front().get();
}

const TbListElement & TbListElement::front_list() const
{
    return frontp()->elemAsList();
}

const TbListElement * TbListElement::front_listp() const
{
    return front().elemAsListp();
}

const TbGroupElement & TbListElement::front_grp() const
{
    return front().elemAsGroup();
}

const TbGroupElement * TbListElement::front_grpp() const
{
    return front().elemAsGroupp();
}


const TbElement & TbListElement::back() const
{
    return *backp();
}
const TbElement * TbListElement::backp() const
{
    return Elements.back().get();
}

const TbListElement & TbListElement::back_list() const
{
    return back().elemAsList();
}

const TbListElement * TbListElement::back_listp() const
{
    return back().elemAsListp();
}

const TbGroupElement & TbListElement::back_grp() const
{
    return back().elemAsGroup();
}

const TbGroupElement * TbListElement::back_grpp() const
{
    return back().elemAsGroupp();
}


const TbElement & TbListElement::byNum(unsigned num) const
{
    return *byNump(num);
}

const TbElement * TbListElement::byNump(unsigned num) const
{
    if(num >= total())
    {
        throw typeb_except(STDLOG,
                           string("Range error! ") +
                                   lexical_cast<string>(num)+"requested, "+
                                   lexical_cast<string>(total())+ " total!");
    }

    if(num < total()/2)
    {
        TbListElement::const_iterator it = Elements.begin();
        std::advance (it, num);
        return it->get();
    }
    else
    {
        TbListElement::const_reverse_iterator it = Elements.rbegin();
        std::advance (it, total()-num-1);
        return it->get();
    }
}
const TbListElement & TbListElement::byNum_list(unsigned num) const
{
    return byNump(num)->elemAsList();
}
const TbListElement * TbListElement::byNum_listp(unsigned num) const
{
    return byNump(num)->elemAsListp();
}

const TbGroupElement & TbListElement::byNum_grp(unsigned num) const
{
    return byNump(num)->elemAsGroup();
}
const TbGroupElement * TbListElement::byNum_grpp(unsigned num) const
{
    return byNump(num)->elemAsGroupp();
}

const TbGroupElement * TbElement::elemAsGroupp() const
{
    return TbElem_cast<TbGroupElement> ( this );
}

const TbGroupElement & TbElement::elemAsGroup() const
{
    return TbElem_cast<TbGroupElement> ( *this );
}

const TbListElement * TbElement::elemAsListp() const
{
    return TbElem_cast<TbListElement> ( this );
}

const TbListElement & TbElement::elemAsList() const
{
    return TbElem_cast<TbListElement> ( *this );
}

}
