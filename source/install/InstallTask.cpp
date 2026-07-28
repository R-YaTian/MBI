#include "install/InstallTask.hpp"
#include "nx/error.hpp"
#include "nx/nca.hpp"
#include "nx/fs.hpp"
#include "nx/Crypto.hpp"
#include "util/i18n.hpp"
#include "facade.hpp"

namespace app
{
    InstallTask::InstallTask(NcmStorageId destStorageId, bool ignoreReqFirmVersion, bool fixTicket, bool skipBase, install::Worker* worker) :
        m_destStorageId(destStorageId),
        m_ignoreReqFirmVersion(ignoreReqFirmVersion),
        m_fixTicket(fixTicket),
        m_skipBase(skipBase),
        m_contentMeta(),
        m_worker(worker)
    {
        if (hosversionAtLeast(5,0,0))
        {
            appletSetAutoSleepDisabled(true);
        }
        else
        {
            appletSetMediaPlaybackState(true);
        }
    }

    InstallTask::~InstallTask()
    {
        if (hosversionAtLeast(5,0,0))
        {
            appletSetAutoSleepDisabled(false);
        }
        else
        {
            appletSetMediaPlaybackState(false);
        }
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

    void InstallTask::RemoveInstalledNcas(int idx, bool skipBase)
    {
        NcmContentMetaKey contentMetaKey = m_contentMeta[idx].GetContentMetaKey();
        NcmContentMetaType contentMetaType = static_cast<NcmContentMetaType>(contentMetaKey.type);
        const auto app_id = nx::ncm::GetBaseTitleId(contentMetaKey.id, contentMetaType);

        // remove current entries (if any).
        s32 db_list_total;
        s32 db_list_count;
        u64 id_min = contentMetaKey.id;
        u64 id_max = contentMetaKey.id;

        if (contentMetaType == NcmContentMetaType_Application && skipBase)
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

            // Parse data and create install content meta
            nx::data::ByteBuffer installContentMetaBuf;
            m_contentMeta[i].SetContentId(cnmtContentRecord.content_id);
            m_contentMeta[i].SetupPackagedContentMeta();
            m_contentMeta[i].GetInstallContentMeta(installContentMetaBuf, cnmtContentRecord, m_ignoreReqFirmVersion);

            this->RemoveInstalledNcas(i, m_skipBase);
            this->InstallContentMetaRecords(installContentMetaBuf, i);
            this->InstallApplicationRecord(i);
        }
    }

