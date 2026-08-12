#include "install/MtpWorker.hpp"
#include "util/ScopedMutex.hpp"
#include "util/i18n.hpp"
#include "nx/error.hpp"

namespace app::install
{
    constexpr u64 MAX_BUFFER_SIZE_1MB = 1024ULL*1024ULL*1ULL;

    MtpWorker::MtpWorker(std::unique_ptr<nx::Content> content, std::stop_token token)
        : Worker(std::move(content))
    {
        m_token = token;
        m_active = true;
        m_offset = 0;
        m_buffer.reserve(MAX_BUFFER_SIZE_1MB);
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

        while (!m_token.stop_requested())
        {
            SCOPED_MUTEX(&m_mutex);
            if (m_active && m_buffer.empty())
            {
                Result rc = condvarWait(std::addressof(m_can_read), std::addressof(m_mutex));
                if (R_FAILED(rc))
                {
                    break;
                }
            }

            if ((!m_active && m_buffer.empty()) || m_token.stop_requested())
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
            if (!size)
            {
                return;
            }
        }

        throw std::runtime_error("inst.mtp.error"_lang.c_str());
    }

    void MtpWorker::StreamToPlaceholder(std::shared_ptr<nx::ncm::ContentStorage>& contentStorage, NcmContentId ncaId, nx::nca::NcaHeader* header)
    {
        if (appletGetAppletType() == AppletType_LibraryApplet)
        {
            this->WriteToPlaceholderDirectly(contentStorage, ncaId, DEFAULT_READ_BUFFER_SIZE, header);
        }
        else
        {
            this->WriteToPlaceholderBuffered(contentStorage, ncaId, (void *)this, header);
        }
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
                size_t skip_size = offset - m_offset;
                std::vector<u8> temp_buf(DEFAULT_READ_BUFFER_SIZE);

                while (skip_size)
                {
                    const auto chunk_size = std::min(skip_size, temp_buf.size());
                    u64 bytes_read;
                    ReadChunk(temp_buf.data(), chunk_size, &bytes_read);

                    m_offset += bytes_read;
                    skip_size -= bytes_read;
                }
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

        while (!m_token.stop_requested())
        {
            SCOPED_MUTEX(&m_mutex);
            if (m_active && m_buffer.size() >= MAX_BUFFER_SIZE_1MB)
            {
                condvarWait(std::addressof(m_can_write), std::addressof(m_mutex));
            }

            if (!m_active)
            {
                break;
            }

            const auto wsize = std::min<s64>(size, MAX_BUFFER_SIZE_1MB - m_buffer.size());
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
