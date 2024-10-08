#ifndef PSEUDOFAT_H
#define PSEUDOFAT_H

#include <fstream>
#include <vector>
#include <string>
#include <cstdint>

const int32_t FAT_UNUSED = INT32_MAX - 1;
const int32_t FAT_FILE_END = INT32_MAX - 2;
const int32_t FAT_BAD_CLUSTER = INT32_MAX - 3;
const int32_t FAT_USED = INT32_MAX - 4; // Define FAT_USED for allocated clusters

// Structure for FAT description
struct Description
{
    char signature[9];          // Author's FS login, e.g., "novak"
    int32_t disk_size;          // Total size of VFS
    int32_t cluster_size;       // Size of each cluster
    int32_t cluster_count;      // Number of clusters
    int32_t fat_count;          // Number of items in each FAT table
    int32_t fat1_start_address; // Start address of FAT1
    int32_t fat2_start_address; // Start address of FAT2
    int32_t data_start_address; // Start address of data blocks (main directory)
};

// Structure for directory items
struct DirectoryItem
{
    char item_name[13];    // 8+3 + /0 C/C++ terminating string character
    bool isFile;           // Identification: TRUE for file, FALSE for directory
    int32_t size;          // Size of file, 0 for directories (will take one block)
    int32_t start_cluster; // Starting cluster of the item
};

// PseudoFAT class
class PseudoFAT
{
public:
    void initialize();                      // Initialize PseudoFAT with default values
    void load(std::ifstream &fsFile);       // Load the PseudoFAT from the file
    void save(const std::string &filename); // Save the PseudoFAT back to the file

    Description desc;                          // FAT description
    std::vector<int32_t> fatTable;             // FAT table for managing clusters
    std::vector<DirectoryItem> directoryItems; // Directory items
};

#endif // PSEUDOFAT_H
