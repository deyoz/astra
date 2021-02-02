#ifdef XP_TESTING
#include <stdlib.h>
#include <string.h>
#include <fstream>
#include <iostream>
#include <set>
#include <cassert>
#include <boost/date_time/gregorian/gregorian_types.hpp>
#include <errno.h>

#include "tscript.h"
#include "testmode.h"
#include "monitor_ctl.h"
#include "memcached_api.h"
#include "exception.h"
#include "func_placeholders.h"
#include "dates.h"
#include "checkunit.h"

#define NICKNAME "DMITRYVM"
#include "slogger.h"
#include "logcout.h"

#define THROW_MSG(msg)\
    do {\
        std::ostringstream msgStream;\
        msgStream << msg;\
        throw comtech::Exception(STDLOG, __FUNCTION__, xp_testing::maybe_recode_answer(msgStream.str()));\
    } while (0)

namespace xp_testing { namespace tscript {

    static bool _nosir_mode;
    static const char* COMPILER = "tscriptc";

    static bool NoCatch()
    {
        const char* noCatch = getenv("XP_NO_CATCH");
        return noCatch && strcmp(noCatch, "1") == 0;
    }

    static std::string Compile(const std::string& fileName)
    {
        const std::string command = "./" + std::string(COMPILER) + " -cw " + fileName;
        FILE* pipe = popen(command.c_str(), "r");
        if (pipe == NULL)
            THROW_MSG("failed to open pipe " << command);

        std::string result;
        char buffer[1024];
        while (!feof(pipe)) {
            size_t len = fread(buffer, 1, sizeof(buffer), pipe);
            result.append(buffer, len);
        }

        const int ret = pclose(pipe);
        if (ret != 0 && errno != ECHILD) /* ECHILD seems to be OK for pclose() */
            THROW_MSG(command << " failed with error status " << ret << ", errno = " << errno);
        return result;
    }

    static bool CheckCompiler()
    {
        std::ifstream f(COMPILER);
        return !f.fail();
    }

    std::vector<Test> Parse(const std::string& fileName)
    {
        VmSuite suite;
        if (CheckCompiler()) {
            const std::string vmcode = Compile(fileName);
            suite = VmLoadSuite(vmcode);
        } else
            LogCout(COUT_ERROR) << "no " << COMPILER << " symlink; pls run buildFromScratch" << std::endl;

        std::vector<Test> tests;
        for (VmModule& module:  suite.modules) {
            tests.emplace_back(std::move(module));
        }
        return tests;
    }

    /***************************************************************************
     * Выполнение
     **************************************************************************/

    static TsCallbacks* _Callbacks;
    static std::shared_ptr<TestContext> _TestContext;

    void SetTsCallbacks(TsCallbacks* callbacks)
    {
        _Callbacks = callbacks;
    }

    std::shared_ptr<TestContext> GetTestContext()
    {
        return _TestContext;
    }

    static std::string Call(const FuncPhProc& f, const std::vector<VmParam>& params)
    {
        std::vector<tok::Param> args;
        for (const VmParam& param:  params)
            args.push_back(tok::Param(param.name, param.value));

        return f.type1 ?
            f.type1(tok::ForcePositionalValues(args)) :
            f.type2(args);
    }

    struct CallFunctor: public VmCallFunctor
    {
        virtual std::string operator()(const std::string& name, const std::vector<VmParam>& params)
        {
            assert(_Callbacks);
            if (name.empty())
                return std::string();

            FuncPhMap_t::const_iterator it = GetFuncPhMap().find(name);
            if (it == GetFuncPhMap().end())
                THROW_MSG("undefined function: " << name);

            _Callbacks->beforeFunctionCall();
            const std::string result = Call(it->second, params);
            _Callbacks->afterFunctionCall();
            return result;
        }
    };

