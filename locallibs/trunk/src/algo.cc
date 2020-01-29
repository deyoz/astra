#include "algo.h"

#ifdef XP_TESTING

#include <string>
#include <vector>
#include <list>
#include <set>
#include <map>
#include <cctype>
#include <functional>
#include <boost/optional.hpp>
#include "checkunit.h"

void init_algo_tests() {}

static_assert(!algo::details::is_map<std::vector<int> >::value, "");
static_assert(!algo::details::is_map<std::list<int> >::value, "");
static_assert(!algo::details::is_map<std::set<int> >::value, "");
static_assert(algo::details::is_map<std::map<int, int> >::value, "");

static_assert(algo::details::provides_push_back<std::vector<int> >::value, "");
static_assert(algo::details::provides_push_back<std::list<int> >::value, "");
static_assert(!algo::details::provides_push_back<std::set<int> >::value, "");
static_assert(!algo::details::provides_push_back<std::map<int, int> >::value, "");

static_assert(!algo::details::provides_find<std::vector<int>, int>::value, "");
static_assert(!algo::details::provides_find<std::list<int>, int>::value, "");
static_assert(algo::details::provides_find<std::set<int>, int>::value, "");
static_assert(algo::details::provides_find<std::map<int, std::string>, int>::value, "");
static_assert(!algo::details::provides_find<std::map<int, std::string>, std::string>::value, "");

START_TEST(test_algo_reserve_same_size)
{
    // dst container has reserve(), src container has size()
    {
        struct Container {
            std::size_t capacity = 0;

            int* begin() { return nullptr; }
            int* end() { return nullptr; }
            void reserve(std::size_t n) { capacity = n; }
        } dst;
        algo::details::reserve_same_size(dst, std::vector<int>{ 1, 2, 3});
        fail_unless(dst.capacity == 3);
    }
    // dst container has reserve(), src container without size()
    // should compile without error
    {
        std::vector<int> dst;
        algo::details::reserve_same_size(dst, 0);
    }
    // dst container without reserve(), src container has size()
    // should compile without error
    {
        std::list<int> dst;
        algo::details::reserve_same_size(dst, std::vector<int>{ 1, 2, 3});
    }
    // no reserve() nor size()
    // should compile without error
    {
        int dst = 0;
        algo::details::reserve_same_size(dst, 0);
    }
}
END_TEST

START_TEST(test_algo_end_inserter)
{
    // container with push_back and insert
    {
        struct Container {
            using value_type = int;
            using iterator = int*;

            int push_back_count = 0;
            int insert_count = 0;

            iterator begin() { return nullptr; }
            iterator end() { return nullptr; }
            void push_back(int) { ++push_back_count; }
            iterator insert(iterator, int) { ++insert_count; return nullptr; }
        } container;
        auto it = algo::details::end_inserter(container);
        it = 0;
        fail_unless(container.push_back_count == 1);
        fail_unless(container.insert_count == 0);
    }
    // container insert without push_back
    {
        struct Container {
            using value_type = int;
            using iterator = int*;

            int insert_count = 0;

            iterator begin() { return nullptr; }
            iterator end() { return nullptr; }
            iterator insert(iterator, int) { ++insert_count; return nullptr; }
        } container;
        auto it = algo::details::end_inserter(container);
        it = 0;
        fail_unless(container.insert_count == 1);
    }
}
END_TEST

