#pragma once

#include "install/Worker.hpp"

namespace app::install
{
    class UsbWorker : public Worker
    {
        public:
            UsbWorker(std::unique_ptr<nx::Content> content, const std::string& filename);
            ~UsbWorker();

            void StreamToPlaceholder(std::shared_ptr<nx::ncm::ContentStorage>& contentStorage, NcmContentId ncaId, nx::nca::NcaHeader* header = nullptr) override;
            void BufferData(void* buf, off_t offset, size_t size) override;
            void ReadThread(void* in) override;

        private:
            std::string m_fileName;
    };
}
