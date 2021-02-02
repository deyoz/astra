#if HAVE_CONFIG_H
#endif

#ifdef XP_TESTING
#include <sys/types.h>
#include <sys/mman.h>
#include <pthread.h>
#include <unistd.h>
#include <stdint.h>
#include <regex.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <tcl.h>
#include <assert.h>
#include <signal.h>
#include <iconv.h>
#include <sys/ioctl.h>

#include <string>
#include <list>
#include <map>
#include <iostream>
#include <iomanip>

#include "checkunit.h"
#include "testmode.h"
#include "lwriter.h"
#include "str_utils.h"
#include "monitor_ctl.h"
#include "tclmon.h"
#include "tcl_utils.h"
#include "query_runner.h"

#define NICKNAME "NONSTOP"
#include "slogger.h"
#include "logcout.h"

void init_localtime_tests();
void init_period_tests();
void init_int_parameters();
void init_right_parameters();
void init_json_tests();
void init_message_tests();
void init_encoding_tests();
void init_xml_tools_tests();
void init_exp_map_tests();
void init_enumset_tests();
void init_logrange_tests();
void init_algo_tests();
void init_httpsrv_tests();
void init_postgres_tests();
void init_bgnd_tests();

static void init_serverlib_tests()
{
    init_localtime_tests();
    init_period_tests();
    init_int_parameters();
    init_right_parameters();
    init_json_tests();
    init_message_tests();
    init_encoding_tests();
    init_xml_tools_tests();
    init_exp_map_tests();
    init_enumset_tests();
    init_logrange_tests();
    init_algo_tests();
    init_httpsrv_tests();
    init_postgres_tests();
    init_bgnd_tests();
}

namespace
{

struct TestFunc
{
    const char* name;
    const char* fullName;
    TestCases::TestFuncPtr ptr;
};
typedef std::list<TestFunc> Tests;
} // namespace

namespace TestCases
{

struct SirenaSuite
{
    const char* name;
    size_t timeout;
    SetupFuncPtr setup;
    SetupFuncPtr teardown;
    Tests tests;
};

} //namespace TestCases

namespace
{
using TestCases::SirenaSuite;

typedef std::multimap<std::string, SirenaSuite> SirenaSuites;

SirenaSuites& suites()
{
    static SirenaSuites suites;
    return suites;
}

std::vector<std::string>& not_found_suites_names()
{
    static std::vector<std::string> names;
    return names;
}

struct TCaseToRun {
    const char* suiteName;
    const char* tcaseName;
    size_t seq_num;

    TCaseToRun(const char* suiteName, const char* tcaseName):
        suiteName(suiteName), tcaseName(tcaseName) {}
};

enum ResultType { RT_PASS, RT_ERROR, RT_FAILURE };
enum ErrorContext { EC_NONE, EC_SETUP, EC_TEST, EC_TEARDOWN };

struct Result {
    ResultType type;
    ErrorContext context;
    std::string file;
    int line;
    std::string message;
    std::string tcaseName;
};

/*
 * Полный список tcase для выполнения.
 * 
 * Память под номер очередного теста и под результаты тестирования
 * общая для всех рабочих процессов.
 *
 * Список допускает параллельное обращение от нескольких процессов
 * к функциям getNext() и addResult() и addSameTestsCount().
 */
class SharedRunList {
public:
    SharedRunList():
        shared_(NULL),
        isForkedJob_(false)
    {
        void* ptr = mmap(NULL, sizeof(SHARED), PROT_READ | PROT_WRITE, MAP_ANON | MAP_SHARED, -1, 0);
        assert(ptr);
        memset(ptr, 0, sizeof(SHARED));
        shared_ = static_cast<SHARED*>(ptr);

        int ret = pthread_mutexattr_init(&shared_->mut_attr);
        assert(ret == 0);
        pthread_mutexattr_setpshared(&shared_->mut_attr, PTHREAD_PROCESS_SHARED);

        ret = pthread_mutex_init(&shared_->mut, &shared_->mut_attr);
        assert(ret == 0);

        shared_->isMutexInitialized = 1;
    }

    ~SharedRunList()
    {
        if (!isForkedJob_) {
            assert(shared_->isMutexInitialized);
            shared_->isMutexInitialized = 0;

            pthread_mutex_destroy(&shared_->mut);
            pthread_mutexattr_destroy(&shared_->mut_attr);
        }
        munmap(static_cast<void*>(shared_), sizeof(SHARED));
    }

    bool isForkedJob() const
    {
        return isForkedJob_;
    }

    void markAsForkedJob()
    {
        isForkedJob_ = true;
    }

    size_t size() const
    {
        return list_.size();
    }

    void add(const TCaseToRun& tcaseToRun)
    {
        assert(!isForkedJob_);
        list_.push_back(tcaseToRun);
    }

    const TCaseToRun* getNext()
    {
        pthread_mutex_lock(&shared_->mut);
        if (shared_->next == list_.size()) {
            pthread_mutex_unlock(&shared_->mut);
            return NULL;
        }
        TCaseToRun* result = &list_[shared_->next];
        result->seq_num = ++shared_->next;
        pthread_mutex_unlock(&shared_->mut);
        return result;
    }

