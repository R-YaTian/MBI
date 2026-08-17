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

#ifdef __DEBUG__
#include <malloc.h>
extern "C"
{
    extern char* fake_heap_start;
    extern char* fake_heap_end;
}
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
        static u64 lastStatsUpdateTick = 0;
        const u64 currentTick = armGetSystemTick();
        const u64 statsRefreshInterval = armGetSystemTickFreq();
        if (lastStatsUpdateTick != 0 && currentTick - lastStatsUpdateTick < statsRefreshInterval)
        {
            return;
        }
        lastStatsUpdateTick = currentTick;

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
            userImage->SetWidth(80_dp);
            userImage->SetHeight(80_dp);
        }
#ifdef __DEBUG__
        auto info = mallinfo();
        size_t mem_used = info.arena, mem_total = static_cast<size_t>(fake_heap_end - fake_heap_start);
        if (mem_used > 0)
        {
            const auto mem_used_str = nx::fs::FormatSizeString(mem_used);
            const auto mem_total_str = nx::fs::FormatSizeString(mem_total);
            this->appletText->SetText(mem_used_str + "/" + mem_total_str);
            this->appletText->SetX(1335_dp);
        }
#endif
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

        this->topRect = pu::ui::elm::Rectangle::New(0, 0, 1920_dp, 94_dp, COLOR("#000000c0"));
        this->botRect = pu::ui::elm::Rectangle::New(0, 990_dp, 1920_dp, 90_dp, COLOR("#000000c0"));
        this->botText = pu::ui::elm::TextBlock::New(15_dp, 1020_dp, "");
        this->botText->SetFont(GetDefaultFont(pu::ui::DefaultFontSize::Medium));
        this->botText->SetColor(COLOR(app::config::BottomInfoTextColor));
        this->infoRect = pu::ui::elm::Rectangle::New(0, 94_dp, 1920_dp, 60_dp, COLOR("#00000080"));
        this->pageInfoText = pu::ui::elm::TextBlock::New(10_dp, 108_dp, "");
        this->pageInfoText->SetFont(GetDefaultFont(pu::ui::DefaultFontSize::Medium));
        this->pageInfoText->SetColor(COLOR(app::config::TopInfoTextColor));
        this->titleImage = pu::ui::elm::Image::New(0, 0, this->logoImg);
        this->titleImage->SetWidth(this->titleImage->GetWidth() / app::config::GetScreenScaleFactor());
        this->titleImage->SetHeight(this->titleImage->GetHeight() / app::config::GetScreenScaleFactor());
        this->appVersionText = pu::ui::elm::TextBlock::New(476_dp, 52_dp, APPVER);
        this->appVersionText->SetFont(GetDefaultFont(pu::ui::DefaultFontSize::Small));
        this->appVersionText->SetColor(COLOR("#FFFFFFFF"));
        this->batteryValueText = pu::ui::elm::TextBlock::New(1105_dp, 9_dp, "misc.battery_charge"_lang + ": ??%");
        this->batteryValueText->SetFont(GetDefaultFont(pu::ui::DefaultFontSize::Medium));
        this->freeSpaceText = pu::ui::elm::TextBlock::New(1105_dp, 49_dp, freeSpaceCurrentText);
        this->freeSpaceText->SetFont(GetDefaultFont(pu::ui::DefaultFontSize::Medium));
        this->freeSpaceText->SetColor(COLOR("#FFFFFFFF"));
        this->appletText = pu::ui::elm::TextBlock::New(1437_dp, 9_dp, appletGetAppletType() == AppletType_LibraryApplet ? "misc.applet_mode"_lang : "");
        this->appletText->SetFont(GetDefaultFont(pu::ui::DefaultFontSize::Medium));
        this->appletText->SetColor(COLOR("#FF0000FF"));
        this->dateText = pu::ui::elm::TextBlock::New(1700_dp, 9_dp, dateCurrentText);
        this->dateText->SetFont(GetDefaultFont(pu::ui::DefaultFontSize::Medium));
        this->dateText->SetColor(COLOR("#FFFFFFFF"));
        this->timeText = pu::ui::elm::TextBlock::New(1700_dp, 49_dp, timeCurrentText);
        this->timeText->SetFont(GetDefaultFont(pu::ui::DefaultFontSize::Medium));
        this->timeText->SetColor(COLOR("#FFFFFFFF"));
        this->userNameText = pu::ui::elm::TextBlock::New(750_dp, 10_dp, "misc.user"_lang + "\n" + "misc.unsel"_lang);
        this->userNameText->SetFont(GetDefaultFont(pu::ui::DefaultFontSize::Medium));
        this->userNameText->SetColor(COLOR("#FFFFFFFF"));

        // Setup clickable buttons
        userImage = ClickableImage::New(660_dp, 7_dp, this->defaultUserImg);
        userImage->SetWidth(80_dp);
        userImage->SetHeight(80_dp);
        userImage->SetOnClick(std::bind(&MainApplication::UserActions, this));
        backButton = ClickableImage::New(1820_dp, 990_dp, this->backImg);
        backButton->SetWidth(backButton->GetWidth() / app::config::GetScreenScaleFactor());
        backButton->SetHeight(backButton->GetHeight() / app::config::GetScreenScaleFactor());
        confirmButton = ClickableImage::New(1710_dp, 990_dp, this->confirmImg);
        confirmButton->SetWidth(confirmButton->GetWidth() / app::config::GetScreenScaleFactor());
        confirmButton->SetHeight(confirmButton->GetHeight() / app::config::GetScreenScaleFactor());
        selectAllButton = ClickableImage::New(1600_dp, 990_dp, this->selectAllImg);
        selectAllButton->SetWidth(selectAllButton->GetWidth() / app::config::GetScreenScaleFactor());
        selectAllButton->SetHeight(selectAllButton->GetHeight() / app::config::GetScreenScaleFactor());
        pageDownButton = ClickableImage::New(1490_dp, 990_dp, this->pageDownImg);
        pageDownButton->SetWidth(pageDownButton->GetWidth() / app::config::GetScreenScaleFactor());
        pageDownButton->SetHeight(pageDownButton->GetHeight() / app::config::GetScreenScaleFactor());
        pageUpButton = ClickableImage::New(1380_dp, 990_dp, this->pageUpImg);
        pageUpButton->SetWidth(pageUpButton->GetWidth() / app::config::GetScreenScaleFactor());
        pageUpButton->SetHeight(pageUpButton->GetHeight() / app::config::GetScreenScaleFactor());
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
            mainApp->SetPageInfoText("inst.sd.source_string"_lang + "inst.top_info"_lang);
            mainApp->SetBottomText("inst.sd.buttons"_lang);
            localinstPage->setStorageSourceToSdmc();
            mainApp->LoadLayout(localinstPage);
            break;
        case Scene::UdiskInstall:
            if (localinstPage->setStorageSourceToUdisk())
            {
                mainApp->SetTouchButtonAreaType(TouchButtonAreaType::Full);
                mainApp->SetPageInfoText("inst.hdd.source_string"_lang + "inst.top_info"_lang);
                mainApp->SetBottomText("inst.sd.buttons"_lang);
                mainApp->LoadLayout(localinstPage);
            }
            break;
        case Scene::MtpInstall:
            userImage->SetEnabled(false);
            mainApp->SetTouchButtonAreaType(TouchButtonAreaType::Hide);
            mainApp->SetPageInfoText("");
            mtpinstPage->Prepare();
            mtpinstPage->onInitInstallMode(app::config::mtpInstallTargetStorage);
            mainApp->SetBottomText("inst.mtp.buttons"_lang);
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
