#pragma once
#include "utility.h"

namespace webcc
{
    template <typename T>
    const T& min(const T& a, const T& b)
    {
        return (b < a) ? b : a;
    }

    template <typename T>
    const T& max(const T& a, const T& b)
    {
        return (a < b) ? b : a;
    }

    template <typename T>
    const T& clamp(const T& v, const T& lo, const T& hi)
    {
        return (v < lo) ? lo : ((hi < v) ? hi : v);
    }

    // Default comparator
    template <typename T>
    struct less
    {
        bool operator()(const T& a, const T& b) const
        {
            return a < b;
        }
    };

    // Partition for QuickSort
    template <typename Iterator, typename Compare>
    Iterator partition(Iterator first, Iterator last, Compare comp)
    {
        Iterator pivot_pos = last - 1;
        Iterator i = first;
        
        for (Iterator j = first; j < pivot_pos; ++j)
        {
            if (comp(*j, *pivot_pos))
            {
                if (i != j) {
                    webcc::swap(*i, *j);
                }
                ++i;
            }
        }
        if (i != pivot_pos) {
            webcc::swap(*i, *pivot_pos);
        }
        return i;
    }

    // QuickSort implementation
    template <typename Iterator, typename Compare>
    void sort(Iterator first, Iterator last, Compare comp)
    {
        if (last - first > 1)
        {
            Iterator pivot = partition(first, last, comp);
            sort(first, pivot, comp);
            sort(pivot + 1, last, comp);
        }
    }

    // Sort overload using default comparator
    template <typename Iterator>
    void sort(Iterator first, Iterator last)
    {
        using ValueType = typename remove_reference<decltype(*first)>::type;
        sort(first, last, less<ValueType>());
    }

} // namespace webcc