#include "FileSystem.h"
// #include "utils.h"
#include <iostream>
#include <fstream>

FileSystem::FileSystem(const std::string &name) : name(name), isRunning(false), currentDirectory("/") {}

namespace fs = std::filesystem;

bool FileSystem::format(size_t sizeInBytes)
{
    std::ofstream fsFile(name, std::ios::binary | std::ios::trunc);
    if (!fsFile.is_open())
    {
        std::cerr << "CANNOT CREATE FILE\n";
        return false;
    }

    fsFile.seekp(sizeInBytes - 1);
    fsFile.write("", 1); // Write a single byte at the end to ensure file size
    fsFile.close();
    fat.initialize(); // Initialize PseudoFAT
    std::cout << "OK\n";
    commandLineInterface();
    return true;
}

bool FileSystem::start()
{
    std::ifstream fsFile(name, std::ios::binary);
    if (fsFile.is_open())
    {
        fat.load(fsFile);

        // fsFile.close();

        isRunning = true;
        std::cout << "File system started: " << name << "\n";
        commandLineInterface();
        return true;
    }
    else
    {
        std::cerr << "CANNOT OPEN FILE SYSTEM\n";
        return false;
    }
}

void FileSystem::exit()
{
    if (isRunning)
    {
        // Save the current state of the file system to the existing .dat file
        fat.save(name); // Pass the current file system name (which should be the .dat filename)
        isRunning = false;
        std::cout << "File system exited and changes saved.\n";
    }
}

void FileSystem::displayPrompt()
{
    std::cout << " > "; // Display prompt with filesystem name
}

void FileSystem::displayFSPrompt(const std::string &FSname)
{
    std::cout << FSname << " ~ % "; // Display prompt with filesystem name
}

