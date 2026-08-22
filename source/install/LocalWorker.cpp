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
        this->WriteToPlaceholderDirectly(contentStorage, ncaId, DEFAULT_READ_BUFFER_SIZE, header);
    }

    void LocalWorker::BufferData(void* buf, off_t offset, size_t size)
    {
        fseeko(m_file, offset, SEEK_SET);
        fread(buf, 1, size, m_file);
    }
}
