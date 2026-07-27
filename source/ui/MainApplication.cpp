#include "util/i18n.hpp"
#include "util/config.hpp"
#include "util/util.hpp"
#include "nx/fs.hpp"
#include "nx/misc.hpp"
#include "nx/acc.hpp"
#include "ui/MainApplication.hpp"
#include "ui/ClickableImage.hpp"
#include "ui/BaseMenuPage.hpp"
#include "ui/OptionsPage.hpp"
#include "ui/UsbInstallPage.hpp"
#include "ui/MtpInstallPage.hpp"
#include "ui/InstallerPage.hpp"
#include "ui/LocalInstallPage.hpp"
#include "ui/TicketPage.hpp"
#include "ui/MainPage.hpp"
#include "facade.hpp"

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
    TicketPage::Ref ticketPage;
    MainPage::Ref mainPage;
    ClickableImage::Ref backButton;
    ClickableImage::Ref confirmButton;
    ClickableImage::Ref pageUpButton;
    ClickableImage::Ref pageDownButton;
    ClickableImage::Ref selectAllButton;
    ClickableImage::Ref userImage;

    static s32 previousTouchCount = 0;

    #define _UI_MAINAPP_MENU_SET_BASE(layout) { \
        layout->SetBackgroundImage(this->bgImg); \
        layout->Add(this->topRect); \
        layout->Add(this->botRect); \
        layout->Add(this->botText); \
        layout->Add(this->titleImage); \
        layout->Add(this->appVersionText); \
        layout->Add(this->batteryValueText); \
        layout->Add(this->appletText); \
        layout->Add(this->freeSpaceText); \
        layout->Add(this->dateText); \
        layout->Add(this->timeText); \
        layout->Add(this->userNameText); \
        layout->Add(userImage); \
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

    pu::sdl2::TextureHandle::Ref MainApplication::LoadBackground(const std::string& bgDir)
    {
        static const std::vector<std::string> exts = {".png", ".jpg", ".bmp", ".webp"};
        for (auto const& ext : exts)
        {
            auto path = bgDir + "/background" + ext;
            if (nx::fs::Exists(path))
            {
                return LoadTexture(path);
            }
        }
        return LoadTexture("romfs:/images/background.webp");
    }

    static std::string GetBatteryColor(u32 batteryValue)
    {
        if (batteryValue <= 15)
        {
            return "#FF0000FF"; // red
        }
        else if (batteryValue <= 30)
        {
            return "#FF8000FF"; // orange
        }
        else if (batteryValue <= 50)
        {
            return "#FFFF00FF"; // yellow
        }
        else
        {
            return "#00FF00FF"; // green
        }
    }

    static std::string GetFreeSpaceInfoForDisplay()
    {
        s64 sizeSd = nx::fs::GetFreeSpaceSize(FsContentStorageId_SdCard);
        s64 sizeUser = nx::fs::GetFreeSpaceSize(FsContentStorageId_User);
        std::string sizeStr = "SD: " + nx::fs::FormatSizeString(sizeSd) + " | User: " + nx::fs::FormatSizeString(sizeUser);
        return sizeStr;
    }

    void MainApplication::UpdateStats()
    {
        const auto newfreeSpaceText = GetFreeSpaceInfoForDisplay();
        if (freeSpaceCurrentText != newfreeSpaceText)
        {
            freeSpaceCurrentText = newfreeSpaceText;
            this->freeSpaceText->SetText(freeSpaceCurrentText);
        }

        const auto newBatteryValue = nx::misc::GetBatteryValue();
        if (batteryCurrentValue != newBatteryValue)
        {
            batteryCurrentValue = newBatteryValue;
            const auto batteryColor = GetBatteryColor(batteryCurrentValue);
            const auto batteryText = batteryCurrentValue == 255 ? "??%" : std::to_string(batteryCurrentValue) + "%";
            this->batteryValueText->SetColor(COLOR(batteryColor));
            this->batteryValueText->SetText("misc.battery_charge"_lang + ": " + batteryText);
        }

        const auto newDateText = util::GetCurrentDate();
        if (dateCurrentText != newDateText)
        {
            dateCurrentText = newDateText;
            this->dateText->SetText(dateCurrentText);
        }

        const auto newTimeText = util::GetCurrentTime(app::config::use12hTime);
        if (timeCurrentText != newTimeText)
        {
            timeCurrentText = newTimeText;
            this->timeText->SetText(timeCurrentText);
        }

        const auto selectedUser = nx::acc::GetSelectedUser();
        if (!nx::acc::EqualUids(&selectedUser, &this->currentSelectedUser))
        {
            this->currentSelectedUser = selectedUser;
            if (nx::acc::HasSelectedUser())
            {
                std::vector<u8> iconData = nx::acc::GetSelectedUserIcon();
                userImage->SetImage(LoadTexture(iconData.data(), iconData.size()));
                const auto rc = nx::acc::ReadSelectedUser(&this->currentProfileBase, nullptr);
                if (R_SUCCEEDED(rc))
                {
                    userNameText->SetText("misc.user"_lang + "\n" + std::string(this->currentProfileBase.nickname));
                }
                else
                {
                    userNameText->SetText("misc.user"_lang + "\n" + "misc.unsel"_lang);
                }
            }
            else
            {
                userImage->SetImage(this->defaultUserImg);
                userNameText->SetText("misc.user"_lang + "\n" + "misc.unsel"_lang);
            }
            userImage->SetWidth(80);
            userImage->SetHeight(80);
        }
    }

    void MainApplication::OnLoad()
    {
        mainApp = this;

        this->checkboxBlank = LoadTexture("romfs:/images/icons/checkbox-blank-outline.webp");
        this->checkboxTick = LoadTexture("romfs:/images/icons/check-box-outline.webp");
        this->bgImg = LoadBackground(app::config::storagePath);
        this->logoImg = LoadTexture("romfs:/images/logo.webp");
        this->dirbackImg = LoadTexture("romfs:/images/icons/folder-upload.webp");
        this->dirImg = LoadTexture("romfs:/images/icons/folder.webp");
        this->backImg = LoadTexture("romfs:/images/icons/backward.webp");
        this->confirmImg = LoadTexture("romfs:/images/icons/confirm.webp");
        this->selectAllImg = LoadTexture("romfs:/images/icons/select-all.webp");
        this->pageUpImg = LoadTexture("romfs:/images/icons/page-up.webp");
        this->pageDownImg = LoadTexture("romfs:/images/icons/page-down.webp");
        this->defaultUserImg = LoadTexture("romfs:/images/icon.webp");

        this->topRect = pu::ui::elm::Rectangle::New(0, 0, 1920, 94, COLOR("#000000c0"));
        this->botRect = pu::ui::elm::Rectangle::New(0, 660 * pu::ui::render::ScreenFactor, 1920, 60 * pu::ui::render::ScreenFactor, COLOR("#000000c0"));
        this->botText = pu::ui::elm::TextBlock::New(10 * pu::ui::render::ScreenFactor, 1020, "");
        this->botText->SetFont("DefaultFont@30");
        this->botText->SetColor(COLOR(app::config::BottomInfoTextColor));
        this->infoRect = pu::ui::elm::Rectangle::New(0, 94, 1920, 60, COLOR("#00000080"));
        this->pageInfoText = pu::ui::elm::TextBlock::New(10, 108, "");
        this->pageInfoText->SetFont("DefaultFont@30");
        this->pageInfoText->SetColor(COLOR(app::config::TopInfoTextColor));
        this->titleImage = pu::ui::elm::Image::New(0, 0, this->logoImg);
        this->appVersionText = pu::ui::elm::TextBlock::New(480, 26, APPVER);
        this->appVersionText->SetFont("DefaultFont@42");
        this->appVersionText->SetColor(COLOR("#FFFFFFFF"));
        this->batteryValueText = pu::ui::elm::TextBlock::New(1105, 9, "misc.battery_charge"_lang + ": ??%");
        this->batteryValueText->SetFont("DefaultFont@32");
        this->freeSpaceText = pu::ui::elm::TextBlock::New(1105, 49, freeSpaceCurrentText);
        this->freeSpaceText->SetFont("DefaultFont@32");
        this->freeSpaceText->SetColor(COLOR("#FFFFFFFF"));
        this->appletText = pu::ui::elm::TextBlock::New(1437, 9, appletGetAppletType() == AppletType_LibraryApplet ? "misc.applet_mode"_lang : "");
        this->appletText->SetFont("DefaultFont@32");
        this->appletText->SetColor(COLOR("#FF0000FF"));
        this->dateText = pu::ui::elm::TextBlock::New(1700, 9, dateCurrentText);
        this->dateText->SetFont("DefaultFont@32");
        this->dateText->SetColor(COLOR("#FFFFFFFF"));
        this->timeText = pu::ui::elm::TextBlock::New(1700, 49, timeCurrentText);
        this->timeText->SetFont("DefaultFont@32");
        this->timeText->SetColor(COLOR("#FFFFFFFF"));
        this->userNameText = pu::ui::elm::TextBlock::New(750, 10, "misc.user"_lang + "\n" + "misc.unsel"_lang);
        this->userNameText->SetFont("DefaultFont@32");
        this->userNameText->SetColor(COLOR("#FFFFFFFF"));

        // Setup clickable buttons
        userImage = ClickableImage::New(660, 7, this->defaultUserImg);
        userImage->SetWidth(80);
        userImage->SetHeight(80);
        userImage->SetOnClick(std::bind(&MainApplication::UserActions, this));
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

        nx::acc::SelectFromPreselectedUser();
        this->UpdateStats();

        mainPage = MainPage::New();
        localinstPage = LocalInstallPage::New();
        usbinstPage = UsbInstallPage::New();
        mtpinstPage = MtpInstallPage::New();
        installerPage = InstallerPage::New();
        ticketPage = TicketPage::New();
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
        _UI_MAINAPP_MENU_SET_BASE(ticketPage);
        _UI_MAINAPP_MENU_SET_EXTRA(ticketPage);

#ifdef ENABLE_NET
        netinstPage = NetInstallPage::New();
        _UI_MAINAPP_MENU_SET_BASE(netinstPage);
        _UI_MAINAPP_MENU_SET_EXTRA(netinstPage);
#endif

        this->AddRenderCallback(std::bind(&MainApplication::UpdateStats, this));
        // Go to main menu
        SceneJump(Scene::Main);
    }

    std::string MainApplication::GetFinalResourcePath(const std::string& type, const std::string& name)
    {
        std::string finalPath = "romfs:/" + type + "/" + name;
        if (nx::fs::Exists(app::config::storagePath + "/" + name))
        {
            finalPath = app::config::storagePath + "/" + name;
        }
        else if (!nx::fs::Exists(finalPath))
        {
            return "";
        }
        return finalPath;
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

    void MainApplication::UserActions()
    {
        if (nx::acc::HasSelectedUser())
        {
            bool isLinked = nx::acc::IsLinked();
            u32 userCount = nx::acc::GetUserCount();
            std::vector<std::string> optionsList;
            if (isLinked)
            {
                optionsList.push_back("user_actions.unlink"_lang);
            }
            else
            {
                optionsList.push_back("user_actions.link"_lang);
            }
            optionsList.push_back("user_actions.sel_user"_lang);
            optionsList.push_back("common.cancel"_lang);
            if (!isLinked && userCount > 1)
            {
                optionsList.push_back("user_actions.del_user"_lang);
            }
            int ret = app::facade::ShowDialog("user_actions.title"_lang, "user_actions.desc"_lang, optionsList, false, "warning");
            if (ret < 0)
            {
                return;
            }
            switch (ret)
            {
                case 0: // Link or Unlink
                {
                    std::string requestConfirm = nx::misc::OpenSoftwareKeyboard("common.confirm"_lang, "", 2);
                    if (requestConfirm != "OK")
                    {
                        return;
                    }
                    svcSleepThread(2e+6); // 2ms
                    app::facade::SendRenderRequest();
                    if (isLinked)
                    {
                        Result ret = nx::acc::UnlinkLocally();
                        app::facade::ShowDialog("user_actions.unlink"_lang,
                                                R_SUCCEEDED(ret) ? "common.success"_lang : "common.fail"_lang,
                                                {"common.ok"_lang}, false, "information");
                    }
                    else
                    {
                        nx::acc::LinkLocally();
                    }
                    return;
                }
                case 1: // Select user
                    break;
                case 2: // Cancel
                    return;
                case 3: // Delete user (only shows when the user is not linked and there are more than 1 users)
                {
                    std::string requestConfirm = nx::misc::OpenSoftwareKeyboard("common.confirm"_lang, "", 2);
                    if (requestConfirm != "OK")
                    {
                        return;
                    }
                    Result ret = nx::acc::DeleteUser();
                    app::facade::ShowDialog("user_actions.del_user"_lang,
                                            R_SUCCEEDED(ret) ? "common.success"_lang : "common.fail"_lang,
                                            {"common.ok"_lang}, false, "information");
                    return;
                }
                default:
                    return;
            }
        }

        nx::acc::SelectUser();
    }

    void SceneJump(Scene idx)
    {
        userImage->SetEnabled(true);
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
            mainApp->SetPageInfoText("");
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
            mainApp->LoadLayout(localinstPage);
            break;
        case Scene::UdiskInstall:
            if (localinstPage->setStorageSourceToUdisk())
            {
                mainApp->SetTouchButtonAreaType(TouchButtonAreaType::Full);
                mainApp->SetPageInfoText("inst.hdd.top_info"_lang);
                mainApp->SetBottomText("inst.hdd.buttons"_lang);
                mainApp->LoadLayout(localinstPage);
            }
            break;
        case Scene::MtpInstall:
            userImage->SetEnabled(false);
            mainApp->SetTouchButtonAreaType(TouchButtonAreaType::Hide);
            mainApp->SetPageInfoText("");
            mtpinstPage->Prepare();
            mtpinstPage->onInitInstallMode();
            mainApp->SetBottomText("inst.usb.buttons"_lang);
            mainApp->LoadLayout(mtpinstPage);
            break;
        case Scene::TicketManager:
            mainApp->SetTouchButtonAreaType(TouchButtonAreaType::Hide);
            mainApp->SetPageInfoText("ticket_manager.scanning"_lang);
            mainApp->SetBottomText("common.waiting"_lang);
            mainApp->LoadLayout(ticketPage);
            if (ticketPage->LoadTickets())
            {
                mainApp->SetTouchButtonAreaType(TouchButtonAreaType::Full);
            }
            break;
        case Scene::Installer:
            userImage->SetEnabled(false);
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
