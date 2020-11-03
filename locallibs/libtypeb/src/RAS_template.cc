#include "RAS_template.h"
#include "typeb/RasTemplate.h"
#include "typeb/tb_elements.h"
#include "typeb/typeb_template.h"

namespace typeb_parser {

RAS_template::RAS_template()
    :typeb_template("RAS", "Request for Availability Status message")
{
    addElement(new RasTemplate)->setUnlimRep();
}

} //namespace typeb_parser
