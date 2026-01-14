#pragma once

#include <string>

namespace app::config
{
    inline const std::string storagePath = "sdmc:/config/MBI";
    inline const std::string settingsFile = storagePath + "/config.json";
    inline const std::string themecolorFile = storagePath + "/color.json";
    inline const std::string appVersion = APPVER;
    inline constexpr auto mainMenuItemSize = 149;
    inline constexpr auto subMenuItemSize = 76;

    extern std::string lastNetUrl;
    extern std::string httpIndexUrl;
    extern std::string TopInfoTextColor;
    extern std::string BottomInfoTextColor;
    extern std::string MenuTextColor;
    extern std::string FileTextColor;
    extern std::string DirTextColor;
    extern std::string InstallerInfoTextColor;
    extern int languageSetting;
    extern bool ignoreReqVers;
    extern bool validateNCAs;
    extern bool overClock;
    extern bool deletePrompt;
    extern bool enableSound;
    extern bool enableLightning;
    extern bool usbAck;
    extern bool fixTicket;

    void SaveSettings();
    void ParseSettings();
    void ParseThemeColor();
}