START_TEST(test_algo_transform)
{
    const auto plus1 = [](int n) { return n + 1; };

    // vector<int> -> vector<int>
    {
        const std::vector<int> v { 1, 2, 3 };
        const auto t = algo::transform(v, plus1);
        fail_unless((t == std::vector<int>{ 2, 3, 4 }));
    }
    // vector<int> -> vector<string>
    {
        const std::vector<int> v { 1, 2, 3 };
        const auto t = algo::transform(v, [](int n) { return std::to_string(n); });
        fail_unless((t == std::vector<std::string>{ "1", "2", "3" }));
    }
    // list<int> -> list<int>
    {
        const std::list<int> v { 1, 2, 3 };
        const auto t = algo::transform(v, plus1);
        fail_unless((t == std::list<int>{ 2, 3, 4 }));
    }
    // list<int> -> list<string>
    {
        const std::list<int> v { 1, 2, 3 };
        const auto t = algo::transform(v, [](int n) { return std::to_string(n); });
        fail_unless((t == std::list<std::string>{ "1", "2", "3" }));
    }
    // string -> string
    {
        const std::string t = algo::transform(std::string("abc"),
                [](char c) -> char { return c + 1; });
        fail_unless(t == "bcd");
    }
    // std::map<int, std::string> -> std::map<int, std::string>
    {
        using Map = std::map<int, std::string>;
        const Map m { { 1, "a" }, { 2, "b" }, { 3, "c" } };
        const auto t = algo::transform(m,
                [](const Map::value_type& kv) {
                    return Map::value_type(kv.first + 1, kv.second);
                });
        const Map expected { { 2, "a" }, { 3, "b" }, { 4, "c" } };
        fail_unless(t == expected);
    }
    // std::map<int, std::string> -> std::map<std::string, int>
    {
        using Map = std::map<int, std::string>;
        using ResultMap = std::map<std::string, int>;
        const Map m { { 1, "a" }, { 2, "b" }, { 3, "c" } };
        const auto t = algo::transform<ResultMap>(m,
                [](const Map::value_type& kv) {
                    return ResultMap::value_type(kv.second, kv.first);
                });
        const ResultMap expected { { "a", 1 }, { "b", 2 }, { "c", 3 } };
        fail_unless(t == expected);
    }

    //--- repacking to a different type of a container ---

    // set<int> -> vector<int>
    {
        const std::set<int> v { 1, 2, 3 };
        const auto t = algo::transform<std::vector>(v, plus1);
        fail_unless((t == std::vector<int>{ 2, 3, 4 }));
    }
    // vector<int> -> list<int>
    {
        const std::vector<int> v { 1, 2, 3 };
        const auto t = algo::transform<std::list>(v, plus1);
        fail_unless((t == std::list<int>{ 2, 3, 4 }));
    }
    // vector<int> -> set<int>
    {
        const std::vector<int> v { 1, 2, 3 };
        const auto t = algo::transform<std::set>(v, plus1);
        fail_unless((t == std::set<int>{ 2, 3, 4 }));
    }
    // list<int> -> vector<string>
    {
        const std::list<int> v { 1, 2, 3 };
        const auto t = algo::transform<std::vector>(v, [](int n) { return std::to_string(n); });
        fail_unless((t == std::vector<std::string>{ "1", "2", "3" }));
    }
    // std::map<int, std::string> -> std::set<int>
    {
        using Map = std::map<int, std::string>;
        const Map m { { 1, "a" }, { 2, "b" }, { 3, "c" } };
        const auto t = algo::transform<std::set>(m,
                [](const Map::value_type& kv) { return kv.first; });
        fail_unless((t == std::set<int>{ 1, 2, 3 }));
    }

    //--- array and initializer_list ---

    // array
    {
        const int v[] = { 1, 2, 3 };
        const auto t = algo::transform<std::vector>(v, plus1);
        fail_unless((t == std::vector<int>{ 2, 3, 4 }));
    }
    // initializer_list
    {
        const auto v = { 1, 2, 3 };
        const auto t = algo::transform<std::vector>(v, plus1);
        fail_unless((t == std::vector<int>{ 2, 3, 4 }));
    }
    // initializer_list rvalue
    {
        const auto t = algo::transform<std::vector>({1, 2, 3}, plus1);
        fail_unless((t == std::vector<int>{ 2, 3, 4 }));
    }
}
END_TEST

