#pragma once

#include <switch.h>
#include <string>

namespace nx::usb
{
    typedef enum
    {
        DeviceSpeed_None = 0x0,
        DeviceSpeed_Low = 0x1,   ///< USB 1.0 Low Speed
        DeviceSpeed_Full = 0x2,  ///< USB 1.1 Full Speed
        DeviceSpeed_High = 0x3,  ///< USB 2.0 High Speed
        DeviceSpeed_Super = 0x4, ///< USB 3.0 Super Speed
        DeviceSpeed_Unknown = 0x5,
    } DeviceSpeed;

    /// Initializes USB device service with the default number of interfaces.
    Result usbDeviceInitialize();

    /// Exits USB device service.
    void usbDeviceExit();

    /// Gets the current state of the USB device.
    UsbState usbDeviceGetState();

    /// Gets the current speed of the USB device.
    DeviceSpeed usbDeviceGetSpeed();

    /// Checks whether the USB device is connected.
    bool usbDeviceIsConnected();

    enum class USBCommandId : u32
    {
        Finished  = 0x00,
        FileRange = 0x01,
        Exit      = 0x0F,
    };

    enum class USBCommandType : u8
    {
        REQUEST = 0,
        RESPONSE = 1
    };

    struct NX_PACKED USBCommandHeader
    {
        u32 magic;
        USBCommandType type;
        u8 protocolVersion;
        u8 padding[0x2] = {0};
        USBCommandId cmdId;
        u64 dataSize;
        u8 reserved[0xC] = {0};
    };

    static_assert(sizeof(USBCommandId) == 0x04, "USBCommandId must be 0x04!");
    static_assert(sizeof(USBCommandType) == 0x01, "USBCommandType must be 0x01!");
    static_assert(sizeof(USBCommandHeader) == 0x20, "USBCommandHeader must be 0x20!");

    struct FileRangeCommandHeader
    {
        u64 size;
        u64 offset;
        u64 fileNameLen;
        u64 padding;
    } NX_PACKED;

    struct FileListHeader
    {
        u32 magic; // TUL0 (Tinfoil USB List 0)
        u32 titleListSize;
        u64 padding;
    } NX_PACKED;

    class USBCommandManager
    {
        public:
            static void SendCommandHeader(USBCommandId cmdId, u64 dataSize, u64 timeout = 5000000000);
            static void SendFinishedCommand();
            static void SendExitCommand();
            static USBCommandHeader SendFileRangeCommand(std::string fileName, u64 offset, u64 size);
    };

    size_t USBReadData(void* out, size_t len, u64 timeout = 5000000000);
    size_t USBWriteData(const void* in, size_t len, u64 timeout = 5000000000);
} // namespace nx::usb
