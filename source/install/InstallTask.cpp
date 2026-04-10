#include "install/InstallTask.hpp"
#include "nx/error.hpp"
#include "nx/nca.hpp"
#include "nx/ext.hpp"
#include "nx/Crypto.hpp"
#include "util/i18n.hpp"
#include "facade.hpp"
#include <thread>

namespace app
{
    InstallTask::InstallTask(NcmStorageId destStorageId, bool ignoreReqFirmVersion, bool fixTicket, bool skipBase, std::unique_ptr<app::install::Worker> worker) :
        m_destStorageId(destStorageId),
        m_ignoreReqFirmVersion(ignoreReqFirmVersion),
        m_fixTicket(fixTicket),
        m_skipBase(skipBase),
        m_contentMeta(),
        m_worker(std::move(worker))
    {
        appletSetMediaPlaybackState(true);
    }

    InstallTask::~InstallTask()
    {
        appletSetMediaPlaybackState(false);
    }

    void InstallTask::InstallContentMetaRecords(nx::data::ByteBuffer& installContentMetaBuf, int i)
    {
        NcmContentMetaDatabase contentMetaDatabase;
        NcmContentMetaKey contentMetaKey = m_contentMeta[i].GetContentMetaKey();

        try
        {
            ASSERT_OK(ncmOpenContentMetaDatabase(&contentMetaDatabase, m_destStorageId), "Failed to open content meta database");
            ASSERT_OK(ncmContentMetaDatabaseSet(&contentMetaDatabase, &contentMetaKey, (NcmContentMetaHeader*)installContentMetaBuf.GetData(), installContentMetaBuf.GetSize()), "Failed to set content records");
            ASSERT_OK(ncmContentMetaDatabaseCommit(&contentMetaDatabase), "Failed to commit content records");
        }
        catch (std::runtime_error& e)
        {
            serviceClose(&contentMetaDatabase.s);
            THROW_FORMAT(e.what());
        }

        serviceClose(&contentMetaDatabase.s);
    }

    void InstallTask::InstallApplicationRecord(int i)
    {
        NcmContentMetaType contentType = this->GetContentMetaType(i);
        const u64 baseTitleId = nx::ncm::GetBaseTitleId(this->GetTitleId(i), contentType);

        // Add our new content meta
        NsExtContentStorageMetaKey storageRecord;
        storageRecord.meta_key = m_contentMeta[i].GetContentMetaKey();
        storageRecord.storage_id = m_destStorageId;

        LOG_DEBUG("Pushing application record...\n");
        ASSERT_OK(nsextPushApplicationRecord(baseTitleId, NsExtApplicationEvent_Present, &storageRecord, 1), "Failed to push application record");
        if (contentType == NcmContentMetaType_Patch)
        {
            if (hosversionAtLeast(6,0,0))
            {
                ASSERT_OK(avmInitialize(), "Failed to initialize avm");
                ASSERT_OK(avmPushLaunchVersion(baseTitleId, storageRecord.meta_key.version), "avm: Failed to push launch version");
                ASSERT_OK(avmUpgradeLaunchRequiredVersion(baseTitleId, storageRecord.meta_key.version), "avm: Failed to upgrade launch required version");
                avmExit();
            }
            ASSERT_OK(nsextPushLaunchVersion(baseTitleId, storageRecord.meta_key.version), "Failed to push launch version");
        }
    }

