//---------------------------------------------------------------------------
#ifndef timerH
#define timerH

#ifndef __WIN32__
#include <tcl.h>
int main_timer_tcl(Tcl_Interp *interp,int in,int out, Tcl_Obj *argslist);
#endif
#include "basic.h"
void ETCheckStatusFlt(void);
void sync_mvd(void);
void createSPP( BASIC::TDateTime utcdate );
void exec_tasks( void );

#endif
