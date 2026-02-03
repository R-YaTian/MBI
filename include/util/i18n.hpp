#pragma once

#include <string>

namespace app::i18n
{
    void Load(int languageCode);
    std::string LanguageEntry(std::string key);
    std::string GetRandomMsg();
}

inline std::string operator ""_lang (const char* key, size_t size) {
    return app::i18n::LanguageEntry(std::string(key, size));
}
