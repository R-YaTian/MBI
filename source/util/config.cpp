#include <jtjson.h>
#include "util/config.hpp"

namespace app::config
{
    std::string lastNetUrl;
    std::string httpIndexUrl;
    std::string TopInfoTextColor;
    std::string BottomInfoTextColor;
    std::string MenuTextColor;
    std::string FileTextColor;
    std::string DirTextColor;
    std::string InstallerInfoTextColor;
    int languageSetting;
    bool fixTicket;
    bool deletePrompt;
    bool enableSound;
    bool enableLightning;
    bool ignoreReqVers;
    bool overClock;
    bool usbAck;

    void SaveSettings()
    {
        jt::Json j;
        j["fixTicket"] = fixTicket;
        j["deletePrompt"] = deletePrompt;
        j["enableSound"] = enableSound;
        j["enableLightning"] = enableLightning;
        j["ignoreReqVers"] = ignoreReqVers;
        j["languageSetting"] = languageSetting;
        j["overClock"] = overClock;
        j["usbAck"] = usbAck;
        j["lastNetUrl"] = lastNetUrl;
        j["httpIndexUrl"] = httpIndexUrl;
        auto json_str = j.dump(2);

        FILE *fpOut = fopen(settingsFile.c_str(), "w");
        fputs(json_str.c_str(), fpOut);
        fclose(fpOut);
    }

    void ParseSettings()
    {
        FILE *fp;
        jt::Json j;
        j.setObject();

        fp = fopen(settingsFile.c_str(), "r");
        if (fp)
        {
            j = jt::Json::parse(fp);
        }

        fixTicket = j.value("fixTicket", false);
        deletePrompt = j.value("deletePrompt", false);
        enableSound = j.value("enableSound", true);
        enableLightning = j.value("enableLightning", true);
        ignoreReqVers = j.value("ignoreReqVers", false);
        languageSetting = j.value("languageSetting", -1);
        overClock = j.value("overClock", false);
        usbAck = j.value("usbAck", false);
        lastNetUrl = j.value("lastNetUrl", std::string("https://"));
        httpIndexUrl = j.value("httpIndexUrl", std::string("http://"));

        if (!fp)
        {
            SaveSettings();
        }
        else
        {
            fclose(fp);
        }
    }

    void ParseThemeColor()
    {
        FILE *fp;
        jt::Json j;
        j.setObject();

        fp = fopen(themecolorFile.c_str(), "r");
        if (fp)
        {
            j = jt::Json::parse(fp);
            fclose(fp);
        }

        TopInfoTextColor = j.value("TopInfoTextColor", std::string("#FFFFFFFF"));
        BottomInfoTextColor = j.value("BottomInfoTextColor", std::string("#FFFFFFFF"));
        MenuTextColor = j.value("MenuTextColor", std::string("#FFFFFFFF"));
        FileTextColor = j.value("FileTextColor", std::string("#FFFFFFFF"));
        DirTextColor = j.value("DirTextColor", std::string("#FFFFFFFF"));
        InstallerInfoTextColor = j.value("InstallerInfoTextColor", std::string("#FFFFFFFF"));
    }
}
