#include "util/util.hpp"

namespace app::util
{
    auto caseInsensitiveLess = [](auto& x, auto& y)->bool {
        return toupper(static_cast<unsigned char>(x)) < toupper(static_cast<unsigned char>(y));
    };

    bool ignoreCaseCompare(const std::string &a, const std::string &b) {
        return std::lexicographical_compare(a.begin(), a.end() , b.begin() ,b.end() , caseInsensitiveLess);
    }

    std::vector<std::filesystem::path> getDirectoryFiles(const std::string & dir, const std::vector<std::string> & extensions) {
        std::vector<std::filesystem::path> files;
        for(auto & p: std::filesystem::directory_iterator(dir))
        {
            try {
            if (std::filesystem::is_regular_file(p))
            {
                std::string ourExtension = p.path().extension().string();
                std::transform(ourExtension.begin(), ourExtension.end(), ourExtension.begin(), ::tolower);
                if (extensions.empty() || std::find(extensions.begin(), extensions.end(), ourExtension) != extensions.end())
                {
                    files.push_back(p.path());
                }
            }
         } catch (std::filesystem::filesystem_error & e) {}
        }
        std::sort(files.begin(), files.end(), ignoreCaseCompare);
        return files;
    }

    std::vector<std::filesystem::path> getDirsAtPath(const std::string & dir) {
        std::vector<std::filesystem::path> files;
        for(auto & p: std::filesystem::directory_iterator(dir))
        {
         try {
            if (std::filesystem::is_directory(p))
            {
                    files.push_back(p.path());
            }
         } catch (std::filesystem::filesystem_error & e) {}
        }
        std::sort(files.begin(), files.end(), ignoreCaseCompare);
        return files;
    }

    std::string getUrlHost(const std::string &url)
    {
        std::string::size_type pos = url.find('/');
        if (pos != std::string::npos)
            return url.substr(0, pos);
        else
            return url;
    }

    std::string shortenString(std::string ourString, int ourLength, bool isFile) {
        std::filesystem::path ourStringAsAPath = ourString;
        std::string ourExtension = ourStringAsAPath.extension().string();
        if (ourString.size() - ourExtension.size() > (unsigned long)ourLength) {
            if(isFile) return (std::string)ourString.substr(0,ourLength) + "(...)" + ourExtension;
            else return (std::string)ourString.substr(0,ourLength) + "...";
        } else return ourString;
    }
}
