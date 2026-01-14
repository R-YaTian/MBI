#pragma once

#include <string>
#include <sstream>
#include <jtjson.h>

namespace app::i18n
{
    void Load(int languageCode);
    std::string LanguageEntry(std::string key);
    std::string GetRandomMsg();

    inline jt::Json GetRelativeJson(const jt::Json& j, std::string key)
    {
        std::istringstream ss(key);
        std::string token;
        jt::Json ret = j;

        while (std::getline(ss, token, '.') && !ret.is_string())
        {
            ret = ret[token];
        }

        return ret;
    }
}

inline std::string operator ""_lang (const char* key, size_t size) {
    return app::i18n::LanguageEntry(std::string(key, size));
}
