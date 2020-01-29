#ifndef __RAS_TEMPLATE_ELEMENT_H__
#define __RAS_TEMPLATE_ELEMENT_H__

#include "typeb/typeb_template.h"

namespace typeb_parser {
    class RasTemplate : public tb_descr_nolexeme_element
    {
        public:
        virtual const char * name() const { return "Request for Availability element"; }
        virtual const char * accessName() const { return "AvailRequest"; }
        virtual bool isItYours(const std::string &, std::string::size_type *till) const;
        virtual TbElement * parse(const std::string &text) const;
    };
} //namespace typeb_parser

#endif //__RAS_TEMPLATE_ELEMENT_H__
