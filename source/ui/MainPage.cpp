#include "ui/MainApplication.hpp"
#include "ui/MainPage.hpp"
#include "util/config.hpp"
#include "util/i18n.hpp"
#include "facade.hpp"
#include "nx/BufferedPlaceholderWriter.hpp"
#include "nx/usb.hpp"

#ifdef ENABLE_NET
#include "nx/network.hpp"
#endif

namespace app::ui
{
    void MainPage::mainMenuThread()
    {
        bool menuLoaded = IsShown();
        if (!appletFinished && appletGetAppletType() == AppletType_LibraryApplet)
        {
            nx::data::NUM_BUFFER_SEGMENTS = 2;
            if (menuLoaded)
            {
                appletFinished = true;
                app::facade::ShowDialog("main.applet.title"_lang, "main.applet.desc"_lang, {"common.ok"_lang}, true);
            }
        }
        else if (!appletFinished)
        {
            appletFinished = true;
            nx::data::NUM_BUFFER_SEGMENTS = 128;
        }
    }

    MainPage::MainPage() : Layout::Layout()
    {
        appletFinished = false;
        this->optionMenu = pu::ui::elm::Menu::New(0, 95, 1920, COLOR("#67000000"), COLOR("#00000033"), app::config::mainMenuItemSize, (894 / app::config::mainMenuItemSize));
        this->optionMenu->SetScrollbarColor(COLOR("#170909FF"));
        this->optionMenu->SetShadowBaseAlpha(0);
        this->sdInstallMenuItem = pu::ui::elm::MenuItem::New("main.menu.sd"_lang);
        this->sdInstallMenuItem->SetColor(COLOR(app::config::MenuTextColor));
        this->sdInstallMenuItem->SetIcon(LoadTexture("romfs:/images/icons/micro-sd.png"));
        this->sdInstallMenuItem->AddOnKey(std::bind(&MainPage::SdInstallMenuItem_Click, this), HidNpadButton_A | HidNpadButton_Verification);
#ifdef ENABLE_NET
        this->netInstallMenuItem = pu::ui::elm::MenuItem::New("main.menu.net"_lang);
        this->netInstallMenuItem->SetColor(COLOR(app::config::MenuTextColor));
        this->netInstallMenuItem->SetIcon(LoadTexture("romfs:/images/icons/cloud-download.png"));
        this->netInstallMenuItem->AddOnKey(std::bind(&MainPage::NetInstallMenuItem_Click, this), HidNpadButton_A | HidNpadButton_Verification);
#endif
        this->usbInstallMenuItem = pu::ui::elm::MenuItem::New("main.menu.usb"_lang);
        this->usbInstallMenuItem->SetColor(COLOR(app::config::MenuTextColor));
        this->usbInstallMenuItem->SetIcon(LoadTexture("romfs:/images/icons/usb-port.png"));
        this->usbInstallMenuItem->AddOnKey(std::bind(&MainPage::UsbInstallMenuItem_Click, this), HidNpadButton_A | HidNpadButton_Verification);
        this->udiskInstallMenuItem = pu::ui::elm::MenuItem::New("main.menu.hdd"_lang);
        this->udiskInstallMenuItem->SetColor(COLOR(app::config::MenuTextColor));
        this->udiskInstallMenuItem->SetIcon(LoadTexture("romfs:/images/icons/disk.png"));
        this->udiskInstallMenuItem->AddOnKey(std::bind(&MainPage::UdiskInstallMenuItem_Click, this), HidNpadButton_A | HidNpadButton_Verification);
        this->settingsMenuItem = pu::ui::elm::MenuItem::New("main.menu.set"_lang);
        this->settingsMenuItem->SetColor(COLOR(app::config::MenuTextColor));
        this->settingsMenuItem->SetIcon(LoadTexture("romfs:/images/icons/settings.png"));
        this->settingsMenuItem->AddOnKey(std::bind(&MainPage::SettingsMenuItem_Click, this), HidNpadButton_A | HidNpadButton_Verification);
        this->exitMenuItem = pu::ui::elm::MenuItem::New("main.menu.exit"_lang);
        this->exitMenuItem->SetColor(COLOR(app::config::MenuTextColor));
        this->exitMenuItem->SetIcon(LoadTexture("romfs:/images/icons/exit-run.png"));
        MenuAddItem(this->optionMenu, this->sdInstallMenuItem);
#ifdef ENABLE_NET
        MenuAddItem(this->optionMenu, this->netInstallMenuItem);
#endif
        MenuAddItem(this->optionMenu, this->usbInstallMenuItem);
        MenuAddItem(this->optionMenu, this->udiskInstallMenuItem);
        MenuAddItem(this->optionMenu, this->settingsMenuItem);
        MenuAddItem(this->optionMenu, this->exitMenuItem);
        this->Add(this->optionMenu);
        this->AddRenderCallback(std::bind(&MainPage::mainMenuThread, this));
    }

    void MainPage::MenuAddItem(pu::ui::elm::Menu::Ref& menu, pu::ui::elm::MenuItem::Ref& Item)
    {
        menu->AddItem(Item);
        menuItemCount++;
    }

    void MainPage::SdInstallMenuItem_Click()
    {
        SceneJump(Scene::SdInstall);
    }

#ifdef ENABLE_NET
    void MainPage::NetInstallMenuItem_Click()
    {
        if (nx::network::getIPAddress() == "1.0.0.127")
        {
            app::facade::ShowDialog("main.net.title"_lang, "main.net.desc"_lang, {"common.ok"_lang}, true);
            return;
        }
        SceneJump(Scene::NetworkInstall);
    }
#endif

    void MainPage::UsbInstallMenuItem_Click()
    {
        if (!app::config::usbAck)
        {
            if (app::facade::ShowDialog("main.usb.warn.title"_lang, "main.usb.warn.desc"_lang, {"common.ok"_lang, "main.usb.warn.opt1"_lang}, false) == 1)
            {
                app::config::usbAck = true;
                app::config::SaveSettings();
            }
        }
        if (nx::usb::usbDeviceIsConnected())
        {
            SceneJump(Scene::UsbInstall);
        }
        else
        {
            app::facade::ShowDialog("main.usb.error.title"_lang, "main.usb.error.desc"_lang, {"common.ok"_lang}, false);
        }
    }

    void MainPage::UdiskInstallMenuItem_Click()
    {
        SceneJump(Scene::UdiskInstall);
    }

    void MainPage::SettingsMenuItem_Click()
    {
        SceneJump(Scene::Options);
    }

    void MainPage::onInput(u64 Down, u64 Up, u64 Held, pu::ui::TouchPoint Pos)
    {
        if ((Down & HidNpadButton_Plus) && IsShown())
        {
            CloseWithFadeOut();
        }

        if ((Down & HidNpadButton_A) || (!IsTouched() && previousTouchCount == 1))
        {
            previousTouchCount = 0;
            if (this->optionMenu->GetSelectedIndex() == menuItemCount - 1)
            {
                CloseWithFadeOut();
            }
        }

        if (IsTouched())
        {
            previousTouchCount = 1;
        }
    }
}
