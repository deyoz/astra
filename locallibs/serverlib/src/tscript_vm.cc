#include "tscript_vm.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <stdexcept>

namespace xp_testing { namespace tscript {

    /***************************************************************************
     * Load
     **************************************************************************/
    
    static void PrintLocation(std::ostream& out, const VmLocation& loc);

    static bool ReadQuoted(std::istream& in, std::string& result)
    {
        char c = 0;
        in >> c;
        if (!in)
            return false;

        if (c != '"') {
            in.unget();
            return false;
        }

        while (in.get(c) && c != '"') result += c;
        if (!in || c != '"')
            throw std::runtime_error("missing closing double quote");
        return true;
    }

    /*
     * "file" firstLine firstcol lastLine lastcol
     * OR
     * "firstFile" firstLine firstcol "lastFile" lastLine lastcol
     */
    static bool ReadLocation(std::istream& in, VmLocation& loc)
    {
        if (!ReadQuoted(in, loc.firstFile))
            return false;
        in >> loc.firstLine >> loc.firstCol;
        ReadQuoted(in, loc.lastFile);
        in >> loc.lastLine >> loc.lastCol;
        if (!in)
            throw std::runtime_error("invalid location data");
        return true;
    }

    static void ReadPush(std::istream& in, VmCmd& cmd)
    {
        cmd.opcode = VM_PUSH;

        char c = 0;
        in >> c;
        if (!in || c != '{')
            throw std::runtime_error("'{' expected");
        
        while (in.get(c) && c != '}') {
            if (c == '\\') {
                if (!in.get(c))
                    throw std::runtime_error("escape at the end of input");
                if (c == 'n')
                    c = '\n';
            }
            cmd.push.value += c;
        }

        if (c != '}')
            throw std::runtime_error("'}' expected");
    }

    static void ReadCall(std::istream& in, VmCmd& cmd)
    {
        cmd.opcode = VM_CALL;
        in >> cmd.call.numParams;
        ReadLocation(in, cmd.call.loc);

        for (;;) {
            VmLocation tmp;
            if (!ReadLocation(in, tmp))
                break;
            cmd.call.macroExpandLocations.push_back(tmp);
        }
    }

    static void ReadConc(std::istream& in, VmCmd& cmd)
    {
        cmd.opcode = VM_CONC;
    }

    static void ParseLine(std::istream& in, VmSuite& suite)
    {
        std::string opcode;
        in >> opcode;
        if (opcode == "BEGN") {
            /* new module */
            VmModule module;
            module.index = suite.modules.size();
            suite.modules.push_back(module);
        } else {
            if (suite.modules.empty())
                throw std::runtime_error("missing BEGN");

            VmModule& currentModule = suite.modules.back();
            VmCmd cmd;
            if      (opcode == "PUSH") ReadPush(in, cmd);
            else if (opcode == "CALL") ReadCall(in, cmd);
            else if (opcode == "CONC") ReadConc(in, cmd);
            else
                throw std::runtime_error("unknown opcode: " + opcode);

            currentModule.cmds.push_back(cmd);
        }

        std::string tail;
        in >> std::noskipws >> tail;
        if (!tail.empty())
            throw std::runtime_error("unexpected character(s) at the end of line: " + tail);
    }

    VmSuite VmLoadSuite(std::istream& in)
    {
        VmSuite suite;
        unsigned lineNumber = 0;
        try {
            while (in) {
                std::string line;
                if (!std::getline(in, line) || line.empty())
                    break;
                ++lineNumber;

                std::istringstream lineStream(line);
                ParseLine(lineStream, suite);
            }
        } catch (const std::runtime_error& e) {
            std::ostringstream msg;
            msg << e.what() << " at line " << lineNumber;
            throw std::runtime_error(msg.str());
        }
        return suite;
    }

    /***************************************************************************
     * Execute
     **************************************************************************/

    static void ExecPush(VmStack_t& stack, const VmPush& cmd)
    {
        stack.push(cmd.value);
    }

    static void CheckMinStackSize(const VmStack_t& stack, size_t minSize)
    {
        if (stack.size() < minSize) {
            std::ostringstream msg;
            msg << "stack size = " << stack.size() << ", at least " << minSize << " expected";
            throw std::runtime_error(msg.str());
        }
    }

    static VmParam PopParam(VmStack_t& stack)
    {
        VmParam param;
        param.value = stack.top();
        stack.pop();
        param.name = stack.top();
        stack.pop();
        return param;
    }

    static void PrintLocation(std::ostream& out, const VmLocation& loc)
    {
        out << loc.firstFile
            << " : line " << loc.firstLine << ", col " << loc.firstCol;
        if (loc.lastFile.empty()) {
            if (loc.lastLine == loc.firstLine)
                out << "-" << loc.lastCol;
            else
                out << " - line " << loc.lastLine << ", col " << loc.lastCol;
        } else {
            out << " - " << loc.lastFile;
            out << " : line " << loc.lastLine << ", col " << loc.lastCol;
        }
    }

    static void ExecCall(VmStack_t& stack, VmCallFunctor& callFunctor, const VmCall& cmd, bool noCatch)
    {
        CheckMinStackSize(stack, 1 + cmd.numParams * 2);

        /* pop params */
        std::vector<VmParam> params(cmd.numParams);
        for (size_t i = 0; i < params.size(); ++i)
            params.at(params.size() - 1 - i) = PopParam(stack);
        /* pop name */
        const std::string functionName = stack.top();
        stack.pop();
        
        /* push return value */
        if (noCatch)
            stack.push(callFunctor(functionName, params));
        else
            try {
                stack.push(callFunctor(functionName, params));
            } catch (const std::exception& e) {
                std::ostringstream msg;
                msg << e.what() << "\n in call to \"" << functionName
                    << "\" with " << cmd.numParams << " params at ";
                PrintLocation(msg, cmd.loc);
                for (std::vector<VmLocation>::const_iterator it = cmd.macroExpandLocations.begin();
                     it != cmd.macroExpandLocations.end();
                     ++it)
                {
                    msg << "\n in macro expand at ";
                    PrintLocation(msg, *it);
                }
                throw std::runtime_error(msg.str());
            }
    }

    static void ExecConc(VmStack_t& stack)
    {
        CheckMinStackSize(stack, 2);

        const std::string rhp = stack.top();
        stack.pop();
        const std::string lhp = stack.top();
        stack.pop();
        stack.push(lhp + rhp);
    }

    static void VmExecModule_(VmEnv& env, VmCallFunctor& callFunctor, const VmModule& module, size_t& ip)
    {
        VmStack_t stack;
        for (std::vector<VmCmd>::const_iterator cmd = module.cmds.begin();
             cmd != module.cmds.end();
             ++cmd)
        {
            ++ip;
            switch (cmd->opcode) {
                case VM_PUSH: ExecPush(env.stack, cmd->push); break;
                case VM_CALL: ExecCall(env.stack, callFunctor, cmd->call, env.noCatch); break;
                case VM_CONC: ExecConc(env.stack); break;
            }
        }
    }

    void VmExecModule(VmEnv& env, VmCallFunctor& callFunctor, const VmModule& module)
    {
        size_t ip = 0; /* instruction pointer */
        if (env.noCatch)
            VmExecModule_(env, callFunctor, module, ip /* out */);
        else
            try {
                VmExecModule_(env, callFunctor, module, ip /* out */);
            } catch (const std::exception& e) {
                std::ostringstream msg;
                msg << e.what()
                    << "\n in module " << (module.index + 1);
                msg << " (ip = " << ip << ")";
                throw std::runtime_error(msg.str());
            }
    }

}} /* namespace */
