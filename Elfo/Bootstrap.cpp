#include "Bootstrap.h"

#include <cstdint>
#include <cstdio>

#include "../Common/stdlib.h"

#include <Common.h>
#include <Console.h>
#include <Elfo.h>
#include <Memo.h>
#include <Publics.h>

#include "Elf64.h"
#include "Map.h"


void Parse_Static_Elf(
        Console const& console,
        uint8_t const* Buffer,
        bool initialize_elfo,
        Entry_Point& entry_point);

void Clear_Bss(Elf64::Section_Header const& Section);

void Extract_Static_Elf_Segment_Information(
        size_t count,
        size_t offset,
        Elf64::Program_Header const* Program_Headers);

void Extract_Static_Elf_Section_Information(
        size_t count,
        size_t offset,
        Elf64::Section_Header const* Section_Headers);

void Print_Static_Elf_Segment_Information();
void Print_Static_Elf_Section_Information();


extern "C" {
    void Initialize()
    {
//        Static_Elf_Count = 0;
    }
}

void Print_Page_Table(uint64_t physical_table_start, uint64_t address, int level = 0)
{
    uint64_t const* iterator = reinterpret_cast<uint64_t*>(address);

    Console console;

    for (int i = 0; i < 512; ++i)
    {
        if (iterator[i] == 0)
            continue;

        for (int j = 0; j < level; ++j)
        {
            console.Print("    ");
        }

        auto v_address = (reinterpret_cast<uint64_t>(&iterator[i]));
        auto p_address = v_address - 0xffffffff00000000ULL + physical_table_start;

        auto new_p_address = (iterator[i] & ~0xfffULL);
        auto new_v_address = new_p_address - physical_table_start + 0xffffffff00000000ULL;

//        console.Puts("[$0] $1", Hex(p_address), Hex(new_p_address));

//        Print_Page_Table(physical_table_start, new_v_address, level + 1);
    }
}

ACTUATOR(Bootstrap)
{
    uint64_t* Screen_Buffer = reinterpret_cast<uint64_t*>(0xb8000);

    Screen_Buffer[0] = 0x044f0446044c0445ULL;

    Initialize();
    Console console;

    console.Puts('A');
    uint64_t memo_scratch_bits = options >> 32;

    uint64_t memo_entry_point_address;
    Entry_Point memo_entry_point;
    Entry_Point elfo_entry_point;

    auto memo_header = reinterpret_cast<Elf64::Header const*>(arg_2);
    memo_entry_point_address = memo_header->Entry;
    size_t section_offset = memo_header->Section_Table_Offset;
    console.Puts('A');

    for (size_t i = 0; i < memo_header->Section_Header_Entry_Count; ++i)
    {
        console.Puts(i);
        auto section_header =
            reinterpret_cast<Elf64::Section_Header*>(arg_2 + section_offset);

        if ((section_header->Address <= memo_entry_point_address) &&
            ((section_header->Address + section_header->Size) >= memo_entry_point_address))
        {
            memo_entry_point_address =
                arg_2 + section_header->Offset +
                (memo_entry_point_address - section_header->Address);

            break;
        }

        section_offset+= memo_header->Section_Header_Entry_Size;
    }
    console.Puts('A');

//    console.Puts(
//            "Bootstrapping Memo through entry point at $0h.",
//            Hex(memo_entry_point_address));
//
//    console.Puts(
//            "Memo scratch space bits: $0.",
//            Hex(memo_scratch_bits));

    memo_entry_point = reinterpret_cast<Entry_Point>(memo_entry_point_address);

    memo_entry_point(
            MEMO_BOOTSTRAP, (memo_scratch_bits | MEMO_BOOTSTRAP_INITIALIZE),
            arg_3, arg_4,
            0, 0);

    section_offset = memo_header->Section_Table_Offset;

//    console.Puts("Processing $0 sections.", memo_header->Section_Header_Entry_Count);

    for (size_t i = 0; i < memo_header->Section_Header_Entry_Count; ++i)
    {
        auto section_header =
            reinterpret_cast<Elf64::Section_Header*>(arg_2 + section_offset);

        size_t physical_address = arg_2 + section_header->Offset;
        size_t section_size = section_header->Size;

        uint64_t return_value = 0;

//        console.Puts(
//                "  Section $0 @$1: Type=$2 Flags=$3 Offset=$4 Size=$5 Address=$6",
//                i,
//                Hex(physical_address),
//                Hex(section_header->Type),
//                Hex(section_header->Flags),
//                Hex(section_header->Offset),
//                Hex(section_header->Size),
//                Hex(section_header->Address));

        if ((section_header->Size != 0) && (section_header->Address != 0))
        {
            uint64_t allocated_size = section_header->Size;

            return_value =
                memo_entry_point(
                    MEMO_BOOTSTRAP, (memo_scratch_bits | MEMO_BOOTSTRAP_ALLOCATE),
                    section_header->Address, reinterpret_cast<uint64_t>(&allocated_size),
                    0, 0);

            if (return_value != 0)
            {
//                console.Puts("Return value: $0", return_value);
                return return_value;
            }

//            Print_Page_Table(0xc3fe00000ULL, 0xffffffff00000000ULL);

            auto src = reinterpret_cast<void*>(physical_address);
            auto dst = reinterpret_cast<void*>(section_header->Address);

            if (section_header->Type != Elf64::SHT_NOBITS)
            {
//                console.Puts(
//                        "Copying $0 bytes from $1 to $2.",
//                        section_header->Size,
//                        Hex(src),
//                        Hex(dst));
                memcpy(dst, src, section_header->Size);
            }
        } else {
//            console.New_Line();
        }

        section_offset+= memo_header->Section_Header_Entry_Size;
    }

    memo_entry_point = reinterpret_cast<Entry_Point>(memo_header->Entry);

    memo_entry_point(
            MEMO_BOOTSTRAP, (memo_scratch_bits | MEMO_BOOTSTRAP_RELOCATE),
            0, 0, 0, 0);

//            Parse_Static_Elf(
//                    reinterpret_cast<uint8_t*>(arg_2),
//                    false,
//                    memo_entry_point);

    Parse_Static_Elf(
            console,
            reinterpret_cast<uint8_t*>(arg_1),
            true,
            elfo_entry_point);

    return reinterpret_cast<uint64_t>(memo_entry_point);
}

