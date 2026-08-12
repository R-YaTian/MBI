#include "install/UsbWorker.hpp"
#include "util/i18n.hpp"
#include "nx/error.hpp"
#include "nx/usb.hpp"
#include <malloc.h>

namespace app::install
{
    UsbWorker::UsbWorker(std::unique_ptr<nx::Content> content, const std::string &filename)
        : Worker(std::move(content)), m_fileName(filename)
    {
        RetrieveHeader();
    }

    UsbWorker::~UsbWorker() = default;

    void UsbWorker::ReadThread(void* in)
    {
        ThreadData* args = static_cast<ThreadData*>(in);
        std::string fileName = std::string(static_cast<char*>(args->in));
        nx::usb::USBCommandHeader header = nx::usb::USBCommandManager::SendFileRangeCommand(fileName, args->dataOffset, args->dataSize);

        u8* buf = (u8*)memalign(0x1000, nx::data::BUFFER_SEGMENT_DATA_SIZE);
        u64 sizeRemaining = header.dataSize;
        size_t tmpSizeRead = 0;

        try
        {
            while (sizeRemaining)
            {
                tmpSizeRead = nx::usb::usbDeviceRead(buf, std::min(sizeRemaining, (u64)nx::data::BUFFER_SEGMENT_DATA_SIZE), 5000000000);
                if (tmpSizeRead == 0)
                {
                    throw std::runtime_error("inst.usb.error"_lang.c_str());
                }
                sizeRemaining -= tmpSizeRead;

                while (true)
                {
                    if (args->bufferedPlaceholderWriter->CanAppendData(tmpSizeRead))
                    {
                        break;
                    }
                }

                args->bufferedPlaceholderWriter->AppendData(buf, tmpSizeRead);
            }
        }
        catch (const std::exception& e)
        {
            stopThreads = true;
            if (args->errorMessage != nullptr)
            {
                *args->errorMessage = e.what();
            }
        }

        free(buf);
    }

    void UsbWorker::StreamToPlaceholder(std::shared_ptr<nx::ncm::ContentStorage>& contentStorage, NcmContentId ncaId, nx::nca::NcaHeader* header)
    {
        if (appletGetAppletType() == AppletType_LibraryApplet)
        {
            this->WriteToPlaceholderDirectly(contentStorage, ncaId, nx::data::BUFFER_SEGMENT_DATA_SIZE, header);
        }
        else
        {
            this->WriteToPlaceholderBuffered(contentStorage, ncaId, (void *)m_fileName.c_str(), header);
        }
    }

    void UsbWorker::BufferData(void* buf, off_t offset, size_t size)
    {
        LOG_DEBUG("buffering 0x%lx-0x%lx\n", offset, offset + size);
        nx::usb::USBCommandHeader header = nx::usb::USBCommandManager::SendFileRangeCommand(m_fileName, offset, size);
        u8* tempBuffer = (u8*)memalign(0x1000, header.dataSize);
        if (nx::usb::USBReadData(tempBuffer, header.dataSize) == 0)
        {
            throw std::runtime_error("inst.usb.error"_lang.c_str());
        }
        std::memcpy(buf, tempBuffer, header.dataSize);
        free(tempBuffer);
    }
}
