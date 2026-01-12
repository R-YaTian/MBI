#pragma once

#include <pu/Plutonium>

namespace app::ui
{
    class MainPage : public pu::ui::Layout
    {
        public:
            MainPage();
            PU_SMART_CTOR(MainPage)
            void SdInstallMenuItem_Click();
            void NetInstallMenuItem_Click();
            void UsbInstallMenuItem_Click();
            void SettingsMenuItem_Click();
            void UdiskInstallMenuItem_Click();
            void ExitMenuItem_Click();
            void onInput(u64 Down, u64 Up, u64 Held, pu::ui::TouchPoint Pos);
            void mainMenuThread();
        private:
            bool appletFinished;
            pu::ui::elm::TextBlock::Ref butText;
            pu::ui::elm::Rectangle::Ref botRect;
            pu::ui::elm::Menu::Ref optionMenu;
            pu::ui::elm::MenuItem::Ref sdInstallMenuItem;
            pu::ui::elm::MenuItem::Ref netInstallMenuItem;
            pu::ui::elm::MenuItem::Ref usbInstallMenuItem;
            pu::ui::elm::MenuItem::Ref udiskInstallMenuItem;
            pu::ui::elm::MenuItem::Ref settingsMenuItem;
            pu::ui::elm::MenuItem::Ref exitMenuItem;
    };
}
