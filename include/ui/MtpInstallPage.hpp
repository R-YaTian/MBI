#pragma once

#include "ui/BaseMenuPage.hpp"

namespace app::ui
{
    class MtpInstallPage : public BaseMenuPage
    {
        public:
            MtpInstallPage();
            PU_SMART_CTOR(MtpInstallPage)
            void onCancel() override;
        private:
            pu::ui::elm::Image::Ref infoImage;
            void onInput(const u64 Down, const u64 Up, const u64 Held, const pu::ui::TouchPoint Pos);
            void updateState();
    };
}
