#ifndef _ANYSTRTEMPLATE_H_
#define _ANYSTRTEMPLATE_H_
#include <vector>
#include <string>
#include "typeb/typeb_template.h"
namespace typeb_parser
{
    class AnyStringTemplate : public tb_descr_nolexeme_element
    {
        std::vector<std::string> matches;
        public:
        virtual const char * name() const { return "String element"; }
        virtual const char * accessName() const { return "Any string"; }
        virtual bool isItYours(const std::string &, std::string::size_type *till) const;
        virtual TbElement * parse(const std::string &text) const; 
        AnyStringTemplate(const std::vector<std::string> & v) : matches(v) {};
    };
} // namespace typeb_parser
#endif /*_ANYSTRTEMPLATE_H_*/
