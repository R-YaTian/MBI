#pragma once

#include <string>
#include <vector>
#include <switch/types.h>
#include "nx/ncm.hpp"
#include "nx/xfs0.hpp"

namespace nx
{
    enum class ContentCollectionType
    {
        ARCHIVE,
        META,
        TIK,
        CERT,
    };

    typedef union
    {
        NcmContentId content_id;
        FsRightsId rights_id;
    } ContentCollectionInfo;

    static_assert(sizeof(ContentCollectionInfo) == 0x10);

    struct ContentCollectionEntry
    {
        // collection name within file.
        std::string name{};
        // collection offset within file.
        s64 offset{};
        // collection size within file, may be compressed size.
        s64 size{};
        ContentCollectionType type{};
        ContentCollectionInfo info{};
    };

    using ContentCollections = std::vector<ContentCollectionEntry>;

    class Content
    {
        protected:
            ContentCollections m_collections;

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
            const ContentCollections& GetCollections();
    };
}
