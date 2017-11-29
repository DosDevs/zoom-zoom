#ifndef PUBLICS_H__INCLUDED
#define PUBLICS_H__INCLUDED

#include <cstdint>

class Publics {
    private:
        typedef uint64_t* const* const Array;
        Array _publics;

    public:
        Publics(uint64_t Base_Address = 0x0e00):
            _publics(reinterpret_cast<Array>(Base_Address))
        {}

        template<typename T>
        void Get(size_t Index, T& Value) const
        { Value = reinterpret_cast<T>(_publics[Index]); }
};


#endif  // PUBLICS_H__INCLUDED
