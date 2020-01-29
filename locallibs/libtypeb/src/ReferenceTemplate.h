#ifndef _REFTEMPLATE_H_
#define _REFTEMPLATE_H_
#include "typeb/typeb_template.h"
namespace typeb_parser
{
    class ReferenceTemplate : public tb_descr_nolexeme_element
    {
        public:
        virtual const char * name() const { return "Message reference"; }
        virtual const char * accessName() const { return "Msg ref"; }
        virtual bool isItYours(const std::string &, std::string::size_type *till) const;
        virtual TbElement * parse(const std::string &text) const; 
    };
} // namespace typeb_parser
#endif /*_REFTEMPLATE_H_*/
