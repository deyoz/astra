#include "tscript_vm.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <stdexcept>
#include <cstring>
#include <cstdlib>

namespace xp_testing { namespace tscript {

    /***************************************************************************
     * Load
     **************************************************************************/
    
    static void PrintLocation(std::ostream& out, const VmLocation& loc);

    static void ParseSpaces(const char*& in)
    {
        while (*in == ' ') {
            ++in;
        }
    }

    static bool ReadQuoted(const char*& in, std::string& result)
    {
        if (*in == 0) {
            return false;
        }

        if (*in != '"') {
            return false;
        }

        ++in;
        const char* start = in;
        while (*in != 0 && *in != '"') {
            ++in;
        };
        result.assign(start, in);
        if (*in == 0) {
            throw std::runtime_error("missing closing double quote");
        }
        ++in;
        ParseSpaces(in);
        return true;
    }

    static void ReadUnsignedInt(const char*& in, unsigned& value)
    {
        char* str_end = 0;
        value = strtol(in, &str_end, 0);
        if (in == str_end) {
            throw std::runtime_error("unsigned integer expected:" + std::string(in, in + 10));
        }
        in = str_end;

        ParseSpaces(in);
    }

    /*
     * "file" firstLine firstcol lastLine lastcol
     * OR
     * "firstFile" firstLine firstcol "lastFile" lastLine lastcol
     */
    static bool ReadLocation(const char*& in, VmLocation& loc)
    {
        if (!ReadQuoted(in, loc.firstFile))
            return false;
        ReadUnsignedInt(in, loc.firstLine);
        ReadUnsignedInt(in, loc.firstCol);

        ReadQuoted(in, loc.lastFile);

        ReadUnsignedInt(in, loc.lastLine);
        ReadUnsignedInt(in, loc.lastCol);
        return true;
    }

    static void ReadPush(const char*& in, VmCmd& cmd)
    {
        cmd.opcode = VM_PUSH;

        if (*in == 0 || *in != '{')
            throw std::runtime_error("'{' expected");

        while (*(++in) != 0 && *in != '}') {
            if (*in == '\\') {
                if (*(++in) == 0)
                    throw std::runtime_error("escape at the end of input");
                if (*in == 'n') {
                    cmd.push.value += '\n';
                    continue;
                }
            }
            cmd.push.value += *in;
        }

        if (*in++ != '}')
            throw std::runtime_error("'}' expected");
    }

    static void ReadCall(const char*& in, VmCmd& cmd)
    {
        cmd.opcode = VM_CALL;
        ReadUnsignedInt(in, cmd.call.numParams);
        ReadLocation(in, cmd.call.loc);

        for (;;) {
            VmLocation tmp;
            if (!ReadLocation(in, tmp))
                break;
            cmd.call.macroExpandLocations.push_back(tmp);
        }
    }

    static void ReadConc(const char*& in, VmCmd& cmd)
    {
        cmd.opcode = VM_CONC;
    }

    static bool TryParseOpCode(const char*& in, const char* code)
    {
        if (0 == strncmp(in, code, 4)) {
            in += 4;
            ParseSpaces(in);
            return true;
        }
        return false;
    }

    static void ParseLine(const char*& in, VmSuite& suite)
    {
        if (TryParseOpCode(in, "BEGN")) {
            /* new module */
            VmModule module;
            module.index = suite.modules.size();
            suite.modules.push_back(std::move(module));
        } else {
            if (suite.modules.empty())
                throw std::runtime_error("missing BEGN");

            VmModule& currentModule = suite.modules.back();

            currentModule.cmds.emplace_back();
            VmCmd& cmd = currentModule.cmds.back();
            if (TryParseOpCode(in, "PUSH")) {
                ReadPush(in, cmd);
            } else if (TryParseOpCode(in, "CALL")) {
                ReadCall(in, cmd);
            } else if (TryParseOpCode(in, "CONC")) {
                ReadConc(in, cmd);
            } else {
                throw std::runtime_error("unknown opcode: " + std::string(in, in + 4));
            }
        }
        if (*in != '\n') {
            throw std::runtime_error("unexpected character(s) at the end of line:" + std::string(in, in + 20));
        }
        ++in;
    }

    VmSuite VmLoadSuite(const std::string& vmcode)
    {
        VmSuite suite;
        unsigned lineNumber = 0;
        const char* in = vmcode.c_str();
        try {
            while (*in != 0) {
                if (*in == '\n') {
                    break;
                }
                ++lineNumber;
                ParseLine(in, suite);
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
