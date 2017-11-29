#include "StaticHeap.h"


using Memo::Static_Heap;


namespace {
    constexpr uint64_t TAKEN_FLAG = 0x8000000000000000ULL;
    constexpr uint64_t SIZE_MASK = 0xffffffffffffULL;
}


Static_Heap::Static_Heap(
        Pager& pager,
        uint64_t begin,
        uint64_t limit):
    _pager(pager),
    _begin(reinterpret_cast<uint64_t*>(begin)),
    _end(_begin),
    _current_limit(_begin),
    _limit(reinterpret_cast<uint64_t*>(limit)),
    _free_block_list(nullptr)
{}

uint64_t*
Static_Heap::Find_Free_Block(uint64_t word_count)  // 64-bit words
{
    auto marker = _begin;

    while (marker < _limit)
    {
        uint64_t header = *marker;

        if (header == 0)
            break;

        uint64_t block_length = header & SIZE_MASK;

        if ((header & TAKEN_FLAG) != 0)
        {
        } else
        if (block_length > word_count)
        {
        } else {
            break;
        }

        marker+= block_length;
    }

    return marker;
}

uint64_t
Static_Heap::Allocate(uint64_t size)
{
    if (size == 0)
        return 0;

    uint64_t block_length = ((size / 8) + int((size % 8) != 0) + 1);

    auto block = Find_Free_Block(block_length);

    if (block >= _limit)
        return 0;

    while ((_current_limit - block) < (block_length + (65536 / 8)))
    {
        uint64_t physical = 0;
        Add_Page(physical);

        if (physical == 0)
            break;

        Console console;
        console.Puts(physical);
    }

    block[0] = block_length | TAKEN_FLAG;

    _end = std::max(_end, &block[block_length]);

    return reinterpret_cast<uint64_t>(&block[1]);
}

void
Static_Heap::Deallocate(uint64_t pointer)
{
    uint64_t* marker = reinterpret_cast<uint64_t*>(pointer);
    marker[-1] |= TAKEN_FLAG;
}

void
Static_Heap::Add_Page(uint64_t& physical_address)
{
    auto Result =
        _pager.Create_Page(
                Page_Size::_2_MiB,
                uint64_t(_current_limit),
                physical_address);

    if (Result == Create_Page_Result::Success)
        _current_limit+= ((2 << 20) / 8);
}

void
Static_Heap::Initialize(uint64_t first_available_physical_page)
{
    Add_Page(first_available_physical_page);
}

