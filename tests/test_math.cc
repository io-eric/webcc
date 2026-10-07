#include "webcc/core/math.h"
#include "framework.h"
#include <cmath>
#include <cstdio>

namespace
{
    // relative error, absolute near zero
    double err(double got, double want)
    {
        if (std::isnan(want))
            return std::isnan(got) ? 0 : 1;
        if (std::isinf(want))
            return got == want ? 0 : 1;
        double d = std::fabs(got - want);
        return std::fabs(want) > 1 ? d / std::fabs(want) : d;
    }

    uint64_t g_seed = 12345;
    double rnd(double lo, double hi)
    {
        g_seed = g_seed * 6364136223846793005ull + 1442695040888963407ull;
        return lo + (hi - lo) * ((g_seed >> 11) * 0x1p-53);
    }

    template <typename F, typename G>
    double worst1(F mine, G ref, double lo, double hi, int n = 20000)
    {
        double w = 0;
        for (int i = 0; i < n; i++)
        {
            double x = rnd(lo, hi);
            double e = err(mine(x), ref(x));
            if (e > w)
                w = e;
        }
        return w;
    }
}

TEST(math_trig_matches_libm)
{
    CHECK(worst1([](double x) { return webcc::sin(x); }, [](double x) { return std::sin(x); }, -1e5, 1e5) < 1e-14);
    CHECK(worst1([](double x) { return webcc::cos(x); }, [](double x) { return std::cos(x); }, -1e5, 1e5) < 1e-14);
    CHECK(worst1([](double x) { return webcc::tan(x); }, [](double x) { return std::tan(x); }, -10, 10) < 1e-13);
    CHECK(worst1([](double x) { return webcc::atan(x); }, [](double x) { return std::atan(x); }, -50, 50) < 1e-15);
    CHECK(worst1([](double x) { return webcc::asin(x); }, [](double x) { return std::asin(x); }, -1, 1) < 1e-14);
    CHECK(worst1([](double x) { return webcc::acos(x); }, [](double x) { return std::acos(x); }, -1, 1) < 1e-14);
    double w = 0;
    for (int i = 0; i < 20000; i++)
    {
        double y = rnd(-100, 100), x = rnd(-100, 100);
        double e = err(webcc::atan2(y, x), std::atan2(y, x));
        if (e > w)
            w = e;
    }
    CHECK(w < 1e-15);
    CHECK_EQ(webcc::sin(0.0), 0.0);
    CHECK(webcc::isnan(webcc::sin(__builtin_inf())));
    CHECK_EQ(webcc::atan2(0.0, -1.0), std::atan2(0.0, -1.0));
    CHECK_EQ(webcc::atan2(1.0, 0.0), std::atan2(1.0, 0.0));
    CHECK_EQ(webcc::atan2(-__builtin_inf(), __builtin_inf()), std::atan2(-__builtin_inf(), __builtin_inf()));
}

TEST(math_exp_log_pow_match_libm)
{
    CHECK(worst1([](double x) { return webcc::exp(x); }, [](double x) { return std::exp(x); }, -700, 700) < 1e-15);
    CHECK(worst1([](double x) { return webcc::log(x); }, [](double x) { return std::log(x); }, 1e-300, 1e300) < 1e-15);
    CHECK(worst1([](double x) { return webcc::log(x); }, [](double x) { return std::log(x); }, 0.5, 2) < 1e-15);
    CHECK(worst1([](double x) { return webcc::log2(x); }, [](double x) { return std::log2(x); }, 1e-10, 1e10) < 1e-15);
    CHECK(worst1([](double x) { return webcc::log10(x); }, [](double x) { return std::log10(x); }, 1e-10, 1e10) < 1e-15);
    double w = 0;
    for (int i = 0; i < 20000; i++)
    {
        double x = rnd(0, 50), y = rnd(-20, 20);
        double e = err(webcc::pow(x, y), std::pow(x, y));
        if (e > w)
            w = e;
    }
    CHECK(w < 1e-15);
    CHECK_EQ(webcc::exp(0.0), 1.0);
    CHECK_EQ(webcc::log(1.0), 0.0);
    CHECK_EQ(webcc::log2(8.0), 3.0);
    CHECK_EQ(webcc::log10(1000.0), 3.0);
    CHECK_EQ(webcc::pow(2.0, 10.0), 1024.0);
    CHECK_EQ(webcc::pow(-2.0, 3.0), -8.0);
    CHECK_EQ(webcc::pow(9.0, 0.5), 3.0);
    CHECK(webcc::isnan(webcc::pow(-2.0, 0.5)));
    CHECK(webcc::isnan(webcc::log(-1.0)));
    CHECK(webcc::isinf(webcc::exp(1000.0)));
    CHECK_EQ(webcc::exp(-1000.0), 0.0);
    CHECK(err(webcc::exp(-740.0), std::exp(-740.0)) < 1e-300);
    CHECK(webcc::isinf(webcc::pow(10.0, 308.9)));
    CHECK_EQ(webcc::pow(10.0, -400.5), 0.0);
}

TEST(math_rounding_and_helpers)
{
    CHECK_EQ(webcc::floor(-1.5), -2.0);
    CHECK_EQ(webcc::ceil(2.0), 2.0);
    CHECK_EQ(webcc::ceil(-1.5), -1.0);
    CHECK_EQ(webcc::round(2.5), 3.0);
    CHECK_EQ(webcc::round(-2.5), -3.0);
    CHECK_EQ(webcc::round(0.49999999999999994), 0.0);
    CHECK_EQ(webcc::round(1e300), 1e300);
    CHECK_EQ(webcc::hypot(3.0, 4.0), 5.0);
    CHECK_EQ(webcc::hypot(1e300, 1e300), std::hypot(1e300, 1e300));
    CHECK_EQ(webcc::clamp(5.0, 0.0, 1.0), 1.0);
    CHECK_EQ(webcc::lerp(2.0, 4.0, 0.5), 3.0);
    CHECK(webcc::isnan(webcc::sqrt(-1.0)));
}
