#include "AMD64.h"

#include <stdint.h>

#include <Console.h>
#include <Publics.h>

#include "Address_Resolver.h"
#include "Page_Repository.h"
#include "StaticHeap.h"


namespace {
    constexpr uint64_t Page_Table_Size = 4 << 10;
}  // namespace


namespace Memo
{
    namespace AMD64
    {
        Pager::Pager(
                uint64_t cr3,
                uint64_t begin,
                uint64_t limit):
            _heap(nullptr),
            _console(Publics(0x0e00)),
            _repo_count(0),
            _repos(nullptr),
            _cr3(cr3),
            _pml4(cr3 & 0xfffffffffffff000ULL),
            _begin(begin),
            _end(_begin + (3 * Page_Table_Size)),
            _limit(limit)
        {}

        bool
        Pager::Initialize(uint64_t count, Page_Repository* repos)
        {
            if ((_repos != nullptr) || (_repo_count != 0))
                return false;

            _repo_count = count;
            _repos = repos;

        //    for (uint64_t i = 0; i < _repo_count; ++i)
        //    {
        //        _console.Put_Char('['); _console.Print(i);                     _console.Put_Char(']'); _console.Put_Char(' ');
        //                                _console.Print(Hex(&repos[i]));        _console.Put_Char(':'); _console.Put_Char(' ');
        //                                _console.Print(Hex(repos[i].Begin())); _console.Put_Char(' '); _console.Put_Char('-'); _console.Put_Char(' ');
        //                                _console.Print(Hex(repos[i].End()));   _console.New_Line();
        //    }

            return true;
        }

        size_t
        Pager::get_entry_index(uint64_t virtual_address, size_t starting_bit)
        { return ((virtual_address >> starting_bit) & 0x1ff); }

        template<
            typename Table,
            typename Parent_Table = typename Table::Parent_Table,
            typename Parent_Entry = typename Parent_Table::Entry>
        bool
        Pager::create_new_table(
                Parent_Table& parent_table,
                size_t table_index,
                uint64_t page_table_address,
                Table& new_table)
        {
            if (page_table_address == 0)  // Zero is invalid.
                return false;

            Create_Page_Result return_value;
            parent_table.Create_Entry(table_index, page_table_address, return_value);

            new_table = Table{get_virtual_address(page_table_address)};

            return true;
        }

        uint64_t Pager::get_virtual_address(uint64_t physical_address)
        {
            return physical_address - (_cr3 & ~0xfffULL) + _pml4;
        }

        uint64_t Pager::get_physical_address(uint64_t virtual_address)
        {
            return virtual_address - _pml4 + (_cr3 & ~0xfffULL);
        }

        template<typename Parent_Table, typename Page_Table>
        Create_Page_Result
        Pager::create_page_helper(
                Parent_Table& parent_table,
                Page_Table& page_table,
                uint64_t index,
                uint64_t physical_address,
                bool entry_is_page)
        {
            Create_Page_Result return_value = Create_Page_Result::Unknown_Error;

            typename Parent_Table::Entry* parent_entry{parent_table[index]};
            page_table = Page_Table{0};

        //    _console.Print('[');
        //        _console.Print(Hex(parent_table[0])); _console.Print(' ');
        //        _console.Print(Hex(parent_entry)); _console.Print(' ');
        //        _console.Print(Hex(parent_entry->entry_bits)); _console.Print(' ');
        //        _console.Print(entry_is_page? 'P': 'T');
        //    _console.Puts(']');

            if (parent_entry->Is_Present())
            {
                // Entry can be page or table.  Let's verify.
                if (parent_entry->Is_Page())
                {
                    return_value =
                        entry_is_page?
                            Create_Page_Result::Already_Exists:
                            Create_Page_Result::Could_Not_Create_Entries;
                } else {
                    auto vaddr = get_virtual_address(parent_entry->Physical_Address());
                    page_table = Page_Table{vaddr};

                    return_value =
                        entry_is_page?
                            Create_Page_Result::Could_Not_Create_Entries:
                            Create_Page_Result::Already_Exists;
                }
            } else {
                if (entry_is_page)
                {
                    parent_table.Create_Entry(index, physical_address, return_value)
                        .Set_Page()
                        .Set_Global_Page();
                } else {
                    return_value =
                        create_new_table(parent_table, index, get_physical_address(_end), page_table)?
                            Create_Page_Result::Success:
                            Create_Page_Result::Could_Not_Create_Entries;

                    _end+= Page_Table_Size;
                }
            }

            return return_value;
        }

        uint64_t Pager::get_free_frame(Page_Size page_size)
        {
            uint64_t physical_address = 0;

            for (int i = _repo_count; i > 0; --i)
            {
                physical_address = _repos[i - 1].Get_Page(uint64_t(page_size));

                if (physical_address != 0)
                    break;
            }

            return physical_address;
        }

