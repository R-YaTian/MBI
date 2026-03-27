#include "util/util.hpp"

namespace app::util
{
    static auto caseInsensitiveLess = [](auto &x, auto &y) -> bool
    {
        return toupper(static_cast<unsigned char>(x)) < toupper(static_cast<unsigned char>(y));
    };

    bool ignoreCaseCompare(const std::string &a, const std::string &b)
    {
        return std::lexicographical_compare(a.begin(), a.end(), b.begin(), b.end(), caseInsensitiveLess);
    }

    std::string getUrlHost(const std::string &url)
    {
        std::string::size_type pos = url.find('/');
        if (pos != std::string::npos)
        {
            return url.substr(0, pos);
        }
        else
        {
            return url;
        }
    }
}
