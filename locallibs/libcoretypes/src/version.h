#ifndef CORETYPES_VERSION_H
#define CORETYPES_VERSION_H

#include <serverlib/objid.h>
#include <serverlib/rip.h>

namespace ct
{

DECL_RIP_LENGTH(Version, std::string, OBJ_ID_LEN, OBJ_ID_LEN);

} // ct

#endif /* CORETYPES_VERSION_H */

