#pragma once

#include <pu/Plutonium>

namespace app::ui {
    class usbInstPage : public pu::ui::Layout
    {
        public:
            usbInstPage();
            PU_SMART_CTOR(usbInstPage)
            void startInstall();
            void startUsb();
            void onInput(u64 Down, u64 Up, u64 Held, pu::ui::TouchPoint Pos);
        private:
            std::vector<std::string> ourTitles;
            std::vector<std::string> selectedTitles;
            std::string lastUrl;
            std::string lastFileID;
            pu::ui::elm::TextBlock::Ref pageInfoText;
            pu::ui::elm::TextBlock::Ref butText;
            pu::ui::elm::Rectangle::Ref infoRect;
            pu::ui::elm::Rectangle::Ref botRect;
            pu::ui::elm::Menu::Ref menu;
            pu::ui::elm::Image::Ref infoImage;
            void drawMenuItems(bool clearItems);
            void selectTitle(int selectedIndex, bool redraw = true);
    };
}
