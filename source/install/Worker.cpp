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

    void Worker::UpdateTransferProgress(size_t& startSize, size_t newSize, size_t totalSize, u64& startTime, u64 freq)
    {
        u64 newTime = armGetSystemTick();
        if (newTime - startTime < freq * 0.02)
        {
            return;
        }

        double mbBuffered = (newSize / 1000000.0) - (startSize / 1000000.0);
        double duration = static_cast<double>(newTime - startTime) / static_cast<double>(freq);
        double speed = mbBuffered / duration;

        startTime = newTime;
        startSize = newSize;

        int progress = static_cast<int>((double)newSize / (double)totalSize * 100.0);
        app::facade::SendInstallProgress(static_cast<double>(progress));

        std::stringstream x;
        x << progress;
        std::stringstream speedStr;
        speedStr << std::fixed << std::setprecision(2) << speed;
        app::facade::SendInstallBarText(x.str() + "% " + "inst.info_page.at"_lang + speedStr.str() + "MB/s");
    }

    void Worker::PlaceholderWrite(void* in)
    {
        ThreadData* args = static_cast<ThreadData*>(in);

        try
        {
            while (!args->bufferedPlaceholderWriter->IsPlaceholderComplete() && !stopThreads)
            {
                if (args->bufferedPlaceholderWriter->CanWriteSegmentToPlaceholder())
                {
                    args->bufferedPlaceholderWriter->WriteSegmentToPlaceholder();
                }
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
    }

    void Worker::ReadThread(void* in)
    {
        ThreadData* args = static_cast<ThreadData*>(in);
        Worker* worker = static_cast<Worker*>(args->in);

        std::vector<u8> buf(DEFAULT_READ_BUFFER_SIZE);

        u64 sizeRemaining = args->dataSize;
        size_t tmpSizeRead = 0;

        try
        {
            while (sizeRemaining)
            {
                const size_t chunkSize = std::min<size_t>(sizeRemaining, DEFAULT_READ_BUFFER_SIZE);
                worker->BufferData(buf.data(), args->dataOffset + tmpSizeRead, chunkSize);

                while (!stopThreads)
                {
                    if (args->bufferedPlaceholderWriter->CanAppendData(chunkSize))
                    {
                        break;
                    }
                }

                if (stopThreads)
                {
                    break;
                }

                args->bufferedPlaceholderWriter->AppendData(buf.data(), chunkSize);
                sizeRemaining -= chunkSize;
                tmpSizeRead += chunkSize;
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
    }

    void Worker::WriteToPlaceholderBuffered(std::shared_ptr<nx::ncm::ContentStorage>& contentStorage, NcmContentId ncaId, void* threadDataIn, nx::nca::NcaHeader* header, u32 numBufferSegments)
    {
        const void* fileEntry = m_content->GetFileEntryByNcaId(ncaId);
        std::string ncaFileName = m_content->GetFileEntryName(fileEntry);
        u64 ncaSize = m_content->GetFileEntrySize(fileEntry);

        nx::data::BufferedPlaceholderWriter bufferedPlaceholderWriter(contentStorage, ncaId, ncaSize, numBufferSegments);
        u64 prependedSize = 0;
        if (header != nullptr)
        {
            prependedSize = sizeof(nx::nca::NcaHeader);
            bufferedPlaceholderWriter.AppendData(header, prependedSize);
        }

        ThreadData args{};
        std::string errorMessage;
        args.bufferedPlaceholderWriter = &bufferedPlaceholderWriter;
        args.dataOffset = m_content->GetFileEntryOffset(fileEntry) + prependedSize;
        args.dataSize = ncaSize - prependedSize;
        args.in = threadDataIn;
        args.errorMessage = &errorMessage;
        stopThreads = false;

        std::thread readThread = std::thread(&Worker::ReadThread, this, &args);
        std::thread writeThread = std::thread(&Worker::PlaceholderWrite, this, &args);

        u64 freq = armGetSystemTickFreq();
        u64 startTime = armGetSystemTick();
        size_t startSizeBuffered = 0;

        app::facade::SendInstallInfoText("inst.info_page.installing"_lang + ncaFileName + "...");
        app::facade::SendInstallProgress(0);
        while (!bufferedPlaceholderWriter.IsBufferDataComplete() && !stopThreads)
        {
            this->UpdateTransferProgress(startSizeBuffered, bufferedPlaceholderWriter.GetSizeBuffered(), bufferedPlaceholderWriter.GetTotalDataSize(), startTime, freq);
        }
        app::facade::SendInstallProgress(100);

#ifdef NXLINK_DEBUG
        u64 totalSizeMB = bufferedPlaceholderWriter.GetTotalDataSize() / 1000000;
#endif

        app::facade::SendInstallInfoText("inst.info_page.finishing"_lang + ncaFileName);
        app::facade::SendInstallProgress(0);
        while (!bufferedPlaceholderWriter.IsPlaceholderComplete() && !stopThreads)
        {
            int installProgress = static_cast<int>((double)bufferedPlaceholderWriter.GetSizeWrittenToPlaceholder() / (double)bufferedPlaceholderWriter.GetTotalDataSize() * 100.0);
#ifdef NXLINK_DEBUG
            u64 installSizeMB = bufferedPlaceholderWriter.GetSizeWrittenToPlaceholder() / 1000000;
            LOG_DEBUG("> Install Progress: %lu/%lu MB (%i%s)\r", installSizeMB, totalSizeMB, installProgress, "%");
#endif
            app::facade::SendInstallProgress((double)installProgress);
            std::stringstream x;
            x << installProgress;
            app::facade::SendInstallBarText(x.str() + "%");
        }

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

        std::string ncaIdStr = nx::ncm::GetContentIdString(ncaId);
        m_hashMap[ncaIdStr] = bufferedPlaceholderWriter.Finalize();
        app::facade::SendInstallProgress(100);
    }

    void Worker::WriteToPlaceholderDirectly(std::shared_ptr<nx::ncm::ContentStorage>& contentStorage, NcmContentId ncaId, const u64 maxBufferSize, nx::nca::NcaHeader* header)
    {
        const void* fileEntry = m_content->GetFileEntryByNcaId(ncaId);
        std::string ncaFileName = m_content->GetFileEntryName(fileEntry);
        u64 ncaSize = m_content->GetFileEntrySize(fileEntry);

        NcaWriter writer(ncaId, contentStorage);

        u64 fileStart = m_content->GetFileEntryOffset(fileEntry);
        u64 fileOff = 0;
        size_t prependedSize = 0;
        size_t readSize = maxBufferSize;
        auto readBuffer = std::make_unique<u8[]>(readSize);
        if (header != nullptr)
        {
            prependedSize = sizeof(nx::nca::NcaHeader);
            std::memcpy(readBuffer.get(), header, prependedSize);
        }

        u64 freq = armGetSystemTickFreq();
        u64 startTime = armGetSystemTick();
        size_t startSizeBuffered = 0;

        app::facade::SendInstallInfoText("inst.info_page.installing"_lang + ncaFileName + "...");
        app::facade::SendInstallProgress(0);
        while (fileOff < ncaSize)
        {
            this->UpdateTransferProgress(startSizeBuffered, fileOff, ncaSize, startTime, freq);

            if (fileOff + readSize >= ncaSize)
            {
                readSize = ncaSize - fileOff;
            }

            this->BufferData(readBuffer.get() + prependedSize, fileOff + fileStart + prependedSize, readSize - prependedSize);
            writer.write(readBuffer.get(), readSize);
            if (prependedSize != 0)
            {
                prependedSize = 0;
            }

            fileOff += readSize;
        }
        app::facade::SendInstallProgress(100);

        std::vector<u8> hash(SHA256_HASH_SIZE);
        writer.close(hash.data());
        std::string ncaIdStr = nx::ncm::GetContentIdString(ncaId);
        m_hashMap[ncaIdStr] = hash;
    }
}
