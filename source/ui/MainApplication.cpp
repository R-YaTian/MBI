#include "util/i18n.hpp"
#include "util/config.hpp"
#include "nx/fs.hpp"
#include "nx/misc.hpp"
#include "nx/udisk.hpp"
#include "ui/MainApplication.hpp"
#include "ui/ClickableImage.hpp"
#include "ui/BaseMenuPage.hpp"
#include "ui/OptionsPage.hpp"
#include "ui/UsbInstallPage.hpp"
#include "ui/MtpInstallPage.hpp"
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
    MtpInstallPage::Ref mtpinstPage;
    InstallerPage::Ref installerPage;
    LocalInstallPage::Ref localinstPage;
    MainPage::Ref mainPage;
    ClickableImage::Ref backButton;
    ClickableImage::Ref confirmButton;
    ClickableImage::Ref pageUpButton;
    ClickableImage::Ref pageDownButton;
    ClickableImage::Ref selectAllButton;

    static s32 previousTouchCount = 0;

    #define _UI_MAINAPP_MENU_SET_BASE(layout) { \
        layout->SetBackgroundImage(this->bgImg); \
        layout->Add(this->topRect); \
        layout->Add(this->botRect); \
        layout->Add(this->botText); \
        layout->Add(this->titleImage); \
        layout->Add(this->appVersionText); \
        layout->Add(this->batteryValueText); \
        layout->Add(this->freeSpaceText); \
        layout->Add(this->dateText); \
        layout->Add(this->timeText); \
    }

    #define _UI_MAINAPP_MENU_SET_EXTRA(layout) { \
        layout->Add(this->infoRect); \
        layout->Add(this->pageInfoText); \
        layout->Add(backButton); \
        layout->Add(confirmButton); \
        layout->Add(pageUpButton); \
        layout->Add(pageDownButton); \
        layout->Add(selectAllButton); \
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
        return LoadTexture("romfs:/images/background.png");
    }

    void MainApplication::UpdateStats()
    {
        const auto newfreeSpaceText = nx::misc::GetFreeSpaceInfo();
        if (freeSpaceCurrentText != newfreeSpaceText)
        {
            freeSpaceCurrentText = newfreeSpaceText;
            this->freeSpaceText->SetText(freeSpaceCurrentText);
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

        const auto newDateText = nx::misc::GetCurrentDate();
        if (dateCurrentText != newDateText)
        {
            dateCurrentText = newDateText;
            this->dateText->SetText(dateCurrentText);
        }

        const auto newTimeText = nx::misc::GetCurrentTime(app::config::use12hTime);
        if (timeCurrentText != newTimeText)
        {
            timeCurrentText = newTimeText;
            this->timeText->SetText(timeCurrentText);
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
        this->backImg = LoadTexture("romfs:/images/icons/backward.png");
        this->confirmImg = LoadTexture("romfs:/images/icons/confirm.png");
        this->selectAllImg = LoadTexture("romfs:/images/icons/select-all.png");
        this->pageUpImg = LoadTexture("romfs:/images/icons/page-up.png");
        this->pageDownImg = LoadTexture("romfs:/images/icons/page-down.png");

        batteryCurrentValue = 255;

        this->topRect = pu::ui::elm::Rectangle::New(0, 0, 1920, 94, COLOR("#000000c0"));
        this->botRect = pu::ui::elm::Rectangle::New(0, 660 * pu::ui::render::ScreenFactor, 1920, 60 * pu::ui::render::ScreenFactor, COLOR("#000000c0"));
        this->botText = pu::ui::elm::TextBlock::New(10 * pu::ui::render::ScreenFactor, 676 * pu::ui::render::ScreenFactor, "");
        this->botText->SetFont("DefaultFont@30");
        this->botText->SetColor(COLOR(app::config::BottomInfoTextColor));
        this->infoRect = pu::ui::elm::Rectangle::New(0, 94, 1920, 60, COLOR("#00000080"));
        this->pageInfoText = pu::ui::elm::TextBlock::New(10, 103, "");
        this->pageInfoText->SetFont("DefaultFont@30");
        this->pageInfoText->SetColor(COLOR(app::config::TopInfoTextColor));
        this->titleImage = pu::ui::elm::Image::New(0, 0, this->logoImg);
        this->appVersionText = pu::ui::elm::TextBlock::New(490, 29, "v" + app::config::appVersion);
        this->appVersionText->SetFont("DefaultFont@42");
        this->appVersionText->SetColor(COLOR("#FFFFFFFF"));
        this->batteryValueText = pu::ui::elm::TextBlock::New(700 * pu::ui::render::ScreenFactor, 9, "misc.battery_charge"_lang + ": ??%");
        this->batteryValueText->SetFont("DefaultFont@32");
        this->freeSpaceText = pu::ui::elm::TextBlock::New(700 * pu::ui::render::ScreenFactor, 49, freeSpaceCurrentText);
        this->freeSpaceText->SetFont("DefaultFont@32");
        this->freeSpaceText->SetColor(COLOR("#FFFFFFFF"));
        this->dateText = pu::ui::elm::TextBlock::New(1700, 9, dateCurrentText);
        this->dateText->SetFont("DefaultFont@32");
        this->dateText->SetColor(COLOR("#FFFFFFFF"));
        this->timeText = pu::ui::elm::TextBlock::New(1700, 49, timeCurrentText);
        this->timeText->SetFont("DefaultFont@32");
        this->timeText->SetColor(COLOR("#FFFFFFFF"));

        this->UpdateStats();

        // Setup clickable buttons
        backButton = ClickableImage::New(1820, 990, this->backImg);
        confirmButton = ClickableImage::New(1710, 990, this->confirmImg);
        selectAllButton = ClickableImage::New(1600, 990, this->selectAllImg);
        pageDownButton = ClickableImage::New(1490, 990, this->pageDownImg);
        pageUpButton = ClickableImage::New(1380, 990, this->pageUpImg);
        backButton->SetOnClick([this]() {
            auto lyt = this->GetLayout<BaseMenuPage>();
            lyt->onCancel();
        });
        confirmButton->SetOnClick([this]() {
            auto lyt = this->GetLayout<BaseMenuPage>();
            lyt->onConfirm();
        });
        selectAllButton->SetOnClick([this]() {
            auto lyt = this->GetLayout<BaseMenuPage>();
            lyt->onSelectAll();
        });
        pageDownButton->SetOnClick([this]() {
            auto lyt = this->GetLayout<BaseMenuPage>();
            lyt->onPageDown();
        });
        pageUpButton->SetOnClick([this]() {
            auto lyt = this->GetLayout<BaseMenuPage>();
            lyt->onPageUp();
        });

        mainPage = MainPage::New();
        localinstPage = LocalInstallPage::New();
        usbinstPage = UsbInstallPage::New();
        mtpinstPage = MtpInstallPage::New();
        installerPage = InstallerPage::New();
        optionspage = OptionsPage::New();
        _UI_MAINAPP_MENU_SET_BASE(mainPage);
        _UI_MAINAPP_MENU_SET_BASE(optionspage);
        _UI_MAINAPP_MENU_SET_EXTRA(optionspage);
        _UI_MAINAPP_MENU_SET_BASE(installerPage);
        _UI_MAINAPP_MENU_SET_EXTRA(installerPage);
        _UI_MAINAPP_MENU_SET_BASE(localinstPage);
        _UI_MAINAPP_MENU_SET_EXTRA(localinstPage);
        _UI_MAINAPP_MENU_SET_BASE(usbinstPage);
        _UI_MAINAPP_MENU_SET_EXTRA(usbinstPage);
        _UI_MAINAPP_MENU_SET_BASE(mtpinstPage);
        _UI_MAINAPP_MENU_SET_EXTRA(mtpinstPage);

#ifdef ENABLE_NET
        netinstPage = NetInstallPage::New();
        _UI_MAINAPP_MENU_SET_BASE(netinstPage);
        _UI_MAINAPP_MENU_SET_EXTRA(netinstPage);
#endif

        this->AddRenderCallback(std::bind(&MainApplication::UpdateStats, this));
        // Go to main menu
        SceneJump(Scene::Main);
    }

    void MainApplication::SetTouchButtonAreaType(TouchButtonAreaType type)
    {
        switch (type)
        {
        case TouchButtonAreaType::Hide:
            backButton->SetVisible(false);
            confirmButton->SetVisible(false);
            selectAllButton->SetVisible(false);
            pageDownButton->SetVisible(false);
            pageUpButton->SetVisible(false);
            break;
        case TouchButtonAreaType::Base: 
            backButton->SetVisible(true);
            confirmButton->SetVisible(false);
            selectAllButton->SetVisible(false);
            pageDownButton->SetVisible(false);
            pageUpButton->SetVisible(false);
            break;
        case TouchButtonAreaType::Full: 
            backButton->SetVisible(true);
            confirmButton->SetVisible(true);
            selectAllButton->SetVisible(true);
            pageDownButton->SetVisible(true);
            pageUpButton->SetVisible(true);
            break;
        }
    }

    void SceneJump(Scene idx)
    {
        int ret = -1;
        int deviceCount = 0;
        switch (idx)
        {
        case Scene::Main:
            mainApp->SetBottomText("main.buttons"_lang);
            mainApp->LoadLayout(mainPage);
            break;
        case Scene::Options:
            mainApp->SetTouchButtonAreaType(TouchButtonAreaType::Base);
            mainApp->SetPageInfoText("options.title"_lang);
            mainApp->SetBottomText("options.buttons"_lang);
            mainApp->LoadLayout(optionspage);
            break;
        case Scene::NetworkInstall:
#ifdef ENABLE_NET
            mainApp->SetTouchButtonAreaType(TouchButtonAreaType::Hide);
            mainApp->SetBottomText("inst.net.buttons"_lang);
            mainApp->LoadLayout(netinstPage);
            if (netinstPage->startNetwork())
            {
                mainApp->SetTouchButtonAreaType(TouchButtonAreaType::Full);
            }
#endif
            break;
        case Scene::UsbInstall:
            mainApp->SetTouchButtonAreaType(TouchButtonAreaType::Hide);
            mainApp->SetPageInfoText("inst.usb.top_info"_lang);
            mainApp->SetBottomText("inst.usb.buttons"_lang);
            mainApp->LoadLayout(usbinstPage);
            if (usbinstPage->startUsb())
            {
                mainApp->SetTouchButtonAreaType(TouchButtonAreaType::Full);
            }
            break;
        case Scene::SdInstall:
            mainApp->SetTouchButtonAreaType(TouchButtonAreaType::Full);
            mainApp->SetPageInfoText("inst.sd.top_info"_lang);
            mainApp->SetBottomText("inst.sd.buttons"_lang);
            localinstPage->setStorageSourceToSdmc();
            localinstPage->drawMenuItems(true, "sdmc:");
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
                mountPointList.push_back("common.cancel"_lang);
                ret = mainApp->CreateShowDialog("main.hdd.title"_lang, "inst.hdd.multi_device_desc"_lang, mountPointList, true);
                if (ret < 0)
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
            mainApp->SetTouchButtonAreaType(TouchButtonAreaType::Full);
            mainApp->SetPageInfoText("inst.hdd.top_info"_lang);
            mainApp->SetBottomText("inst.hdd.buttons"_lang);
            localinstPage->setStorageSourceToUdisk();
            localinstPage->drawMenuItems(true, nx::udisk::getMountPointName(ret));
            mainApp->LoadLayout(localinstPage);
            break;
        case Scene::MtpInstall:
            mainApp->SetTouchButtonAreaType(TouchButtonAreaType::Hide);
            mainApp->SetPageInfoText("inst.usb.top_info"_lang);
            mainApp->SetBottomText("inst.usb.buttons"_lang);
            mainApp->LoadLayout(mtpinstPage);
            break;
        case Scene::Installer:
            mainApp->SetTouchButtonAreaType(TouchButtonAreaType::Hide);
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

    bool IsTouchUp()
    {
        s32 touchCount = mainApp->GetTouchState().count;
        if (touchCount == 0 && previousTouchCount == 1)
        {
            previousTouchCount = 0;
            return true;
        }
        return false; 
    }

    void UpdateTouchState(const pu::ui::TouchPoint pos, const s32 region_x, const s32 region_y, const s32 region_w, const s32 region_h)
    {
        s32 touchCount = mainApp->GetTouchState().count;
        if (touchCount == 1 && pos.HitsRegion(region_x, region_y, region_w, region_h))
        {
            previousTouchCount = 1;
        }
        else
        {
            previousTouchCount = 0;
        }
    }
}
