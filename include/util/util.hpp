#pragma once

#include <string>
#include <vector>
#include <filesystem>

namespace app::util
{
    bool ignoreCaseCompare(const std::string &a, const std::string &b);
    std::vector<std::filesystem::path> getDirectoryFiles(const std::string & dir, const std::vector<std::string> & extensions);
    std::vector<std::filesystem::path> getDirsAtPath(const std::string & dir);
    std::string getUrlHost(const std::string &url);
    std::string shortenString(std::string ourString, int ourLength, bool isFile);
}