    void addResult(const Result& r)
    {
        pthread_mutex_lock(&shared_->mut);

        assert(shared_->numResults < MAX_RESULTS);
        volatile RESULT& result = shared_->results[shared_->numResults];
        ++shared_->numResults;

        result.type = r.type;
        result.context = r.context;
        result.file = allocStringInSharedPool(r.file.c_str());
        result.line = r.line;
        result.message = allocStringInSharedPool(r.message.c_str());
        result.tcaseName = allocStringInSharedPool(r.tcaseName.c_str());

        pthread_mutex_unlock(&shared_->mut);
    }

    std::vector<Result> getResults() const
    {
        std::vector<Result> results;
        assert(!isForkedJob_);
        for (size_t i = 0; i < shared_->numResults; ++i) {
            const volatile RESULT& r = shared_->results[i];
            const Result result = {
                r.type,
                r.context,
                r.file ? r.file : "",
                r.line,
                r.message ? r.message : "",
                r.tcaseName ? r.tcaseName : ""
            };
            results.push_back(result);
        }
        return results;
    }
    
    void addSameTestsCount(size_t cnt)
    {
      if (cnt!=0)
      {
        pthread_mutex_lock(&shared_->mut);
        
        shared_->sameTestsCount+=cnt;
        
        pthread_mutex_unlock(&shared_->mut);
      }
    }

    size_t getSameTestsCount() const
    {
        assert(!isForkedJob_);
        return shared_->sameTestsCount;
    }
    
private:
    static const size_t MAX_RESULTS = 65536;
    static const size_t STRINGS_POOL_SIZE = 4 * 1024 * 1024;

    /* prevent copying */
    SharedRunList(const SharedRunList&) = delete;
    SharedRunList& operator=(const SharedRunList&) = delete;

    /* mutex must be locked before call to this function */
    const char* allocStringInSharedPool(const char* str)
    {
        if (!str)
            return NULL;
        const size_t size = strlen(str) + 1;
        assert(shared_->stringsPoolAllocatedBytes + size <= STRINGS_POOL_SIZE);
        char* p = shared_->stringsPool + shared_->stringsPoolAllocatedBytes;
        memcpy(p, str, size);
        shared_->stringsPoolAllocatedBytes += size;
        return p;
    }

    /* POD to place in shared memory */
    struct RESULT {
        ResultType type;
        ErrorContext context;
        const char* file; /* may be NULL */
        int line;
        const char* message; /* may be NULL */
        const char* tcaseName; /* may be NULL */
    };

    /* POD to store in shared memory */
    struct SHARED {
        volatile uint8_t isMutexInitialized;
        pthread_mutex_t mut;
        pthread_mutexattr_t mut_attr;
        volatile size_t next;
        /* results */
        volatile size_t numResults;
        volatile RESULT results[MAX_RESULTS];
        volatile size_t sameTestsCount;
        /* strings */
        volatile size_t stringsPoolAllocatedBytes;
        char stringsPool[4 * 1024 * 1024];
    };

    SHARED* shared_;
    bool isForkedJob_;    /* POD to place in shared memory */
    std::vector<TCaseToRun> list_;
};

static Result MakeResult(TestResult* r)
{
    ResultType type = RT_PASS;
    switch (tr_rtype(r)) {
        case CK_PASS: type = RT_PASS; break;
        case CK_ERROR: type = RT_ERROR; break;
        case CK_FAILURE: type = RT_FAILURE; break;
        default: assert(0); break;
    }

    ErrorContext context = EC_NONE;
    if (type != RT_PASS)
        switch (tr_ctx(r)) {
            case CK_CTX_SETUP: context = EC_SETUP; break;
            case CK_CTX_TEST: context = EC_TEST; break;
            case CK_CTX_TEARDOWN: context = EC_TEARDOWN; break;
            default: assert(0); break;
        }

    const char* lfile = tr_lfile(r);
    const char* msg = tr_msg(r);
    const char* tcname = tr_tcname(r);
    const Result result = {
        type,
        context,
        lfile ? lfile : "",
        tr_lno(r),
        msg ? msg : "",
        tcname ? tcname : ""
    };
    return result;
}

static void TCaseAddTest(TCase* tcase, const TestFunc& test)
{
#if (CHECK_MAJOR_VERSION*10000+CHECK_MINOR_VERSION*100+CHECK_MICRO_VERSION > 906)
    _tcase_add_test(tcase, test.ptr, test.fullName, 0, 0, 0, 1);
#else
    _tcase_add_test(tcase, test.ptr, test.fullName, 0, 0, 1);
#endif
}

static TCase* CreateAndConfigureTCase(const SirenaSuite& sirSuite, const char* tcaseName)
{
    TCase* tcase = tcase_create(tcaseName);
    tcase_set_timeout(tcase, sirSuite.timeout ? sirSuite.timeout : 120);

    // Checked fixtures are run inside the address space
    // created by the fork to create the unit test (for each test in test case)
    tcase_add_checked_fixture(tcase, sirSuite.setup, sirSuite.teardown);
    return tcase;
}

static SirenaSuite FilterSuite(const SirenaSuite& suite, regex_t* re, const std::vector<regex_t>& excludes)
{
    SirenaSuite result = suite;
    if (re) {
        result.tests.clear();
        for (auto& test : suite.tests) {
            bool exclude = false;
            for (auto& excl : excludes) {
               if (regexec(&excl, test.fullName, 0, NULL, 0) == 0) {
                    exclude = true;
                    break;
               }
            }
            if (!exclude && regexec(re, test.fullName, 0, NULL, 0) == 0)
                result.tests.push_back(test);
        }
    }
    return result;
}

static void AddSuiteToSRunner(SRunner* sr, const SirenaSuite& sirSuite, bool oneTestPerTCase)
{
    Suite* suite = suite_create(sirSuite.name);
    if (oneTestPerTCase) {
        for (Tests::const_iterator test = sirSuite.tests.begin(); test != sirSuite.tests.end(); ++test) {
            TCase* tcase = CreateAndConfigureTCase(sirSuite, test->fullName);
            TCaseAddTest(tcase, *test);
            suite_add_tcase(suite, tcase);
        }
    } else {
        TCase* tcase = CreateAndConfigureTCase(sirSuite, sirSuite.name);
        for (Tests::const_iterator test = sirSuite.tests.begin(); test != sirSuite.tests.end(); ++test)
            TCaseAddTest(tcase, *test);
        suite_add_tcase(suite, tcase);
    }
    srunner_add_suite(sr, suite);
}

static void AddSuitesToSRunner(SRunner* sr, const std::vector<SirenaSuite>& suites, bool oneTestPerTCase)
{
    for (auto const& suite : suites)
        AddSuiteToSRunner(sr, suite, oneTestPerTCase);
}

static void PrepareRunList(const std::vector<SirenaSuite>& suites, SharedRunList& runList)
{
    for (auto const& suite : suites) {
        for (auto const& test : suite.tests)
            runList.add(TCaseToRun(suite.name, test.fullName));
    }
}

} // namespace

