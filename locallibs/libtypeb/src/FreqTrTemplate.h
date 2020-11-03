#ifndef _FREQTRTEMPLATE_H_
#define _FREQTRTEMPLATE_H_
#include "typeb/typeb_template.h"
namespace typeb_parser
{
    class FreqTrTemplate :  public tb_descr_element{
        public:
            virtual const char * name() const { return "Frequent Traveller element"; }
            virtual const char * lexeme() const { return ".F/"; }
            virtual const char * accessName() const { return "FreqTr"; }
            virtual TbElement * parse(const std::string &text) const;
    };
} // namespace typeb_parser
#endif /*_FREQTRTEMPLATE_H_*/
