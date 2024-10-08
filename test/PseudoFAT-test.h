#ifndef PSEUDOFAT_H
#define PSEUDOFAT_H

#include <iostream>
#include <vector>
#include <fstream>
#include <cstring>
#include <climits>

// FAT markers
extern const int32_t FAT_UNUSED;
extern const int32_t FAT_FILE_END;
extern const int32_t FAT_BAD_CLUSTER;

// Cluster and Disk sizes
extern const int32_t CLUSTER_SIZE;
extern const int32_t DISK_SIZE;

// Description structure
struct description
{
    char signature[9];
    int32_t disk_size;
    int32_t cluster_size;
    int32_t cluster_count;
    int32_t fat_count;
    int32_t fat1_start_address;
    int32_t fat2_start_address;
    int32_t data_start_address;
};

// Directory item structure
struct directory_item
{
    char item_name[13];
    bool isFile;
    int32_t size;
    int32_t start_cluster;
};

// PseudoFAT class
class PseudoFAT
{
public:
    PseudoFAT(const std::string &filename);

    // Format the disk (if it doesn't exist)
    void formatDisk();

    // Create a directory
    bool createDirectory(const std::string &dirname);

    // List all directories
    void listDirectories() const;

private:
    description desc;                          // File system description
    std::vector<int32_t> fat1;                 // First FAT table
    std::vector<int32_t> fat2;                 // Second FAT table (redundant)
    std::vector<directory_item> rootDirectory; // Root directory items

    std::string filename; // File to save the data
    void initializeFAT(); // Initialize FAT tables

    void saveToFile();   // Save the file system to the .dat file
    void loadFromFile(); // Load the file system from the .dat file
};

#endif