START_TEST(test_algo_filter)
{
    const auto is_odd = [](int n) { return n & 1; };

    // vector<int> -> vector<int>
    {
        const std::vector<int> v { 1, 2, 3, 4, 5 };
        const auto t = algo::filter(v, is_odd);
        fail_unless((t == std::vector<int>{ 1, 3, 5 }));
    }
    // list<int> -> list<int>
    {
        const std::list<int> v { 1, 2, 3, 4, 5 };
        const auto t = algo::filter(v, is_odd);
        fail_unless((t == std::list<int>{ 1, 3, 5 }));
    }
    // string -> string
    {
        const std::string t = algo::filter(std::string("aBcDeF"),
                [](char c) { return std::islower(c); });
        fail_unless(t == "ace");
    }
    // std::map<int, std::string> -> std::map<int, std::string>
    {
        using Map = std::map<int, std::string>;
        const Map m { { 1, "a" }, { 2, "b" }, { 3, "c" } };
        const auto t = algo::filter(m,
                [](const Map::value_type& kv) { return kv.first > 1; });
        const Map expected { { 2, "b" }, { 3, "c" } };
        fail_unless(t == expected);
    }

    //--- repacking to a different type of a container ---

    // set<int> -> vector<int>
    {
        const std::set<int> v { 1, 2, 3, 4, 5 };
        const auto t = algo::filter<std::vector>(v, is_odd);
        fail_unless((t == std::vector<int>{ 1, 3, 5 }));
    }
    // vector<int> -> list<int>
    {
        const std::vector<int> v { 1, 2, 3, 4, 5 };
        const auto t = algo::filter<std::list>(v, is_odd);
        fail_unless((t == std::list<int>{ 1, 3, 5 }));
    }
    // vector<int> -> set<int>
    {
        const std::vector<int> v { 1, 2, 3, 4, 5 };
        const auto t = algo::filter<std::set>(v, is_odd);
        fail_unless((t == std::set<int>{ 1, 3, 5 }));
    }

    //--- array and initializer_list ---

    // array
    {
        const int v[] = { 1, 2, 3, 4, 5 };
        const auto t = algo::filter<std::vector>(v, is_odd);
        fail_unless((t == std::vector<int>{ 1, 3, 5 }));
    }
    // initializer_list
    {
        const auto v = { 1, 2, 3, 4, 5 };
        const auto t = algo::filter<std::vector>(v, is_odd);
        fail_unless((t == std::vector<int>{ 1, 3, 5 }));
    }
    // initializer_list rvalue
    {
        const auto t = algo::filter<std::vector>({ 1, 2, 3, 4, 5 }, is_odd);
        fail_unless((t == std::vector<int>{ 1, 3, 5 }));
    }
}
END_TEST

START_TEST(test_algo_sort)
{
    // without custom compare
    {
        const std::vector<int> v { 3, 2, 5, 4, 1 };
        const auto t = algo::sort(v);
        fail_unless((t == std::vector<int>{ 1, 2, 3, 4, 5 }));
    }
    // with custom compare
    {
        const std::vector<int> v { 3, 2, 5, 4, 1 };
        const auto t = algo::sort(v, [](int a, int b) { return a > b; });
        fail_unless((t == std::vector<int>{ 5, 4, 3, 2, 1 }));
    }
}
END_TEST

START_TEST(test_algo_combine)
{
    // homogeneous combine (two containers)
    {
        const std::vector<int> v1 { 1, 2, 3 };
        const std::vector<int> v2 { 4, 5 };
        const auto t = algo::combine(v1, v2);
        fail_unless((t == std::vector<int>{ 1, 2, 3, 4, 5 }));
    }
    // heterogeneous combine (many containers, rvalues)
    {
        const auto t = algo::combine(
                std::list<int>{ 1, 2 },
                std::vector<int>{ 3, 4 },
                std::set<int>{ 5, 6 });
        fail_unless((t == std::list<int>{ 1, 2, 3, 4, 5, 6 }));
    }
}
END_TEST