namespace TestCases
{

SirenaSuite* createSuite(const char* name, SetupFuncPtr setup_, TeardownFuncPtr teardown_)
{
    SirenaSuite ss = {};
    ss.name = name;
    ss.setup = setup_;
    ss.teardown = teardown_;
    auto it = suites().emplace(name, ss);
    //fprintf(stderr, "createSuite: [%s] size: %zd\n", name, suites().size());
    return &it->second;
}

void setTimeout(struct SirenaSuite* ss, int timeout)
{
    ss->timeout = (timeout > 0 ? timeout : 0);
}

void addTest(struct SirenaSuite* ss, const char* funcName, const char* fullFuncName, TestFuncPtr testPtr)
{
    TestFunc tf = {};
    tf.name = funcName;
    tf.fullName = fullFuncName;
    tf.ptr = testPtr;
    ss->tests.push_back(tf);
}

void printAllSuites(int verbose)
{
    //fprintf(stderr, "Suites size: %zd\n", suites().size());
    for (SirenaSuites::const_iterator si = suites().begin(); si != suites().end(); ++si) {
        fprintf(stderr, "%s\n", si->first.c_str());
        if (verbose) {
            for (Tests::const_iterator ti = si->second.tests.begin(); ti != si->second.tests.end(); ++ti) {
                fprintf(stderr, "  %s\n", ti->name);
            }
        }
    }
}

static SRunner *sr = NULL;

class ExceptionToExit
{
  public:
    ExceptionToExit() {}
};

class TSignalsOnTests
{
  private:
    static const int sigs[];
  public:
    static void on_signal(int sig);
    static const char* fname_tests_log()
    {
        static const char* x = getenv("SIRENA_XP_TESTS_LOG");
        return x ? *x ? x : NULL : "tests.log";
    }
    TSignalsOnTests();
    ~TSignalsOnTests();
};



const int TSignalsOnTests::sigs[] = {SIGINT,/*SIGTERM,SIGKILL,SIGQUIT,SIGILL,SIGBUS,SIGSEGV,*/};

void TSignalsOnTests::on_signal(int sig)
{
  if (sr)
  {
    fflush(stdout);

    #define STDERR fileno(stderr)
    #define STDOUT fileno(stdout)

    fflush(stdout);    
    /* создать дополнительный дескриптор для stdout */
    int oldstdout = dup(STDOUT);
    /* перенаправить стандартный вывод в stderr, путем дублирования его дескриптора */
    dup2(STDERR,STDOUT);

    fprintf(stdout,"\n");
    srunner_print(sr, CK_NORMAL);
    fprintf(stdout,"Look at %s\n",fname_tests_log());
    fflush(stdout);

    /* восстановить исходный дескриптор stdout */
    dup2(oldstdout,STDOUT);
    /* закрыть второй дескриптор stdout */
    close(oldstdout);
  }
  throw ExceptionToExit();  
}        

TSignalsOnTests::TSignalsOnTests()
{
  for(size_t i=0; i<sizeof(sigs)/sizeof(sigs[0]); ++i)
  {
    signal(sigs[i],TSignalsOnTests::on_signal);
  }  
} 

TSignalsOnTests::~TSignalsOnTests()
{
  for(size_t i=0; i<sizeof(sigs)/sizeof(sigs[0]); ++i)
  {
    signal(sigs[i],SIG_DFL);
  }  
}

struct RegList
{
    std::vector<regex_t> reglist;
    RegList(size_t n) {  reglist.reserve(n);  }
    ~RegList() {  for(auto& r : reglist)  regfree(&r);  }
};

static auto split_string_to_un_list(const char* str)
{
    auto v = StrUtils::split_string(str, ',');
    std::sort(v.begin(), v.end());
    v.erase(std::unique(v.begin(), v.end()), v.end());
    return v;
}

/* returns 0 on success */
static int PrepareSuitesToRun(const char* xpList, const char* xpExclude, char const* start_from,
        std::vector<SirenaSuite>& filteredSuites)
{
    assert(xpList);
    assert(xpExclude);

    if (xpList && *xpList) {
        auto suitesList = split_string_to_un_list(xpList);
        auto excludeList = split_string_to_un_list(xpExclude);

        RegList reglist(excludeList.size());
        for (auto& sl : excludeList)
        {
            const std::string exclExpr = "\\b" + sl + "\\b";
            reglist.reglist.emplace_back();
            if(/*const int ret = */regcomp(&reglist.reglist.back(), exclExpr.c_str(), REG_NOSUB))
            {
                fprintf(stderr, "bad regex: %s\n", exclExpr.c_str());
                return 1;
            }
        }
        for (auto& sli : suitesList)
        {
            const std::string testExpr = "\\b" + sli + "\\b";
            regex_t testRe;
            const int ret = regcomp(&testRe, testExpr.c_str(), REG_NOSUB);
            if (ret) {
                fprintf(stderr, "bad regex: %s\n", testExpr.c_str());
                return 1;
            }
            size_t numTestsAdded = 0;
            for (SirenaSuites::const_iterator si = suites().begin(); si != suites().end(); ++si) {
                assert(si->first != testExpr);
                auto filtered = FilterSuite(si->second, (si->first == testExpr) ? NULL : &testRe, reglist.reglist);
                numTestsAdded += filtered.tests.size(); // HERE!!!!
                auto f = std::find_if(filteredSuites.begin(), filteredSuites.end(),
                                      [&filtered](auto& x){ return x.name == filtered.name and x.timeout == filtered.timeout and
                                                                  x.setup == filtered.setup and x.teardown == filtered.teardown; });
                if(f != filteredSuites.end())
                    f->tests.splice(f->tests.end(), std::move(filtered.tests));
                else if(not filtered.tests.empty())
                    filteredSuites.push_back(filtered);
            }
            regfree(&testRe);
            if (numTestsAdded == 0 and strcmp(xpExclude,"0_count") != 0) {
                not_found_suites_names().emplace_back(sli);
            }
        }
    } else {
        filteredSuites.resize(suites().size());
        std::transform(suites().begin(), suites().end(), filteredSuites.begin(), [](auto&s){ return s.second; });
    }
    for(auto& x : filteredSuites)
    {
        x.tests.sort([](auto& l, auto& r){ return strcmp(l.fullName,r.fullName) < 0; });
        x.tests.unique([](auto& l, auto& r){ return strcmp(l.fullName,r.fullName) == 0; });
    }
    if(start_from and *start_from)
    {
        char const* dot = strchr(start_from, '.');
        auto _match = [dot](char const* s, char const* n){ return dot ? strncmp(s,n,dot-n)<0 : strcmp(s,n)<0; };
        auto el = std::lower_bound(filteredSuites.begin(), filteredSuites.end(), start_from, [&](auto&s, char const*n){ return _match(s.name,n); });
        auto eu = std::upper_bound(el,                     filteredSuites.end(), start_from, [&](char const*n, auto&s){ return _match(n,s.name); });
        if(el != eu)
        {
            for(auto e = el; e != eu; ++e)
            {
                auto te = std::lower_bound(e->tests.begin(), e->tests.end(), start_from,
                                           [](auto& t, char const* n){ return strcmp(t.fullName,n) < 0; });
                e->tests.erase(e->tests.begin(), te);
            }
            filteredSuites.erase(filteredSuites.begin(), el);
        }
    }
    std::sort(filteredSuites.begin(), filteredSuites.end(), [](auto& a, auto& b){  return strcmp(a.name,b.name) < 0;  });
    return 0;
}

static SRunner* CreateSRunner()
{
    const char* no_fork_ = getenv("XP_NO_FORK");
    const bool no_fork = (no_fork_ && !strcmp(no_fork_, "1"));

    sr = srunner_create(NULL);
    if(auto x = TSignalsOnTests::fname_tests_log())
        srunner_set_log(sr, x);
    srunner_set_fork_status(sr, no_fork ? CK_NOFORK : CK_FORK);
    return sr;
}

static std::string FixConnectString(const std::string& connectString,
    const std::string& connectStringModifier, size_t jobIdx)
// Oracle format:
//     <user>/<password>[@<hostname+instance>]
// JMS Oracle format:
//     [oracle://]<user>/<password>[@<hostname+instance>]
// JMS AMQP format:
//     {amqp|amqps}://<user>:<password>@<hostname>[:<port>][/<vhost>]
{
    std::string fixedConnectString = connectString;
    std::string modification = connectStringModifier + std::to_string(jobIdx);

    size_t cssPos = fixedConnectString.find("://");
    size_t userPos = 0;
    bool isAmqpPg = false;
    if(cssPos != std::string::npos)
    {
        userPos = cssPos + 3; // length("://")
        if (fixedConnectString.substr(0, cssPos).find("amqp") != std::string::npos
            || fixedConnectString.substr(0, cssPos).find("postgres") != std::string::npos)
        {
            isAmqpPg = true;
        }
    }

    size_t afterUserPos = fixedConnectString.find_first_of("/:", userPos);
    if(afterUserPos == std::string::npos)
        fixedConnectString += modification;
    else
        fixedConnectString.insert(afterUserPos, modification);

    if(isAmqpPg)
    {
        size_t vhostPos = fixedConnectString.find_last_of("/");
        bool isExistsVhost = (vhostPos != std::string::npos) &&
              (vhostPos > userPos);
        if(!isExistsVhost)
            fixedConnectString += "/";
        fixedConnectString += modification;
    }

    return fixedConnectString;
}

static void FixConnectStrings(size_t jobIdx)
{
    if (jobIdx == 0)
        return;

    std::vector<std::string> vars;
    readStringListFromTcl(vars, "CONNECT_STRING_VARS", ",");
    std::string connect_string_modifier=readStringFromTcl("CONNECT_STRING_MODIFIER", std::string());

    for(auto& var : vars)
    {
        const std::string connectString = readStringFromTcl(var, std::string());
        if (!connectString.empty()) {
            std::string fixedConnectString =
                FixConnectString(connectString, connect_string_modifier, jobIdx);
            setTclVar(var, fixedConnectString);
        }
    }
}

static size_t CountTests(const std::vector<SirenaSuite>& suites)
{
    size_t result = 0;
    for (std::vector<SirenaSuite>::const_iterator it = suites.begin(); it != suites.end(); ++it)
        result += it->tests.size();
    return result;
}

void check_test_duplicates(const std::vector<SirenaSuite>& suites)
{
    std::vector<std::string> test_names;
    for(auto const& suite : suites)
        for(auto const& t : suite.tests)
            test_names.push_back(t.fullName);

    std::sort(test_names.begin(), test_names.end());
    auto a_b = std::adjacent_find(test_names.begin(), test_names.end());
    if(a_b == test_names.end())
        return;

    LogCout(COUT_INFO) << "\n!!! Found test name duplicates:\n";
    while(a_b != test_names.end())
    {
        auto a_e = std::find_if(a_b+1, test_names.end(), [a_b](auto& s){ return *a_b != s; });
        LogCout(COUT_INFO) << *a_b << " : " << a_e-a_b << " duples found\n";
        a_b = std::adjacent_find(a_e, test_names.end());
    }
    LogCout(COUT_INFO)<< std::endl;
}

namespace {
constexpr char JobIdxVar[] = {"XP_CURRENT_JOB_ID"};
static void SetJobIdx(size_t jobIdx) {
    setTclVar(JobIdxVar, std::to_string(jobIdx));
}

//-----------------------------------------------------------------------

struct TCasePrinter
{
    pid_t pid;
    unsigned jobIdx;
    unsigned n_tests, xpFrom;
    unsigned crisp_columns;
    void operator<<(TCaseToRun const* t) const;
};
void TCasePrinter::operator<<(TCaseToRun const* t) const
{
    if(crisp_columns and xpFrom > t->seq_num)
        return;
    if(crisp_columns)
        LogCout(COUT_INFO) /* << '\r' << "\x1b[48;5;67m" << std::setw(crisp_columns * t->seq_num / n_tests) << ' ' */
                           << '\r' << std::right << std::setw(5) << t->seq_num
                           << '\\' << std::left << std::setw(5) << n_tests
                           << ": " << std::setw(crisp_columns - 13) << t->tcaseName
                           << std::flush;
    else
        LogCout(COUT_INFO) << t->seq_num << '\\' << n_tests << ' ' << pid << '/' << jobIdx
                           << ": " << t->tcaseName << (xpFrom > t->seq_num ? "<skipped>":"")
                           << std::endl;
}

} //anonymous ns

int GetJobIdx() {
    int ret = readIntFromTcl(JobIdxVar, 0);
    return (ret > 0) ? ret : 0;
}

static void ForkedJob(unsigned jobIdx, unsigned xpFrom, const std::vector<SirenaSuite>& suites, SharedRunList& runList, bool suppress_stdout, unsigned crisp_columns)
{
    LogCout(COUT_DEBUG) << "forked job with pid = " << getpid() << ", job idx = " << jobIdx << ", suppress_stdout = " << std::boolalpha << suppress_stdout << ", crisp_columns=" << crisp_columns << std::endl;
    SetJobIdx(jobIdx);
    FixConnectStrings(jobIdx);
    TSignalsOnTests signals_on_test;

    SRunner* sr = CreateSRunner();
    AddSuitesToSRunner(sr, suites, true /* oneTestPerTCase */);
    const unsigned n_tests = CountTests(suites);
    size_t test_diff_count=0;
    size_t local_seq_num=0;
    auto saved_stdout = suppress_stdout ? dup(1) : 0;
    auto devnull = suppress_stdout ? open("/dev/null", O_WRONLY) : 0;

    TCasePrinter print = { getpid(), jobIdx, n_tests, xpFrom, crisp_columns };

    while(const TCaseToRun* tcaseToRun = runList.getNext())
    {
        print << tcaseToRun;

        if (xpFrom>tcaseToRun->seq_num)
          continue;
        
        ++local_seq_num;

        auto coutbuf = suppress_stdout ? std::cout.rdbuf(nullptr) : nullptr;
        auto cerrbuf = suppress_stdout ? std::cerr.rdbuf(nullptr) : nullptr;
        if(suppress_stdout)  dup2(devnull, 1);

#if CHECK_MAJOR_VERSION*10000 + CHECK_MINOR_VERSION*100 + CHECK_MICRO_VERSION >= 909
        srunner_run(sr, tcaseToRun->suiteName, tcaseToRun->tcaseName, CK_SILENT);
#else
#warning check version >= 0.9.9 required to run tests with TEST_J environment variable
        LogCout(COUT_ERROR) << "check version >= 0.9.9 required to run tests with TEST_J environment variable" << std::endl;
        exit(1);
#endif
        const size_t numTestsRun = srunner_ntests_run(sr);
        if (local_seq_num+test_diff_count!=numTestsRun)
        {
          size_t current_runned_count=numTestsRun-local_seq_num-test_diff_count;
          test_diff_count+=current_runned_count;
          LogCout(COUT_INFO)
            << "<<<<< " << tcaseToRun->tcaseName << ": RUNNED ADDITIONAL "<<current_runned_count
            << " tests DUE SAME TEST NAME >>>>>"
            << std::endl;
        }

        if(suppress_stdout) {
            std::cout.rdbuf(coutbuf);
            std::cerr.rdbuf(cerrbuf);
            dup2(saved_stdout, 1);
        }
    }
    if(suppress_stdout) {
        close(devnull);
        close(saved_stdout);
    }

    std::unique_ptr<TestResult*, decltype(&free)> results(srunner_results(sr), free);
    const int numTestsRun = srunner_ntests_run(sr);
    for (int i = 0; i < numTestsRun; ++i)
        runList.addResult(MakeResult(results.get()[i]));
    runList.addSameTestsCount(test_diff_count);

    srunner_free(sr);
    LogCout(COUT_DEBUG) << "done job with pid = " << getpid() << ", job idx = " << jobIdx << std::endl;

    /* 
     * Неприятная бага в tcl - если делать fork(), потом при завершении
     * процесса он (процесс) вешается в Tcl_FinalizeNotifier.
     * Тема про это:
     * http://objectmix.com/tcl/209626-tcl-crashes-hangs-rhel5.html
     *
     * Поэтому просто мочим процесс через exit(0), чтобы
     * дело не дошло до Tcl_FinalizeNotifier.
     */
    exit(0);
}

static void PrintErrorOrFailure(const Result& r)
{
    LogCout(COUT_ERROR)
        << (r.file.empty() ? "(null)" : r.file)
        << ":"
        << r.line
        << ": "
        << r.tcaseName
        << " : (after this point) "
        << r.message
        << std::endl;
}

static size_t PrintResults(const std::vector<Result>& results,size_t sameTestsCount)
{
    size_t numPassed = 0;
    size_t numErrors = 0;
    size_t numFailures = 0;
    size_t numNotFoundSuites = not_found_suites_names().size();

    for (std::vector<Result>::const_iterator r = results.begin(); r != results.end(); ++r) {
        switch (r->type) {
            case RT_PASS: ++numPassed; break;
            case RT_ERROR: ++numErrors; break;
            case RT_FAILURE: ++numFailures; break;
            default: assert(0); break;
        }
    }
    size_t numProblemsTotal = numErrors + numFailures + numNotFoundSuites;

    std::ostringstream os_sub;
    if (sameTestsCount!=0)
      os_sub<< "("<<sameTestsCount<<" duplicated)";

    std::string color_on = numProblemsTotal ? "\033[1;31m"/*red*/ : "\033[1;32m"/*green*/;
    std::string color_off = "\033[0m";

    const size_t percent = results.empty() ? 0 : numPassed * 100 / results.size();
    /* 0%: Checks: 11, Failures: 0, Errors: 11 */
    LogCout(COUT_INFO) << color_on << percent << "%: Checks: " << results.size() 
        << os_sub.str()
        << ", Failures: " << numFailures
        << ", Errors: " << numErrors
        << color_off << std::endl;
    
    /* Not Found Suites: 2: suite1, suite2 */
    if(numNotFoundSuites)
        LogCout(COUT_INFO) << color_on
            << "Not Found Suites: " << numNotFoundSuites << ": "
            << StrUtils::join(", ",not_found_suites_names())
            << color_off << std::endl;
    
    for (std::vector<Result>::const_iterator r = results.begin(); r != results.end(); ++r) {
        switch (r->type) {
            case RT_PASS:
                break;

            case RT_ERROR:
            case RT_FAILURE:
                PrintErrorOrFailure(*r);
                break;

            default:
                assert(0);
                break;
        }
    }

    return numProblemsTotal;
}

struct TestRef {
    const SirenaSuite* suite;
    const TestFunc* test;
};

static bool GetBooleanEnv(const char* name)
{
    const char* env = getenv(name);
    return env && strcmp(env,"1") == 0;
}


struct PidHolder
{
    pid_t pid = 0;
    ~PidHolder() { if(pid) kill(pid,SIGTERM); }
};

static size_t RunSuites(const std::vector<SirenaSuite>& suites,size_t xpFrom)
{
    const char* numJobs_env = getenv("TEST_J");
    const int numJobs_i = numJobs_env ? atoi(numJobs_env) : 1;
    if(numJobs_i <= 0) {
        std::cerr << "TEST_J set to invalid value" << numJobs_env << std::endl;
        return 1;
    }

    const size_t n_tests = CountTests(suites);
    const size_t numJobs = std::min(static_cast<size_t>(numJobs_i), n_tests);

    const bool suppress_stdout = GetBooleanEnv("SIRENA_XP_NO_STDOUT");
    unsigned crisp_columns = GetBooleanEnv("SIRENA_XP_CRISP_OUTPUT");
    if(crisp_columns) {
        struct winsize w;
        crisp_columns = ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0 ? w.ws_col : 0;
    }

    PidHolder memcached_pid;
    const auto memcached_socket = readStringFromTcl("XP_TESTS_MEMCACHED_HOST","");
    if(not memcached_socket.empty())
    {
        memcached_pid.pid = fork();
        if(memcached_pid.pid == 0)
        {
            auto e = execlp("memcached", "memcached", "-s", memcached_socket.c_str(), "-m500", "-M", nullptr);
            LogCout(errno == ENOENT ? COUT_DEBUG : COUT_ERROR) << "execlp(memcached -s " << memcached_socket << ") :: ret = "
                                                               << e << ", errno = " << strerror(errno) << std::endl;
            return 0;
        }
    }
    SharedRunList runList; /* shared among jobs */
    PrepareRunList(suites, runList);
    check_test_duplicates(suites);

    std::vector<pid_t> jobPids(numJobs, 0);
    for (size_t jobIdx = 0; jobIdx < numJobs; ++jobIdx) {
        if(const pid_t pid = fork())
            jobPids[jobIdx] = pid;
        else
        {
            runList.markAsForkedJob();
            ForkedJob(jobIdx, xpFrom, suites, runList, suppress_stdout, crisp_columns);
            return 0;
        }
    }

    bool allForkedJobsDone = true;
    for (std::vector<pid_t>::const_iterator jobPid = jobPids.begin(); jobPid != jobPids.end(); ++jobPid) {
        int status = 0;
        waitpid(*jobPid, &status, 0);

        if (allForkedJobsDone && !(WIFEXITED(status) && WEXITSTATUS(status) == 0)) {
            LogCout(COUT_ERROR) << *jobPid << " abnormal termination or non-zero exit code" << std::endl;
            allForkedJobsDone = false;
            /* kill all remaining forked jobs */
            for (std::vector<pid_t>::const_iterator killPid = jobPid + 1;
                killPid != jobPids.end();
                ++killPid)
            {
                LogCout(COUT_ERROR) << "killing " << *killPid << std::endl;
                const int killret = kill(*killPid, SIGKILL);
                if (killret != 0)
                    LogCout(COUT_ERROR) << "kill failed with return value " << killret;
            }
        }
    }
    if(suppress_stdout)
        LogCout(COUT_INFO) << std::endl;
    if (allForkedJobsDone) {
        LogCout(COUT_DEBUG) << "all forked jobs completed successfully" << std::endl;
        std::vector<Result> results = runList.getResults();
        size_t sameTestsCount=runList.getSameTestsCount();
        std::sort(begin(results), end(results), [](const Result& a, const Result& b){ return a.tcaseName < b.tcaseName; });
        return PrintResults(results,sameTestsCount);
    } else {
        LogCout(COUT_ERROR) << "some or all forked jobs completed with error(s)" << std::endl;
        return PrintResults(runList.getResults(),runList.getSameTestsCount());
    }
}

int run_all_tests(void)
{
    const char* xplg = getenv("XP_LOGFILE");
    setLogging(xplg and *xplg ? xplg : "tclmon.log", LOGGER_SYSTEM_FILE, tclmonCurrentProcessName());
    const char* xpList = getenv("XP_LIST");
    const char* c_xpFrom = getenv("XP_FROM");
    const char* xpExclude = getenv("XP_LIST_EXCLUDE");
    const char* xpStartFrom = getenv("XP_START_FROM");

    if(GetBooleanEnv("XP_NO_RUN")) {
        fprintf(stdout,"\nPRODUCTION SYSTEM! DON'T RUN TESTS!!\n");
        return 1;
    }

    size_t xpFrom=0; 
    if (c_xpFrom!=nullptr)
    {
      int i_xpFrom=strtol(c_xpFrom,NULL,10);
      xpFrom=i_xpFrom<0?0:i_xpFrom;
    }
    if(!xpExclude){
        xpExclude="";
    }
    if(!xpList){
        xpList= strlen(xpExclude)==0 ? "" :".*";
    }
    if (strcmp(xpList, "1") == 0) {
        TestCases::printAllSuites(0);
        return 0;
    }
    if (strcmp(xpList, "2") == 0) {
        TestCases::printAllSuites(1);
        return 0;
    }

    std::vector<SirenaSuite> filteredSuites;
    const int ret = PrepareSuitesToRun(xpList, xpExclude, xpStartFrom, filteredSuites);
    if (ret)
        return ret;

    const size_t tcount = CountTests(filteredSuites);
    
    if (strcmp(xpExclude, "0_count") == 0) {
        printf("%zu\n", tcount);
        return 0;
    }

    if (tcount > 1)
    {
        const char* x = getenv("SIRENA_XP_AUTO_SPEED");
        if(x and *x)
            CutLogHolder::Instance().setLogLevel(atoi(x));
    }
    return RunSuites(filteredSuites,xpFrom) ? 1 : 0;
}

} //namespace TestCases

