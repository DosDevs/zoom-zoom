#include "Console.h"

#include <stdlib.h>

#include "Publics.h"


// Publics const* Console::_publics;
// size_t Console::_x, Console::_y, Console::_pos;
// 
// void (__cdecl* Console::_kprint)(char const* Message);
// void (__cdecl* Console::_kputchar)(char ch);
// void (__cdecl* Console::_knewline)();


void Console::_putchar(char const ch) const
{
    uint8_t* const _screen_buffer = (uint8_t*) 0xb8000;

    _getcursor(_x, _y);
    _screen_buffer[2 * _pos()] = ch;
    
    if (++_x == 80)
        _newline();
    else
        _setcursor(_pos());
}

void Console::_handle_scroll() const
{
    uint16_t* const _screen_buffer = (uint16_t*) 0xb8000;

    if (_y == 25)
    {
        --_y;

        std::copy(&_screen_buffer[80], &_screen_buffer[2000], &_screen_buffer[0]);
        std::fill(&_screen_buffer[1920], &_screen_buffer[2000], (_screen_buffer[1999] & 0xff00));
    }
}

void Console::_newline() const
{
    _getcursor(_x, _y);

    _x = 0;
    ++_y;

    _handle_scroll();
    _setcursor(_pos());
}

void Console::_print(char const* msg) const
{
    while ((*msg) != 0)
        _putchar(*(msg++));
}

void Console::_puts(char const* msg) const
{
    _print(msg);
    _newline();
}

void Console::_setcursor(int value) const
{
    _ksetcursor(value);
}

void Console::_getcursor(size_t& x, size_t& y) const
{
    size_t pos = _kgetcursor();

    x = pos % 80;
    y = pos / 80;
}

size_t Console::_pos() const
{
    return (80 * _y) + _x;
}

char Make_Hex(int Digit)
{
    return ((Digit > 9)? (char(Digit) + 'A' - 10): (char(Digit) + '0'));
}

void Show_Public(int index, void* ptr)
{
    uint64_t Value = reinterpret_cast<uint64_t>(ptr);
    char* const Screen = reinterpret_cast<char* const>(0xb8000);

    for (int i = 0; i < 16; ++i)
    {
        char Digit = (Value >> (60 - (i * 4))) & 0xf;
        Screen[(index + 1) * 160 + (i * 2)] = Make_Hex(Digit);
        Screen[(index + 1) * 160 + (i * 2) + 1] = 0x07;
    }
}

Console::Console():
    Console(Publics())
{}

Console::Console(Publics const& publics):
    _publics(publics),
    _x(0),
    _y(0)
{
    _publics.Get(1, _kprint);
    _publics.Get(2, _kputchar);
    _publics.Get(3, _knewline);
    _publics.Get(4, _ksetcursor);
    _publics.Get(5, _kgetcursor);

    _getcursor(_x, _y);
}

void Console::Dump(void const* buffer, size_t size) const
{
    auto buf = reinterpret_cast<uint8_t const*>(buffer);

    bool FirstTime = true;

    for (size_t i = 0; i < size; ++i)
    {
        if ((i % 0x080) == 0 && !FirstTime)
            New_Line();

        if ((i % 0x010) == 0)
        {
            if (!FirstTime)
                New_Line();

            Print(Hex(&buf[i]));
        } else
        if ((i % 0x08) == 0)

        {
            Put_Char(' ');
            Put_Char('-');
        }

        Put_Char(' ');
        Print(Pad<2, '0'>(Hex(int(buf[i]))).Get());

        FirstTime = false;
    }

    New_Line();
}
