#pragma once

#include <stdint.h>


namespace Memo
{
    namespace AMD64
    {
        class Address_Resolver
        {
            private:
                uint64_t _virtual_address;
                uint64_t _physical_address;
                uint64_t _pml4_index;
                uint64_t _pdpt_index;
                uint64_t _pd_index;
                uint64_t _pt_index;

                uint64_t index(uint64_t table_address, int index)
                {
                    return reinterpret_cast<uint64_t*>(table_address)[index];
                }

            public:
                Address_Resolver(
                        uint64_t virtual_address,
                        uint64_t physical_address):
                    _virtual_address(virtual_address),
                    _physical_address(physical_address),
                    _pml4_index((virtual_address >> 30) & 0xfff),
                    _pdpt_index((virtual_address >> 21) & 0x1ff),
                    _pd_index(  (virtual_address >> 12) & 0x1ff),
                    _pt_index(  (virtual_address >>  0) & 0xfff)
                {}

                uint64_t Get_PDPT_Physical_Address(
                        uint64_t PML4_virtual_address)
                {
                    return index(PML4_virtual_address, _pml4_index);
                }

                uint64_t Get_PD_Physical_Address(
                        uint64_t PDPT_virtual_address)
                {
                    return index(PDPT_virtual_address, _pdpt_index);
                }

                uint64_t Get_PT_Physical_Address(
                        uint64_t PD_virtual_address)
                {
                    return index(PD_virtual_address, _pd_index);
                }

                uint64_t Get_Physical_Address(
                        uint64_t PT_virtual_address)
                {
                    return index(PT_virtual_address, _pt_index);
                }
        };  // struct Address_Resolver
    }  // namespace AMD64
}  // namespace Memo

