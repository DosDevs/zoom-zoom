#include <cstdint>

#include <Common.h>
#include <Memo.h>

#include "Bootstrap.h"


extern "C" {
    ENTRY_POINT(Entry)
    {
        uint16_t* Screen_Buffer = reinterpret_cast<uint16_t*>(0xb8000);

        Screen_Buffer[80] = 'M';
        Screen_Buffer[81] = 'E';
        Screen_Buffer[82] = 'M';
        Screen_Buffer[83] = 'O';

        uint64_t return_value = 0;

        if (function == MEMO_BOOTSTRAP)
        {
            return_value = Memo::Bootstrap::Run(options, arg_1, arg_2, arg_3, arg_4);
        }

        return return_value;
    }
}

