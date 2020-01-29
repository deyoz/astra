#include "AVA_template.h"
#include "typeb/AvaTemplate.h"
#include "typeb/tb_elements.h"
#include "typeb/typeb_template.h"
namespace typeb_parser {

AVA_template::AVA_template()
    :typeb_template("AVA", "Availability status message")
{
    addElement(new AvaTemplate)->setUnlimRep();
}
}// typeb_parser

