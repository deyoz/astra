#ifndef SERVERLIB_OBJID_H
#define SERVERLIB_OBJID_H

#include <string>

#include "rip.h"
#include "rip_validators.h"

#define OBJ_ID_LEN 15

#define DECL_OBJ_ID( name ) \
    DECL_RIP_LENGTH( name, std::string, OBJ_ID_LEN, OBJ_ID_LEN )

namespace ct
{

DECL_RIP_LENGTH(CommandId, std::string, OBJ_ID_LEN, OBJ_ID_LEN);
DECL_RIP_LENGTH(UserId, std::string, OBJ_ID_LEN, OBJ_ID_LEN);
DECL_RIP_LENGTH(ObjectId, std::string, OBJ_ID_LEN, OBJ_ID_LEN);

} // ct

#endif /* SERVERLIB_OBJID_H */
