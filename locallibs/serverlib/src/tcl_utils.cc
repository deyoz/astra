#if HAVE_CONFIG_H
#endif


#include <syslog.h>
#include <stdexcept>
#include <string.h>

#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/trim.hpp>

#include "exception.h"
#include "tcl_utils.h"
#include "str_utils.h"

#define NICKNAME "KONST"
#include "slogger.h"

using std::string;

static Tcl_Interp * Tcl_Interpretator = NULL;

Tcl_Interp * getTclInterpretator()
{
    return Tcl_Interpretator;
}
void setTclInterpretator(Tcl_Interp *inter)
{
    Tcl_Interpretator = inter;
}

int getConfigTclInt(char const*var1, char const*var2, int def)
{
    if(!Tcl_Interpretator) {
        fprintf(stderr, "trying to get tcl config before Tcl_Interpretator init. pid:%d\n", getpid());
        return def;
    }

    int val;
    Tcl_Obj *obj=Tcl_GetVar2Ex(Tcl_Interpretator, var1, var2, TCL_GLOBAL_ONLY);
    if(!obj)
    {
        return def;
    }
    if(TCL_OK!=Tcl_GetIntFromObj(Tcl_Interpretator, obj, &val))
    {
        return def;
    }
    return val;
}


int getVariableStaticBool(const char *var,int *flagv,int def)
{
#define ArrS(x)    (sizeof(x) / sizeof(x[0]))

    const char* TrueVS[]  = { "1", "y", "yes", "true", "on"  };
    const char* FalseVS[] = { "0", "n", "no", "false", "off" };

    int flag_ = -1;
    int *flag = (flagv) ? flagv : &flag_;

    const string defVal = (def ? TrueVS[0] : FalseVS[0]);

    if (*flag < 0) {
        string value = StrUtils::ToLower(readStringFromTcl(var, defVal));

        if ( std::find(TrueVS, TrueVS + ArrS(TrueVS), value) != TrueVS + ArrS(TrueVS) ) {
            return (*flag = 1);
        }

        if ( std::find(FalseVS, FalseVS + ArrS(FalseVS), value) != FalseVS + ArrS(FalseVS) ) {
            return (*flag = 0);
        }

        ProgTrace(TRACE0, "wrong setting %s=%s, set to %d", var, value.c_str(), def);
        return def; /*don't set flag*/
    }
    return *flag;
#undef ArrS
}
//#############################################################################
TclObjHolder::TclObjHolder(Tcl_Obj* o)
{
    if(!o){
        throw std::logic_error(std::string("Null Tcl_Obj pointer used").c_str());
    }
    obj = o;
    Tcl_IncrRefCount(obj);
}

TclObjHolder::~TclObjHolder()
{
    Tcl_DecrRefCount(obj);
}

static Tcl_Obj* getObject(const std::string& name)
{
    TclObjHolder hld( Tcl_NewStringObj(name.c_str(), -1) );

    return Tcl_ObjGetVar2(
            getTclInterpretator(),
                hld.obj, NULL, TCL_GLOBAL_ONLY );
}

std::string readStringFromTcl(Tcl_Obj* obj, const char* encoding)
{
    assert(obj != 0);
    Tcl_Encoding enc = NULL;

    Tcl_Interp* interp = getTclInterpretator();
    if (encoding) {
        enc = Tcl_GetEncoding(interp, encoding);
        if (enc == 0) {
            ProgError(STDLOG, "Tcl_GetEncoding :%s",
                    Tcl_GetString(Tcl_GetObjResult(interp)));
            Tcl_ResetResult(interp);
            sleep(10);
            abort();
        }
    }

    char* tmp = Tcl_GetString(obj);
    if (!tmp) {
        ProgTrace(TRACE0, "Tcl_GetString :%s",
                Tcl_GetString(Tcl_GetObjResult(interp)));
        sleep(10);
        abort();
    }
    // TODO check converted char count after Tcl_UtfToExternal
    Tcl_DString dstPtr;
    char* tmps = Tcl_UtfToExternalDString(enc, tmp, strlen(tmp), &dstPtr);
    if (!tmp) {
        ProgTrace(TRACE0, "Tcl_UtfToExternalDString :%s",
                Tcl_GetString(Tcl_GetObjResult(interp)));
        sleep(10);
        abort();
    }
    std::string value = tmps;
    Tcl_DStringFree(&dstPtr);
    if (enc) {
        Tcl_FreeEncoding(enc);
    }
    return value;
}

