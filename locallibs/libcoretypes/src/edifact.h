#ifndef CORETYPES_EDIFACT_H
#define CORETYPES_EDIFACT_H

#include <string>
#include <serverlib/rip.h>
#include <serverlib/rip_validators.h>

namespace ct
{

DECL_RIP_LENGTH(Carf, std::string, 1, 17);

} // ct

#endif /* CORETYPES_EDIFACT_H */

