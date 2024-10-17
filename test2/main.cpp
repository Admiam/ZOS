#include <iostream>
#include <string>
#include <vector>
#include <sstream> // This header provides std::stringstream
#include "PseudoFAT-test2.h"

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        std::cerr << "Usage: " << argv[0] << " <filename>\n";
        return 1;
    }

    PseudoFAT2 fs(argv[1]);
    std::string command, arg;

    // Define currentDirectory
    directory_item *currentDirectory = nullptr;

    std::cout << "main\n";

    // Initialize currentDirectory to point to the root directory
    currentDirectory = fs.getRootDirectory(); // Assuming you have a method to get the root directory

    std::cout << "main1\n";

    while (true)
    {
        std::string prompt = fs.getFullPath(currentDirectory) + " > ";
        std::cout << prompt;

        std::cin >> command;

        if (command == "exit")
        {
            break;
        }
        else if (command == "mkdir" || command == "rmdir" || command == "cd" || command == "ls" || command == "pwd" || command == "incp" || command == "cat")
        {

            // For these commands, read the rest of the line
            std::getline(std::cin, arg);

            // Check if arg contains any input before trimming whitespace
            if (!arg.empty())
            {
                arg = arg.substr(arg.find_first_not_of(" \t")); // Trim leading whitespace
            }

        }

        if (command == "mkdir")
        {
            fs.createDirectory(arg);
        }
        else if (command == "rmdir")
        {
            fs.rmdir(arg);
        }
        else if (command == "ls")
        {
            // std::cout << "ls\n";

            // std::getline(std::cin, arg);

            // std::cout << "ls\n";

            // Check if arg contains any input before trimming whitespace
           

            // std::cout << "ls\n";

            if (arg.empty())
            {
                // If no path is provided, list the contents of the current directory
                fs.listDirectory(currentDirectory);
            }
            else
            {
                // If a path is provided, find the target directory
                std::vector<std::string> pathParts = fs.splitPath(arg);
                directory_item *targetDir = nullptr;

                if (arg[0] == '/') // Absolute path
                {
                    targetDir = fs.findDirectoryFromRoot(pathParts);
                }
                else // Relative path
                {
                    std::cout << "currentDirectory: " << currentDirectory << '\n';
                    targetDir = fs.findDirectory(pathParts);
                }

                if (targetDir != nullptr)
                {
                    fs.listDirectory(targetDir);
                }
                else
                {
                    std::cerr << "Directory not found.\n";
                }
            }
        }
        else if (command == "cd")
        {
            if (arg.empty())
            {
                // If no path is provided, change to the root directory
                fs.changeDirectory("/");
                currentDirectory = fs.currentDirectory;
                std::cout << "currentDirectory: " << currentDirectory << '\n';
            }
            else
            {
                // Change to the specified path
                currentDirectory = fs.getCurrentDirectory(); // Make sure this method returns the correct directory

                fs.changeDirectory(arg);
                currentDirectory = fs.currentDirectory;
                std::cout << "currentDirectory: " << currentDirectory << '\n';
            }
        }
        else if (command == "pwd")
        {
            // Call the pwd method to get the current path
            std::string path = fs.pwd();
            std::cout << "PATH: " << path << std::endl;
        }
        else if (command == "incp")
        {
            // Handle incp (import copy) command: requires two arguments
            std::string srcPath, destPath;
            size_t splitPos = arg.find(' ');

            if (splitPos != std::string::npos)
            {
                srcPath = arg.substr(0, splitPos);   // First argument: source file (s1)
                destPath = arg.substr(splitPos + 1); // Second argument: destination path (s2)

                // Perform the incp operation
                fs.incp(srcPath, destPath);
            }
            else
            {
                std::cerr << "Error: incp command requires two arguments (source and destination)\n";
            }
        }
        else if (command == "cat")
        {
            // Handle the cat command to print file contents
            if (!arg.empty())
            {
                fs.cat(arg);
            }
            else
            {
                std::cerr << "Error: cat command requires a file path.\n";
            }
        }
        else
        {
            std::cout << "Unknown command\n";
        }
    }

    return 0;
}
