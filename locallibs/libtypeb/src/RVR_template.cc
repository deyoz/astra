#include "RVR_template.h"
#include "typeb/RvrTemplate.h"
#include "typeb/tb_elements.h"
#include "typeb/typeb_template.h"

namespace typeb_parser {

RVR_template::RVR_template()
    :typeb_template("RVR", "Recap request message")
{
    addElement(new RvrTemplate)->setUnlimRep();
}

} //namespace typeb_parser

