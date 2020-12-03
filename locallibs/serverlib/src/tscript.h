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
     * ���⥪�� ��⨢���� ���
     **************************************************************************/

    struct TestContext {
        /* ����騩 ᯨ᮪ ��� �� �஢�७��� �⢥��� ⥫��ࠬ� */
        std::queue<std::string> outq;
        /* ��६���� */
        std::map<std::string, std::string> vars;
        /* ������� �믮������ ��������� ����� (���ਬ��, ��᪠) */
        std::string reply;
        /* captures �� ��᫥����� regex */
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
     * �ᯮ����⥫�� �㭪樨
     **************************************************************************/

    void CheckEmpty(std::queue<std::string>& outq);

    bool nosir_mode();

}} /* namespace xp_testing::tscript */
