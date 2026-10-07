#pragma once
#include <stdint.h>
#include "math.h"

// double <-> text, shortest round-trip, double-double scaling
namespace webcc
{
    namespace detail
    {
        struct dd
        {
            double hi, lo;
        };

        inline dd quick_two_sum(double a, double b)
        {
            double s = a + b;
            return {s, b - (s - a)};
        }

        inline dd dd_mul_d(dd a, double b)
        {
            double p, e;
            two_prod(a.hi, b, p, e);
            return quick_two_sum(p, e + a.lo * b);
        }

        inline dd dd_add(dd a, dd b)
        {
            double s, e, t, f;
            two_sum(a.hi, b.hi, s, e);
            two_sum(a.lo, b.lo, t, f);
            dd r = quick_two_sum(s, e + t);
            return quick_two_sum(r.hi, r.lo + f);
        }

        inline dd dd_div_d(dd a, double b)
        {
            double q1 = a.hi / b;
            dd r = dd_add(a, dd_mul_d({-q1, 0}, b));
            double q2 = r.hi / b;
            r = dd_add(r, dd_mul_d({-q2, 0}, b));
            double q3 = r.hi / b;
            dd q = quick_two_sum(q1, q2);
            return dd_add(q, {q3, 0});
        }

        inline double pow10_exact(int k)
        {
            static constexpr double t[] = {1e0, 1e1, 1e2, 1e3, 1e4, 1e5, 1e6, 1e7, 1e8, 1e9, 1e10, 1e11,
                                           1e12, 1e13, 1e14, 1e15, 1e16, 1e17, 1e18, 1e19, 1e20, 1e21, 1e22};
            return t[k];
        }

        // x * 10^k = r * 2^bexp, r near 1 so nothing overflows on the way
        inline dd scale10(dd x, int k, int &bexp)
        {
            bexp = 0;
            auto norm = [&]() {
                if (x.hi == 0)
                    return;
                if (((bits(x.hi) >> 52) & 0x7ff) == 0)
                {
                    x.hi *= 0x1p64;
                    x.lo *= 0x1p64;
                    bexp -= 64;
                }
                int ex = (int)((bits(x.hi) >> 52) & 0x7ff) - 1023;
                x.hi = scalbn(x.hi, -ex);
                x.lo = scalbn(x.lo, -ex);
                bexp += ex;
            };
            norm();
            while (k > 0)
            {
                int c = k > 22 ? 22 : k;
                x = dd_mul_d(x, pow10_exact(c));
                k -= c;
                norm();
            }
            while (k < 0)
            {
                int c = -k > 22 ? 22 : -k;
                x = dd_div_d(x, pow10_exact(c));
                k += c;
                norm();
            }
            return x;
        }

        inline dd u64_to_dd(uint64_t n)
        {
            double p, e;
            two_prod((double)(n / 1000000000ull), 1e9, p, e);
            dd hi = quick_two_sum(p, e);
            return dd_add(hi, {(double)(n % 1000000000ull), 0});
        }

        inline double fmod_2(double m) { return m - 2 * floor(m * 0.5); }

        inline uint64_t pow10_u64(int k)
        {
            uint64_t r = 1;
            while (k-- > 0)
                r *= 10;
            return r;
        }

        // m * 10^e, m >= 0
        inline double dd_decimal_to_double(dd m, int e)
        {
            if (m.hi == 0)
                return 0;
            if (e > 310)
                return __builtin_inf();
            if (e < -400)
                return 0;
            int bexp;
            dd v = scale10(m, e, bexp);
            if (bexp >= -1022)
                return scalbn(v.hi + v.lo, bexp);
            // subnormal: round once, in units of 2^-1074
            double th = scalbn(v.hi, bexp + 1074), tl = scalbn(v.lo, bexp + 1074);
            double f = floor(th);
            double rem = (th - f) + tl;
            double fr = floor(rem);
            double r = f + fr, frac = rem - fr;
            if (frac > 0.5 || (frac == 0.5 && fmod_2(r) != 0))
                r += 1;
            return scalbn(r, -1074);
        }

        inline double decimal_to_double(uint64_t n, int e) { return dd_decimal_to_double(u64_to_dd(n), e); }

