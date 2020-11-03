#ifndef _CATEGORYTEMPLATE_H_
#define _CATEGORYTEMPLATE_H_
#include "typeb/typeb_template.h"
namespace typeb_parser
{
    class CategoryTemplate : public tb_descr_nolexeme_element
{
    public:
            virtual const char * name() const { return "Category Template"; }
            virtual bool isItYours(const std::string &, std::string::size_type *till) const;
            virtual const char * accessName() const { return "Category"; }
            virtual TbElement * parse(const std::string &text) const;
};
} // namespace typeb_parser
#endif /*_CATEGORYTEMPLATE_H_*/
