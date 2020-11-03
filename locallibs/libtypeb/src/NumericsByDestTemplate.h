#ifndef _NUMERICBYDESTTEMPLATE_H_
#define _NUMERICBYDESTTEMPLATE_H_
#include "typeb/typeb_template.h"
namespace typeb_parser
{
    class NumericsByDestTemplate : public tb_descr_nolexeme_multiline_element
    {
        public:
            virtual const char * name() const { return "Numeric By Destination"; }
            virtual bool isItYours(const std::string &, std::string::size_type *till) const;
            virtual bool check_nextline(const std::string &, const std::string &, size_t *start, size_t *till) const;
            virtual const char * accessName() const { return "NumsByDest"; }
            virtual TbElement * parse(const std::string &text) const;
    };
} // namespace typeb_parser
#endif /*_NUMERICBYDESTTEMPLATE_H_*/
