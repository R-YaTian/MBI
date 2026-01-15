#pragma once

#include <pu/Plutonium>

namespace app::ui {
    class netInstPage : public pu::ui::Layout
    {
        public:
            netInstPage();
            PU_SMART_CTOR(netInstPage)
            void startInstall(bool urlMode);
            void startNetwork();
            void onInput(u64 Down, u64 Up, u64 Held, pu::ui::TouchPoint Pos);
            pu::ui::elm::TextBlock::Ref pageInfoText;
        private:
            std::vector<std::string> ourUrls;
            std::vector<std::string> selectedUrls;
            std::vector<std::string> alternativeNames;
            std::vector<long unsigned int> menuIndices;
            pu::ui::elm::TextBlock::Ref botText;
            pu::ui::elm::Rectangle::Ref infoRect;
            pu::ui::elm::Rectangle::Ref botRect;
            pu::ui::elm::Menu::Ref menu;
            pu::ui::elm::Image::Ref infoImage;
            void drawMenuItems(bool clearItems);
            void selectTitle(int selectedIndex, bool redraw = true);
    };
}
