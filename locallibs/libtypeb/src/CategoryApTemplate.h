#ifndef _CATEGORYAPTEMPLATE_H_
#define _CATEGORYAPTEMPLATE_H_
#include "typeb/typeb_template.h"
namespace typeb_parser
{
    class CategoryApTemplate : public tb_descr_nolexeme_element
    {
        public:
            virtual const char * name() const { return "Category Airport Name Template"; }
            virtual bool isItYours(const std::string &, std::string::size_type *till) const;
            virtual const char * accessName() const { return "CategoryAp"; }
            virtual TbElement * parse(const std::string &text) const;
    };
} // namespace typeb_parser
#endif /*_CATEGORYAPTEMPLATE_H_*/
