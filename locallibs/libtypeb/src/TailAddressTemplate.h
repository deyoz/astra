#ifndef _TAILADDRESSTEMPLATE_H_
#define _TAILADDRESSTEMPLATE_H_
#include "typeb/typeb_template.h"
namespace typeb_parser
{
    class TailAddressTemplate : public tb_descr_nolexeme_element
    {
        public:
            virtual const char * name() const { return "Tail address element"; }
            virtual const char * accessName() const { return "Tail Dubai waltz element"; }
            virtual bool isItYours(const std::string &str, std::string::size_type *till) const
            {
                //MOWEU1H SVXRCU6
                if( str.length() >= 7 )
                    return true;
                else
                    return false;
            }

            virtual TbElement * parse(const std::string &text) const { return 0; }
    };
} // namespace typeb_parser
#endif /*_TAILADDRESSTEMPLATE_H_*/
