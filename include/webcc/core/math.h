#pragma once

#include <stdint.h>
#include <stddef.h>

namespace webcc
{
    // --- Constants ---
    static constexpr float PI = 3.14159265358979323846f;
    static constexpr float HALF_PI = 1.57079632679489661923f;
    static constexpr float TAU = 6.28318530717958647692f;
    static constexpr float DEG2RAD = PI / 180.0f;
    static constexpr float RAD2DEG = 180.0f / PI;

    // --- Basic Math ---
    // double precision, freestanding: polynomials from fdlibm
    namespace detail
    {
        inline uint64_t bits(double x) { return __builtin_bit_cast(uint64_t, x); }
        inline double from_bits(uint64_t b) { return __builtin_bit_cast(double, b); }
    }

    inline bool isnan(double x) { return x != x; }
    inline bool isinf(double x) { return (detail::bits(x) << 1) == 0xffe0000000000000ull; }
    inline double abs(double x) { return __builtin_fabs(x); }
    inline double sqrt(double x) { return __builtin_sqrt(x); }
    inline double floor(double x) { return __builtin_floor(x); }
    inline double ceil(double x) { return __builtin_ceil(x); }
    inline double trunc(double x) { return __builtin_trunc(x); }
    inline double copysign(double x, double y) { return __builtin_copysign(x, y); }

    // half away from zero
    inline double round(double x)
    {
        double t = trunc(x);
        if (abs(x - t) >= 0.5)
            t += copysign(1.0, x);
        return t;
    }

    inline double min(double a, double b) { return a < b ? a : b; }
    inline double max(double a, double b) { return a > b ? a : b; }
    inline double clamp(double v, double lo, double hi) { return v < lo ? lo : (v > hi ? hi : v); }
    inline double lerp(double a, double b, double t) { return a + (b - a) * t; }

    inline double scalbn(double x, int n)
    {
        if (n > 1023)
        {
            x *= 0x1p1023;
            n -= 1023;
            if (n > 1023)
            {
                x *= 0x1p1023;
                n -= 1023;
                if (n > 1023)
                    n = 1023;
            }
        }
        else if (n < -1022)
        {
            x *= 0x1p-1022 * 0x1p53;
            n += 1022 - 53;
            if (n < -1022)
            {
                x *= 0x1p-1022 * 0x1p53;
                n += 1022 - 53;
                if (n < -1022)
                    n = -1022;
            }
        }
        return x * detail::from_bits((uint64_t)(0x3ff + n) << 52);
    }

    inline double hypot(double x, double y)
    {
        x = abs(x);
        y = abs(y);
        if (isinf(x) || isinf(y))
            return __builtin_inf();
        double m = max(x, y);
        if (m == 0 || isnan(m))
            return x + y;
        x /= m;
        y /= m;
        return m * sqrt(x * x + y * y);
    }

    // --- Trigonometry ---
    namespace detail
    {
        // x = k*pi/2 + r, |r| <= ~pi/4
        inline double rem_pio2(double x, int &k)
        {
            double n = round(x * 6.36619772367581382433e-01);
            k = (int)((int64_t)n & 3);
            return ((x - n * 1.57079632673412561417e+00) - n * 6.07710050630396597660e-11) - n * 2.02226624871116645580e-21;
        }

        inline double sin_kernel(double r)
        {
            double z = r * r;
            return r + r * z * (-1.66666666666666324348e-01 + z * (8.33333333332248946124e-03 + z * (-1.98412698298579493134e-04 + z * (2.75573137070700676789e-06 + z * (-2.50507602534068634195e-08 + z * 1.58969099521155010221e-10)))));
        }

        inline double cos_kernel(double r)
        {
            double z = r * r;
            double p = z * z * (4.16666666666666019037e-02 + z * (-1.38888888888741095749e-03 + z * (2.48015872894767294178e-05 + z * (-2.75573143513906633035e-07 + z * (2.08757232129817482790e-09 + z * -1.13596475577881948265e-11)))));
            double hz = 0.5 * z;
            double w = 1.0 - hz;
            return w + (((1.0 - w) - hz) + p);
        }
    }

