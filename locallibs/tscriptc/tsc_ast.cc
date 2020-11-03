#include "tsc_ast.h"
#include <algorithm>
#include <functional>
#include <iterator>
#include <cassert>
#include "string.h"

namespace ast {

    template<class NodeT, class ChildrenT> void WalkRef(NodeT* node, ChildrenT& children, Visitor& visitor)
    {
        visitor.preOrder(node);
        for (auto child = children.begin(); child != children.end(); ++child)
        {
            child->walk(visitor);

            const auto next = std::next(child);

            if (next != children.end())
                visitor.inOrder(node, &*child, &*next);
        }
        visitor.postOrder(node);
    }

    template<class NodeT, class ChildrenT>
    void Walk(NodeT* node, ChildrenT& children, Visitor& visitor)
    {
        visitor.preOrder(node);
        for (auto child = children.begin(); child != children.end(); ++child)
        {
            (*child)->walk(visitor);

            const auto next = std::next(child);

            if (next != children.end())
                visitor.inOrder(node, *child, *next);
        }
        visitor.postOrder(node);
    }

    LOC UnknownLoc()
    {
        static const std::string file(__FILE__);
        LOC loc;
        memset(&loc, 0, sizeof(loc));
        loc.first_file = loc.last_file = &file;
        loc.first_line = loc.last_line = __LINE__;
        loc.first_column = loc.last_column = 1;
        return loc;
    }

    /***********************************************************************
     * Node
     **********************************************************************/

    Node::Node()
    {
        Suite::getCurrentInstance()->nodes.push_back(this);
    }
    
    template<class T>
    static void Delete(const T* ptr) { delete ptr; }

    /***********************************************************************
     * PosTracker
     **********************************************************************/

    PosTracker::PosTracker(): line(1), col(1)
    {
        files_.push_back(new std::string);
    }

    PosTracker::~PosTracker()
    {
        std::for_each(files_.begin(), files_.end(), Delete<std::string>);
    }

    void PosTracker::setFile(const std::string& file)
    {
        if (file != *files_.back())
            files_.push_back(new std::string(file));
    }

    /***********************************************************************
     * Suite
     **********************************************************************/

    static Suite* _CurrentSuite;

    Suite::~Suite()
    {
        std::for_each(nodes.begin(), nodes.end(), Delete<Node>);
    }

    /* static */
    void Suite::setCurrentInstance(Suite* suite)
    {
        assert(suite == NULL || !_CurrentSuite);
        _CurrentSuite = suite;
    }

    /* static */
    Suite* Suite::getCurrentInstance()
    {
        assert(_CurrentSuite);
        return _CurrentSuite;
    }

    void Suite::walk(Visitor& visitor)
    {
        Walk(this, modules, visitor);
    }

    /***********************************************************************
     * Seq::Elem
     **********************************************************************/
    
    const LOC& Seq::Elem::loc() const
    {
        if      (str)   return str->loc;
        else if (quote) return quote->loc;
        else if (call)  return call->loc;
        else            throw std::runtime_error("Seq::Elem::loc");
    }

    Literal_ot Seq::Elem::toLiteral() const
    {
        if      (str)   return std::make_pair(true, str->text);
        else if (quote) return quote->toLiteral();
        else            return std::make_pair(false, std::string());
    }

    void Seq::Elem::walk(Visitor& visitor)
    {
        if      (str)   str->walk(visitor);
        else if (quote) quote->walk(visitor);
        else if (call)  call->walk(visitor);
        else            throw std::runtime_error("Seq::Elem::walk");
    }

    Seq::Elem Seq::Elem::clone() const
    {
        if      (str)   return Elem(str->clone());
        else if (quote) return Elem(quote->clone());
        else if (call)  return Elem(call->clone());
        else            throw std::runtime_error("Seq::Elem::clone");
    }

    /***************************************************************************
     * Seq
     **************************************************************************/

    Literal_ot Seq::toLiteral() const
    {
        Literal_ot literal = std::make_pair(true, std::string());

        for(Elem const& elem : elems)
        {
            const Literal_ot elemLiteral = elem.toLiteral();
            if (!elemLiteral.first)
                return std::make_pair(false, std::string());

            literal.second += elemLiteral.second;
        }

        return literal;
    }

    bool Seq::isEqual(const char* str) const
    {
        const Literal_ot literal = toLiteral();
        return literal.first && literal.second == str;
    }
    
    LOC Seq::loc() const
    {
        if (elems.empty())
            return UnknownLoc();

        LOC loc = elems.front().loc();
        loc.last_file = elems.back().loc().last_file;
        loc.last_line = elems.back().loc().last_line;
        loc.last_column = elems.back().loc().last_column;

        return loc;
    }

    void Seq::append(const LOC& loc, char c)
    {
        if (!elems.empty() && elems.back().str)
            elems.back().str->append(loc, c);
        else
            elems.emplace_back(new Str(loc, c));
    }
    
    void Seq::walk(Visitor& visitor)
    {
        WalkRef(this, elems, visitor);
    }

    Seq* Seq::clone() const
    {
        Seq* result = new ast::Seq;
        for(auto& e : elems)
            result->elems.push_back(e.clone());
        return result;
    }

    /***********************************************************************
     * Str
     **********************************************************************/

    void Str::walk(Visitor& visitor)
    {
        visitor.leaf(this);
    }
    
    void Str::append(const LOC& cloc, char c)
    {
        loc.last_file = cloc.last_file;
        loc.last_line = cloc.last_line;
        loc.last_column = cloc.last_column;
        text += c;
    }

    /***********************************************************************
     * Quote
     **********************************************************************/

    void Quote::walk(Visitor& visitor)
    {
        visitor.preOrder(this);
        seq->walk(visitor);
        visitor.postOrder(this);
    }

    /***********************************************************************
     * Call
     **********************************************************************/

    void Call::walk(Visitor& visitor)
    {
        visitor.preOrder(this);
        name->walk(visitor);
        for(Param* param : params)
        {
            visitor.inOrder(this);
            param->walk(visitor);
        }
        visitor.postOrder(this);
    }

    Call* Call::clone() const
    {
        std::vector<Param*> cloneParams(params.size());
        std::transform(params.begin(), params.end(), cloneParams.begin(),
                       [](Param const* c){ return c->clone(); });
        Call* call = new Call(isBlock, loc, name->clone(), cloneParams);
        call->macroExpandLocations = macroExpandLocations;
        return call;
    }

    /***********************************************************************
     * Param
     **********************************************************************/

    void Param::walk(Visitor& visitor)
    {
        visitor.preOrder(this);
        if (name) {
            name->walk(visitor);
            visitor.inOrder(this);
        }
        value->walk(visitor);
        visitor.postOrder(this);
    }
    
    Param* Param::clone() const
    {
        return new ast::Param(name ? name->clone() : NULL, value->clone());
    }

} /* namespace ast */

