#ifndef NEWFS_BPB_H__INCLUDED
#define NEWFS_BPB_H__INCLUDED


#include "I_Sector.h"


namespace NewFs
{
    uint64_t const k_Bpb_Offset = 0x0b;

    struct I_Bpb
    {
        virtual
        void
        Write(
            I_Sector<uint8_t>& Sector,
            uint64_t& Offset) const = 0;

        virtual ~I_Bpb() {}
    };

    struct Bpb_Dos_2_0: I_Bpb
    {
        uint16_t Bytes_Per_Sector;
        uint8_t  Sectors_Per_Cluster;
        uint16_t Reserved_Sectors;
        uint8_t  Total_Fat_Count;
        uint16_t Root_Directory_Entries;
        uint16_t Total_Sector_Count;
        uint8_t  Medium_Descriptor;
        uint16_t Sectors_Per_Fat;

        virtual
        void
        Write(
                I_Sector<uint8_t>& Sector,
                uint64_t& Offset) const
        {
            Write(Bytes_Per_Sector, Sector, Offset);
            Write(Sectors_Per_Cluster, Sector, Offset);
            Write(Reserved_Sectors, Sector, Offset);
            Write(Total_Fat_Count, Sector, Offset);
            Write(Root_Directory_Entries, Sector, Offset);
            Write(Total_Sector_Count, Sector, Offset);
            Write(Medium_Descriptor, Sector, Offset);
            Write(Sectors_Per_Fat, Sector, Offset);
        }

        virtual uint64_t First_Fat_Sector() const
        {
            return Reserved_Sectors;
        }

        virtual uint64_t Total_Fat_Sector_Count() const
        {
            return (Total_Fat_Count * Sectors_Per_Fat);
        }

        virtual uint64_t First_Root_Directory_Sector() const
        {
            return (First_Fat_Sector() + Total_Fat_Sector_Count());
        }

        virtual uint64_t Total_Root_Directory_Sector_Count() const
        {
            uint64_t const Rde = Root_Directory_Entries;
            uint64_t const Eps = (512 / 32);

            return ((Rde + Eps - 1) / Eps);
        }

        virtual uint64_t First_Data_Sector() const
        {
            return
                First_Root_Directory_Sector() +
                Total_Root_Directory_Sector_Count();
        }

        uint16_t
        Total_Data_Sector_Count() const
        {
            return (Total_Sector_Count - First_Data_Sector() + 1);
        }

        uint16_t
        Total_Data_Cluster_Count() const
        {
            uint64_t Spc = Sectors_Per_Cluster;
            uint64_t Tdsc = Total_Data_Sector_Count();

            return ((Spc == 1)? Tdsc: (((Tdsc + Spc - 1) / Spc) - 1));
        }

        void
        Write(
            uint8_t Value,
            I_Sector<uint8_t>& Destination,
            uint64_t& Offset) const
        {
            Destination[Offset++] = Value;
        }

        void
        Write(
            uint16_t Value,
            I_Sector<uint8_t>& Destination,
            uint64_t& Offset) const
        {
            Write(uint8_t(Value), Destination, Offset);
            Write(uint8_t(Value >> 8), Destination, Offset);
        }
    };

    struct Bpb_Dos_3_0_Base: Bpb_Dos_2_0
    {
        uint16_t Sectors_Per_Track;
        uint16_t Number_Of_Heads;

        using Bpb_Dos_2_0::Write;

        virtual
        void
        Write(
                I_Sector<uint8_t>& Sector,
                uint64_t& Offset) const
        {
            Bpb_Dos_2_0::Write(Sector, Offset);

            Write(Sectors_Per_Track, Sector, Offset);
            Write(Number_Of_Heads, Sector, Offset);
        }
    };

    struct Bpb_Dos_3_0: Bpb_Dos_3_0_Base
    {
        uint16_t Hidden_Sectors;

        using Bpb_Dos_2_0::Write;

        virtual
        void
        Write(
                I_Sector<uint8_t>& Sector,
                uint64_t& Offset) const
        {
            Bpb_Dos_3_0_Base::Write(Sector, Offset);

            Write(Hidden_Sectors, Sector, Offset);
        }
    };

    struct Bpb_Dos_3_2: Bpb_Dos_3_0
    {
        uint16_t Total_Sectors;

        using Bpb_Dos_2_0::Write;

        virtual
        void
        Write(
                I_Sector<uint8_t>& Sector,
                uint64_t& Offset) const
        {
            Bpb_Dos_3_0_Base::Write(Sector, Offset);

            Write(Total_Sectors, Sector, Offset);
        }
    };
}

#endif  // NEWFS_BPB_H__INCLUDED
