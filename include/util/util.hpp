#pragma once

#include <string>

namespace app::util
{
    bool ignoreCaseCompare(const std::string &a, const std::string &b);
    std::string getUrlHost(const std::string &url);
}
