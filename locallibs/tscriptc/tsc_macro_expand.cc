#include "tsc_macro.h"
#include <sstream>
#include <iostream>
#include <stdexcept>
#include <algorithm>
#include <cassert>
#include <ctype.h>

/*******************************************************************************
 * Error reporting
 ******************************************************************************/

static void ExceptionAtLocation(const ast::LOC& loc, const std::runtime_error& e)
{
    std::ostringstream msg;
    msg << e.what()
        << "\n at " << (loc.first_file  ? *loc.first_file + ":" : std::string())
        << " line " << loc.first_line << ", col " << loc.first_column;
    if (loc.last_file && loc.first_file && *loc.last_file != *loc.first_file) {
        msg << " - " << *loc.last_file + ":"
            << " line " << loc.last_line << ", col " << loc.last_column;
    } else {
        if (loc.last_line == loc.first_line)
            msg << "-" << loc.last_column;
        else
            msg << " - line " << loc.last_line << ", col " << loc.last_column;
    }
    throw std::runtime_error(msg.str());
}

/*******************************************************************************
 * macro expand
 ******************************************************************************/

static size_t ReplaceSeqElem(ast::Seq* seq, size_t i, ast::Seq* replacement)
{
    assert(i < seq->elems.size());
    seq->elems.erase(seq->elems.begin() + i);
    seq->elems.insert(
            seq->elems.begin() + i,
            replacement->elems.begin(),
            replacement->elems.end());
    return replacement->elems.size();
}

struct MacroEvalVisitor: public ast::Visitor {
    ActualParams params; /* actual parameters */

    /* Seq */
    virtual void postOrder(ast::Seq* node)
    {
        for (size_t i = 0; i < node->elems.size(); ++i) {
            ast::Seq::Elem* elem = &node->elems[i];
            if (!elem->call || !elem->call->params.empty())
                continue;

            const ast::Literal_ot literalName = elem->call->name->toLiteral();
            if (!literalName.first)
                continue;

            ParamMap_t::const_iterator param = params.pmap.find(literalName.second);
            if (param != params.pmap.end())
                /* replace macro param by its value in the sequence */
                i += ReplaceSeqElem(node, i, param->second->clone()) - 1;
        }
    }
};

static void SetPositionalParam(ActualParams& actualParams, const Macro& macro, const ast::Seq* value)
{
    for (std::vector<MacroParam>::const_iterator macroParam = macro.params.begin();
         macroParam != macro.params.end();
         ++macroParam)
    {
        if (!actualParams.pmap.count(macroParam->name)) {
            actualParams.pmap.insert(std::make_pair(macroParam->name, value));
            return;
        }
    }
    
    if (macro.variadic)
        actualParams.rest.push_back(value);
    else
        throw std::runtime_error("too many positional macro parameters passed");
}

static ActualParams GetMacroParams(const Macro& macro, const std::vector<ast::Param*>& passed)
{
    ActualParams actualParams;

    for (std::vector<ast::Param*>::const_iterator actualParam = passed.begin();
         actualParam != passed.end();
         ++actualParam)
    {
        if ((*actualParam)->name) {
            const ast::Literal_ot literalName = (*actualParam)->name->toLiteral();
            if (!literalName.first)
                throw std::runtime_error("name of macro parameter must be string literal (after expansion)");

            const std::string name = literalName.second;
            if (!MacroParam::findByName(macro.params, name))
                throw std::runtime_error("no such macro parameter with name " + name);
            if (actualParams.pmap.count(name))
                throw std::runtime_error("macro parameter " + name + " assigned more than once");
            actualParams.pmap[name] = (*actualParam)->value;
        } else {
            SetPositionalParam(actualParams, macro, (*actualParam)->value);
        }
    }

    /* default values */
    for (std::vector<MacroParam>::const_iterator macroParam = macro.params.begin();
         macroParam != macro.params.end();
         ++macroParam)
    {
        /* will not overwrite already assigned values */
        actualParams.pmap.insert(std::make_pair(macroParam->name, macroParam->value));
    }
    
    return actualParams;
}

struct UpdateMacroExpandLocationsVisitor: public ast::Visitor {
    UpdateMacroExpandLocationsVisitor(const ast::LOC& expandLoc):
        expandLoc_(expandLoc)
    {}

