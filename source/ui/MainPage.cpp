#include "ui/MainApplication.hpp"
#include "ui/MainPage.hpp"
#include "util/config.hpp"
#include "util/i18n.hpp"
#include "manager.hpp"
#include "nx/BufferedPlaceholderWriter.hpp"
#include "nx/udisk.hpp"
#include "nx/usb.hpp"
#include "nx/network.hpp"

namespace app::ui {
    extern MainApplication *mainApp;
    static s32 prev_touchcount = 0;
    bool appletFinished = false;

    void MainPage::mainMenuThread() {
        bool menuLoaded = mainApp->IsShown();
        if (!appletFinished && appletGetAppletType() == AppletType_LibraryApplet) {
            nx::data::NUM_BUFFER_SEGMENTS = 2;
            if (menuLoaded) {
                app::ui::appletFinished = true;
                mainApp->CreateShowDialog("main.applet.title"_lang, "main.applet.desc"_lang, {"common.ok"_lang}, true);
            }
        } else if (!appletFinished) {
            app::ui::appletFinished = true;
            nx::data::NUM_BUFFER_SEGMENTS = 128;
        }
    }

    MainPage::MainPage() : Layout::Layout() {
        this->botRect = pu::ui::elm::Rectangle::New(0, 659 * pu::ui::render::ScreenFactor, 1920, 92, COLOR("#17090980"));
        this->butText = pu::ui::elm::TextBlock::New(10 * pu::ui::render::ScreenFactor, 678 * pu::ui::render::ScreenFactor, "main.buttons"_lang);
        this->butText->SetFont("DefaultFont@30");
        this->butText->SetColor(COLOR(app::config::BottomInfoTextColor));
        this->optionMenu = pu::ui::elm::Menu::New(0, 95, 1920, COLOR("#67000000"), COLOR("#00000033"), app::config::mainMenuItemSize, (894 / app::config::mainMenuItemSize));
        this->optionMenu->SetScrollbarColor(COLOR("#170909FF"));
        this->optionMenu->SetItemAlphaIncrementSteps(1);
        this->optionMenu->SetShadowBaseAlpha(0);
        this->sdInstallMenuItem = pu::ui::elm::MenuItem::New("main.menu.sd"_lang);
        this->sdInstallMenuItem->SetColor(COLOR(app::config::MenuTextColor));
        this->sdInstallMenuItem->SetIcon(app::manager::LoadTexture("romfs:/images/icons/micro-sd.png"));
        this->netInstallMenuItem = pu::ui::elm::MenuItem::New("main.menu.net"_lang);
        this->netInstallMenuItem->SetColor(COLOR(app::config::MenuTextColor));
        this->netInstallMenuItem->SetIcon(app::manager::LoadTexture("romfs:/images/icons/cloud-download.png"));
        this->usbInstallMenuItem = pu::ui::elm::MenuItem::New("main.menu.usb"_lang);
        this->usbInstallMenuItem->SetColor(COLOR(app::config::MenuTextColor));
        this->usbInstallMenuItem->SetIcon(app::manager::LoadTexture("romfs:/images/icons/usb-port.png"));
        this->udiskInstallMenuItem = pu::ui::elm::MenuItem::New("main.menu.hdd"_lang);
        this->udiskInstallMenuItem->SetColor(COLOR(app::config::MenuTextColor));
        this->udiskInstallMenuItem->SetIcon(app::manager::LoadTexture("romfs:/images/icons/disk.png"));
        this->settingsMenuItem = pu::ui::elm::MenuItem::New("main.menu.set"_lang);
        this->settingsMenuItem->SetColor(COLOR(app::config::MenuTextColor));
        this->settingsMenuItem->SetIcon(app::manager::LoadTexture("romfs:/images/icons/settings.png"));
        this->exitMenuItem = pu::ui::elm::MenuItem::New("main.menu.exit"_lang);
        this->exitMenuItem->SetColor(COLOR(app::config::MenuTextColor));
        this->exitMenuItem->SetIcon(app::manager::LoadTexture("romfs:/images/icons/exit-run.png"));
        this->Add(this->botRect);
        this->Add(this->butText);
        this->optionMenu->AddItem(this->sdInstallMenuItem);
        this->optionMenu->AddItem(this->netInstallMenuItem);
        this->optionMenu->AddItem(this->usbInstallMenuItem);
        this->optionMenu->AddItem(this->udiskInstallMenuItem);
        this->optionMenu->AddItem(this->settingsMenuItem);
        this->optionMenu->AddItem(this->exitMenuItem);
        this->Add(this->optionMenu);
        this->AddRenderCallback(std::bind(&MainPage::mainMenuThread, this));
    }

