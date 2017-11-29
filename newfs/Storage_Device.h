#ifndef STORAGE_DEVICE_H__INCLUDED
#define STORAGE_DEVICE_H__INCLUDED


#include "I_Storage_Device.h"

#include <vector>

#include "Octet.h"


namespace NewFs {
    using std::vector;


    template<
        typename Sector_Type,
        typename Data_Type = typename Sector_Type::Data_Type>
    class Storage_Device: public I_Storage_Device<Data_Type>
    {
        private:
            size_t _sector_size;
            vector<Sector_Type> _sectors;

        public:
            Storage_Device(size_t sector_size, size_t sector_count):
                _sector_size(sector_size),
                _sectors(sector_count)
            {}

            /*
             * Interface from I_Storage_Device
             */
            virtual Sector_Type const& operator[](size_t index) const
            { return _sectors[index]; }

            virtual Sector_Type& operator[](size_t index)
            { return _sectors[index]; }

            virtual size_t Sector_Size() const
            { return _sector_size; }

            virtual size_t Sector_Count() const
            { return _sectors.size(); }

            virtual size_t Octet_Count() const
            { return Sector_Size() * Sector_Count(); }
    };  // Storage_Device
}

#endif  // STORAGE_DEVICE_H__INCLUDED
