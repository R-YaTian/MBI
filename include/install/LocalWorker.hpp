#pragma once

#include "install/Worker.hpp"

namespace app::install
{
    class LocalWorker : public Worker
    {
        public:
            LocalWorker(std::unique_ptr<nx::Content> content, const std::string& path);
            ~LocalWorker();

            void StreamToPlaceholder(std::shared_ptr<nx::ncm::ContentStorage>& contentStorage, NcmContentId ncaId) override;
            void BufferData(void* buf, off_t offset, size_t size) override;

        private:
            FILE* m_file = nullptr;
    };
}
