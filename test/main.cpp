#include <iostream>
#include <string>
#include "PseudoFAT-test.h"

class FileSystem
{
private:
    PseudoFAT fat;
    std::string filename;

public:
    FileSystem(const std::string &file) : filename(file), fat(file) {}

    void runCommand(const std::string &command)
    {
        if (command == "ls")
        {
            fat.listDirectories();
        }
        else if (command.find("mkdir ") == 0)
        {
            std::string dirname = command.substr(6); // Extract directory name
            if (!fat.createDirectory(dirname))
            {
                std::cerr << "Failed to create directory.\n";
            }
        }
        else
        {
            std::cerr << "Unknown command: " << command << '\n';
        }
    }
};

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        std::cerr << "Usage: ./main filesystem.dat\n";
        return 1;
    }

    std::string filename = argv[1];
    FileSystem fs(filename);

    std::string command;
    std::cout << "PseudoFAT File System\n";
    std::cout << "Type 'mkdir <directory_name>' to create a directory, 'ls' to list directories, or 'exit' to quit.\n";

    while (true)
    {
        std::cout << "> ";
        std::getline(std::cin, command);

        if (command == "exit")
        {
            std::cout << "Exiting...\n";
            break;
        }

        fs.runCommand(command);
    }

    return 0;
}
