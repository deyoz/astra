#ifndef _NAMETEMPLATE_H_
#define _NAMETEMPLATE_H_
#include "typeb/typeb_template.h"
namespace typeb_parser
{
    class NameTemplate : public tb_descr_nolexeme_element
    {
        public:
            virtual const char * name() const { return "Name element"; }
            virtual bool isItYours(const std::string &, std::string::size_type *till) const;
            virtual const char * accessName() const { return "Name"; }
            virtual TbElement * parse(const std::string &text) const;
    };

} // namespace typeb_parser
#endif /*_NAMETEMPLATE_H_*/
