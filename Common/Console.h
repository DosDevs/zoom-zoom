#ifndef CONSOLE_H__INCLUDED
#define CONSOLE_H__INCLUDED


#include "Argument_List.h"

#include "Publics.h"

#include "stdlib.h"


class Hex {
    private:
        uint64_t _value;

    public:
        template<typename T>
        explicit Hex(T value):
            _value(uint64_t(value))
        {}

        operator uint64_t() const
        { return _value; }
};

template<size_t Width, char Char = ' '>
class Pad {
    private:
        char _buffer[Width + 1];

    protected:
        void init(char const* string)
        {
            memset(&_buffer[0], Char, Width);
            _buffer[Width] = 0;

            size_t length = strlen(string);

            if (length > Width)
                return;

            size_t pad_size = Width - length;

            memcpy(&_buffer[pad_size], &string[0], length);
        }

    public:
        Pad(char const* string)
        {
            init(string);
        }

        template<typename T>
        Pad(T const& arg)
        {
            init(Stringer(arg));
        }

        operator char const*() const
        { return Get(); }

        char const* Get() const
        { return &_buffer[0]; }
};

class Console {
    public:
        Publics const _publics;
        mutable size_t _x, _y;

        void _putchar(char const ch) const;
        void _newline() const;
        void _print(char const* msg) const;
        void _puts(char const* msg) const;
        void _setcursor(int value) const;
        void _getcursor(size_t& x, size_t& y) const;
        void _handle_scroll() const;
        size_t _pos() const;

        void (__cdecl* _kprint)(char const* Message);
        void (__cdecl* _kputchar)(char ch);
        void (__cdecl* _knewline)();
        void (__cdecl* _ksetcursor)(int value);
        size_t (__cdecl* _kgetcursor)();

    public:
        Console();
        Console(Publics const& publics);

        void Put_Char(char ch) const
        {
            return _putchar(ch);
        }

       void New_Line() const
        {
            return _newline();
        }

        template<typename T>
        void Puts(T const& value) const
        {
            Print(value);
            New_Line();
        }

        template<typename... Arguments>
        void Puts(char const* Message, Arguments const&... args) const
        {
            Print(Message, args...);
            New_Line();
        }

        int DecodeIndex(char ch) const
        {
            return (ch > '@')? (ch - '@' + 9): (ch - '0');
        }

        template<typename... Arguments>
        void Print(char const* Message, Arguments const&... args) const
        {
             uint16_t Index;
             Argument_List<sizeof...(args)> arg_list(args...);
 
             while (*Message)
             {
                 char const ch = *(Message++);
                 char const la = *Message;
 
                 if (ch == '$')
                 {
                     ++Message;
 
                     if (la != '$')
                     {
                         Index = DecodeIndex(la);
                         char const* Buffer = arg_list[Index];
                         _print(Buffer);
                         continue;
                     }
                 }
 
                 _putchar(ch);
             }
        }

        void Print(char ch) const
        {
            _putchar(ch);
        }

        void Print(Hex hex) const
        {
            _print(Argument_List<1>(hex)[0]);
        }

        template<typename T>
        void Print(T arg) const
        {
            _print(Argument_List<1>(arg)[0]);
        }

    public:
        void Dump(void const* buffer, size_t size) const;
};


#endif  // CONSOLE_H__INCLUDED
