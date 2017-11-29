#ifndef ELFO__ELF_HEADER_H__INCLUDED
#define ELFO__ELF_HEADER_H__INCLUDED


#include <cstdint>


namespace Elf64
{
    typedef  uint8_t Byte;
    typedef uint16_t Half;
    typedef  int32_t Sword;
    typedef uint32_t Word;
    typedef  int64_t Sxword;
    typedef uint64_t Xword;
    typedef uint64_t Address;
    typedef uint64_t Offset;

    struct Header
    {
        Word      Magic;            // LE - 0x464c457f; BE - 0x7f454c46.
        Byte      Class;            // 1 - 32-bit; 2 - 64-bit.
        Byte      Endianness;       // 1 - Little-Endian; 2 - Big-Endian.
        Byte      Header_Version;   // Usually, 1.
        Byte      Padding[9];
        Half      Type;
        Half      Machine;
        Word      Program_Version;
        Address   Entry;
        Offset    Program_Table_Offset;
        Offset    Section_Table_Offset;
        Word      Flags;
        Half      E_Header_Size;
        Half      Program_Header_Entry_Size;
        Half      Program_Header_Entry_Count;
        Half      Section_Header_Entry_Size;
        Half      Section_Header_Entry_Count;
        Half      Section_Name_String_Index;
    };


    enum Segment_Type
    {
        PT_NULL =       0,
        PT_LOAD =       1,
        PT_DYNAMIC =    2,
        PT_INTERP =     3,
        PT_NOTE =       4,
        PT_SHLIB =      5,
        PT_PHDR =       6,
        PT_MAX_SEGMENT_TYPE,
        PT_GNU_EH_FRAME =   0x6474e550,
        PT_GNU_STACK =      0x6474e551,
        PT_GNU_RELRO =      0x6474e552,
        PT_LOPROC =         0x70000000,
        PT_HIPROC =         0x7fffffff,
        PT_MIPS_REGINFO =   0x70000000,
        PT_MIPS_OPTIONS =   0x70000001
    };

    struct Program_Header
    {
        Segment_Type Type;
        Word Flags;
        Offset Offset;
        Address Virtual_Address;
        Address Physical_Address;
        Xword Size_In_File;
        Xword Size_In_Memory;
        Xword Alignment;
    };

    enum Section_Type
    {
        SHT_NULL        =  0,
        SHT_PROGBITS    =  1,
        SHT_SYMTAB      =  2,
        SHT_STRTAB      =  3,
        SHT_RELA        =  4,
        SHT_HASH        =  5,
        SHT_DYNAMIC     =  6,
        SHT_NOTE        =  7,
        SHT_NOBITS      =  8,
        SHT_REL         =  9,
        SHT_SHLIB       = 10,
        SHT_DYNSYM      = 11,
        SHT_MAX_SECTION_TYPE,
        SHT_LOPROC      = 0x70000000,
        SHT_HIPROC      = 0x7fffffff,
        SHT_LOUSER      = 0x80000000,
        SHT_HIUSER      = 0x8fffffff,
    };

    struct Section_Header
    {
        Word Name;
        Word Type;
        Xword Flags;
        Address Address;
        Offset Offset;
        Xword Size;
        Word Link;
        Word Info;
        Xword Address_Alignment;
        Xword Entry_Size;
    };

    char const* Get_Segment_Type_Name(
        Segment_Type segment_type);

    char const* Get_Section_Type_Name(
        Section_Type section_type);
}  // namespace Elf64


#endif  // ELFO__ELF_HEADER_H__INCLUDED
