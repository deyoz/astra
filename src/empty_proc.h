//---------------------------------------------------------------------------
#ifndef _EMPTY_PROC_H_
#define _EMPTY_PROC_H_

#ifndef __WIN32__
#include <tcl.h>
int main_empty_proc_tcl(Tcl_Interp *interp,int in,int out, Tcl_Obj *argslist);
#endif

#endif
