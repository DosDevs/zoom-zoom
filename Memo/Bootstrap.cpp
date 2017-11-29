#include "Bootstrap.h"

#include <stdlib.h>

#include <Common.h>
#include <Console.h>
#include <Memo.h>
#include <Publics.h>

#include "AMD64.h"
#include "StaticHeap.h"
#include "Page_Repository.h"

#include "Lowlies.h"


namespace Memo
{
    constexpr uint64_t Page_Tables_Begin = 0xffffffff00000000ULL;
    constexpr uint64_t Static_Heap_Begin = 0xffffffffc0000000ULL;
    constexpr uint64_t Page_Tables_Limit = Static_Heap_Begin;
    constexpr uint64_t Static_Heap_End = 0xffffffffd0000000ULL;
    constexpr uint64_t First_Available_Physical_Page = 0x200000ULL;


    enum class Memory_Map_Entry_Type
    {
        Usable            = 1,
        Reserved          = 2,
        ACPI_Reclaimable  = 3,
        ACPI_Non_Volatile = 4,
        Bad               = 5
    };

    struct Memory_Map_Entry
    {
        uint64_t Start;
        uint64_t Size: 60;
        uint64_t Type:  4;

        Memory_Map_Entry_Type Type_As_Enum() const
        { return Memory_Map_Entry_Type(Type); }
    };

#pragma pack(1)
    struct __attribute__((packed)) BIOS_Memory_Map_Entry
    {
        uint64_t Start;
        uint64_t Size;
        uint32_t Type;

        Memory_Map_Entry_Type Type_As_Enum() const
        { return Memory_Map_Entry_Type(Type); }
    };
#pragma options align=reset

    struct Scratch_Header
    {
        Memo::Static_Heap* Heap;

        uint64_t Total_Usable_Memory;

        uint64_t Memory_Map_Entry_Count;
        BIOS_Memory_Map_Entry* BIOS_Memory_Map;

        uint64_t Page_Repository_Count;
        Page_Repository* Page_Repositories;

        AMD64::Pager* pager;

        uint64_t Reserved_MBZ[1];
    };

    Scratch_Header* scratch_header;

    struct Scratch_Page_Entry
    {
        enum Flags {
            Four_KiB_Page   = 0x0001,
            Two_MiB_Page    = 0x0002,
            One_GiB_Page    = 0x0004,

            Downwards_Page  = 0x0010,

            IOPL_Bit_0      = 0x0040,
            IOPL_Bit_1      = 0x0080,

            Readable        = 0x0100,
            Writable        = 0x0200,
            Runnable        = 0x0400,
            System          = 0x0800,
        };

        // Addresses:
        //    Low 32 bits: physical address.
        //    High 32 bits: virtual address.
        //
        // Addresses are in 4KiB blocks from 0.
        uint64_t Addresses;

        uint64_t Flags;
    };

    struct Scratch_Process_Pages
    {
        uint64_t Process_Id;
        uint32_t Reserved_MBZ_1;
        uint16_t Reserved_MBZ_0;
        uint16_t Page_Count;

        Scratch_Page_Entry Pages[0xff];
    };

    namespace Bootstrap
    {
        uint64_t Initialize(Scratch_Header& scratch_header);

        uint64_t
        Allocate(
                Scratch_Header& scratch_header,
                uint64_t virtual_address,
                uint64_t* size,
                uint64_t flags);

        uint64_t Relocate(Scratch_Header& scratch_header);