        Create_Page_Result
        Pager::Create_Page(
                Page_Size page_size,
                uint64_t virtual_address,
                uint64_t& physical_address)
        {
            switch(page_size)
            {
                case Page_Size::_4_KiB:
                case Page_Size::_2_MiB:
                case Page_Size::_1_GiB:
                    break;

                default:
                    return Create_Page_Result::Size_Unknown;
            }

            if (physical_address == 0)
                physical_address = get_free_frame(page_size);

            if (physical_address == 0)
                return Create_Page_Result::Not_Enough_Memory;

            size_t pml4_index = get_entry_index(virtual_address, 39); // 512 GiB per entry.
            size_t pdpt_index = get_entry_index(virtual_address, 30); // 1 GiB per entry.
            size_t pd_index   = get_entry_index(virtual_address, 21); // 2 MiB per entry.
            size_t pt_index   = get_entry_index(virtual_address, 12); // 4 KiB per entry.

        //    _console.New_Line();
        //
        //                if ((virtual_address & 0x3fff) == 0)
        //    {
        //        _console.Print('[');    _console.Print(Hex(virtual_address));
        //        _console.Print(' '); _console.Print('-'); _console.Print('>'); _console.Print(' ');
        //        _console.Print(Hex(physical_address)); _console.Print(']'); _console.Print(' ');
        //
        //        _console.Print('[');
        //        _console.Print(Hex(pml4_index));    _console.Print(',');    _console.Print(' ');
        //        _console.Print(Hex(pdpt_index));    _console.Print(',');    _console.Print(' ');
        //        _console.Print(Hex(pd_index));      _console.Print(',');    _console.Print(' ');
        //        _console.Print(Hex(pt_index));      _console.Print(']');    _console.Print(' ');
        //
        //        _console.New_Line();
        //    }

            PDPT pdpt {0};
            PD pd{0};
            PT pt{0};
            PT dummy{0};

            Create_Page_Result return_value = Create_Page_Result::Unknown_Error;

        //    _console.Puts(Hex(_pml4));

            return_value =
                Pager::create_page_helper(
                        _pml4,
                        pdpt,
                        pml4_index,
                        physical_address,
                        false);

            if ((return_value != Create_Page_Result::Success) &&
                (return_value != Create_Page_Result::Already_Exists))
                return return_value;

            return_value =
                Pager::create_page_helper(
                        pdpt,
                        pd,
                        pdpt_index,
                        physical_address,
                        (page_size == Page_Size::_1_GiB));

            if ((return_value != Create_Page_Result::Success) &&
                (return_value != Create_Page_Result::Already_Exists))
                return return_value;

            if (page_size == Page_Size::_1_GiB)
                return return_value;

            return_value =
                Pager::create_page_helper(
                        pd,
                        pt,
                        pd_index,
                        physical_address,
                        (page_size == Page_Size::_2_MiB));

            if ((return_value != Create_Page_Result::Success) &&
                (return_value != Create_Page_Result::Already_Exists))
                return return_value;

            if (page_size == Page_Size::_2_MiB)
                return return_value;

            return_value =
                Pager::create_page_helper(
                        pt,
                        dummy,
                        pt_index,
                        physical_address,
                        true);

            return return_value;
        }

        void
        Pager::Migrate_Page_Tables(
                uint64_t Physical_Source_Address,
                uint64_t Virtual_Destination_Address,
                uint64_t Physical_Destination_Address,
                uint64_t Size_In_Bytes)
        {
            uint64_t Virtual_Source_Address = _begin;
            uint64_t* source = reinterpret_cast<uint64_t*>(Virtual_Source_Address);
            uint64_t* destination = reinterpret_cast<uint64_t*>(Virtual_Destination_Address);
            uint64_t word_count = Size_In_Bytes / sizeof(uint64_t);

            for (uint64_t i = 0; i < word_count; ++i)
            {
                uint64_t& orig = source[i];
                uint64_t& vdst = destination[i];

                if (orig == 0)
                {
                    vdst = 0;
                    continue;
                } else
                if (Base_Page_Table::Entry{orig}.Is_Page())
                {
                    vdst = orig;
                } else {
                    vdst = orig - Physical_Source_Address + Physical_Destination_Address;
                }
            }

            _cr3 = Physical_Destination_Address;
            _pml4 = Virtual_Destination_Address;
            _begin = Virtual_Destination_Address;
            _end = _end - Virtual_Source_Address + Virtual_Destination_Address;
            _limit = _limit - Virtual_Source_Address + Virtual_Destination_Address;

            Set_Paging_Root(_cr3);
        }
    }
}

