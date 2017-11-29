#ifndef I_FAT_H__INCLUDED
#define I_FAT_H__INCLUDED


#include "I_Storage_Device.h"

#include <vector>


namespace NewFs {
    using std::vector;

    struct File_Directory_Entry {};

    struct I_Fat
    {
        virtual vector<uint8_t> Read_Boot_Record() const = 0;

        virtual
        File_Directory_Entry
        Find_File(
            size_t Directory_First_Cluster) const = 0;

        virtual
        bool
        Write_Boot_Record(
            uint8_t const* Data_Begin,
            uint8_t const* Data_End) = 0;

        virtual
        File_Directory_Entry
        Write_File(
            string const& File_Name,
            uint8_t const* Data_Begin,
            uint8_t const* Data_End) = 0;

        virtual ~I_Fat() {}
    };
}

#endif  // I_FAT_H__INCLUDED
