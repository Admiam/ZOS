#include <iostream>
#include "FileSystem.h"
#include "utils.h"
int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        std::cerr << "Usage: " << argv[0] << " <filesystem name>\n";
        return 1;
    }

    std::string filesystemName = argv[1];

    // Check if the filename ends with .dat
    if (!hasDatExtension(filesystemName))
    {
        std::cerr << "Error: File name must have a .dat extension\n";
        return 1;
    }

    FileSystem fs(filesystemName);
    std::string command;

    // Infinite loop to handle user input
    while (true)
    {
        fs.displayPrompt();
        std::getline(std::cin, command);

        if (command.empty())
        {
            continue; // Skip to the next iteration
        }
        
        if (command == "exit")
        {
            fs.exit();
            break; // Exit the loop
        }
        else if (command.find("format") == 0)
        {
            // Parse the command to get the size (e.g., "format 600MB")
            size_t size = 0;
            if (command.find("MB") != std::string::npos)
            {
                size_t mbSize = std::stoi(command.substr(7, command.size() - 9));
                size = mbSize * 1024 * 1024; // Convert MB to bytes
            }
            else
            {
                std::cerr << "Invalid size format\n";
                continue;
            }

            // Call the format function
            fs.format(size);
        }
        else if (command.find("start") == 0)
        {
            // Call the start function if the file exists
            if (fileExists(filesystemName))
            {
                fs.start();
            }
            else
            {
                std::cerr << "File system not found.\n";
            }
        }
        else
        {
            std::cerr << "Unknown command. Please use 'format', 'start', or 'exit'.\n";
        }
    }

    return 0;
}
