#pragma once

#include <pu/Plutonium>

namespace app::ui
{
    class MtpInstallPage : public pu::ui::Layout
    {
        public:
            MtpInstallPage();
            PU_SMART_CTOR(MtpInstallPage)
        private:
            pu::ui::elm::Image::Ref infoImage;
            void onInput(const u64 Down, const u64 Up, const u64 Held, const pu::ui::TouchPoint Pos);
    };
}