        // v > 0: v ~= n * 10^(e - p + 1), n has p digits
        inline void decimal_digits(double v, int p, uint64_t &n, int &e)
        {
            e = (int)floor(log10(v));
            for (int tries = 0; tries < 4; tries++)
            {
                int bexp;
                dd s = scale10({v, 0}, p - 1 - e, bexp);
                s = {scalbn(s.hi, bexp), scalbn(s.lo, bexp)};
                double f = floor(s.hi);
                double rem = (s.hi - f) + s.lo;
                double fr = floor(rem);
                n = (uint64_t)f + (uint64_t)(int64_t)fr;
                if (rem - fr >= 0.5)
                    n++;
                if (n >= pow10_u64(p))
                    e++;
                else if (n < pow10_u64(p - 1))
                    e--;
                else
                    return;
            }
        }

        // shortest digits; single = the value is a float32
        inline void shortest_digits(double v, bool single, uint64_t &n, int &e, int &p)
        {
            int max_p = single ? 9 : 17;
            for (p = 1; p <= max_p; p++)
            {
                decimal_digits(v, p, n, e);
                double back = decimal_to_double(n, e - p + 1);
                if (single ? (float)back == (float)v : back == v)
                    return;
            }
            p = max_p;
            decimal_digits(v, p, n, e);
        }

        inline uint32_t write_u64(char *out, uint64_t n)
        {
            char tmp[20];
            uint32_t len = 0;
            do
            {
                tmp[len++] = (char)('0' + n % 10);
                n /= 10;
            } while (n);
            for (uint32_t i = 0; i < len; i++)
                out[i] = tmp[len - 1 - i];
            return len;
        }

        // JS Number.toString layout
        inline uint32_t layout(char *out, bool neg, uint64_t n, int p, int e)
        {
            char d[20];
            write_u64(d, n);
            while (p > 1 && d[p - 1] == '0')
                p--;
            uint32_t len = 0;
            if (neg)
                out[len++] = '-';
            if (e >= -6 && e < 21)
            {
                if (e < 0)
                {
                    out[len++] = '0';
                    out[len++] = '.';
                    for (int i = 0; i < -e - 1; i++)
                        out[len++] = '0';
                    for (int i = 0; i < p; i++)
                        out[len++] = d[i];
                }
                else
                {
                    for (int i = 0; i <= e; i++)
                        out[len++] = i < p ? d[i] : '0';
                    if (p > e + 1)
                    {
                        out[len++] = '.';
                        for (int i = e + 1; i < p; i++)
                            out[len++] = d[i];
                    }
                }
            }
            else
            {
                out[len++] = d[0];
                if (p > 1)
                {
                    out[len++] = '.';
                    for (int i = 1; i < p; i++)
                        out[len++] = d[i];
                }
                out[len++] = 'e';
                out[len++] = e < 0 ? '-' : '+';
                len += write_u64(out + len, (uint64_t)(e < 0 ? -e : e));
            }
            out[len] = '\0';
            return len;
        }

        inline uint32_t format_number(double v, bool single, char *out)
        {
            if (isnan(v))
            {
                out[0] = 'N', out[1] = 'a', out[2] = 'N', out[3] = '\0';
                return 3;
            }
            bool neg = v < 0;
            if (isinf(v))
            {
                const char *s = neg ? "-Infinity" : "Infinity";
                uint32_t len = 0;
                while (s[len])
                    out[len] = s[len], len++;
                out[len] = '\0';
                return len;
            }
            if (v == 0)
            {
                out[0] = '0', out[1] = '\0';
                return 1;
            }
            uint64_t n;
            int e, p;
            shortest_digits(neg ? -v : v, single, n, e, p);
            return layout(out, neg, n, p, e);
        }
    }

    // out needs 32 bytes; "0.1", "1e+21", "NaN", "-Infinity"
    inline uint32_t format_double(double v, char *out) { return detail::format_number(v, false, out); }
    inline uint32_t format_float(float v, char *out) { return detail::format_number(v, true, out); }

