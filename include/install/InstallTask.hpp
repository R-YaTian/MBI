#pragma once

#include <memory>
#include <vector>
#include <tuple>
#include "nx/ncm.hpp"
#include "install/Worker.hpp"

namespace app
{
    class InstallTask
    {
        protected:
            const NcmStorageId m_destStorageId;
            bool m_ignoreReqFirmVersion = false;
            std::vector<nx::ncm::ContentMeta> m_contentMeta;

        public:
            InstallTask(NcmStorageId destStorageId, bool ignoreReqFirmVersion, std::unique_ptr<app::install::Worker> worker);
            ~InstallTask();

            void Prepare();
            void Begin();
            void InstallTicketCert();

        private:
            std::unique_ptr<app::install::Worker> m_worker;

            u64 GetTitleId(int i = 0);
            NcmContentMetaType GetContentMetaType(int i = 0);
            std::vector<std::tuple<nx::ncm::ContentMeta, NcmContentInfo>> ReadContentMeta();
            void ParseTicketCert();
            void InstallNCA(const NcmContentId &ncaId, nx::nca::NcaHeader* outHeader = nullptr);
            void InstallContentMetaRecords(nx::data::ByteBuffer& installContentMetaBuf, int i);
            void InstallApplicationRecord(int i);
    };
}
