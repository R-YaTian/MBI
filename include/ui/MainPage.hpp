#pragma once

#include <pu/Plutonium>

namespace app::ui
{
    class MainPage : public pu::ui::Layout
    {
        public:
            MainPage();
            PU_SMART_CTOR(MainPage)
            void onInput(u64 Down, u64 Up, u64 Held, pu::ui::TouchPoint Pos);
        private:
            s32 menuItemCount = 0;
            bool appletFinished;
            pu::ui::elm::Menu::Ref optionMenu;
            pu::ui::elm::MenuItem::Ref sdInstallMenuItem;
#ifdef ENABLE_NET
            pu::ui::elm::MenuItem::Ref netInstallMenuItem;
            void NetInstallMenuItem_Click();
#endif
            pu::ui::elm::MenuItem::Ref usbInstallMenuItem;
            pu::ui::elm::MenuItem::Ref udiskInstallMenuItem;
            pu::ui::elm::MenuItem::Ref settingsMenuItem;
            pu::ui::elm::MenuItem::Ref exitMenuItem;
            void SdInstallMenuItem_Click();
            void UsbInstallMenuItem_Click();
            void UdiskInstallMenuItem_Click();
            void SettingsMenuItem_Click();
            void mainMenuThread();
            void MenuAddItem(pu::ui::elm::Menu::Ref& menu, pu::ui::elm::MenuItem::Ref& Item);
    };
}
