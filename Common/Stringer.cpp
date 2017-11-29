#include "Stringer.h"

#include "stdlib.h"

#include "Console.h"

void Number_To_String(uint64_t number, uint64_t base, char* buffer)
{
    char temp[25] = { 0 };
    size_t i = 0, j = 0;

    do {
        int digit = number % base;
        temp[i++] = (digit > 9)? (digit - 10 + 'A'): (digit + '0');
        number/= base;
    } while (number != 0);

    for (j = 0; j < i; ++j)
        buffer[j] = temp[i - j - 1];

    buffer[j] = 0;
}

Stringer::Stringer():
    _pointer(0),
    _array(),
    _source(_use_array)
{}

Stringer::Stringer(Stringer const& that):
    _pointer(0),
    _array(),
    _source(_use_array)
{
    std::copy(&that._array[0], &that._array[25], &this->_array[0]);
}

Stringer::Stringer(uint64_t Number):
    _pointer(0),
    _array(),
    _source(_use_array)
{
    Init_From_Number(Number, 10);
}

Stringer::Stringer(Hex Number):
    _pointer(0),
    _array(),
    _source(_use_array)
{
    Init_From_Number(Number, 16);
}

Stringer::Stringer(Char Letter):
    _pointer(0),
    _array(),
    _source(_use_array)
{
    _array[0] = Letter;
    _array[1] = 0;
}

Stringer::Stringer(char const* String):
    _pointer(String),
    _array(),
    _source(_use_pointer)
{}

Stringer& Stringer::operator=(Stringer const& that)
{
    _pointer = that._pointer;
    _source = that._source;
    std::copy(&that._array[0], &that._array[25], &this->_array[0]);

    return *this;
}

void Stringer::Init_From_Number(uint64_t Number, uint64_t Base)
{
    Number_To_String(Number, Base, _array);
}

char const* Stringer::String() const
{
    switch(_source)
    {
        case _use_pointer: return _pointer;
        case _use_array: return _array;
        default: return 0;
    }
}
