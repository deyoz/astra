#ifndef _ENDTEMPLATE_H_
#define _ENDTEMPLATE_H_
#include "typeb/typeb_template.h"
namespace typeb_parser
{
    class EndTemplate : public tb_descr_element
    {
        public:
            virtual const char * name() const { return "End element"; }
            virtual const char * lexeme() const { return "END"; }
            virtual const char * accessName() const { return "TheEnd"; }
            virtual TbElement * parse(const std::string &text) const;
    };
} // namespace typeb_parser
#endif /*_ENDTEMPLATE_H_*/
