#pragma once

#include "ui/BaseMenuPage.hpp"

namespace app::ui
{
    class OptionsPage : public BaseMenuPage
    {
        public:
            OptionsPage();
            PU_SMART_CTOR(OptionsPage)
            void onCancel() override;
        private:
            bool inputGuard = false;
            std::vector<std::string> languageStrings = {"English", "日本語", "Français", "Deutsch", "Italiano", "Español", "한국어", "Português", "Русский", "简体中文", "正體中文"};
            pu::ui::elm::Menu::Ref menu;
            pu::ui::elm::MenuItem::Ref ignoreFirmOption;
            pu::ui::elm::MenuItem::Ref overclockOption;
            pu::ui::elm::MenuItem::Ref deletePromptOption;
            pu::ui::elm::MenuItem::Ref enableSoundOption;
            pu::ui::elm::MenuItem::Ref fixTicketOption;
            pu::ui::elm::MenuItem::Ref use12hTimeOption;
            pu::ui::elm::MenuItem::Ref languageOption;
            pu::ui::elm::MenuItem::Ref creditsOption;
            void IgnoreFirmOption_Click();
            void OverclockOption_Click();
            void DeletePromptOption_Click();
            void EnableSoundOption_Click();
            void FixTicketOption_Click();
            void Use12hTimeOption_Click();
            void LanguageOption_Click();
            void CreditsOption_Click();
            void onInput(const u64 Down, const u64 Up, const u64 Held, const pu::ui::TouchPoint Pos);
            pu::sdl2::TextureHandle::Ref getMenuOptionIcon(bool ourBool);
            std::string getMenuLanguage(int ourLangCode);
    };
}
