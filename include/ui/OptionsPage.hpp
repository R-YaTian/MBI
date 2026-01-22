#pragma once

#include <pu/Plutonium>

namespace app::ui
{
    class OptionsPage : public pu::ui::Layout
    {
        public:
            OptionsPage();
            PU_SMART_CTOR(OptionsPage)
            void onInput(u64 Down, u64 Up, u64 Held, pu::ui::TouchPoint Pos);
        private:
            std::vector<std::string> languageStrings = {"English", "日本語", "Français", "Deutsch", "Italiano", "Español", "한국어", "Português", "Русский", "简体中文", "正體中文"};
            pu::ui::elm::Menu::Ref menu;
            bool touchGuard = false;
            void setMenuText();
            pu::sdl2::TextureHandle::Ref getMenuOptionIcon(bool ourBool);
            std::string getMenuLanguage(int ourLangCode);
    };
}