void Parse_Static_Elf(
        Console const& console,
        uint8_t const* buffer,
        bool initialize_elfo,
        Entry_Point& entry_point)
{
//    size_t elf_count = Static_Elf_Count;
    auto Elf_Header = reinterpret_cast<Elf64::Header const*>(&buffer[0]);

    entry_point = reinterpret_cast<Entry_Point>(Elf_Header->Entry);

//    console.Puts(
//            "Loading ELF #$0 at address $1h, with entry point at $2h.",
//            elf_count,
//            Hex(buffer),
//            entry_point);

    /*
     * First, find and clear any BSS's.
     */
    size_t Section_Count = Elf_Header->Section_Header_Entry_Count;
    auto Sections =
        reinterpret_cast<Elf64::Section_Header const*>(&buffer[Elf_Header->Section_Table_Offset]);

    for (size_t i = 0; i < Section_Count; ++i)
    {
//        console.Puts(
//                "  Section $0: Type=$1 Offset=$2 Size=$3 Address=$4",
//                i,
//                Hex(Sections[i].Type),
//                Hex(Sections[i].Offset),
//                Hex(Sections[i].Size),
//                Hex(Sections[i].Address));

        if (Sections[i].Type == Elf64::SHT_NOBITS)
        {
            Clear_Bss(Sections[i]);

            if (initialize_elfo)
                Initialize();  // Keep it initialized.
        }
    }

    /*
     * Second, find everything else.
     */
//    Static_Headers[elf_count] = *Elf_Header;

    Extract_Static_Elf_Segment_Information(
        Elf_Header->Program_Header_Entry_Count,
        Elf_Header->Program_Table_Offset,
        reinterpret_cast<Elf64::Program_Header const*>(
            &buffer[Elf_Header->Program_Table_Offset]));

    Extract_Static_Elf_Section_Information(
        Elf_Header->Section_Header_Entry_Count,
        Elf_Header->Section_Table_Offset,
        reinterpret_cast<Elf64::Section_Header const*>(
            &buffer[Elf_Header->Section_Table_Offset]));

    /*
     * Third, increment counter.
     */
//    Static_Elf_Count = ++elf_count;
//    console.Puts("Elfs loaded so far: $0.", Static_Elf_Count);
}

void Clear_Bss(Elf64::Section_Header const& Section)
{
//    console.Puts(
//            "  Clearing BSS: [$0, $1).",
//            Hex(Section.Address),
//            Hex(Section.Address + Section.Size));

    memset(reinterpret_cast<void*>(Section.Address), 0, Section.Size);
}

void Extract_Static_Elf_Segment_Information(
        size_t count,
        size_t offset,
        Elf64::Program_Header const* Program_Headers)
{
//    for (size_t i = 0; i < count; ++i)
//        Static_Program_Headers[Static_Elf_Count][i] = Program_Headers[i];
//
//    Static_Program_Header_Counts[Static_Elf_Count] = count;
}

void Extract_Static_Elf_Section_Information(
        size_t count,
        size_t offset,
        Elf64::Section_Header const* Section_Headers)
{
//    for (size_t i = 0; i < count; ++i)
//        Static_Section_Headers[Static_Elf_Count][i] = Section_Headers[i];
//
//    Static_Section_Header_Counts[Static_Elf_Count] = count;
}

