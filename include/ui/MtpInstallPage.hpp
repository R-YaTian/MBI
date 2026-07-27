#pragma once

#include <memory>

#include "ui/InstallerPage.hpp"

namespace app::ui
{
    class MtpInstallPage : public InstallerPage
    {
        public:
            MtpInstallPage();
            ~MtpInstallPage();
            PU_SMART_CTOR(MtpInstallPage)
            void onCancel() override;
            void onInitInstallMode();
        protected:
            bool onInstallStart(const char* path);
            bool onInstallWrite(const void* buf, size_t size);
            void onInstallClose();
        private:
            pu::ui::elm::Image::Ref infoImage;
            struct InternalData;
            std::unique_ptr<InternalData> pageData;
            void onInput(const u64 Down, const u64 Up, const u64 Held, const pu::ui::TouchPoint Pos);
            void updateState();
            void onInstallTask();
    };
}
