#include "stdlib.h"


typedef uint8_t byte;

extern "C"
{
    size_t strlen(char const* str)
    {
        char const* ptr = str;
        while (*ptr != 0) ++ptr;

        return size_t(ptr - str);
    }

    void memset(void* buffer, char ch, size_t length)
    {
        char* byte_buffer = reinterpret_cast<char*>(buffer);

        for (size_t i = 0; i < length; ++i)
            byte_buffer[i] = ch;
    }

    void memcpy(void* dest, void const* source, size_t length)
    {
        auto src = reinterpret_cast<byte const*>(source);
        auto dst = reinterpret_cast<byte*>(dest);

        std::copy(&src[0], &src[length], &dst[0]);
    }
}
