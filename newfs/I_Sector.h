#ifndef I_SECTOR_H__INCLUDED
#define I_SECTOR_H__INCLUDED


#include <string>

#include "Octet.h"


namespace NewFs {
    template<typename T_Data_Type = Octet>
    struct I_Sector 
    {
        typedef T_Data_Type Data_Type;

        virtual Data_Type const& operator[](size_t index) const = 0;
        virtual Data_Type& operator[](size_t index) = 0;

        virtual size_t Octet_Count() const = 0;

        virtual ~I_Sector() {}
    };  // I_Sector
}

#endif  // I_SECTOR_H__INCLUDED