        //
        // Initialize:
        //   arg_1: BIOS Memory map.
        //   arg_2: Memory map entry count.
        //
        // Allocate:
        //   arg_1: Virtual address.
        //   arg_2: Pointer to size (in/out).
        //   arg_3: Flags.
        //
        ACTUATOR(Run)
        {
            int return_value = 0;

            uint64_t* Screen_Buffer = (uint64_t*) 0xb8000;
            Screen_Buffer[0] = 0x044f044d0445044dULL;

            Console console;

            uint64_t function = options & 0x0f;
            uint32_t scratch_bits = options;
            uint64_t scratch_address = scratch_bits & 0xfff000;  // 4 KiB aligned.
    //        uint64_t scratch_size = ((scratch_bits >> 24) << 10);

    //        console.Print('['); console.Print(Hex(function)); console.Puts(']');

            auto* scratch_header = reinterpret_cast<Scratch_Header*>(scratch_address);

            scratch_header->BIOS_Memory_Map = reinterpret_cast<BIOS_Memory_Map_Entry*>(arg_1);
            scratch_header->Memory_Map_Entry_Count = arg_2;
            scratch_header->Total_Usable_Memory = 0;

            switch (Memo_Bootstrap_Options(function))
            {
                case MEMO_BOOTSTRAP_INITIALIZE:
                    return Initialize(*scratch_header);

                case MEMO_BOOTSTRAP_ALLOCATE:
                    return
                        Allocate(
                                *scratch_header,
                                arg_1,
                                reinterpret_cast<uint64_t*>(arg_2),
                                arg_3);

                    Scratch_Header* scratch_header;

                case MEMO_BOOTSTRAP_RELOCATE:   return Relocate(*scratch_header);
            }

            return 0xDeadBeefCafeFaceULL;
        }

        // arg_1: BIOS Memory map.
        // arg_2: Memory map entry count.
        uint64_t Initialize(Scratch_Header& scratch_header)
        {
            Console console;
            Memo::Static_Heap*& heap = scratch_header.Heap;

            constexpr uint64_t Initial_Paging_Structures_Begin = 0x100000;
            constexpr uint64_t Initial_Paging_Structures_Limit = 0x200000;

            AMD64::Pager temp_pager(
                    Get_Paging_Root(),
                    Initial_Paging_Structures_Begin,
                    Initial_Paging_Structures_Limit);

            Memo::Static_Heap
                temp_heap(
                    temp_pager,
                    Static_Heap_Begin,
                    Static_Heap_End);

            temp_pager.Set_Heap(&temp_heap);

            // Initialize and relocate the heap object.
            temp_heap.Initialize(First_Available_Physical_Page);
            temp_heap.Allocate(heap);
            memcpy(heap, &temp_heap, sizeof(temp_heap));

            // TODO: Relocate the static header.
            scratch_header.Page_Repository_Count = 0;

            // Relocate the pager.
            heap->Allocate(scratch_header.pager);
            memcpy(scratch_header.pager, &temp_pager, sizeof(temp_pager));

            // Count the number of free regions.
            for (uint64_t i = 0; i < scratch_header.Memory_Map_Entry_Count; ++i)
            {
                auto const& entry = scratch_header.BIOS_Memory_Map[i];

                if (entry.Type_As_Enum() == Memory_Map_Entry_Type::Usable)
                {
                    ++scratch_header.Page_Repository_Count;
                    scratch_header.Total_Usable_Memory+= entry.Size;
                }
            }

            uint64_t usable_memory_amount = scratch_header.Total_Usable_Memory;
            char usable_memory_char = '\0';

            if (usable_memory_amount > 99000)
            {
                usable_memory_amount >>= 10;
                usable_memory_char = 'K';
            }

            if (usable_memory_amount > 99000)
            {
                usable_memory_amount >>= 10;
                usable_memory_char = 'M';
            }

            if (usable_memory_amount > 99000)
            {
                usable_memory_amount >>= 10;
                usable_memory_char = 'G';
            }

            if (usable_memory_amount > 99000)
            {
                usable_memory_amount >>= 10;
                usable_memory_char = 'T';
            }

            // Create the page repositories.
            heap->Allocate(scratch_header.Page_Repositories, scratch_header.Memory_Map_Entry_Count);

            for (uint64_t i = 0, j = 0; i < scratch_header.Page_Repository_Count;)
            {
                if (scratch_header.BIOS_Memory_Map[j].Type_As_Enum() != Memory_Map_Entry_Type::Usable)
                {
                    ++j;
                    continue;
                }

                auto* repo = &scratch_header.Page_Repositories[i];
                uint64_t start = scratch_header.BIOS_Memory_Map[j].Start;
                uint64_t end = scratch_header.BIOS_Memory_Map[j].Start + scratch_header.BIOS_Memory_Map[j].Size;

                while (++j < scratch_header.Memory_Map_Entry_Count)
                {
                    uint64_t next_start = scratch_header.BIOS_Memory_Map[j].Start;

                    if (scratch_header.BIOS_Memory_Map[j].Type_As_Enum() != Memory_Map_Entry_Type::Usable)
                        break;

                    // Join
                    end+= scratch_header.BIOS_Memory_Map[j].Size;
                }

                repo->Initialize(*heap, start, end);
                ++i;
            }

            // Relocate the page structures.
            uint64_t pml4_physical_address = 0;
            scratch_header.pager->Initialize(scratch_header.Page_Repository_Count, scratch_header.Page_Repositories);
            Create_Page_Result create_page_result = scratch_header.pager->Create_Page(Page_Size::_2_MiB, Page_Tables_Begin, pml4_physical_address);

            scratch_header.pager->Migrate_Page_Tables(
                    Initial_Paging_Structures_Begin,
                    Page_Tables_Begin,
                    pml4_physical_address,
                    1 << 20);

            return 0;
        }

