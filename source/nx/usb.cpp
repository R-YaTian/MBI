#include "nx/usb.hpp"
#include "nx/error.hpp"
#include "nx/Scoped.hpp"
#include <cstring>
#include <malloc.h>

namespace nx::usb
{
    struct usbDeviceInterface
    {
        UsbDsInterface *interface{};
        UsbDsEndpoint *endpoint_in{}, *endpoint_out{};
    };

    static bool g_UsbDeviceInitialized = false;
    static usbDeviceInterface g_UsbDeviceInterfaces;

    constexpr std::align_val_t Align = std::align_val_t(0x1000);
    constexpr size_t BlockSize = 0x1000;

    static Result usbDeviceInterfaceInit1x()
    {
        usbDeviceInterface *interface = &g_UsbDeviceInterfaces;

        struct usb_interface_descriptor interface_descriptor = {
            .bLength = USB_DT_INTERFACE_SIZE,
            .bDescriptorType = USB_DT_INTERFACE,
            .bInterfaceNumber = 0,
            .bInterfaceClass = USB_CLASS_VENDOR_SPEC,
            .bInterfaceSubClass = USB_CLASS_VENDOR_SPEC,
            .bInterfaceProtocol = USB_CLASS_VENDOR_SPEC,
        };

        struct usb_endpoint_descriptor endpoint_descriptor_in = {
            .bLength = USB_DT_ENDPOINT_SIZE,
            .bDescriptorType = USB_DT_ENDPOINT,
            .bEndpointAddress = USB_ENDPOINT_IN,
            .bmAttributes = USB_TRANSFER_TYPE_BULK,
            .wMaxPacketSize = 0x200,
        };

        struct usb_endpoint_descriptor endpoint_descriptor_out = {
            .bLength = USB_DT_ENDPOINT_SIZE,
            .bDescriptorType = USB_DT_ENDPOINT,
            .bEndpointAddress = USB_ENDPOINT_OUT,
            .bmAttributes = USB_TRANSFER_TYPE_BULK,
            .wMaxPacketSize = 0x200,
        };

        // Setup interface.
        R_TRY(usbDsGetDsInterface(&interface->interface, &interface_descriptor, "usb"));

        // Setup endpoints.
        R_TRY(usbDsInterface_GetDsEndpoint(interface->interface, &interface->endpoint_in, &endpoint_descriptor_in)); // device -> host
        R_TRY(usbDsInterface_GetDsEndpoint(interface->interface, &interface->endpoint_out, &endpoint_descriptor_out)); // host -> device
        R_TRY(usbDsInterface_EnableInterface(interface->interface));

        R_SUCCEED();
    }

