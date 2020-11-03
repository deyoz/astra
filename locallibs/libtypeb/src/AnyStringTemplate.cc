#include "AnyStringTemplate.h"
#include "AnyStringElem.h"
#define NICKNAME "ASH"
#define NICKTRACE ASH_TRACE
#include <serverlib/slogger.h>

namespace typeb_parser
{

bool AnyStringTemplate::isItYours(const std::string &name_str, std::string::size_type *till) const
{
    for( const std::string & s:  this->matches ){
        if( name_str.substr(0,3) == s )
            return true;
    }
    return false;
}

TbElement * AnyStringTemplate::parse(const std::string & text) const
{
    return AnyStringElem::parse(text);
}

} // namespace typeb_parser