START_TEST(test_algo_append)
{
    // make a long string to avoid [possible] std::string
    // small string optimization;
    // may help to verify non-destruction of a src container
    const auto s = [](int n) {
        const std::string tmp = std::to_string(n);
        std::string result;
        for (int i = 0; i < 1024; ++i)
            result += tmp;
        return result;
    };

    // append const
    {
        std::vector<int> v1 { 1, 2, 3 };
        const std::vector<int> v2 { 4, 5 };
        algo::append(v1, v2);
        fail_unless((v1 == std::vector<int>{ 1, 2, 3, 4, 5 }));
    }
    // append non-const
    {
        std::vector<std::string> v1 { s(1), s(2), s(3) };
        std::vector<std::string> v2 { s(4), s(5) };
        algo::append(v1, v2);
        fail_unless((v1 == std::vector<std::string>{ s(1), s(2), s(3), s(4), s(5) }));
        fail_unless((v2 == std::vector<std::string>{ s(4), s(5) }));
    }
    // append non-const to an empty vector
    {
        std::vector<std::string> v1;
        std::vector<std::string> v2 { s(1), s(2), s(3) };
        algo::append(v1, v2);
        fail_unless((v1 == std::vector<std::string>{ s(1), s(2), s(3) }));
        fail_unless((v2 == std::vector<std::string>{ s(1), s(2), s(3) }));
    }
    // append rvalue
    {
        std::vector<int> v1 { 1, 2, 3 };
        std::vector<int> v2 { 4, 5 };
        algo::append(v1, std::move(v2));
        fail_unless((v1 == std::vector<int>{ 1, 2, 3, 4, 5 }));
    }

    //----- append a different type of container -----

    // append const
    {
        std::vector<int> v1 { 1, 2, 3 };
        const std::list<int> v2 { 4, 5 };
        algo::append(v1, v2);
        fail_unless((v1 == std::vector<int>{ 1, 2, 3, 4, 5 }));
    }
    // append non-const
    {
        std::vector<std::string> v1 { s(1), s(2), s(3) };
        std::list<std::string> v2 { s(4), s(5) };
        algo::append(v1, v2);
        fail_unless((v1 == std::vector<std::string>{ s(1), s(2), s(3), s(4), s(5) }));
        fail_unless((v2 == std::list<std::string>{ s(4), s(5) }));
    }
    // append non-const to an empty vector
    {
        std::vector<std::string> v1;
        std::list<std::string> v2 { s(1), s(2), s(3) };
        algo::append(v1, v2);
        fail_unless((v1 == std::vector<std::string>{ s(1), s(2), s(3) }));
        fail_unless((v2 == std::list<std::string>{ s(1), s(2), s(3) }));
    }
    // append rvalue
    {
        std::vector<int> v1 { 1, 2, 3 };
        std::list<int> v2 { 4, 5 };
        algo::append(v1, std::move(v2));
        fail_unless((v1 == std::vector<int>{ 1, 2, 3, 4, 5 }));
    }
}
END_TEST

START_TEST(test_algo_all_of)
{
    const std::vector<int> v { 1, 2, 3, 4, 5 };
    const bool r1 = algo::all_of(v, [](int n) { return n > 0; });
    const bool r2 = algo::all_of(v, [](int n) { return n > 1; });
    fail_unless(r1);
    fail_unless(!r2);
}
END_TEST

START_TEST(test_algo_any_of)
{
    const std::vector<int> v { 1, 2, 3, 4, 5 };
    const bool r1 = algo::any_of(v, [](int n) { return n == 3; });
    const bool r2 = algo::any_of(v, [](int n) { return n == 6; });
    fail_unless(r1);
    fail_unless(!r2);
}
END_TEST

START_TEST(test_algo_none_of)
{
    const std::vector<int> v { 1, 2, 3, 4, 5 };
    const bool r1 = algo::none_of(v, [](int n) { return n < 1; });
    const bool r2 = algo::none_of(v, [](int n) { return n == 3; });
    fail_unless(r1);
    fail_unless(!r2);
}
END_TEST

