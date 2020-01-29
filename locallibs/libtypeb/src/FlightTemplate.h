#ifndef _FLIGHTTEMPLATE_H_
#define _FLIGHTTEMPLATE_H_
#include "typeb/typeb_template.h"
namespace typeb_parser
{
    class FlightTemplate :  public tb_descr_element{
        public:
            virtual const char * name() const { return "Flight element"; }
            virtual const char * lexeme() const { return ""; }
            virtual const char * accessName() const { return "Flight"; }
            virtual TbElement * parse(const std::string &text) const;
    };
} // namespace typeb_parser
#endif /*_FLIGHTTEMPLATE_H_*/
