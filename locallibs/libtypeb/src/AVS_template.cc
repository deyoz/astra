#include "AVS_template.h"
#include "typeb/AvsTemplate.h"
#include "typeb/tb_elements.h"
#include "typeb/typeb_template.h"
namespace typeb_parser {

AVS_template::AVS_template()
    :typeb_template("AVS", "Availability status message")
{
    addElement(new AvsTemplate)->setUnlimRep();
}
}// typeb_parser