START_TEST(test_algo_count)
{
    const std::vector<int> v { 1, 2, 2, 2, 5 };
    const auto r = algo::count(v, 2);
    fail_unless(r == 3);
}
END_TEST

START_TEST(test_algo_count_if)
{
    const std::vector<int> v { 1, 2, 2, 2, 5 };
    const auto r = algo::count_if(v, [](int n) { return n == 2; });
    fail_unless(r == 3);
}
END_TEST

START_TEST(test_algo_accumulate)
{
    // default binary operation "+"
    {
        const std::vector<int> v { 1, 2, 3 };
        const auto r = algo::accumulate(v, 1);
        fail_unless(r == 7);
    }
    // custom binary operation
    {
        const std::vector<std::string> v { "a", "b", "c" };
        const auto r = algo::accumulate(v,
                std::string("init"),
                [](const std::string& a, const std::string& b) {
                    return a + "," + b;
                });
        fail_unless(r == "init,a,b,c");
    }
}
END_TEST

START_TEST(test_algo_contains)
{
    {
        const std::vector<int> v { 1, 2, 3, 4, 5 };
        const bool r1 = algo::contains(v, 3);
        const bool r2 = algo::contains(v, 6);
        fail_unless(r1);
        fail_unless(!r2);
    }
    // if a container provides find() method, call it
    {
        const struct Container1 {
            const int front = 0;
            using const_iterator = const int*;
            const_iterator begin() const { return &front; }
            const_iterator end() const { return nullptr; }
            const_iterator find(int) const { return begin(); }
        } container1 = {};
        const struct Container2 {
            const int front = 0;
            using const_iterator = const int*;
            const_iterator begin() const { return &front; }
            const_iterator end() const { return nullptr; }
            const_iterator find(int) const { return end(); }
        } container2 = {};
        static_assert(algo::details::provides_find<Container1, int>::value, "");
        static_assert(algo::details::provides_find<Container2, int>::value, "");
        const bool r1 = algo::contains(container1, 0);
        const bool r2 = algo::contains(container2, 0);
        fail_unless(r1);
        fail_unless(!r2);
    }
    // search in an array
    {
        const int v[] = { 1, 2, 3, 4, 5 };
        const bool r1 = algo::contains(v, 3);
        const bool r2 = algo::contains(v, 6);
        fail_unless(r1);
        fail_unless(!r2);
    }
    // search in initializer_list variable
    {
        const auto v = { 1, 2, 3, 4, 5 };
        const bool r1 = algo::contains(v, 3);
        const bool r2 = algo::contains(v, 6);
        fail_unless(r1);
        fail_unless(!r2);
    }
    // search in initializer_list temporary
    {
        const bool r1 = algo::contains({ 1, 2, 3, 4, 5 }, 3);
        const bool r2 = algo::contains({ 1, 2, 3, 4, 5 }, 6);
        fail_unless(r1);
        fail_unless(!r2);
    }
    // search in std::map by key
    {
        using Map = std::map<int, std::string>;
        const Map m { { 1, "a" }, { 2, "b" }, { 3, "c" } };
        const bool r1 = algo::contains(m, 2);
        const bool r2 = algo::contains(m, 4);
        fail_unless(r1);
        fail_unless(!r2);
    }
    // linear search in std::map by key+value
    {
        using Map = std::map<int, std::string>;
        const Map m { { 1, "a" }, { 2, "b" }, { 3, "c" } };
        const bool r1 = algo::contains(m, Map::value_type(1, "a"));
        const bool r2 = algo::contains(m, Map::value_type(4, "c"));
        fail_unless(r1);
        fail_unless(!r2);
    }
}
END_TEST

