#ifndef _ROZYSK_H_
#define _ROZYSK_H_
#include "basic.h"

namespace rozysk
{

void sync_pax(int pax_id, const std::string &term);
void sync_pax_grp(int grp_id, const std::string &term);
void sync_crs_pax(int pax_id, const std::string &term);
void sync_crs_pnr(int pnr_id, const std::string &term);

}

void sync_sirena_rozysk( BASIC::TDateTime utcdate );

#endif /*_ROZYSK_H_*/
