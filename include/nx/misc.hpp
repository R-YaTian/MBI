#pragma once

#include <switch/types.h>
#include <vector>
#include <string>

namespace nx::misc
{
    std::string OpenSoftwareKeyboard(std::string guideText, std::string initialText, int LenMax);
    std::vector<uint32_t> SetClockSpeed(int deviceToClock, uint32_t clockSpeed);
    u32 GetBatteryValue();
    std::string GetBatteryColor(u32 charge);
}
