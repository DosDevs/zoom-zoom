#pragma once

#include <stdint.h>

#include <stdlib.h>

#include <Console.h>
#include <Publics.h>

#include "Lowlies.h"
#include "Page_Repository.h"


#define ENTRIES_PER_TABLE 512


namespace Memo {
    class Static_Heap;


    enum class Create_Page_Result {
        Success,
        Unknown_Error,
        Already_Exists,
        Size_Unknown,
        Not_Enough_Memory,
        Could_Not_Create_Entries
    };


    enum class Page_Size {
        _4_KiB = (1ULL << 12),
        _2_MiB = (1ULL << 21),
        _1_GiB = (1ULL << 30)
    };


    namespace AMD64
    {
        class Base_Page_Table
        {
            public:
                struct Entry
                {
                    uint64_t entry_bits;

                    Entry(Entry const& that): entry_bits(that.entry_bits) {}
                    Entry(uint64_t const& bits): entry_bits(bits) {}
                    Entry(uint64_t const* bits): entry_bits(*bits) {}

                    bool bit(int bit_index) const
                    { return ((entry_bits >> bit_index) & 1); }

                    uint64_t bits(int begin, int span) const
                    { return ((entry_bits << (64 - (begin + span))) >> (64 - span)); }

                    bool Is_Present() const             { return bit(0); }
                    bool Has_Write_Access() const       { return bit(1); }
                    bool Is_User_Memory() const         { return bit(2); }
                    bool Is_Write_Through() const       { return bit(3); }
                    bool Has_Cache_Disabled() const     { return bit(4); }
                    bool Was_Accessed() const           { return bit(5); }
                    bool Is_Dirty() const               { return Is_Page() && bit(6); }
                    bool Is_Page() const                { return bit(7); }
                    bool Is_Global_Page() const         { return Is_Page() && bit(8); }
                    bool Has_Pat() const                { return Is_Page() && bit(12); }
                    uint64_t Physical_Address() const   { return bits(12, 40) << 12; }
                    uint8_t Key() const                 { return !Is_Page()? 0: bits(59, 4); }
                    bool Has_Execute_Disabled() const   { return bit(63); }

                    Entry& set(int bit_index)
                    {
                        entry_bits|= (uint64_t(1) << bit_index);
                        return *this;
                    }

                    Entry& Set_Present()           { return set(0); }
                    Entry& Set_Write_Access()      { return set(1); }
                    Entry& Set_User_Memory()       { return set(2); }
                    Entry& Set_Write_Through()     { return set(3); }
                    Entry& Set_Cache_Disabled()    { return set(4); }
                    Entry& Set_Accessed()          { return set(5); }
                    Entry& Set_Dirty()             { return set(6); }
                    Entry& Set_Page()              { return set(7); }
                    Entry& Set_Global_Page()       { return set(8); }
                    Entry& Set_Execute_Disabled()  { return set(63); }

                    template<typename Entry_Type>
                    Entry_Type& Set(Entry_Type const& that)
                    {
                        this->entry_bits = that.entry_bits;
                        return static_cast<Entry_Type&>(*this);
                    }

                    operator uint64_t const() const { return entry_bits; }
                };  // struct Entry

            public:  // private:
                Entry* entries;

            protected:
                Base_Page_Table(uint64_t address):
                    entries(reinterpret_cast<Entry*>(address))
                {}

                Base_Page_Table(uint64_t* address):
                    entries(reinterpret_cast<Entry*>(*address))
                {}

                Base_Page_Table(Base_Page_Table const& that):
                    entries(that.entries)
                {}

                Base_Page_Table& operator=(Base_Page_Table  const& that)
                {
                    entries = that.entries;
                    return *this;
                }

                template<typename Entry_Type>
                Entry_Type* get(size_t index)
                {
                    return
                        ((index > ENTRIES_PER_TABLE)?
                            0:
                            static_cast<Entry_Type*>(&entries[index]));
                }

                // Since there is no real way to know if a page table
                // has been initialized, we will, for now, forbid page
                // tables to start at physical address 0.
                bool Is_Valid()
                {
                    return (entries != 0);
                }

                template<typename Entry_Type>
                Entry_Type const* get(size_t index) const
                { return const_cast<Base_Page_Table*>(this)->get<Entry_Type>(index); }

