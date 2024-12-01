#ifndef PseudoFAT_H
#define PseudoFAT_H

#include <iostream>
#include <vector>
#include <fstream>
#include <cstring>
#include <climits>
#include <sstream>

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
    int32_t directory_start_address; // New field for directory metadata start
};

// Directory item structure
struct directory_item
{
    char item_name[12]; // 8 chars for name + 3 for extension + 1 for null terminator
    bool isFile;
    int32_t size;
    int32_t start_cluster;
    int32_t parent_id;
    int32_t id;
    std::vector<directory_item> children;

    // Constructor
    directory_item(const std::string &name = "", bool isFile = false)
        : isFile(isFile), size(0), start_cluster(-1), parent_id(-1), id(-1)
    {
        std::memset(item_name, 0, sizeof(item_name)); // Initialize all to null
        if (name.length() >= sizeof(item_name))
        {
            std::strncpy(item_name, name.c_str(), sizeof(item_name) - 1); // Copy and ensure null-termination
        }
        else
        {
            std::strcpy(item_name, name.c_str()); // Safe to use strcpy when the length is within bounds
        }
    }
};

// PseudoFAT class
class PseudoFAT
{
public:
    PseudoFAT(const std::string &filename);

    bool formatDisk(const std::string &sizeStr);
    bool createDirectory(const std::string &path);
    // void listDirectories(const std::string &path);
    void listDirectory(const std::string &filePath);
    bool changeDirectory(const std::string &path);
    directory_item *findDirectoryFromRoot(const std::vector<std::string> &pathParts);
    directory_item *findDirectory(const std::vector<std::string> &pathParts); // Add const
    std::vector<std::string> splitPath(const std::string &path) const;
    directory_item *getRootDirectory();
    directory_item *currentDirectory;
    directory_item *getCurrentDirectory();
    std::string getFullPath(directory_item *dir);
    bool rmdir(const std::string &path);
    std::string pwd();
    bool incp(const std::string &srcPath, const std::string &destPath);
    bool cat(const std::string &filePath);
    void info(const std::string &path);
    void outcp(const std::string &srcPath, const std::string &destPath);
    void rm(const std::string &filePath);
    void mv(const std::string &srcPath, const std::string &destPath);
    void cp(const std::string &srcPath, const std::string &destPath);
    bool load(const std::string &filePath);
    void bug(const std::string &targetFile);
    bool check();
    directory_item *locateDirectoryOrFile(const std::vector<std::string> &pathParts, directory_item *startDir);
    directory_item *locateParentDirectory(const std::vector<std::string> &pathParts);
    directory_item *locateDirectoryOrFileUsingParentIds(const std::vector<std::string> &pathParts, directory_item *startDir);

private: 
    description desc;
    std::vector<int32_t> fat1;
    std::vector<int32_t> fat2;
    std::vector<directory_item> rootDirectory;
    std::string filename;
    int32_t next_dir_id;

    void initializeFAT();
    void saveToFile();
    void loadFromFile();
    std::string trimItemName(const char *itemName);
    bool validateAndFormatName(const std::string &name, char *formattedName);
    directory_item *findDirectoryById(int32_t id, directory_item *dir);
    std::string removeSpaces(const std::string &input);
    void updateNextDirId();
    int countFreeClusters();
    int allocateCluster();
    void loadDirectory(directory_item &dir, std::ifstream &in);
    directory_item *findItem(const std::vector<std::string> &pathParts, bool fromRoot);
    std::string generateUniqueFileName(directory_item *targetDir, const std::string &originalName);
    void saveDirectory(std::ofstream &out, const directory_item &dir);
    void loadDirectory(std::ifstream &in, directory_item &dir);
    directory_item *findChildDirectory(directory_item *parent, const std::string &name);            // Helper function to find a child directory by name within a given directory
    void writeFileData(std::ofstream &outFile, const directory_item &file);
    bool validateDirectory(const directory_item &dir);
    directory_item *resolvePath(const std::vector<std::string> &pathParts, directory_item *startDir);
    directory_item *findParentRecursive(int32_t parentId, directory_item *currentDir);
    directory_item *findParent(directory_item *child);
    std::string trimWhitespace(const std::string &str);
    void listDirectoryByPath(const std::string &path);
    std::vector<directory_item> getAllFiles();
    void collectFilesRecursively(directory_item *dir, std::vector<directory_item> &files);
};

#endif // PSEUDOFAT_H