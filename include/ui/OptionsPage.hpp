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
            s32 menuItemCount = 0;
            bool touchGuard = false;
            std::vector<std::string> languageStrings = {"English", "日本語", "Français", "Deutsch", "Italiano", "Español", "한국어", "Português", "Русский", "简体中文", "正體中文"};
            pu::ui::elm::Menu::Ref menu;
            pu::ui::elm::MenuItem::Ref ignoreFirmOption;
            pu::ui::elm::MenuItem::Ref overclockOption;
            pu::ui::elm::MenuItem::Ref deletePromptOption;
            pu::ui::elm::MenuItem::Ref enableSoundOption;
            pu::ui::elm::MenuItem::Ref enableLightningOption;
            pu::ui::elm::MenuItem::Ref fixTicketOption;
            pu::ui::elm::MenuItem::Ref languageOption;
            pu::ui::elm::MenuItem::Ref creditsOption;
            void MenuAddItem(pu::ui::elm::Menu::Ref& menu, pu::ui::elm::MenuItem::Ref& Item);
            void IgnoreFirmOption_Click();
            void OverclockOption_Click();
            void DeletePromptOption_Click();
            void EnableSoundOption_Click();
            void EnableLightningOption_Click();
            void FixTicketOption_Click();
            void CreditsOption_Click();
            pu::sdl2::TextureHandle::Ref getMenuOptionIcon(bool ourBool);
            std::string getMenuLanguage(int ourLangCode);
    };
}
