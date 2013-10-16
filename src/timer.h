//---------------------------------------------------------------------------
#ifndef _TIMER_H_
#define _TIMER_H_

#include "basic.h"
#include <tcl.h>
//#include <string>
#include <libxml/parser.h>

int main_timer_tcl(Tcl_Interp *interp,int in,int out, Tcl_Obj *argslist);
void ETCheckStatusFlt(void);
void utg_prl(void);
void utg(void);
void sync_mvd(void);
void createSPP( BASIC::TDateTime utcdate );
void exec_tasks( const char *proc_name );
void get_full_stat( BASIC::TDateTime utcdate );
void sync_sirena_codes( void );
const int CREATE_SPP_DAYS();

#endif
