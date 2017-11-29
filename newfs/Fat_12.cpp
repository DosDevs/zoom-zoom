#include "Fat_12.h"

#include <algorithm>
#include <iostream>
#include <vector>

#include <string.h>


void Dump(uint8_t* Buffer, size_t Byte_Count)
{
    std::cout << std::hex;

    for (size_t i = 0; i < Byte_Count; i += 128)
    {
        for (size_t j = 0; j < 128; j+= 16)
        {
            size_t ij = i + j, k = 0;
            printf("%08lx ", ij);

            for (; k < 8; ++k)
                printf(" %02x", Buffer[ij+k]);

            printf(" -");

            for (; k < 16; ++k)
                printf(" %02x", Buffer[ij+k]);

            puts("");
        }

        puts("");
    }
}

namespace NewFs {
    using std::cout;
    using std::endl;
    using std::vector;

    uint64_t const k_End_Of_Chain_12 = 0x0fff;

    bool
    Fat_12::Format(Bpb const& bpb)
    {
        _bpb = bpb;
        uint64_t Offset = NewFs::k_Bpb_Offset;
        _bpb.Write(Storage[0], Offset);

        cout
            << "BPB is " << (Offset - NewFs::k_Bpb_Offset)
            << " bytes long." << endl;

        uint64_t Index = _bpb.First_Fat_Sector();

        for(size_t i = 0; i < _bpb.Total_Fat_Count; ++i)
        {
            auto& Sector = Storage[Index];

            Sector[0] = 0xf0;
            Sector[1] = 0xff;
            Sector[2] = 0xff;

            Index+= _bpb.Sectors_Per_Fat;
        }

        return true;
    }

    vector<uint8_t>
    Fat_12::Read_Boot_Record() const
    {
        vector<uint8_t> Result(Boot_Record_Sector_Count * k_Sector_Size, 0);

        for(size_t i = 0; i < Boot_Record_Sector_Count; ++i)
            std::copy(
                &Storage[i][0],
                &Storage[i][k_Sector_Size],
                &Result[i * k_Sector_Size]);

        return Result;
    }

    File_Directory_Entry
    Fat_12::Find_File(size_t Directory_First_Cluster) const
    {
        return File_Directory_Entry();
    }

    bool
    Fat_12::Write_Boot_Record(
        uint8_t const* Data_Begin,
        uint8_t const* Data_End)
    {
        size_t Expected_Size = (Boot_Record_Sector_Count * k_Sector_Size);
        size_t Buffer_Size = (Data_End - Data_Begin);

        if (Buffer_Size < Expected_Size)
            return false;

        for(size_t i = 0; i < Boot_Record_Sector_Count; ++i)
            std::copy(
                Data_Begin + (i * k_Sector_Size),
                Data_Begin + ((i + 1) * k_Sector_Size),
                &Storage[i][0]);

        uint64_t Offset = NewFs::k_Bpb_Offset;
        _bpb.Write(Storage[0], Offset);

        return true;
    }

    uint64_t
    Fat_12::Get_Cluster_Value(uint16_t Index)
    {
        size_t const Fat_Offset = ((Index * 3) / 2);

        size_t const First_Sector_Number =
            (_bpb.First_Fat_Sector() + Fat_Offset / _bpb.Bytes_Per_Sector);

        size_t const First_Sector_Offset =
            (Fat_Offset % _bpb.Bytes_Per_Sector);

        size_t const Second_Sector_Number =
            (First_Sector_Offset < 511)?
                First_Sector_Number:
                (First_Sector_Number + 1);

        size_t const Second_Sector_Offset =
            (First_Sector_Offset < 511)?
                (First_Sector_Offset + 1):
                0;

        uint64_t const Low_Byte =
            Storage[First_Sector_Number][First_Sector_Offset];

        uint64_t const High_Byte =
            Storage[Second_Sector_Number][Second_Sector_Offset];

        return ((Low_Byte | (High_Byte << 8)) & k_End_Of_Chain_12);
    }

    bool
    Fat_12::Is_Cluster_Free(uint16_t Index)
    {
        return (Get_Cluster_Value(Index) == 0);
    }

    vector<uint16_t> 
    Fat_12::Find_Cluster_Chain(size_t File_Size)
    {
        vector<uint16_t> Result;
        uint64_t Total_Size_Found = 0;
        uint64_t End = uint64_t(_bpb.Total_Data_Cluster_Count() + 2);

        for(uint64_t i = 2; i < End; ++i)
        {
            if (Is_Cluster_Free(i))
            {
                Result.push_back(uint16_t(i));
                Total_Size_Found+= 512;
            }

            if (Total_Size_Found >= File_Size)
                return Result;
        }

        Result.clear();
        return Result;
    }

