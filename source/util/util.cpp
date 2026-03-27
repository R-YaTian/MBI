#include "util/util.hpp"
#include <time.h>

namespace app::util
{
    bool IgnoreCaseCompare(const std::string &a, const std::string &b)
    {
        const auto caseInsensitiveLess = [](auto &x, auto &y) -> bool
        {
            return toupper(static_cast<unsigned char>(x)) < toupper(static_cast<unsigned char>(y));
        };

        return std::lexicographical_compare(a.begin(), a.end(), b.begin(), b.end(), caseInsensitiveLess);
    }

    std::string GetUrlHost(const std::string &url)
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

    std::string GetCurrentDate()
    {
        const auto posix_time = time(nullptr);
        const auto local_time = localtime(&posix_time);

        char date_str[0x20] = {};
        snprintf(date_str, sizeof(date_str), "%04d/%02d/%02d", local_time->tm_year + 1900, local_time->tm_mon + 1, local_time->tm_mday);
        return date_str;
    }

    std::string GetCurrentTime(const bool use_12h_time)
    {
        const auto posix_time = time(nullptr);
        const auto local_time = localtime(&posix_time);

        char time_str[0x20] = {};
        if (use_12h_time)
        {
            auto hour = local_time->tm_hour;
            if (hour > 12)
            {
                hour -= 12;
            }
            else if (hour == 0)
            {
                hour = 12;
            }

            const auto ampm_str = (local_time->tm_hour >= 12) ? "PM" : "AM";
            snprintf(time_str, sizeof(time_str), "%02d:%02d:%02d %s", hour, local_time->tm_min, local_time->tm_sec, ampm_str);
        }
        else
        {
            snprintf(time_str, sizeof(time_str), "%02d:%02d:%02d", local_time->tm_hour, local_time->tm_min, local_time->tm_sec);
        }

        return time_str;
    }
}
