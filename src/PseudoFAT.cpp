#include "PseudoFAT.h"
#include <sstream> 
#include <algorithm>  

// FAT markers definitions
const int32_t FAT_UNUSED = INT32_MAX - 1;
const int32_t FAT_FILE_END = INT32_MAX - 2;
const int32_t FAT_BAD_CLUSTER = INT32_MAX - 3;

const int32_t CLUSTER_SIZE = 1024;   
 
PseudoFAT::PseudoFAT(const std::string &file) : filename(file), currentDirectory(nullptr), next_dir_id(0)
{
    std::ifstream in(filename, std::ios::binary);
    if (!in)
    {
        std::string command, arg;
        std::cout << "File not found, formatting disk...\n";
        bool notFormated = true;
        while (notFormated)
        {
            std::string prompt = " > ";
            std::cout << prompt;

            std::cin >> command;
            std::string arg; // Reset argument for each command

            if (command == "exit")
            {
                break;
            }else if (command == "format")
            {
                std::cin >> arg;
                formatDisk(arg);
                notFormated = false;
            }else{
                std::cout << "Please format the disk first\n";
            }
        }
    } else
    {
        loadFromFile();
        updateNextDirId(); // Ensure next_dir_id is set correctly after loading
    }
}

    // Format the disk
bool PseudoFAT::formatDisk(const std::string &sizeStr)
{
    // Parse size in MB from the format command (e.g., "600MB")
    int sizeMB = 0;
    try
    {
        sizeMB = std::stoi(sizeStr);
    }
    catch (...)
    {
        std::cerr << "Invalid size format. Please specify as 'format <size>MB'.\n";
        return false;
    }

    // Set DISK_SIZE based on the parsed MB size
    const int32_t DISK_SIZE = sizeMB * 1024 * 1024;

    // Initialize the description structure with formatted settings
    std::strcpy(desc.signature, "mikaa");
    desc.disk_size = DISK_SIZE;
    desc.cluster_size = CLUSTER_SIZE;

    // Calculate cluster count and FAT entries based on disk size and cluster size
    desc.cluster_count = desc.disk_size / desc.cluster_size;
    desc.fat_count = desc.cluster_count;

    // Set starting addresses for FAT1, FAT2, and data
    desc.fat1_start_address = sizeof(description);
    desc.fat2_start_address = desc.fat1_start_address + desc.fat_count * sizeof(int32_t);
    desc.data_start_address = desc.fat2_start_address + desc.fat_count * sizeof(int32_t);
    desc.directory_start_address = desc.data_start_address;

    // Initialize the FAT tables
    initializeFAT();

    // Create or overwrite the .dat file with the specified disk size
    std::ofstream out(filename, std::ios::binary | std::ios::trunc);

    if (!out.is_open())
    {
        std::cerr << "CANNOT CREATE FILE\n";
        return false;
    }

    // Set the file to the specified size by writing empty clusters
    const size_t chunk_size = 4096;          // Write in 4KB chunks
    std::vector<char> buffer(chunk_size, 0); // Buffer of zeros
    size_t total_written = 0;

    while (total_written < DISK_SIZE)
    {
        out.write(buffer.data(), std::min(chunk_size, DISK_SIZE - total_written));
        total_written += chunk_size;
    }
    out.close();

    // Clear and set up the root directory
    rootDirectory.clear();
    directory_item root;
    std::strcpy(root.item_name, "/");
    root.isFile = false;
    root.size = 0;
    root.start_cluster = -1;
    root.parent_id = -1;
    root.id = next_dir_id++;
    rootDirectory.push_back(root);
    currentDirectory = &rootDirectory[0];

    // Save formatted file system to .dat file
    saveToFile();

    std::cout << "OK\n";
    return true;
}

    void PseudoFAT::saveToFile()
    {
        std::ofstream outFile(filename, std::ios::binary | std::ios::in | std::ios::out);
        if (!outFile)
        {
            std::cerr << "Error opening file for saving.\n";
            return;
        }

        // 1. Write the file system description
        outFile.seekp(0);
        outFile.write(reinterpret_cast<const char *>(&desc), sizeof(desc));

        // 2. Write the updated FAT tables
        outFile.seekp(desc.fat1_start_address);
        outFile.write(reinterpret_cast<const char *>(fat1.data()), fat1.size() * sizeof(int32_t));
        outFile.seekp(desc.fat2_start_address);
        outFile.write(reinterpret_cast<const char *>(fat2.data()), fat2.size() * sizeof(int32_t));

        // 3. Write the directory entries at the `directory_start_address`
        outFile.seekp(desc.directory_start_address);
        for (const auto &dir : rootDirectory)
        {
            saveDirectory(outFile, dir);
        }

        outFile.close();
        std::cout << "File system saved.\n";
    }

    void PseudoFAT::saveDirectory(std::ofstream &outFile, const directory_item &dir)
    {
        // Write directory item fields
        outFile.write(reinterpret_cast<const char *>(&dir.item_name), sizeof(dir.item_name));
        outFile.write(reinterpret_cast<const char *>(&dir.isFile), sizeof(dir.isFile));
        outFile.write(reinterpret_cast<const char *>(&dir.size), sizeof(dir.size));
        outFile.write(reinterpret_cast<const char *>(&dir.start_cluster), sizeof(dir.start_cluster));
        outFile.write(reinterpret_cast<const char *>(&dir.parent_id), sizeof(dir.parent_id));
        outFile.write(reinterpret_cast<const char *>(&dir.id), sizeof(dir.id));

        // Write the children count
        size_t childrenCount = dir.children.size();
        outFile.write(reinterpret_cast<const char *>(&childrenCount), sizeof(size_t));

        // Recursively save each child directory
        for (const auto &child : dir.children)
        {
            saveDirectory(outFile, child);
        }
    }

    void PseudoFAT::loadFromFile()
    {
        std::ifstream in(filename, std::ios::binary);
        if (!in)
        {
            std::cerr << "Error opening file for loading.\n";
            return;
        }

        // Read the file system description (metadata)
        in.read(reinterpret_cast<char *>(&desc), sizeof(desc));

        // Read FAT tables
        fat1.resize(desc.fat_count);
        in.seekg(desc.fat1_start_address);
        in.read(reinterpret_cast<char *>(fat1.data()), fat1.size() * sizeof(int32_t));
        fat2.resize(desc.fat_count);
        in.seekg(desc.fat2_start_address);
        in.read(reinterpret_cast<char *>(fat2.data()), fat2.size() * sizeof(int32_t));

        // Position the stream to the start of directory data
        in.seekg(desc.directory_start_address);

        // Clear current root directory
        rootDirectory.clear();

        // Load directories only within the directory data region
        size_t readCount = 0; // To prevent infinite loops
        while (in && readCount < 1000)
        { // Arbitrary max count to prevent overreading
            directory_item dir;
            loadDirectory(in, dir);

            // Stop if we detect an invalid directory entry
            if (!validateDirectory(dir))
            {
                std::cerr << "Invalid directory entry detected, stopping load.\n";
                break;
            }

            rootDirectory.push_back(dir);
            readCount++;
        }

        // Set root directory as current
        currentDirectory = &rootDirectory[0];
        in.close();
        std::cout << "Load complete\n";
    }

    void PseudoFAT::loadDirectory(std::ifstream &in, directory_item &dir)
    {
        in.read(reinterpret_cast<char *>(&dir.item_name), sizeof(dir.item_name));
        in.read(reinterpret_cast<char *>(&dir.isFile), sizeof(dir.isFile));
        in.read(reinterpret_cast<char *>(&dir.size), sizeof(dir.size));
        in.read(reinterpret_cast<char *>(&dir.start_cluster), sizeof(dir.start_cluster));
        in.read(reinterpret_cast<char *>(&dir.parent_id), sizeof(dir.parent_id));
        in.read(reinterpret_cast<char *>(&dir.id), sizeof(dir.id));

        // Check if this is a valid directory name; if not, stop loading
        if (std::strlen(dir.item_name) == 0 || std::all_of(std::begin(dir.item_name), std::end(dir.item_name), [](char c)
                                                           { return c == ' '; }))
        {
            std::cerr << "Invalid or empty directory name found.\n";
            return;
        }

        // Load children count
        size_t childrenCount;
        in.read(reinterpret_cast<char *>(&childrenCount), sizeof(size_t));

        // Validate children count
        if (childrenCount > 100)
        { // Arbitrary reasonable maximum
            std::cerr << "Unreasonable children count (" << childrenCount << ") in directory: " << dir.item_name << "\n";
            return;
        }

        dir.children.resize(childrenCount);

        // Load each child directory recursively
        for (auto &child : dir.children)
        {
            loadDirectory(in, child);
        }

        // Debug output
        std::cout << "Loaded directory: " << dir.item_name
                  << ", ID: " << dir.id << ", Parent ID: " << dir.parent_id
                  << ", Children Count: " << childrenCount << std::endl;
    }

    bool PseudoFAT::validateDirectory(const directory_item &dir)
    {
        // Basic validation for directory entries to avoid loading unintended data
        if (std::strlen(dir.item_name) == 0)
        {
            return false;
        }
        if (dir.id < 0 || dir.parent_id < -1)
        {
            return false;
        }
        return true;
    }

    bool PseudoFAT::createDirectory(const std::string &path)
    {

        std::vector<std::string> pathParts = splitPath(path);
        if (pathParts.empty())
        {
            std::cerr << "INVALID PATH\n";
            return false;
        }


        std::string newDirName = pathParts.back();
        pathParts.pop_back(); // Remove the new directory name from the path


        directory_item *parentDir = nullptr;

        if (path[0] == '/') // Absolute path
        {

            parentDir = findDirectoryFromRoot(pathParts);
        }
        else // Relative path
        {

            parentDir = findDirectory(pathParts);
        }

        if (!parentDir)
        {
            std::cerr << "PATH NOT FOUND\n";
            return false;
        }


        char formattedName[12];
        if (!validateAndFormatName(newDirName, formattedName))
        {
            std::cerr << "INVALID NAME\n";
            return false;
        }


        for (const auto &dir : parentDir->children)
        {

            if (std::strcmp(dir.item_name, formattedName) == 0)
            {
                std::cerr << "EXIST\n";
                return false;
            }
        }

        directory_item newDir(path, false);
        std::strcpy(newDir.item_name, formattedName);
        newDir.isFile = false;
        newDir.size = 0;
        newDir.start_cluster = -1;
        newDir.parent_id = parentDir->id;

        std::cout << "next_dir_id: " << next_dir_id << "\n";

        newDir.id = next_dir_id++;


        parentDir->children.push_back(newDir);

        std::cout << "Creating directory: " << newDir.item_name << "\n";
        std::cout << "parentDir->id: " << newDir.parent_id << "\n";
        std::cout << "newDir->id: " << newDir.id << "\n";
        std::cout << "OK\n";

        saveToFile();
        return true;
    }

    void PseudoFAT::listDirectory(const std::string &filePath)
    {
        directory_item *dir = currentDirectory; // Start from the current directory

        if(!filePath.empty())
        {
            std::vector<std::string> pathParts = splitPath(filePath);
            dir = (filePath[0] == '/')
                                      ? locateDirectoryOrFile(pathParts, &rootDirectory[0]) // Absolute path
                                      : locateDirectoryOrFile(pathParts, currentDirectory); // Relative path
            if (dir == nullptr)
            {
                std::cerr << "Directory not found.\n";
                return;
            }
            // If the target is a file, print error and exit
            if (dir->isFile)
            {
                std::cerr << "Error: " << trimItemName(dir->item_name) << " is a file, not a directory.\n";
                return;
            }
        }

        if (dir == nullptr || dir->children.empty())
        {
            std::cout << "Directory is empty.\n";
            return;
        }


        // Iterate over the children of the directory
        for (const auto &child : dir->children)
        {

            std::cout << (child.isFile ? "FILE: " : "DIR: ") << child.item_name << '\n'; // Correctly output the name
        }
    }

    std::string PseudoFAT::trimItemName(const char *itemName)
    {
        std::string name(itemName);

        // Trim trailing spaces
        name.erase(std::find_if(name.rbegin(), name.rend(), [](unsigned char ch)
                                { return !std::isspace(ch); })
                       .base(),
                   name.end());

        return name;
    }

    directory_item *PseudoFAT::findDirectory(const std::vector<std::string> &pathParts)
    {
        directory_item *current = currentDirectory; // Start from the current directory

        for (const std::string &part : pathParts)
        {
            bool found = false;

            // Traverse the children to find the next directory in the path
            for (auto &child : current->children)
            {
                // Use std::strcmp to compare the directory names, but first trim item_name
                std::string trimmedItemName = trimItemName(child.item_name);

                // std::cout << "Comparing: '" << trimmedItemName << "' with '" << part << "'\n";

                if (!child.isFile && trimmedItemName == part)
                {
                    current = &child;
                    found = true;
                    break;
                }
            }

            if (!found)
            {
                return nullptr; // Directory not found
            }
        }

        return current;
    }

    directory_item *PseudoFAT::findDirectoryFromRoot(const std::vector<std::string> &pathParts)
    {
        directory_item *current = &rootDirectory[0]; // Start from the root directory

        for (const std::string &part : pathParts)
        {
            bool found = false;

            for (auto &child : current->children)
            {
                if (!child.isFile && child.item_name == part)
                {
                    current = &child;
                    found = true;
                    break;
                }
            }

            if (!found)
            {
                return nullptr; // Directory not found
            }
        }

        return current;
    }

    std::vector<std::string> PseudoFAT::splitPath(const std::string &path) const
    {
        std::vector<std::string> result;
        std::istringstream stream(path);
        std::string token;
    
        while (std::getline(stream, token, '/'))
        {
            if (!token.empty() && token != ".")
            {
                result.push_back(token);
            }
        }
        return result;
    }

    directory_item *PseudoFAT::getRootDirectory()
    {
        return &rootDirectory[0]; // Assuming the root is stored in rootDirectory[0]
    }

    void PseudoFAT::initializeFAT()
    {
        // Initialize FAT tables with the calculated number of clusters
        fat1.resize(desc.fat_count, FAT_UNUSED);
        fat2.resize(desc.fat_count, FAT_UNUSED); // Redundant FAT2
    }

    bool PseudoFAT::validateAndFormatName(const std::string &name, char *formattedName)
    {
        if (name.empty() || name.length() > 11)
            return false;
    
        size_t dotPos = name.find('.');
        std::string baseName = (dotPos == std::string::npos) ? name : name.substr(0, dotPos);
        std::string extension = (dotPos == std::string::npos) ? "" : name.substr(dotPos + 1);
    
        if (baseName.length() > 8 || extension.length() > 3)
            return false;
    
        std::memset(formattedName, ' ', 11);
        formattedName[11] = '\0';
    
        std::copy(baseName.begin(), baseName.end(), formattedName);
        if (!extension.empty())
        {
            std::copy(extension.begin(), extension.end(), formattedName + 8);
        }
    
        return true;
    }

    bool PseudoFAT::changeDirectory(const std::string &path)
    {
        // Case 1: Empty path means return to the root directory
        if (path.empty() || path == "/")
        {
            currentDirectory = &rootDirectory[0]; // Reset to root
            std::cout << "OK - Changed to root directory\n";
            return true;
        }

        // Case 2: Handle "cd .." (move to parent directory)
        std::cout << "Current directory: " << currentDirectory->item_name << ", ID: " << currentDirectory->id
                  << ", Parent ID: " << currentDirectory->parent_id << std::endl;

        if (path == "..")
        {
            if (currentDirectory->parent_id == -1)
            {
                std::cerr << "Already at the root directory.\n";
                return false;
            }

            // Find the parent directory based on parent_id
            directory_item *parentDir = findDirectoryById(currentDirectory->parent_id, &rootDirectory[0]);

            if (parentDir)
            {
                currentDirectory = parentDir; // Move to the parent directory
                std::cout << "Moved to parent: " << currentDirectory->item_name << '\n';
                return true;
            }
            else
            {
                std::cerr << "Parent directory not found.\n";
                return false;
            }
        }

        // Case 3: Handle absolute and relative paths
        std::vector<std::string> pathParts = splitPath(path);
        directory_item *targetDir = nullptr;

        if (path[0] == '/') // Absolute path
        {
            targetDir = findDirectoryFromRoot(pathParts);
        }
        else // Relative path
        {
            // std::cout << "pathParts: " << pathParts[0] << '\n';
            targetDir = findDirectory(pathParts);
        }

        if (targetDir != nullptr && !targetDir->isFile)
        {
            currentDirectory = targetDir;
            std::cout << "OK - Changed directory to: " << targetDir->item_name << '\n';
            return true;
        }
        else
        {
            std::cerr << "Directory not found or is a file.\n";
            return false;
        }
    }

    directory_item *PseudoFAT::findDirectoryById(int32_t id, directory_item *dir)
    {
        if (dir->id == id)
        {
            return dir; // Found the directory
        }

        // Search in the children recursively
        for (auto &child : dir->children)
        {
            directory_item *foundDir = findDirectoryById(id, &child);
            if (foundDir)
            {
                return foundDir;
            }
        }

        return nullptr; // Directory not found
    }

    directory_item *PseudoFAT::getCurrentDirectory()
    {
        return currentDirectory; // Return the current directory pointer
    }

    std::string PseudoFAT::getFullPath(directory_item* dir)
{
    if (dir->parent_id == -1) {
        return ""; // Root directory
    }

    std::vector<std::string> pathParts;
    directory_item* current = dir;

    while (current->parent_id != -1) {
        pathParts.push_back(current->item_name);
        current = findDirectoryById(current->parent_id, &rootDirectory[0]);
    }

    // Reverse the path since we're building it from child to parent
    std::reverse(pathParts.begin(), pathParts.end());

    std::string fullPath;
    for (const auto& part : pathParts) {
        fullPath += part + "/";
    }

    // Remove trailing slash for aesthetic
    if (!fullPath.empty()) {
        fullPath.pop_back();
    }

    return removeSpaces(fullPath);
}

    std::string PseudoFAT::removeSpaces(const std::string &input)
{
    std::string result = input;
    result.erase(std::remove(result.begin(), result.end(), ' '), result.end());
    return result;
}

    void PseudoFAT::updateNextDirId()
{
    next_dir_id = 0; // Start at 0 (will be updated)

    // Helper lambda function to recursively find the highest ID
    std::function<void(const directory_item &)> findMaxId = [&](const directory_item &dir)
    {
        if (dir.id > next_dir_id)
        {
            next_dir_id = dir.id;
        }

        // Recursively check all children
        for (const auto &child : dir.children)
        {
            findMaxId(child);
        }
    };

    // Start with the root directory
    for (const auto &dir : rootDirectory)
    {
        findMaxId(dir);
    }

    // Increment next_dir_id to ensure the next created directory gets a unique ID
    next_dir_id++;
}

    bool PseudoFAT::rmdir(const std::string &path)
{
    // Step 1: Find the target directory
    directory_item *targetDir = nullptr;
    std::vector<std::string> pathParts = splitPath(path);

    // Handle absolute or relative paths
    if (path[0] == '/')
    { // Absolute path
        targetDir = findDirectoryFromRoot(pathParts);
    }
    else
    { // Relative path
        targetDir = findDirectory(pathParts);
    }

    if (!targetDir)
    {
        std::cerr << "FILE NOT FOUND\n";
        return false;
    }

    // Step 2: Check if the directory is empty
    if (!targetDir->children.empty())
    {
        std::cerr << "NOT EMPTY\n";
        return false;
    }

    // Step 3: Find the parent directory
    directory_item *parentDir = findDirectoryById(targetDir->parent_id, &rootDirectory[0]);

    if (!parentDir)
    {
        std::cerr << "Parent directory not found.\n";
        return false;
    }

    // Step 4: Remove the directory from the parent's children vector
    auto it = std::remove_if(parentDir->children.begin(), parentDir->children.end(),
                             [targetDir](const directory_item &child)
                             { return child.id == targetDir->id; });
    if (it != parentDir->children.end())
    {
        parentDir->children.erase(it, parentDir->children.end());
        std::cout << "OK\n";
    }
    else
    {
        std::cerr << "FILE NOT FOUND\n";
        return false;
    }

    // Step 5: Update the .dat file
    saveToFile();

    return true;
}

    std::string PseudoFAT::pwd()
{
    // Start from the current directory and trace back to the root
    directory_item *current = currentDirectory;
    std::vector<std::string> pathParts;

    while (current->parent_id != -1)
    {
        pathParts.push_back(removeSpaces(current->item_name));              // Add current directory name after trimming spaces
        current = findDirectoryById(current->parent_id, &rootDirectory[0]); // Go to parent directory
    }

    // Handle root directory case
    if (pathParts.empty())
    {
        return "/"; // Root directory
    }

    // Build the full path from root to the current directory
    std::string fullPath;
    for (auto it = pathParts.rbegin(); it != pathParts.rend(); ++it)
    {
        fullPath += "/" + *it;
    }

    return fullPath;
}

