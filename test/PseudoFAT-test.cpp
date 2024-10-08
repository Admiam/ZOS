#include "PseudoFAT-test.h"

// FAT markers definitions
const int32_t FAT_UNUSED = INT32_MAX - 1;
const int32_t FAT_FILE_END = INT32_MAX - 2;
const int32_t FAT_BAD_CLUSTER = INT32_MAX - 3;

const int32_t CLUSTER_SIZE = 4096;           // 4KB cluster size
const int32_t DISK_SIZE = 600 * 1024 * 1024; // 600MB disk size

// Constructor: load from file or format disk if it doesn't exist
PseudoFAT::PseudoFAT(const std::string &file) : filename(file)
{
    std::ifstream in(filename, std::ios::binary);
    if (!in)
    {
        std::cout << "File not found, formatting disk...\n";
        formatDisk();
    }
    else
    {
        loadFromFile();
    }
}

void PseudoFAT::formatDisk()
{
    // Initialize description
    std::strcpy(desc.signature, "novak");
    desc.disk_size = DISK_SIZE;
    desc.cluster_size = CLUSTER_SIZE;

    // Calculate the number of clusters and FAT entries
    desc.cluster_count = desc.disk_size / desc.cluster_size;
    desc.fat_count = desc.cluster_count;

    // Set starting addresses for FAT1, FAT2, and data block
    desc.fat1_start_address = sizeof(description);
    desc.fat2_start_address = desc.fat1_start_address + desc.fat_count * sizeof(int32_t);
    desc.data_start_address = desc.fat2_start_address + desc.fat_count * sizeof(int32_t);

    // Initialize FAT tables
    initializeFAT();

    // Create the blank .dat file and set its size to 600MB by writing zeros in chunks
    std::ofstream out(filename, std::ios::binary);

    if (!out.is_open())
    {
        std::cerr << "Error creating the file.\n";
        return;
    }

    // Set file to 600MB by writing empty clusters (zeros)
    const size_t chunk_size = 4096;          // Write in 4KB chunks
    std::vector<char> buffer(chunk_size, 0); // Buffer of zeros

    size_t total_written = 0;
    while (total_written < DISK_SIZE)
    {
        out.write(buffer.data(), chunk_size);
        total_written += chunk_size;
    }

    // Save the rest of the file system (description, FAT, etc.)
    saveToFile();

    out.close();
}

void PseudoFAT::initializeFAT()
{
    // Initialize FAT tables with the calculated number of clusters
    fat1.resize(desc.fat_count, FAT_UNUSED);
    fat2.resize(desc.fat_count, FAT_UNUSED); // Redundant FAT2

    // Example of marking some clusters as bad (optional)
    if (desc.fat_count > 5)
    {
        fat1[5] = FAT_BAD_CLUSTER;
        fat2[5] = FAT_BAD_CLUSTER;
    }
}

// Save the current state of the file system to the .dat file
void PseudoFAT::saveToFile()
{
    std::ofstream out(filename, std::ios::binary);

    // Write the description
    out.write(reinterpret_cast<const char *>(&desc), sizeof(desc));

    // Write FAT1
    out.write(reinterpret_cast<const char *>(fat1.data()), fat1.size() * sizeof(int32_t));

    // Write FAT2
    out.write(reinterpret_cast<const char *>(fat2.data()), fat2.size() * sizeof(int32_t));

    // Write root directory
    size_t dirCount = rootDirectory.size();
    out.write(reinterpret_cast<const char *>(&dirCount), sizeof(size_t));
    out.write(reinterpret_cast<const char *>(rootDirectory.data()), dirCount * sizeof(directory_item));

    out.close();
}

// Load the file system from the .dat file
void PseudoFAT::loadFromFile()
{
    std::ifstream in(filename, std::ios::binary);

    // Read the description
    in.read(reinterpret_cast<char *>(&desc), sizeof(desc));

    // Read FAT1
    fat1.resize(desc.fat_count);
    in.read(reinterpret_cast<char *>(fat1.data()), fat1.size() * sizeof(int32_t));

    // Read FAT2
    fat2.resize(desc.fat_count);
    in.read(reinterpret_cast<char *>(fat2.data()), fat2.size() * sizeof(int32_t));

    // Read root directory
    size_t dirCount;
    in.read(reinterpret_cast<char *>(&dirCount), sizeof(size_t));
    rootDirectory.resize(dirCount);
    in.read(reinterpret_cast<char *>(rootDirectory.data()), dirCount * sizeof(directory_item));

    in.close();
}

// Create a directory
bool PseudoFAT::createDirectory(const std::string &dirname)
{
    for (const auto &dir : rootDirectory)
    {
        if (strcmp(dir.item_name, dirname.c_str()) == 0)
        {
            std::cerr << "Directory already exists.\n";
            return false;
        }
    }

    directory_item newDir = {};
    strncpy(newDir.item_name, dirname.c_str(), sizeof(newDir.item_name) - 1);
    newDir.isFile = false;
    newDir.size = 0;
    newDir.start_cluster = -1; // In this simplified version, we don't allocate clusters yet

    rootDirectory.push_back(newDir);
    saveToFile();
    return true;
}

// List directories
void PseudoFAT::listDirectories() const
{
    if (rootDirectory.empty())
    {
        std::cout << "No directories found.\n";
        return;
    }

    for (const auto &dir : rootDirectory)
    {
        std::cout << dir.item_name << (dir.isFile ? " (file)" : " (directory)") << '\n';
    }
}