    // fixed decimals, out needs 48 bytes
    inline uint32_t format_fixed(double v, int places, char *out)
    {
        if (places < 0)
            places = 0;
        if (places > 15)
            places = 15;
        double av = abs(v);
        if (isnan(v) || isinf(v) || av * detail::pow10_exact(places) >= 0x1p63)
            return format_double(v, out);
        int bexp;
        detail::dd s = detail::scale10({av, 0}, places, bexp);
        s = {scalbn(s.hi, bexp), scalbn(s.lo, bexp)};
        double f = floor(s.hi);
        double rem = (s.hi - f) + s.lo;
        uint64_t n = (uint64_t)f + (uint64_t)(int64_t)floor(rem);
        if (rem - floor(rem) >= 0.5)
            n++;
        char d[24];
        uint32_t dl = detail::write_u64(d, n);
        uint32_t len = 0;
        if (v < 0 && n != 0)
            out[len++] = '-';
        int int_digits = (int)dl - places;
        if (int_digits <= 0)
            out[len++] = '0';
        else
            for (int i = 0; i < int_digits; i++)
                out[len++] = d[i];
        if (places > 0)
        {
            out[len++] = '.';
            for (int i = 0; i < places; i++)
            {
                int idx = int_digits + i;
                out[len++] = idx < 0 ? '0' : d[idx];
            }
        }
        out[len] = '\0';
        return len;
    }

    // JSON/JS number syntax, plus "Infinity"/"NaN". used = chars read, 0 if none
    inline double parse_double(const char *s, uint32_t len, uint32_t &used)
    {
        uint32_t i = 0;
        bool neg = false;
        if (i < len && (s[i] == '-' || s[i] == '+'))
            neg = s[i++] == '-';
        auto word = [&](const char *w) {
            uint32_t k = 0;
            while (w[k] && i + k < len && s[i + k] == w[k])
                k++;
            return w[k] == '\0' ? k : 0u;
        };
        if (uint32_t k = word("Infinity"))
        {
            used = i + k;
            return neg ? -__builtin_inf() : __builtin_inf();
        }
        if (uint32_t k = word("NaN"))
        {
            used = i + k;
            return __builtin_nan("");
        }
        uint64_t n = 0, n2 = 0;
        int digits = 0, exp = 0;
        bool any = false;
        auto digit = [&](int d, bool frac) {
            if (digits == 0 && d == 0)
            {
                if (frac)
                    exp--;
                return;
            }
            if (digits < 19)
                n = n * 10 + (uint64_t)d;
            else if (digits < 38)
                n2 = n2 * 10 + (uint64_t)d;
            else
            {
                if (!frac)
                    exp++;
                return;
            }
            digits++;
            if (frac)
                exp--;
        };
        while (i < len && s[i] >= '0' && s[i] <= '9')
        {
            any = true;
            digit(s[i++] - '0', false);
        }
        if (i < len && s[i] == '.')
        {
            i++;
            while (i < len && s[i] >= '0' && s[i] <= '9')
            {
                any = true;
                digit(s[i++] - '0', true);
            }
        }
        if (!any)
        {
            used = 0;
            return 0;
        }
        if (i < len && (s[i] == 'e' || s[i] == 'E'))
        {
            uint32_t j = i + 1;
            bool eneg = false;
            if (j < len && (s[j] == '-' || s[j] == '+'))
                eneg = s[j++] == '-';
            if (j < len && s[j] >= '0' && s[j] <= '9')
            {
                int ev = 0;
                while (j < len && s[j] >= '0' && s[j] <= '9')
                {
                    if (ev < 100000)
                        ev = ev * 10 + (s[j] - '0');
                    j++;
                }
                exp += eneg ? -ev : ev;
                i = j;
            }
        }
        used = i;
        detail::dd m = detail::u64_to_dd(n);
        if (digits > 19)
        {
            int bexp;
            m = detail::scale10(m, digits - 19 > 19 ? 19 : digits - 19, bexp);
            m = {scalbn(m.hi, bexp), scalbn(m.lo, bexp)};
            m = detail::dd_add(m, detail::u64_to_dd(n2));
        }
        double v = detail::dd_decimal_to_double(m, exp);
        return neg ? -v : v;
    }
}
