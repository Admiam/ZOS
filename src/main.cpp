#include <iostream>
#include <string>
#include <vector>
#include <sstream> // This header provides std::stringstream
#include "PseudoFAT.h"

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        std::cerr << "Usage: " << argv[0] << " <filename>\n";
        return 1;
    }

    PseudoFAT fs(argv[1]);
    std::string command, arg;

    // Define currentDirectory
    directory_item *currentDirectory = nullptr;

    // std::cout << "main\n";

    // Initialize currentDirectory to point to the root directory
    currentDirectory = fs.getRootDirectory(); // Assuming you have a method to get the root directory

    // std::cout << "main1\n";

    while (true)
    {
        std::string prompt = fs.getFullPath(currentDirectory) + " > ";
        std::cout << prompt;

        std::cin >> command;
        std::string arg; // Reset argument for each command

        if (command == "exit")
        {
            break;
        }
        else if (command == "mkdir" || command == "rmdir" || command == "cd" || command == "ls" ||
                 command == "pwd" || command == "incp" || command == "cat" || command == "info" ||
                 command == "outcp" || command == "rm" || command == "mv")
        {
            // For these commands, read the rest of the line
            std::getline(std::cin, arg);

            // Trim leading whitespace from the argument
            if (!arg.empty())
            {
                arg = arg.substr(arg.find_first_not_of(" \t"));
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
            if (arg.empty())
            {
                fs.listDirectory(currentDirectory); // List current directory if no path is provided
            }
            else
            {
                std::vector<std::string> pathParts = fs.splitPath(arg);
                directory_item *targetDir = nullptr;

                if (arg[0] == '/') // Absolute path
                {
                    targetDir = fs.findDirectoryFromRoot(pathParts);
                }
                else // Relative path
                {
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
                fs.changeDirectory("/"); // Change to root if no path is provided
                currentDirectory = fs.currentDirectory;
            }
            else
            {
                fs.changeDirectory(arg);
                currentDirectory = fs.currentDirectory;
            }
        }
        else if (command == "pwd")
        {
            std::string path = fs.pwd();
            std::cout << "PATH: " << path << std::endl;
        }
        else if (command == "incp")
        {
            std::string srcPath, destPath;
            size_t splitPos = arg.find(' ');

            if (splitPos != std::string::npos)
            {
                srcPath = arg.substr(0, splitPos);
                destPath = arg.substr(splitPos + 1);
                fs.incp(srcPath, destPath);
            }
            else
            {
                std::cerr << "Error: incp command requires two arguments (source and destination)\n";
            }
        }
        else if (command == "cat")
        {
            if (!arg.empty())
            {
                fs.cat(arg);
            }
            else
            {
                std::cerr << "Error: cat command requires a file path.\n";
            }
        }
        else if (command == "info")
        {
            // Handle the `info` command to display cluster information for a file/directory
            if (!arg.empty())
            {
                fs.info(arg);
            }
            else
            {
                std::cerr << "Error: info command requires a path.\n";
            }
        }
        else if (command == "outcp")
        {
            // Handle the `outcp` command for exporting a file from the custom file system to the host system
            std::string srcPath, destPath;
            size_t splitPos = arg.find(' ');

            if (splitPos != std::string::npos)
            {
                // Separate arguments: first part is `s1` (source path in custom FS), second part is `s2` (destination path on host)
                srcPath = arg.substr(0, splitPos);   // Extract source path
                destPath = arg.substr(splitPos + 1); // Extract destination path

                // Perform the outcp operation
                fs.outcp(srcPath, destPath);
            }
            else
            {
                std::cerr << "Error: outcp command requires two arguments (source and destination).\n";
            }
        }
        else if (command == "rm")
        {
            // Handle the `rm` command to delete a file
            if (!arg.empty())
            {
                fs.rm(arg);
            }
            else
            {
                std::cerr << "Error: rm command requires a file path.\n";
            }
        }
        else if (command == "mv")
        {
            // Handle the `mv` command to move or rename a file/directory
            std::string srcPath, destPath;
            size_t splitPos = arg.find(' ');

            if (splitPos != std::string::npos)
            {
                srcPath = arg.substr(0, splitPos);
                destPath = arg.substr(splitPos + 1);

                fs.mv(srcPath, destPath);
            }
            else
            {
                std::cerr << "Error: mv command requires two arguments (source and destination)\n";
            }
        }
        else
        {
            std::cout << "Unknown command\n";
        }
    }

    return 0;
}
