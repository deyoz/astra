#ifndef _MARKETINGFLINFOTEMPLATE_H_
#define _MARKETINGFLINFOTEMPLATE_H_
#include "typeb/typeb_template.h"
namespace typeb_parser
{
    class MarketingFlInfoTemplate :  public tb_descr_element{
        public:
            virtual const char * name() const { return "Marketing Flight Information"; }
            virtual const char * lexeme() const { return ".M"; }
            virtual const char * accessName() const { return "MarkFlInfo"; }
            virtual TbElement * parse(const std::string &text) const;
    };

} // namespace typeb_parser
#endif /*_MARKETINGFLINFOTEMPLATE_H_*/