void FileSystem::commandLineInterface()
{
    std::string command;

    while (isRunning)
    {
        displayFSPrompt(name);           // Show prompt
        std::getline(std::cin, command); // Get command input

        if (command == "exit")
        {
            exit(); // Call exit method
        }
        else if (command.find("mkdir") == 0)
        {
            // Extract the directory name after "mkdir"
            std::string dirname = command.substr(6);

            // Trim any whitespace from the directory name
            dirname.erase(0, dirname.find_first_not_of(" \t")); // Leading whitespace
            dirname.erase(dirname.find_last_not_of(" \t") + 1); // Trailing whitespace

            if (dirname.empty())
            {
                std::cout << "INVALID DIRECTORY NAME\n";
            }
            else
            {
                // Normalize the directory name for consistency
                std::string normalizedDir = dirname;
                normalizedDir.erase(0, normalizedDir.find_first_not_of('/')); // Remove leading slashes
                normalizedDir.erase(normalizedDir.find_last_not_of('/') + 1); // Remove trailing slashes

                // Check if the full path exists already
                if (pathExists(normalizedDir))
                {
                    std::cout << "EXIST\n";
                }
                else
                {
                    // Split the path to check if the parent directory exists
                    std::string parentDir = normalizedDir.substr(0, normalizedDir.find_last_of('/'));

                    // If it's a nested directory, we need to check if the parent exists
                    if (parentDir.empty())
                    {
                        parentDir = "/"; // Root directory
                    }

                    // Check if the parent directory exists
                    // if (!directoryExists(parentDir))
                    // {
                    //     std::cout << "PATH NOT FOUND\n"; // Parent directory doesn't exist
                    // }
                    // else 
                    if (createDirectory("dir1")) // Try to create the new directory
                    {
                        std::cout << "OK\n";
                    }
                    else
                    {
                        std::cout << "COULD NOT CREATE DIRECTORY\n";
                    }
                }
            }
        }
        else if (command.find("rmdir") == 0)
        {
            std::string dirname = command.substr(6); // Extract directory name
            // if (!directoryExists(dirname))
            // {
            //     std::cout << "FILE NOT FOUND\n";
            // }
            // else if (directoryIsEmpty(dirname))
            // {
            //     if (removeDirectory(dirname))
            //     {
            //         std::cout << "OK\n";
            //     }
            // }
            // else
            // {
            //     std::cout << "NOT EMPTY\n";
            // }
        }
        else if (command == "ls")
        {
            listDirectories(); // List current directory contents
        }
        else if (command.find("ls ") == 0)
        {
            std::string path = command.substr(3);
            if (directoryExists(path))
            {
                listDirectory(path); // List specific directory contents
            }
            else
            {
                std::cout << "PATH NOT FOUND\n";
            }
        }
        else if (command.find("cp ") == 0)
        {
            // cp s1 s2 (Copy file s1 to s2)
            std::string src = command.substr(3, command.find(" ") - 3);
            std::string dest = command.substr(command.find(" ") + 1);
            // if (!fileExists(src))
            // {
            //     std::cout << "FILE NOT FOUND\n";
            // }
            // else if (!pathExists(dest))
            // {
            //     std::cout << "PATH NOT FOUND\n";
            // }
            // else if (copyFile(src, dest))
            // {
            //     std::cout << "OK\n";
            // }
        }
        else if (command.find("mv ") == 0)
        {
            // mv s1 s2 (Move/rename file s1 to s2)
            std::string src = command.substr(3, command.find(" ") - 3);
            std::string dest = command.substr(command.find(" ") + 1);
            // if (!FSfileExists(src))
            // {
            //     std::cout << "FILE NOT FOUND\n";
            // }
            // else if (!pathExists(dest))
            // {
            //     std::cout << "PATH NOT FOUND\n";
            // }
            // else if (moveFile(src, dest))
            // {
            //     std::cout << "OK\n";
            // }
        }
        else if (command.find("rm ") == 0)
        {
            // rm s1 (Remove file s1)
            std::string file = command.substr(3);
            // if (!fileExists(file))
            // {
            //     std::cout << "FILE NOT FOUND\n";
            // }
            // else if (removeFile(file))
            // {
            //     std::cout << "OK\n";
            // }
        }
        else if (command.find("cat ") == 0)
        {
            // cat s1 (Display file contents)
            std::string file = command.substr(4);
            // if (!fileExists(file))
            // {
            //     std::cout << "FILE NOT FOUND\n";
            // }
            // else
            // {
            //     displayFileContents(file); // Display file contents
            // }
        }
        else if (command.find("cd ") == 0)
        {
            // cd a1 (Change directory)
            std::string path = command.substr(3);
            if (!directoryExists(path))
            {
                std::cout << "PATH NOT FOUND\n";
            }
            else
            {
                changeDirectory(path); // Change directory
                std::cout << "OK\n";
            }
        }
        else if (command.find("cd") == 0)
        {
            std::string dirname = command.substr(3); // Extract directory name
            if (changeDirectory(dirname))
            {
                std::cout << "Changed directory to: " << currentDirectory << "\n";
            }
        }
        else if (command == "help")
        {
            execute_help(); // Display help message
        }
        else if (command == "pwd")
        {
            // Print working directory
            std::cout << getCurrentDirectory() << "\n"; // Print current directory
        }
        else if (command.find("info ") == 0)
        {
            // info s1/a1 (Print info of file or directory)
            std::string path = command.substr(5);
            if (!pathExists(path))
            {
                std::cout << "FILE NOT FOUND\n";
            }
            else
            {
                printFileInfo(path); // Print file/directory cluster info
            }
        }
        else if (command.find("incp ") == 0)
        {
            // incp s1 s2 (Copy file from disk to FS)
            std::string src = command.substr(5, command.find(" ") - 5);
            std::string dest = command.substr(command.find(" ") + 1);
            // if (!fileExistsOnDisk(src))
            // {
            //     std::cout << "FILE NOT FOUND\n";
            // }
            // else if (!pathExists(dest))
            // {
            //     std::cout << "PATH NOT FOUND\n";
            // }
            // else if (importFileToFS(src, dest))
            // {
            //     std::cout << "OK\n";
            // }
        }
        else if (command.find("outcp ") == 0)
        {
            // outcp s1 s2 (Copy file from FS to disk)
            std::string src = command.substr(6, command.find(" ") - 6);
            std::string dest = command.substr(command.find(" ") + 1);
            // if (!FSfileExists(src))
            // {
            //     std::cout << "FILE NOT FOUND\n";
            // }
            // else if (!pathExistsOnDisk(dest))
            // {
            //     std::cout << "PATH NOT FOUND\n";
            // }
            // else if (exportFileFromFS(src, dest))
            // {
            //     std::cout << "OK\n";
            // }
        }
        else if (command.find("load ") == 0)
        {
            // load s1 (Load and execute commands from file)
            std::string file = command.substr(5);
            // if (!fileExistsOnDisk(file))
            // {
            //     std::cout << "FILE NOT FOUND\n";
            // }
            // else
            // {
            //     loadAndExecuteCommands(file); // Load and execute commands
            //     std::cout << "OK\n";
            // }
        }
        else if (command.empty())
        {
            continue; // Ignore empty commands
        }
        else
        {
            std::cerr << "UNKNOWN COMMAND\n";
        }
    }
}

