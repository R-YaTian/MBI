#pragma once

#include <pu/Plutonium>

namespace app::ui
{
    class MtpInstallPage : public pu::ui::Layout
    {
        public:
            MtpInstallPage();
            PU_SMART_CTOR(MtpInstallPage)
            void onInput(u64 Down, u64 Up, u64 Held, pu::ui::TouchPoint Pos);
        private:
            pu::ui::elm::Image::Ref infoImage;
    };
}
