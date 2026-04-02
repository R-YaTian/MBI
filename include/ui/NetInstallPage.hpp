#pragma once

#include "ui/BaseMenuPage.hpp"

namespace app::ui
{
    class NetInstallPage : public BaseMenuPage
    {
        public:
            NetInstallPage();
            PU_SMART_CTOR(NetInstallPage)
            bool startNetwork();
            void onCancel() override;
            void onConfirm() override;
            void onSelectAll() override;
        private:
            std::vector<std::string> ourUrls;
            std::map<size_t, std::string> selectedUrls;  // index -> URL
            pu::ui::elm::Menu::Ref menu;
            pu::ui::elm::Menu::Ref GetMenu() override { return this->menu; }
            pu::ui::elm::Image::Ref infoImage;
            void drawMenuItems();
            void selectTitle(int selectedIndex);
            void startInstall(bool urlMode = false);
            void onInput(const u64 Down, const u64 Up, const u64 Held, const pu::ui::TouchPoint Pos);
    };
}
