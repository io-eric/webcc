#pragma once
#include <cstdint>
#include "ratio.h"
#include "limits.h"
#include "../system.h"

namespace webcc {
namespace chrono {

    template <typename Rep, typename Period = webcc::ratio<1>>
    class duration {
    public:
        using rep = Rep;
        using period = Period;
        
        constexpr duration() = default;
        constexpr explicit duration(const Rep& r) : m_rep(r) {}
        
        constexpr Rep count() const { return m_rep; }
        
        static constexpr duration zero() { return duration(0); }
        static constexpr duration min() { return duration(webcc::numeric_limits<Rep>::min()); }
        static constexpr duration max() { return duration(webcc::numeric_limits<Rep>::max()); }

        constexpr duration operator+() const { return *this; }
        constexpr duration operator-() const { return duration(-m_rep); }
        
        constexpr duration operator+(const duration& d) const { return duration(m_rep + d.m_rep); }
        constexpr duration operator-(const duration& d) const { return duration(m_rep - d.m_rep); }
        constexpr duration operator*(const Rep& s) const { return duration(m_rep * s); }
        constexpr duration operator/(const Rep& s) const { return duration(m_rep / s); }
        
        constexpr duration& operator+=(const duration& d) { m_rep += d.m_rep; return *this; }
        constexpr duration& operator-=(const duration& d) { m_rep -= d.m_rep; return *this; }
        constexpr duration& operator*=(const Rep& s) { m_rep *= s; return *this; }
        constexpr duration& operator/=(const Rep& s) { m_rep /= s; return *this; }

        constexpr bool operator==(const duration& d) const { return m_rep == d.m_rep; }
        constexpr bool operator!=(const duration& d) const { return m_rep != d.m_rep; }
        constexpr bool operator<(const duration& d) const { return m_rep < d.m_rep; }
        constexpr bool operator<=(const duration& d) const { return m_rep <= d.m_rep; }
        constexpr bool operator>(const duration& d) const { return m_rep > d.m_rep; }
        constexpr bool operator>=(const duration& d) const { return m_rep >= d.m_rep; }

    private:
        Rep m_rep;
    };

    // Common durations
    using nanoseconds  = duration<int64_t, webcc::nano>;
    using microseconds = duration<int64_t, webcc::micro>;
    using milliseconds = duration<int64_t, webcc::milli>;
    using seconds      = duration<int64_t>;
    using minutes      = duration<int64_t, webcc::ratio<60>>;
    using hours        = duration<int64_t, webcc::ratio<3600>>;

    template <typename ToDuration, typename Rep, typename Period>
    constexpr ToDuration duration_cast(const duration<Rep, Period>& d) {
        using Ratio = webcc::ratio_divide<Period, typename ToDuration::period>;
        return ToDuration(static_cast<typename ToDuration::rep>(
            d.count() * Ratio::num / Ratio::den
        ));
    }

    template <typename Clock, typename Duration = typename Clock::duration>
    class time_point {
    public:
        using clock = Clock;
        using duration = Duration;
        using rep = typename Duration::rep;
        using period = typename Duration::period;

        constexpr time_point() : m_d(Duration::zero()) {}
        constexpr explicit time_point(const Duration& d) : m_d(d) {}

        constexpr Duration time_since_epoch() const { return m_d; }

        constexpr time_point& operator+=(const Duration& d) { m_d += d; return *this; }
        constexpr time_point& operator-=(const Duration& d) { m_d -= d; return *this; }

        constexpr time_point operator+(const Duration& d) const { return time_point(m_d + d); }
        constexpr time_point operator-(const Duration& d) const { return time_point(m_d - d); }
        constexpr Duration operator-(const time_point& tp) const { return m_d - tp.m_d; }

        constexpr bool operator==(const time_point& tp) const { return m_d == tp.m_d; }
        constexpr bool operator!=(const time_point& tp) const { return m_d != tp.m_d; }
        constexpr bool operator<(const time_point& tp) const { return m_d < tp.m_d; }
        constexpr bool operator<=(const time_point& tp) const { return m_d <= tp.m_d; }
        constexpr bool operator>(const time_point& tp) const { return m_d > tp.m_d; }
        constexpr bool operator>=(const time_point& tp) const { return m_d >= tp.m_d; }

    private:
        Duration m_d;
    };

    struct system_clock {
        using rep = int64_t;
        using period = webcc::milli;
        using duration = webcc::chrono::duration<rep, period>;
        using time_point = webcc::chrono::time_point<system_clock>;
        static constexpr bool is_steady = false;

        static time_point now() {
            return time_point(duration(static_cast<rep>(webcc::system::get_date_now())));
        }
    };

    struct steady_clock {
        using rep = int64_t;
        using period = webcc::nano;
        using duration = webcc::chrono::duration<rep, period>;
        using time_point = webcc::chrono::time_point<steady_clock>;
        static constexpr bool is_steady = true;

        static time_point now() {
            // get_time returns double ms. Convert to ns.
            return time_point(duration(static_cast<rep>(webcc::system::get_time() * 1000000.0)));
        }
    };

    using high_resolution_clock = steady_clock;

} // namespace chrono
} // namespace webcc
