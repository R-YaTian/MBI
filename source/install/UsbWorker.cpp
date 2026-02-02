#include "install/UsbWorker.hpp"
#include "util/i18n.hpp"
#include "nx/error.hpp"
#include "nx/usb.hpp"
#include "facade.hpp"
#include <malloc.h>
#include <thread>

namespace app::install
{
    UsbWorker::UsbWorker(std::unique_ptr<nx::Content> content, const std::string &filename)
        : Worker(std::move(content)), m_fileName(filename)
    {
        RetrieveHeader();
    }

    UsbWorker::~UsbWorker() = default;

    void UsbWorker::USBReadThread(void* in)
    {
        USBArgs* args = static_cast<USBArgs*>(in);
        nx::usb::USBCommandHeader header = nx::usb::USBCommandManager::SendFileRangeCommand(args->fileName, args->xfs0Offset, args->ncaSize);

        u8* buf = (u8*)memalign(0x1000, 0x800000);
        u64 sizeRemaining = header.dataSize;
        size_t tmpSizeRead = 0;

        try
        {
            while (sizeRemaining && !stopThreads)
            {
                tmpSizeRead = nx::usb::usbDeviceRead(buf, std::min(sizeRemaining, (u64)0x800000), 5000000000);
                if (tmpSizeRead == 0)
                {
                    THROW_FORMAT(("inst.usb.error"_lang).c_str());
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
        catch (std::exception& e)
        {
            stopThreads = true;
            errorMessage = e.what();
        }

        free(buf);
    }

    void UsbWorker::PlaceholderWrite(void* in)
    {
        USBArgs* args = static_cast<USBArgs*>(in);

        while (!args->bufferedPlaceholderWriter->IsPlaceholderComplete() && !stopThreads)
        {
            if (args->bufferedPlaceholderWriter->CanWriteSegmentToPlaceholder())
            {
                args->bufferedPlaceholderWriter->WriteSegmentToPlaceholder();
            }
        }
    }

    void UsbWorker::StreamToPlaceholder(std::shared_ptr<nx::ncm::ContentStorage>& contentStorage, NcmContentId ncaId)
    {
        const void* fileEntry = m_content->GetFileEntryByNcaId(ncaId);
        std::string ncaFileName = m_content->GetFileEntryName(fileEntry);

        LOG_DEBUG("Retrieving %s\n", ncaFileName.c_str());
        size_t ncaSize = m_content->GetFileEntrySize(fileEntry);

        nx::data::BufferedPlaceholderWriter bufferedPlaceholderWriter(contentStorage, ncaId, ncaSize);
        USBArgs args;
        args.fileName = m_fileName;
        args.bufferedPlaceholderWriter = &bufferedPlaceholderWriter;
        args.xfs0Offset = m_content->GetFileEntryOffset(fileEntry);
        args.ncaSize = ncaSize;
        stopThreads = false;

        std::thread usbThread = std::thread(&UsbWorker::USBReadThread, this, &args);
        std::thread writeThread = std::thread(&UsbWorker::PlaceholderWrite, this, &args);

        u64 freq = armGetSystemTickFreq();
        u64 startTime = armGetSystemTick();
        size_t startSizeBuffered = 0;
        double speed = 0.0;

        app::facade::SendInstallInfoText("inst.info_page.downloading"_lang + ncaFileName + "...");
        app::facade::SendInstallProgress(0);
        while (!bufferedPlaceholderWriter.IsBufferDataComplete() && !stopThreads)
        {
            u64 newTime = armGetSystemTick();

            if (newTime - startTime >= freq)
            {
                size_t newSizeBuffered = bufferedPlaceholderWriter.GetSizeBuffered();
                double mbBuffered = (newSizeBuffered / 1000000.0) - (startSizeBuffered / 1000000.0);
                double duration = ((double)(newTime - startTime) / (double)freq);
                speed =  mbBuffered / duration;

                startTime = newTime;
                startSizeBuffered = newSizeBuffered;
                int downloadProgress = (int)(((double)bufferedPlaceholderWriter.GetSizeBuffered() / (double)bufferedPlaceholderWriter.GetTotalDataSize()) * 100.0);
#ifdef NXLINK_DEBUG
                u64 totalSizeMB = bufferedPlaceholderWriter.GetTotalDataSize() / 1000000;
                u64 downloadSizeMB = bufferedPlaceholderWriter.GetSizeBuffered() / 1000000;
                LOG_DEBUG("> Download Progress: %lu/%lu MB (%i%s) (%.2f MB/s)\r", downloadSizeMB, totalSizeMB, downloadProgress, "%", speed);
#endif
                std::stringstream x;
                x << downloadProgress;
                app::facade::SendInstallBarText(x.str() + "% " + "inst.info_page.at"_lang + std::to_string(speed).substr(0, std::to_string(speed).size() - 4) + "MB/s");
                app::facade::SendInstallProgress((double)downloadProgress);
            }
        }
        app::facade::SendInstallProgress(100);

#ifdef NXLINK_DEBUG
        u64 totalSizeMB = bufferedPlaceholderWriter.GetTotalDataSize() / 1000000;
#endif

        app::facade::SendInstallInfoText("inst.info_page.top_info0"_lang + ncaFileName + "...");
        app::facade::SendInstallProgress(0);
        while (!bufferedPlaceholderWriter.IsPlaceholderComplete() && !stopThreads)
        {
            int installProgress = (int)(((double)bufferedPlaceholderWriter.GetSizeWrittenToPlaceholder() / (double)bufferedPlaceholderWriter.GetTotalDataSize()) * 100.0);
#ifdef NXLINK_DEBUG
            u64 installSizeMB = bufferedPlaceholderWriter.GetSizeWrittenToPlaceholder() / 1000000;
            LOG_DEBUG("> Install Progress: %lu/%lu MB (%i%s)\r", installSizeMB, totalSizeMB, installProgress, "%");
#endif
            app::facade::SendInstallProgress((double)installProgress);
            std::stringstream x;
            x << (int)(installProgress);
            app::facade::SendInstallBarText(x.str() + "%");
        }
        std::string ncaIdStr = nx::nca::GetNcaIdString(ncaId);
        m_hashMap[ncaIdStr] = bufferedPlaceholderWriter.ExportSha256Hash();
        app::facade::SendInstallProgress(100);

        if (usbThread.joinable())
        {
            usbThread.join();
        }
        if (writeThread.joinable())
        {
            writeThread.join();
        }
        if (stopThreads)
        {
            throw std::runtime_error(errorMessage.c_str());
        }
    }

    void UsbWorker::BufferData(void* buf, off_t offset, size_t size)
    {
        LOG_DEBUG("buffering 0x%lx-0x%lx\n", offset, offset + size);
        nx::usb::USBCommandHeader header = nx::usb::USBCommandManager::SendFileRangeCommand(m_fileName, offset, size);
        u8* tempBuffer = (u8*)memalign(0x1000, header.dataSize);
        if (nx::usb::USBReadData(tempBuffer, header.dataSize) == 0)
        {
            THROW_FORMAT(("inst.usb.error"_lang).c_str());
        }
        memcpy(buf, tempBuffer, header.dataSize);
        free(tempBuffer);
    }
}
