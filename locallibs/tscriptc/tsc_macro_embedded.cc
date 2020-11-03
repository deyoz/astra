/*
 * Embedded macro library
 */

#include "tsc_macro.h"
#include <sstream>
#include <stdexcept>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <sys/wait.h>

/*******************************************************************************
 * Utils
 ******************************************************************************/

static ast::LOC MakeLoc(unsigned line)
{
    static std::string file(__FILE__);
    ast::LOC loc;
    memset(&loc, 0, sizeof(loc));
    loc.first_file = loc.last_file = &file;
    loc.first_line = loc.last_line = line;
    loc.first_column = loc.last_column = 1;
    return loc;
}

/* from space-separated list */
static std::vector<MacroParam> MakeFormalParams(const std::string& str)
{
    std::vector<MacroParam> result;
    const ast::Seq* empty = new ast::Seq;

    std::string name;
    for (std::string::const_iterator c = str.begin(); c != str.end(); ++c)
        if (*c == ' ') {
            result.push_back(MacroParam(name, empty));
            name.clear();
        } else
            name += *c;
        
    if (!name.empty())
        result.push_back(MacroParam(name, empty));
    return result;
}

/*******************************************************************************
 * $(expr)
 ******************************************************************************/

/* execute shell command */
static std::string Shell(const std::string cmd, int* retStatus /* may be NULL */)
{
    FILE* shellPipe = NULL;
    try {
        FILE* shellPipe = popen(cmd.c_str(), "r");
        if (shellPipe == NULL)
            throw std::runtime_error("failed to open shellPipe");

        std::string result;
        for (;;) {
            const int c = fgetc(shellPipe);
            if (c == EOF)
                break;
            result += c;
        }

        const int status = pclose(shellPipe);
        if (status != 0 && retStatus == NULL) {
            shellPipe = NULL;
            std::ostringstream err;
            err << cmd << " exited with error status = " << status;
            throw std::runtime_error(err.str());
        }
        if (retStatus)
            *retStatus = status;

        return result;
    } catch (...) {
        if (shellPipe) pclose(shellPipe);
        throw;
    }
}

static ast::Seq* ExprMacro(const Macro& macro, const ActualParams& params)
{
    if (params.rest.empty())
        return NULL; /* do not expand */

    std::string cmd = "expr";
    for (std::vector<const ast::Seq*>::const_iterator val = params.rest.begin();
         val != params.rest.end();
         ++val)
    {
        const ast::Literal_ot literalVal = (*val)->toLiteral();
        if (!literalVal.first)
            return NULL;

        cmd += " '" + literalVal.second + "'";
    }

    int status = 0;
    std::string exprOut = Shell(cmd, &status);
    if (status != 0 && !(WIFEXITED(status) && WEXITSTATUS(status) == 1)) {
        std::ostringstream err;
        err << cmd << " exited with error status = " << status;
        throw std::runtime_error(err.str());
    }

    /* remove trailing whitespace */
    while (!exprOut.empty() && isspace(exprOut.at(exprOut.size() - 1)))
        exprOut.resize(exprOut.size() - 1);

    return new ast::Seq(new ast::Str(MakeLoc(__LINE__), exprOut));
}

/*******************************************************************************
 * Registration
 ******************************************************************************/

void RegisterEmbeddedMacros(MacroMap_t& macroMap)
{
    /* variadic macro */
    macroMap.insert(std::make_pair(
                "expr",
                Macro(MakeLoc(__LINE__), "expr", MakeFormalParams(""), ExprMacro, true)));
}

