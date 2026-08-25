#pragma once

#include <string>
#include <vector>
#include <functional>

#include <usbhsfs.h>
#include <switch/types.h>

namespace nx::udisk
{
    Result Init();
    void Exit();
	bool GetConsumeDrivesChanged();
	void DoWithDrives(const std::function<void(const std::vector<UsbHsFsDevice>&)> &cb);
    bool UnmountDrive(const UsbHsFsDevice &drv);

	std::string GetMountPointName(const UsbHsFsDevice &drive);
    std::string GetMountPointName(u32 driveIndex);
    u32 GetDriveCount();

	NX_CONSTEXPR bool DrivesEqual(const UsbHsFsDevice &drive_a, const UsbHsFsDevice &drive_b)
	{
        return drive_a.usb_if_id == drive_b.usb_if_id && drive_a.lun == drive_b.lun && drive_a.fs_idx == drive_b.fs_idx;
    }
}
