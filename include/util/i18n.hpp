#pragma once

#include <string>

namespace app
{
    namespace i18n
    {
        int Load(int index);
        std::string LanguageEntry(std::string key);
        std::string GetRandomMsg();
        std::string GetRelativeMsgAt(const std::string& key, size_t index);
    }

    inline std::string operator ""_lang(const char* key, size_t size)
    {
        return i18n::LanguageEntry(std::string(key, size));
    }
}
