#ifndef _NILTEMPLATE_H_
#define _NILTEMPLATE_H_
#include "typeb/typeb_template.h"
namespace typeb_parser
{
    class NilTemplate : public tb_descr_element
    {
        public:
            virtual const char * name() const { return "Nil element"; }
            virtual const char * accessName() const { return "Nil"; }
            virtual const char * lexeme() const { return "NIL"; }
            virtual TbElement * parse(const std::string &text) const { return new TbElement; }
    };
} // namespace typeb_parser
#endif /*_NILTEMPLATE_H_*/
