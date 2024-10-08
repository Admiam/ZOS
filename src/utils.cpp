#include "utils.h"
#include <sys/stat.h> // For checking file existence
#include <set>
#include <unordered_map>
#include <iostream>

bool hasDatExtension(const std::string &filename)
{
    return filename.length() >= 4 && filename.substr(filename.length() - 4) == ".dat";
}

bool fileExists(const std::string &filename)
{
    struct stat buffer;
    return (stat(filename.c_str(), &buffer) == 0);
}
