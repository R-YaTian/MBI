#pragma once

#include "install/Worker.hpp"
#include <atomic>

namespace app::install
{
    enum class MTPInstallState
    {
        None,
        Progress,
        Finished,
    };

    class MtpWorker : public Worker
    {
        public:
            MtpWorker(std::unique_ptr<nx::Content> content, const std::string& path);
            ~MtpWorker();

            void StreamToPlaceholder(std::shared_ptr<nx::ncm::ContentStorage>& contentStorage, NcmContentId ncaId, nx::nca::NcaHeader* header = nullptr) override;
            void BufferData(void* _buf, off_t offset, size_t size) override;
            bool Push(const void* buf, s64 size);
            void Disable();

            MTPInstallState GetInstallState() const
            {
                return INSTALL_STATE.load();
            }

            void SetInstallState(MTPInstallState state)
            {
                INSTALL_STATE.store(state);
            }

            Mutex m_mutex{};
            std::atomic_bool m_active{};
        private:
            s64 m_offset{};
            std::vector<u8> m_buffer{};
            CondVar m_can_read{};
            CondVar m_can_write{};
            std::atomic<MTPInstallState> INSTALL_STATE{MTPInstallState::None};
            void ReadChunk(void* buf, s64 size, u64* bytes_read);
    };
}