    static void RunTest_Internal(TestContext& context, const Test& test)
    {
        VmEnv env;
        CallFunctor callFunctor;
        env.noCatch = NoCatch();
        VmExecModule(env, callFunctor, test.module);
        LogTrace(TRACE5) << __FUNCTION__ << ": " << env.stack.size() << " values on stack";

        if (env.noCatch)
            CheckEmpty(context.outq);
        else
            try {
                CheckEmpty(context.outq);
            } catch (const std::runtime_error& e) {
                LogCout(COUT_ERROR) << "tscript: exception after last block (at the end of file)" << std::endl;
                throw;
            }
    }
    struct SetupTeardown
    {
        explicit SetupTeardown() {
            _TestContext.reset(new TestContext);
            _Callbacks->beforeTest();
        }
        ~SetupTeardown() {
            _Callbacks->afterTest();
            _TestContext.reset();
            if(memcache::callbacksInitialized()) {
                memcache::callbacks()->flushAll();
            }
        }
    };
    void RunTest(const Test& test)
    {
        assert(!_TestContext);
        assert(_Callbacks);
        SetupTeardown env_keeper;
        if (NoCatch()) {
            RunTest_Internal(*_TestContext, test);
        } else {
            try {
                RunTest_Internal(*_TestContext, test);
            } catch (const SkipTest& e) {
                LogTrace(TRACE0) << "Skip test";
            //} catch (const std::runtime_error& e) {
            //    LogTrace(TRACE0) << e.what();
            //    throw;
            }
        }
    }

    void RunAllTests(const std::string& fileName)
    {
        for (const auto& t : Parse(fileName)) {
            RunTest(t);
        }
    }

    /***************************************************************************
     * Вспомогательные функции
     **************************************************************************/

    void CheckEmpty(std::queue<std::string>& outq)
    {
        if (!outq.empty()) {
            std::string outstanding_msgs;
            std::queue<std::string> outq_tmp = outq;
            while(!outq_tmp.empty()) {
                outstanding_msgs += "\n >> \n" + outq_tmp.front();
                outq_tmp.pop();
            }

            THROW_MSG(
                    "number of results in FIFO more than expected. the following are not expected: "
                    << outstanding_msgs);
        }
    }

void TsCallbacks::beforeTest()
{
#ifdef XP_TESTING
    fixDate(boost::gregorian::date());
#endif // XP_TESTING
}

void TsCallbacks::afterTest()
{
#ifdef XP_TESTING
    unfixDate();
#endif // XP_TESTING
}

bool nosir_mode() { return _nosir_mode; }

} // tscript
} // xp_testing

/*******************************************************************************
 * nosir-интерпретатор
 ******************************************************************************/

static bool nosir_tscript_internal(const std::string& fileName, const std::set<size_t>& nums)
{
    LogCout(COUT_DEBUG) << "parsing " << fileName << std::endl;
    namespace tscript = xp_testing::tscript;
    const std::vector<tscript::Test> tests = tscript::Parse(fileName);

    LogCout(COUT_DEBUG) << tests.size() << " test(s) loaded" << std::endl;

    std::vector<size_t> failed;
    for (const tscript::Test& test:  tests) {
        const size_t testNumber = test.module.index + 1;
        if (!nums.empty() && !nums.count(testNumber))
            continue;

        LogCout(COUT_DEBUG) << "running test #" << testNumber << std::endl;

        if (xp_testing::tscript::NoCatch())
        {
            tscript::RunTest(test);
            LogCout(COUT_DEBUG) << "OK" << std::endl;
        }
        else
        {
            try
            {
                tscript::RunTest(test);
                LogCout(COUT_DEBUG) << "OK" << std::endl;
            }
            catch (const std::runtime_error& e)
            {
                LogCout(COUT_ERROR) << "test #" << testNumber << " failed: " << e.what() << std::endl;
                failed.push_back(testNumber);
            }
        }
    }

    if (!failed.empty()) {
        LogCout(COUT_ERROR) << fileName << ": " << failed.size() << " of " << tests.size() << " failed:";
        for (size_t number:  failed)
            LogCout(COUT_ERROR) << " #" << number;
        LogCout(COUT_ERROR) << std::endl;
    }

    return failed.empty();
}

int nosir_tscript(int argc, char** argv)
{
    xp_testing::tscript::_nosir_mode = true;

    if (argc < 2) {
        LogCout(COUT_ERROR) << "usage: obrzap -nosir -tscript file.ts [test# test# ...]" << std::endl;
        return 0;
    }

    initTestMode();
    set_signal(term3);

    std::set<size_t> nums;
    for (int i = 2; i < argc; ++i)
        nums.insert(std::stoul(argv[i]));

    return nosir_tscript_internal(argv[1], nums) ? 0 : 1;
}

#endif // XP_TESTING
