#pragma once

#include "ui/BaseMenuPage.hpp"

namespace app::ui
{
    class UsbInstallPage : public BaseMenuPage
    {
        public:
            UsbInstallPage();
            PU_SMART_CTOR(UsbInstallPage)
            bool startUsb();
            void onCancel() override;
            void onConfirm() override;
            void onSelectAll() override;
        private:
            std::vector<std::string> ourTitles;
            std::vector<std::string> selectedTitles;
            pu::ui::elm::Menu::Ref menu;
            pu::ui::elm::Menu::Ref GetMenu() override { return this->menu; }
            pu::ui::elm::Image::Ref infoImage;
            void drawMenuItems(bool clearItems);
            void selectTitle(int selectedIndex, bool redraw = true);
            void startInstall();
            void onInput(const u64 Down, const u64 Up, const u64 Held, const pu::ui::TouchPoint Pos);
    };
}
