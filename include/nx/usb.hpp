#pragma once

#include <switch.h>
#include <string>

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