    void
    Fat_12::Write_Cluster(
        uint64_t Cluster_Index,
        uint64_t Cluster_Value,
        uint8_t const* Data_Begin,
        uint8_t const* Data_End)
    {
        size_t const Fat_Offset = ((Cluster_Index * 3) / 2);
        bool const Byte_Offset = bool(Cluster_Index & 1);

        size_t First_Sector_Number =
            (_bpb.First_Fat_Sector() + Fat_Offset / _bpb.Bytes_Per_Sector);

        size_t const First_Sector_Offset =
            (Fat_Offset % _bpb.Bytes_Per_Sector);

        size_t Second_Sector_Number =
            (First_Sector_Offset < 511)?
                First_Sector_Number:
                (First_Sector_Number + 1);

        size_t const Second_Sector_Offset =
            (First_Sector_Offset < 511)?
                (First_Sector_Offset + 1):
                0;

        uint8_t& First_Byte =
            Storage[First_Sector_Number][First_Sector_Offset];

        uint8_t& Second_Byte =
            Storage[Second_Sector_Number][Second_Sector_Offset];

        if (Byte_Offset)
        {
            Cluster_Value <<= 4;
            Cluster_Value |= (First_Byte & 0x0f);
        } else {
            Cluster_Value |= (uint64_t(Second_Byte & 0x0f) << 12);
        }

        size_t Temp_First_Sector_Number = First_Sector_Number;
        size_t Temp_Second_Sector_Number = Second_Sector_Number;
        for(size_t i = 0; i < _bpb.Total_Fat_Count; ++i)
        {
            Storage[Temp_First_Sector_Number][First_Sector_Offset] =
                uint8_t(Cluster_Value);

            Storage[Temp_Second_Sector_Number][Second_Sector_Offset] =
                uint8_t(Cluster_Value >> 8);

            Temp_First_Sector_Number+= _bpb.Sectors_Per_Fat;
            Temp_Second_Sector_Number+= _bpb.Sectors_Per_Fat;
        }

        First_Sector_Number =
            (_bpb.First_Data_Sector() +
                ((Cluster_Index - 2) * _bpb.Sectors_Per_Cluster));

        size_t Sector_Number = First_Sector_Number;
//        cout
//            << "\tWriting cluster " << Cluster_Index
//            << " at sectors [" << Sector_Number
//            << ", " << Sector_Number + _bpb.Sectors_Per_Cluster - 1
//            << "]" << endl;

        for(size_t i = 0; i < _bpb.Sectors_Per_Cluster; ++i)
        {
            std::copy(
                &Data_Begin[i * 512],
                &Data_Begin[(i + 1) * 512],
                &Storage[Sector_Number][0]);

            ++Sector_Number;
        }
    }

    File_Directory_Entry
    Fat_12::Write_Directory_Entry(
            string const& Full_File_Name,
            uint64_t First_Cluster,
            uint64_t File_Size)
    {
        Fat_12_Directory_Entry Entry;
        memset(&Entry, 0x20, 12); // Include Attibutes.
        memset(&Entry.Reserved_MBZ_1, 0x00, sizeof(Entry) - 12);

        size_t Last_Slash = Full_File_Name.rfind('/') + 1;
        size_t Last_Dot = Full_File_Name.rfind('.');

        if ((Last_Dot < Last_Slash) || (Last_Dot > Full_File_Name.size()))
            Last_Dot = Full_File_Name.size();

        string File_Name =
                Full_File_Name.substr(Last_Slash, (Last_Dot - Last_Slash));

        string Extension =
            (Last_Dot < Full_File_Name.size())?
                Full_File_Name.substr(Last_Dot):
                "";

        for(size_t Sector = _bpb.First_Root_Directory_Sector();
            Sector < (_bpb.First_Root_Directory_Sector() +
                _bpb.Total_Root_Directory_Sector_Count());
            ++Sector)
        {
            for(size_t Offset = 0;
                Offset < 512;
                Offset+= 32)
            {
                if (Storage[Sector][Offset] == 0)
                {
                    for (int i = 0; i < 8; ++i)
                    {
                        Entry.File_Name[i] =
                            (File_Name.size() > i)?
                                char(std::toupper(File_Name[i])):
                                ' ';
                    }

                    for (int i = 0; i < 3; ++i)
                    {
                        Entry.File_Name[8 + i] =
                            (Extension.size() > i)?
                                char(std::toupper(Extension[i])):
                                ' ';
                    }

                    Entry.Last_Modified_Date = 0x978f;  // 18:30:30
                    Entry.Last_Modified_Date = 0x6a4e;  // 2015-02-14

                    Entry.Start_Cluster = uint16_t(First_Cluster);
                    Entry.File_Size_In_Bytes = (File_Size);

                    memcpy(
                        &Storage[Sector][Offset],
                        &Entry,
                        sizeof(Entry));

                    return Entry;
                }
            }
        }

        return Entry;
    }

    File_Directory_Entry
    Fat_12::Write_File(
        string const& File_Name,
        uint8_t const* Data_Begin,
        uint8_t const* Data_End)
    {
        vector<uint16_t> Cluster_Chain =
            Find_Cluster_Chain(Data_End - Data_Begin);

        if (Cluster_Chain.size() == 0)
            return Fat_12_Directory_Entry();

        cout
            << "FAT chain for " << File_Name
            << " has " << Cluster_Chain.size()
            << " clusters!" << endl;

        size_t Next_Cluster;

        for (size_t i = 0; i < Cluster_Chain.size(); ++i)
        {
            Next_Cluster =
                (((i + 1) < Cluster_Chain.size())?
                    (Cluster_Chain[i + 1]):
                    (k_End_Of_Chain_12));

            Write_Cluster(
                Cluster_Chain[i],
                Next_Cluster,
                &Data_Begin[i * 512],
                &Data_Begin[(i + 1) * 512]);
        }

        return
            Write_Directory_Entry(
                File_Name,
                Cluster_Chain[0],
                (Data_End - Data_Begin));
    }
}