bool PseudoFAT::incp(const std::string &srcPath, const std::string &destPath)
{
    // Step 1: Check if the source file exists on the PC
    std::ifstream srcFile(srcPath, std::ios::binary | std::ios::ate);
    if (!srcFile)
    {
        std::cerr << "FILE NOT FOUND\n";
        return false;
    }

    // Step 2: Get the size of the source file
    std::streamsize srcFileSize = srcFile.tellg();
    srcFile.seekg(0, std::ios::beg); // Reset the file pointer to the start

    // Step 3: Split destination path and check if it ends with a slash
    std::vector<std::string> pathParts = splitPath(destPath);
    directory_item *targetDir = nullptr;
    std::string newFileName;

    bool isDirectoryPath = destPath.back() == '/';
    if (isDirectoryPath)
    {
        // If the path ends with '/', treat the last component as a directory
        targetDir = destPath[0] == '/' ? findDirectoryFromRoot(pathParts) : findDirectory(pathParts);
        if (!targetDir || targetDir->isFile)
        {
            std::cerr << "PATH NOT FOUND\n";
            return false;
        }
        // Use the original source file name in this case
        newFileName = srcPath.substr(srcPath.find_last_of("/\\") + 1);
    }
    else
    {
        // If no trailing '/', check each part in the path except the last as a directory
        targetDir = destPath[0] == '/' ? &rootDirectory[0] : currentDirectory;
        for (size_t i = 0; i < pathParts.size() - 1; ++i)
        {
            targetDir = findChildDirectory(targetDir, pathParts[i]);
            if (!targetDir)
            {
                std::cerr << "PATH NOT FOUND\n";
                return false;
            }
        }
        newFileName = pathParts.back(); // Last part is the filename
    }

    // Step 4: Check for duplicates and modify name if necessary
    std::string originalName = newFileName;
    while (std::any_of(targetDir->children.begin(), targetDir->children.end(),
                       [&](const directory_item &child)
                       {
                           // Trim trailing spaces from child.item_name before comparison
                           std::string trimmedChildName = trimItemName(child.item_name);
                           return trimmedChildName == newFileName;
                       }))
    {
        // Append a unique counter to the name if a duplicate is found
        // newFileName = originalName + "(" + std::to_string(counter++) + ")";
        std::cerr << "SAME NAME\n";
        return false;
    }

    // Step 5: Ensure enough space in the file system and proceed with file saving as before
    int requiredClusters = (srcFileSize + CLUSTER_SIZE - 1) / CLUSTER_SIZE;
    int availableClusters = countFreeClusters();
    if (requiredClusters > availableClusters)
    {
        std::cerr << "NOT ENOUGH SPACE\n";
        return false;
    }

    // Step 6: Allocate clusters and write file data to allocated clusters
    std::vector<int> allocatedClusters;
    for (int i = 0; i < requiredClusters; ++i)
    {
        int freeCluster = allocateCluster();
        std::cout << "Allocated cluster: " << freeCluster << '\n';
        if (freeCluster == -1)
        {
            std::cerr << "NOT ENOUGH SPACE\n";
            return false;
        }
        allocatedClusters.push_back(freeCluster);
    }

    std::ofstream outFile(filename, std::ios::binary | std::ios::in | std::ios::out);
    for (int i = 0; i < requiredClusters; ++i)
    {
        char buffer[CLUSTER_SIZE] = {0};
        srcFile.read(buffer, CLUSTER_SIZE);
        outFile.seekp(desc.data_start_address + allocatedClusters[i] * CLUSTER_SIZE);
        outFile.write(buffer, CLUSTER_SIZE);

        std::cout << "Writing to cluster: " << allocatedClusters[i] << '\n';
        
        if (i < requiredClusters - 1)
        {
            fat1[allocatedClusters[i]] = allocatedClusters[i + 1];
        }
        else
        {
            fat1[allocatedClusters[i]] = FAT_FILE_END;
        }
    }

    // Step 7: Add file to directory structure with the final unique name
    directory_item newFile;
    std::strcpy(newFile.item_name, newFileName.c_str());
    newFile.isFile = true;
    newFile.size = srcFileSize;
    newFile.start_cluster = allocatedClusters[0];
    newFile.id = next_dir_id++;

    targetDir->children.push_back(newFile);

    // Step 8: Save updated file system back to the .dat file
    saveToFile();

    std::cout << "OK\n";
    return true;
}

