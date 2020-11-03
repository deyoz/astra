#include "tsc_ast.h"
#include <ctype.h>
#include <cassert>
#include <iostream>
#include <algorithm>
#include <functional>
#include <iterator>

struct Vm {
    struct Location {
        std::string first_file;
        unsigned first_line;
        unsigned first_col;
        std::string last_file;
        unsigned last_line;
        unsigned last_col;

        Location(
                const std::string& first_file,
                unsigned first_line,
                unsigned first_col,
                const std::string& last_file,
                unsigned last_line,
                unsigned last_col):
            first_file(first_file),
            first_line(first_line),
            first_col(first_col),
            last_file(last_file),
            last_line(last_line),
            last_col(last_col)
        {}
    };

    virtual ~Vm() {}

    virtual void begin() = 0;
    virtual void push(const std::string& val) = 0;
    /* name and parameters are on the stack */
    virtual void call(
            size_t numArgs,
            const Location& loc,
            const std::vector<Location>& macroExpandLocations) = 0;
    /* concatenate two strings on the stack, push back concatenated */
    virtual void concat() = 0;
};

static std::string Escape(const std::string& text)
{
    std::string result;
    result.reserve(text.size());
    for (std::string::const_iterator c = text.begin(); c != text.end(); ++c)
        if (*c == '\n')
            result += "\\n";
        else if (*c == '\\' || *c == '{' || *c == '}') {
            result += "\\";
            result += *c;
        } else
            result += *c;
    return result;
}

static void PrintLocation(std::ostream& out, const Vm::Location& loc)
{
    out << "\"" << loc.first_file << "\" " << loc.first_line << ' ' << loc.first_col;
    if (loc.last_file != loc.first_file)
        out << " \"" << loc.last_file << "\"";
    out << ' ' << loc.last_line << ' ' << loc.last_col;
}

struct CodegenVm: public Vm {
    CodegenVm(std::ostream& out): out(out) {}

    virtual void begin()
    {
        out << "BEGN\n";
    }

    virtual void push(const std::string& val)
    {
        out << "PUSH {" << Escape(val) << "}\n";
    }

    virtual void call(
            size_t numArgs,
            const Location& loc,
            const std::vector<Location>& macroExpandLocations)
    {
        out << "CALL " << numArgs << ' ';
        PrintLocation(out, loc);
        for (std::vector<Location>::const_iterator it = macroExpandLocations.begin();
             it != macroExpandLocations.end();
             ++it)
        {
            out << ' ';
            PrintLocation(out, *it);
        }
        out << '\n';
    }
    
    virtual void concat()
    {
        out << "CONC\n";
    }
private:
    std::ostream& out;
};

static Vm::Location ConvLocation(const ast::LOC& loc)
{
    return Vm::Location(
            loc.first_file ? *loc.first_file : std::string(),
            loc.first_line,
            loc.first_column,
            loc.last_file ? *loc.last_file : std::string(),
            loc.last_line,
            loc.last_column);
}

struct Interpretator: public ast::Visitor {
    Interpretator(Vm& vm): vm(vm) {}

    /* Suite */
    virtual void preOrder(ast::Suite* node)
    {
        if (!node->modules.empty())
            vm.begin();
    }
    
    virtual void inOrder(ast::Suite* node, ast::Seq* prev, ast::Seq* next)
    {
        vm.begin();
    }

    /* Seq */
    virtual void postOrder(ast::Seq* node)
    {
        for (size_t i = 1; i < node->elems.size(); ++i)
            vm.concat();
        if (node->elems.empty())
            vm.push(std::string());
    }

    /* Str (leaf node) */
    virtual void leaf(ast::Str* node)
    {
        vm.push(node->text);
    }

    /* Call */
    virtual void postOrder(ast::Call* node)
    {
        assert(node->loc.first_line > 0);
        assert(node->loc.first_column > 0);

        std::vector<Vm::Location> macroExpandLocations;
        std::transform(
                node->macroExpandLocations.begin(),
                node->macroExpandLocations.end(),
                std::back_inserter(macroExpandLocations),
                ConvLocation);

        vm.call(node->params.size(), ConvLocation(node->loc), macroExpandLocations);
    }

    /* Param */
    virtual void preOrder(ast::Param* node)
    {
        if (!node->name)
            vm.push(std::string());
    }
private:
    Vm& vm;
};

void GenVmCode(std::ostream& out, ast::Suite& suite)
{
    CodegenVm vm(out);
    Interpretator visitor(vm);
    suite.walk(visitor);
}

