#ifndef STRINGER_H__INCLUDED
#define STRINGER_H__INCLUDED


#include <cstdint>


class Hex;


void Number_To_String(uint64_t Number, uint64_t Base, char* Buffer);


struct Char {
    char ch;

    Char(char ch = 0): ch(ch) {}
    operator char() { return ch; }
};


class Stringer {
    private:
        enum Source { _use_pointer, _use_array };

        char const* _pointer;
        char _array[25] = { 0 };
        Source _source;

        void Init_From_Number(uint64_t Number, uint64_t Base);

    public:
        explicit Stringer();
        explicit Stringer(Stringer const& that);
        explicit Stringer(uint64_t Number);
        explicit Stringer(Hex Number);
        explicit Stringer(Char Letter);
        explicit Stringer(char const* String);

        Stringer& operator=(Stringer const& that);

        char const* String() const;

        operator char const*() const
        {
            return String();
        }
};


#endif  // STRINGER_H__INCLUDED
