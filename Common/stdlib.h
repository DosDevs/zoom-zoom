#ifndef __STDLIB
#define __STDLIB

#include <cstdint>


namespace std
{
    typedef __size_t size_t;

    template<typename T1, typename T2>
    void copy(T1 src_begin, T1 src_end, T2 dest)
    {
        while (src_begin != src_end)
            *(dest++) = *(src_begin++);
    }

    template<typename T1, typename T2>
    void fill(T1 src_begin, T1 src_end, T2 value)
    {
        while (src_begin != src_end)
            *(src_begin++) = value;
    }

    template <typename Iter, typename Callback>
    void for_each(Iter iter_1, Iter iter_2, Callback callback)
    {
        for(Iter iter = iter_1; iter != iter_2; ++iter)
            callback(*iter);
    }

    template<typename T1, typename T2>
    auto min(T1 const& a, T2 const& b)
    {
        return ((a < b)? a: b);
    }

    template<typename T>
    T max(T a, T b)
    {
        return ((a > b)? a: b);
    }
}


using std::size_t;


extern "C"
{
    size_t strlen(char const* str);
    void memset(void* buffer, char ch, size_t length);
    void memcpy(void* dest, void const* source, size_t length);
}


#endif  // __STDLIB