directory_item *PseudoFAT::findChildDirectory(directory_item *parent, const std::string &name)
{
    // std::cout << "Searching for: " << name << " in directory: " << parent->item_name << '\n';
    if (!parent)
        return nullptr;

    for (auto &child : parent->children)
    {
        std::string trimmedItemName = trimItemName(child.item_name); // Trim any extra spaces in item_name
        // std::cout << "Comparing: '" << trimmedItemName << "' with '" << name << "'\n";

        if (!child.isFile && trimmedItemName == name)
        {
            // std::cout << "Found: " << trimmedItemName << '\n';
            return &child;
        }
    }
    return nullptr;
}

int PseudoFAT::countFreeClusters()
{
    int freeCount = 0;
    for (const auto &entry : fat1)
    {
        if (entry == FAT_UNUSED)
        {
            freeCount++;
        }
    }
    return freeCount;
}

int PseudoFAT::allocateCluster()
{
    for (size_t i = 1; i < fat1.size(); ++i)
    {
        if (fat1[i] == FAT_UNUSED)
        {
            fat1[i] = FAT_FILE_END; // Mark as used
            return i;               // Return the allocated cluster index
        }
    }
    return -1; // No free cluster found
}

bool PseudoFAT::cat(const std::string &filePath)
{
    std::vector<std::string> pathParts = splitPath(filePath);

    directory_item *targetFile = (filePath[0] == '/')
                                     ? locateDirectoryOrFile(pathParts, &rootDirectory[0]) // Absolute path
                                     : locateDirectoryOrFile(pathParts, currentDirectory); // Relative path

    if (!targetFile || !targetFile->isFile)
    {
        std::cerr << "FILE NOT FOUND\n";
        return false;
    }

    // std::cout << "Opening file '" << targetFile->item_name << "' for reading.\n";
    // std::cout << "File size: " << targetFile->size << ", Start cluster: " << targetFile->start_cluster << '\n';

    std::ifstream inFile(filename, std::ios::binary);
    if (!inFile)
    {
        std::cerr << "Error opening file system .dat file.\n";
        return false;
    }

    int cluster = targetFile->start_cluster;
    std::vector<char> fileData(targetFile->size);
    size_t bytesRead = 0;

    while (cluster != FAT_FILE_END && bytesRead < targetFile->size)
    {
        // Calculate the position in the file for the current cluster
        inFile.seekg(desc.data_start_address + cluster * CLUSTER_SIZE);

        // Determine the number of bytes to read from this cluster
        size_t bytesToRead = std::min(static_cast<size_t>(CLUSTER_SIZE), targetFile->size - bytesRead);

        // Read the data from the current cluster into the buffer
        inFile.read(fileData.data() + bytesRead, bytesToRead);

        bytesRead += bytesToRead;

        // Check the FAT to find the next cluster
        int nextCluster = fat1[cluster];
        cluster = nextCluster;
    }

    // Close the file after reading all clusters
    inFile.close();

    // Print the entire contents read from the file
    std::cout.write(fileData.data(), targetFile->size);
    std::cout << std::endl;


    return true;
}

