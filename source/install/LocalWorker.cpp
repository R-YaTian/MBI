#include "install/LocalWorker.hpp"
#include "nx/error.hpp"

namespace app::install
{
    LocalWorker::LocalWorker(std::unique_ptr<nx::Content> content, const std::string &path, const LocalStorageSource& storageSrc)
        : Worker(std::move(content))
    {
        m_storageSrc = storageSrc;
        m_file = fopen(path.c_str(), "rb");
        if (!m_file)
        {
            THROW_FORMAT("can't open file at %s\n", path.c_str());
        }
        RetrieveHeader();
    }

    LocalWorker::~LocalWorker()
    {
        if (m_file)
        {
            fclose(m_file);
        }
    }

    void LocalWorker::StreamToPlaceholder(std::shared_ptr<nx::ncm::ContentStorage>& contentStorage, NcmContentId ncaId, nx::nca::NcaHeader* header)
    {
        if (m_storageSrc == LocalStorageSource::UDISK)
        {
            this->WriteToPlaceholderBuffered(contentStorage, ncaId, (void *)this, header, appletGetAppletType() == AppletType_LibraryApplet ? 8 : 128);
        }
        else
        {
            this->WriteToPlaceholderDirectly(contentStorage, ncaId, DEFAULT_READ_BUFFER_SIZE, header);
        }
    }

    void LocalWorker::BufferData(void* buf, off_t offset, size_t size)
    {
        if (fseeko(m_file, offset, SEEK_SET) != 0)
        {
            THROW_FORMAT("File seek failed: %s\n", strerror(errno));
        }
        size_t readSize = fread(buf, 1, size, m_file);
        if (readSize != size)
        {
            THROW_FORMAT("File read failed at offset %ld\n", offset);
        }
    }
}
