//---------------------------------------------------------------------------
#ifndef _EMPTY_PROC_H_
#define _EMPTY_PROC_H_

#ifndef __WIN32__
#include <tcl.h>
int main_empty_proc_tcl(Tcl_Interp *interp,int in,int out, Tcl_Obj *argslist);
#endif

int alter_db(int argc,char **argv);
bool alter_arx(void);
int alter_pax_doc(int argc,char **argv);
int alter_pax_doc2(int argc,char **argv);
int alter_arx_pax_doc(int argc,char **argv);
int alter_arx_pax_doc2(int argc,char **argv);

#endif
