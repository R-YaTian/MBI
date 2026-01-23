#pragma once

#include <string>
#include <switch/types.h>
#include "nx/ncm.hpp"
#include "nx/xfs0.hpp"

namespace nx
{
    class Content
    {
        public:
            enum class Type
            {
                NSP,
                XCI
            };

            virtual ~Content() = default;
            virtual Type GetType() const = 0;

            virtual const XFS0BaseHeader* GetBaseHeader() = 0;
            virtual const u64 GetDataOffset() = 0;
            virtual const u64 GetFileEntrySize(const void *fileEntry) = 0;
            virtual const u64 GetFileEntryOffset(const void *fileEntry) = 0;
            virtual const void* GetFileEntry(unsigned int index) = 0;
            virtual const char* GetFileEntryName(const void* fileEntry) = 0;

            const void* GetFileEntryByName(std::string name);
            const void* GetFileEntryByNcaId(const NcmContentId& ncaId);
            std::vector<const void*> GetFileEntriesByExtension(std::string extension);
    };
}
