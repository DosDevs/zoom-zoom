#include "Argument_List.h"

#include "Console.h"


template<int Count>
void Argument_List<Count>::_show(Console const& console, int i)
{
    console.Put_Char(char(i + 0x30));
    console.New_Line();
}

template<int Count>
void Argument_List<Count>::_show(Console const& console, char const* str)
{
    console.Puts(str);
}


template class Argument_List<0>;
template class Argument_List<1>;
template class Argument_List<2>;
template class Argument_List<3>;
template class Argument_List<4>;
template class Argument_List<5>;
template class Argument_List<6>;
template class Argument_List<7>;
template class Argument_List<8>;
template class Argument_List<9>;
