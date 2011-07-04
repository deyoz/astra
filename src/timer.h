//---------------------------------------------------------------------------
#ifndef _TIMER_H_
#define _TIMER_H_

#include "basic.h"
#include <tcl.h>
//#include <string>
#include <libxml/parser.h>

int main_timer_tcl(Tcl_Interp *interp,int in,int out, Tcl_Obj *argslist);
void ETCheckStatusFlt(void);
void sync_mvd(void);
void create_apis_file(int point_id);
void createSPP( BASIC::TDateTime utcdate );
void exec_tasks( const char *proc_name );
void get_full_stat( BASIC::TDateTime utcdate );
void sync_sirena_codes( void );
const int CREATE_SPP_DAYS();

void put_string_into_snapshot_points( int point_id, std::string file_type,
	                                    std::string point_addr, bool pr_old_record, std::string record );
void get_string_into_snapshot_points( int point_id, const std::string &file_type,
	                                    const std::string &point_addr, std::string &record );

#endif