        uint64_t
        Allocate(
                Scratch_Header& scratch_header,
                uint64_t virtual_address,
                uint64_t* size,
                uint64_t flags)
        {
            uint64_t allocated = 0;
            uint64_t to_allocate = *size;

            virtual_address&= ~0xfffULL;
            uint64_t address = virtual_address;

            if (virtual_address < 0x1000)
                return -1;

            Console console;

    //        console.Print('A');
    //        console.Print('d');
    //        console.Print('d');
    //        console.Print('r');
    //        console.Print('e');
    //        console.Print('s');
    //        console.Print('s');
    //        console.Print(':');
    //        console.Print(' ');
    //        console.Print(Hex(virtual_address));
    //        console.New_Line();

    //        console.Puts(Hex(virtual_address));
    //        console.Puts(Hex(size));
    //        console.Puts(Hex(*size));
    //        console.Puts(Hex(flags));
    //        console.New_Line();
    //
    //        console.Puts(Hex(scratch_header.pager));
    //        console.New_Line();

    //        console.Dump(scratch_header.pager, 64);

            while (allocated < (*size))
            {
                uint64_t _ = 0;  // Ignored.

    //            console.Print('A');
    //            console.Print('l');
    //            console.Print('l');
    //            console.Print('o');
    //            console.Print('c');
    //            console.Print('a');
    //            console.Print('t');
    //            console.Print('i');
    //            console.Print('n');
    //            console.Print('g');
    //            console.Print(' ');
    //            console.Print('@');
    //            console.Print(Hex(address));
    //            console.Print(':');
    //            console.Print(' ');
    //            console.Print(Hex(_));
    //            console.New_Line();

                constexpr uint64_t page_size{uint64_t(Page_Size::_4_KiB)};

                switch (auto result = scratch_header.pager->Create_Page(Page_Size::_4_KiB, address, _))
                {
                    case Create_Page_Result::Success:
                        memset(reinterpret_cast<void*>(address), 0, page_size);
                        allocated+= page_size;
                        address+= page_size;
                        continue;

                    case Create_Page_Result::Already_Exists:
                        allocated+= page_size;
                        address+= page_size;
                        continue;

                    default:
                        console.Print('E');
                        console.Print('r');
                        console.Print('r');
                        console.Print('o');
                        console.Print('r');
                        console.Print(':');
                        console.Print(' ');
                        console.Print(int(result));
                        console.New_Line();
                        return -1;
                }
            }

            console.Print('A');
            console.Print('l');
            console.Print('l');
            console.Print('o');
            console.Print('c');
            console.Print('a');
            console.Print('t');
            console.Print('e');
            console.Print('d');
            console.Print(' ');
            console.Print(allocated);
            console.Print(' ');
            console.Print('(');
            console.Print(Hex(allocated));
            console.Print('h');
            console.Print(')');
            console.Print(' ');
            console.Print('a');
            console.Print('t');
            console.Print(' ');
            console.Print(Hex(virtual_address));
            console.New_Line();

            (*size) = allocated;

            return 0;
        }

        uint64_t Relocate(Scratch_Header& scratch_header)
        {
            // Change this to Initialize.

            Memo::scratch_header = &scratch_header;
            Console().Puts("Memo initialized.");
            // Count of contiguous 4 KiB pages needed for this.
        //    Pager.Create_Page(Page_Size::_4_KiB, arg_3, 0x200000ULL);

            return 0;
        }
    }
}

