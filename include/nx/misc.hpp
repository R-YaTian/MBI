#pragma once

#include <switch/types.h>
#include <string>

namespace nx::misc
{
    u32 GetBatteryValue();
    void SetBoostMode(bool enable);
    void AttemptForceReboot();
    std::string OpenSoftwareKeyboard(std::string guideText, std::string initialText, int LenMax);
    std::string UTF16toUTF8(const std::u16string& src);
    std::u16string UTF8toUTF16(const std::string& src);
    std::string ShortenString(const std::string& in, size_t maxLength, size_t preserve_tail_length = 0, const std::string& marker = "(...)");
    const std::string GetLocale();
    const std::string GetTimeZone();
    const std::string GetCountryCode(const std::string& locale);
    const std::string GetSerialNumber();
}