    inline double sin(double x)
    {
        if (isnan(x) || isinf(x))
            return __builtin_nan("");
        int k;
        double r = detail::rem_pio2(x, k);
        switch (k)
        {
        case 0: return detail::sin_kernel(r);
        case 1: return detail::cos_kernel(r);
        case 2: return -detail::sin_kernel(r);
        default: return -detail::cos_kernel(r);
        }
    }

    inline double cos(double x)
    {
        if (isnan(x) || isinf(x))
            return __builtin_nan("");
        int k;
        double r = detail::rem_pio2(x, k);
        switch (k)
        {
        case 0: return detail::cos_kernel(r);
        case 1: return -detail::sin_kernel(r);
        case 2: return -detail::cos_kernel(r);
        default: return detail::sin_kernel(r);
        }
    }

    inline double tan(double x)
    {
        if (isnan(x) || isinf(x))
            return __builtin_nan("");
        int k;
        double r = detail::rem_pio2(x, k);
        double s = detail::sin_kernel(r), c = detail::cos_kernel(r);
        return (k & 1) ? -c / s : s / c;
    }

    inline double atan(double x)
    {
        static constexpr double hi[] = {4.63647609000806093515e-01, 7.85398163397448278999e-01, 9.82793723247329054082e-01, 1.57079632679489655800e+00};
        static constexpr double lo[] = {2.26987774529616870924e-17, 3.06161699786838301793e-17, 1.39033110312309984516e-17, 6.12323399573676603587e-17};
        if (isnan(x))
            return x;
        double ax = abs(x);
        if (ax >= 0x1p66)
            return copysign(hi[3] + lo[3], x);
        int id;
        if (ax < 0.4375)
        {
            if (ax < 0x1p-27)
                return x;
            id = -1;
        }
        else if (ax < 1.1875)
        {
            if (ax < 0.6875) { id = 0; ax = (2.0 * ax - 1.0) / (2.0 + ax); }
            else { id = 1; ax = (ax - 1.0) / (ax + 1.0); }
        }
        else if (ax < 2.4375) { id = 2; ax = (ax - 1.5) / (1.0 + 1.5 * ax); }
        else { id = 3; ax = -1.0 / ax; }
        double z = ax * ax, w = z * z;
        double s1 = z * (3.33333333333329318027e-01 + w * (1.42857142725034663711e-01 + w * (9.09088713343650656196e-02 + w * (6.66107313738753120669e-02 + w * (4.97687799461593236017e-02 + w * 1.62858201153657823623e-02)))));
        double s2 = w * (-1.99999999998764832476e-01 + w * (-1.11111104054623557880e-01 + w * (-7.69187620504482999495e-02 + w * (-5.83357013379057348645e-02 + w * -3.65315727442169155270e-02))));
        if (id < 0)
            return x - x * (s1 + s2);
        double r = hi[id] - ((ax * (s1 + s2) - lo[id]) - ax);
        return copysign(r, x);
    }

    inline double atan2(double y, double x)
    {
        constexpr double pi = 3.1415926535897931160E+00, pi_lo = 1.2246467991473531772E-16;
        if (isnan(x) || isnan(y))
            return x + y;
        if (x == 1.0)
            return atan(y);
        bool xneg = __builtin_signbit(x);
        if (y == 0)
            return xneg ? copysign(pi, y) : y;
        if (x == 0)
            return copysign(pi / 2, y);
        if (isinf(x))
        {
            if (isinf(y))
                return copysign(xneg ? 3 * pi / 4 : pi / 4, y);
            return xneg ? copysign(pi, y) : copysign(0.0, y);
        }
        if (isinf(y))
            return copysign(pi / 2, y);
        double z;
        double ratio = abs(y / x);
        if (ratio > 0x1p60)
            z = pi / 2 + 0.5 * pi_lo;
        else if (xneg && ratio < 0x1p-60)
            z = 0;
        else
            z = atan(ratio);
        if (xneg)
            z = pi - (z - pi_lo);
        return copysign(z, y);
    }

    inline double asin(double x) { return atan2(x, sqrt((1.0 - x) * (1.0 + x))); }
    inline double acos(double x) { return atan2(sqrt((1.0 - x) * (1.0 + x)), x); }

