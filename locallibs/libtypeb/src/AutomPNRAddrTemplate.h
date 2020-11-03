#ifndef _AUTOMPNRADDRTEMPLATE_H_
#define _AUTOMPNRADDRTEMPLATE_H_
#include "typeb/typeb_template.h"
#include "AutomPNRAddr.h"
namespace typeb_parser
{
    class AutomPNRAddrTemplate : public tb_descr_element {
        public:
            virtual const char * name() const { return "Automated PNR address element"; }
            virtual const char * lexeme() const { return ".L/"; }
            virtual const char * accessName() const { return "AutoPNR"; }
            virtual TbElement * parse(const std::string &text) const ;
    };
} // namespace typeb_parser
#endif /*_AUTOMPNRADDRTEMPLATE_H_*/
