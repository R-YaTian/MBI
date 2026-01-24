#pragma once

#include "nx/BufferedPlaceholderWriter.hpp"
#include "install/Worker.hpp"
#include <atomic>

namespace app::install
{
    class UsbWorker : public Worker
    {
        public:
            UsbWorker(std::unique_ptr<nx::Content> content, const std::string& filename);
            ~UsbWorker();

            void StreamToPlaceholder(std::shared_ptr<nx::ncm::ContentStorage>& contentStorage, NcmContentId ncaId) override;
            void BufferData(void* buf, off_t offset, size_t size) override;

        private:
            std::string m_fileName;

            struct USBArgs
            {
                std::string fileName;
                nx::data::BufferedPlaceholderWriter* bufferedPlaceholderWriter;
                u64 xfs0Offset;
                u64 ncaSize;
            };

            std::atomic<bool> stopThreads{false};
            std::string errorMessage;

            void USBReadThread(void* in);
            void PlaceholderWrite(void* in);
    };
}
