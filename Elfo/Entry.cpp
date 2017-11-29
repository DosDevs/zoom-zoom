#include <cstdint>
#include <cstdio>

#include "../Common/stdlib.h"

#include <Common.h>
#include <Console.h>
#include <Elfo.h>
#include <Memo.h>
#include <Publics.h>

#include "Bootstrap.h"
#include "Elf64.h"
#include "Map.h"


ACTUATOR(Load);
ACTUATOR(Execute);
ACTUATOR(Discard);
ACTUATOR(Get_Properties);


extern "C" {
    ENTRY_POINT(Entry)
    {
        uint16_t* Screen_Buffer = reinterpret_cast<uint16_t*>(0xb8000);

        uint64_t return_value = 0;
        Actuator actuator = 0;

        Screen_Buffer[0] = 'E';
        Screen_Buffer[1] = 'L';
        Screen_Buffer[2] = 'F';
        Screen_Buffer[3] = 'O';

        Initialize();
        uint8_t option_enum = uint8_t(options);

        if ((function == ELFO_EXECUTE) &&
            (option_enum == uint8_t(ELFO_EXECUTE_BOOTSTRAP)))
        {
            return Bootstrap(options, arg_1, arg_2, arg_3, arg_4);
        }

        switch (function)
        {
            case ELFO_NOOP:
                /* Do nothing. */
                break;

            case ELFO_LOAD:
                actuator = &Load;
                break;

            case ELFO_EXECUTE:
                actuator = &Execute;

            case ELFO_DISCARD:
                actuator = &Discard;

            case ELFO_GET_PROPERTIES:
                actuator = &Get_Properties;

            default:
//                Screen_Buffer[0] = 0x044f045204520445ULL;
//                Screen_Buffer[1] = 0x0421042104210452ULL;
                return_value = 0;
        }

        return return_value;
    }
}

ACTUATOR(Load)
{
    return 0;
}

ACTUATOR(Execute)
{
    return 0;
}

ACTUATOR(Discard)
{
    return 0;
}

ACTUATOR(Get_Properties)
{
    return 0;
}