    static Result usbDeviceInterfaceInit5x()
    {
        usbDeviceInterface *interface = &g_UsbDeviceInterfaces;

        struct usb_interface_descriptor interface_descriptor = {
            .bLength = USB_DT_INTERFACE_SIZE,
            .bDescriptorType = USB_DT_INTERFACE,
            .bInterfaceNumber = USBDS_DEFAULT_InterfaceNumber, // set below
            .bNumEndpoints = 2,
            .bInterfaceClass = USB_CLASS_VENDOR_SPEC,
            .bInterfaceSubClass = USB_CLASS_VENDOR_SPEC,
            .bInterfaceProtocol = USB_CLASS_VENDOR_SPEC,
        };

        struct usb_endpoint_descriptor endpoint_descriptor_in = {
            .bLength = USB_DT_ENDPOINT_SIZE,
            .bDescriptorType = USB_DT_ENDPOINT,
            .bEndpointAddress = USB_ENDPOINT_IN,
            .bmAttributes = USB_TRANSFER_TYPE_BULK,
            .wMaxPacketSize = 0x40,
        };

        struct usb_endpoint_descriptor endpoint_descriptor_out = {
            .bLength = USB_DT_ENDPOINT_SIZE,
            .bDescriptorType = USB_DT_ENDPOINT,
            .bEndpointAddress = USB_ENDPOINT_OUT,
            .bmAttributes = USB_TRANSFER_TYPE_BULK,
            .wMaxPacketSize = 0x40,
        };

        struct usb_ss_endpoint_companion_descriptor endpoint_companion = {
            .bLength = sizeof(struct usb_ss_endpoint_companion_descriptor),
            .bDescriptorType = USB_DT_SS_ENDPOINT_COMPANION,
            .bMaxBurst = 0x0F,
            .bmAttributes = 0x00,
            .wBytesPerInterval = 0x00,
        };

        R_TRY(usbDsRegisterInterface(&interface->interface));

        interface_descriptor.bInterfaceNumber = interface->interface->interface_index;
        endpoint_descriptor_in.bEndpointAddress += interface_descriptor.bInterfaceNumber + 1;
        endpoint_descriptor_out.bEndpointAddress += interface_descriptor.bInterfaceNumber + 1;

        // Full Speed Config
        R_TRY(usbDsInterface_AppendConfigurationData(interface->interface, UsbDeviceSpeed_Full, &interface_descriptor, USB_DT_INTERFACE_SIZE));
        R_TRY(usbDsInterface_AppendConfigurationData(interface->interface, UsbDeviceSpeed_Full, &endpoint_descriptor_in, USB_DT_ENDPOINT_SIZE));
        R_TRY(usbDsInterface_AppendConfigurationData(interface->interface, UsbDeviceSpeed_Full, &endpoint_descriptor_out, USB_DT_ENDPOINT_SIZE));

        // High Speed Config
        endpoint_descriptor_in.wMaxPacketSize = 0x200;
        endpoint_descriptor_out.wMaxPacketSize = 0x200;
        R_TRY(usbDsInterface_AppendConfigurationData(interface->interface, UsbDeviceSpeed_High, &interface_descriptor, USB_DT_INTERFACE_SIZE));
        R_TRY(usbDsInterface_AppendConfigurationData(interface->interface, UsbDeviceSpeed_High, &endpoint_descriptor_in, USB_DT_ENDPOINT_SIZE));
        R_TRY(usbDsInterface_AppendConfigurationData(interface->interface, UsbDeviceSpeed_High, &endpoint_descriptor_out, USB_DT_ENDPOINT_SIZE));

        // Super Speed Config
        endpoint_descriptor_in.wMaxPacketSize = 0x400;
        endpoint_descriptor_out.wMaxPacketSize = 0x400;
        R_TRY(usbDsInterface_AppendConfigurationData(interface->interface, UsbDeviceSpeed_Super, &interface_descriptor, USB_DT_INTERFACE_SIZE));
        R_TRY(usbDsInterface_AppendConfigurationData(interface->interface, UsbDeviceSpeed_Super, &endpoint_descriptor_in, USB_DT_ENDPOINT_SIZE));
        R_TRY(usbDsInterface_AppendConfigurationData(interface->interface, UsbDeviceSpeed_Super, &endpoint_companion, USB_DT_SS_ENDPOINT_COMPANION_SIZE));
        R_TRY(usbDsInterface_AppendConfigurationData(interface->interface, UsbDeviceSpeed_Super, &endpoint_descriptor_out, USB_DT_ENDPOINT_SIZE));
        R_TRY(usbDsInterface_AppendConfigurationData(interface->interface, UsbDeviceSpeed_Super, &endpoint_companion, USB_DT_SS_ENDPOINT_COMPANION_SIZE));

        // Setup endpoints.
        R_TRY(usbDsInterface_RegisterEndpoint(interface->interface, &interface->endpoint_in, endpoint_descriptor_in.bEndpointAddress));
        R_TRY(usbDsInterface_RegisterEndpoint(interface->interface, &interface->endpoint_out, endpoint_descriptor_out.bEndpointAddress));
        R_TRY(usbDsInterface_EnableInterface(interface->interface));

        R_SUCCEED();
    }

    static Result usbDeviceInterfaceInit()
    {
        if (hosversionAtLeast(5,0,0))
        {
            return usbDeviceInterfaceInit5x();
        }
        else
        {
            return usbDeviceInterfaceInit1x();
        }
    }

