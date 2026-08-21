#pragma once

#include "install/Worker.hpp"

namespace app::install
{
    enum class LocalStorageSource : u8
    {
        SDMC,
        UDISK
    };

    class LocalWorker : public Worker
    {
        public:
            LocalWorker(std::unique_ptr<nx::Content> content, const std::string& path, const LocalStorageSource& storageSrc);
            ~LocalWorker();

            void StreamToPlaceholder(std::shared_ptr<nx::ncm::ContentStorage>& contentStorage, NcmContentId ncaId, nx::nca::NcaHeader* header = nullptr) override;
            void BufferData(void* buf, off_t offset, size_t size) override;

        private:
            FILE* m_file = nullptr;
            LocalStorageSource m_storageSrc;
    };
}
