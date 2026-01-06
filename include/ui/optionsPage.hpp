#pragma once
#include <pu/Plutonium>

using namespace pu::ui::elm;
namespace app::ui {
    class optionsPage : public pu::ui::Layout
    {
        public:
            optionsPage();
            PU_SMART_CTOR(optionsPage)
            void onInput(u64 Down, u64 Up, u64 Held, pu::ui::TouchPoint Pos);
            void updateStatsThread();
            Image::Ref titleImage;
            TextBlock::Ref appVersionText;
            TextBlock::Ref freeSpaceText;
            TextBlock::Ref batteryValueText;
        private:
            TextBlock::Ref butText;
            Rectangle::Ref topRect;
            Rectangle::Ref infoRect;
            Rectangle::Ref botRect;
            TextBlock::Ref pageInfoText;
            pu::ui::elm::Menu::Ref menu;
            void setMenuText();
            pu::sdl2::TextureHandle::Ref getMenuOptionIcon(bool ourBool);
            std::string getMenuLanguage(int ourLangCode);
    };
}