#ifndef ARGUMENT_LIST_H__INCLUDED
#define ARGUMENT_LIST_H__INCLUDED


#include <cstddef>

#include "Stringer.h"

class Console;


template<int Count>
class Argument_List {
    private:
        Stringer _array[Count];

        template<typename T, typename... Arguments>
        void _init(int i, T const& arg, Arguments const&... args)
        {
            _array[i] = Stringer(arg);
//            _show(_array[i]);
            _init(++i, args...);
        }

        void _show(Console const& console, int i);
        void _show(Console const& console, char const* str);

        void _init(int i) {}

    public:
        template<typename... Arguments>
        Argument_List(Arguments const&... args):
            _array()
        {
            _init(0, args...);
        }

        Stringer const& operator[](size_t i) const
        {
            return _array[i];
        }
};


#endif  // ARGUMENT_LIST_H__INCLUDED

