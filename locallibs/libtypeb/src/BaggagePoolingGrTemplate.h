#ifndef _BAGGAGEPOOLINGGRTEMPLATE_H_
#define _BAGGAGEPOOLINGGRTEMPLATE_H_
#include "typeb/typeb_template.h"
namespace typeb_parser
{
    class BaggagePoolingGrTemplate : public tb_descr_element {
        public:
            virtual const char * name() const { return "Baggage pooling group indication element"; }
            virtual const char * lexeme() const { return ".BG/"; }
            virtual const char * accessName() const { return "Baggage"; }
            virtual TbElement * parse(const std::string &text) const {return 0;}
    };
} // namespace typeb_parser
#endif /*_BAGGAGEPOOLINGGRTEMPLATE_H_*/
