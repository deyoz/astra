#include "tsc_ast.h"
#include <iostream>
#include <sstream>

static const char* RESET_COLOR = "\e[0m";
static const char* RED = "\e[31m";
static const char* YELLOW = "\e[33m";
static const char* BLUE = "\e[34m";
static const char* CYAN = "\e[36m";
static const char* LT_GREEN = "\e[1;32m";
static const char* LT_MAGENTA = "\e[1;35m";

struct GenSourceVisitor: public ast::Visitor {
    GenSourceVisitor(std::ostream& out, bool color): out(out), color_(color) {}

    /* Suite */
    virtual void inOrder(ast::Suite* node, ast::Seq* prev, ast::Seq* next)
    {
        out << color(LT_MAGENTA, "%%") << '\n';
    }

    /* Str (leaf node) */
    virtual void leaf(ast::Str* node)
    {
        for (char c : node->text)
            if (c == '{' || c == '}' || c == '"')
                out << color(RED, std::string("\\") + c);
            else
                out << c;
    }

    /* Quote */
    virtual void preOrder(ast::Quote* node) { out << color(CYAN, "{"); }
    virtual void postOrder(ast::Quote* node) { out << color(CYAN, "}"); }

    /* Call */
    virtual void preOrder(ast::Call* node)
    {
        /* block always begins at the new line, so its correct sync location */
        if (node->isBlock)
            printSync(node->loc);
        out << color(YELLOW, "$") << color(LT_GREEN, "(");
    }
    virtual void inOrder(ast::Call* node) { out << ' '; }
    virtual void postOrder(ast::Call* node)
    {
        out << color(LT_GREEN, ")");
        if (node->isBlock)
            out << '\n';
    }

    /* Param */
    virtual void inOrder(ast::Param* node) { out << color(YELLOW, "="); }
private:
    std::string color(const std::string& color, const std::string& s)
    {
        return color_ ?
            color + s + RESET_COLOR :
            s;
    }

    void printSync(const ast::LOC& loc)
    {
        std::ostringstream syn;
        syn << "#line "
            << loc.first_line
            << ' '
            << loc.first_column;
        if (loc.first_file)
            syn << " \"" << *loc.first_file << "\"";
        syn << '\n';
        out << color(BLUE, syn.str());
    }

    std::ostream& out;
    bool color_;
};

void GenSource(std::ostream& out, ast::Suite& suite, bool color)
{
    GenSourceVisitor visitor(out, color);
    suite.walk(visitor);
}


