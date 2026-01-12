#pragma once

#include <pu/Plutonium>

namespace app::ui
{
    class InstallerPage : public pu::ui::Layout
    {
        public:
            InstallerPage();
            PU_SMART_CTOR(InstallerPage)
            void onInput(u64 Down, u64 Up, u64 Held, pu::ui::TouchPoint Pos);
            pu::ui::elm::TextBlock::Ref pageInfoText;
            pu::ui::elm::TextBlock::Ref installInfoText;
            pu::ui::elm::ProgressBar::Ref installBar;
            static void setTopInstInfoText(std::string ourText);
            static void setInstInfoText(std::string ourText);
            static void setInstBarPerc(double ourPercent);
            static void loadMainMenu();
            static void loadInstallScreen();
        private:
            pu::ui::elm::Rectangle::Ref infoRect;
    };
}
