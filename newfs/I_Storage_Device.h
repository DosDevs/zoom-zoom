#ifndef I_STORAGE_DEVICE_H__INCLUDED
#define I_STORAGE_DEVICE_H__INCLUDED


#include <string>

#include "I_Sector.h"


namespace NewFs {
    template<typename Data_Type>
    struct I_Storage_Device
    {
        typedef I_Sector<Data_Type> Sector_Type;

        virtual Sector_Type const& operator[](size_t index) const = 0;
        virtual Sector_Type& operator[](size_t index) = 0;

        virtual size_t Sector_Size() const = 0;
        virtual size_t Sector_Count() const = 0;
        virtual size_t Octet_Count() const = 0;

        virtual ~I_Storage_Device() {}
    };  // I_Storage_Device
}

#endif  // I_STORAGE_DEVICE_H__INCLUDED
