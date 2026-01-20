#pragma once

#include <pu/Plutonium>

namespace app::ui
{
    class NetInstallPage : public pu::ui::Layout
    {
        public:
            NetInstallPage();
            PU_SMART_CTOR(NetInstallPage)
            void startNetwork();
            void onInput(u64 Down, u64 Up, u64 Held, pu::ui::TouchPoint Pos);
        private:
            std::vector<std::string> ourUrls;
            std::vector<std::string> selectedUrls;
            std::vector<long unsigned int> menuIndices;
            pu::ui::elm::Menu::Ref menu;
            pu::ui::elm::Image::Ref infoImage;
            std::string sourceString = "";
            bool netListRevceived = false;
            void drawMenuItems(bool clearItems);
            void selectTitle(int selectedIndex, bool redraw = true);
            void startInstall(bool urlMode);
    };
}
