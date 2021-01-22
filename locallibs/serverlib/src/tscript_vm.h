/*
 * tscript virtual machine interpretator
 */

#pragma once

#include <string>
#include <vector>
#include <stack>
#include <iosfwd>

namespace xp_testing { namespace tscript {

    /***************************************************************************
     * Data types
     **************************************************************************/
    
    enum VM_OPCODE { VM_PUSH, VM_CALL, VM_CONC };

    struct VmLocation {
        std::string firstFile;
        unsigned firstLine;
        unsigned firstCol;
        std::string lastFile; /* may be empty */
        unsigned lastLine;
        unsigned lastCol;
        VmLocation(): firstLine(0), firstCol(0), lastLine(0), lastCol(0) {}
    };

    struct VmPush {
        std::string value;
    };

    struct VmCall {
        unsigned numParams;
        VmLocation loc;
        std::vector<VmLocation> macroExpandLocations;
        VmCall(): numParams(0) {}
    };

    struct VmCmd {
        VM_OPCODE opcode;
        VmPush push;
        VmCall call;
        VmCmd(): opcode(VM_PUSH) {}
    };

    struct VmModule {
        size_t index; /* module index in suite */
        std::vector<VmCmd> cmds;
        VmModule(): index(0) {}
    };

    struct VmSuite {
        std::vector<VmModule> modules;
    };

    struct VmParam {
        std::string name;
        std::string value;
    };

    /***************************************************************************
     * Load
     **************************************************************************/

    VmSuite VmLoadSuite(const std::string& vmcode);

    /***************************************************************************
     * Execute
     **************************************************************************/

    struct VmCallFunctor {
        virtual ~VmCallFunctor() {}
        virtual std::string operator()(const std::string& name, const std::vector<VmParam>& params) = 0;
    };

    typedef std::stack<std::string> VmStack_t;

    struct VmEnv {
        bool noCatch;
        VmStack_t stack;
        VmEnv(): noCatch(false) {}
    };

    void VmExecModule(VmEnv& env, VmCallFunctor& callFunctor, const VmModule& module);
}} /* namespace */