    // --- Exponential ---
    inline double exp(double x)
    {
        constexpr double ln2_hi = 6.93147180369123816490e-01, ln2_lo = 1.90821492927058770002e-10;
        if (isnan(x))
            return x;
        if (x > 709.782712893383973096)
            return __builtin_inf();
        if (x < -745.13321910194110842)
            return 0;
        double n = round(x * 1.44269504088896338700e+00);
        double hi = x - n * ln2_hi, lo = n * ln2_lo;
        double r = hi - lo;
        double z = r * r;
        double c = r - z * (1.66666666666666019037e-01 + z * (-2.77777777770155933842e-03 + z * (6.61375632143793436117e-05 + z * (-1.65339022054652515390e-06 + z * 4.13813679705723846039e-08))));
        double y = 1.0 - ((lo - (r * c) / (2.0 - c)) - hi);
        return scalbn(y, (int)n);
    }

    namespace detail
    {
        // x = 2^k * (1 + f), log(1 + f) = f - hfsq + rest
        inline void log_parts(double x, int &k, double &f, double &hfsq, double &rest)
        {
            uint64_t b = bits(x);
            k = 0;
            if (b < (1ull << 52))
            {
                x *= 0x1p54;
                k -= 54;
                b = bits(x);
            }
            uint32_t hx = (uint32_t)(b >> 32);
            k += (int)(hx >> 20) - 1023;
            hx &= 0x000fffff;
            uint32_t i = (hx + 0x95f64) & 0x100000;
            k += (int)(i >> 20);
            double m = from_bits(((uint64_t)(hx | (i ^ 0x3ff00000)) << 32) | (b & 0xffffffffull));
            f = m - 1.0;
            double s = f / (2.0 + f);
            double z = s * s, w = z * z;
            double t1 = w * (3.999999999940941908e-01 + w * (2.222219843214978396e-01 + w * 1.531383769920937332e-01));
            double t2 = z * (6.666666666666735130e-01 + w * (2.857142874366239149e-01 + w * (1.818357216161805012e-01 + w * 1.479819860511658591e-01)));
            hfsq = 0.5 * f * f;
            rest = s * (hfsq + t1 + t2);
        }

        inline double log_m(double x, int &k)
        {
            double f, hfsq, rest;
            log_parts(x, k, f, hfsq, rest);
            return f - (hfsq - rest);
        }

        inline void two_sum(double a, double b, double &s, double &e)
        {
            s = a + b;
            double bb = s - a;
            e = (a - (s - bb)) + (b - bb);
        }

        inline void two_prod(double a, double b, double &p, double &e)
        {
            p = a * b;
            double ca = 134217729.0 * a, cb = 134217729.0 * b;
            double ah = ca - (ca - a), al = a - ah, bh = cb - (cb - b), bl = b - bh;
            e = ((ah * bh - p) + ah * bl + al * bh) + al * bl;
        }

        // log(x) as hi + lo, x > 0 and finite
        inline void log_dd(double x, double &hi, double &lo)
        {
            constexpr double ln2_hi = 6.93147180369123816490e-01, ln2_lo = 1.90821492927058770002e-10;
            int k;
            double f, hfsq, rest;
            log_parts(x, k, f, hfsq, rest);
            double s1, e1, s2, e2, sq, sqe;
            two_sum(k * ln2_hi, f, s1, e1);
            two_prod(f, f, sq, sqe);
            two_sum(s1, -0.5 * sq, s2, e2);
            double l = e1 + e2 - 0.5 * sqe + rest + k * ln2_lo;
            two_sum(s2, l, hi, lo);
        }
    }

    inline double log(double x)
    {
        constexpr double ln2_hi = 6.93147180369123816490e-01, ln2_lo = 1.90821492927058770002e-10;
        if (isnan(x) || x < 0)
            return __builtin_nan("");
        if (x == 0)
            return -__builtin_inf();
        if (isinf(x))
            return x;
        int k;
        double lm = detail::log_m(x, k);
        return k * ln2_hi + (lm + k * ln2_lo);
    }

