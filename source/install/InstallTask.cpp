#include "install/InstallTask.hpp"
#include "nx/error.hpp"
#include "nx/nca.hpp"
#include "nx/Crypto.hpp"
#include "util/config.hpp"
#include "util/i18n.hpp"
#include "manager.hpp"
#include "facade.hpp"
#include <thread>

namespace app
{
    InstallTask::InstallTask(NcmStorageId destStorageId, bool ignoreReqFirmVersion, std::unique_ptr<app::install::Worker> worker) :
        m_destStorageId(destStorageId),
        m_ignoreReqFirmVersion(ignoreReqFirmVersion),
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
        const u64 baseTitleId = nx::ncm::GetBaseTitleId(this->GetTitleId(i), this->GetContentMetaType(i));

        // Add our new content meta
        NsExtContentStorageMetaKey storageRecord;
        storageRecord.meta_key = m_contentMeta[i].GetContentMetaKey();
        storageRecord.storage_id = m_destStorageId;

        LOG_DEBUG("Pushing application record...\n");
        ASSERT_OK(nsextPushApplicationRecord(baseTitleId, NsExtApplicationEvent_Present, &storageRecord, 1), "Failed to push application record");
    }

    // Validate and obtain all data needed for install
    void InstallTask::Prepare()
    {
        nx::data::ByteBuffer cnmtBuf;

        std::vector<std::tuple<nx::ncm::ContentMeta, NcmContentInfo>> tupelList = this->ReadContentMeta();

        for (size_t i = 0; i < tupelList.size(); i++)
        {
            std::tuple<nx::ncm::ContentMeta, NcmContentInfo> cnmtTuple = tupelList[i];

            m_contentMeta.push_back(std::get<0>(cnmtTuple));
            NcmContentInfo cnmtContentRecord = std::get<1>(cnmtTuple);

            nx::ncm::ContentStorage contentStorage(m_destStorageId);
            if (!contentStorage.Has(cnmtContentRecord.content_id))
            {
                LOG_DEBUG("Installing CNMT NCA...\n");
                this->InstallNCA(cnmtContentRecord.content_id, true);
            }
            else
            {
                LOG_DEBUG("CNMT NCA already installed. Proceeding...\n");
            }

            // Parse data and create install content meta
            if (m_ignoreReqFirmVersion)
            {
                LOG_DEBUG("WARNING: Required system firmware version is being IGNORED!\n");
            }

            nx::data::ByteBuffer installContentMetaBuf;
            m_contentMeta[i].SetupPackagedContentMeta();
            m_contentMeta[i].GetInstallContentMeta(installContentMetaBuf, cnmtContentRecord, m_ignoreReqFirmVersion);

            this->InstallContentMetaRecords(installContentMetaBuf, i);
            this->InstallApplicationRecord(i);
        }
    }

    void InstallTask::Begin()
    {
        for (nx::ncm::ContentMeta contentMeta : m_contentMeta)
        {
            LOG_DEBUG("Installing NCAs...\n");
            for (auto& record : contentMeta.GetContentInfos())
            {
                LOG_DEBUG("Installing from %s\n", nx::nca::GetNcaIdString(record.content_id).c_str());
                this->InstallNCA(record.content_id);
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
            NcmContentId cnmtContentId = nx::nca::GetNcaIdFromString(cnmtNcaName);
            size_t cnmtNcaSize = m_worker->GetContent()->GetFileEntrySize(fileEntry);

            nx::ncm::ContentStorage contentStorage(m_destStorageId);

            LOG_DEBUG("CNMT Name: %s\n", cnmtNcaName.c_str());

            // We install the cnmt nca early to read from it later
            this->InstallNCA(cnmtContentId, true);
            std::string cnmtNCAFullPath = contentStorage.GetPath(cnmtContentId);

            NcmContentInfo cnmtContentInfo;
            cnmtContentInfo.content_id = cnmtContentId;
            ncmU64ToContentInfoSize(cnmtNcaSize & 0xFFFFFFFFFFFF, &cnmtContentInfo);
            cnmtContentInfo.content_type = NcmContentType_Meta;

            contentMetaList.push_back( { nx::ncm::GetContentMetaFromNCA(cnmtNCAFullPath), cnmtContentInfo } );
        }

        return contentMetaList;
    }

    void InstallTask::InstallNCA(const NcmContentId& ncaId, bool isContentMeta)
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

        if (app::config::validateNCAs && !m_declinedValidation)
        {
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
                if (app::config::enableLightning)
                {
                    app::manager::lightningStart();
                }
                std::thread audioThread(app::manager::playAudio, "/fail.wav");
                int rc = app::facade::ShowDialog("inst.nca_verify.title"_lang, "inst.nca_verify.desc"_lang, {"common.cancel"_lang, "inst.nca_verify.opt1"_lang}, false);
                audioThread.join();
                if (app::config::enableLightning)
                {
                    app::manager::lightningStop();
                }
                if (rc != 1)
                {
                    THROW_FORMAT(("inst.nca_verify.error"_lang + nx::nca::GetNcaIdString(ncaId)).c_str());
                }
                m_declinedValidation = true;
            }
            delete header;
        }

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
        std::vector<const void*> certFileEntries = m_worker->GetContent()->GetFileEntriesByExtension("cert");

        for (size_t i = 0; i < tikFileEntries.size(); i++)
        {
            if (tikFileEntries[i] == nullptr)
            {
                LOG_DEBUG("Remote tik file is missing.\n");
                THROW_FORMAT("Remote tik file is not present!");
            }

            u64 tikSize = m_worker->GetContent()->GetFileEntrySize(tikFileEntries[i]);
            auto tikBuf = std::make_unique<u8[]>(tikSize);
            LOG_DEBUG("> Reading tik\n");
            m_worker->BufferData(tikBuf.get(), m_worker->GetContent()->GetFileEntryOffset(tikFileEntries[i]), tikSize);

            if (certFileEntries[i] == nullptr)
            {
                LOG_DEBUG("Remote cert file is missing.\n");
                THROW_FORMAT("Remote cert file is not present!");
            }

            u64 certSize = m_worker->GetContent()->GetFileEntrySize(certFileEntries[i]);
            auto certBuf = std::make_unique<u8[]>(certSize);
            LOG_DEBUG("> Reading cert\n");
            m_worker->BufferData(certBuf.get(), m_worker->GetContent()->GetFileEntryOffset(certFileEntries[i]), certSize);

            // try to fix a temp ticket and change it to a permanent one
            if (app::config::fixTicket)
            {
                u16 ECDSA = 0;
                u16 RSA_2048 = 0;
                u16 RSA_4096 = 0;

                // https://switchbrew.org/wiki/Ticket#Certificate_chain
                ECDSA = (0x4 + 0x3C + 0x40 + 0x146);
                RSA_2048 = (0x4 + 0x100 + 0x3C + 0x146);
                RSA_4096 = (0x4 + 0x200 + 0x3C + 0x146);

                // ECDSA SHA256
                if (tikBuf.get()[0x0] == 0x5 && (tikBuf.get()[ECDSA] == 0x10 || tikBuf.get()[ECDSA] == 0x30))
                {
                    tikBuf.get()[ECDSA] = 0x0;
                    tikBuf.get()[ECDSA - 1] = 0x10; // fix broken Master key revision
                }

                // RSA_2048 SHA256
                else if (tikBuf.get()[0x0] == 0x4 && (tikBuf.get()[RSA_2048] == 0x10 || tikBuf.get()[RSA_2048] == 0x30))
                {
                    tikBuf.get()[RSA_2048] = 0x0;
                    tikBuf.get()[RSA_2048 - 1] = 0x10;
                }

                // RSA_4096 SHA256
                else if (tikBuf.get()[0x0] == 0x3 && (tikBuf.get()[RSA_4096] == 0x10 || tikBuf.get()[RSA_4096] == 0x30))
                {
                    tikBuf.get()[RSA_4096] = 0x0;
                    tikBuf.get()[RSA_4096 - 1] = 0x10;
                }

                // ECDSA SHA1
                else if (tikBuf.get()[0x0] == 0x2 && (tikBuf.get()[ECDSA] == 0x10 || tikBuf.get()[ECDSA] == 0x30))
                {
                    tikBuf.get()[ECDSA] = 0x0;
                    tikBuf.get()[ECDSA - 1] = 0x10;
                }

                // RSA_2048 SHA1
                else if (tikBuf.get()[0x0] == 0x1 && (tikBuf.get()[RSA_2048] == 0x10 || tikBuf.get()[RSA_2048] == 0x30))
                {
                    tikBuf.get()[RSA_2048] = 0x0;
                    tikBuf.get()[RSA_2048 - 1] = 0x10;
                }

                // RSA_4096 SHA1
                else if (tikBuf.get()[0x0] == 0x0 && (tikBuf.get()[RSA_4096] == 0x10 || tikBuf.get()[RSA_4096] == 0x30))
                {
                    tikBuf.get()[RSA_4096] = 0x0;
                    tikBuf.get()[RSA_4096 - 1] = 0x10;
                }
            }

            // Finally, let's actually import the ticket
            ASSERT_OK(esImportTicket(tikBuf.get(), tikSize, certBuf.get(), certSize), "Failed to import ticket");
        }
    }
}
