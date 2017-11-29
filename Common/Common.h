#pragma once

#include "stdlib.h"

#define ACTUATOR_ARGUMENTS                                  \
            uint64_t options,                               \
            uint64_t arg_1, uint64_t arg_2,                 \
            uint64_t arg_3, uint64_t arg_4                  \

#define ACTUATOR(Name)                                      \
    uint64_t Name(ACTUATOR_ARGUMENTS)                       \

/*
 * Arguments in System V AMD64 ABI's C calling convention:
 *      RDI, RSI, RDX, RCX, R8, R9.
 *
 * Return value:
 *      RDX:RAX
 */
#define ENTRY_POINT(Name)                                   \
    uint64_t Name(uint64_t function, ACTUATOR_ARGUMENTS)    \

extern "C" {
    typedef ENTRY_POINT((*Entry_Point));
    typedef ACTUATOR((*Actuator));

    ENTRY_POINT(Entry);     // Forward declaration.
}  // extern "C"

