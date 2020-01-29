#ifndef _RVRTEMPLATE_H_
#define _RVRTEMPLATE_H_
#include "typeb/typeb_template.h"
namespace typeb_parser
{
    class RvrTemplate : public tb_descr_nolexeme_element
    {
        public:
        virtual const char * name() const { return "Availability element"; }
        virtual const char * accessName() const { return "Availability"; }
        virtual bool isItYours(const std::string &, std::string::size_type *till) const;
        virtual TbElement * parse(const std::string &text) const; 
    };
} // namespace typeb_parser
#endif /*_RVRTEMPLATE_H_*/
