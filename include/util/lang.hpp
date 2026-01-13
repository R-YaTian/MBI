#pragma once

#include <string>
#include <sstream>
#include <fstream>
#include <jtjson.h>

namespace app::i18n
{
    void Load();
    std::string LanguageEntry(std::string key);
    std::string GetRandomMsg();

    inline jt::Json GetRelativeJson(jt::Json j, std::string key)
    {
        std::istringstream ss(key);
        std::string token;
        jt::Json ret = j;

        while (std::getline(ss, token, '.') && !ret.is_null())
        {
            ret = ret[token];
        }

        return ret;
    }
}

inline std::string operator ""_lang (const char* key, size_t size) {
    return app::i18n::LanguageEntry(std::string(key, size));
}
