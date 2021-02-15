#pragma once

#include <memory>
#include <queue>
#include <map>
#include "tscript_vm.h"

namespace xp_testing { namespace tscript {

    struct Test {
        Test(VmModule&& module) : module(std::move(module)) {};
        VmModule module;
    };

    struct SkipTest {
    };

    std::vector<Test> Parse(const std::string& fileName);
    void RunTest(const Test& test);
    void RunAllTests(const std::string& fileName);

    /***************************************************************************
     * Контекст активного теста
     **************************************************************************/

    struct TestContext {
        /* Текущий список ещё не проверенных ответных телеграмм */
        std::queue<std::string> outq;
        /* Переменные */
        std::map<std::string, std::string> vars;
        /* Результат выполнения поледнего запроса (например, маска) */
        std::string reply;
        /* captures от последнего regex */
        std::map<size_t, std::string> captures;
    };

    std::shared_ptr<TestContext> GetTestContext();

    /***************************************************************************
     * Callbacks to main application
     **************************************************************************/

    struct TsCallbacks {
        virtual ~TsCallbacks() {}
        virtual void beforeTest();
        virtual void afterTest();
        virtual void beforeFunctionCall() = 0;
        virtual void afterFunctionCall() = 0;
    };

    void SetTsCallbacks(TsCallbacks* callbacks);

    /***************************************************************************
     * Вспомогательные функции
     **************************************************************************/

    void CheckEmpty(std::queue<std::string>& outq);

    bool nosir_mode();

}} /* namespace xp_testing::tscript */
