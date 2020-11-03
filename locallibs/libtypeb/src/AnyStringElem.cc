#include "AnyStringElem.h"
#include "typeb_msg.h"
#define NICKNAME "ASH"
#define NICKTRACE ASH_TRACE

namespace typeb_parser
{

AnyStringElem * AnyStringElem::parse(const std::string & text) 
{
    return new AnyStringElem(text);
}

std::ostream & operator << (std::ostream& os, const AnyStringElem &name)
{
    os << name.toString();
    return os;
}

} //namespace typeb_parser