bool FileSystem::createDirectory(const std::string &dirname)
{
    if (std::any_of(fat.directoryItems.begin(), fat.directoryItems.end(),
                    [&](const DirectoryItem &item)
                    { return item.item_name == dirname && !item.isFile; }))
    {
        std::cout << "EXIST\n";
        return false;
    }

    DirectoryItem newDir;
    std::strncpy(newDir.item_name, dirname.c_str(), sizeof(newDir.item_name) - 1);
    newDir.item_name[sizeof(newDir.item_name) - 1] = '\0';
    newDir.isFile = false;
    newDir.size = 0;
    newDir.start_cluster = allocateCluster();

    if (newDir.start_cluster == -1)
    {
        std::cout << "COULD NOT CREATE DIRECTORY\n";
        return false;
    }

    fat.directoryItems.push_back(newDir);
    saveFAT();
    std::cout << "OK\n";
    return true;
}

// Removes a directory if it's empty
bool FileSystem::removeDirectory(const std::string &dirname)
{
    if (!fs::exists(dirname))
    {
        std::cout << "FILE NOT FOUND\n";
        return false;
    }

    if (!fs::is_empty(dirname))
    {
        std::cout << "NOT EMPTY\n";
        return false;
    }

    try
    {
        if (fs::remove(dirname))
        {
            return true; // Directory successfully removed
        }
    }
    catch (const fs::filesystem_error &e)
    {
        std::cerr << "ERROR: " << e.what() << '\n';
    }
    return false;
}

// Lists the contents of the current or specified directory
void FileSystem::listDirectory(const std::string &dirname)
{
    if (!fs::exists(dirname))
    {
        std::cout << "PATH NOT FOUND\n";
        return;
    }

    for (const auto &entry : fs::directory_iterator(dirname))
    {
        if (entry.is_directory())
        {
            std::cout << "DIR: " << entry.path().filename().string() << "\n";
        }
        else
        {
            std::cout << "FILE: " << entry.path().filename().string() << "\n";
        }
    }
}

// Lists the contents of the current directory
void FileSystem::listDirectories()
{
    if (fat.directoryItems.empty())
    {
        std::cout << "Directory is empty.\n";
        return;
    }

    std::cout << "Directories and files:\n";
    for (const auto &item : fat.directoryItems)
    {
        if (!item.isFile)
        {
            std::cout << "DIR: " << item.item_name << "\n"; // Display directories
        }
        else
        {
            std::cout << "FILE: " << item.item_name << "\n"; // Display files
        }
    }
}

// Copies a file from source to destination
bool FileSystem::copyFile(const std::string &source, const std::string &destination)
{
    if (!fs::exists(source))
    {
        std::cout << "FILE NOT FOUND\n";
        return false;
    }

    try
    {
        fs::copy(source, destination);
        return true;
    }
    catch (const fs::filesystem_error &e)
    {
        std::cerr << "ERROR: " << e.what() << '\n';
    }
    return false;
}

// Moves or renames a file
bool FileSystem::moveFile(const std::string &source, const std::string &destination)
{
    if (!fs::exists(source))
    {
        std::cout << "FILE NOT FOUND\n";
        return false;
    }

    try
    {
        fs::rename(source, destination);
        return true;
    }
    catch (const fs::filesystem_error &e)
    {
        std::cerr << "ERROR: " << e.what() << '\n';
    }
    return false;
}

// Removes a file
bool FileSystem::removeFile(const std::string &filename)
{
    if (!fs::exists(filename))
    {
        std::cout << "FILE NOT FOUND\n";
        return false;
    }

    try
    {
        if (fs::remove(filename))
        {
            return true;
        }
    }
    catch (const fs::filesystem_error &e)
    {
        std::cerr << "ERROR: " << e.what() << '\n';
    }
    return false;
}

// Displays the contents of a file
void FileSystem::displayFileContents(const std::string &filename)
{
    if (!fs::exists(filename))
    {
        std::cout << "FILE NOT FOUND\n";
        return;
    }

    std::ifstream file(filename);
    if (file.is_open())
    {
        std::string line;
        while (getline(file, line))
        {
            std::cout << line << "\n";
        }
        file.close();
    }
    else
    {
        std::cerr << "CANNOT OPEN FILE\n";
    }
}

// Changes the current working directory
bool FileSystem::changeDirectory(const std::string &dirname)
{
    // Check if the directory exists
    if (directoryExists(dirname))
    {
        currentDirectory = dirname; // Update the current directory
        return true;
    }
    else
    {
        std::cerr << "PATH NOT FOUND\n";
        return false;
    }
}

// Returns the current working directory
std::string FileSystem::getCurrentDirectory() 
{
    return currentDirectory;
}

