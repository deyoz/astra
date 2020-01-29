#ifndef _BAGGAGETAGDETAILSTEMPLATE_H_
#define _BAGGAGETAGDETAILSTEMPLATE_H_
#include "typeb/typeb_template.h"
namespace typeb_parser
{
    class BaggageTagDetailsTemplate : public tb_descr_element {
        public:
            virtual const char * name() const { return "Baggage tag details element"; }
            virtual const char * lexeme() const { return ".N/"; }
            virtual const char * accessName() const { return "BaggageDetails"; }
            virtual TbElement * parse(const std::string &text) const {return 0;}
    };
} // namespace typeb_parser
#endif /*_BAGGAGETAGDETAILSTEMPLATE_H_*/
