#pragma once

#include <filesystem>
#include <pu/Plutonium>

namespace app::ui
{
    class sdInstPage : public pu::ui::Layout
    {
        public:
            sdInstPage();
            PU_SMART_CTOR(sdInstPage)
            void startInstall();
            void onInput(u64 Down, u64 Up, u64 Held, pu::ui::TouchPoint Pos);
            void drawMenuItems(bool clearItems, std::filesystem::path ourPath);
            pu::ui::elm::Menu::Ref menu;
        private:
            std::vector<std::filesystem::path> ourDirectories;
            std::vector<std::filesystem::path> ourFiles;
            std::vector<std::filesystem::path> selectedTitles;
            std::vector<long unsigned int> menuIndices;
            std::filesystem::path currentDir;
            pu::ui::elm::TextBlock::Ref butText;
            pu::ui::elm::Rectangle::Ref infoRect;
            pu::ui::elm::Rectangle::Ref botRect;
            pu::ui::elm::TextBlock::Ref pageInfoText;
            void followDirectory();
            void selectNsp(int selectedIndex, bool redraw = true);
    };
}