// Prints file or directory information
void FileSystem::printFileInfo(const std::string &path)
{
    if (!fs::exists(path))
    {
        std::cout << "FILE NOT FOUND\n";
        return;
    }

    // Assuming a simple method to print file or directory info
    std::cout << path << " occupies clusters: "; // Simplified info output
    // You'd need to retrieve and print cluster information from your FAT structure here
    std::cout << "2, 3, 4, 7, 10\n"; // Example output, adapt to your logic
}

// Loads and executes commands from a file
void FileSystem::loadAndExecuteCommands(const std::string &filename)
{
    if (!fs::exists(filename))
    {
        std::cout << "FILE NOT FOUND\n";
        return;
    }

    std::ifstream file(filename);
    if (file.is_open())
    {
        std::string command;
        while (getline(file, command))
        {
            // Execute each command sequentially
            std::cout << "Executing: " << command << "\n";
            // Reuse your command processing logic here, for example:
            executeCommand(command);
        }
        file.close();
    }
    else
    {
        std::cerr << "CANNOT OPEN FILE\n";
    }
}

// Imports a file from the disk into the file system
bool FileSystem::importFileToFS(const std::string &source, const std::string &destination)
{
    if (!fs::exists(source))
    {
        std::cout << "FILE NOT FOUND\n";
        return false;
    }

    // Add your logic to import the file into your pseudoFAT system
    // For now, we simulate success
    std::cout << "Importing file from " << source << " to " << destination << "\n";
    return true;
}

// Exports a file from the file system to the disk
bool FileSystem::exportFileFromFS(const std::string &source, const std::string &destination)
{
    // if (!FSfileExists(source))
    // {
    //     std::cout << "FILE NOT FOUND\n";
    //     return false;
    // }

    // Add your logic to export the file from your pseudoFAT system to disk
    // For now, we simulate success
    std::cout << "Exporting file from " << source << " to " << destination << "\n";
    return true;
}

// Helper function to execute a command (can be used for 'load' command)
void FileSystem::executeCommand(const std::string &command)
{
    // Reuse your existing commandLineInterface logic here to process the command
    commandLineInterface(); // Adapt this to call individual commands
}

// Help command to display all available commands and their usage
void FileSystem::execute_help()
{
    std::cout << "Available commands:\n";
    std::cout << "cp <source> <destination>    - Copy file from source to destination\n";
    std::cout << "mv <source> <destination>    - Move or rename file from source to destination\n";
    std::cout << "rm <filename>                - Remove file\n";
    std::cout << "mkdir <directory>            - Create a directory\n";
    std::cout << "rmdir <directory>            - Remove a directory\n";
    std::cout << "ls [directory]               - List contents of directory (current if not specified)\n";
    std::cout << "cat <filename>               - Display contents of a file\n";
    std::cout << "cd <directory>               - Change the current directory\n";
    std::cout << "pwd                          - Print the current directory\n";
    std::cout << "help                         - Display this help message\n";
    std::cout << "exit                         - Exit the pseudoFAT file system\n";
}


/***************************** Check methods ************************************** */
bool FileSystem::pathExists(const std::string &path)
{
    // Check if the full path exists
    std::string normalizedPath = path;
    normalizedPath.erase(0, normalizedPath.find_first_not_of('/')); // Remove leading slashes
    normalizedPath.erase(normalizedPath.find_last_not_of('/') + 1); // Remove trailing slashes

    for (const auto &item : fat.directoryItems)
    {
        if (item.item_name == normalizedPath)
        {
            return true; // Path exists
        }
    }
    return false; // Path does not exist
}

bool FileSystem::directoryExists(const std::string &dirname)
{
    for (const auto &item : fat.directoryItems)
    {
        if (item.item_name == dirname && !item.isFile) // Ensure it's a directory, not a file
        {
            return true; // Directory exists
        }
    }
    return false; // Directory does not exist
}

void FileSystem::loadFAT()
{
    std::ifstream fsFile(name, std::ios::binary);
    if (!fsFile.is_open())
    {
        std::cerr << "Cannot open file for loading.\n";
        return;
    }

    fat.load(fsFile);
    fsFile.close();
}

int32_t FileSystem::allocateCluster()
{
    for (size_t i = 0; i < fat.fatTable.size(); ++i)
    {
        if (fat.fatTable[i] == FAT_UNUSED) // Check for an unused cluster
        {
            fat.fatTable[i] = FAT_USED;     // Mark the cluster as used (or another suitable marker)
            return static_cast<int32_t>(i); // Return the index as the cluster number
        }
    }
    return -1; // No available cluster found
}

void FileSystem::saveFAT()
{
    fat.save(name);
}
