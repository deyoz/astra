#pragma once

#include "tcl_utils.h"

#include "slogger_nonick.h"

namespace tcl_utils {

template<typename T>
void emplace(std::vector<T>& c, Tcl_Obj* const obj, std::true_type)
{
    c.emplace_back(static_cast<T>(readIntFromTcl(obj)));
}

template<typename T>
void emplace(std::vector<T>& c, Tcl_Obj* const obj, std::false_type)
{
    c.emplace_back(readStringFromTcl(obj, nullptr));
}

template<typename T>
std::vector<T> readListFromTcl(const std::string& varname)
{
    std::vector<T> result;
    Tcl_Interp* interp = getTclInterpretator();
    TclObjHolder hld(Tcl_NewStringObj(varname.c_str(), -1));
    Tcl_Obj* const list = Tcl_ObjGetVar2(interp, hld.obj, nullptr, TCL_GLOBAL_ONLY);
    if (nullptr == list) {
        LogTrace1 << "Tcl variable " << varname << " not found";
        return result;
    }

    int objc;
    if (TCL_OK != Tcl_ListObjLength(interp, list, &objc)) {
        LogTrace1 << "Can't get length of list: " << Tcl_GetString(Tcl_GetObjResult(interp));
        return result;
    }

    for (int i = 0; i < objc; ++i) {
        Tcl_Obj* listObj = nullptr;
        Tcl_ListObjIndex(interp, list, i, &listObj);
        emplace(result, listObj, std::is_integral< std::decay_t< T > >());
    }

    return result;
}

template< typename T, typename = std::enable_if_t< std::is_integral<T>::value > >
std::vector<T> readListOfIntsFromTcl(const std::string& varname)
{
    return readListFromTcl<T>(varname);
}

std::vector<std::string> readListOfStringsFromTcl(const std::string& varname)
{
    return tcl_utils::readListFromTcl<std::string>(varname);
}

} // namespace tcl_utils
