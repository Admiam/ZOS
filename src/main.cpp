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
    bool isCorrupted = fs.check();
    std::string command, arg;

    // Define currentDirectory
    directory_item *currentDirectory = nullptr;

    // Initialize currentDirectory to point to the root directory
    currentDirectory = fs.getRootDirectory(); // Assuming you have a method to get the root directory

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

        // Read remaining argument for commands
        if (command == "mkdir" || command == "rmdir" || command == "cd" || command == "ls" ||
            command == "pwd" || command == "incp" || command == "cat" || command == "info" ||
            command == "outcp" || command == "rm" || command == "mv" || command == "cp" ||
            command == "load" || command == "format" || command == "check" || command == "bug")
        {
            std::getline(std::cin, arg);
            if (!arg.empty())
            {
                arg = arg.substr(arg.find_first_not_of(" \t"));
            }
        }

        // Limit commands if corrupted
        if (isCorrupted && (command != "check" && command != "format" && command != "exit"))
        {
            std::cerr << "Error: Only 'check', 'format', and 'exit' commands are allowed in corrupted mode.\n";
            continue;
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
            fs.listDirectory(arg); // List current directory if no path is provided
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
            std::cout << "PATH: " << fs.pwd() << std::endl;
        }
        else if (command == "incp")
        {
            // Split the argument into parts
            std::istringstream stream(arg);
            std::vector<std::string> parts;
            std::string part;

            while (stream >> part) // Split by whitespace
            {
                parts.push_back(part);
            }

            // Check if there are exactly two arguments
            if (parts.size() != 2)
            {
                std::cerr << "Error: incp command requires two arguments (source and destination)\n";
            }else{
                // Extract source and destination paths
                std::string srcPath = parts[0];
                std::string destPath = parts[1];

                // Call the `incp` function with validated arguments
                fs.incp(srcPath, destPath);
            }
        }
        else if (command == "cat")
        {
            if (!arg.empty())
            {
                fs.cat(arg);
            }else{
                std::cerr << "Error: cat command requires a file path.\n";
            }
            
        }
        else if (command == "info")
        {
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
            // Split the argument into parts
            std::istringstream stream(arg);
            std::vector<std::string> parts;
            std::string part;

            while (stream >> part) // Split by whitespace
            {
                parts.push_back(part);
            }

            // Check if there are exactly two arguments
            if (parts.size() != 2)
            {
                std::cerr << "Error: outcp command requires two arguments (source and destination)\n";
            }
            else
            {
                // Extract source and destination paths
                std::string srcPath = parts[0];
                std::string destPath = parts[1];

                // Call the `outcp` function with validated arguments
                fs.outcp(srcPath, destPath);
            }
        }
        else if (command == "rm")
        {
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
            // Split the argument into parts
            std::istringstream stream(arg);
            std::vector<std::string> parts;
            std::string part;

            while (stream >> part) // Split by whitespace
            {
                parts.push_back(part);
            }

            // Check if there are exactly two arguments
            if (parts.size() != 2)
            {
                std::cerr << "Error: incp command requires two arguments (source and destination)\n";
            }
            else
            {
                // Extract source and destination paths
                std::string srcPath = parts[0];
                std::string destPath = parts[1];

                // Call the `incp` function with validated arguments
                fs.mv(srcPath, destPath);
            }
        }
        else if (command == "cp")
        {
            // Split the argument into parts
            std::istringstream stream(arg);
            std::vector<std::string> parts;
            std::string part;

            while (stream >> part) // Split by whitespace
            {
                parts.push_back(part);
            }

            // Check if there are exactly two arguments
            if (parts.size() != 2)
            {
                std::cerr << "Error: incp command requires two arguments (source and destination)\n";
            }
            else
            {
                // Extract source and destination paths
                std::string srcPath = parts[0];
                std::string destPath = parts[1];

                // Call the `incp` function with validated arguments
                fs.cp(srcPath, destPath);
            }
        }
        else if (command == "format")
        {
            if (!arg.empty())
            {
                if (fs.formatDisk(arg))
                {
                    isCorrupted = false;
                    std::cout << "OK\n";
                }
                else
                {
                    std::cout << "CANNOT CREATE FILE\n";
                }
            }
            else
            {
                std::cerr << "Error: format command requires a size argument.\n";
            }
        }
        else if (command == "check")
        {
            fs.check();
        }
        else if (command == "bug")
        {
            if (!arg.empty())
            {
                fs.bug(arg);
                isCorrupted = true;
                std::cout << "OK\n";
            }
            else
            {
                std::cerr << "Error: bug command requires a file path.\n";
            }
        }
        else
        {
            std::cout << "Unknown command\n";
        }
    }

    return 0;
}
