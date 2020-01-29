#ifndef _ACTDEPTIMETEMPLATE_H_
#define _ACTDEPTIMETEMPLATE_H_
#include "typeb/typeb_template.h"
namespace typeb_parser
{
    class ActDepTimeTemplate : public tb_descr_element{
        public:
            virtual const char * name() const { return "Actual time of departure element"; }
            virtual const char * lexeme() const { return "ATD/"; }
            virtual const char * accessName() const { return "ActDepTime"; }
            virtual TbElement * parse(const std::string &text) const {return 0;}
    };
} // namespace typeb_parser
#endif /*_ACTDEPTIMETEMPLATE_H_*/
