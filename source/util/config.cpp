#include <jtjson.h>
#include "util/config.hpp"

namespace app::config
{
    double ScreenScaleFactor = 1.0f;
    constexpr int mainMenuItemSize = 112;
    constexpr int mainMenuHeight = 896;
    constexpr int subMenuItemSize = 76;
    constexpr int subMenuHeight = 836;

    std::string lastNetUrl;
    std::string httpIndexUrl;
    std::string TopInfoTextColor;
    std::string BottomInfoTextColor;
    std::string MenuTextColor;
    std::string FileTextColor;
    std::string DirTextColor;
    std::string InstallerInfoTextColor;
    int languageSetting;
    int mtpInstallTargetStorage;
    bool fixTicket;
    bool skipBase;
    bool deletePrompt;
    bool enableSound;
    bool ignoreReqVers;
    bool overClock;
    bool usbAck;
    bool appletAck;
    bool use12hTime;

    void SaveSettings()
    {
        jt::Json j;
        j["fixTicket"] = fixTicket;
        j["skipBase"] = skipBase;
        j["deletePrompt"] = deletePrompt;
        j["enableSound"] = enableSound;
        j["ignoreReqVers"] = ignoreReqVers;
        j["languageSetting"] = languageSetting;
        j["mtpInstallTargetStorage"] = mtpInstallTargetStorage;
        j["overClock"] = overClock;
        j["usbAck"] = usbAck;
        j["appletAck"] = appletAck;
        j["lastNetUrl"] = lastNetUrl;
        j["httpIndexUrl"] = httpIndexUrl;
        j["use12hTime"] = use12hTime;
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

        fixTicket = j.value("fixTicket", true);
        skipBase = j.value("skipBase", true);
        deletePrompt = j.value("deletePrompt", true);
        enableSound = j.value("enableSound", true);
        ignoreReqVers = j.value("ignoreReqVers", false);
        languageSetting = j.value("languageSetting", -1);
        mtpInstallTargetStorage = j.value("mtpInstallTargetStorage", 1);
        overClock = j.value("overClock", false);
        usbAck = j.value("usbAck", false);
        appletAck = j.value("appletAck", false);
        lastNetUrl = j.value("lastNetUrl", std::string("https://"));
        httpIndexUrl = j.value("httpIndexUrl", std::string("http://"));
        use12hTime = j.value("use12hTime", false);

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

    const double GetScreenScaleFactor()
    {
        return ScreenScaleFactor;
    }

    void SetScreenScaleFactor(const double& scaleFactor)
    {
        ScreenScaleFactor = scaleFactor;
    }

    const int GetMainMenuItemSize()
    {
        return static_cast<int>(mainMenuItemSize / GetScreenScaleFactor());
    }

    const int GetMainMenuHeight()
    {
        return static_cast<int>(mainMenuHeight / GetScreenScaleFactor());
    }

    const int GetSubMenuItemSize()
    {
        return static_cast<int>(subMenuItemSize / GetScreenScaleFactor());
    }

    const int GetSubMenuHeight()
    {
        return static_cast<int>(subMenuHeight / GetScreenScaleFactor());
    }
}
