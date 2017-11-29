#include "Page_Repository.h"

#include <Console.h>

#include "StaticHeap.h"



Page_Repository::Page_Repository():
    _heap(nullptr),
    _head(nullptr),
    _begin(nullptr),
    _end(nullptr)
{}

uint64_t
Page_Repository::Get_Page(uint64_t size)
{
    // Zero (0) is invalid.
    uint64_t address = 0;

    for(Free_Block* iter = _head, *prev = nullptr;
        iter != nullptr;
        prev = iter, iter = iter->Next)
    {
        uint64_t block_size = iter->End - iter->Begin;

        if (block_size < size)
            continue;

        uint64_t end = iter->End;
        uint64_t begin = end - size;
        uint64_t aligned_begin = begin - (begin % size);
        uint64_t aligned_end = aligned_begin + size;

        if (aligned_begin < iter->Begin)
            continue;

        Free_Block* one = nullptr;
        Free_Block* two = nullptr;
        Free_Block* three = nullptr;
        Split_Block(iter, aligned_begin, one, two);
        Split_Block(two, aligned_end, two, three);

        // Take address.
        address = two->Begin;

        // No longer free.
        Remove_Block(one? one: prev, two);

        break;
    }

    return address;
}

void
Page_Repository::Initialize(
        Memo::Static_Heap& heap,
        uint64_t begin,
        uint64_t end)
{
    _begin = reinterpret_cast<void*>(begin);
    _end = reinterpret_cast<void*>(end);

    _heap = &heap;
    heap.Allocate(_head);

    if (_head != nullptr)
        (*_head) = Free_Block(begin, end, nullptr);
}

void
Page_Repository::Split_Block(
        Free_Block* _this,
        uint64_t address,
        Free_Block*& one,
        Free_Block*& two)
{
    uint64_t this_begin = _this->Begin;
    uint64_t this_end = _this->End;
    Free_Block* next = _this->Next;

    one = nullptr;
    two = nullptr;

    if ((_this == nullptr) ||
        (address < this_begin) ||
        (address > this_end))
    {
        return;
    }

    // Case one: split at the beginning.  Block is two.
    if (address == this_begin)
    {
        two = _this;
        return;
    }

    one = _this;

    // Case two: split at the end.  Block is one.
    if (address == this_end)
    {
        return;
    }

    // Case three: split somewhere in the middle.
    _heap->Allocate(two);

    one->Begin = this_begin;
    one->End = address;
    one->Next = two;

    two->Begin = address;
    two->End = this_end;
    two->Next = next;
}

Page_Repository::Free_Block*
Page_Repository::Remove_Block(
        Free_Block* prev,
        Free_Block*& _this)
{
    if (_this == nullptr)
        return nullptr;

    if (prev != nullptr)
        prev->Next = _this->Next;

    _heap->Deallocate(_this);
    return prev->Next;
}

