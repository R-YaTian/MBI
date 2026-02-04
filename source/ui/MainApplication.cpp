#include "util/i18n.hpp"
#include "util/config.hpp"
#include "nx/fs.hpp"
#include "nx/misc.hpp"
#include "nx/udisk.hpp"
#include "nx/mtp.hpp"
#include "ui/MainApplication.hpp"
#include "ui/OptionsPage.hpp"
#include "ui/UsbInstallPage.hpp"
#include "ui/InstallerPage.hpp"
#include "ui/LocalInstallPage.hpp"
#include "ui/MainPage.hpp"

#ifdef ENABLE_NET
#include "ui/NetInstallPage.hpp"
#endif

namespace app::ui
{
    MainApplication *mainApp;
    OptionsPage::Ref optionspage;
#ifdef ENABLE_NET
    NetInstallPage::Ref netinstPage;
#endif
    UsbInstallPage::Ref usbinstPage;
    InstallerPage::Ref installerPage;
    LocalInstallPage::Ref localinstPage;
    MainPage::Ref mainPage;

    #define _UI_MAINAPP_MENU_SET_BASE(layout) { \
        layout->SetBackgroundColor(COLOR("#670000FF")); \
        layout->SetBackgroundImage(this->bgImg); \
        layout->Add(this->topRect); \
        layout->Add(this->botRect); \
        layout->Add(this->botText); \
        layout->Add(this->infoRect); \
        layout->Add(this->pageInfoText); \
        layout->Add(this->titleImage); \
        layout->Add(this->appVersionText); \
        layout->Add(this->batteryValueText); \
        layout->Add(this->freeSpaceText); \
    }

    pu::sdl2::TextureHandle::Ref MainApplication::LoadBackground(std::string bgDir)
    {
        static const std::vector<std::string> exts = {".png", ".jpg", ".bmp"};
        for (auto const& ext : exts)
        {
            auto path = bgDir + "/background" + ext;
            if (std::filesystem::exists(path))
            {
                return LoadTexture(path);
            }
        }
        return LoadTexture("romfs:/images/background.jpg");
    }

    void MainApplication::UpdateStats()
    {
        const auto newfreeSpaceText = nx::fs::GetFreeStorageSpace();
        if (freeSpaceCurrentText != newfreeSpaceText)
        {
            freeSpaceCurrentText = newfreeSpaceText;
            this->freeSpaceText->SetText("misc.sd_free"_lang + ": " + freeSpaceCurrentText);
        }

        const auto newBatteryValue = nx::misc::GetBatteryValue();
        if (batteryCurrentValue != newBatteryValue)
        {
            batteryCurrentValue = newBatteryValue;
            const auto batteryColor = nx::misc::GetBatteryColor(batteryCurrentValue);
            const auto batteryText = batteryCurrentValue == 255 ? "??%" : std::to_string(batteryCurrentValue) + "%";
            this->batteryValueText->SetColor(COLOR(batteryColor));
            this->batteryValueText->SetText("misc.battery_charge"_lang + ": " + batteryText);
        }
    }

    void MainApplication::OnLoad()
    {
        mainApp = this;

        app::i18n::Load(app::config::languageSetting);

        this->checkboxBlank = LoadTexture("romfs:/images/icons/checkbox-blank-outline.png");
        this->checkboxTick = LoadTexture("romfs:/images/icons/check-box-outline.png");
        this->bgImg = LoadBackground(app::config::storagePath);
        this->logoImg = LoadTexture("romfs:/images/logo.png");
        this->dirbackImg = LoadTexture("romfs:/images/icons/folder-upload.png");
        this->dirImg = LoadTexture("romfs:/images/icons/folder.png");

        batteryCurrentValue = 255;

        this->topRect = pu::ui::elm::Rectangle::New(0, 0, 1920, 94, COLOR("#170909C0"));
        this->botRect = pu::ui::elm::Rectangle::New(0, 660 * pu::ui::render::ScreenFactor, 1920, 60 * pu::ui::render::ScreenFactor, COLOR("#17090980"));
        this->botText = pu::ui::elm::TextBlock::New(10 * pu::ui::render::ScreenFactor, 676 * pu::ui::render::ScreenFactor, "");
        this->botText->SetFont("DefaultFont@30");
        this->botText->SetColor(COLOR(app::config::BottomInfoTextColor));
        this->infoRect = pu::ui::elm::Rectangle::New(0, 94, 1920, 60, COLOR("#17090980"));
        this->pageInfoText = pu::ui::elm::TextBlock::New(10, 103, "");
        this->pageInfoText->SetFont("DefaultFont@30");
        this->pageInfoText->SetColor(COLOR(app::config::TopInfoTextColor));
        this->titleImage = pu::ui::elm::Image::New(0, 0, this->logoImg);
        this->appVersionText = pu::ui::elm::TextBlock::New(490, 29, "v" + app::config::appVersion);
        this->appVersionText->SetFont("DefaultFont@42");
        this->appVersionText->SetColor(COLOR("#FFFFFFFF"));
        this->batteryValueText = pu::ui::elm::TextBlock::New(700 * pu::ui::render::ScreenFactor, 9, "misc.battery_charge"_lang + ": ??%");
        this->batteryValueText->SetFont("DefaultFont@32");
        this->freeSpaceText = pu::ui::elm::TextBlock::New(700 * pu::ui::render::ScreenFactor, 49, "misc.sd_free"_lang + ": " + freeSpaceCurrentText);
        this->freeSpaceText->SetFont("DefaultFont@32");
        this->freeSpaceText->SetColor(COLOR("#FFFFFFFF"));

        this->UpdateStats();

        mainPage = MainPage::New();
        localinstPage = LocalInstallPage::New();
        usbinstPage = UsbInstallPage::New();
        installerPage = InstallerPage::New();
        optionspage = OptionsPage::New();
        mainPage->SetOnInput(std::bind(&MainPage::onInput, mainPage, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
        localinstPage->SetOnInput(std::bind(&LocalInstallPage::onInput, localinstPage, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
        usbinstPage->SetOnInput(std::bind(&UsbInstallPage::onInput, usbinstPage, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
        optionspage->SetOnInput(std::bind(&OptionsPage::onInput, optionspage, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
        _UI_MAINAPP_MENU_SET_BASE(mainPage);
        _UI_MAINAPP_MENU_SET_BASE(optionspage);
        _UI_MAINAPP_MENU_SET_BASE(installerPage);
        _UI_MAINAPP_MENU_SET_BASE(localinstPage);
        _UI_MAINAPP_MENU_SET_BASE(usbinstPage);

#ifdef ENABLE_NET
        netinstPage = NetInstallPage::New();
        netinstPage->SetOnInput(std::bind(&NetInstallPage::onInput, netinstPage, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
        _UI_MAINAPP_MENU_SET_BASE(netinstPage);
#endif

        this->AddRenderCallback(std::bind(&MainApplication::UpdateStats, this));
        SceneJump(Scene::Main);
    }

    void SceneJump(Scene idx)
    {
        int ret = -1;
        int deviceCount = 0;
        switch (idx)
        {
        case Scene::Main:
            mainApp->HidePageInfo();
            mainApp->SetBottomText("main.buttons"_lang);
            mainApp->LoadLayout(mainPage);
            break;
        case Scene::Options:
            mainApp->ShowPageInfo();
            mainApp->SetPageInfoText("options.title"_lang);
            mainApp->SetBottomText("options.buttons"_lang);
            mainApp->LoadLayout(optionspage);
            break;
        case Scene::NetworkInstall:
#ifdef ENABLE_NET
            mainApp->ShowPageInfo();
            mainApp->SetBottomText("inst.net.buttons"_lang);
            mainApp->LoadLayout(netinstPage);
            netinstPage->startNetwork();
#endif
            break;
        case Scene::UsbInstall:
            mainApp->ShowPageInfo();
            mainApp->SetPageInfoText("inst.usb.top_info"_lang);
            mainApp->SetBottomText("inst.usb.buttons"_lang);
            mainApp->LoadLayout(usbinstPage);
            usbinstPage->startUsb();
            break;
        case Scene::SdInstall:
            mainApp->ShowPageInfo();
            mainApp->SetPageInfoText("inst.sd.top_info"_lang);
            mainApp->SetBottomText("inst.sd.buttons"_lang);
            localinstPage->drawMenuItems(true, "sdmc:");
            localinstPage->setMenuIndex(0);
            localinstPage->setStorageSourceToSdmc();
            mainApp->LoadLayout(localinstPage);
            break;
        case Scene::UdiskInstall:
            deviceCount = nx::udisk::getDeviceCount();
            if(deviceCount > 1)
            {
                std::vector<std::string> mountPointList;
                for (int i=0; i < deviceCount; i++)
                {
                    mountPointList.push_back(nx::udisk::getMountPointName(i));
                }
                ret = mainApp->CreateShowDialog("main.hdd.title"_lang, "inst.hdd.multi_device_desc"_lang, mountPointList, false);
                if (ret == -1)
                {
                    return;
                }
            }
            else if (deviceCount == 1)
            {
                ret = 0;
            }
            else
            {
                mainApp->CreateShowDialog("main.hdd.title"_lang, "main.hdd.notfound"_lang, {"common.ok"_lang}, true);
                return;
            }
            mainApp->ShowPageInfo();
            mainApp->SetPageInfoText("inst.hdd.top_info"_lang);
            mainApp->SetBottomText("inst.hdd.buttons"_lang);
            localinstPage->drawMenuItems(true, nx::udisk::getMountPointName(ret));
            localinstPage->setMenuIndex(0);
            localinstPage->setStorageSourceToUdisk();
            mainApp->LoadLayout(localinstPage);
            break;
        case Scene::MtpInstall:
            nx::mtp::Setup();
            break;
        case Scene::Installer:
            mainApp->ShowPageInfo();
            mainApp->SetPageInfoText("");
            installerPage->Prepare();
            mainApp->LoadLayout(installerPage);
            mainApp->CallForRender();
            break;
        }
    }

    pu::sdl2::TextureHandle::Ref GetResource(Resources idx)
    {
        switch (idx)
        {
        case Resources::UncheckedImage:
            return mainApp->checkboxBlank;
        case Resources::CheckedImage:
            return mainApp->checkboxTick;
        case Resources::DirectoryImage:
            return mainApp->dirImg;
        case Resources::BackToParentImage:
            return mainApp->dirbackImg;
        default:
            return nullptr;
        }
    }

    void CloseWithFadeOut()
    {
        mainApp->FadeOut();
        mainApp->Close();
    }

    bool IsShown()
    {
        return mainApp->IsShown();
    }

    bool IsTouched()
    {
        s32 touchCount = mainApp->GetTouchState().count;
        if (touchCount == 1)
        {
            return true;
        }
        else
        {
            return false;
        }
    }
}
