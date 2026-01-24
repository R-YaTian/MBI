#include "install/HttpWorker.hpp"
#include "util/i18n.hpp"
#include "nx/error.hpp"
#include "facade.hpp"
#include <thread>

namespace app::install
{
    HttpWorker::HttpWorker(std::unique_ptr<nx::Content> content, const std::string &url)
        : Worker(std::move(content)), m_download(url)
    {
        RetrieveHeader();
    }

    HttpWorker::~HttpWorker() = default;

    void HttpWorker::CurlStreamThread(void* in)
    {
        StreamArgs* args = static_cast<StreamArgs*>(in);

        auto streamFunc = [&](u8* streamBuf, size_t streamBufSize) -> size_t
        {
            while (true)
            {
                if (args->bufferedPlaceholderWriter->CanAppendData(streamBufSize))
                {
                    break;
                }
            }

            args->bufferedPlaceholderWriter->AppendData(streamBuf, streamBufSize);
            return streamBufSize;
        };

        if (args->download->StreamDataRange(args->xfs0Offset, args->ncaSize, streamFunc) == 1)
        {
            stopThreads = true;
        }
    }

    void HttpWorker::PlaceholderWrite(void* in)
    {
        StreamArgs* args = static_cast<StreamArgs*>(in);

        while (!args->bufferedPlaceholderWriter->IsPlaceholderComplete() && !stopThreads)
        {
            if (args->bufferedPlaceholderWriter->CanWriteSegmentToPlaceholder())
            {
                args->bufferedPlaceholderWriter->WriteSegmentToPlaceholder();
            }
        }
    }

    void HttpWorker::StreamToPlaceholder(std::shared_ptr<nx::ncm::ContentStorage>& contentStorage, NcmContentId ncaId)
    {
        const void* fileEntry = m_content->GetFileEntryByNcaId(ncaId);
        std::string ncaFileName = m_content->GetFileEntryName(fileEntry);

        LOG_DEBUG("Retrieving %s\n", ncaFileName.c_str());
        size_t ncaSize = m_content->GetFileEntrySize(fileEntry);

        nx::data::BufferedPlaceholderWriter bufferedPlaceholderWriter(contentStorage, ncaId, ncaSize);
        StreamArgs args;
        args.download = &m_download;
        args.bufferedPlaceholderWriter = &bufferedPlaceholderWriter;
        args.xfs0Offset = m_content->GetFileEntryOffset(fileEntry);
        args.ncaSize = ncaSize;
        stopThreads = false;

        std::thread curlThread = std::thread(&HttpWorker::CurlStreamThread, this, &args);
        std::thread writeThread = std::thread(&HttpWorker::PlaceholderWrite, this, &args);

        u64 freq = armGetSystemTickFreq();
        u64 startTime = armGetSystemTick();
        size_t startSizeBuffered = 0;
        double speed = 0.0;

        app::facade::SendInstallProgress(0);
        while (!bufferedPlaceholderWriter.IsBufferDataComplete() && !stopThreads)
        {
            u64 newTime = armGetSystemTick();

            if (newTime - startTime >= freq * 0.5)
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

                app::facade::SendInstallInfoText("inst.info_page.downloading"_lang + nx::network::formatUrlString(ncaFileName) + "inst.info_page.at"_lang + std::to_string(speed).substr(0, std::to_string(speed).size() - 4) + "MB/s");
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
            app::facade::SendInstallInfoText("inst.info_page.top_info0"_lang + ncaFileName + " " + x.str() + "%");
        }
        app::facade::SendInstallProgress(100);

        if (curlThread.joinable())
        {
            curlThread.join();
        }
        if (writeThread.joinable())
        {
            writeThread.join();
        }
        if (stopThreads)
        {
            THROW_FORMAT(("inst.net.transfer_interput"_lang).c_str());
        }
    }

    void HttpWorker::BufferData(void* buf, off_t offset, size_t size)
    {
        m_download.BufferDataRange(buf, offset, size, nullptr);
    }
}
