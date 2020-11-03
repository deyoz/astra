#ifndef _AVATEMPLATE_H_
#define _AVATEMPLATE_H_
#include "typeb/typeb_template.h"
namespace typeb_parser
{
    class AvaTemplate : public tb_descr_nolexeme_element
    {
        public:
        virtual const char * name() const { return "Availability element"; }
        virtual const char * accessName() const { return "Availability"; }
        virtual bool isItYours(const std::string &, std::string::size_type *till) const;
        virtual TbElement * parse(const std::string &text) const; 
    };
} // namespace typeb_parser
#endif /*_AVATEMPLATE_H_*/
