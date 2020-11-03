#ifndef __TCL_UTILS__
#define __TCL_UTILS__

#include <string>
#include <list>
#include <vector>
#include <tcl.h>

Tcl_Interp * getTclInterpretator();
void setTclInterpretator(Tcl_Interp *);
int getConfigTclInt(char const*var1, char const*var2, int def);

int getVariableStaticBool(const char *var,int *flag,int def) ;


/**
 * Performs operations necessary for correct memory cleanup after Tcl_Obj
 **/
struct TclObjHolder
{
    Tcl_Obj * obj;
    TclObjHolder(Tcl_Obj *);
    ~TclObjHolder();
};

/**
 * Reads int from tcl variable. If there is no such variable - throws.
 * */
int readIntFromTcl(const std::string &varname);
int readIntFromTcl(Tcl_Obj*);
/**
 * Reads int from tcl variable. If there is no such variable - returns defaultValue.
 * */
int readIntFromTcl(const std::string &varname, int defaultValue);
int readIntFromTcl(Tcl_Obj*, int defaultValue);
/**
 * Reads string from tcl variable. If there is no such variable - throws.
 * */
std::string readStringFromTcl(const std::string &varname);
std::string readStringFromTcl(Tcl_Obj*, const char* encoding);
/**
 * Reads string from tcl variable. If there is no such variable - returns defaultValue.
 * */
std::string readStringFromTcl(const std::string &varname, const std::string& defaultValue);

bool readStringListFromTcl(std::vector<std::string>& oprList, const std::string& varname, const char* delims);
bool readStringListFromTcl(std::vector<std::string>& oprList, const std::string& varname);
bool readStringListFromTcl_np(std::vector<std::string>& oprList, const std::string& varname);

/**
 * Sets Tcl variable with value
 * WARNING: use this only for testing, e.g. inTestMode, no error checks inside
 * */
void setTclVar(const std::string& varname, const std::string& value, const char* encoding = "cp866");

#endif //__TCL_UTILS__
