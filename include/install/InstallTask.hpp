#pragma once

#include <memory>
#include <vector>
#include <tuple>
#include <span>
#include "nx/ncm.hpp"
#include "nx/ext.hpp"
#include "install/Worker.hpp"

namespace app
{
    class InstallTask
    {
        protected:
            const NcmStorageId m_destStorageId;
            bool m_overClock = false;
            bool m_ignoreReqFirmVersion = false;
            bool m_fixTicket = false;
            bool m_skipBase = false;
            std::vector<nx::ncm::ContentMeta> m_contentMeta;

        public:
            InstallTask(NcmStorageId destStorageId, bool overClock, bool ignoreReqFirmVersion, bool fixTicket, bool skipBase, install::Worker* worker);
            ~InstallTask();

            void Prepare();
            void Begin();
            void InstallTicketCert();
            void InstallFromCollections();

        private:
            install::Worker* m_worker;

            u64 GetTitleId(int i = 0);
            NcmContentMetaType GetContentMetaType(int i = 0);
            std::vector<std::tuple<nx::ncm::ContentMeta, NcmContentInfo>> ReadContentMeta();
            void ParseTicketsIntoCollection(std::vector<nx::ext::TikCollection>& tickets, const nx::ContentCollections& collections, bool read_data = false);
            void ImportTickets(std::span<nx::ext::TikCollection> collections);
            void InstallNCA(const NcmContentId &ncaId, bool skipRegister = false, nx::nca::NcaHeader* outHeader = nullptr);
            void InstallContentMetaRecords(nx::data::ByteBuffer& installContentMetaBuf, int i);
            void InstallApplicationRecord(int i);
            void RemoveInstalledNcas(int idx, bool skipBase = false);
    };
}
