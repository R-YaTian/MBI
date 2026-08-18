#include "ui/MainApplication.hpp"
#include "ui/MainPage.hpp"
#include "util/config.hpp"
#include "util/i18n.hpp"
#include "facade.hpp"
#include "manager.hpp"
#include "nx/BufferedPlaceholderWriter.hpp"
#include "nx/usb.hpp"
#include "nx/mtp.hpp"

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
            if (menuLoaded)
            {
                appletFinished = true;
                if (!app::config::appletAck)
                {
                    if (app::facade::ShowDialog("main.applet.title"_lang,
                                                "main.applet.desc"_lang,
                                               {"common.ok"_lang, "common.donot_show_again"_lang}, false, "warning") == 1)
                    {
                        app::config::appletAck = true;
                        app::config::SaveSettings();
                    }
                }
            }
        }
        else if (!appletFinished)
        {
            appletFinished = true;
        }
    }

    MainPage::MainPage() : Layout::Layout()
    {
        appletFinished = false;
        this->SetOnInput(std::bind(&MainPage::onInput, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
        this->optionMenu = pu::ui::elm::Menu::New(0, 95_dp, 1920_dp, COLOR("#67000000"), COLOR("#5F5F5FFF"), config::GetMainMenuItemSize(), config::GetMainMenuHeight() / config::GetMainMenuItemSize());
        this->optionMenu->SetItemsFocusBorderRadius(this->optionMenu->GetItemsFocusBorderRadius() / config::GetScreenScaleFactor());
        this->optionMenu->SetIconMargin(37_dp);
        this->optionMenu->SetTextMargin(37_dp);
        this->sdInstallMenuItem = pu::ui::elm::MenuItem::New("main.menu.sd"_lang);
        this->sdInstallMenuItem->SetColor(COLOR(app::config::MenuTextColor));
        this->sdInstallMenuItem->SetIcon(LoadTexture("romfs:/images/icons/micro-sd.webp"));
        this->sdInstallMenuItem->AddOnKey(std::bind(&MainPage::SdInstallMenuItem_Click, this), HidNpadButton_A | HidNpadButton_Verification);
#ifdef ENABLE_NET
        this->netInstallMenuItem = pu::ui::elm::MenuItem::New("main.menu.net"_lang);
        this->netInstallMenuItem->SetColor(COLOR(app::config::MenuTextColor));
        this->netInstallMenuItem->SetIcon(LoadTexture("romfs:/images/icons/cloud-download.webp"));
        this->netInstallMenuItem->AddOnKey(std::bind(&MainPage::NetInstallMenuItem_Click, this), HidNpadButton_A | HidNpadButton_Verification);
#endif
        this->usbInstallMenuItem = pu::ui::elm::MenuItem::New("main.menu.usb"_lang);
        this->usbInstallMenuItem->SetColor(COLOR(app::config::MenuTextColor));
        this->usbInstallMenuItem->SetIcon(LoadTexture("romfs:/images/icons/usb-port.webp"));
        this->usbInstallMenuItem->AddOnKey(std::bind(&MainPage::UsbInstallMenuItem_Click, this), HidNpadButton_A | HidNpadButton_Verification);
        this->udiskInstallMenuItem = pu::ui::elm::MenuItem::New("main.menu.hdd"_lang);
        this->udiskInstallMenuItem->SetColor(COLOR(app::config::MenuTextColor));
        this->udiskInstallMenuItem->SetIcon(LoadTexture("romfs:/images/icons/disk.webp"));
        this->udiskInstallMenuItem->AddOnKey(std::bind(&MainPage::UdiskInstallMenuItem_Click, this), HidNpadButton_A | HidNpadButton_Verification);
        this->mtpInstallMenuItem = pu::ui::elm::MenuItem::New("main.menu.mtp"_lang);
        this->mtpInstallMenuItem->SetColor(COLOR(app::config::MenuTextColor));
        this->mtpInstallMenuItem->SetIcon(LoadTexture("romfs:/images/icons/usb-mtp.webp"));
        this->mtpInstallMenuItem->AddOnKey(std::bind(&MainPage::MtpInstallMenuItem_Click, this), HidNpadButton_A | HidNpadButton_Verification);
        this->ticketManagerMenuItem = pu::ui::elm::MenuItem::New("main.menu.tickets"_lang);
        this->ticketManagerMenuItem->SetColor(COLOR(app::config::MenuTextColor));
        this->ticketManagerMenuItem->SetIcon(LoadTexture("romfs:/images/icons/ticket.webp"));
        this->ticketManagerMenuItem->AddOnKey(std::bind(&MainPage::TicketManagerMenuItem_Click, this), HidNpadButton_A | HidNpadButton_Verification);
        this->settingsMenuItem = pu::ui::elm::MenuItem::New("main.menu.set"_lang);
        this->settingsMenuItem->SetColor(COLOR(app::config::MenuTextColor));
        this->settingsMenuItem->SetIcon(LoadTexture("romfs:/images/icons/settings.webp"));
        this->settingsMenuItem->AddOnKey(std::bind(&MainPage::SettingsMenuItem_Click, this), HidNpadButton_A | HidNpadButton_Verification);
        this->exitMenuItem = pu::ui::elm::MenuItem::New("main.menu.exit"_lang);
        this->exitMenuItem->SetColor(COLOR(app::config::MenuTextColor));
        this->exitMenuItem->SetIcon(LoadTexture("romfs:/images/icons/exit-run.webp"));
        this->exitMenuItem->AddOnKey(std::bind(&MainPage::ExitMenuItem_Click, this), HidNpadButton_A | HidNpadButton_Verification);
        this->optionMenu->AddItem(this->sdInstallMenuItem);
#ifdef ENABLE_NET
        this->optionMenu->AddItem(this->netInstallMenuItem);
#endif
        this->optionMenu->AddItem(this->usbInstallMenuItem);
        this->optionMenu->AddItem(this->udiskInstallMenuItem);
        this->optionMenu->AddItem(this->mtpInstallMenuItem);
        this->optionMenu->AddItem(this->ticketManagerMenuItem);
        this->optionMenu->AddItem(this->settingsMenuItem);
        this->optionMenu->AddItem(this->exitMenuItem);
        this->Add(this->optionMenu);
        this->AddRenderCallback(std::bind(&MainPage::mainMenuThread, this));
    }

    void MainPage::SdInstallMenuItem_Click()
    {
        if (inputGuard)
        {
            return;
        }
        SceneJump(Scene::SdInstall);
    }

#ifdef ENABLE_NET
    void MainPage::NetInstallMenuItem_Click()
    {
        if (inputGuard)
        {
            return;
        }
        if (nx::network::GetIPAddress() == "1.0.0.127")
        {
            app::facade::ShowDialog("main.net.title"_lang, "main.net.desc"_lang, {"common.ok"_lang}, true, "information");
            return;
        }
        SceneJump(Scene::NetworkInstall);
    }
#endif

    void MainPage::UsbInstallMenuItem_Click()
    {
        if (inputGuard)
        {
            return;
        }
        if (!app::config::usbAck)
        {
            if (app::facade::ShowDialog("common.warning"_lang,
                                        "main.usb.warn_desc"_lang,
                                       {"common.ok"_lang, "common.donot_show_again"_lang}, false,
                                        "warning") == 1)
            {
                app::config::usbAck = true;
                app::config::SaveSettings();
            }
        }
        nx::usb::usbDeviceInitialize();
        SceneJump(Scene::UsbInstall);
    }

    void MainPage::UdiskInstallMenuItem_Click()
    {
        if (inputGuard)
        {
            return;
        }
        SceneJump(Scene::UdiskInstall);
    }

    void MainPage::MtpInstallMenuItem_Click()
    {
        if (inputGuard)
        {
            return;
        }
        SceneJump(Scene::MtpInstall);
        nx::mtp::Setup(app::manager::getAppPath());
    }

    void MainPage::TicketManagerMenuItem_Click()
    {
        if (inputGuard)
        {
            return;
        }
        SceneJump(Scene::TicketManager);
    }

    void MainPage::SettingsMenuItem_Click()
    {
        if (inputGuard)
        {
            return;
        }
        SceneJump(Scene::Options);
    }

    void MainPage::ExitMenuItem_Click()
    {
        if (inputGuard)
        {
            return;
        }
        inputGuard = true;
        CloseWithFadeOut();
    }

    void MainPage::onInput(const u64 Down, const u64 Up, const u64 Held, const pu::ui::TouchPoint Pos)
    {
        if (inputGuard)
        {
            return;
        }

        if ((Down & HidNpadButton_Plus) && IsShown())
        {
            ExitMenuItem_Click();
        }
    }
}
