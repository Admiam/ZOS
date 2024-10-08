#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <string>
#include "PseudoFAT.h" // Include the PseudoFAT class

class FileSystem
{
public:
    // Constructor
    FileSystem(const std::string &name);

    // File system operations
    bool format(size_t sizeInBytes);                 // Format the file system
    bool start();                                    // Start the file system
    void exit();                                     // Exit the file system
    void displayPrompt();                            // Display the command prompt
    void displayFSPrompt(const std::string &FSname); // Display the file system prompt
    void commandLineInterface();                     // Command-line interface for managing files and directories

private:
    std::string name; // Name of the file system
    bool isRunning;   // Indicates if the file system is running
    PseudoFAT fat;    // Instance of PseudoFAT class to manage the FAT
    std::string currentDirectory; // Current working directory

    // Helper functions for directory and file management
    bool createDirectory(const std::string &dirname); // Create a new directory
    bool removeDirectory(const std::string &dirname); // Remove a directory
    
    void listDirectories();                           // List directories
    void listDirectory(const std::string &dirname);   // List a specific directory
    void execute_help();                              // Display help message

    // File operations
    bool copyFile(const std::string &source, const std::string &destination); // Copy a file
    bool moveFile(const std::string &source, const std::string &destination); // Move or rename a file
    bool removeFile(const std::string &filename);                             // Remove a file
    void displayFileContents(const std::string &filename);                    // Display file contents

    // Directory and path operations
    bool changeDirectory(const std::string &dirname); // Change the current working directory
    std::string getCurrentDirectory();             // Get the current working directory
    void printFileInfo(const std::string &path);   // Print file or directory information

    // External file loading
    bool importFileToFS(const std::string &source, const std::string &destination);   // Import a file to FS
    bool exportFileFromFS(const std::string &source, const std::string &destination); // Export a file from FS

    // Command execution
    void loadAndExecuteCommands(const std::string &filename); // Load and execute commands from a file
    void executeCommand(const std::string &command);          // Execute a single command
    int32_t allocateCluster();                                // Allocate a new cluster in FAT
    void saveFAT();
    void loadFAT();

        // Helper functions for path and file checks
        bool
        pathExists(const std::string &path);           // Check if a path exists
    bool directoryExists(const std::string &dirname);  // Check if a directory exists
    // bool directoryIsEmpty(const std::string &dirname); // Check if a directory is empty
    // bool fileExists(const std::string &src);           // Check if a file exists
    // bool fileExistsOnDisk(const std::string &src);     // Check if a file exists on disk
    // bool pathExistsOnDisk(const std::string &dest);    // Save the FAT structure to the .dat file
};

#endif // FILESYSTEM_H
