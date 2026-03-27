#pragma once

#include <memory>

#include "nx/fs.hpp"
#include "ui/BaseMenuPage.hpp"

namespace app::ui
{
    class LocalInstallPage : public BaseMenuPage
    {
        public:
            LocalInstallPage();
            ~LocalInstallPage();
            PU_SMART_CTOR(LocalInstallPage)
            void drawMenuItems(bool clearItems, nx::fs::Path ourPath);
            void setStorageSourceToSdmc();
            void setStorageSourceToUdisk();
            void onCancel() override;
            void onConfirm() override;
            void onSelectAll() override;
        private:
            pu::ui::elm::Menu::Ref menu;
            pu::ui::elm::Menu::Ref GetMenu() override { return this->menu; }
            struct InternalData;
            std::unique_ptr<InternalData> pageData;

            void followDirectory();
            void selectFile(int selectedIndex, bool redraw = true);
            void startInstall();
            void onInput(const u64 Down, const u64 Up, const u64 Held, const pu::ui::TouchPoint Pos);
    };
}
