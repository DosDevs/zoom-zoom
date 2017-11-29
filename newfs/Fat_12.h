#ifndef FAT_12_H__INCLUDED
#define FAT_12_H__INCLUDED


#include "I_Fat.h"

#include <vector>

#include "Bpb.h"
#include "I_Storage_Device.h"


namespace NewFs {
    using std::vector;

    struct Fat_12_Directory_Entry: File_Directory_Entry
    {
        char File_Name[11];
        uint8_t Attributes;
        uint8_t Reserved_MBZ_1;
        uint8_t Reserved_MBZ_2;
        uint16_t Create_Time;
        uint16_t Create_Date;
        uint16_t Reserved_MBZ_3;
        uint16_t Reserved_MBZ_4;
        uint16_t Last_Modified_Time;
        uint16_t Last_Modified_Date;
        uint16_t Start_Cluster;
        uint16_t File_Size_In_Bytes;
    };

    class Fat_12: public I_Fat
    {
        public:
            typedef Bpb_Dos_3_0 Bpb;

        private:
            size_t const k_Sector_Size = 512;

            I_Storage_Device<uint8_t>& Storage;
            size_t Boot_Record_Sector_Count;

            Bpb _bpb;

            uint64_t
            Get_Cluster_Value(uint16_t Index);

            bool
            Is_Cluster_Free(uint16_t Index);

            vector<uint16_t>
            Find_Cluster_Chain(size_t File_Size);

            vector<uint16_t>
            GetTotalClusterCount();

            void
            Write_Cluster(
                uint64_t Cluster_Index,
                uint64_t Cluster_Value,
                uint8_t const* Data_Begin,
                uint8_t const* Data_End);

            File_Directory_Entry
            Write_Directory_Entry(
                string const& File_Name,
                uint64_t First_Cluster,
                uint64_t File_Size);

        public:
            Fat_12(I_Storage_Device<uint8_t>& Storage):
                Storage(Storage),
                Boot_Record_Sector_Count(1),
                _bpb()
            {}

            virtual bool Format(Bpb const& bpb);

            virtual vector<uint8_t> Read_Boot_Record() const;

            virtual
            File_Directory_Entry
            Find_File(
                size_t Directory_First_Cluster) const;

            virtual
            bool
            Write_Boot_Record(
                uint8_t const* Data_Begin,
                uint8_t const* Data_End);

            virtual
            File_Directory_Entry
            Write_File(
                string const& File_Name,
                uint8_t const* Data_Begin,
                uint8_t const* Data_End);
    };
}

#endif  // FAT_12_H__INCLUDED
