#include "nx/misc.hpp"
#include <time.h>
#include <switch.h>

namespace nx::misc
{
    std::string OpenSoftwareKeyboard(std::string guideText, std::string initialText, int LenMax)
    {
        Result rc = 0;
        SwkbdConfig kbd;
        char tmpoutstr[LenMax + 1] = {0};
        rc = swkbdCreate(&kbd, 0);
        if (R_SUCCEEDED(rc))
        {
            swkbdConfigMakePresetDefault(&kbd);
            swkbdConfigSetGuideText(&kbd, guideText.c_str());
            swkbdConfigSetInitialText(&kbd, initialText.c_str());
            swkbdConfigSetStringLenMax(&kbd, LenMax);
            rc = swkbdShow(&kbd, tmpoutstr, sizeof(tmpoutstr));
            swkbdClose(&kbd);
            if (R_SUCCEEDED(rc) && tmpoutstr[0] != 0)
            {
                return std::string(tmpoutstr);
            }
        }
        return "";
    }

    uint32_t SetClockSpeed(int deviceToClock, uint32_t clockSpeed)
    {
        uint32_t hz = 0;
        uint32_t previousHz = 0;

        if (deviceToClock > 2 || deviceToClock < 0)
        {
            return 0;
        }

        if(hosversionAtLeast(8,0,0))
        {
            ClkrstSession session = {0};
            PcvModuleId pcvModuleId;
            pcvInitialize();
            clkrstInitialize();

            switch (deviceToClock)
            {
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

            return previousHz;
        }
        else
        {
            PcvModule pcvModule;
            pcvInitialize();

            switch (deviceToClock)
            {
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

            return previousHz;
        }
    }

    u32 GetBatteryValue()
    {
        u32 value = 255;
        Result rc = psmInitialize();

        if (R_SUCCEEDED(rc))
        {
            u32 tmp;
            rc = psmGetBatteryChargePercentage(&tmp);
            if (R_SUCCEEDED(rc))
            {
                value = tmp;
            }
            psmExit();
        }

        return value;
    }

    std::string GetBatteryColor(u32 batteryValue)
    {
        if (batteryValue <= 15)
        {
            return "#FF0000FF"; // red
        }
        else if (batteryValue <= 30)
        {
            return "#FF8000FF"; // orange
        }
        else if (batteryValue <= 50)
        {
            return "#FFFF00FF"; // yellow
        }
        else
        {
            return "#00FF00FF"; // green
        }
    }

    std::string GetCurrentDate()
    {
        const auto posix_time = time(nullptr);
        const auto local_time = localtime(&posix_time);

        char date_str[0x20] = {};
        snprintf(date_str, sizeof(date_str), "%04d/%02d/%02d", local_time->tm_year + 1900, local_time->tm_mon + 1, local_time->tm_mday);
        return date_str;
    }

    std::string GetCurrentTime(const bool use_12h_time)
    {
        const auto posix_time = time(nullptr);
        const auto local_time = localtime(&posix_time);

        char time_str[0x20] = {};
        if (use_12h_time)
        {
            auto hour = local_time->tm_hour;
            if (hour > 12)
            {
                hour -= 12;
            }
            else if (hour == 0)
            {
                hour = 12;
            }

            const auto ampm_str = (local_time->tm_hour >= 12) ? "PM" : "AM";
            snprintf(time_str, sizeof(time_str), "%02d:%02d:%02d %s", hour, local_time->tm_min, local_time->tm_sec, ampm_str);
        }
        else
        {
            snprintf(time_str, sizeof(time_str), "%02d:%02d:%02d", local_time->tm_hour, local_time->tm_min, local_time->tm_sec);
        }

        return time_str;
    }
}
