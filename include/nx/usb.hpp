#pragma once

#include <switch.h>

namespace nx::usb
{
    static constexpr auto TOTAL_INTERFACES = 4;

    struct usbDeviceInterface
    {
        RwLock lock, lock_in, lock_out;
        bool initialized;

        UsbDsInterface *interface;
        UsbDsEndpoint *endpoint_in, *endpoint_out;

        u8 *endpoint_in_buffer, *endpoint_out_buffer;
    };

    struct usbDeviceInterfaceInfo
    {
        u8 bInterfaceClass;
        u8 bInterfaceSubClass;
        u8 bInterfaceProtocol;
    } NX_PACKED;

    /// Initializes USB device service with the default number of interfaces.
    Result usbDeviceInitialize();

    /// Initializes USB device service with a specific number of interfaces.
    Result usbDeviceInitializeEx(u32 num_interfaces, const usbDeviceInterfaceInfo* infos);

    /// Exits USB device service.
    void usbDeviceExit();

    /// Read data with the default interface.
    size_t usbDeviceRead(void* buffer, size_t size, u64 timeout);

    /// Write data with the default interface.
    size_t usbDeviceWrite(const void* buffer, size_t size, u64 timeout);

    /// Read data with the specified interface.
    size_t usbDeviceReadEx(void* buffer, size_t size, u32 interface, u64 timeout);

    /// Write data with the specified interface.
    size_t usbDeviceWriteEx(const void* buffer, size_t size, u32 interface, u64 timeout);

    /// Reinitialize the USB device service.
    void usbDeviceReset();

    /// Checks whether the USB device is connected.
    bool usbDeviceIsConnected();
} // namespace nx::usb
