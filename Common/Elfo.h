#pragma once

enum Elfo_Function {
    ELFO_NOOP,
    ELFO_EXECUTE = 1,
    ELFO_LOAD,
    ELFO_DISCARD,
    ELFO_GET_PROPERTIES  // Maybe... GET_INFO?  GET_INFORMATION?
};

enum Elfo_Load_Options {
    ELFO_LOAD_NO_OPTIONS,
    ELFO_LOAD_KERNEL,
    ELFO_LOAD_RING_1,
    ELFO_LOAD_RING_2,
    ELFO_LOAD_USER,
    ELFO_LOAD_CONTIGUOUS,
    ELFO_LOAD_IDENTITY
};

enum Elfo_Execute_Options {
    ELFO_EXECUTE_NO_OPTIONS,
    ELFO_EXECUTE_KERNEL,
    ELFO_EXECUTE_RING_1,
    ELFO_EXECUTE_RING_2,
    ELFO_EXECUTE_USER,
    ELFO_EXECUTE_CONTIGUOUS,
    ELFO_EXECUTE_IDENTITY,
    ELFO_EXECUTE_BOOTSTRAP = 0xffff
};

enum Elfo_Discard_Options {
};

enum Elfo_Get_Properties_Options {
};

