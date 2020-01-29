/*
 * Abstract syntax tree (AST) elements
 */

#ifndef __TSCRIPT_TREE
#define __TSCRIPT_TREE

#include <string>
#include <vector>
#include <stack>

namespace ast {
    class Visitor;

    /* for BISON location tracking (must be POD type) */
    struct LOC {
        const std::string* first_file;
        int first_line;
        int first_column;
        const std::string* last_file;
        int last_line;
        int last_column;
    };

    LOC UnknownLoc();

    /* ctor registers 'this' pointer in Suite::nodes */
    struct Node {
        Node();
        virtual ~Node() {}
    private:
        Node(const Node&);
        Node& operator=(const Node&);
    };

    struct Str;
    struct Seq;
    struct Quote;
    struct Call;
    struct Params;
    struct Param;

    struct Pos {
        std::string file;
        unsigned line;
        unsigned col;
        Pos(const std::string& file, unsigned line, unsigned col):
            file(file), line(line), col(col)
        {}
    };

    struct PosTracker {
        unsigned line;
        unsigned col;
        const std::string* file() const { return files_.back(); }

        PosTracker();
        ~PosTracker();
        
        Pos getPos() const { return Pos(*file(), line, col); }
        void setPos(const Pos& pos) { setFile(pos.file); line = pos.line; col = pos.col; }

        void setFile(const std::string& file);
    private:
        PosTracker(const PosTracker&) {}
        PosTracker& operator=(const PosTracker&) { return *this; }
        std::vector<const std::string*> files_;
    };

    /* root container */
    struct Suite {
        /* data */
        struct LexerState {
            PosTracker pos; /* tracked by lexer */
            std::stack<Pos> posBeforeLf;
            bool nosync = false;
        } lexer;
        std::vector<Seq*> modules;
        std::vector<const Node*> nodes;
        /* methods */
        Suite() = default;
        ~Suite();
        static void setCurrentInstance(Suite* suite);
        static Suite* getCurrentInstance();
        void walk(Visitor& visitor);
    private:
        Suite(const Suite&) = delete;
        Suite& operator=(const Suite&) = delete;
    };

    struct Str: public Node {
        /* data */
        LOC loc;
        std::string text;
        /* methods */
        explicit Str(const LOC& loc, char c): loc(loc), text(1, c) {}
        explicit Str(const LOC& loc, std::string s): loc(loc), text(std::move(s)) {}
        explicit Str(const LOC& loc, const char* s, size_t len): loc(loc), text(s, len) {}

        void append(const LOC& cloc, char c);

        void walk(Visitor& visitor);
        Str* clone() const { return new Str(loc, text); }
    };

    typedef std::pair<bool, std::string> Literal_ot;

    struct Seq: public Node {
        /* data */
        struct Elem {
            Str* str;
            Quote* quote;
            Call* call;
            std::vector<LOC> macroExpandLocations;

            explicit Elem(Str* str): str(str), quote(NULL), call(NULL) {}
            explicit Elem(Quote* quote): str(NULL), quote(quote), call(NULL) {}
            explicit Elem(Call* call): str(NULL), quote(NULL), call(call) {}
        
            const LOC& loc() const;
            Literal_ot toLiteral() const;
            void walk(Visitor& visitor);
            Elem clone() const;
        };
        std::vector<Elem> elems;
        /* methods */
        Seq() {}
        explicit Seq(const LOC& loc, char c): elems(1, Elem(new Str(loc, c))) {}
        explicit Seq(Str* str): elems(1, Elem(str)) {}
        explicit Seq(Quote* quote): elems(1, Elem(quote)) {}
        explicit Seq(Call* call): elems(1, Elem(call)) {}

        Literal_ot toLiteral() const;
        bool isEqual(const char* str) const;
        LOC loc() const;

        void append(const LOC& loc, char c); /* optimized */
        void append(Str* str) { elems.push_back(Elem(str)); }
        void append(Seq* seq) { elems.insert(elems.end(), seq->elems.begin(), seq->elems.end());  }
        void append(Quote* quote) { elems.push_back(Elem(quote)); }
        void append(Call* call) { elems.push_back(Elem(call)); }

        void walk(Visitor& visitor);
        Seq* clone() const;
    };

    struct Quote: public Node {
        /* data */
        ast::LOC loc;
        Seq* seq;
        /* methods */
        explicit Quote(const ast::LOC& loc, Seq* seq): loc(loc), seq(seq) {}
        Literal_ot toLiteral() const { return seq->toLiteral(); }
        void walk(Visitor& visitor);
        Quote* clone() const { return new ast::Quote(loc, seq->clone()); }
    };

    struct Params: public Node {
        std::vector<Param*> params;
    };

    struct Call: public Node {
        /* data */
        bool isBlock;
        LOC loc;
        Seq* name;
        std::vector<Param*> params;
        std::vector<LOC> macroExpandLocations;
        /* methods */
        Call(bool isBlock, const LOC& loc, Seq* name, Params* params):
            isBlock(isBlock), loc(loc), name(name), params(params->params) {}
        void walk(Visitor& visitor);
        Call* clone() const;
    private:
        Call(bool isBlock, const LOC& loc, Seq* name, const std::vector<Param*>& params):
            isBlock(isBlock), loc(loc), name(name), params(params) {}
    };

    struct Param: public Node {
        /* data */
        Seq* name; /* may be NULL */
        Seq* value;
        /* methods */
        Param(Seq* name, Seq* value): name(name), value(value) {}
        void walk(Visitor& visitor);
        Param* clone() const;
    };

    /***********************************************************************
     * Tree traversal support
     **********************************************************************/

    struct Visitor {
        virtual ~Visitor() {}

        /* Suite */
        virtual void preOrder(Suite* node) {}
        virtual void inOrder(Suite* node, Seq* prev, Seq* next) {}
        virtual void postOrder(Suite* node) {}

        /* Seq */
        virtual void preOrder(Seq* node) {}
        virtual void inOrder(Seq* node, Seq::Elem* prev, Seq::Elem* next) {}
        virtual void postOrder(Seq* node) {}

        /* Str (leaf node) */
        virtual void leaf(Str* node) {}

        /* Quote */
        virtual void preOrder(Quote* node) {}
        virtual void postOrder(Quote* node) {}

        /* Call */
        virtual void preOrder(Call* node) {}
        virtual void inOrder(Call* node) {}
        virtual void postOrder(Call* node) {}

        /* Param */
        virtual void preOrder(Param* node) {}
        virtual void inOrder(Param* node) {}
        virtual void postOrder(Param* node) {}
    };
    
} /* namespace ast */

#endif /* #ifndef __TSCRIPT_TREE */

