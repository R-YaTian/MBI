#pragma once

#include "nx/BufferedPlaceholderWriter.hpp"
#include "nx/network.hpp"
#include "install/Worker.hpp"
#include <atomic>

namespace app::install
{
    class HttpWorker : public Worker
    {
        public:
            HttpWorker(std::unique_ptr<nx::Content> content, const std::string& url);
            ~HttpWorker();

            void StreamToPlaceholder(std::shared_ptr<nx::ncm::ContentStorage>& contentStorage, NcmContentId ncaId) override;
            void BufferData(void* buf, off_t offset, size_t size) override;

        private:
            nx::network::HTTPDownload m_download;

            struct StreamArgs
            {
                nx::network::HTTPDownload* download;
                nx::data::BufferedPlaceholderWriter* bufferedPlaceholderWriter;
                u64 xfs0Offset;
                u64 ncaSize;
            };

            std::atomic<bool> stopThreads{false};

            void CurlStreamThread(void* in);
            void PlaceholderWrite(void* in);
    };
}
