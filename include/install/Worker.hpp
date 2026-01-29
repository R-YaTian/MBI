#pragma once

#include <map>
#include <vector>
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
            void ClearHashMap() { m_hashMap.clear(); }
        protected:
            explicit Worker(std::unique_ptr<nx::Content> content) : m_content(std::move(content)) {}

            std::unique_ptr<nx::Content> m_content;
            std::map<std::string, std::vector<u8>> m_hashMap;
    };
}
