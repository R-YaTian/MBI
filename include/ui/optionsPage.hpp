#pragma once

#include <pu/Plutonium>

namespace app::ui {
    class OptionsPage : public pu::ui::Layout
    {
        public:
            OptionsPage();
            PU_SMART_CTOR(OptionsPage)
            void onInput(u64 Down, u64 Up, u64 Held, pu::ui::TouchPoint Pos);
        private:
            pu::ui::elm::TextBlock::Ref botText;
            pu::ui::elm::Rectangle::Ref infoRect;
            pu::ui::elm::Rectangle::Ref botRect;
            pu::ui::elm::TextBlock::Ref pageInfoText;
            pu::ui::elm::Menu::Ref menu;
            void setMenuText();
            pu::sdl2::TextureHandle::Ref getMenuOptionIcon(bool ourBool);
            std::string getMenuLanguage(int ourLangCode);
    };
}