    void InstallTask::RemoveInstalledNcas(int idx)
    {
        NcmContentMetaKey contentMetaKey = m_contentMeta[idx].GetContentMetaKey();
        NcmContentMetaType contentMetaType = static_cast<NcmContentMetaType>(contentMetaKey.type);
        const auto app_id = nx::ncm::GetBaseTitleId(contentMetaKey.id, contentMetaType);

        // remove current entries (if any).
        s32 db_list_total;
        s32 db_list_count;
        u64 id_min = contentMetaKey.id;
        u64 id_max = contentMetaKey.id;

        if (contentMetaType == NcmContentMetaType_Application && m_skipBase)
        {
            LOG_DEBUG("Skipping base NCAs removal\n");
            return;
        }

        // if installing a patch, remove all previously installed ncas.
        if (contentMetaType == NcmContentMetaType_Patch)
        {
            id_min = 0;
            id_max = UINT64_MAX;
        }

        const NcmStorageId storageIDs[] { NcmStorageId_SdCard, NcmStorageId_BuiltInUser };
        for (size_t i = 0; i < std::size(storageIDs); i++)
        {
            NcmContentMetaDatabase db = {};
            ASSERT_OK(ncmOpenContentMetaDatabase(std::addressof(db), storageIDs[i]), "Failed to open content meta database");
            nx::ncm::ContentStorage contentStorage(storageIDs[i]);

            std::vector<NcmContentMetaKey> keys(1);
            ASSERT_OK(ncmContentMetaDatabaseList(std::addressof(db), std::addressof(db_list_total), std::addressof(db_list_count), keys.data(), keys.size(), contentMetaType, app_id, id_min, id_max, NcmContentInstallType_Full), "Failed to list content meta database");

            if ((size_t)db_list_total != keys.size())
            {
                keys.resize(db_list_total);
                if (keys.size())
                {
                    ASSERT_OK(ncmContentMetaDatabaseList(std::addressof(db), std::addressof(db_list_total), std::addressof(db_list_count), keys.data(), keys.size(), contentMetaType, app_id, id_min, id_max, NcmContentInstallType_Full), "Failed to list content meta database");
                }
            }

            for (const auto& key : keys)
            {
                LOG_DEBUG("found key: 0x%016lX type: %u version: %u\n", key.id, key.type, key.version);
                NcmContentMetaHeader header;
                u64 out_size;
                ASSERT_OK(ncmContentMetaDatabaseGet(std::addressof(db), std::addressof(key), std::addressof(out_size), std::addressof(header), sizeof(header)), "Unable to fetch header from ncm database");
                if (out_size != sizeof(header))
                {
                    THROW_FORMAT("Unable to fetch header from ncm database");
                }

                std::vector<NcmContentInfo> infos(header.content_count);
                s32 content_info_out;
                ASSERT_OK(ncmContentMetaDatabaseListContentInfo(std::addressof(db), std::addressof(content_info_out), infos.data(), infos.size(), std::addressof(key), 0), "Unable to get infos from ncm database");
                if ((size_t)content_info_out != infos.size())
                {
                    THROW_FORMAT("Unable to get infos from ncm database");
                }

                for (const auto& info : infos)
                {
                    // Skip delete current meta nca
                    if (!std::memcmp(&info.content_id, &m_contentMeta[idx].GetContentId(), sizeof(info.content_id)))
                    {
                        continue;
                    }

                    if (contentStorage.Delete(info.content_id))
                    {
                        app::facade::SendInstallInfoText("inst.info_page.removing"_lang + nx::ncm::GetContentIdString(info.content_id));
                    }
                }

                ASSERT_OK(ncmContentMetaDatabaseRemove(std::addressof(db), std::addressof(key)), "Failed to remove content records");
                ASSERT_OK(ncmContentMetaDatabaseCommit(std::addressof(db)), "Failed to commit content records");
            }
            ncmContentMetaDatabaseClose(std::addressof(db));
        }
    }

    // Validate and obtain all data needed for install
    void InstallTask::Prepare()
    {
        std::vector<std::tuple<nx::ncm::ContentMeta, NcmContentInfo>> tupleList = this->ReadContentMeta();

        for (size_t i = 0; i < tupleList.size(); i++)
        {
            std::tuple<nx::ncm::ContentMeta, NcmContentInfo> cnmtTuple = tupleList[i];

            m_contentMeta.push_back(std::get<0>(cnmtTuple));
            NcmContentInfo cnmtContentRecord = std::get<1>(cnmtTuple);

            nx::ncm::ContentStorage contentStorage(m_destStorageId);
            if (!contentStorage.Has(cnmtContentRecord.content_id))
            {
                THROW_FORMAT("CNMT NCA not found after installation!");
            }

            // Parse data and create install content meta
            nx::data::ByteBuffer installContentMetaBuf;
            m_contentMeta[i].SetContentId(cnmtContentRecord.content_id);
            m_contentMeta[i].SetupPackagedContentMeta();
            m_contentMeta[i].GetInstallContentMeta(installContentMetaBuf, cnmtContentRecord, m_ignoreReqFirmVersion);

            this->RemoveInstalledNcas(i);
            this->InstallContentMetaRecords(installContentMetaBuf, i);
            this->InstallApplicationRecord(i);
        }
    }