//-----------------------------------------------------------------------

namespace xp_testing {

std::string maybe_recode_answer(std::string&& answer)
{
    if(std::none_of(answer.begin(), answer.end(), [](char c){ return c & 0x80; }))
        return std::move(answer);
    const char* xp_io = getenv("SIRENA_XP_AUTO_IOCHARSET");
    if(xp_io and (strcmp(xp_io,"1") or strcmp(xp_io,"YES") or strcmp(xp_io,"TRUE")))
    {
        const char* lang = getenv("LANG");
        if(lang and strcmp(lang, "C") == 0)
            return std::move(answer);
        if(const char* dot = strchr(lang,'.'))
            lang = dot + 1;
        iconv_t cd = iconv_open(lang, "cp866");

        if(cd == (iconv_t)-1)
        {
            if(errno == EINVAL)  std::cerr << "iconv : CP866->" << lang << " convertion is not supported" << std::endl;
            return std::move(answer);
        }
        std::string res;
        char to_buf[100];
        while(not answer.empty())
        {
            size_t from_len = answer.size();
            size_t to_len = sizeof(to_buf);
            char* in = const_cast<char*>(answer.data());
            char* out = to_buf;
            size_t r = iconv(cd, &in, &from_len, &out, &to_len);
            res.append(to_buf, sizeof(to_buf) - to_len);
            answer.erase(0, answer.size() - from_len);
            if(r == static_cast<size_t>(-1) and errno != E2BIG)
            {
                if(strcmp(lang,"UTF-8") == 0)  res.append("\xFF\xFD");
                answer.erase(0, 1);
            }
        }
        iconv_close(cd);
        return res;
    }
    return std::move(answer);
}

} // namespace xp_testing

