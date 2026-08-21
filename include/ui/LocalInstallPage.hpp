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
            bool setStorageSourceToUdisk();
            void setStorageSourceToSdmc();
            void onCancel() override;
            void onConfirm() override;
            void onSelectAll() override;
        private:
            pu::ui::elm::Menu::Ref menu;
            pu::ui::elm::Menu::Ref GetMenu() override { return this->menu; }
            struct InternalData;
            std::unique_ptr<InternalData> pageData;
            void ClearPageData();
            void drawMenuItems(nx::fs::Path ourPath);
            void followDirectory();
            void selectFile(int selectedIndex);
            void startInstall();
            void onInput(const u64 Down, const u64 Up, const u64 Held, const pu::ui::TouchPoint Pos);
    };
}
