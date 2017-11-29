#include <algorithm>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "Bpb.h"
#include "Fat_12.h"
#include "Octet.h"
#include "Sector.h"
#include "Storage_Device.h"


using std::cout;
using std::endl;
using std::ifstream;
using std::string;
using std::vector;


typedef char* PChar;


// Preset sector counts.
size_t const k_144_Floppy = 2880;

#define SHOWB(b) cout << #b << ": " << ((b)? "true.": "false.") << endl;

bool Seek(
        string const& File_Name,
        ifstream& Stream,
        size_t Offset,
        bool From_Start)
{
    auto Reference = (From_Start? Stream.beg:Stream.end);

    if (!Stream.seekg(Offset, Reference))
    {
        cout << "Cannot seek within file '" << File_Name << "'!" << endl;
        return false;
    }

    return true;
}

bool ReadFile(string const& File_Name, vector<uint8_t>& File_Contents)
{
    ifstream Stream(File_Name);

    if (!Stream)
    {
        cout << "Cannot read file '" << File_Name << "'!" << endl;
        return false;
    }

    if (!Seek(File_Name, Stream, 0, false))
        return false;

    size_t File_Size = Stream.tellg();

    if (!Seek(File_Name, Stream, 0, true))
        return false;

    File_Contents.resize(File_Size);

    return bool(Stream.read(PChar(&File_Contents[0]), File_Size));
}

bool
Write_File(
        NewFs::I_Storage_Device<uint8_t> const& Storage,
        string const& File_Name)
{
    std::ofstream Stream(File_Name.c_str());

    if (!Stream)
    {
        cout
            << "Cannot open file '" << File_Name
            << "' for writing!" << endl;

        return false;
    }

    for (int i = 0; i < Storage.Sector_Count(); ++i)
    {
        auto& Sector = Storage[i];

        Stream.write(PChar(&Sector[0]), Sector.Octet_Count());
        cout
            << "Writing sector " << i << ": "
            << Sector.Octet_Count() << " Bytes.\r";

        if (!Stream)
        {
            cout
                << "\nCannot write to file '"
                << File_Name << "'!!" << endl;
            return false;
        }
    }

    cout << endl;

    return true;
}

NewFs::Bpb_Dos_3_2 Create_Bpb()
{
    NewFs::Bpb_Dos_3_2 Bpb;

    // Bpb 2.0
    Bpb.Bytes_Per_Sector = 512;
    Bpb.Sectors_Per_Cluster = 1;
    Bpb.Reserved_Sectors = 1;
    Bpb.Total_Fat_Count = 2;
    Bpb.Root_Directory_Entries = 224;
    Bpb.Total_Sector_Count = 2880;
    Bpb.Medium_Descriptor = 0xf0;
    Bpb.Sectors_Per_Fat = 9;

    // Bpb 3.0
    Bpb.Sectors_Per_Track = 18;
    Bpb.Number_Of_Heads = 2;
    Bpb.Hidden_Sectors = 0;

    // Bpb 3.2
    Bpb.Total_Sectors = 2880;

    return Bpb;
}

int main(int argc, char* argv[])
{
    if (argc < 3)
    {
        cout
            << "Usage: "
                << argv[0]
                << " <boot record image> <disk image> [file [...]]"
            << endl;

        return -1;
    }

    string const Disk_Image_File_Name = argv[1];
    string const Boot_Record_Image_File_Name = argv[2];
    vector<string> Files(argc - 3);
    std::copy(&argv[3], &argv[argc], &Files[0]);
    
    typedef NewFs::Sector<512, uint8_t> Sector_Type;

    cout << "Creating storage object." << endl;
    NewFs::Storage_Device<Sector_Type> Storage(512, k_144_Floppy);

    cout << "Reading boot record contents." << endl;
    vector<uint8_t> Boot_Record;
    if (!ReadFile(Boot_Record_Image_File_Name, Boot_Record))
    {
        cout << "There was an error reading the boot record image." << endl;
        return -2;
    }

    cout
        << "Boot record is " << Boot_Record.size() << " bytes long."
        << endl;

    cout << "Creating File System." << std::endl;
    NewFs::Fat_12 Fat(Storage);
    Fat.Format(Create_Bpb());
    Fat.Write_Boot_Record(
        &Boot_Record[0],
        &Boot_Record[Boot_Record.size()]);

    for(size_t i = 0; i < Files.size(); ++i)
    {
        cout << "Reading file " << Files[i];

        vector<uint8_t> File_Contents;
        if (!ReadFile(Files[i], File_Contents))
        {
            cout
                << "There was an error reading file '" << Files[i]
                << "'." << endl;

            return -2;
        }

        cout
            << "\rAdding file '" << Files[i]
            << "' which is " << File_Contents.size()
            << " bytes long." << endl;

        Fat.Write_File(
            Files[i],
            &File_Contents[0],
            &File_Contents[File_Contents.size()]);
    }

    cout << "Writing Image File." << std::endl;
    Write_File(Storage, Disk_Image_File_Name);

    return 0;
}