    void InstallTask::Begin()
    {
        for (nx::ncm::ContentMeta contentMeta : m_contentMeta)
        {
            m_worker->ClearHashMap();
            LOG_DEBUG("Installing NCAs...\n");
            size_t skippedNcas = 0;
            for (auto& record : contentMeta.GetContentInfos())
            {
                if (m_skipBase)
                {
                    bool alreadyExists = false;
                    const NcmStorageId storageIDs[] { NcmStorageId_SdCard, NcmStorageId_BuiltInUser };
                    for (size_t i = 0; i < std::size(storageIDs); i++)
                    {
                        nx::ncm::ContentStorage contentStorage(storageIDs[i]);
                        if (contentStorage.Has(record.content_id))
                        {
                            alreadyExists = true;
                            break;
                        }
                    }
                    if (alreadyExists)
                    {
                        ++skippedNcas;
                        continue;
                    }
                }
                std::string ncaIdStr = nx::ncm::GetContentIdString(record.content_id);
                LOG_DEBUG("Installing %s\n", ncaIdStr.c_str());
                this->InstallNCA(record.content_id);
                if (contentMeta.GetDistributionType() == 0)
                {
                    const u8* metaHash = contentMeta.GetHashByContentId(record.content_id);
                    const u8* workerHash = m_worker->GetHashByContentIdString(ncaIdStr);
                    if (metaHash != nullptr &&
                        workerHash != nullptr &&
                        memcmp(metaHash, workerHash, SHA256_HASH_SIZE) != 0)
                    {
                        app::facade::SendInstallInfoText("inst.nca_verify.hash_failed"_lang + ncaIdStr);
                    }
                }
            }
            if (contentMeta.GetDistributionType() == 1 && skippedNcas == 0)
            {
                app::facade::SendInstallInfoText("inst.nca_verify.missing_digital"_lang);
                std::map<std::string, std::vector<u8>> hashMap = m_worker->GetHashMap();
                contentMeta.RebuildNcaToInstall(m_destStorageId, hashMap);
            }
        }
    }

    void InstallTask::InstallTicketCert()
    {
        LOG_DEBUG("Installing ticket and cert...\n");
        try
        {
            this->ParseTicketCert();
        }
        catch (std::runtime_error& e)
        {
            LOG_DEBUG("WARNING: Ticket installation failed! This may not be an issue, depending on your use case.\nProceed with caution!\n");
        }
    }

    u64 InstallTask::GetTitleId(int i)
    {
        return m_contentMeta[i].GetContentMetaKey().id;
    }

    NcmContentMetaType InstallTask::GetContentMetaType(int i)
    {
        return static_cast<NcmContentMetaType>(m_contentMeta[i].GetContentMetaKey().type);
    }

    std::vector<std::tuple<nx::ncm::ContentMeta, NcmContentInfo>> InstallTask::ReadContentMeta()
    {
        std::vector<std::tuple<nx::ncm::ContentMeta, NcmContentInfo>> contentMetaList;

        for (const void* fileEntry : m_worker->GetContent()->GetFileEntriesByExtension("cnmt.nca"))
        {
            std::string cnmtNcaName(m_worker->GetContent()->GetFileEntryName(fileEntry));
            NcmContentId cnmtContentId = nx::ncm::GetContentIdFromString(cnmtNcaName);
            size_t cnmtNcaSize = m_worker->GetContent()->GetFileEntrySize(fileEntry);

            nx::ncm::ContentStorage contentStorage(m_destStorageId);

            LOG_DEBUG("CNMT Name: %s\n", cnmtNcaName.c_str());

            // We install the cnmt nca early to read from it later
            nx::nca::NcaHeader ncaHeader;
            this->InstallNCA(cnmtContentId, &ncaHeader);
            std::string cnmtNCAFullPath = contentStorage.GetPath(cnmtContentId);

            NcmContentInfo cnmtContentInfo;
            cnmtContentInfo.content_id = cnmtContentId;
            ncmU64ToContentInfoSize(cnmtNcaSize, &cnmtContentInfo);
            cnmtContentInfo.content_type = NcmContentType_Meta;

            contentMetaList.push_back( { nx::ncm::GetContentMetaFromNCA(cnmtNCAFullPath), cnmtContentInfo } );
            std::get<0>(contentMetaList.back()).SetNcaHeader(ncaHeader);
        }

        return contentMetaList;
    }