directory_item *PseudoFAT::findItem(const std::vector<std::string> &pathParts, bool fromRoot)
{
    // Determine the starting point: root directory or current directory
    directory_item *current = fromRoot ? &rootDirectory[0] : currentDirectory;

    for (const std::string &part : pathParts)
    {
        bool found = false;
        std::cout << "Searching for: " << part << " in directory: " << current->item_name << '\n';

        // Traverse the children to find the next item (file or directory) in the path
        for (auto &child : current->children)
        {
            std::string trimmedItemName = trimItemName(child.item_name); // Trim any extra spaces in item_name
            std::cout << "Comparing with: '" << trimmedItemName << "'\n";

            if (std::strcmp(trimmedItemName.c_str(), part.c_str()) == 0)
            {
                current = &child;
                found = true;
                break;
            }
        }

        if (!found)
        {
            return nullptr; // Item not found
        }
    }

    return current;
}

void PseudoFAT::info(const std::string &path)
{
    // Split the path and determine if it's absolute or relative
    std::vector<std::string> pathParts = splitPath(path);
    directory_item *target = nullptr;

    if (path[0] == '/') // Absolute path
    {
        target = locateDirectoryOrFile(pathParts, &rootDirectory[0]);
    }
    else // Relative path
    {
        target = locateDirectoryOrFile(pathParts, currentDirectory);
    }

    if (!target)
    {
        std::cout << "FILE NOT FOUND\n";
        return;
    }

    // Retrieve the cluster chain if it's a valid file or directory
    int cluster = target->start_cluster;
    std::vector<int> clusters;

    while (cluster != FAT_FILE_END && cluster >= 0 && cluster < fat1.size())
    {
        clusters.push_back(cluster);
        cluster = fat1[cluster];
    }

    // Output the result
    std::cout << target->item_name << " ";
    for (size_t i = 0; i < clusters.size(); ++i)
    {
        std::cout << clusters[i];
        if (i < clusters.size() - 1)
            std::cout << ",";
    }
    std::cout << "\n";
}

