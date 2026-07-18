/*
Copyright (c) 2017-2018 Adubbz

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "nx/ncm.hpp"
#include "nx/error.hpp"
#include "nx/fs.hpp"
#include <string.h>

namespace nx::ncm
{
    ContentStorage::ContentStorage(NcmStorageId storageId) : m_storageId(storageId)
    {
        ASSERT_OK(ncmOpenContentStorage(&m_contentStorage, storageId), "Failed to open NCM ContentStorage");
    }

    ContentStorage::~ContentStorage()
    {
        serviceClose(&m_contentStorage.s);
    }

    void ContentStorage::CreatePlaceholder(const NcmContentId &placeholderId, const NcmPlaceHolderId &registeredId, size_t size)
    {
        ASSERT_OK(ncmContentStorageCreatePlaceHolder(&m_contentStorage, &placeholderId, &registeredId, size), "Failed to create placeholder");
    }

    void ContentStorage::DeletePlaceholder(const NcmPlaceHolderId &placeholderId)
    {
        ASSERT_OK(ncmContentStorageDeletePlaceHolder(&m_contentStorage, &placeholderId), "Failed to delete placeholder");
    }

    void ContentStorage::WritePlaceholder(const NcmPlaceHolderId &placeholderId, u64 offset, void *buffer, size_t bufSize)
    {
        ASSERT_OK(ncmContentStorageWritePlaceHolder(&m_contentStorage, &placeholderId, offset, buffer, bufSize), "Failed to write to placeholder");
    }

    void ContentStorage::Register(const NcmPlaceHolderId &placeholderId, const NcmContentId &registeredId)
    {
        ASSERT_OK(ncmContentStorageRegister(&m_contentStorage, &registeredId, &placeholderId), "Failed to register placeholder NCA");
    }

    bool ContentStorage::Delete(const NcmContentId &registeredId)
    {
        if (!this->Has(registeredId))
        {
            return false;
        }
        ASSERT_OK(ncmContentStorageDelete(&m_contentStorage, &registeredId), "Failed to delete registered NCA");
        return true;
    }

    bool ContentStorage::Has(const NcmContentId &registeredId)
    {
        bool hasNCA = false;
        ASSERT_OK(ncmContentStorageHas(&m_contentStorage, &hasNCA, &registeredId), "Failed to check if NCA is present");
        return hasNCA;
    }

    std::string ContentStorage::GetPath(const NcmContentId &registeredId)
    {
        char pathBuf[FS_MAX_PATH] = {0};
        ASSERT_OK(ncmContentStorageGetPath(&m_contentStorage, pathBuf, FS_MAX_PATH, &registeredId), "Failed to get installed NCA path");
        return std::string(pathBuf);
    }

    std::vector<NcmContentId> ContentStorage::ListContentId()
    {
        std::vector<NcmContentId> contentIds;
        s32 contentCount = 0;
        ASSERT_OK(ncmContentStorageGetContentCount(&m_contentStorage, &contentCount), "Failed to get content count");
        contentIds.resize(contentCount);

        s32 listCount = 0;
        ASSERT_OK(ncmContentStorageListContentId(&m_contentStorage, contentIds.data(), contentCount, &listCount, 0), "Failed to list content IDs");
        if (contentCount != listCount)
        {
            THROW_FORMAT("Failed to list content IDs");
        }

        return contentIds;
    }

    void ContentStorage::CleanupAllPlaceHolder()
    {
        ASSERT_OK(ncmContentStorageCleanupAllPlaceHolder(&m_contentStorage), "Failed to cleanup all placeholder");
    }

    ContentMeta::ContentMeta()
    {
        m_bytes.Resize(sizeof(NcmExtPackagedContentMetaHeader));
    }

    ContentMeta::ContentMeta(u8* data, size_t size, std::string cnmtFileName) :
        m_bytes(size)
    {
        if (size < sizeof(NcmExtPackagedContentMetaHeader))
            THROW_FORMAT("Content meta data size is too small!");

        m_bytes.Resize(size);
        memcpy(m_bytes.GetData(), data, size);
        m_cnmtFileName = cnmtFileName;
    }

    NcmExtPackagedContentMetaHeader ContentMeta::GetPackagedContentMetaHeader()
    {
        return m_bytes.Read<NcmExtPackagedContentMetaHeader>(0);
    }

    NcmContentMetaKey ContentMeta::GetContentMetaKey()
    {
        NcmContentMetaKey metaRecord;
        NcmExtPackagedContentMetaHeader contentMetaHeader = this->GetPackagedContentMetaHeader();

        memset(&metaRecord, 0, sizeof(NcmContentMetaKey));
        metaRecord.id = contentMetaHeader.id;
        metaRecord.version = contentMetaHeader.version;
        metaRecord.type = static_cast<NcmContentMetaType>(contentMetaHeader.type);

        return metaRecord;
    }

    void ContentMeta::SetupPackagedContentMeta()
    {
        NcmExtPackagedContentMetaHeader contentMetaHeader = this->GetPackagedContentMetaHeader();
        NcmPackagedContentInfo* packagedContentInfos = (NcmPackagedContentInfo*)(m_bytes.GetData() + sizeof(NcmExtPackagedContentMetaHeader) + contentMetaHeader.extended_header_size);

        for (u16 i = 0; i < contentMetaHeader.content_count; i++)
        {
            NcmPackagedContentInfo packagedContentInfo = packagedContentInfos[i];

            // Don't install delta fragments. Even patches don't seem to install them.
            if (packagedContentInfo.info.content_type < NcmContentType_DeltaFragment)
            {
                m_packagedContentInfos.push_back(packagedContentInfo);
            }
        }
    }

    std::vector<NcmContentInfo> ContentMeta::GetContentInfos()
    {
        std::vector<NcmContentInfo> contentInfos;

        for (unsigned int i = 0; i < m_packagedContentInfos.size(); i++)
        {
            contentInfos.push_back(m_packagedContentInfos[i].info);
        }

        return contentInfos;
    }

    void ContentMeta::GetInstallContentMeta(nx::data::ByteBuffer& installContentMetaBuffer, NcmContentInfo& cnmtNcmContentInfo, bool ignoreReqFirmVersion)
    {
        NcmExtPackagedContentMetaHeader packagedContentMetaHeader = this->GetPackagedContentMetaHeader();
        std::vector<NcmContentInfo> contentInfos = this->GetContentInfos();

        // Setup the content meta header
        NcmContentMetaHeader contentMetaHeader;
        contentMetaHeader.extended_header_size = packagedContentMetaHeader.extended_header_size;
        contentMetaHeader.content_count = contentInfos.size() + 1; // Add one for the cnmt content record
        contentMetaHeader.content_meta_count = packagedContentMetaHeader.content_meta_count;
        contentMetaHeader.attributes = packagedContentMetaHeader.attributes; // Sparse Titles use 0x04 not 0x0
        contentMetaHeader.storage_id = packagedContentMetaHeader.storage_id;

        installContentMetaBuffer.Append<NcmContentMetaHeader>(contentMetaHeader);

        // Setup the meta extended header
        LOG_DEBUG("Install content meta pre size: 0x%lx\n", installContentMetaBuffer.GetSize());
        installContentMetaBuffer.Resize(installContentMetaBuffer.GetSize() + contentMetaHeader.extended_header_size);
        LOG_DEBUG("Install content meta post size: 0x%lx\n", installContentMetaBuffer.GetSize());
        auto* extendedHeaderSourceBytes = m_bytes.GetData() + sizeof(NcmExtPackagedContentMetaHeader);
        u8* installExtendedHeaderStart = installContentMetaBuffer.GetData() + sizeof(NcmContentMetaHeader);
        memcpy(installExtendedHeaderStart, extendedHeaderSourceBytes, contentMetaHeader.extended_header_size);

        // Optionally disable the required system version field
        if (ignoreReqFirmVersion && (packagedContentMetaHeader.type == NcmContentMetaType_Application || packagedContentMetaHeader.type == NcmContentMetaType_Patch))
        {
            installContentMetaBuffer.Write<u32>(0, sizeof(NcmContentMetaHeader) + 8);
        }

        // Setup cnmt content record
        installContentMetaBuffer.Append<NcmContentInfo>(cnmtNcmContentInfo);

        // Setup the content records
        for (auto& contentInfo : contentInfos)
        {
            installContentMetaBuffer.Append<NcmContentInfo>(contentInfo);
        }

        if (packagedContentMetaHeader.type == NcmContentMetaType_Patch)
        {
            NcmPatchMetaExtendedHeader* patchMetaExtendedHeader = (NcmPatchMetaExtendedHeader*)extendedHeaderSourceBytes;
            installContentMetaBuffer.Resize(installContentMetaBuffer.GetSize() + patchMetaExtendedHeader->extended_data_size);
        }
        else if (packagedContentMetaHeader.type == NcmContentMetaType_Delta)
        {
            NcmExtDeltaMetaExtendedHeader* deltaMetaExtendedHeader = (NcmExtDeltaMetaExtendedHeader*)extendedHeaderSourceBytes;
            installContentMetaBuffer.Resize(installContentMetaBuffer.GetSize() + deltaMetaExtendedHeader->extended_data_size);
        }
    }

    const u8* ContentMeta::GetHashByContentId(const NcmContentId& ncaId) const
    {
        for (const auto& packagedContentInfo : m_packagedContentInfos)
        {
            if (memcmp(&packagedContentInfo.info.content_id, &ncaId, sizeof(NcmContentId)) == 0)
            {
                return packagedContentInfo.hash;
            }
        }
        return nullptr;
    }

    void ContentMeta::RebuildNcaToInstall(const NcmStorageId& destStorageId, const std::map<std::string, std::vector<u8>>& hashMap)
    {
        NcmExtPackagedContentMetaHeader contentMetaHeader = m_bytes.Read<NcmExtPackagedContentMetaHeader>(0);
        data::ByteBuffer cnmtBuffer;
        cnmtBuffer.Resize(m_bytes.GetSize());
        std::memcpy(cnmtBuffer.GetData(), m_bytes.GetData(), m_bytes.GetSize());
        NcmPackagedContentInfo* packagedContentInfos = (NcmPackagedContentInfo*)(cnmtBuffer.GetData() + sizeof(NcmExtPackagedContentMetaHeader) + contentMetaHeader.extended_header_size);

        for (u16 i = 0; i < contentMetaHeader.content_count; i++)
        {
            if (packagedContentInfos[i].info.content_type > 5)
            {
                continue;
            }
            std::string ncaIdStr = GetContentIdString(packagedContentInfos[i].info.content_id);
            auto it = hashMap.find(ncaIdStr);
            if (it != hashMap.end())
            {
                std::memcpy(packagedContentInfos[i].hash, it->second.data(), SHA256_HASH_SIZE);
            }
        }

        std::vector<nca::FileEntry> entries;
        nca::FileEntry entry;
        entry.name = m_cnmtFileName;
        entry.data.resize(cnmtBuffer.GetSize());
        std::memcpy(entry.data.data(), cnmtBuffer.GetData(), cnmtBuffer.GetSize());
        entries.emplace_back(entry);

        nca::NcaHeader ncaHeader = m_ncaHeader;
        data::ByteIO ncaBuf;
        ncaBuf.write(&ncaHeader, sizeof(nca::NcaHeader));
        nca::BuildNcaByHeader(ncaHeader, 0, entries, 0x1000, ncaBuf);

        nx::ncm::ContentStorage contentStorage(destStorageId);
        try { contentStorage.DeletePlaceholder(*(NcmPlaceHolderId*)&m_contentId); } catch (...) {}
        contentStorage.CreatePlaceholder(m_contentId, *(NcmPlaceHolderId*)&m_contentId, ncaBuf.buf.size());
        contentStorage.WritePlaceholder(*(NcmPlaceHolderId*)&m_contentId, 0, ncaBuf.buf.data(), ncaBuf.buf.size());
        contentStorage.Delete(m_contentId);
        contentStorage.Register(*(NcmPlaceHolderId*)&m_contentId, m_contentId);
        try { contentStorage.DeletePlaceholder(*(NcmPlaceHolderId*)&m_contentId); } catch (...) {}
    }

    bool CleanupPlaceHolder(const NcmStorageId& storageId)
    {
        try
        {
            ContentStorage contentStorage(storageId);
            contentStorage.CleanupAllPlaceHolder();
        }
        catch (const std::exception& e)
        {
            return false;
        }

        return true;
    }

    s32 DeleteOrphanContent(const NcmStorageId& storageId, s32* outContentCount)
    {
        s32 orphanedContentCount = 0;
        ContentStorage contentStorage(storageId);
        std::vector<NcmContentId> contentIds;
        s32 contentCount = 0;
        try
        {
            contentIds = contentStorage.ListContentId();
        }
        catch (const std::exception& e)
        {
            LOG_DEBUG("Failed to list content IDs for storage ID %d: %s\n", storageId, e.what());
            return orphanedContentCount;
        }
        contentCount = contentIds.size();
        std::unique_ptr<bool[]> orphaned(new bool[contentCount]);

        NcmContentMetaDatabase db = {};
        ASSERT_OK(ncmOpenContentMetaDatabase(std::addressof(db), storageId), "Failed to open content meta database");
        ASSERT_OK(ncmContentMetaDatabaseLookupOrphanContent(std::addressof(db), orphaned.get(), contentIds.data(), contentCount), "Failed to lookup orphan content");
        ncmContentMetaDatabaseClose(std::addressof(db));
        for (s32 i = 0; i < contentCount; i++)
        {
            if (orphaned[i])
            {
                LOG_DEBUG("Found orphan content ID: %s\n", GetContentIdString(contentIds[i]).c_str());
                contentStorage.Delete(contentIds[i]);
                ++orphanedContentCount;
            }
        }

        if (outContentCount != nullptr)
        {
            *outContentCount = contentCount;
        }

        return orphanedContentCount;
    }

    ContentMeta GetContentMetaFromNCA(const std::string& ncaPath)
    {
        // Create the cnmt filesystem
        nx::fs::IFileSystem cnmtNCAFileSystem;
        cnmtNCAFileSystem.OpenFileSystemWithId(ncaPath, FsFileSystemType_ContentMeta, 0);
        nx::fs::SimpleFileSystem cnmtNCASimpleFileSystem(cnmtNCAFileSystem, "/", ncaPath + "/");

        // Find and read the cnmt file
        auto cnmtName = cnmtNCASimpleFileSystem.GetFileNameFromExtension("", "cnmt");
        auto cnmtFile = cnmtNCASimpleFileSystem.OpenFile(cnmtName);
        u64 cnmtSize = cnmtFile.GetSize();

        nx::data::ByteBuffer cnmtBuf;
        cnmtBuf.Resize(cnmtSize);
        cnmtFile.Read(0x0, cnmtBuf.GetData(), cnmtSize);

        return ContentMeta(cnmtBuf.GetData(), cnmtBuf.GetSize(), cnmtName);
    }

    u64 GetBaseTitleId(u64 titleId, NcmContentMetaType contentMetaType)
    {
        switch (contentMetaType)
        {
            case NcmContentMetaType_Patch:
                return titleId ^ 0x800;

            case NcmContentMetaType_AddOnContent:
                return (titleId ^ 0x1000) & ~0xFFF;

            default:
                return titleId;
        }
    }

    constexpr auto CONTENT_ID_STRING_SIZE = 32;

    std::string GetContentIdString(const NcmContentId& id)
    {
        char idStr[CONTENT_ID_STRING_SIZE + 1] = {0};
        u64 idLower = __bswap64(*(u64 *)id.c);
        u64 idUpper = __bswap64(*(u64 *)(id.c + 0x8));
        std::snprintf(idStr, sizeof(idStr), "%016lx%016lx", idLower, idUpper);
        return std::string(idStr);
    }

    NcmContentId GetContentIdFromString(const std::string& idStr)
    {
        NcmContentId contentId = {0};
        char lowerU64[17] = {0};
        char upperU64[17] = {0};
        memcpy(lowerU64, idStr.c_str(), 16);
        memcpy(upperU64, idStr.c_str() + 16, 16);

        *(u64 *)contentId.c = __bswap64(strtoul(lowerU64, NULL, 16));
        *(u64 *)(contentId.c + 8) = __bswap64(strtoul(upperU64, NULL, 16));

        return contentId;
    }
}
