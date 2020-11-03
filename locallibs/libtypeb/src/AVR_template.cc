#include "AVR_template.h"
#include "typeb/AvaTemplate.h"
#include "typeb/tb_elements.h"
#include "typeb/typeb_template.h"
namespace typeb_parser {

AVR_template::AVR_template()
    :typeb_template("AVR", "Availability status message")
{
    addElement(new AvaTemplate)->setUnlimRep();
    //yes, it's right - AvaTemplate, no mistype.
    //element of AVR is exactly the same as in AVA
}
}// typeb_parser

