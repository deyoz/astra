#ifndef __MERIDIAN_H__
#define __MERIDIAN_H__
#include "xml_unit.h"
#include "jxtlib/JxtInterface.h"

namespace MERIDIAN {

bool is_sync_meridian( const TTripInfo &tripInfo );

void GetFlightInfo(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
void GetPaxsInfo(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);

} // namespace MERIDIAN

#endif // __MERIDIAN_H__