    inline double log2(double x)
    {
        if (isnan(x) || x < 0)
            return __builtin_nan("");
        if (x == 0)
            return -__builtin_inf();
        if (isinf(x))
            return x;
        int k;
        double lm = detail::log_m(x, k);
        return k + lm * 1.44269504088896338700e+00;
    }

    inline double log10(double x)
    {
        double r = log(x) * 4.34294481903251827651e-01;
        double n = round(r);
        if (abs(r - n) < 1e-9 && n >= 0 && n <= 22)
        {
            double p = 1;
            for (int i = 0; i < (int)n; i++)
                p *= 10;
            if (p == x)
                return n;
        }
        return r;
    }

    inline double pow(double x, double y)
    {
        if (y == 0 || x == 1)
            return 1;
        if (isnan(x) || isnan(y))
            return x + y;
        bool y_int = trunc(y) == y;
        if (y_int && abs(y) <= 64)
        {
            double base = y < 0 ? 1.0 / x : x;
            uint32_t n = (uint32_t)abs(y);
            double r = 1;
            while (n)
            {
                if (n & 1)
                    r *= base;
                base *= base;
                n >>= 1;
            }
            return r;
        }
        if (y == 0.5 && x >= 0 && !isinf(x))
            return sqrt(x);
        bool neg = false;
        if (x < 0)
        {
            if (!y_int)
                return __builtin_nan("");
            neg = abs(y) < 0x1p53 && ((int64_t)y & 1);
            x = -x;
        }
        if (x == 0)
            return neg ? copysign(y > 0 ? 0.0 : __builtin_inf(), -1.0) : (y > 0 ? 0.0 : __builtin_inf());
        if (isinf(x))
            return (y > 0 ? __builtin_inf() : 0.0) * (neg ? -1 : 1);
        double lh, ll;
        detail::log_dd(x, lh, ll);
        double p = y * lh;
        double r;
        if (p > 710)
            r = __builtin_inf();
        else if (p < -746)
            r = 0;
        else
        {
            double pe;
            detail::two_prod(y, lh, p, pe);
            pe += y * ll;
            r = exp(p);
            if (!isinf(r))
                r += r * pe;
        }
        return neg ? -r : r;
    }

    // --- Linear Algebra ---
    struct Vec3
    {
        float x, y, z;

        // Operator overloads for clean syntax: v1 + v2
        Vec3 operator+(const Vec3 &v) const { return {x + v.x, y + v.y, z + v.z}; }
        Vec3 operator-(const Vec3 &v) const { return {x - v.x, y - v.y, z - v.z}; }
        Vec3 operator*(float s) const { return {x * s, y * s, z * s}; }

        float dot(const Vec3 &v) const { return x * v.x + y * v.y + z * v.z; }

        Vec3 cross(const Vec3 &v) const
        {
            return {
                y * v.z - z * v.y,
                z * v.x - x * v.z,
                x * v.y - y * v.x};
        }

        float length() const { return (float)webcc::sqrt(dot(*this)); }

        Vec3 normalize() const
        {
            float len = length();
            return (len > 0) ? (*this * (1.0f / len)) : Vec3{0, 0, 0};
        }
    };

    struct Mat4
    {
        float m[16]; // Column-major 

        static Mat4 identity()
        {
            return {{1, 0, 0, 0,
                     0, 1, 0, 0,
                     0, 0, 1, 0,
                     0, 0, 0, 1}};
        }

        // Basic translation matrix
        static Mat4 translation(float x, float y, float z)
        {
            Mat4 res = identity();
            res.m[12] = x;
            res.m[13] = y;
            res.m[14] = z;
            return res;
        }

        // Matrix multiplication
        Mat4 operator*(const Mat4 &b) const
        {
            Mat4 res = {0};
            for (int col = 0; col < 4; ++col)
            {
                for (int row = 0; row < 4; ++row)
                {
                    for (int k = 0; k < 4; ++k)
                    {
                        res.m[col * 4 + row] += m[k * 4 + row] * b.m[col * 4 + k];
                    }
                }
            }
            return res;
        }
    };

} // namespace webcc