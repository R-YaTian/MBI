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
    ContentStorage::ContentStorage(NcmStorageId storageId) 
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

    void ContentStorage::Delete(const NcmContentId &registeredId)
    {
        ASSERT_OK(ncmContentStorageDelete(&m_contentStorage, &registeredId), "Failed to delete registered NCA");
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

    ContentMeta::ContentMeta()
    {
        m_bytes.Resize(sizeof(NcmExtPackagedContentMetaHeader));
    }

    ContentMeta::ContentMeta(u8* data, size_t size) :
        m_bytes(size)
    {
        if (size < sizeof(NcmExtPackagedContentMetaHeader))
            THROW_FORMAT("Content meta data size is too small!");

        m_bytes.Resize(size);
        memcpy(m_bytes.GetData(), data, size);
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

    std::vector<NcmContentInfo> ContentMeta::GetContentInfos()
    {
        NcmExtPackagedContentMetaHeader contentMetaHeader = this->GetPackagedContentMetaHeader();

        std::vector<NcmContentInfo> contentInfos;
        PackagedContentInfo* packagedContentInfos = (PackagedContentInfo*)(m_bytes.GetData() + sizeof(NcmExtPackagedContentMetaHeader) + contentMetaHeader.extended_header_size);

        for (unsigned int i = 0; i < contentMetaHeader.content_count; i++)
        {
            PackagedContentInfo packagedContentInfo = packagedContentInfos[i];

            // Don't install delta fragments. Even patches don't seem to install them.
            if (static_cast<u8>(packagedContentInfo.content_info.content_type) <= 5)
            {
                contentInfos.push_back(packagedContentInfo.content_info); 
            }
        }

        return contentInfos;
    }

    void ContentMeta::GetInstallContentMeta(app::data::ByteBuffer& installContentMetaBuffer, NcmContentInfo& cnmtNcmContentInfo, bool ignoreReqFirmVersion)
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

        app::data::ByteBuffer cnmtBuf;
        cnmtBuf.Resize(cnmtSize);
        cnmtFile.Read(0x0, cnmtBuf.GetData(), cnmtSize);

        return ContentMeta(cnmtBuf.GetData(), cnmtBuf.GetSize());
    }
}
