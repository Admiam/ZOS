#include "PseudoFAT-test2.h"
#include <sstream> // This header provides std::stringstream
#include <algorithm> // For std::find_if

// FAT markers definitions
const int32_t FAT_UNUSED = INT32_MAX - 1;
const int32_t FAT_FILE_END = INT32_MAX - 2;
const int32_t FAT_BAD_CLUSTER = INT32_MAX - 3;

const int32_t CLUSTER_SIZE = 4096;           // 4KB cluster size
const int32_t DISK_SIZE = 600 * 1024 * 1024; // 600MB disk size

PseudoFAT2::PseudoFAT2(const std::string &file) : filename(file), currentDirectory(nullptr), next_dir_id(0)
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
        updateNextDirId(); // Ensure next_dir_id is set correctly after loading
    }
}

    // Format the disk
    void PseudoFAT2::formatDisk()
    {
        // Initialize description
        std::strcpy(desc.signature, "mikaa");
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

        rootDirectory.clear();
        directory_item root;
        strcpy(root.item_name, "/");
        root.isFile = false;
        root.size = 0;
        root.start_cluster = -1;
        root.parent_id = -1;
        root.id = next_dir_id++;
        rootDirectory.push_back(root);

        currentDirectory = &rootDirectory[0];

        saveToFile();
    }

    void PseudoFAT2::saveToFile()
    {
        std::ofstream out(filename, std::ios::binary);
        if (!out)
        {
            std::cerr << "Error opening file for saving.\n";
            return;
        }

        // Write the file system description (metadata)
        out.write(reinterpret_cast<const char *>(&desc), sizeof(desc));

        // Write the FAT1 and FAT2 tables
        out.write(reinterpret_cast<const char *>(fat1.data()), fat1.size() * sizeof(int32_t));
        out.write(reinterpret_cast<const char *>(fat2.data()), fat2.size() * sizeof(int32_t));

        // Recursive function to save a directory and its children
        std::function<void(const directory_item &)> saveDirectory = [&](const directory_item &dir)
        {
            // Save the directory itself
            out.write(reinterpret_cast<const char *>(&dir.item_name), sizeof(dir.item_name));
            out.write(reinterpret_cast<const char *>(&dir.isFile), sizeof(dir.isFile));
            out.write(reinterpret_cast<const char *>(&dir.size), sizeof(dir.size));
            out.write(reinterpret_cast<const char *>(&dir.start_cluster), sizeof(dir.start_cluster));
            out.write(reinterpret_cast<const char *>(&dir.parent_id), sizeof(dir.parent_id));
            out.write(reinterpret_cast<const char *>(&dir.id), sizeof(dir.id));

            // Save the number of children
            size_t childrenCount = dir.children.size();
            out.write(reinterpret_cast<const char *>(&childrenCount), sizeof(size_t));

            // Recursively save each child directory
            for (const auto &child : dir.children)
            {
                saveDirectory(child);
            }
        };

        // Save root directory and all its children recursively
        size_t dirCount = rootDirectory.size();
        out.write(reinterpret_cast<const char *>(&dirCount), sizeof(size_t));

        for (const auto &dir : rootDirectory)
        {
            saveDirectory(dir);
        }

        out.close();
    }

    void PseudoFAT2::loadFromFile() 
    {
        std::ifstream in(filename, std::ios::binary);
        if (!in)
        {
            std::cerr << "Error opening file for loading.\n";
            return;
        }

        // Read the file system description
        in.read(reinterpret_cast<char *>(&desc), sizeof(desc));
        std::cout << "Disk size: " << desc.disk_size << ", FAT count: " << desc.fat_count << "\n";

        // Read FAT1 and FAT2
        fat1.resize(desc.fat_count);
        in.read(reinterpret_cast<char *>(fat1.data()), fat1.size() * sizeof(int32_t));
        fat2.resize(desc.fat_count);
        in.read(reinterpret_cast<char *>(fat2.data()), fat2.size() * sizeof(int32_t));

        // Debugging FAT table entries
        std::cout << "FAT1[0]: " << fat1[0] << "\n";
        std::cout << "FAT1[1]: " << fat1[1] << "\n";

        // Recursive function to load a directory and its children
        std::function<void(directory_item &)> loadDirectory = [&](directory_item &dir)
        {
            // Load the directory itself
            in.read(reinterpret_cast<char *>(&dir.item_name), sizeof(dir.item_name));
            in.read(reinterpret_cast<char *>(&dir.isFile), sizeof(dir.isFile));
            in.read(reinterpret_cast<char *>(&dir.size), sizeof(dir.size));
            in.read(reinterpret_cast<char *>(&dir.start_cluster), sizeof(dir.start_cluster));
            in.read(reinterpret_cast<char *>(&dir.parent_id), sizeof(dir.parent_id));
            in.read(reinterpret_cast<char *>(&dir.id), sizeof(dir.id));

            // Debugging: Print directory info
            std::cout << "Loaded directory: " << dir.item_name << ", ID: " << dir.id << ", Parent ID: " << dir.parent_id << "\n";

            // Load the number of children
            size_t childrenCount;
            in.read(reinterpret_cast<char *>(&childrenCount), sizeof(size_t));

            // Debugging: Check children count before resizing
            if (childrenCount > 1000) // You can adjust this threshold based on expected maximum children
            {
                std::cerr << "Error: Invalid children count (" << childrenCount << ") for directory: " << dir.item_name << "\n";
                return; // Avoid crashing by returning early if the count is clearly invalid
            }

            dir.children.resize(childrenCount);

            // Recursively load each child directory
            for (auto &child : dir.children)
            {
                loadDirectory(child);
            }
        };

        // Load root directory and all its children recursively
        size_t dirCount;
        in.read(reinterpret_cast<char *>(&dirCount), sizeof(size_t));
        std::cout << "Root directory count: " << dirCount << "\n";

        // Debugging: Check root directory count before resizing
        if (dirCount > 1000) // Adjust this threshold as needed
        {
            std::cerr << "Error: Invalid root directory count (" << dirCount << ")\n";
            return; // Avoid crashing by returning early if the count is clearly invalid
        }

        rootDirectory.resize(dirCount);

        for (auto &dir : rootDirectory)
        {
            loadDirectory(dir);
        }

        currentDirectory = &rootDirectory[0]; // Set current directory to root

        in.close();
    }

    bool PseudoFAT2::createDirectory(const std::string &path)
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

    void PseudoFAT2::listDirectory(const directory_item *dir)
    {

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

    std::string PseudoFAT2::trimItemName(const char *itemName)
    {
        std::string name(itemName);

        // Trim trailing spaces
        name.erase(std::find_if(name.rbegin(), name.rend(), [](unsigned char ch)
                                { return !std::isspace(ch); })
                       .base(),
                   name.end());

        return name;
    }

    directory_item *PseudoFAT2::findDirectory(const std::vector<std::string> &pathParts)
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

    directory_item *PseudoFAT2::findDirectoryFromRoot(const std::vector<std::string> &pathParts)
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

    std::vector<std::string> PseudoFAT2::splitPath(const std::string &path) const
    {
        std::vector<std::string> result;
        std::istringstream stream(path);
        std::string token;
    
        while (std::getline(stream, token, '/'))
        {
            if (token == "..")
            {
                if (!result.empty())
                {
                    result.pop_back(); // Go up a directory
                }
            }
            else if (!token.empty() && token != ".")
            {
                result.push_back(token); // Add non-empty, non-current directory tokens
            }
        }
        return result;
    }

    directory_item *PseudoFAT2::getRootDirectory()
    {
        return &rootDirectory[0]; // Assuming the root is stored in rootDirectory[0]
    }

    void PseudoFAT2::initializeFAT()
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

    bool PseudoFAT2::validateAndFormatName(const std::string &name, char *formattedName)
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

    bool PseudoFAT2::changeDirectory(const std::string &path)
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

    directory_item *PseudoFAT2::findDirectoryById(int32_t id, directory_item *dir)
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

    directory_item *PseudoFAT2::getCurrentDirectory()
    {
        return currentDirectory; // Return the current directory pointer
    }

    std::string PseudoFAT2::getFullPath(directory_item* dir)
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

    std::string PseudoFAT2::removeSpaces(const std::string &input)
{
    std::string result = input;
    result.erase(std::remove(result.begin(), result.end(), ' '), result.end());
    return result;
}

    void PseudoFAT2::updateNextDirId()
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

    bool PseudoFAT2::rmdir(const std::string &path)
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

    std::string PseudoFAT2::pwd()
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

    bool PseudoFAT2::incp(const std::string &srcPath, const std::string &destPath)
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

    // Step 3: Check if the destination path exists in the file system
    std::vector<std::string> pathParts = splitPath(destPath);
    directory_item *targetDir = nullptr;

    if (destPath[0] == '/')
    { // Absolute path
        targetDir = findDirectoryFromRoot(pathParts);
    }
    else
    { // Relative path
        targetDir = findDirectory(pathParts);
    }

    if (!targetDir)
    {
        std::cerr << "PATH NOT FOUND\n";
        return false;
    }

    // Step 4: Ensure there is enough space in the file system to store the file
    int requiredClusters = (srcFileSize + CLUSTER_SIZE - 1) / CLUSTER_SIZE; // Round up
    int availableClusters = countFreeClusters();                            // Count how many free clusters are available

    if (requiredClusters > availableClusters)
    {
        std::cerr << "NOT ENOUGH SPACE\n";
        return false;
    }

    // Step 5: Allocate clusters for the file and update the FAT
    std::vector<int> allocatedClusters;
    for (int i = 0; i < requiredClusters; ++i)
    {
        int freeCluster = allocateCluster();
        if (freeCluster == -1)
        {
            std::cerr << "NOT ENOUGH SPACE\n";
            return false;
        }
        allocatedClusters.push_back(freeCluster);
    }

    // Step 6: Write the file to the allocated clusters in the .dat file
    std::ofstream outFile(filename, std::ios::binary | std::ios::in | std::ios::out);

    for (int i = 0; i < requiredClusters; ++i)
    {
        char buffer[CLUSTER_SIZE] = {0};
        srcFile.read(buffer, CLUSTER_SIZE);
        outFile.seekp(desc.data_start_address + allocatedClusters[i] * CLUSTER_SIZE);
        outFile.write(buffer, CLUSTER_SIZE);

        // Update FAT to link clusters
        if (i < requiredClusters - 1)
        {
            fat1[allocatedClusters[i]] = allocatedClusters[i + 1]; // Link to next cluster
        }
        else
        {
            fat1[allocatedClusters[i]] = FAT_FILE_END; // Mark the last cluster
        }
    }

    // Step 7: Update directory structure in the file system (add the new file to the target directory)
    directory_item newFile;
    std::strcpy(newFile.item_name, pathParts.back().c_str()); // Set the file name
    newFile.isFile = true;
    newFile.size = srcFileSize;
    newFile.start_cluster = allocatedClusters[0];
    newFile.id = next_dir_id++; // Assign unique ID

    targetDir->children.push_back(newFile);

    // Step 8: Save the updated file system back to the .dat file
    saveToFile();

    std::cout << "OK\n";
    return true;
}

    int PseudoFAT2::countFreeClusters()
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

    int PseudoFAT2::allocateCluster()
{
    for (size_t i = 0; i < fat1.size(); ++i)
    {
        if (fat1[i] == FAT_UNUSED)
        {
            fat1[i] = FAT_FILE_END; // Mark as used
            return i;               // Return the allocated cluster index
        }
    }
    return -1; // No free cluster found
}

    bool PseudoFAT2::cat(const std::string &filePath)
{
    // Step 1: Find the target file
    std::vector<std::string> pathParts = splitPath(filePath);
    directory_item *targetFile = nullptr;

    if (filePath[0] == '/')
    { // Absolute path
        targetFile = findDirectoryFromRoot(pathParts);
    }
    else
    { // Relative path
        targetFile = findDirectory(pathParts);
    }

    std::cout << "targetFile: " << targetFile << '\n';

    if (!targetFile || !targetFile->isFile)
    {
        std::cerr << "FILE NOT FOUND\n";
        return false;
    }

    // Step 2: Read the file data from the clusters
    std::ifstream inFile(filename, std::ios::binary);
    if (!inFile)
    {
        std::cerr << "Error opening file system .dat file.\n";
        return false;
    }

    int cluster = targetFile->start_cluster;
    std::vector<char> fileData(targetFile->size); // Create buffer for file data
    size_t bytesRead = 0;

    while (cluster != FAT_FILE_END && bytesRead < targetFile->size)
    {
        inFile.seekg(desc.data_start_address + cluster * CLUSTER_SIZE);

        // Read the data from the cluster
        size_t bytesToRead = std::min(static_cast<size_t>(CLUSTER_SIZE), targetFile->size - bytesRead);
        inFile.read(fileData.data() + bytesRead, bytesToRead);
        bytesRead += bytesToRead;

        // Move to the next cluster in the FAT
        cluster = fat1[cluster];
    }

    inFile.close();

    // Step 3: Print the file content
    std::cout.write(fileData.data(), targetFile->size);
    std::cout << std::endl;

    return true;
}
