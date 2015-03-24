//---------------------------------------------------------------------------
#ifndef _TIMER_H_
#define _TIMER_H_

#include "basic.h"
#include "astra_service.h"
#include <tcl.h>
//#include <string>
#include <libxml/parser.h>

int main_timer_tcl(int supervisorSocket, int argc, char *argv[]);
void ETCheckStatusFlt(void);
void utg(void);
void sync_mvd(void);
void createSPP( BASIC::TDateTime utcdate );
void get_full_stat( BASIC::TDateTime utcdate );
void sync_sirena_codes( void );
int CREATE_SPP_DAYS();

#endif
