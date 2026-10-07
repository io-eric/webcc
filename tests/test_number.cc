#include "webcc/core/number.h"
#include "webcc/core/format.h"
#include "webcc/core/string.h"
#include "framework.h"
#include <charconv>
#include <cstdlib>
#include <cstring>
#include <string>

namespace
{
    std::string fmt(double v)
    {
        char b[32];
        webcc::format_double(v, b);
        return b;
    }

    std::string fixed(double v, int places)
    {
        char b[48];
        webcc::format_fixed(v, places, b);
        return b;
    }

    double parse(const char *s)
    {
        uint32_t used;
        return webcc::parse_double(s, (uint32_t)std::strlen(s), used);
    }

    uint64_t g_state = 99;
    uint64_t next()
    {
        g_state = g_state * 6364136223846793005ull + 1442695040888963407ull;
        return g_state ^ (g_state >> 29);
    }
}

TEST(number_format_like_js)
{
    CHECK_EQ(fmt(0.1), std::string("0.1"));
    CHECK_EQ(fmt(0.1 + 0.2), std::string("0.30000000000000004"));
    CHECK_EQ(fmt(1.0), std::string("1"));
    CHECK_EQ(fmt(-0.5), std::string("-0.5"));
    CHECK_EQ(fmt(-0.0), std::string("0"));
    CHECK_EQ(fmt(1e21), std::string("1e+21"));
    CHECK_EQ(fmt(123456789012345680000.0), std::string("123456789012345680000"));
    CHECK_EQ(fmt(1e-7), std::string("1e-7"));
    CHECK_EQ(fmt(1.5e-7), std::string("1.5e-7"));
    CHECK_EQ(fmt(0.000001), std::string("0.000001"));
    CHECK_EQ(fmt(5e-324), std::string("5e-324"));
    CHECK_EQ(fmt(1.7976931348623157e308), std::string("1.7976931348623157e+308"));
    CHECK_EQ(fmt(__builtin_inf()), std::string("Infinity"));
    CHECK_EQ(fmt(-__builtin_inf()), std::string("-Infinity"));
    CHECK_EQ(fmt(__builtin_nan("")), std::string("NaN"));
    char b[32];
    webcc::format_float(1.1f, b);
    CHECK_EQ(std::string(b), std::string("1.1"));
}

TEST(number_format_fixed)
{
    CHECK_EQ(fixed(0.999, 2), std::string("1.00"));
    CHECK_EQ(fixed(-0.5, 2), std::string("-0.50"));
    CHECK_EQ(fixed(2.675, 2), std::string("2.67"));
    CHECK_EQ(fixed(1234567.891, 2), std::string("1234567.89"));
    CHECK_EQ(fixed(-0.001, 2), std::string("0.00"));
    CHECK_EQ(fixed(3.0, 0), std::string("3"));
    CHECK_EQ(fixed(5e9, 1), std::string("5000000000.0"));
}

TEST(number_parse)
{
    CHECK_EQ(parse("0.1"), 0.1);
    CHECK_EQ(parse("-12.5e3"), -12500.0);
    CHECK_EQ(parse("1E-7"), 1e-7);
    CHECK_EQ(parse(".5"), 0.5);
    CHECK_EQ(parse("000123.4500"), 123.45);
    CHECK_EQ(parse("4.9406564584124654e-324"), 5e-324);
    CHECK_EQ(parse("1e400"), __builtin_inf());
    CHECK_EQ(parse("1e-400"), 0.0);
    CHECK_EQ(parse("95770568.6863700374981365999016"), 95770568.686370045);
    uint32_t used;
    webcc::parse_double("12px", 4, used);
    CHECK_EQ(used, 2u);
    webcc::parse_double("abc", 3, used);
    CHECK_EQ(used, 0u);
    CHECK_EQ(webcc::string(" 3.25x").to_float(), 3.25);
}

TEST(number_round_trips_shortest)
{
    int bad = 0, longer = 0;
    char a[32], b[64];
    for (int i = 0; i < 200000; i++)
    {
        uint64_t bits = next();
        double v;
        std::memcpy(&v, &bits, 8);
        if (v != v || v - v != 0)
            continue;
        webcc::format_double(v, a);
        if (std::strtod(a, nullptr) != v)
            bad++;
        if (parse(a) != v)
            bad++;
        auto r = std::to_chars(b, b + 64, v);
        *r.ptr = 0;
        if (parse(b) != v)
            bad++;
        auto sig = [](const char *s) {
            std::string t;
            bool st = false;
            for (; *s && *s != 'e'; s++)
            {
                if (*s >= '1' && *s <= '9')
                    st = true;
                if (st && *s >= '0' && *s <= '9')
                    t += *s;
            }
            while (!t.empty() && t.back() == '0')
                t.pop_back();
            return t.size();
        };
        if (sig(a) > sig(b))
            longer++;
    }
    CHECK_EQ(bad, 0);
    CHECK_EQ(longer, 0);
}

TEST(formatter_prints_numbers_in_full)
{
    webcc::formatter<64> f;
    f << 1234567.89 << " " << -0.5 << " " << 3.0 << " " << webcc::precision(0.999, 2);
    CHECK(std::strcmp(f.c_str(), "1234567.89 -0.5 3 1.00") == 0);
}
