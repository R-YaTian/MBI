#include "install/Worker.hpp"
#include "util/i18n.hpp"
#include "nx/error.hpp"
#include "nx/xfs0.hpp"
#include "nx/nsp.hpp"
#include "nx/xci.hpp"
#include "facade.hpp"
#include <thread>
#include <sstream>
#include <iomanip>

namespace app::install
{
    void RetrieveNSPHeader(Worker& worker, nx::NSP& nsp)
    {
        LOG_DEBUG("Retrieving remote NSP header...\n");

        // Retrieve the base header
        std::vector<u8> headerBytes(sizeof(nx::XFS0BaseHeader), 0);
        worker.BufferData(headerBytes.data(), 0x0, sizeof(nx::XFS0BaseHeader));

        nx::XFS0BaseHeader* header = reinterpret_cast<nx::XFS0BaseHeader*>(headerBytes.data());

        // Retrieve the full header
        size_t remainingHeaderSize = header->numFiles * sizeof(nx::PFS0FileEntry) + header->stringTableSize;
        headerBytes.resize(sizeof(nx::XFS0BaseHeader) + remainingHeaderSize, 0);
        worker.BufferData(headerBytes.data() + sizeof(nx::XFS0BaseHeader), sizeof(nx::XFS0BaseHeader), remainingHeaderSize);

        nsp.CommitHeader(std::move(headerBytes));
    }

    void RetrieveXCIHeader(Worker& worker, nx::XCI& xci)
    {
        LOG_DEBUG("Retrieving HFS0 header...\n");

        // Retrieve hfs0 offset
        u64 hfs0Offset = HFS0_ROOT_HEADER_OFFSET;

        // Retrieve main hfs0 header
        std::vector<u8> m_headerBytes;
        m_headerBytes.resize(sizeof(nx::XFS0BaseHeader), 0);
        worker.BufferData(m_headerBytes.data(), hfs0Offset, sizeof(nx::XFS0BaseHeader));

        // Retrieve full header
        nx::XFS0BaseHeader* header = reinterpret_cast<nx::XFS0BaseHeader*>(m_headerBytes.data());
        if (header->magic != MAGIC_HFS0)
        {
            // otherwise, try and again as maybe the key area pre-prended.
            hfs0Offset = HFS0_ROOT_HEADER_OFFSET_WITH_KEY_AREA;
            worker.BufferData(m_headerBytes.data(), hfs0Offset, sizeof(nx::XFS0BaseHeader));
            if (header->magic != MAGIC_HFS0)
            {
                THROW_FORMAT("hfs0 magic doesn't match at 0x%lx\n", hfs0Offset);
            }
        }
        size_t remainingHeaderSize = header->numFiles * sizeof(nx::HFS0FileEntry) + header->stringTableSize;
        m_headerBytes.resize(sizeof(nx::XFS0BaseHeader) + remainingHeaderSize, 0);
        worker.BufferData(m_headerBytes.data() + sizeof(nx::XFS0BaseHeader), hfs0Offset + sizeof(nx::XFS0BaseHeader), remainingHeaderSize);

        // Find Secure partition
        header = reinterpret_cast<nx::XFS0BaseHeader*>(m_headerBytes.data());
        for (unsigned int i = 0; i < header->numFiles; i++)
        {
            const nx::HFS0FileEntry* entry = nx::hfs0GetFileEntry(header, i);
            std::string entryName(nx::hfs0GetFileName(header, entry));

            if (entryName != "secure")
            {
                continue;
            }

            std::vector<u8> secureHeaderBytes;
            u64 secureHeaderOffset = hfs0Offset + m_headerBytes.size() + entry->dataOffset;
            secureHeaderBytes.resize(sizeof(nx::XFS0BaseHeader), 0);
            worker.BufferData(secureHeaderBytes.data(), secureHeaderOffset, sizeof(nx::XFS0BaseHeader));

            nx::XFS0BaseHeader* secureHeader = reinterpret_cast<nx::XFS0BaseHeader*>(secureHeaderBytes.data());

            if (secureHeader->magic != MAGIC_HFS0)
            {
                THROW_FORMAT("hfs0 magic doesn't match at 0x%lx\n", secureHeaderOffset);
            }

            // Retrieve full header
            remainingHeaderSize = secureHeader->numFiles * sizeof(nx::HFS0FileEntry) + secureHeader->stringTableSize;
            secureHeaderBytes.resize(sizeof(nx::XFS0BaseHeader) + remainingHeaderSize, 0);
            worker.BufferData(secureHeaderBytes.data() + sizeof(nx::XFS0BaseHeader), secureHeaderOffset + sizeof(nx::XFS0BaseHeader), remainingHeaderSize);

            xci.CommitHeader(std::move(secureHeaderBytes), secureHeaderOffset);
            return;
        }
        THROW_FORMAT("couldn't optain secure hfs0 header\n");
    }

    void Worker::RetrieveHeader()
    {
        switch (m_content->GetType())
        {
            case nx::Content::Type::NSP:
                RetrieveNSPHeader(*this, static_cast<nx::NSP&>(*m_content));
                break;

            case nx::Content::Type::XCI:
                RetrieveXCIHeader(*this, static_cast<nx::XCI&>(*m_content));
                break;

            default:
                THROW_FORMAT("Logic error: invalid content type");
        }
    }

    void Worker::WriteToPlaceholderBuffered(std::shared_ptr<nx::ncm::ContentStorage>& contentStorage, NcmContentId ncaId, void* threadDataIn, nx::nca::NcaHeader* header)
    {
        const void* fileEntry = m_content->GetFileEntryByNcaId(ncaId);
        std::string ncaFileName = m_content->GetFileEntryName(fileEntry);
        u64 ncaSize = m_content->GetFileEntrySize(fileEntry);

        nx::data::BufferedPlaceholderWriter bufferedPlaceholderWriter(contentStorage, ncaId, ncaSize);
        ThreadData args{};
        std::string errorMessage;
        args.bufferedPlaceholderWriter = &bufferedPlaceholderWriter;
        args.xfs0Offset = m_content->GetFileEntryOffset(fileEntry);
        args.ncaSize = ncaSize;
        args.in = threadDataIn;
        args.errorMessage = &errorMessage;
        stopThreads = false;

        std::thread readThread = std::thread(&Worker::ReadThread, this, &args);
        std::thread writeThread = std::thread(&Worker::PlaceholderWrite, this, &args);

        u64 freq = armGetSystemTickFreq();
        u64 startTime = armGetSystemTick();
        size_t startSizeBuffered = 0;
        double speed = 0.0;

        app::facade::SendInstallInfoText("inst.info_page.downloading"_lang + ncaFileName + "...");
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

                std::stringstream x;
                x << downloadProgress;
                std::stringstream speedStr;
                speedStr << std::fixed << std::setprecision(2) << speed;
                app::facade::SendInstallBarText(x.str() + "% " + "inst.info_page.at"_lang + speedStr.str() + "MB/s");
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
            x << installProgress;
            app::facade::SendInstallBarText(x.str() + "%");
        }
        std::string ncaIdStr = nx::ncm::GetContentIdString(ncaId);
        m_hashMap[ncaIdStr] = bufferedPlaceholderWriter.Finalize();
        app::facade::SendInstallProgress(100);

        if (readThread.joinable())
        {
            readThread.join();
        }
        if (writeThread.joinable())
        {
            writeThread.join();
        }
        if (stopThreads)
        {
            THROW_FORMAT("%s", errorMessage.c_str());
        }
    }
}