// Enhanced helper function to locate file or directory, handling `..` and `.`
directory_item *PseudoFAT::locateDirectoryOrFile(const std::vector<std::string> &pathParts, directory_item *startDir)
{
    directory_item *current = startDir;

    for (const auto &part : pathParts)
    {
        if (part == "..") // Move up to parent directory if possible
        {
            if (current->parent_id != -1) // Ensure we're not already at root
            {
                current = findParent(current);
            }
        }
        else if (part != ".") // Ignore current directory references (.)
        {
            bool found = false;
            for (auto &child : current->children)
            {
                if (std::string(child.item_name).find(part) == 0) // Match directory name
                {
                    current = &child;
                    found = true;
                    break;
                }
            }
            if (!found)
                return nullptr;
        }
    }
    return current;
}

// Finds the parent directory of a given directory item by matching its `parent_id`
directory_item *PseudoFAT::findParent(directory_item *child)
{
    return findParentRecursive(child->parent_id, &rootDirectory[0]);
}

// Helper recursive function to find the directory with a specific ID
directory_item *PseudoFAT::findParentRecursive(int32_t parentId, directory_item *currentDir)
{
    // Check if the current directory's ID matches the specified parent ID
    if (currentDir->id == parentId)
    {
        std::cout << "Found parent directory: " << trimItemName(currentDir->item_name)
                  << " (ID: " << currentDir->id << ")\n";
        return currentDir;
    }

    // Recursively search through each child directory of the current directory
    for (auto &subdir : currentDir->children)
    {
        if (!subdir.isFile) // Only continue searching in directories
        {
            directory_item *parent = findParentRecursive(parentId, &subdir);
            if (parent != nullptr) // Found the matching directory
            {
                return parent;
            }
        }
    }

    // If no match is found in this branch, return nullptr
    return nullptr;
}

