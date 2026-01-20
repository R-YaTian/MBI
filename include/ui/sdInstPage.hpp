#pragma once

#include <filesystem>
#include <vector>
#include <memory>
#include <pu/Plutonium>

namespace app::ui
{
    class sdInstPage : public pu::ui::Layout
    {
        public:
            sdInstPage();
            ~sdInstPage();
            PU_SMART_CTOR(sdInstPage)
            void onInput(u64 Down, u64 Up, u64 Held, pu::ui::TouchPoint Pos);
            void drawMenuItems(bool clearItems, std::filesystem::path ourPath);
            void setMenuIndex(int index);
            void setStorageSourceToSdmc();
            void setStorageSourceToUdisk();
        private:
            std::vector<std::filesystem::path> ourDirectories;
            std::vector<std::filesystem::path> ourFiles;
            std::vector<std::filesystem::path> selectedTitles;
            std::vector<long unsigned int> menuIndices;
            std::filesystem::path currentDir;
            pu::ui::elm::Menu::Ref menu;
            struct InternalData;
            std::unique_ptr<InternalData> pageData;
            void followDirectory();
            void selectFile(int selectedIndex, bool redraw = true);
            void startInstall();
    };
}
