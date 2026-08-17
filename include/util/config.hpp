#pragma once

#include <string>

namespace app
{
    namespace config
    {
        inline const std::string storagePath = "sdmc:/config/MBI";
        inline const std::string settingsFile = storagePath + "/config.json";
        inline const std::string themecolorFile = storagePath + "/color.json";

        extern std::string lastNetUrl;
        extern std::string httpIndexUrl;
        extern std::string TopInfoTextColor;
        extern std::string BottomInfoTextColor;
        extern std::string MenuTextColor;
        extern std::string FileTextColor;
        extern std::string DirTextColor;
        extern std::string InstallerInfoTextColor;
        extern int languageSetting;
        extern int mtpInstallTargetStorage;
        extern bool ignoreReqVers;
        extern bool overClock;
        extern bool deletePrompt;
        extern bool enableSound;
        extern bool fixTicket;
        extern bool skipBase;
        extern bool usbAck;
        extern bool appletAck;
        extern bool use12hTime;

        void SaveSettings();
        void ParseSettings();
        void ParseThemeColor();

        const double GetScreenScaleFactor();
        void SetScreenScaleFactor(const double& scaleFactor);

        const int GetMainMenuItemSize();
        const int GetMainMenuHeight();
        const int GetSubMenuItemSize();
        const int GetSubMenuHeight();
    }

    inline int operator ""_dp(unsigned long long value)
    {
        return static_cast<int>(value / config::GetScreenScaleFactor());
    }
}
