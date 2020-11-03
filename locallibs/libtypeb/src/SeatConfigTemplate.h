#ifndef _SEATCONFIGTEMPLATE_H_
#define _SEATCONFIGTEMPLATE_H_
#include "typeb/typeb_template.h"
namespace typeb_parser
{
    class SeatConfigTemplate : public tb_descr_element {
        public:
            virtual const char * name() const { return "Seat configuration element"; }
            virtual const char * lexeme() const { return "CFG/"; }
            virtual const char * accessName() const { return "SeatCfg"; }
            virtual TbElement * parse(const std::string &text) const;
    };
} // namespace typeb_parser
#endif /*_SEATCONFIGTEMPLATE_H_*/
