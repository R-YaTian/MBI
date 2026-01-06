#pragma once

#include <vector>

namespace app::config {
    static const std::string appDir = "sdmc:/config/MBI";
    static const std::string configPath = appDir + "/config.json";
    static const std::string themecolorPath = appDir + "/themecolor.json";
    static const std::string appVersion = APPVER;

    extern std::string lastNetUrl;
    extern std::string httpIndexUrl;
    extern std::string themeColorTextTopInfo;
    extern std::string themeColorTextBottomInfo;
    extern std::string themeColorTextMenu;
    extern std::string themeColorTextFile;
    extern std::string themeColorTextDir;
    extern std::string themeColorTextInstall;
    extern int languageSetting;
    extern int mainMenuItemSize;
    extern int subMenuItemSize;
    extern bool ignoreReqVers;
    extern bool validateNCAs;
    extern bool overClock;
    extern bool deletePrompt;
    extern bool enableSound;
    extern bool enableLightning;
    extern bool usbAck;
    extern bool fixTicket;

    void setConfig();
    void parseConfig();
    void parseThemeColorConfig();
}