    void MainPage::SdInstallMenuItem_Click() {
        mainApp->sdinstPage->drawMenuItems(true, "sdmc:/");
        mainApp->sdinstPage->menu->SetSelectedIndex(0);
        mainApp->LoadLayout(mainApp->sdinstPage);
    }

    void MainPage::NetInstallMenuItem_Click() {
        if (nx::network::getIPAddress() == "1.0.0.127")
        {
            app::ui::mainApp->CreateShowDialog("main.net.title"_lang, "main.net.desc"_lang, {"common.ok"_lang}, true);
            return;
        }
        mainApp->netinstPage->startNetwork();
    }

    void MainPage::UsbInstallMenuItem_Click() {
        if (!app::config::usbAck)
        {
            if (mainApp->CreateShowDialog("main.usb.warn.title"_lang, "main.usb.warn.desc"_lang, {"common.ok"_lang, "main.usb.warn.opt1"_lang}, false) == 1)
            {
                app::config::usbAck = true;
                app::config::SaveSettings();
            }
        }
        if (nx::usb::usbDeviceIsConnected())
        {
            mainApp->usbinstPage->startUsb();
        }
        else
        {
            mainApp->CreateShowDialog("main.usb.error.title"_lang, "main.usb.error.desc"_lang, {"common.ok"_lang}, false);
        }
    }

    void MainPage::UdiskInstallMenuItem_Click() {
		if(nx::udisk::getDeviceCount() && nx::udisk::getMountPointName()) {
			mainApp->usbhddinstPage->drawMenuItems(true, nx::udisk::getMountPointName());
			mainApp->usbhddinstPage->menu->SetSelectedIndex(0);
			mainApp->LoadLayout(mainApp->usbhddinstPage);
		} else {
			app::ui::mainApp->CreateShowDialog("main.hdd.title"_lang, "main.hdd.notfound"_lang, {"common.ok"_lang}, true);
		}
    }

    void MainPage::ExitMenuItem_Click() {
        mainApp->FadeOut();
        mainApp->Close();
    }

    void MainPage::SettingsMenuItem_Click() {
        mainApp->LoadLayout(mainApp->optionspage);
    }

    void MainPage::onInput(u64 Down, u64 Up, u64 Held, pu::ui::TouchPoint Pos) {
        if ((Down & HidNpadButton_Plus) && mainApp->IsShown()) {
            mainApp->FadeOut();
            mainApp->Close();
        }

        if ((Down & HidNpadButton_A) || (mainApp->GetTouchState().count == 0 && prev_touchcount == 1))
        {
            prev_touchcount = 0;
            switch (this->optionMenu->GetSelectedIndex())
            {
                case 0:
                    this->SdInstallMenuItem_Click();
                    break;
                case 1:
                    this->NetInstallMenuItem_Click();
                    break;
                case 2:
                    this->UsbInstallMenuItem_Click();
                    break;
                case 3:
                    this->UdiskInstallMenuItem_Click();
                    break;
                case 4:
                    this->SettingsMenuItem_Click();
                    break;
                case 5:
                    this->ExitMenuItem_Click();
                    break;
                default:
                    break;
            }
        }

        if (mainApp->GetTouchState().count == 1)
        {
            prev_touchcount = 1;
        }
    }
}
