#include <fstream>
#include <iomanip>
#include "util/config.hpp"
#include "util/json.hpp"

namespace app::config {
    std::string lastNetUrl;
    std::string httpIndexUrl;
    std::string themeColorTextTopInfo;
    std::string themeColorTextBottomInfo;
    std::string themeColorTextMenu;
    std::string themeColorTextFile;
    std::string themeColorTextDir;
    std::string themeColorTextInstall;
    int languageSetting;
    int mainMenuItemSize = 149;
    int subMenuItemSize = 76;
    bool fixTicket;
    bool deletePrompt;
    bool enableSound;
    bool enableLightning;
    bool ignoreReqVers;
    bool overClock;
    bool usbAck;
    bool validateNCAs;

    void setConfig() {
        nlohmann::json j = {
            {"fixTicket", fixTicket},
            {"deletePrompt", deletePrompt},
            {"enableSound", enableSound},
            {"enableLightning", enableLightning},
            {"ignoreReqVers", ignoreReqVers},
            {"languageSetting", languageSetting},
            {"overClock", overClock},
            {"usbAck", usbAck},
            {"validateNCAs", validateNCAs},
            {"lastNetUrl", lastNetUrl},
            {"httpIndexUrl", httpIndexUrl}
        };
        std::ofstream file(app::config::configPath);
        file << std::setw(4) << j << std::endl;
    }

    void parseConfig() {
        try {
            std::ifstream file(app::config::configPath);
            nlohmann::json j;
            file >> j;
            fixTicket = j["fixTicket"].get<bool>();
            deletePrompt = j["deletePrompt"].get<bool>();
            enableSound = j["enableSound"].get<bool>();
            enableLightning = j["enableLightning"].get<bool>();
            ignoreReqVers = j["ignoreReqVers"].get<bool>();
            languageSetting = j["languageSetting"].get<int>();
            overClock = j["overClock"].get<bool>();
            usbAck = j["usbAck"].get<bool>();
            validateNCAs = j["validateNCAs"].get<bool>();
            lastNetUrl = j["lastNetUrl"].get<std::string>();
            httpIndexUrl = j["httpIndexUrl"].get<std::string>();
        }
        catch (...) {
            // If loading values from the config fails, we just load the defaults and overwrite the old config
            languageSetting = 99;
            fixTicket = true;
            deletePrompt = false;
            enableSound = true;
            enableLightning = true;
            ignoreReqVers = false;
            overClock = false;
            usbAck = false;
            validateNCAs = true;
            lastNetUrl = "https://";
            httpIndexUrl = "http://";
            setConfig();
        }
    }

    void parseThemeColorConfig() {
        try {
            std::ifstream file(app::config::themecolorPath);
            nlohmann::json j;
            file >> j;
            try {
                themeColorTextTopInfo = j["themeColorTextTopInfo"].get<std::string>();
            }
            catch (...) {
                themeColorTextTopInfo = "#FFFFFFFF";
            }
            try {
                themeColorTextBottomInfo = j["themeColorTextBottomInfo"].get<std::string>();
            }
            catch (...) {
                themeColorTextBottomInfo = "#FFFFFFFF";
            }
            try {
                themeColorTextMenu = j["themeColorTextMenu"].get<std::string>();
            }
            catch (...) {
                themeColorTextMenu = "#FFFFFFFF";
            }
            try {
                themeColorTextFile = j["themeColorTextFile"].get<std::string>();
            }
            catch (...) {
                themeColorTextFile = "#FFFFFFFF";
            }
            try {
                themeColorTextDir = j["themeColorTextDir"].get<std::string>();
            }
            catch (...) {
                themeColorTextDir = "#FFFFFFFF";
            }
            try {
                themeColorTextInstall = j["themeColorTextInstall"].get<std::string>();
            }
            catch (...) {
                themeColorTextInstall = "#FFFFFFFF";
            }
        }
        catch (...) {
            // If themecolor.json is missing, load the defaults
            themeColorTextTopInfo = "#FFFFFFFF";
            themeColorTextBottomInfo = "#FFFFFFFF";
            themeColorTextMenu = "#FFFFFFFF";
            themeColorTextFile = "#FFFFFFFF";
            themeColorTextDir = "#FFFFFFFF";
            themeColorTextInstall = "#FFFFFFFF";
        }
    }
}
