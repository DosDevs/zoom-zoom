#ifndef STATIC_HEAP_H__INCLUDED
#define STATIC_HEAP_H__INCLUDED

#include "AMD64.h"


namespace AMD64
{
    class Pager;
}

namespace Memo
{
    class Static_Heap
    {
        private:
            typedef AMD64::Pager Pager;

            struct Free_Block
            {
                uint32_t Offset;
                uint32_t Size;
            };  // struct Free_Block

            Pager& _pager;

            uint64_t* _begin;
            uint64_t* _end;
            uint64_t* _current_limit;
            uint64_t* _limit;

            Free_Block* _free_block_list;

            uint64_t* Find_Free_Block(uint64_t size);
            uint64_t Allocate(uint64_t size);
            void Deallocate(uint64_t pointer);

            void Add_Page(uint64_t& physical_address);

        public:
            Static_Heap(
                    Pager& pager,
                    uint64_t begin,
                    uint64_t limit);

            template<typename T>
            void Allocate(T*& pointer, int element_count = 1)
            {
                uint64_t address = Allocate(element_count * sizeof(T));
                pointer = reinterpret_cast<T*>(address);
            }

            template<typename T>
            void Deallocate(T*& pointer)
            {
                Deallocate(reinterpret_cast<uint64_t>(pointer));
                pointer = nullptr;
            }

            void Initialize(uint64_t first_available_physical_page);

            uint64_t Begin() const { return reinterpret_cast<uint64_t>(_begin); }
            uint64_t End() const { return reinterpret_cast<uint64_t>(_end); }
    };
}  //namespace Memo

#endif  // STATIC_HEAP_H__INCLUDED