static int readString__(const string& varName, string& value)
{
    Tcl_Obj *obj = getObject(varName);
    if (!obj) {
        return -1;
    }
    TclObjHolder hld(obj);
    value = readStringFromTcl(hld.obj, NULL);
    return 0;
}

string readStringFromTcl(const string &varname)
{
    std::string val;
    if (readString__(varname, val) < 0) {
        throw std::logic_error(string("readStringFromTcl failed:'"+varname+"'").c_str());
    }
    return val;
}

string readStringFromTcl(const string &varname, const string& defaultValue)
{
    string val;
    if (readString__(varname, val) < 0) {
        return defaultValue;
    }
    return val;
}
//#############################################################################
static bool readLong__(std::string& strVal, long& val)
{
    boost::algorithm::trim(strVal);
    if (strVal.empty()) {
        return false;
    }
    const char* pval = strVal.c_str();
    char* end = NULL;
    val = strtol(pval, &end, 10);
    if (end && static_cast<size_t>(end - pval) != strVal.length()) {
        return false;
    }
    return true;
}

int readIntFromTcl(Tcl_Obj* obj)
{
    string strVal = readStringFromTcl(obj, NULL);
    if (strVal.empty()) {
        throw std::logic_error(string("CantReadTclVariable from obj"));
    }
    long val = 0;
    if (!readLong__(strVal, val))  {
        throw std::logic_error(string("CantReadTclVariable from obj"));
    }
    return static_cast<int>(val);
}

int readIntFromTcl(Tcl_Obj* obj, int defaultValue)
{
    string strVal = readStringFromTcl(obj, NULL);
    if (strVal.empty()) {
        return defaultValue;
    }
    long val = 0;
    if (!readLong__(strVal, val))  {
        return defaultValue;
    }
    return static_cast<int>(val);
}

int readIntFromTcl(const string& varname)
{
    string strVal = readStringFromTcl(varname, "");
    if (strVal.empty()) {
        throw std::logic_error(string("CantReadTclVariable :'" + varname + "'"));
    }
    long val = 0;
    if (!readLong__(strVal, val))  {
        throw std::logic_error(string("CantReadTclVariable :'" + varname + "'"));
    }
    return static_cast<int>(val);
}

int readIntFromTcl(const string& varname, int defaultValue)
{
    string strVal = readStringFromTcl(varname, "");
    if (strVal.empty()) {
        return defaultValue;
    }
    long val = defaultValue;
    if (!readLong__(strVal, val))  {
        return defaultValue;
    }
    return static_cast<int>(val);
}

bool readStringListFromTcl(std::vector<std::string>& oprList, const string& varname, const char* delims)
{
    string value;
    if ( readString__(varname, value) < 0 ) {
        return false;
    }

    boost::split(oprList, value, boost::algorithm::is_any_of(delims));
    return true;
}

bool readStringListFromTcl(std::vector<std::string>& oprList, const string& varname)
{
    return readStringListFromTcl(oprList, varname, ",./");
}

bool readStringListFromTcl_np(std::vector<std::string>& oprList, const string& varname)
{
    return readStringListFromTcl(oprList, varname, ",/");
}

void setTclVar(const string& varname, const string& value, const char* encoding)
{
    // assume there is no errors at all
    // use this for tests only
    // cheers
    Tcl_DString s;
    Tcl_DStringInit(&s);
    Tcl_ExternalToUtfDString(
            Tcl_GetEncoding(getTclInterpretator(), encoding),
            value.c_str(),
            value.size(),
            &s);
    Tcl_SetVar(getTclInterpretator(), varname.c_str(), Tcl_DStringValue(&s), TCL_GLOBAL_ONLY);
    Tcl_DStringFree(&s);
}
