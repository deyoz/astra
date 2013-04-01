#ifndef _AODB_H_
#define _AODB_H_

#include <string>
#include <map>
#include <tcl.h>
#include "astra_service.h"
#include "basic.h"
#include "astra_misc.h"

const std::string FILE_AODB_TYPE = "AODB";
const std::string FILE_AODB_IN_TYPE = "AODBI";
const std::string FILE_AODB_OUT_TYPE = "AODBO";

bool createAODBFiles( int point_id, const std::string &point_addr, TFileDatas &fds );
//bool createAODBCheckInInfoFile( int point_id, bool pr_unaccomp, const std::string &point_addr, TFileDatas &fds );
//bool BuildAODBTimes( int point_id, const std::string &point_addr, TFileDatas &fds );

void ParseAndSaveSPP( const std::string &filename, const std::string &canon_name,
                      const std::string &airline, const std::string &airp,
	                    std::string &fd, const std::string &convert_aodb );
bool Get_AODB_overload_alarm( int point_id, int max_commerce );
void Set_AODB_overload_alarm( int point_id, bool overload_alarm );

void VerifyParseFlight( );

int main_aodb_handler_tcl(Tcl_Interp *interp,int in,int out, Tcl_Obj *argslist);
//void update_aodb_pax_change( int point_id, int pax_id, int reg_no, const std::string &work_mode );
//bool is_sync_aodb( int point_id );
void bindingAODBFlt( const std::string &airline, const int flt_no, const std::string suffix,
                     const BASIC::TDateTime locale_scd_out, const std::string airp );
bool is_sync_aodb_pax( const TTripInfo &tripInfo );

#endif /*_AODB_H_*/