    void InstallTask::InstallNCA(const NcmContentId& ncaId, nx::nca::NcaHeader* outHeader)
    {
        const void* fileEntry = m_worker->GetContent()->GetFileEntryByNcaId(ncaId);
        std::string ncaFileName = m_worker->GetContent()->GetFileEntryName(fileEntry);

#ifdef NXLINK_DEBUG
        size_t ncaSize = m_worker->GetContent()->GetFileEntrySize(fileEntry);
        LOG_DEBUG("Installing %s to storage Id %u\n", ncaFileName.c_str(), m_destStorageId);
        LOG_DEBUG("Size: 0x%lx\n", ncaSize);
#endif

        std::shared_ptr<nx::ncm::ContentStorage> contentStorage(new nx::ncm::ContentStorage(m_destStorageId));

        // Attempt to delete any leftover placeholders
        try { contentStorage->DeletePlaceholder(*(NcmPlaceHolderId*)&ncaId); } catch (...) {}

        nx::nca::NcaHeader* header = new nx::nca::NcaHeader;
        m_worker->BufferData(header, m_worker->GetContent()->GetFileEntryOffset(fileEntry), sizeof(nx::nca::NcaHeader));

        nx::Crypto::AesXtr crypto(nx::Crypto::Keys().headerKey, false);
        crypto.decrypt(header, header, sizeof(nx::nca::NcaHeader), 0, 0x200);

        if (header->magic != MAGIC_NCA3)
        {
            THROW_FORMAT("Invalid NCA magic");
        }

        if (!nx::Crypto::rsa2048PssVerify(&header->magic, 0x200, header->fixed_key_sig, nx::Crypto::NCAHeaderSignature))
        {
            app::facade::SendInstallInfoText("inst.nca_verify.error"_lang + nx::ncm::GetContentIdString(ncaId));
        }

        // outHeader not nullptr means we are installing a CNMT NCA
        if (outHeader != nullptr)
        {
            memcpy(outHeader, header, sizeof(nx::nca::NcaHeader));
            // Delete CNMT NCA from ContentStorage if already exists
            contentStorage->Delete(ncaId);
        }

        delete header;
        m_worker->StreamToPlaceholder(contentStorage, ncaId);

        LOG_DEBUG("Registering placeholder...\n");
        try
        {
            contentStorage->Register(*(NcmPlaceHolderId*)&ncaId, ncaId);
        }
        catch (...)
        {
            LOG_DEBUG(("Failed to register " + ncaFileName + ". It may already exist.\n").c_str());
        }
        try { contentStorage->DeletePlaceholder(*(NcmPlaceHolderId*)&ncaId); } catch (...) {}
    }