START_TEST(test_algo_find_opt)
{
    // std::vector
    {
        const std::vector<int> v { 1, 2, 3, 4, 5 };
        const auto r1 = algo::find_opt<boost::optional>(v, 3);
        const auto r2 = algo::find_opt<boost::optional>(v, 6);
        fail_unless(r1 == boost::optional<int>(3));
        fail_unless(r2 == boost::optional<int>());
    }
    // std::set
    {
        const std::vector<int> v { 1, 2, 3, 4, 5 };
        const auto r1 = algo::find_opt<boost::optional>(v, 3);
        const auto r2 = algo::find_opt<boost::optional>(v, 6);
        fail_unless(r1 == boost::optional<int>(3));
        fail_unless(r2 == boost::optional<int>());
    }
    // array
    {
        const int v[] = { 1, 2, 3, 4, 5 };
        const auto r1 = algo::find_opt<boost::optional>(v, 3);
        const auto r2 = algo::find_opt<boost::optional>(v, 6);
        fail_unless(r1 == boost::optional<int>(3));
        fail_unless(r2 == boost::optional<int>());
    }
    // std::map
    {
        const std::map<int, std::string> m = {
            { 1, "a" },
            { 2, "b" },
            { 3, "c" }
        };
        const auto r1 = algo::find_opt<boost::optional>(m, 2);
        const auto r2 = algo::find_opt<boost::optional>(m, 4);
        fail_unless(r1 == boost::optional<std::string>("b"));
        fail_unless(r2 == boost::optional<std::string>());
    }
}
END_TEST

START_TEST(test_algo_find_opt_if)
{
    const auto c1 = [](int n) { return n > 2; };
    const auto c2 = [](int n) { return n > 5; };
    // std::vector
    {
        const std::vector<int> v { 1, 2, 3, 4, 5 };
        const auto r1 = algo::find_opt_if<boost::optional>(v, c1);
        const auto r2 = algo::find_opt_if<boost::optional>(v, c2);
        fail_unless(r1 == boost::optional<int>(3));
        fail_unless(r2 == boost::optional<int>());
    }
    // std::set
    {
        const std::vector<int> v { 1, 2, 3, 4, 5 };
        const auto r1 = algo::find_opt_if<boost::optional>(v, c1);
        const auto r2 = algo::find_opt_if<boost::optional>(v, c2);
        fail_unless(r1 == boost::optional<int>(3));
        fail_unless(r2 == boost::optional<int>());
    }
    // array
    {
        const int v[] = { 1, 2, 3, 4, 5 };
        const auto r1 = algo::find_opt_if<boost::optional>(v, c1);
        const auto r2 = algo::find_opt_if<boost::optional>(v, c2);
        fail_unless(r1 == boost::optional<int>(3));
        fail_unless(r2 == boost::optional<int>());
    }
}
END_TEST

START_TEST(test_algo_is_sorted)
{
    const std::vector<int> v1 { 1, 2, 3 };
    const std::vector<int> v2 { 3, 2, 1 };
    fail_unless(algo::is_sorted(v1));
    fail_unless(!algo::is_sorted(v2));
    fail_unless(!algo::is_sorted(v1, std::greater<int>()));
    fail_unless(algo::is_sorted(v2, std::greater<int>()));
}
END_TEST;

#define SUITENAME "Serverlib"
TCASEREGISTER(0, 0)
{
    ADD_TEST(test_algo_reserve_same_size);
    ADD_TEST(test_algo_end_inserter);
    ADD_TEST(test_algo_transform);
    ADD_TEST(test_algo_filter);
    ADD_TEST(test_algo_sort);
    ADD_TEST(test_algo_combine);
    ADD_TEST(test_algo_append);
    ADD_TEST(test_algo_all_of);
    ADD_TEST(test_algo_any_of);
    ADD_TEST(test_algo_none_of);
    ADD_TEST(test_algo_count);
    ADD_TEST(test_algo_count_if);
    ADD_TEST(test_algo_accumulate);
    ADD_TEST(test_algo_contains);
    ADD_TEST(test_algo_find_opt);
    ADD_TEST(test_algo_find_opt_if);
    ADD_TEST(test_algo_is_sorted);
}
TCASEFINISH

#endif // ifdef XP_TESTING
