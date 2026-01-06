#include <filesystem>
#include <switch.h>
#include "ui/MainApplication.hpp"
#include "ui/mainPage.hpp"
#include "util/util.hpp"
#include "util/config.hpp"
#include "util/lang.hpp"
#include "nx/BufferedPlaceholderWriter.hpp"
#include "nx/fs.hpp"
#include "nx/udisk.hpp"

#define COLOR(hex) pu::ui::Color::FromHex(hex)

namespace app::ui {
    extern MainApplication *mainApp;
    static s32 prev_touchcount = 0;
    bool appletFinished = false;
    static std::string getFreeSpaceText = nx::fs::GetFreeStorageSpace();
    static std::string getFreeSpaceOldText = getFreeSpaceText;
    static std::string* getBatteryChargeText = app::util::getBatteryCharge();
    static std::string* getBatteryChargeOldText = getBatteryChargeText;

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
        this->updateStatsThread();
    }

    MainPage::MainPage() : Layout::Layout() {
        this->SetBackgroundColor(COLOR("#670000FF"));
        this->SetBackgroundImage(mainApp->bgImg);
        this->topRect = Rectangle::New(0, 0, 1920, 94, COLOR("#170909FF"));
        this->botRect = Rectangle::New(0, 659 * pu::ui::render::ScreenFactor, 1920, 92, COLOR("#17090980"));
        this->titleImage = Image::New(0, 0, mainApp->logoImg);
        this->appVersionText = TextBlock::New(490, 29, "v" + app::config::appVersion);
        this->appVersionText->SetFont("DefaultFont@42");
        this->appVersionText->SetColor(COLOR("#FFFFFFFF"));
        this->batteryValueText = TextBlock::New(700 * pu::ui::render::ScreenFactor, 9, "misc.battery_charge"_lang+": " + getBatteryChargeText[0]);
        this->batteryValueText->SetFont("DefaultFont@32");
        this->batteryValueText->SetColor(COLOR(getBatteryChargeText[1]));
        this->freeSpaceText = TextBlock::New(700 * pu::ui::render::ScreenFactor, 49, "misc.sd_free"_lang+": " + getFreeSpaceText);
        this->freeSpaceText->SetFont("DefaultFont@32");
        this->freeSpaceText->SetColor(COLOR("#FFFFFFFF"));
        this->butText = TextBlock::New(10 * pu::ui::render::ScreenFactor, 678 * pu::ui::render::ScreenFactor, "main.buttons"_lang);
        this->butText->SetFont("DefaultFont@30");
        this->butText->SetColor(COLOR(app::config::themeColorTextBottomInfo));
        this->optionMenu = pu::ui::elm::Menu::New(0, 95, 1920, COLOR("#67000000"), COLOR("#00000033"), app::config::mainMenuItemSize, (894 / app::config::mainMenuItemSize));
        this->optionMenu->SetScrollbarColor(COLOR("#170909FF"));
        this->optionMenu->SetItemAlphaIncrementSteps(1);
        this->optionMenu->SetShadowBaseAlpha(0);
        this->installMenuItem = pu::ui::elm::MenuItem::New("main.menu.sd"_lang);
        this->installMenuItem->SetColor(COLOR(app::config::themeColorTextMenu));
        this->installMenuItem->SetIcon(app::util::LoadTexture("romfs:/images/icons/micro-sd.png"));
        this->netInstallMenuItem = pu::ui::elm::MenuItem::New("main.menu.net"_lang);
        this->netInstallMenuItem->SetColor(COLOR(app::config::themeColorTextMenu));
        this->netInstallMenuItem->SetIcon(app::util::LoadTexture("romfs:/images/icons/cloud-download.png"));
        this->usbInstallMenuItem = pu::ui::elm::MenuItem::New("main.menu.usb"_lang);
        this->usbInstallMenuItem->SetColor(COLOR(app::config::themeColorTextMenu));
        this->usbInstallMenuItem->SetIcon(app::util::LoadTexture("romfs:/images/icons/usb-port.png"));
        this->usbHDDInstallMenuItem = pu::ui::elm::MenuItem::New("main.menu.hdd"_lang);
        this->usbHDDInstallMenuItem->SetColor(COLOR(app::config::themeColorTextMenu));
        this->usbHDDInstallMenuItem->SetIcon(app::util::LoadTexture("romfs:/images/icons/disk.png"));
        this->settingsMenuItem = pu::ui::elm::MenuItem::New("main.menu.set"_lang);
        this->settingsMenuItem->SetColor(COLOR(app::config::themeColorTextMenu));
        this->settingsMenuItem->SetIcon(app::util::LoadTexture("romfs:/images/icons/settings.png"));
        this->exitMenuItem = pu::ui::elm::MenuItem::New("main.menu.exit"_lang);
        this->exitMenuItem->SetColor(COLOR(app::config::themeColorTextMenu));
        this->exitMenuItem->SetIcon(app::util::LoadTexture("romfs:/images/icons/exit-run.png"));
        this->Add(this->topRect);
        this->Add(this->botRect);
        this->Add(this->titleImage);
        this->Add(this->appVersionText);
        this->Add(this->batteryValueText);
        this->Add(this->freeSpaceText);
        this->Add(this->butText);
        this->optionMenu->AddItem(this->installMenuItem);
        this->optionMenu->AddItem(this->netInstallMenuItem);
        this->optionMenu->AddItem(this->usbInstallMenuItem);
        this->optionMenu->AddItem(this->usbHDDInstallMenuItem);
        this->optionMenu->AddItem(this->settingsMenuItem);
        this->optionMenu->AddItem(this->exitMenuItem);
        this->Add(this->optionMenu);
        this->updateStatsThread();
        this->AddRenderCallback(std::bind(&MainPage::mainMenuThread, this));
    }

    void MainPage::installMenuItem_Click() {
        mainApp->sdinstPage->drawMenuItems(true, "sdmc:/");
        mainApp->sdinstPage->menu->SetSelectedIndex(0);
        mainApp->LoadLayout(mainApp->sdinstPage);
    }

    void MainPage::netInstallMenuItem_Click() {
        if (app::util::getIPAddress() == "1.0.0.127") {
            app::ui::mainApp->CreateShowDialog("main.net.title"_lang, "main.net.desc"_lang, {"common.ok"_lang}, true);
            return;
        }
        mainApp->netinstPage->startNetwork();
    }

    void MainPage::usbInstallMenuItem_Click() {
        if (!app::config::usbAck) {
            if (mainApp->CreateShowDialog("main.usb.warn.title"_lang, "main.usb.warn.desc"_lang, {"common.ok"_lang, "main.usb.warn.opt1"_lang}, false) == 1) {
                app::config::usbAck = true;
                app::config::setConfig();
            }
        }
        if (app::util::usbIsConnected()) mainApp->usbinstPage->startUsb();
        else mainApp->CreateShowDialog("main.usb.error.title"_lang, "main.usb.error.desc"_lang, {"common.ok"_lang}, false);
    }

    void MainPage::usbHDDInstallMenuItem_Click() {
		if(nx::udisk::getDeviceCount() && nx::udisk::getMountPointName()) {
			mainApp->usbhddinstPage->drawMenuItems(true, nx::udisk::getMountPointName());
			mainApp->usbhddinstPage->menu->SetSelectedIndex(0);
			mainApp->LoadLayout(mainApp->usbhddinstPage);
		} else {
			app::ui::mainApp->CreateShowDialog("main.hdd.title"_lang, "main.hdd.notfound"_lang, {"common.ok"_lang}, true);
		}
    }

    void MainPage::exitMenuItem_Click() {
        mainApp->FadeOut();
        mainApp->Close();
    }

    void MainPage::settingsMenuItem_Click() {
        mainApp->LoadLayout(mainApp->optionspage);
    }

    void MainPage::onInput(u64 Down, u64 Up, u64 Held, pu::ui::TouchPoint Pos) {
        if ((Down & HidNpadButton_Plus) && mainApp->IsShown()) {
            mainApp->FadeOut();
            mainApp->Close();
        }

        if ((Down & HidNpadButton_A) || (mainApp->GetTouchState().count == 0 && prev_touchcount == 1)) {
            prev_touchcount = 0;
            switch (this->optionMenu->GetSelectedIndex()) {
                case 0:
                    this->installMenuItem_Click();
                    break;
                case 1:
                    this->netInstallMenuItem_Click();
                    break;
                case 2:
                    this->usbInstallMenuItem_Click();
                    break;
                case 3:
                    this->usbHDDInstallMenuItem_Click();
                    break;
                case 4:
                    this->settingsMenuItem_Click();
                    break;
                case 5:
                    this->exitMenuItem_Click();
                    break;
                default:
                    break;
            }
        }
        if (mainApp->GetTouchState().count == 1)
            prev_touchcount = 1;
    }

    void MainPage::updateStatsThread() {
        getFreeSpaceText = nx::fs::GetFreeStorageSpace();
        if (getFreeSpaceOldText != getFreeSpaceText) {
            getFreeSpaceOldText = getFreeSpaceText;
            mainApp->instpage->freeSpaceText->SetText("misc.sd_free"_lang+": " + getFreeSpaceText);
            mainApp->usbhddinstPage->freeSpaceText->SetText("misc.sd_free"_lang+": " + getFreeSpaceText);
            mainApp->sdinstPage->freeSpaceText->SetText("misc.sd_free"_lang+": " + getFreeSpaceText);
            mainApp->netinstPage->freeSpaceText->SetText("misc.sd_free"_lang+": " + getFreeSpaceText);
            mainApp->usbinstPage->freeSpaceText->SetText("misc.sd_free"_lang+": " + getFreeSpaceText);
            mainApp->mainPage->freeSpaceText->SetText("misc.sd_free"_lang+": " + getFreeSpaceText);
            mainApp->optionspage->freeSpaceText->SetText("misc.sd_free"_lang+": " + getFreeSpaceText);
        }

        getBatteryChargeText = app::util::getBatteryCharge();
        if (getBatteryChargeOldText[0] != getBatteryChargeText[0]) {
            getBatteryChargeOldText = getBatteryChargeText;

            mainApp->instpage->batteryValueText->SetColor(COLOR(getBatteryChargeText[1]));
            mainApp->usbhddinstPage->batteryValueText->SetColor(COLOR(getBatteryChargeText[1]));
            mainApp->sdinstPage->batteryValueText->SetColor(COLOR(getBatteryChargeText[1]));
            mainApp->netinstPage->batteryValueText->SetColor(COLOR(getBatteryChargeText[1]));
            mainApp->usbinstPage->batteryValueText->SetColor(COLOR(getBatteryChargeText[1]));
            mainApp->mainPage->batteryValueText->SetColor(COLOR(getBatteryChargeText[1]));
            mainApp->optionspage->batteryValueText->SetColor(COLOR(getBatteryChargeText[1]));

            mainApp->instpage->batteryValueText->SetText("misc.battery_charge"_lang+": " + getBatteryChargeText[0]);
            mainApp->usbhddinstPage->batteryValueText->SetText("misc.battery_charge"_lang+": " + getBatteryChargeText[0]);
            mainApp->sdinstPage->batteryValueText->SetText("misc.battery_charge"_lang+": " + getBatteryChargeText[0]);
            mainApp->netinstPage->batteryValueText->SetText("misc.battery_charge"_lang+": " + getBatteryChargeText[0]);
            mainApp->usbinstPage->batteryValueText->SetText("misc.battery_charge"_lang+": " + getBatteryChargeText[0]);
            mainApp->mainPage->batteryValueText->SetText("misc.battery_charge"_lang+": " + getBatteryChargeText[0]);
            mainApp->optionspage->batteryValueText->SetText("misc.battery_charge"_lang+": " + getBatteryChargeText[0]);
        }
    }
}
