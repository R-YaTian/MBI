#pragma once

#include <pu/Plutonium>

namespace app::ui
{
    class UsbInstallPage : public pu::ui::Layout
    {
        public:
            UsbInstallPage();
            PU_SMART_CTOR(UsbInstallPage)
            void startUsb();
            void onInput(u64 Down, u64 Up, u64 Held, pu::ui::TouchPoint Pos);
        private:
            std::vector<std::string> ourTitles;
            std::vector<std::string> selectedTitles;
            pu::ui::elm::Menu::Ref menu;
            pu::ui::elm::Image::Ref infoImage;
            void drawMenuItems(bool clearItems);
            void selectTitle(int selectedIndex, bool redraw = true);
            void startInstall();
    };
}
