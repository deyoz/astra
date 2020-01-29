#ifndef __TSC_MACRO_H
#define __TSC_MACRO_H

#include <map>
#include <algorithm>
#include "tsc_ast.h"

/* Formal or actual param */
struct MacroParam {
    std::string name;
    const ast::Seq* value;
    MacroParam(const std::string& name, const ast::Seq* value):
        name(name),
        value(value)
    {}

    template<class ContainerT> static bool findByName(const ContainerT& params, const std::string& name)
    {
        return std::any_of(params.begin(), params.end(), [&name](auto& p){ return p.name == name; });
    }
};

typedef std::map<std::string, const ast::Seq*> ParamMap_t;

struct ActualParams {
    ParamMap_t pmap;
    std::vector<const ast::Seq*> rest; /* for variadic macros */
};

struct Macro;
typedef ast::Seq* (*EmbeddedMacroProc_t)(const Macro& macro, const ActualParams& params);

struct Macro {
    ast::LOC loc;
    std::string name;
    std::vector<MacroParam> params;
    const ast::Seq* body; /* NULL for embedded macros */
    EmbeddedMacroProc_t embedded; /* NULL for user-defined macros */
    bool variadic;

    Macro(
            const ast::LOC& loc,
            const std::string& name,
            const std::vector<MacroParam>& params,
            const ast::Seq* body):
        loc(loc),
        name(name),
        params(params),
        body(body),
        embedded(NULL),
        variadic(false)
    {}

    Macro(
            const ast::LOC& loc,
            const std::string& name,
            const std::vector<MacroParam>& params,
            const EmbeddedMacroProc_t embedded,
            bool variadic):
        loc(loc),
        name(name),
        params(params),
        body(NULL),
        embedded(embedded),
        variadic(variadic)
    {}
};

typedef std::map<std::string, Macro> MacroMap_t;

void RegisterEmbeddedMacros(MacroMap_t& macroMap);

#endif
