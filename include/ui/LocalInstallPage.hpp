#pragma once

#include <filesystem>
#include <vector>
#include <memory>
#include <pu/Plutonium>

namespace app::ui
{
    class LocalInstallPage : public pu::ui::Layout
    {
        public:
            LocalInstallPage();
            ~LocalInstallPage();
            PU_SMART_CTOR(LocalInstallPage)
            void drawMenuItems(bool clearItems, std::filesystem::path ourPath);
            void setMenuIndex(int index);
            void setStorageSourceToSdmc();
            void setStorageSourceToUdisk();
            void onCancel();
            void onConfirm();
        private:
            std::vector<std::filesystem::path> ourDirectories;
            std::vector<std::filesystem::path> ourFiles;
            std::vector<std::filesystem::path> selectedTitles;
            std::vector<long unsigned int> menuIndices;
            std::filesystem::path currentDir;
            bool isRootDirectory = true;
            pu::ui::elm::Menu::Ref menu;
            struct InternalData;
            std::unique_ptr<InternalData> pageData;
            void followDirectory();
            void selectFile(int selectedIndex, bool redraw = true);
            void startInstall();
            void onInput(const u64 Down, const u64 Up, const u64 Held, const pu::ui::TouchPoint Pos);
    };
}
