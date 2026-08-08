#pragma once

#include <map>
#include <vector>
#include <memory>
#include <string>
#include <atomic>
#include "nx/content.hpp"
#include "nx/BufferedPlaceholderWriter.hpp"

namespace app::install
{
    struct ThreadData
    {
        nx::data::BufferedPlaceholderWriter* bufferedPlaceholderWriter = nullptr;
        u64 dataOffset = 0;
        u64 dataSize = 0;
        void* in = nullptr;
        std::string* errorMessage = nullptr;
    };

    class Worker
    {
        public:
            virtual ~Worker() = default;
            virtual void StreamToPlaceholder(std::shared_ptr<nx::ncm::ContentStorage>& contentStorage, NcmContentId ncaId, nx::nca::NcaHeader* header = nullptr) = 0;
            virtual void BufferData(void* buf, off_t offset, size_t size) = 0;
            virtual void ReadThread(void* in);

            void WriteToPlaceholderBuffered(std::shared_ptr<nx::ncm::ContentStorage>& contentStorage, NcmContentId ncaId, void* threadDataIn, nx::nca::NcaHeader* header = nullptr);
            void WriteToPlaceholderDirectly(std::shared_ptr<nx::ncm::ContentStorage>& contentStorage, NcmContentId ncaId, const u64 maxBufferSize, nx::nca::NcaHeader* header = nullptr);
            void RetrieveHeader();
            nx::Content* GetContent() { return m_content.get(); }
            const nx::Content* GetContent() const { return m_content.get(); }

            const u8* GetHashByContentIdString(const std::string& ncaId) const
            {
                auto it = m_hashMap.find(ncaId);
                if (it != m_hashMap.end())
                {
                    return it->second.data();
                }
                return nullptr;
            }
            const std::map<std::string, std::vector<u8>>& GetHashMap() const { return m_hashMap; }
        protected:
            explicit Worker(std::unique_ptr<nx::Content> content) : m_content(std::move(content)) {}

            std::unique_ptr<nx::Content> m_content;
            std::map<std::string, std::vector<u8>> m_hashMap;

            std::atomic<bool> stopThreads{false};
        private:
            void PlaceholderWrite(void* in);
            void UpdateTransferProgress(size_t& startSize, size_t newSize, size_t totalSize, u64& startTime, u64 freq);
    };
}
