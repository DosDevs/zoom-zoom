#ifndef SECTOR_H__INCLUDED
#define SECTOR_H__INCLUDED


#include "I_Sector.h"

#include <algorithm>
#include <vector>

#include "Octet.h"

namespace NewFs {
    using std::vector;

    template<size_t k_Octet_Count = 512, typename Data_Type = uint8_t>
    class Sector: public I_Sector<Data_Type>
    {
        private:
            Data_Type _units[k_Octet_Count];

        public:
            Sector():
                _units()
            {
                std::fill(&_units[0], &_units[k_Octet_Count], 0);
            }

            /*
             * Interface from I_Sector
             */
            virtual Data_Type const& operator[](size_t index) const
            { return _units[index]; }

            virtual Data_Type& operator[](size_t index)
            { return _units[index]; }

            virtual size_t Octet_Count() const
            { return k_Octet_Count; }
    };  // Sector
}

#endif  // SECTOR_H__INCLUDED
