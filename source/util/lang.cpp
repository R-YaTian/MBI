#include <switch.h>

#include "nx/error.hpp"
#include "util/lang.hpp"
#include "util/config.hpp"

namespace app::i18n
{
    jt::Json lang;

    void Load()
    {
        FILE *fp;
        std::string languagePath;
        int langInt = app::config::languageSetting;
        if (langInt == -1)
        {
            SetLanguage ourLang;
            u64 lcode = 0;
            setInitialize();
            setGetSystemLanguage(&lcode);
            setMakeLanguage(lcode, &ourLang);
            setExit();
            langInt = (int)ourLang;
        }

        switch (langInt)
        {
            case 0:
                languagePath = "romfs:/lang/jp.json";
                break;
            case 2:
            case 13:
                languagePath = "romfs:/lang/fr.json";
                break;
            case 3:
                languagePath = "romfs:/lang/de.json";
                break;
            case 4:
                languagePath = "romfs:/lang/it.json";
                break;
            case 5:
            case 14:
                languagePath = "romfs:/lang/es-419.json";
                break;
            case 6:
            case 15:
                languagePath = "romfs:/lang/zh-Hans.json";
                break;
            case 7:
                languagePath = "romfs:/lang/ko-KR.json";
                break;
            case 8:
                languagePath = "romfs:/lang/nl.json";
                break;
            case 9:
            case 17:
                languagePath = "romfs:/lang/pt.json";
                break;
            case 10:
                languagePath = "romfs:/lang/ru.json";
                break;
            case 11:
            case 16:
                languagePath = "romfs:/lang/zh-Hant.json";
                break;
            default:
                languagePath = "romfs:/lang/en.json";
        }

        fp = fopen(languagePath.c_str(), "rb");
        if (!fp)
        {
            LOG_DEBUG("FAILED TO LOAD LANGUAGE FILE\n");
            return;
        }
        lang = jt::Json::parse(fp);
        fclose(fp);
    }

    std::string LanguageEntry(std::string key)
    {
        jt::Json j = GetRelativeJson(lang, key);
        if (j.is_null())
        {
            return "Missing key: " + key;
        }
        return j.get<std::string>();
    }

    std::string GetRandomMsg()
    {
        jt::Json j = app::i18n::GetRelativeJson(lang, "inst.finished");
        srand(time(NULL));
        return j[rand() % j.getArray().size()];
    }
}
