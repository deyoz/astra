#ifndef _SENTTEMPLATE_H_
#define _SENTTEMPLATE_H_
#include "typeb/typeb_template.h"
namespace typeb_parser
{
    class SentTemplate : public tb_descr_element
    {
        public:
            virtual const char * name() const { return "Sent element"; }
            virtual const char * lexeme() const { return "SENT"; }
            virtual const char * accessName() const { return "Dubai waltz element"; }
            virtual TbElement * parse(const std::string &text) const { return 0; }
    };
} // namespace typeb_parser
#endif /*_SENTTEMPLATE_H_*/
