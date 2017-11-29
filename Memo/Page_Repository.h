#ifndef PAGE_REPOSITORY_H__INCLUDED
#define PAGE_REPOSITORY_H__INCLUDED


#include <stdint.h>


namespace Memo
{
    class Static_Heap;
}


class Page_Repository
{
    private:
        struct Free_Block
        {
            uint64_t Begin;
            uint64_t End;
            Free_Block* Next;

            Free_Block(uint64_t begin, uint64_t end, Free_Block* next):
                Begin(begin),
                End(end),
                Next(next)
            {}

            void
            Split(
                    Free_Block* _this,
                    uint64_t address,
                    Free_Block*& one,
                    Free_Block*& two)
            {
            }
        };

        Memo::Static_Heap* _heap;
        Free_Block* _head;

        void* _begin;
        void* _end;

        void
        Split_Block(
                Free_Block* _this,
                uint64_t address,
                Free_Block*& one,
                Free_Block*& two);

        Free_Block*
        Remove_Block(
                Free_Block* prev,
                Free_Block*& _this);

    public:
        Page_Repository();

        void Initialize(Memo::Static_Heap& heap, uint64_t begin, uint64_t end);

        uint64_t Get_Page(uint64_t size);

        void const* Begin() const { return _begin; }
        void const* End() const { return _end; }
};

#endif  // PAGE_REPOSITORY_H__INCLUDED

