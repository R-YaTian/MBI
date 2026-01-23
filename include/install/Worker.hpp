#pragma once

#include <memory>
#include "nx/content.hpp"

namespace app::install
{
    class Worker
    {
        public:
            virtual ~Worker() = default;
            virtual void StreamToPlaceholder(std::shared_ptr<nx::ncm::ContentStorage>& contentStorage, NcmContentId ncaId) = 0;
            virtual void BufferData(void* buf, off_t offset, size_t size) = 0;

            void RetrieveHeader();
            nx::Content* GetContent() { return m_content.get(); }
            const nx::Content* GetContent() const { return m_content.get(); }

        protected:
            explicit Worker(std::unique_ptr<nx::Content> content) : m_content(std::move(content)) {}

            std::unique_ptr<nx::Content> m_content;
    };
}