    void InstallTask::Begin()
    {
        for (nx::ncm::ContentMeta contentMeta : m_contentMeta)
        {
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
            std::vector<nx::ext::TikCollection> tickets;
            const nx::ContentCollections& collections = this->m_worker->GetContent()->GetCollections();
            this->ParseTicketsIntoCollection(tickets, collections, true);
            this->ImportTickets(std::span<nx::ext::TikCollection>(tickets.data(), tickets.size()));
        }
        catch (std::runtime_error& e)
        {
            THROW_FORMAT("Ticket installation failed!\n%s", e.what());
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

            LOG_DEBUG("CNMT Name: %s\n", cnmtNcaName.c_str());

            // We install the cnmt nca early to read from it later
            nx::nca::NcaHeader ncaHeader;
            this->InstallNCA(cnmtContentId, false, &ncaHeader);

            nx::ncm::ContentStorage contentStorage(m_destStorageId);
            if (!contentStorage.Has(cnmtContentId))
            {
                THROW_FORMAT("CNMT NCA not found after installation!");
            }
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

    void InstallTask::InstallNCA(const NcmContentId& ncaId, bool skipRegister, nx::nca::NcaHeader* outHeader)
    {
        const void* fileEntry = m_worker->GetContent()->GetFileEntryByNcaId(ncaId);
        std::string ncaFileName = m_worker->GetContent()->GetFileEntryName(fileEntry);
        u64 ncaSize = m_worker->GetContent()->GetFileEntrySize(fileEntry);

#ifdef NXLINK_DEBUG
        LOG_DEBUG("Installing %s to storage Id %u\n", ncaFileName.c_str(), m_destStorageId);
        LOG_DEBUG("Size: 0x%lx\n", ncaSize);
#endif

        std::shared_ptr<nx::ncm::ContentStorage> contentStorage(new nx::ncm::ContentStorage(m_destStorageId));

        // Attempt to delete any leftover placeholders
        try { contentStorage->DeletePlaceholder(*(NcmPlaceHolderId*)&ncaId); } catch (...) {}

        nx::nca::NcaHeader* header = new nx::nca::NcaHeader;
        nx::nca::NcaHeader decryptedHeader;
        m_worker->BufferData(header, m_worker->GetContent()->GetFileEntryOffset(fileEntry), sizeof(nx::nca::NcaHeader));

        nx::Crypto::AesXtr crypto(nx::Crypto::Keys().headerKey, false);
        crypto.decrypt(&decryptedHeader, header, sizeof(nx::nca::NcaHeader), 0, 0x200);

        if (decryptedHeader.magic != MAGIC_NCA3)
        {
            THROW_FORMAT("Invalid NCA magic");
        }

        if (decryptedHeader.sig_key_gen >= std::size(nx::Crypto::NCAHeaderSignature))
        {
            THROW_FORMAT("Invalid signature key generation");
        }

        auto mod = nx::Crypto::NCAHeaderSignature[decryptedHeader.sig_key_gen];
        if (!nx::Crypto::rsa2048PssVerify(&decryptedHeader.magic, 0x200, decryptedHeader.fixed_key_sig, mod))
        {
            app::facade::SendInstallInfoText("inst.nca_verify.error"_lang + nx::ncm::GetContentIdString(ncaId));
        }

        // outHeader not nullptr means we are installing a CNMT NCA
        if (outHeader != nullptr)
        {
            memcpy(outHeader, &decryptedHeader, sizeof(nx::nca::NcaHeader));
        }

        if (ncaSize > (u64)nx::fs::GetFreeSpaceSize(static_cast<FsContentStorageId>(contentStorage->GetStorageId() - 3)))
        {
            THROW_FORMAT("%s %s!", ("inst.info_page.no_space"_lang).c_str(), ncaFileName.c_str());
        }

        m_worker->StreamToPlaceholder(contentStorage, ncaId, header);
        delete header;

        if (skipRegister)
        {
            return;
        }

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

    static void TryFixTicket(std::vector<u8>& tikBuf)
    {
        if (tikBuf.empty())
        {
            return;
        }

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
        if ((tikBuf[0] == 5 || tikBuf[0] == 2) && tikBuf.size() > static_cast<size_t>(ECDSA_RightsId + 0x0F) && tikBuf[ECDSA_Properties - 1] != tikBuf[ECDSA_RightsId + 0x0F])
        {
            tikBuf[ECDSA_Properties] = 0x0; // Bad ticket dump may place key generation at wrong position, clearing it...
            tikBuf[ECDSA_Properties - 1] = tikBuf[ECDSA_RightsId + 0x0F]; // Fix key generation using rights_id + 0x0F (last byte of rights_id should equal key generation)
        }

        // RSA_2048 SHA256 & SHA1
        else if ((tikBuf[0] == 4 || tikBuf[0] == 1) && tikBuf.size() > static_cast<size_t>(RSA_2048_RightsId + 0x0F) && tikBuf[RSA_2048_Properties - 1] != tikBuf[RSA_2048_RightsId + 0x0F])
        {
            tikBuf[RSA_2048_Properties] = 0x0;
            tikBuf[RSA_2048_Properties - 1] = tikBuf[RSA_2048_RightsId + 0x0F];
        }

        // RSA_4096 SHA256 & SHA1
        else if ((tikBuf[0] == 3 || tikBuf[0] == 0) && tikBuf.size() > static_cast<size_t>(RSA_4096_RightsId + 0x0F) && tikBuf[RSA_4096_Properties - 1] != tikBuf[RSA_4096_RightsId + 0x0F])
        {
            tikBuf[RSA_4096_Properties] = 0x0;
            tikBuf[RSA_4096_Properties - 1] = tikBuf[RSA_4096_RightsId + 0x0F];
        }

        // HMAC_160 SHA1
        else if (tikBuf[0] == 6 && tikBuf.size() > static_cast<size_t>(HMAC_160_RightsId + 0x0F) && tikBuf[HMAC_160_Properties - 1] != tikBuf[HMAC_160_RightsId + 0x0F])
        {
            tikBuf[HMAC_160_Properties] = 0x0;
            tikBuf[HMAC_160_Properties - 1] = tikBuf[HMAC_160_RightsId + 0x0F];
        }
    }

    void InstallTask::ParseTicketsIntoCollection(std::vector<nx::ext::TikCollection>& tickets, const nx::ContentCollections& collections, bool read_data)
    {
        for (const auto& collection : collections)
        {
            if (collection.type == nx::ContentCollectionType::TIK)
            {
                nx::ext::TikCollection entry{};
                entry.rights_id = collection.info.rights_id;

                const auto cert = std::ranges::find_if(collections, [&collection](const auto& e){
                    return e.type == nx::ContentCollectionType::CERT
                        && std::memcmp(e.info.rights_id.c, collection.info.rights_id.c, sizeof(e.info.rights_id.c)) == 0;
                });

                entry.ticket.resize(collection.size);
                if (cert == collections.cend())
                {
                    entry.cert.resize(nx::ext::CommonCertificateSize);
                    memcpy(entry.cert.data(), nx::ext::CommonCertificateData, nx::ext::CommonCertificateSize);
                }
                else
                {
                    entry.cert.resize(cert->size);
                    if (read_data)
                    {
                        m_worker->BufferData(entry.cert.data(), cert->offset, cert->size);
                    }
                }

                if (read_data)
                {
                    m_worker->BufferData(entry.ticket.data(), collection.offset, collection.size);
                }

                tickets.emplace_back(entry);
            }
        }
    }

    void InstallTask::ImportTickets(std::span<nx::ext::TikCollection> collections)
    {
        if (collections.size() == 0)
        {
            LOG_DEBUG("No tickets found, skipping ticket installation\n");
            return;
        }

        for (auto& collection : collections)
        {
            // Try to fix a bad ticket dump
            if (m_fixTicket)
            {
                TryFixTicket(collection.ticket);
            }

            // Finally, let's actually import the ticket
            ASSERT_OK(esImportTicket(collection.ticket.data(),
                                     collection.ticket.size(),
                                     collection.cert.data(),
                                     collection.cert.size()), "Failed to import ticket");
        }
    }

    void InstallTask::InstallFromCollections()
    {
        std::vector<std::tuple<nx::ncm::ContentMeta, NcmContentInfo>> contentMetaList;
        const nx::ContentCollections& collections = m_worker->GetContent()->GetCollections();
        std::vector<nx::ext::TikCollection> tickets;
        this->ParseTicketsIntoCollection(tickets, collections);
        for (const auto& entry : collections)
        {
            if (entry.type == nx::ContentCollectionType::ARCHIVE)
            {
                this->InstallNCA(entry.info.content_id, true);
            }
            else if (entry.type == nx::ContentCollectionType::META)
            {
                nx::nca::NcaHeader ncaHeader;
                this->InstallNCA(entry.info.content_id, false, &ncaHeader);

                nx::ncm::ContentStorage contentStorage(m_destStorageId);
                if (!contentStorage.Has(entry.info.content_id))
                {
                    THROW_FORMAT("CNMT NCA not found after installation!");
                }
                std::string cnmtNCAFullPath = contentStorage.GetPath(entry.info.content_id);

                NcmContentInfo cnmtContentInfo;
                cnmtContentInfo.content_id = entry.info.content_id;
                ncmU64ToContentInfoSize(entry.size, &cnmtContentInfo);
                cnmtContentInfo.content_type = NcmContentType_Meta;

                contentMetaList.push_back( { nx::ncm::GetContentMetaFromNCA(cnmtNCAFullPath), cnmtContentInfo } );
                std::get<0>(contentMetaList.back()).SetNcaHeader(ncaHeader);
            }
            else if (entry.type == nx::ContentCollectionType::TIK || entry.type == nx::ContentCollectionType::CERT)
            {
                const FsRightsId& rights_id = entry.info.rights_id;
                auto ticketIt = std::ranges::find_if(tickets, [&rights_id](const auto& e){
                    return !std::memcmp(&rights_id, &e.rights_id, sizeof(rights_id));
                });
                if (ticketIt != tickets.end())
                {
                    if (entry.type == nx::ContentCollectionType::CERT)
                    {
                        m_worker->BufferData(ticketIt->cert.data(), entry.offset, entry.size);
                    }
                    else
                    {
                        m_worker->BufferData(ticketIt->ticket.data(), entry.offset, entry.size);
                    }
                }
            }
        }

        for (size_t i = 0; i < contentMetaList.size(); i++)
        {
            std::tuple<nx::ncm::ContentMeta, NcmContentInfo> cnmtTuple = contentMetaList[i];

            m_contentMeta.push_back(std::get<0>(cnmtTuple));
            NcmContentInfo cnmtContentRecord = std::get<1>(cnmtTuple);

            // Parse data and create install content meta
            nx::data::ByteBuffer installContentMetaBuf;
            m_contentMeta[i].SetContentId(cnmtContentRecord.content_id);
            m_contentMeta[i].SetupPackagedContentMeta();
            m_contentMeta[i].GetInstallContentMeta(installContentMetaBuf, cnmtContentRecord, m_ignoreReqFirmVersion);

            this->RemoveInstalledNcas(i);
            this->InstallContentMetaRecords(installContentMetaBuf, i);
            this->InstallApplicationRecord(i);
        }

        this->ImportTickets(std::span<nx::ext::TikCollection>(tickets.data(), tickets.size()));

        for (nx::ncm::ContentMeta contentMeta : m_contentMeta)
        {
            for (auto& record : contentMeta.GetContentInfos())
            {
                std::string ncaIdStr = nx::ncm::GetContentIdString(record.content_id);

                nx::ncm::ContentStorage contentStorage(m_destStorageId);
                LOG_DEBUG("Registering placeholder...\n");
                try
                {
                    contentStorage.Register(*(NcmPlaceHolderId*)&record.content_id, record.content_id);
                }
                catch (...)
                {
                    LOG_DEBUG(("Failed to register " + ncaIdStr + ". It may already exist.\n").c_str());
                }

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
            if (contentMeta.GetDistributionType() == 1)
            {
                app::facade::SendInstallInfoText("inst.nca_verify.missing_digital"_lang);
                std::map<std::string, std::vector<u8>> hashMap = m_worker->GetHashMap();
                contentMeta.RebuildNcaToInstall(m_destStorageId, hashMap);
            }
        }

        for (const auto& entry : collections)
        {
            nx::ncm::ContentStorage contentStorage(m_destStorageId);
            if (entry.type != nx::ContentCollectionType::ARCHIVE)
            {
                continue;
            }
            try { contentStorage.DeletePlaceholder(*(NcmPlaceHolderId*)&entry.info.content_id); } catch (...) {}
        }
    }
}
