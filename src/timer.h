//---------------------------------------------------------------------------
#ifndef _TIMER_H_
#define _TIMER_H_

#include "date_time.h"
#include "astra_service.h"
#include <tcl.h>
//#include <string>
#include <libxml/parser.h>

using BASIC::date_time::TDateTime;

int main_timer_tcl(int supervisorSocket, int argc, char *argv[]);
void ETCheckStatusFlt(void);
void utg(void);
void sync_mvd(void);
void createSPP( TDateTime utcdate );
void get_full_stat( TDateTime utcdate );
void sync_sirena_codes( void );
int CREATE_SPP_DAYS();

#endif
