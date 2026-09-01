#pragma once

#include <string>

#include <usbhsfs.h>
#include <switch/types.h>

namespace nx::udisk
{
    Result Init();
    void Exit();
    bool UnmountDrive(const UsbHsFsDevice &drv);

    std::string GetMountPointName(u32 driveIndex);
    u32 GetDriveCount();

    NX_CONSTEXPR bool DriveExists(u32 driveIndex, const std::string& mountPointName)
    {
        return GetMountPointName(driveIndex) == mountPointName;
    }
}
