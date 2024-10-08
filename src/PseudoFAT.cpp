#include "PseudoFAT.h"
#include <iostream>
#include <cstring>


void PseudoFAT::initialize()
{
    std::strncpy(desc.signature, "mikaa", 8);
    desc.disk_size = 600 * 1024 * 1024; // Example: 600MB disk size
    desc.cluster_size = 1024;           // Example: 4KB cluster size
    desc.cluster_count = desc.disk_size / desc.cluster_size;
    desc.fat_count = desc.cluster_count;
    desc.fat1_start_address = sizeof(Description);
    desc.fat2_start_address = desc.fat1_start_address + desc.fat_count * sizeof(int32_t);
    desc.data_start_address = desc.fat2_start_address + desc.fat_count * sizeof(int32_t);

    fatTable.resize(desc.fat_count, FAT_UNUSED); // All clusters start as unused
}

void PseudoFAT::save(const std::string &filename)
{
    std::ofstream fsFile(filename, std::ios::binary | std::ios::trunc);
    if (!fsFile.is_open())
    {
        std::cerr << "Cannot open file for saving.\n";
        return;
    }

    fsFile.write(reinterpret_cast<const char *>(&desc), sizeof(Description));
    fsFile.write(reinterpret_cast<const char *>(fatTable.data()), desc.fat_count * sizeof(int32_t));

    size_t itemCount = directoryItems.size();
    fsFile.write(reinterpret_cast<const char *>(&itemCount), sizeof(size_t));
    fsFile.write(reinterpret_cast<const char *>(directoryItems.data()), itemCount * sizeof(DirectoryItem));

    fsFile.close();
}

void PseudoFAT::load(std::ifstream &fsFile)
{
    fsFile.read(reinterpret_cast<char *>(&desc), sizeof(Description));
    fatTable.resize(desc.fat_count);
    fsFile.read(reinterpret_cast<char *>(fatTable.data()), desc.fat_count * sizeof(int32_t));

    size_t itemCount;
    fsFile.read(reinterpret_cast<char *>(&itemCount), sizeof(size_t));
    directoryItems.resize(itemCount);
    fsFile.read(reinterpret_cast<char *>(directoryItems.data()), itemCount * sizeof(DirectoryItem));
}
