#pragma once

#include <pu/Plutonium>

namespace app::ui
{
    class MainPage : public pu::ui::Layout
    {
        public:
            MainPage();
            PU_SMART_CTOR(MainPage)
        private:
            bool inputGuard = false;
            bool appletFinished;
            pu::ui::elm::Menu::Ref optionMenu;
            pu::ui::elm::MenuItem::Ref sdInstallMenuItem;
#ifdef ENABLE_NET
            pu::ui::elm::MenuItem::Ref netInstallMenuItem;
            void NetInstallMenuItem_Click();
#endif
            pu::ui::elm::MenuItem::Ref usbInstallMenuItem;
            pu::ui::elm::MenuItem::Ref udiskInstallMenuItem;
            pu::ui::elm::MenuItem::Ref mtpInstallMenuItem;
            pu::ui::elm::MenuItem::Ref settingsMenuItem;
            pu::ui::elm::MenuItem::Ref exitMenuItem;
            void SdInstallMenuItem_Click();
            void UsbInstallMenuItem_Click();
            void UdiskInstallMenuItem_Click();
            void MtpInstallMenuItem_Click();
            void SettingsMenuItem_Click();
            void ExitMenuItem_Click();
            void mainMenuThread();
            void onInput(const u64 Down, const u64 Up, const u64 Held, const pu::ui::TouchPoint Pos);
    };
}
