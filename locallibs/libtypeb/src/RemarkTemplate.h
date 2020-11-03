#ifndef _REMARKTEMPLATE_H_
#define _REMARKTEMPLATE_H_
#include "typeb/typeb_template.h"

namespace typeb_parser
{
    class RemarkTemplate : public tb_descr_multiline_element{
        public:
            virtual const char * name() const { return "Remark"; }
            virtual const char * lexeme() const { return ".R/"; }
            virtual const char * nextline_lexeme() const { return ".RN/"; }
            virtual const char * accessName() const { return "Remark"; }
            virtual TbElement * parse(const std::string &text) const;
    };

} // namespace typeb_parser
#endif /*_REMARKTEMPLATE_H_*/