    Result usbDeviceInitialize()
    {
        if (g_UsbDeviceInitialized)
        {
            R_THROW(MAKERESULT(Module_Libnx, LibnxError_AlreadyInitialized));
        }
        else
        {
            R_TRY(usbDsInitialize());
            static SetSysSerialNumber serial_number{};
            R_TRY(setsysInitialize());
            ON_SCOPE_EXIT(setsysExit());
            R_TRY(setsysGetSerialNumber(&serial_number));

            if (hosversionAtLeast(5,0,0))
            {
                u8 iManufacturer, iProduct, iSerialNumber;
                static const u16 supported_langs[1] = { 0x0409 };
                // Send language descriptor
                R_TRY(usbDsAddUsbLanguageStringDescriptor(nullptr, supported_langs, sizeof(supported_langs)/sizeof(u16)));
                // Send manufacturer
                R_TRY(usbDsAddUsbStringDescriptor(&iManufacturer, "Nintendo"));
                // Send product
                R_TRY(usbDsAddUsbStringDescriptor(&iProduct, "Nintendo Switch"));
                // Send serial number
                R_TRY(usbDsAddUsbStringDescriptor(&iSerialNumber, serial_number.number));

                // Send device descriptors
                struct usb_device_descriptor device_descriptor = {
                    .bLength = USB_DT_DEVICE_SIZE,
                    .bDescriptorType = USB_DT_DEVICE,
                    .bcdUSB = 0x0110,
                    .bDeviceClass = 0x00,
                    .bDeviceSubClass = 0x00,
                    .bDeviceProtocol = 0x00,
                    .bMaxPacketSize0 = 0x40,
                    .idVendor = 0x057e,
                    .idProduct = 0x3000,
                    .bcdDevice = 0x0100,
                    .iManufacturer = iManufacturer,
                    .iProduct = iProduct,
                    .iSerialNumber = iSerialNumber,
                    .bNumConfigurations = 0x01
                };

                // Full Speed is USB 1.1
                R_TRY(usbDsSetUsbDeviceDescriptor(UsbDeviceSpeed_Full, &device_descriptor));

                // High Speed is USB 2.0
                device_descriptor.bcdUSB = 0x0200;
                R_TRY(usbDsSetUsbDeviceDescriptor(UsbDeviceSpeed_High, &device_descriptor));

                // Super Speed is USB 3.0
                device_descriptor.bcdUSB = 0x0300;
                // Upgrade packet size to 512
                device_descriptor.bMaxPacketSize0 = 0x09;
                R_TRY(usbDsSetUsbDeviceDescriptor(UsbDeviceSpeed_Super, &device_descriptor));

                // Define Binary Object Store
                u8 bos[0x16] = {
                    0x05, // .bLength
                    USB_DT_BOS, // .bDescriptorType
                    0x16, 0x00, // .wTotalLength
                    0x02, // .bNumDeviceCaps

                    // USB 2.0
                    0x07, // .bLength
                    USB_DT_DEVICE_CAPABILITY, // .bDescriptorType
                    0x02, // .bDevCapabilityType
                    0x02, 0x00, 0x00, 0x00, // dev_capability_data

                    // USB 3.0
                    0x0A, // .bLength
                    USB_DT_DEVICE_CAPABILITY, // .bDescriptorType
                    0x03, // .bDevCapabilityType
                    0x00, 0x0E, 0x00, 0x03, 0x00, 0x00, 0x00
                };
                R_TRY(usbDsSetBinaryObjectStore(bos, sizeof(bos)));
            }

            R_TRY(usbDeviceInterfaceInit());

            if (hosversionAtLeast(5,0,0))
            {
                R_TRY(usbDsEnable());
            }
        }

        g_UsbDeviceInitialized = true;

        R_SUCCEED();
    }

    static void usbDeviceInterfaceFree(usbDeviceInterface *interface)
    {
        interface->endpoint_in = nullptr;
        interface->endpoint_out = nullptr;
        interface->interface = nullptr;
    }

    void usbDeviceExit()
    {
        usbDsExit();
        if (g_UsbDeviceInitialized)
        {
            usbDeviceInterfaceFree(&g_UsbDeviceInterfaces);
            g_UsbDeviceInitialized = false;
        }
    }

