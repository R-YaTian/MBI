#include "nx/misc.hpp"
#include "nx/error.hpp"
#include <switch.h>
#include <map>

namespace nx::misc
{
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

    void SetBoostMode(bool enable)
    {
        static u32 previousCPU = 0;
        static u32 previousGPU = 0;
        static u32 previousEMC = 0;
        if (hosversionAtLeast(8,0,0))
        {
            Result rc = appletSetCpuBoostMode(enable ? ApmCpuBoostMode_FastLoad : ApmCpuBoostMode_Normal);
            if (R_FAILED(rc))
            {
                THROW_FORMAT("appletSetCpuBoostMode failed: 0x%x", rc);
            }
        }
        else
        {
            pcvInitialize();
            if (enable)
            {
                pcvGetClockRate(PcvModule_CpuBus, &previousCPU);
                pcvGetClockRate(PcvModule_GPU, &previousGPU);
                pcvGetClockRate(PcvModule_EMC, &previousEMC);
                pcvSetClockRate(PcvModule_CpuBus, 1785000000);
                pcvSetClockRate(PcvModule_GPU, 76800000);
                pcvSetClockRate(PcvModule_EMC, 1600000000);
            }
            else
            {
                if (previousCPU != 0)
                {
                    pcvSetClockRate(PcvModule_CpuBus, previousCPU);
                    previousCPU = 0;
                }
                if (previousGPU != 0)
                {
                    pcvSetClockRate(PcvModule_GPU, previousGPU);
                    previousGPU = 0;
                }
                if (previousEMC != 0)
                {
                    pcvSetClockRate(PcvModule_EMC, previousEMC);
                    previousEMC = 0;
                }
            }
            pcvExit();
        }
    }

    void AttemptForceReboot()
    {
        Result rc = spsmInitialize();
        if (R_FAILED(rc))
        {
            return;
        }
        else
        {
            spsmShutdown(true);
            spsmExit();
        }
    }

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

    std::string UTF16toUTF8(const std::u16string& src)
    {
        ssize_t units = 0;
        units = utf16_to_utf8(nullptr, reinterpret_cast<const uint16_t*>(src.c_str()), 0);
        if (units <= 0)
        {
            return "";
        }

        std::string dst(units, '\0');
        units = utf16_to_utf8(reinterpret_cast<uint8_t*>(&dst[0]), reinterpret_cast<const uint16_t*>(src.c_str()), dst.size());
        if (units <= 0)
        {
            return "";
        }

        return dst;
    }

    std::u16string UTF8toUTF16(const std::string& src)
    {
        ssize_t units = 0;
        units = utf8_to_utf16(nullptr, reinterpret_cast<const uint8_t*>(src.c_str()), 0);
        if (units <= 0)
        {
            return u"";
        }

        std::u16string dst(units, '\0');
        units = utf8_to_utf16(reinterpret_cast<uint16_t*>(&dst[0]), reinterpret_cast<const uint8_t*>(src.c_str()), dst.size());
        if (units <= 0)
        {
            return u"";
        }

        return dst;
    }

    std::string ShortenString(const std::string& in, size_t maxLength, size_t preserve_tail_length, const std::string& marker)
    {
        const size_t marker_u16_len = UTF8toUTF16(marker).size();
        if (maxLength <= marker_u16_len)
        {
            return in;
        }

        std::u16string in_utf16 = UTF8toUTF16(in);
        size_t units = in_utf16.size();
        if (units > preserve_tail_length && units - preserve_tail_length > maxLength)
        {
            std::u16string shortened = in_utf16.substr(0, maxLength - marker_u16_len);
            if (preserve_tail_length > 0)
            {
                std::u16string tail = in_utf16.substr(units - preserve_tail_length);
                return UTF16toUTF8(shortened) + marker + UTF16toUTF8(tail);
            }
            return UTF16toUTF8(shortened) + marker;
        }
        else if (in.size() > preserve_tail_length && in.size() - preserve_tail_length > maxLength)
        {
            std::string shortened = in.substr(0, maxLength - marker.size());
            if (preserve_tail_length > 0)
            {
                std::string tail = in.substr(in.size() - preserve_tail_length);
                return shortened + marker + tail;
            }
            return shortened + marker;
        }
        else
        {
            return in;
        }
    }

    const std::string GetLocale()
    {
        u64 languageCode = 0;
        setInitialize();
        setGetSystemLanguage(&languageCode);
        setExit();
        return std::string(reinterpret_cast<char*>(&languageCode));
    }

    const std::string GetTimeZone()
    {
        TimeLocationName tl;
        setsysInitialize();
        setsysGetDeviceTimeZoneLocationName(&tl);
        setsysExit();
        return tl.name;
    }

    const static std::map<std::string, std::string> regionMap = {
        {"ja", "JP"},
        {"en-US", "US"},
        {"fr", "FR"},
        {"de", "DE"},
        {"it", "IT"},
        {"es", "ES"},
        {"zh-CN", "CN"},
        {"ko", "KR"},
        {"nl", "NL"},
        {"pt", "PT"},
        {"ru", "RU"},
        {"zh-TW", "TW"}, // Taiwan, Province of China
        {"en-GB", "GB"},
        {"fr-CA", "CA"},
        {"es-419", "MX"}, // Simply using MX for Latin America region...
        {"zh-Hans", "CN"},
        {"zh-Hant", "HK"},
        {"pt-BR", "BR"}
    };

    const std::string GetCountryCode(const std::string& locale)
    {
        auto it = regionMap.find(locale);
        if (it != regionMap.end())
        {
            return it->second;
        }
        return "US";
    }

    const std::string GetSerialNumber()
    {
        SetSysSerialNumber serial_number{};
        setsysInitialize();
        setsysGetSerialNumber(&serial_number);
        setsysExit();
        return serial_number.number;
    }
}
