#include <algorithm>
#include <fstream>
#include <unistd.h>
#include <regex>

#include <switch.h>
#include <switch-ipcext.h>

#include "util/util.hpp"
#include "util/config.hpp"
#include "ui/MainApplication.hpp"
#include "nx/usb.hpp"
#include "nx/udisk.hpp"
#include "nx/error.hpp"

namespace app::util
{
    auto caseInsensitiveLess = [](auto& x, auto& y)->bool {
        return toupper(static_cast<unsigned char>(x)) < toupper(static_cast<unsigned char>(y));
    };

    bool ignoreCaseCompare(const std::string &a, const std::string &b) {
        return std::lexicographical_compare(a.begin(), a.end() , b.begin() ,b.end() , caseInsensitiveLess);
    }

    std::vector<std::filesystem::path> getDirectoryFiles(const std::string & dir, const std::vector<std::string> & extensions) {
        std::vector<std::filesystem::path> files;
        for(auto & p: std::filesystem::directory_iterator(dir))
        {
            try {
            if (std::filesystem::is_regular_file(p))
            {
                std::string ourExtension = p.path().extension().string();
                std::transform(ourExtension.begin(), ourExtension.end(), ourExtension.begin(), ::tolower);
                if (extensions.empty() || std::find(extensions.begin(), extensions.end(), ourExtension) != extensions.end())
                {
                    files.push_back(p.path());
                }
            }
         } catch (std::filesystem::filesystem_error & e) {}
        }
        std::sort(files.begin(), files.end(), ignoreCaseCompare);
        return files;
    }

    std::vector<std::filesystem::path> getDirsAtPath(const std::string & dir) {
        std::vector<std::filesystem::path> files;
        for(auto & p: std::filesystem::directory_iterator(dir))
        {
         try {
            if (std::filesystem::is_directory(p))
            {
                    files.push_back(p.path());
            }
         } catch (std::filesystem::filesystem_error & e) {}
        }
        std::sort(files.begin(), files.end(), ignoreCaseCompare);
        return files;
    }

    std::string getUrlHost(const std::string &url)
    {
        std::string::size_type pos = url.find('/');
        if (pos != std::string::npos)
            return url.substr(0, pos);
        else
            return url;
    }

    std::string shortenString(std::string ourString, int ourLength, bool isFile) {
        std::filesystem::path ourStringAsAPath = ourString;
        std::string ourExtension = ourStringAsAPath.extension().string();
        if (ourString.size() - ourExtension.size() > (unsigned long)ourLength) {
            if(isFile) return (std::string)ourString.substr(0,ourLength) + "(...)" + ourExtension;
            else return (std::string)ourString.substr(0,ourLength) + "...";
        } else return ourString;
    }

    std::string softwareKeyboard(std::string guideText, std::string initialText, int LenMax) {
        Result rc=0;
        SwkbdConfig kbd;
        char tmpoutstr[LenMax + 1] = {0};
        rc = swkbdCreate(&kbd, 0);
        if (R_SUCCEEDED(rc)) {
            swkbdConfigMakePresetDefault(&kbd);
            swkbdConfigSetGuideText(&kbd, guideText.c_str());
            swkbdConfigSetInitialText(&kbd, initialText.c_str());
            swkbdConfigSetStringLenMax(&kbd, LenMax);
            rc = swkbdShow(&kbd, tmpoutstr, sizeof(tmpoutstr));
            swkbdClose(&kbd);
            if (R_SUCCEEDED(rc) && tmpoutstr[0] != 0) return(((std::string)(tmpoutstr)));
        }
        return "";
    }

    std::vector<uint32_t> setClockSpeed(int deviceToClock, uint32_t clockSpeed) {
        uint32_t hz = 0;
        uint32_t previousHz = 0;

        if (deviceToClock > 2 || deviceToClock < 0) return {0,0};

        if(hosversionAtLeast(8,0,0)) {
            ClkrstSession session = {0};
            PcvModuleId pcvModuleId;
            pcvInitialize();
            clkrstInitialize();

            switch (deviceToClock) {
                case 0:
                    pcvGetModuleId(&pcvModuleId, PcvModule_CpuBus);
                    break;
                case 1:
                    pcvGetModuleId(&pcvModuleId, PcvModule_GPU);
                    break;
                case 2:
                    pcvGetModuleId(&pcvModuleId, PcvModule_EMC);
                    break;
            }

            clkrstOpenSession(&session, pcvModuleId, 3);
            clkrstGetClockRate(&session, &previousHz);
            clkrstSetClockRate(&session, clockSpeed);
            clkrstGetClockRate(&session, &hz);

            pcvExit();
            clkrstCloseSession(&session);
            clkrstExit();

            return {previousHz, hz};
        } else {
            PcvModule pcvModule;
            pcvInitialize();

            switch (deviceToClock) {
                case 0:
                    pcvModule = PcvModule_CpuBus;
                    break;
                case 1:
                    pcvModule = PcvModule_GPU;
                    break;
                case 2:
                    pcvModule = PcvModule_EMC;
                    break;
            }

            pcvGetClockRate(pcvModule, &previousHz);
            pcvSetClockRate(pcvModule, clockSpeed);
            pcvGetClockRate(pcvModule, &hz);

            pcvExit();

            return {previousHz, hz};
        }
    }

    



    std::string* getBatteryCharge() {
        std::string batColBlue = "#0000FFFF";
        std::string batColGreen = "#00FF00FF";
        std::string batColYellow = "#FFFF00FF";
        std::string batColOrange = "#FF8000FF";
        std::string batColRed = "#FF0000FF";
        std::string* batValue = new std::string[2];
        batValue[0] = "???";
        batValue[1] = batColBlue;
        u32 charge;

        Result rc = psmInitialize();
        if (!R_FAILED(rc)) {
            rc = psmGetBatteryChargePercentage(&charge);
            if (!R_FAILED(rc)) {
            if (charge < 15.0) {
                batValue[1] = batColRed;
            } else if (charge < 30.0) {
                batValue[1] = batColOrange;
            } else if (charge < 50.0) {
                batValue[1] = batColYellow;
            } else {
                batValue[1] = batColGreen;
            }
                batValue[0] = std::to_string(charge) + "%";
            }
        }
        psmExit();
        return batValue;
    }
}
