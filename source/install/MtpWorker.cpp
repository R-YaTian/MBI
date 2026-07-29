#include "install/MtpWorker.hpp"
#include "util/ScopedMutex.hpp"
#include "util/i18n.hpp"
#include "nx/NcaWriter.hpp"
#include "nx/error.hpp"
#include "nx/fs.hpp"
#include "facade.hpp"
#include <sstream>

namespace app::install
{
    constexpr u64 MAX_BUFFER_SIZE = 1024ULL*1024ULL*1ULL;

    MtpWorker::MtpWorker(std::unique_ptr<nx::Content> content, const std::string &path)
        : Worker(std::move(content))
    {
        m_active = true;
        m_offset = 0;
        m_buffer.reserve(MAX_BUFFER_SIZE);
        INSTALL_STATE = MTPInstallState::None;

        mutexInit(&m_mutex);
        condvarInit(&m_can_read);
        condvarInit(&m_can_write);
    }

    MtpWorker::~MtpWorker() = default;

    void MtpWorker::ReadChunk(void* _buf, s64 size, u64* bytes_read)
    {
        auto buf = static_cast<u8*>(_buf);
        *bytes_read = 0;

        while (size)
        {
            SCOPED_MUTEX(&m_mutex);
            if (m_active && m_buffer.empty())
            {
                condvarWait(std::addressof(m_can_read), std::addressof(m_mutex));
            }

            if (!m_active && m_buffer.empty())
            {
                break;
            }

            const auto rsize = std::min<s64>(size, m_buffer.size());
            std::memcpy(buf, m_buffer.data(), rsize);
            m_buffer.erase(m_buffer.begin(), m_buffer.begin() + rsize);
            condvarWakeOne(&m_can_write);

            size -= rsize;
            buf += rsize;
            *bytes_read += rsize;
        }
    }

    void MtpWorker::StreamToPlaceholder(std::shared_ptr<nx::ncm::ContentStorage>& contentStorage, NcmContentId ncaId, nx::nca::NcaHeader* header)
    {
        const void* fileEntry = m_content->GetFileEntryByNcaId(ncaId);
        std::string ncaFileName = m_content->GetFileEntryName(fileEntry);
        u64 ncaSize = m_content->GetFileEntrySize(fileEntry);

        NcaWriter writer(ncaId, contentStorage);

        u64 fileStart = m_content->GetFileEntryOffset(fileEntry);
        u64 fileOff = 0;
        size_t prependedSize = 0;
        size_t readSize = 0x400000; // 4MB buff
        auto readBuffer = std::make_unique<u8[]>(readSize);
        if (header != nullptr)
        {
            prependedSize = sizeof(nx::nca::NcaHeader);
            std::memcpy(readBuffer.get(), header, prependedSize);
        }

        u64 freq = armGetSystemTickFreq();
        u64 startTime = armGetSystemTick();
        size_t startSizeBuffered = 0;
        double speed = 0.0;

        float progress;
        try
        {
            app::facade::SendInstallInfoText("inst.info_page.top_info0"_lang + ncaFileName + "...");
            app::facade::SendInstallProgress(0);
            while (fileOff < ncaSize)
            {
                progress = (float) fileOff / (float) ncaSize;
                u64 newTime = armGetSystemTick();

                if (fileOff % (0x400000 * 3) == 0)
                {
                    size_t newSizeBuffered = fileOff;
                    double mbBuffered = (newSizeBuffered / 1000000.0) - (startSizeBuffered / 1000000.0);
                    double duration = ((double)(newTime - startTime) / (double)freq);
                    speed =  mbBuffered / duration;
                    startTime = newTime;
                    startSizeBuffered = newSizeBuffered;

                    LOG_DEBUG("> Progress: %lu/%lu MB (%d%s)\r", (fileOff / 1000000), (ncaSize / 1000000), (int)(progress * 100.0), "%");
                    app::facade::SendInstallProgress((double)(progress * 100.0));
                    std::stringstream x;
                    x << (int)(progress * 100.0);
                    std::stringstream speedStr;
                    speedStr << std::fixed << std::setprecision(2) << speed;
                    app::facade::SendInstallBarText(x.str() + "% " + "inst.info_page.at"_lang + speedStr.str() + "MB/s");
                }

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
        }
        catch (std::exception& e)
        {
            LOG_DEBUG("something went wrong: %s\n", e.what());
        }

        std::vector<u8> hash(SHA256_HASH_SIZE);
        writer.close(hash.data());
        std::string ncaIdStr = nx::ncm::GetContentIdString(ncaId);
        m_hashMap[ncaIdStr] = hash;
    }

    void MtpWorker::BufferData(void* _buf, off_t offset, size_t size)
    {
        if (offset < m_offset)
        {
            THROW_FORMAT("streams don't allow for random access (seeking backwards)");
        }

        auto buf = static_cast<u8*>(_buf);

        // check if we already have some data in the buffer.
        while (size)
        {
            // while it is invalid to seek backwards, it is valid to seek forwards.
            // this can be done to skip padding, skip undeeded files etc.
            // to handle this, simply read the data into a buffer and discard it.
            if (offset > m_offset)
            {
                const auto skip_size = offset - m_offset;
                std::vector<u8> temp_buf(skip_size);
                u64 bytes_read;
                ReadChunk(temp_buf.data(), temp_buf.size(), &bytes_read);

                m_offset += bytes_read;
            } else {
                u64 bytes_read;
                ReadChunk(buf, size, &bytes_read);

                buf += bytes_read;
                offset += bytes_read;
                m_offset += bytes_read;
                size -= bytes_read;
            }
        }
    }

    bool MtpWorker::Push(const void* _buf, s64 size)
    {
        auto buf = static_cast<const u8*>(_buf);
        if (!size)
        {
            return true;
        }

        while (size)
        {
            SCOPED_MUTEX(&m_mutex);
            if (m_active && m_buffer.size() >= MAX_BUFFER_SIZE)
            {
                condvarWait(std::addressof(m_can_write), std::addressof(m_mutex));
            }

            if (!m_active)
            {
                break;
            }

            const auto wsize = std::min<s64>(size, MAX_BUFFER_SIZE - m_buffer.size());
            const auto offset = m_buffer.size();
            m_buffer.resize(offset + wsize);

            std::memcpy(m_buffer.data() + offset, buf, wsize);
            condvarWakeOne(&m_can_read);

            size -= wsize;
            buf += wsize;
            if (!size)
            {
                return true;
            }
        }

        if (INSTALL_STATE == MTPInstallState::Finished)
        {
            return true;
        }

        return false;
    }

    void MtpWorker::Disable()
    {
        SCOPED_MUTEX(&m_mutex);
        m_active = false;
        condvarWakeOne(&m_can_read);
        condvarWakeOne(&m_can_write);
    }
}
