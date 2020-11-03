#ifndef _ANYSTRELEM_H_
#define _ANYSTRELEM_H_
#include <string>
#include "typeb_message.h"
#include "tb_elements.h"

namespace typeb_parser
{

class AnyStringElem : public TbElement 
{
    std::string str_;
    AnyStringElem(const std::string & s) : str_(s) {}
public:
    static AnyStringElem * parse(const std::string & text);
    const std::string & toString() const { return str_; }
};

std::ostream & operator << (std::ostream& os, const AnyStringElem &name);
}
#endif /*_ANYSTRELEM_H_*/
