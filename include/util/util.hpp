#pragma once

#include <string>

namespace app::util
{
    bool IgnoreCaseCompare(const std::string &a, const std::string &b);
    std::string GetUrlHost(const std::string &url);
    std::string GetCurrentDate();
    std::string GetCurrentTime(const bool use_12h_time);
}