void PseudoFAT::outcp(const std::string &srcPath, const std::string &destPath)
{
    // Split the path and determine if it's absolute or relative
    std::vector<std::string> srcPathParts = splitPath(srcPath);
    directory_item *srcFile = nullptr;

    if (srcPath[0] == '/') // Absolute path
    {
        srcFile = locateDirectoryOrFile(srcPathParts, &rootDirectory[0]);
    }
    else // Relative path
    {
        srcFile = locateDirectoryOrFile(srcPathParts, currentDirectory);
    }

    if (!srcFile || !srcFile->isFile) // Ensure it is a file, not a directory
    {
        std::cout << "FILE NOT FOUND\n";
        return;
    }

    // Open the destination file on the host file system
    std::ofstream outFile(destPath, std::ios::binary);
    if (!outFile)
    {
        std::cout << "PATH NOT FOUND\n";
        return;
    }

    // Retrieve the cluster chain and write data to the output file, up to the file's actual size
    int cluster = srcFile->start_cluster;
    size_t bytesRemaining = srcFile->size; // Track how many bytes to write based on actual file size

    while (cluster != FAT_FILE_END && bytesRemaining > 0)
    {
        // Determine the number of bytes to read for this cluster (either the whole cluster or remaining bytes)
        size_t bytesToRead = std::min(static_cast<size_t>(CLUSTER_SIZE), bytesRemaining);

        char buffer[CLUSTER_SIZE] = {0}; // Initialize buffer

        // Open the .dat file and read the current cluster's data
        std::ifstream in(filename, std::ios::binary);
        in.seekg(desc.data_start_address + cluster * CLUSTER_SIZE);
        in.read(buffer, bytesToRead);
        in.close();

        // Write only the necessary bytes to the destination file
        outFile.write(buffer, bytesToRead);

        // Update the bytes remaining and move to the next cluster
        bytesRemaining -= bytesToRead;
        cluster = fat1[cluster];
    }

    outFile.close();
    std::cout << "OK\n";
}

void PseudoFAT::rm(const std::string &filePath)
{
    // Split the path and determine if it's absolute or relative
    std::vector<std::string> pathParts = splitPath(filePath);
    if (pathParts.empty())
    {
        std::cout << "FILE NOT FOUND\n";
        return;
    }

    // Determine the path to the parent directory
    std::vector<std::string> parentPathParts(pathParts.begin(), pathParts.end() - 1);
    directory_item *parentDir = nullptr;

    if (filePath[0] == '/') // Absolute path
    {
        parentDir = locateDirectoryOrFile(parentPathParts, &rootDirectory[0]);
    }
    else // Relative path
    {
        parentDir = locateDirectoryOrFile(parentPathParts, currentDirectory);
    }

    if (!parentDir) // If parent directory not found
    {
        std::cout << "FILE NOT FOUND\n";
        return;
    }

    // Locate the file within the parent directory
    directory_item *targetFile = nullptr;
    std::string fileName = pathParts.back();

    for (auto &child : parentDir->children)
    {
        if (std::string(child.item_name) == fileName && child.isFile) // Match file name and check it's a file
        {
            targetFile = &child;
            break;
        }
    }

    if (!targetFile) // If file not found in parent directory
    {
        std::cout << "FILE NOT FOUND\n";
        return;
    }

    // Release clusters used by the file in the FAT
    int cluster = targetFile->start_cluster;
    while (cluster != FAT_FILE_END && cluster >= 0 && cluster < fat1.size())
    {
        int nextCluster = fat1[cluster];
        fat1[cluster] = FAT_UNUSED; // Mark the cluster as unused
        cluster = nextCluster;
    }

    // Remove the file entry from the parent directory
    auto it = std::remove_if(parentDir->children.begin(), parentDir->children.end(),
                             [&](const directory_item &child)
                             {
                                 return &child == targetFile;
                             });
    parentDir->children.erase(it, parentDir->children.end());

    // Save the updated file system to persist changes
    saveToFile();
    std::cout << "OK\n";
}

void PseudoFAT::mv(const std::string &srcPath, const std::string &destPath)
{
    // Split the source path and locate the source item
    std::vector<std::string> srcPathParts = splitPath(srcPath);
    if (srcPathParts.empty())
    {
        std::cout << "FILE NOT FOUND\n";
        return;
    }

    directory_item *srcItem = (srcPath[0] == '/')
                                  ? locateDirectoryOrFile(srcPathParts, &rootDirectory[0]) // Absolute path
                                  : locateDirectoryOrFile(srcPathParts, currentDirectory); // Relative path

    if (!srcItem || srcItem->isFile == false)
    { // Ensure the source item exists and is a file
        std::cout << "FILE NOT FOUND\n";
        return;
    }

    // Determine the destination directory and possible new name
    std::vector<std::string> destPathParts = splitPath(destPath);
    directory_item *destDir = (destPath[0] == '/')
                                  ? resolvePath({destPathParts.begin(), destPathParts.end() - 1}, &rootDirectory[0]) // Absolute path
                                  : resolvePath({destPathParts.begin(), destPathParts.end() - 1}, currentDirectory); // Relative path

    if (!destDir || destDir->isFile)
    {
        std::cout << "PATH NOT FOUND\n";
        return;
    }

    std::string newName = destPathParts.back();
    bool nameConflict = false;

    for (const auto &child : destDir->children)
    {
        if (trimItemName(child.item_name) == newName)
        {
            if (child.isFile)
            {
                std::cout << "SAME NAME\n";
                return;
            }
            else
            {
                destDir = const_cast<directory_item *>(&child);
                // Move to subdirectory if name matches a directory
                newName = trimItemName(srcItem->item_name); // Keep original name in the subdirectory
                nameConflict = true;
                break;
            }
        }
    }

    directory_item *srcParent = (srcItem->parent_id == -1) ? &rootDirectory[0] : findParent(srcItem);

    if (srcParent)
    {
        // std::cout << "Located parent directory: " << trimItemName(srcParent->item_name)
        //           << " (ID: " << srcParent->id << ")\n";

        // Check if the parent is the root directory
        if (srcParent == &rootDirectory[0])
        {
            // Attempt to remove the item directly from rootDirectory by comparing IDs explicitly
            srcParent->children.erase(
                std::remove_if(srcParent->children.begin(), srcParent->children.end(),
                               [&](const directory_item &item)
                               {
                                   // Compare both the ID and name to avoid accidental deletion of similarly named items
                                   bool isMatch = (item.id == srcItem->id && trimItemName(item.item_name) == trimItemName(srcItem->item_name));

                                   // Debug output to confirm when a match is found
                                   if (isMatch)
                                   {
                                    //    std::cout << "Match found for removal: " << trimItemName(item.item_name)
                                    //              << " (ID: " << item.id << ")\n";
                                   }
                                   return isMatch;
                               }),
                srcParent->children.end());

            // Confirm rootDirectory contents after removal
            for (const auto &item : rootDirectory)
            {
                // std::cout << "Remaining item - Name: " << trimItemName(item.item_name)
                //           << ", ID: " << item.id << '\n';
            }
        }
        else
        {
            // std::cout << "Parent is a subdirectory: " << trimItemName(srcParent->item_name) << "\n";

            // Remove srcItem from its parent directory (subdirectory)
            srcParent->children.erase(
                std::remove_if(srcParent->children.begin(), srcParent->children.end(),
                               [&](const directory_item &item)
                               {
                                //    std::cout << "Checking child item with name: " << trimItemName(item.item_name)
                                //              << " and ID: " << item.id << '\n';
                                   // Explicitly compare the ID instead of pointer address
                                   bool isMatch = (item.id == srcItem->id && trimItemName(item.item_name) == trimItemName(srcItem->item_name));
                                   if (isMatch)
                                   {
                                    //    std::cout << "Match found in subdirectory for removal: " << trimItemName(item.item_name)
                                    //              << " (ID: " << item.id << ")\n";
                                   }
                                   return isMatch;
                               }),
                srcParent->children.end());

            
            // Confirmsubdirectory contents after removal
            for (const auto &child : srcParent->children)
            {
                // std::cout << "Remaining child item - Name: " << trimItemName(child.item_name)
                //           << ", ID: " << child.id << '\n';
            }
        }
        // std::cout << "Removed item from parent directory: " << trimItemName(srcParent->item_name) << "\n";
    }
    else
    {
        std::cout << "Could not find the parent directory for item: " << trimItemName(srcItem->item_name)
                  << " (ID: " << srcItem->id << ")\n";
    }

    // Create a copy of the item with updated attributes for the destination directory
    directory_item movedItem = *srcItem;
    movedItem.parent_id = destDir->id;
    std::strcpy(movedItem.item_name, newName.c_str());

    // Add the moved item to the destination directory
    destDir->children.push_back(movedItem);

    std::cout << "Moved item successfully to destination directory with ID: " << destDir->id << "\n";

    // Save changes to the .dat file to persist the move
    saveToFile();

    std::cout << "OK\n";
}