//-----------------------------------------------------------------------

void initTsFuncs();
int main_tests()
{
    init_serverlib_tests();
    initTestMode();
    set_signal(term3);

    initTsFuncs();
    try {

    if (TestCases::run_all_tests() != 0) {
        Tcl_SetErrorCode(getTclInterpretator(), "run_all_tests failed", NULL);
        return TCL_ERROR;
    }
    }
    catch (TestCases::ExceptionToExit const&)
    {
    }

    return TCL_OK;
}

const char* __wtfailed_c_str(const char* s) {  return s;  }

//------------------------------- Begin Tests ----------------------------------

using namespace TestCases;

START_TEST(FixConnectString_test)
{
    std::string newConStr, errMsg = "FixConnectString() failed, newConStr: ";

    newConStr = FixConnectString("user/password", "_", 3);
    fail_if(newConStr!="user_3/password", (errMsg+newConStr).c_str());

    newConStr = FixConnectString("user/password@host", "_", 3);
    fail_if(newConStr!="user_3/password@host", (errMsg+newConStr).c_str());

    newConStr = FixConnectString("amqp://user:password@localhost/user", "_", 3);
    fail_if(newConStr!="amqp://user_3:password@localhost/user_3",
        (errMsg+newConStr).c_str());

    newConStr = FixConnectString("postgres://user:password@localhost/user", "_", 3);
    fail_if(newConStr!="postgres://user_3:password@localhost/user_3",
        (errMsg+newConStr).c_str());

    newConStr = FixConnectString("oracle://user/password", "_", 3);
    fail_if(newConStr!="oracle://user_3/password", (errMsg+newConStr).c_str());
}
END_TEST

#define SUITENAME "Serverlib"
TCASEREGISTER(nullptr,nullptr)
{
    ADD_TEST(FixConnectString_test);
}
TCASEFINISH
#undef SUITENAME

//-------------------------------- End Tests -----------------------------------

#else // XP_TESTING
#include <tcl.h>

namespace TestCases
{
int run_all_tests(void)
{
    return 0;
}
} //namespace TestCases

int main_tests()
{
    return TCL_OK;
}

#endif // XP_TESTING
