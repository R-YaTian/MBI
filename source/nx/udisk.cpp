#include <atomic>
#include "nx/udisk.hpp"
#include "nx/error.hpp"
#include "nx/Scoped.hpp"

namespace nx::udisk
{
    namespace
    {
        std::atomic_bool g_DrivesChanged = false;
        Lock g_DrivesLock;
        std::vector<UsbHsFsDevice> g_Drives;
    }

    Result Init()
    {
        R_TRY(usbHsFsInitialize(0));

        usbHsFsSetPopulateCallback([](const UsbHsFsDevice *devices, u32 device_count, void *user_data) {
            ScopedLock lk(g_DrivesLock);
            g_Drives.clear();
            if (devices != nullptr && device_count > 0)
            {
                g_Drives.insert(g_Drives.end(), devices, devices + device_count);
            }
            g_DrivesChanged = true;
        }, nullptr);

        R_SUCCEED();
    }

    void Exit()
    {
        usbHsFsExit();
    }

    bool GetConsumeDrivesChanged()
    {
        return g_DrivesChanged.exchange(false);
    }

    bool UnmountDrive(const UsbHsFsDevice &drv)
    {
        return usbHsFsUnmountDevice(std::addressof(drv), true);
    }

    void DoWithDrives(const std::function<void(const std::vector<UsbHsFsDevice>&)> &cb)
    {
        ScopedLock lk(g_DrivesLock);
        cb(g_Drives);
    }

    std::string GetMountPointName(const UsbHsFsDevice &drive)
    {
        const std::string drive_name = drive.name;
        return drive_name;
    }

    std::string GetMountPointName(u32 driveIndex)
    {
        ScopedLock lk(g_DrivesLock);
        if (driveIndex >= g_Drives.size())
        {
            return "";
        }
        return g_Drives[driveIndex].name;
    }

    u32 GetDriveCount()
    {
        ScopedLock lk(g_DrivesLock);
        return g_Drives.size();
    }
}