    /* Seq */
    virtual void postOrder(ast::Seq* node)
    {
        for (std::vector<ast::Seq::Elem>::iterator elem = node->elems.begin();
             elem != node->elems.end();
             ++elem)
        {
            elem->macroExpandLocations.push_back(expandLoc_);
        }
    }

    /* Call */
    virtual void postOrder(ast::Call* node)
    {
        node->macroExpandLocations.push_back(expandLoc_);
    }
private:
    ast::LOC expandLoc_;
};

/* may return NULL */
static ast::Seq* ExpandMacro(
        const ast::LOC& loc,
        const Macro& macro,
        const std::vector<ast::Param*>& passed)
{
    ast::Seq* result = NULL;
    if (macro.body) {
        /* user-defined macro */
        MacroEvalVisitor eval;
        eval.params = GetMacroParams(macro, passed);
        result = macro.body->clone();
        result->walk(eval);
        /* unquote */
        if (result->elems.size() == 1 && result->elems.front().quote)
            result = result->elems.front().quote->seq;
    } else
        /* embedded macro */
        result = macro.embedded(macro, GetMacroParams(macro, passed));

    if (result) {
        UpdateMacroExpandLocationsVisitor updateMacroExpandLocations(loc);
        result->walk(updateMacroExpandLocations);
    }

    return result;
}

static bool IsFalseValue(const std::string& val)
{
    return val.empty() || val == "0";
}

static bool CheckStaticCondition(const ast::Call* call, ast::Seq*& result)
{
    /*
     * $(if cond then else)
     * OR
     * $(if cond then)
     */
    if (call &&
        call->name->isEqual("if") &&
        (call->params.size() == 2 || call->params.size() == 3) &&
        call->params.at(0)->name == NULL &&
        call->params.at(1)->name == NULL &&
        (call->params.size() == 2 || call->params.at(2)->name == NULL))
    {
        const ast::Literal_ot literalCondition = call->params.at(0)->value->toLiteral();
        if (literalCondition.first) {
            const bool isFalse = IsFalseValue(literalCondition.second);
            result = isFalse ?
                (call->params.size() == 3 ? call->params.at(2)->value : new ast::Seq) :
                call->params.at(1)->value;
            return true;
        }
    }
    return false;
}

static size_t EvaluateStaticConditionals(const MacroMap_t& macroMap, ast::Seq* seq)
{
    size_t count = 0;
    for (size_t i = 0; i < seq->elems.size(); ++i) {
        ast::Seq* conditionResult;
        if (CheckStaticCondition(seq->elems[i].call, conditionResult)) {
            i += ReplaceSeqElem(seq, i, conditionResult) - 1;
            ++count;
        }
    }
    return count;
}

static size_t ExpandSeq(const MacroMap_t& macroMap, ast::Seq* seq)
{
    size_t count = 0;
    for (size_t i = 0; i < seq->elems.size(); ++i) {
        ast::Seq::Elem* elem = &seq->elems[i];

        if (!elem->call)
            continue;
        const ast::Literal_ot literalName = elem->call->name->toLiteral();
        if (!literalName.first)
            continue;

        try {
            /* lookup macro by name */
            MacroMap_t::const_iterator macro = macroMap.find(literalName.second);
            if (macro != macroMap.end()) {
                /* replace macro call by expansion */
                ast::Seq* expansion = ExpandMacro(elem->call->loc, macro->second, elem->call->params);
                if (expansion) {
                    ++count;
                    i += ReplaceSeqElem(seq, i, expansion) - 1;
                }
            }
        } catch (const std::runtime_error& e) {
            ExceptionAtLocation(elem->call->loc, e);
        }
    }
    return count;
}

struct MacroExpandVisitor: public ast::Visitor {
    size_t count;

    MacroExpandVisitor(const MacroMap_t& macroMap):
        count(0),
        macroMap_(macroMap)
    {}

    /* Seq */
    virtual void preOrder(ast::Seq* node)
    {
        count += EvaluateStaticConditionals(macroMap_, node);
    }

    virtual void postOrder(ast::Seq* node)
    {
        count += ExpandSeq(macroMap_, node);
    }
private:
    const MacroMap_t& macroMap_;
};

/*******************************************************************************
 * macro define
 ******************************************************************************/