            public:
                operator uint64_t const() const { return reinterpret_cast<uint64_t>(&entries[0]); }
        };

        class PML4;
        class PDPT;
        class PD;
        class PT;

        class PML4: public Base_Page_Table
        {
            private:
                typedef Base_Page_Table::Entry Base_Entry;

            public:
                struct Entry: Base_Entry
                {
                    Entry(Entry const& that): Base_Entry(that.entry_bits) {}
                    Entry(Entry const* that): Base_Entry(that->entry_bits) {}
                    Entry(uint64_t& bits): Base_Entry(bits) {}
                    Entry(uint64_t* bits): Base_Entry(*bits) {}

                    bool Is_Page() const            { return 0; }

                    // TODO: Make this fail.
                    Entry& Set_Page()               { return *this; }
                };

                typedef PDPT Child_Table;

                PML4(uint64_t pml4_address): Base_Page_Table(pml4_address) {}

                PML4 operator=(PML4 const& that)
                { return static_cast<PML4&>(Base_Page_Table::operator=(that)); }

                Entry* operator[](size_t index) { return get(index); }
                Entry const* operator[](size_t index) const { return get(index); }

                Entry* get(size_t index) { return Base_Page_Table::get<Entry>(index); }
                Entry const* get(size_t index) const { return Base_Page_Table::get<Entry>(index); }

                Entry&
                Create_Entry(size_t index, uint64_t physical_address, Create_Page_Result& return_value)
                {
                    return_value = Create_Page_Result::Success;

                    return
                        get(index)->Set<Entry>(
                            static_cast<Entry&>(
                                Entry{physical_address}
                                    .Set_Present()
                                    .Set_Cache_Disabled()
                                    .Set_Write_Access()
                                    .Set_Write_Through()));
                }
        };  // class PML4

        class PDPT: public Base_Page_Table
        {
            private:
                typedef Base_Page_Table::Entry Base_Entry;

            public:
                struct Entry: Base_Entry
                {
                    Entry(Base_Entry const& that): Base_Entry(that.entry_bits) {}
                    Entry(uint64_t& bits): Base_Entry(bits) {}
                    Entry(uint64_t* bits): Base_Entry(*bits) {}

                    uint64_t Physical_Address() const
                    {
                        return
                            Is_Page()?
                                bits(30, 40) << 30:
                                Base_Entry::Physical_Address();
                    }
                };

                typedef PML4 Parent_Table;
                typedef PD Child_Table;

                PDPT(uint64_t pdpt_address): Base_Page_Table(pdpt_address) {}

                PDPT operator=(PDPT const& that)
                { return static_cast<PDPT&>(Base_Page_Table::operator=(that)); }

                Entry* operator[](size_t index) { return get(index); }
                Entry const* operator[](size_t index) const { return get(index); }

                Entry* get(size_t index) { return Base_Page_Table::get<Entry>(index); }
                Entry const* get(size_t index) const { return Base_Page_Table::get<Entry>(index); }

                Entry&
                Create_Entry(size_t index, uint64_t physical_address, Create_Page_Result& return_value)
                {
                    return_value = Create_Page_Result::Success;

                    return
                        get(index)->Set<Entry>(
                            static_cast<Entry&>(
                                Entry{physical_address}
                                    .Set_Present()
                                    .Set_Cache_Disabled()
                                    .Set_Write_Access()
                                    .Set_Write_Through()));
                }
        };  // class PDPT

        class PD: public Base_Page_Table
        {
            private:
                typedef Base_Page_Table::Entry Base_Entry;

            public:
                struct Entry: Base_Entry
                {
                    Entry(Base_Entry const& that): Base_Entry(that.entry_bits) {}
                    Entry(uint64_t& bits): Base_Entry(bits) {}
                    Entry(uint64_t* bits): Base_Entry(*bits) {}

                    uint64_t Physical_Address() const
                    {
                        return
                            Is_Page()?
                                bits(21, 40) << 21:
                                Base_Entry::Physical_Address();
                    }
                };

                typedef PDPT Parent_Table;
                typedef PT Child_Table;

                PD(uint64_t pd_address): Base_Page_Table(pd_address) {}

                PD operator=(PD const& that)
                { return static_cast<PD&>(Base_Page_Table::operator=(that)); }