directory_item *PseudoFAT::resolvePath(const std::vector<std::string> &pathParts, directory_item *startDir)
{
    directory_item *current = startDir;

    for (const auto &part : pathParts)
    {
        // std::cout << "Processing path part: " << part << "\n";

        if (part == "..")
        {
            // Move up to the parent directory if not already at root
            if (current->parent_id != -1)
            {
                current = findDirectoryById(current->parent_id, &rootDirectory[0]);
                // std::cout << "Moved up to parent directory: " << (current ? trimItemName(current->item_name) : "nullptr") << "\n";
            }
        }
        else if (part != ".")
        {
            bool found = false;
            for (auto &child : current->children)
            {
                if (trimItemName(child.item_name) == part)
                {
                    current = &child;
                    found = true;
                    // std::cout << "Moved into directory: " << trimItemName(current->item_name) << "\n";
                    break;
                }
            }
            if (!found)
            {
                // std::cout << "Directory or file not found: " << part << "\n";
                return nullptr;
            }
        }
    }
    return current;
}

void PseudoFAT::cp(const std::string &srcPath, const std::string &destPath)
{
    // Step 1: Locate the source file
    std::vector<std::string> srcPathParts = splitPath(srcPath);
    directory_item *srcFile = (srcPath[0] == '/')
                                  ? locateDirectoryOrFile(srcPathParts, &rootDirectory[0]) // Absolute path
                                  : locateDirectoryOrFile(srcPathParts, currentDirectory); // Relative path

    if (!srcFile || !srcFile->isFile)
    {
        std::cout << "FILE NOT FOUND\n";
        return;
    }

    // Step 2: Locate the destination directory
    std::vector<std::string> destPathParts = splitPath(destPath);
    directory_item *destDir = (destPath[0] == '/')
                                  ? locateDirectoryOrFile({destPathParts.begin(), destPathParts.end() - 1}, &rootDirectory[0]) // Absolute path
                                  : locateDirectoryOrFile({destPathParts.begin(), destPathParts.end() - 1}, currentDirectory); // Relative path

    if (!destDir || destDir->isFile)
    {
        std::cout << "PATH NOT FOUND\n";
        return;
    }

    // Step 3: Check for duplicate file name in the destination
    std::string newName = destPathParts.back();
    for (const auto &child : destDir->children)
    {
        if (trimItemName(child.item_name) == newName)
        {
            std::cout << "SAME NAME\n";
            return;
        }
    }

    // Step 4: Allocate clusters for the copy
    int cluster = srcFile->start_cluster;
    std::vector<int> newClusters;
    int remainingSize = srcFile->size;

    while (cluster != FAT_FILE_END && cluster >= 0 && cluster < fat1.size())
    {
        int newCluster = allocateCluster();
        if (newCluster == -1)
        {
            std::cerr << "NOT ENOUGH SPACE\n";
            return;
        }
        newClusters.push_back(newCluster);

        // Copy data from the source cluster to the destination cluster
        char buffer[CLUSTER_SIZE] = {0};
        std::ifstream inFile(filename, std::ios::binary);
        inFile.seekg(desc.data_start_address + cluster * CLUSTER_SIZE);
        inFile.read(buffer, std::min(CLUSTER_SIZE, remainingSize)); // Only read as much as needed

        std::ofstream outFile(filename, std::ios::binary | std::ios::in | std::ios::out);
        outFile.seekp(desc.data_start_address + newCluster * CLUSTER_SIZE);
        outFile.write(buffer, std::min(CLUSTER_SIZE, remainingSize)); // Only write up to actual size

        cluster = fat1[cluster];
        remainingSize -= CLUSTER_SIZE; // Decrease remaining size by cluster size
    }

    // Update FAT to mark the end of the copied file's cluster chain
    for (size_t i = 0; i < newClusters.size(); ++i)
    {
        fat1[newClusters[i]] = (i == newClusters.size() - 1) ? FAT_FILE_END : newClusters[i + 1];
    }

    // Step 5: Add copied file to the destination directory
    directory_item newFile;
    std::strcpy(newFile.item_name, newName.c_str());
    newFile.isFile = true;
    newFile.size = srcFile->size;
    newFile.start_cluster = newClusters[0];
    newFile.parent_id = destDir->id;
    newFile.id = next_dir_id++;

    destDir->children.push_back(newFile);

    // Step 6: Persist changes to the disk
    saveToFile();

    std::cout << "OK\n";
}

