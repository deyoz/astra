#ifndef _ROZYSK_H_
#define _ROZYSK_H_
#include "basic.h"

namespace rozysk
{

void sync_pax    (int pax_id, const std::string &term, const std::string &user_descr);
void sync_pax_grp(int grp_id, const std::string &term, const std::string &user_descr);
void sync_crs_pax(int pax_id, const std::string &term, const std::string &user_descr);
void sync_crs_pnr(int pnr_id, const std::string &term, const std::string &user_descr);

}

void sync_sirena_rozysk( BASIC::TDateTime utcdate );
void create_mintrans_file(int point_id);
void save_mintrans_files();

#endif /*_ROZYSK_H_*/
