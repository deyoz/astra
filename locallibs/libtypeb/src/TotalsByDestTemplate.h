#ifndef _TOTALSBYDESTTEMPLATE_H_
#define _TOTALSBYDESTTEMPLATE_H_
#include "typeb/typeb_template.h"
namespace typeb_parser
{
    class TotalsByDestTemplate : public tb_descr_element {
        public:
            virtual const char * name() const { return "Totals by destination element"; }
            virtual const char * lexeme() const { return "-"; }
            virtual const char * accessName() const { return "Totals"; }
            virtual TbElement * parse(const std::string &text) const;
    };
} // namespace typeb_parser
#endif /*_TOTALSBYDESTTEMPLATE_H_*/