bool PseudoFAT::load(const std::string &filePath)
{
    std::ifstream commandFile(filePath);
    if (!commandFile)
    {
        return false;
    }

    std::string line;
    while (std::getline(commandFile, line))
    {
        line = trimWhitespace(line);
        if (line.empty())
        {
            continue;
        }

        std::istringstream lineStream(line);
        std::string command;
        lineStream >> command;
        std::string arguments;
        std::getline(lineStream, arguments);
        arguments = trimWhitespace(arguments);

        if (command == "mkdir")
        {
            createDirectory(arguments);
        }
        else if (command == "rmdir")
        {
            rmdir(arguments);
        }
        else if (command == "cd")
        {
            changeDirectory(arguments);
        }
        else if (command == "ls")
        {
            listDirectory(arguments);
        }
        else if (command == "pwd")
        {
            std::cout << pwd() << "\n";
        }
        else if (command == "incp" || command == "outcp" || command == "mv" || command == "cp")
        {
            std::istringstream argsStream(arguments);
            std::string src, dest;
            argsStream >> src >> dest;
            if (command == "incp")
                incp(src, dest);
            else if (command == "outcp")
                outcp(src, dest);
            else if (command == "mv")
                mv(src, dest);
            else if (command == "cp")
                cp(src, dest);
        }
        else if (command == "rm")
        {
            rm(arguments);
        }
        else if (command == "cat")
        {
            cat(arguments);
        }
        else if (command == "info")
        {
            info(arguments);
        }
        else
        {
            std::cerr << "Unknown command in file: " << command << "\n";
        }
    }

    commandFile.close();
    return true;
}

std::string PseudoFAT::trimWhitespace(const std::string &str)
{
    size_t start = str.find_first_not_of(" \t");
    size_t end = str.find_last_not_of(" \t");
    return (start == std::string::npos) ? "" : str.substr(start, end - start + 1);
}

// void PseudoFAT::listDirectoryByPath(const std::string &path)
// {
//     // If no path is provided, list the contents of the current directory
//     if (path.empty())
//     {
//         listDirectory(path);
//         return;
//     }

//     // Split the path and determine if it’s absolute or relative
//     std::vector<std::string> pathParts = splitPath(path);
//     directory_item *targetDir = nullptr;

//     if (path[0] == '/') // Absolute path
//     {
//         targetDir = locateDirectoryOrFile(pathParts, &rootDirectory[0]);
//     }
//     else // Relative path
//     {
//         targetDir = locateDirectoryOrFile(pathParts, currentDirectory);
//     }

//     // Check if the target directory was found
//     if (targetDir != nullptr)
//     {
//         listDirectory(targetDir);
//     }
//     else
//     {
//         std::cerr << "Directory not found.\n";
//     }
// }

void PseudoFAT::bug(const std::string &filePath)
{
    // Locate the file by path
    std::vector<std::string> pathParts = splitPath(filePath);
    directory_item *file = locateDirectoryOrFile(pathParts, &rootDirectory[0]);

    if (!file || !file->isFile)
    {
        std::cout << "FILE NOT FOUND\n";
        return;
    }

    // Simulate corruption by setting a random cluster in the file’s chain to FAT_BAD_CLUSTER
    int cluster = file->start_cluster;
    bool corrupted = false;
    while (cluster != FAT_FILE_END && cluster >= 0 && cluster < fat1.size())
    {
        if (!corrupted) // Corrupt the first valid cluster we find
        {
            fat1[cluster] = FAT_BAD_CLUSTER;
            corrupted = true;
        }
        cluster = fat1[cluster];
    }

    if (corrupted)
    {
        std::cout << "OK - File system corrupted for testing\n";
        saveToFile(); // Save the corrupted state to persist changes
    }
    else
    {
        std::cout << "Error: Unable to corrupt the file.\n";
    }
}

bool PseudoFAT::check()
{
    bool corruptionFound = false;
    bool foundInAnyFile = true;
    std::vector<directory_item> allFiles = getAllFiles(); // Collect all files

    for (const auto &file : allFiles)
    {
        int cluster = file.start_cluster;
        while (cluster != FAT_FILE_END && cluster >= 0 && cluster < fat1.size())
        {
            if (fat1[cluster] == FAT_BAD_CLUSTER)
            {
                std::cout << "Corruption detected in file " << file.item_name
                          << ": Bad cluster " << cluster << " found in its chain.\n";
                corruptionFound = true;
                return true;
                break;
            }
            cluster = fat1[cluster];
        }
    }

    // Report orphaned bad clusters that aren’t part of any file’s cluster chain
    for (size_t i = 0; i < fat1.size(); ++i)
    {
        // std::cout << "Corruption detected  " << fat1[i] << " == " << FAT_BAD_CLUSTER << "\n";
        if (fat1[i] == FAT_BAD_CLUSTER)
        {
            bool foundInAnyFile = false;

            for (const auto &file : allFiles)
            {
                int cluster = file.start_cluster;
                while (cluster != FAT_FILE_END && cluster >= 0 && cluster < fat1.size())
                {
                    // std::cout << "Cluster : " << cluster << " == " << i << std::endl;
                    if (cluster == i)
                    {
                        foundInAnyFile = true;
                        return true;
                        break;
                    }
                    cluster = fat1[cluster];
                }
                if (foundInAnyFile)
                    break;
            }

            if (!foundInAnyFile)
            {
                std::cout << "Orphaned bad cluster detected at index " << i << ".\n";
                return true;
                corruptionFound = true;
            }
        }
    }

    if (!corruptionFound)
    {
        std::cout << "File system check complete: No corruption detected.\n";
        return false;
    }
    return false;
}

std::vector<directory_item> PseudoFAT::getAllFiles()
{
    std::vector<directory_item> files;
    collectFilesRecursively(&rootDirectory[0], files); // Start from the root directory
    return files;
}

// Helper method to recursively collect files
void PseudoFAT::collectFilesRecursively(directory_item *dir, std::vector<directory_item> &files)
{
    for (auto &item : dir->children)
    {
        if (item.isFile)
        {
            files.push_back(item); // Add file to the list
        }
        else
        {
            // If it's a directory, recursively explore it
            collectFilesRecursively(&item, files);
        }
    }
}