    void InstallTask::ParseTicketCert()
    {
        // Read the tik files and put it into a buffer
        std::vector<const void*> tikFileEntries = m_worker->GetContent()->GetFileEntriesByExtension("tik");
        if (tikFileEntries.size() == 0)
        {
            THROW_FORMAT("No tik file found in the content!");
        }

        std::vector<const void*> tmpFileEntries = m_worker->GetContent()->GetFileEntriesByExtension("cert");
        std::vector<const void*> certFileEntries(tikFileEntries.size(), nullptr);
        for (size_t i = 0; i < tmpFileEntries.size(); i++)
        {
            if (i >= tikFileEntries.size())
            {
                break;
            }
            certFileEntries[i] = tmpFileEntries[i];
        }

        for (size_t i = 0; i < tikFileEntries.size(); i++)
        {
            u64 tikSize = m_worker->GetContent()->GetFileEntrySize(tikFileEntries[i]);
            auto tikBuf = std::make_unique<u8[]>(tikSize);
            LOG_DEBUG("> Reading tik\n");
            m_worker->BufferData(tikBuf.get(), m_worker->GetContent()->GetFileEntryOffset(tikFileEntries[i]), tikSize);

            u64 certSize;
            std::unique_ptr<u8[]> certBuf;
            if (certFileEntries[i] == nullptr)
            {
                certSize = nx::ext::CommonCertificateSize;
                certBuf = std::make_unique<u8[]>(certSize);
                memcpy(certBuf.get(), nx::ext::CommonCertificateData, certSize);
            }
            else
            {
                certSize = m_worker->GetContent()->GetFileEntrySize(certFileEntries[i]);
                certBuf = std::make_unique<u8[]>(certSize);
                LOG_DEBUG("> Reading cert\n");
                m_worker->BufferData(certBuf.get(), m_worker->GetContent()->GetFileEntryOffset(certFileEntries[i]), certSize);
            }

            // Try to fix a bad ticket dump
            if (m_fixTicket)
            {
                // https://switchbrew.org/wiki/Ticket#Certificate_chain
                u16 ECDSA_Properties = 0x4 + 0x3C + 0x40 + 0x146;
                u16 RSA_2048_Properties = 0x4 + 0x100 + 0x3C + 0x146;
                u16 RSA_4096_Properties = 0x4 + 0x200 + 0x3C + 0x146;
                u16 HMAC_160_Properties = 0x4 + 0x14 + 0x28 + 0x146;

                u16 ECDSA_RightsId = 0x4 + 0x3C + 0x40 + 0x160;
                u16 RSA_2048_RightsId = 0x4 + 0x100 + 0x3C + 0x160;
                u16 RSA_4096_RightsId = 0x4 + 0x200 + 0x3C + 0x160;
                u16 HMAC_160_RightsId = 0x4 + 0x14 + 0x28 + 0x160;

                // ECDSA SHA256 & SHA1
                if ((tikBuf.get()[0] == 5 || tikBuf.get()[0] == 2) && tikBuf.get()[ECDSA_Properties - 1] != tikBuf.get()[ECDSA_RightsId + 0x0F])
                {
                    tikBuf.get()[ECDSA_Properties] = 0x0; // Bad ticket dump may place key generation at wrong position, clearing it...
                    tikBuf.get()[ECDSA_Properties - 1] = tikBuf.get()[ECDSA_RightsId + 0x0F]; // Fix key generation using rights_id + 0x0F (last byte of rights_id should equal key generation)
                }

                // RSA_2048 SHA256 & SHA1
                else if ((tikBuf.get()[0] == 4 || tikBuf.get()[0] == 1) && (tikBuf.get()[RSA_2048_Properties - 1] != tikBuf.get()[RSA_2048_RightsId + 0x0F]))
                {
                    tikBuf.get()[RSA_2048_Properties] = 0x0;
                    tikBuf.get()[RSA_2048_Properties - 1] = tikBuf.get()[RSA_2048_RightsId + 0x0F];
                }

                // RSA_4096 SHA256 & SHA1
                else if ((tikBuf.get()[0] == 3 || tikBuf.get()[0] == 0) && (tikBuf.get()[RSA_4096_Properties - 1] != tikBuf.get()[RSA_4096_RightsId + 0x0F]))
                {
                    tikBuf.get()[RSA_4096_Properties] = 0x0;
                    tikBuf.get()[RSA_4096_Properties - 1] = tikBuf.get()[RSA_4096_RightsId + 0x0F];
                }

                // HMAC_160 SHA1
                else if (tikBuf.get()[0] == 6 && (tikBuf.get()[HMAC_160_Properties - 1] != tikBuf.get()[HMAC_160_RightsId + 0x0F]))
                {
                    tikBuf.get()[HMAC_160_Properties] = 0x0;
                    tikBuf.get()[HMAC_160_Properties - 1] = tikBuf.get()[HMAC_160_RightsId + 0x0F];
                }
            }

            // Finally, let's actually import the ticket
            ASSERT_OK(esImportTicket(tikBuf.get(), tikSize, certBuf.get(), certSize), "Failed to import ticket");
        }
    }
}
