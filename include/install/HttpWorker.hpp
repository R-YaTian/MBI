#pragma once

#include "nx/network.hpp"
#include "install/Worker.hpp"

namespace app::install
{
    class HttpWorker : public Worker
    {
        public:
            HttpWorker(std::unique_ptr<nx::Content> content, const std::string& url);
            ~HttpWorker();

            void StreamToPlaceholder(std::shared_ptr<nx::ncm::ContentStorage>& contentStorage, NcmContentId ncaId, nx::nca::NcaHeader* header = nullptr) override;
            void BufferData(void* buf, off_t offset, size_t size) override;
            void ReadThread(void* in) override;
            void PlaceholderWrite(void* in) override;

        private:
            nx::network::HTTPDownload m_download;
    };
}
