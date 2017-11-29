#include "Elf64.h"


namespace Elf64
{
    namespace {
        char const* _segment_type_names[] = {
            "Null",
            "Loadable",
            "Dynamic",
            "Interpreter",
            "Note",
            "Shared library",
            "Program header"
        };

        char const* _unknown            = "Unknown";
        char const* _pt_gnu_eh_frame    = "Exception handling";
        char const* _pt_gnu_stack       = "Stack";
        char const* _pt_gnu_relro       = "Read-only after relocation";

        char const* _section_type_names[] = {
            "Null",
            "Program bits",
            "Symbol table",
            "String table",
            "Relocation (a)",
            "Hash",
            "Dynamic",
            "Note",
            "No bits",
            "Relocation",
            "Shared library",
            "Dynamic symbol"
        };
    }

    char const* Get_Segment_Type_Name(
            Segment_Type segment_type)
    {
        char const* result = _unknown;

        if (segment_type > PT_MAX_SEGMENT_TYPE)
        {
            switch (segment_type)
            {
                case PT_GNU_EH_FRAME:   result = _pt_gnu_eh_frame;  break;
                case PT_GNU_STACK:      result = _pt_gnu_stack;     break;
                case PT_GNU_RELRO:      result = _pt_gnu_relro;     break;
                default:;
            }

            return result;
        }

        return _segment_type_names[segment_type];
    }

    char const* Get_Section_Type_Name(
            Section_Type section_type)
    {
        return
            (section_type >= SHT_MAX_SECTION_TYPE)?
                _unknown:
                _section_type_names[section_type];
    }
}
