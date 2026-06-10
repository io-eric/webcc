// Tiny zero-dependency test framework for WebCC.
//
// Usage:
//   TEST(my_test_name) {
//       CHECK(some_condition);
//       CHECK_EQ(actual, expected);
//   }
//
// Tests self-register at static-init time. The `tests` binary (test_main.cc)
// calls webcc_test::run_all() to execute them all and returns non-zero on any
// failure. No macros leak a main(); link as many test_*.cc files as you like.
#pragma once

#include <cstdint>
#include <functional>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace webcc_test
{
    struct TestCase
    {
        std::string name;
        std::function<void()> fn;
    };

    // Per-run state. Failures are appended here by the CHECK macros.
    struct Context
    {
        std::vector<std::string> failures;
        std::string current_test;
    };

    inline std::vector<TestCase> &registry()
    {
        static std::vector<TestCase> r;
        return r;
    }

    inline Context &ctx()
    {
        static Context c;
        return c;
    }

    inline int register_test(const std::string &name, std::function<void()> fn)
    {
        registry().push_back({name, std::move(fn)});
        return 0;
    }

    inline void record_failure(const std::string &msg)
    {
        ctx().failures.push_back(ctx().current_test + ": " + msg);
    }

    inline int run_all()
    {
        int passed = 0;
        int failed = 0;
        for (auto &tc : registry())
        {
            ctx().current_test = tc.name;
            size_t before = ctx().failures.size();
            try
            {
                tc.fn();
            }
            catch (const std::exception &e)
            {
                record_failure(std::string("threw exception: ") + e.what());
            }
            catch (...)
            {
                record_failure("threw unknown exception");
            }
            bool ok = ctx().failures.size() == before;
            std::cout << (ok ? "  PASS  " : "  FAIL  ") << tc.name << "\n";
            ok ? ++passed : ++failed;
        }

        std::cout << "\n";
        if (failed)
        {
            std::cout << "Failures:\n";
            for (const auto &f : ctx().failures)
                std::cout << "  - " << f << "\n";
            std::cout << "\n";
        }
        std::cout << passed << " passed, " << failed << " failed, "
                  << registry().size() << " total.\n";
        return failed == 0 ? 0 : 1;
    }

    // Stringify helper used by CHECK_EQ.
    template <typename T>
    std::string to_str(const T &v)
    {
        std::ostringstream ss;
        ss << v;
        return ss.str();
    }
} // namespace webcc_test

#define WEBCC_CAT_(a, b) a##b
#define WEBCC_CAT(a, b) WEBCC_CAT_(a, b)

#define TEST(name)                                                                \
    static void name();                                                           \
    static int WEBCC_CAT(name, _reg) =                                            \
        ::webcc_test::register_test(#name, name);                                 \
    static void name()

#define CHECK(cond)                                                               \
    do                                                                            \
    {                                                                             \
        if (!(cond))                                                              \
        {                                                                         \
            ::webcc_test::record_failure(                                         \
                std::string("CHECK failed: " #cond " (") + __FILE__ + ":" +       \
                std::to_string(__LINE__) + ")");                                  \
        }                                                                         \
    } while (0)

#define CHECK_EQ(actual, expected)                                                \
    do                                                                            \
    {                                                                             \
        auto _a = (actual);                                                       \
        auto _e = (expected);                                                     \
        if (!(_a == _e))                                                          \
        {                                                                         \
            ::webcc_test::record_failure(                                         \
                std::string("CHECK_EQ failed: " #actual " == " #expected          \
                            " (got '") +                                          \
                ::webcc_test::to_str(_a) + "', expected '" +                      \
                ::webcc_test::to_str(_e) + "') (" + __FILE__ + ":" +              \
                std::to_string(__LINE__) + ")");                                  \
        }                                                                         \
    } while (0)
