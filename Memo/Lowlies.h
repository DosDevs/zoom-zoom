#pragma once

#include <stdlib.h>

extern "C"
{
    uint64_t Get_Paging_Root();

    void Set_Paging_Root(uint64_t address);

    uint64_t Count_Bits_Set(uint64_t value);

    uint64_t Get_Stack_Pointer();
}

