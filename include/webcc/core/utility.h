#pragma once

namespace webcc
{
    // Minimal implementation of move/forward
    template<typename T> struct remove_reference { typedef T type; };
    template<typename T> struct remove_reference<T&> { typedef T type; };
    template<typename T> struct remove_reference<T&&> { typedef T type; };

    template<typename T>
    typename remove_reference<T>::type&& move(T&& t) {
        return static_cast<typename remove_reference<T>::type&&>(t);
    }

    template<typename T>
    T&& forward(typename remove_reference<T>::type& t) {
        return static_cast<T&&>(t);
    }
    template<typename T>
    T&& forward(typename remove_reference<T>::type&& t) {
        return static_cast<T&&>(t);
    }

    template<typename T>
    void swap(T& a, T& b)
    {
        T temp = move(a);
        a = move(b);
        b = move(temp);
    }

    // Pair implementation
    template<typename T1, typename T2>
    struct pair
    {
        typedef T1 first_type;
        typedef T2 second_type;

        T1 first;
        T2 second;

        // Default constructor
        pair() : first(), second() {}

        // Constructor with values
        pair(const T1& a, const T2& b) : first(a), second(b) {}

        // Move constructor with values
        template<typename U1, typename U2>
        pair(U1&& a, U2&& b) 
            : first(forward<U1>(a)), second(forward<U2>(b)) {}

        // Copy constructor
        pair(const pair& other) = default;

        // Move constructor
        pair(pair&& other) = default;

        // Copy constructor from different pair type
        template<typename U1, typename U2>
        pair(const pair<U1, U2>& other) 
            : first(other.first), second(other.second) {}

        // Move constructor from different pair type
        template<typename U1, typename U2>
        pair(pair<U1, U2>&& other) 
            : first(forward<U1>(other.first)), second(forward<U2>(other.second)) {}

        // Copy assignment
        pair& operator=(const pair& other) = default;

        // Move assignment
        pair& operator=(pair&& other) = default;

        // Copy assignment from different pair type
        template<typename U1, typename U2>
        pair& operator=(const pair<U1, U2>& other)
        {
            first = other.first;
            second = other.second;
            return *this;
        }

        // Move assignment from different pair type
        template<typename U1, typename U2>
        pair& operator=(pair<U1, U2>&& other)
        {
            first = forward<U1>(other.first);
            second = forward<U2>(other.second);
            return *this;
        }

        void swap(pair& other)
        {
            webcc::swap(first, other.first);
            webcc::swap(second, other.second);
        }
    };

    // Comparison operators
    template<typename T1, typename T2>
    bool operator==(const pair<T1, T2>& lhs, const pair<T1, T2>& rhs)
    {
        return lhs.first == rhs.first && lhs.second == rhs.second;
    }

    template<typename T1, typename T2>
    bool operator!=(const pair<T1, T2>& lhs, const pair<T1, T2>& rhs)
    {
        return !(lhs == rhs);
    }

    template<typename T1, typename T2>
    bool operator<(const pair<T1, T2>& lhs, const pair<T1, T2>& rhs)
    {
        if (lhs.first < rhs.first) return true;
        if (rhs.first < lhs.first) return false;
        return lhs.second < rhs.second;
    }

    template<typename T1, typename T2>
    bool operator<=(const pair<T1, T2>& lhs, const pair<T1, T2>& rhs)
    {
        return !(rhs < lhs);
    }

    template<typename T1, typename T2>
    bool operator>(const pair<T1, T2>& lhs, const pair<T1, T2>& rhs)
    {
        return rhs < lhs;
    }

    template<typename T1, typename T2>
    bool operator>=(const pair<T1, T2>& lhs, const pair<T1, T2>& rhs)
    {
        return !(lhs < rhs);
    }

    // make_pair helper function
    template<typename T1, typename T2>
    pair<typename remove_reference<T1>::type, typename remove_reference<T2>::type>
    make_pair(T1&& a, T2&& b)
    {
        return pair<typename remove_reference<T1>::type, 
                    typename remove_reference<T2>::type>(
            forward<T1>(a), forward<T2>(b));
    }
} // namespace webcc