    Result TransferImpl(void *buf, const size_t size, UsbDsEndpoint *ep, u64 timeout)
    {
        if(!usbDeviceIsConnected())
        {
            return MAKERESULT(Module_Libnx, LibnxError_BadUsbCommsRead);
        }

        Result rc = usbDsWaitReady(timeout);
        if (R_FAILED(rc)) return rc;

        u32 urb_id = 0;
        rc = usbDsEndpoint_PostBufferAsync(ep, buf, size, &urb_id);
        if (R_SUCCEEDED(rc))
        {
            rc = eventWait(&ep->CompletionEvent, timeout);
            if (R_FAILED(rc))
            {
                usbDsEndpoint_Cancel(ep);
                eventWait(&ep->CompletionEvent, UINT64_MAX);
                eventClear(&ep->CompletionEvent);
                return rc;
            }
            eventClear(&ep->CompletionEvent);

            if (R_SUCCEEDED(rc))
            {
                UsbDsReportData report_data;
                rc = usbDsEndpoint_GetReportData(ep, &report_data);
                u32 report_size = 0;
                if (R_SUCCEEDED(rc))
                {
                    rc = usbDsParseReportData(&report_data, urb_id, nullptr, &report_size);
                }
                // if (report_size != size)
                // {
                //     return MAKERESULT(Module_Libnx, LibnxError_BadUsbCommsRead);
                // }
            }
        }

        return rc;
    }

    UsbState usbDeviceGetState()
    {
        UsbState state = UsbState_Detached;
        usbDsGetState(&state);
        return state;
    }

    DeviceSpeed usbDeviceGetSpeed()
    {
        if (hosversionAtLeast(8,0,0))
        {
            UsbDeviceSpeed speed = UsbDeviceSpeed_None;
            usbDsGetSpeed(&speed);
            return (DeviceSpeed)speed;
        }
        return DeviceSpeed_Unknown;
    }

    bool usbDeviceIsConnected()
    {
        return usbDeviceGetState() == UsbState_Configured;
    }

    void USBCommandManager::SendCommandHeader(USBCommandId cmdId, u64 dataSize, u64 timeout)
    {
        const USBCommandHeader header {
            .magic = 0x30435554, // TUC0 (Tinfoil USB Command 0)
            .type = USBCommandType::REQUEST,
            .protocolVersion = 1,
            .padding = {0},
            .cmdId = cmdId,
            .dataSize = dataSize,
            .reserved = {0}
        };

        USBWriteData(&header, sizeof(USBCommandHeader), timeout);
    }

    void USBCommandManager::SendFinishedCommand()
    {
        USBCommandManager::SendCommandHeader(USBCommandId::Finished, 0, 1000000);
    }

    void USBCommandManager::SendExitCommand()
    {
        USBCommandManager::SendCommandHeader(USBCommandId::Exit, 0, 1000000);
    }

    USBCommandHeader USBCommandManager::SendFileRangeCommand(std::string fileName, u64 offset, u64 size)
    {
        const FileRangeCommandHeader fRangeHeader {
            .size = size,
            .offset = offset,
            .fileNameLen = fileName.size(),
            .padding = 0
        };

        USBCommandManager::SendCommandHeader(USBCommandId::FileRange, sizeof(FileRangeCommandHeader) + fRangeHeader.fileNameLen);
        USBWriteData(&fRangeHeader, sizeof(FileRangeCommandHeader));
        USBWriteData(fileName.c_str(), fRangeHeader.fileNameLen);

        USBCommandHeader responseHeader;
        USBReadData(&responseHeader, sizeof(USBCommandHeader));
        return responseHeader;
    }

    size_t USBReadData(void* out, size_t len, u64 timeout)
    {
        auto aligned_buf = new (Align) u8[len]();
        ON_SCOPE_EXIT(operator delete[](aligned_buf, Align));
        Result rc = TransferImpl(aligned_buf, len, g_UsbDeviceInterfaces.endpoint_out, timeout);
        if (R_FAILED(rc))
        {
            return 0;
        }
        std::memcpy(out, aligned_buf, len);
        return len;
    }

    size_t USBWriteData(const void* in, size_t len, u64 timeout)
    {
        auto aligned_buf = new (Align) u8[len]();
        ON_SCOPE_EXIT(operator delete[](aligned_buf, Align));
        std::memcpy(aligned_buf, in, len);
        Result rc = TransferImpl(aligned_buf, len, g_UsbDeviceInterfaces.endpoint_in, timeout);
        if (R_FAILED(rc))
        {
            return 0;
        }
        return len;
    }
}
