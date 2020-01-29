#ifndef __TCL_ARGS_H_
#define __TCL_ARGS_H_

Tcl_Obj* tcl_group(Tcl_Interp* interp, Tcl_Obj* argslist);
Tcl_Obj* obj_tcl_arg(Tcl_Interp* interp, Tcl_Obj* grp, const char* name, bool throw_if_notf = true);
const char* str_tcl_arg(Tcl_Interp* interp, Tcl_Obj* grp, const char* name, const char* nvl = NULL);
int int_tcl_arg(Tcl_Interp* interp, Tcl_Obj* grp, const char* name, const char* nvl = NULL);

#endif // __TCL_ARGS_H_