static bool IsMacroDefine(const ast::Call* call)
{
    return call->name->isEqual("defmacro");
}

static std::string GetMacroName(const ast::Param& param)
{
    if (param.name)
        throw std::runtime_error("macro name must be unnamed parameter");

    const ast::Literal_ot literal = param.value->toLiteral();
    if (!literal.first)
        throw std::runtime_error("macro name must be string literal (after expansion)");

    return literal.second;
}

static std::string GetMacroParamName(const ast::Param& param)
{
    const ast::Literal_ot literal = param.name ? param.name->toLiteral() : param.value->toLiteral();
    if (!literal.first)
        throw std::runtime_error("macro parameter name must be string literal (after expansion)");
    return literal.second;
}

static void DefineMacro(
        MacroMap_t& macroMap,
        const ast::LOC& loc,
        const std::vector<ast::Param*>& params)
{
    if (params.empty())
        throw std::runtime_error("macro name is expected after 'macro'");
    if (params.size() < 2 || params.back()->name)
        throw std::runtime_error("macro body is expected as last unnamed parameter");

    /* name */
    const std::string name = GetMacroName(*params.front());
    /* params */
    std::vector<MacroParam> macroParams;
    std::vector<ast::Param*>::const_iterator param = params.begin();
    for (++param; &*param != &params.back(); ++param) {
        const std::string paramName = GetMacroParamName(**param);
        if (MacroParam::findByName(macroParams, paramName))
            throw std::runtime_error("duplicate macro parameter name: " + paramName);

        if ((*param)->name)
            macroParams.push_back(MacroParam(paramName, (*param)->value));
        else
            macroParams.push_back(MacroParam(paramName, new ast::Seq));
    }

    const Macro macro(
            loc,
            name,
            macroParams,
            params.back()->value);
    macroMap.insert(make_pair(name, macro));
}

struct MacroDefineVisitor: public ast::Visitor {
    size_t count;

    MacroDefineVisitor(MacroMap_t& macroMap):
        count(0),
        macroMap_(macroMap)
    {}

    /* Seq */
    virtual void preOrder(ast::Seq* node)
    {
        for (size_t i = 0; i < node->elems.size(); ++i) {
            ast::Seq::Elem* elem = &node->elems[i];
            if (elem->call && IsMacroDefine(elem->call))
                try {
                    DefineMacro(macroMap_, elem->call->loc, elem->call->params);
                    ++this->count;
                    /* delete from sequence */
                    node->elems.erase(node->elems.begin() + i);
                } catch (const std::runtime_error& e) {
                    ExceptionAtLocation(elem->call->loc, e);
                }
        }
    }
private:
    MacroMap_t& macroMap_;
};

static bool NeedQuoteElem(const ast::Seq::Elem& elem)
{
    return elem.str
        && std::find_if(
                elem.str->text.begin(),
                elem.str->text.end(),
                isspace) != elem.str->text.end();
}

static bool NeedQuoteSeq(const ast::Seq* seq)
{
    return std::find_if(
            seq->elems.begin(),
            seq->elems.end(),
            NeedQuoteElem) != seq->elems.end();
}

struct QuoteParamsVisitor: public ast::Visitor {
    /* Seq */
    virtual void postOrder(ast::Param* node)
    {
        if (node->name && NeedQuoteSeq(node->name))
            node->name = new ast::Seq(new ast::Quote(node->name->loc(), node->name));
        if (NeedQuoteSeq(node->value))
            node->value = new ast::Seq(new ast::Quote(node->value->loc(), node->value));
    }
};

void MacroProcessor(ast::Suite& suite)
{
    MacroMap_t macroMap;
    RegisterEmbeddedMacros(macroMap);
    MacroDefineVisitor define(macroMap);
    MacroExpandVisitor expand(macroMap);
    size_t numIterations = 0;
    do {
        define.count = 0;
        expand.count = 0;
        suite.walk(define);
        suite.walk(expand);
        ++numIterations;
    } while ((define.count > 0 || expand.count > 0) && numIterations < 256);

    /* Macro expansion could result in param name or value sequence
     * that must be quoted in source code, but is not quoted in the AST.
     * Fix it to allow correct source code generation.
     */
    QuoteParamsVisitor quoteParams;
    suite.walk(quoteParams);
}