                Entry* operator[](size_t index) { return get(index); }
                Entry const* operator[](size_t index) const { return get(index); }

                Entry* get(size_t index) { return Base_Page_Table::get<Entry>(index); }
                Entry const* get(size_t index) const { return Base_Page_Table::get<Entry>(index); }

                Entry&
                Create_Entry(size_t index, uint64_t physical_address, Create_Page_Result& return_value)
                {
                    return_value = Create_Page_Result::Success;

                    return
                        get(index)->Set<Entry>(
                            static_cast<Entry&>(
                                Entry{physical_address}
                                    .Set_Present()
                                    .Set_Cache_Disabled()
                                    .Set_Write_Access()
                                    .Set_Write_Through()));
                }
        };  // class PD

        class PT: public Base_Page_Table
        {
            private:
                typedef Base_Page_Table::Entry Base_Entry;

            public:
                struct Entry: Base_Entry
                {
                    Entry(Base_Entry const& that): Base_Entry(that.entry_bits) {}
                    Entry(uint64_t& bits): Base_Entry(bits) {}
                    Entry(uint64_t* bits): Base_Entry(*bits) {}

                    bool Is_Page() const            { return 1; }
                    bool Has_Pat() const            { return bit(7); }

                    // TODO: Make this fail.
                    Entry& Set_Page()               { return *this; }
                    Entry& Set_Pat()                { return static_cast<Entry&>(set(7)); }
                };

                typedef PD Parent_Table;

                PT(uint64_t pt_address): Base_Page_Table(pt_address) {}

                PT operator=(PT const& that)
                { return static_cast<PT&>(Base_Page_Table::operator=(that)); }

                Entry* operator[](size_t index) { return get(index); }
                Entry const* operator[](size_t index) const { return get(index); }

                Entry * get(size_t index) { return Base_Page_Table::get<Entry>(index); }
                Entry const* get(size_t index) const { return Base_Page_Table::get<Entry>(index); }

                Entry&
                Create_Entry(size_t index, uint64_t physical_address, Create_Page_Result& return_value)
                {
                    return_value = Create_Page_Result::Success;

                    return
                        get(index)->Set<Entry>(
                            static_cast<Entry&>(
                                Entry{physical_address}
                                    .Set_Page()  // Entry can never be table.
                                    .Set_Present()
                                    .Set_Cache_Disabled()
                                    .Set_Write_Access()
                                    .Set_Write_Through()));
                }
        };  // class PT

        class Pager
        {
            private:
                Memo::Static_Heap* _heap;

                uint64_t _repo_count;
                ::Page_Repository* _repos;

                Console _console;

                uint64_t _cr3;
                PML4 _pml4;

                uint64_t _begin;
                uint64_t _end;
                uint64_t _limit;

                size_t get_entry_index(uint64_t virtual_address, size_t starting_bit);
                uint64_t get_virtual_address(uint64_t physical_address);
                uint64_t get_physical_address(uint64_t virtual_address);

                template<
                    typename Table,
                    typename Parent_Table,
                    typename Parent_Entry>
                bool
                create_new_table(
                        Parent_Table& parent_table,
                        size_t table_index,
                        uint64_t page_table_address,
                        Table& new_table);

                template<typename Parent_Table, typename Page_Table>
                Create_Page_Result
                create_page_helper(
                        Parent_Table& parent_table,
                        Page_Table& page_table,
                        uint64_t index,
                        uint64_t physical_address,
                        bool entry_is_page);

                uint64_t get_free_frame(Page_Size size);

            public:
                Pager(uint64_t cr3, uint64_t begin, uint64_t limit);

                bool Set_Heap(Memo::Static_Heap* heap)
                {
                    _heap = heap;
                    return true;
                }

                bool Initialize(uint64_t count, ::Page_Repository* repos);

                Create_Page_Result
                Create_Page(
                        Page_Size page_size,
                        uint64_t virtual_address,
                        uint64_t& physical_address);

                void
                Migrate_Page_Tables(
                        uint64_t Physical_Source_Address,
                        uint64_t Virtual_Destination_Address,
                        uint64_t Physical_Destination_Address,
                        uint64_t Size_In_Bytes);
        };
    }  // namespace AMD64
}  // namespace Memo

