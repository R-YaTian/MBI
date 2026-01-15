#include "ui/MainApplication.hpp"
#include "manager.hpp"
#include "util/i18n.hpp"
#include "util/config.hpp"
#include "nx/fs.hpp"
#include "nx/misc.hpp"

namespace app::ui {
    MainApplication *mainApp;

    #define _UI_MAINAPP_MENU_SET_BASE(layout) { \
        layout->SetBackgroundColor(COLOR("#670000FF")); \
        layout->SetBackgroundImage(this->bgImg); \
        layout->Add(this->topRect); \
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

    void MainApplication::OnLoad() {
        mainApp = this;

        app::i18n::Load(app::config::languageSetting);

        this->checkboxBlank = LoadTexture("romfs:/images/icons/checkbox-blank-outline.png");
        this->checkboxTick = LoadTexture("romfs:/images/icons/check-box-outline.png");
        this->bgImg = LoadBackground(app::config::storagePath);
        this->logoImg = LoadTexture("romfs:/images/logo.png");
        this->dirbackImg = LoadTexture("romfs:/images/icons/folder-upload.png");
        this->dirImg = LoadTexture("romfs:/images/icons/folder.png");

        batteryCurrentValue = 255;

        this->topRect = pu::ui::elm::Rectangle::New(0, 0, 1920, 94, COLOR("#170909FF"));
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

        this->mainPage = MainPage::New();
        this->netinstPage = netInstPage::New();
        this->sdinstPage = sdInstPage::New();
        this->usbinstPage = usbInstPage::New();
        this->usbhddinstPage = usbHDDInstPage::New();
        this->instpage = InstallerPage::New();
        this->optionspage = OptionsPage::New();
        this->mainPage->SetOnInput(std::bind(&MainPage::onInput, this->mainPage, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
        this->netinstPage->SetOnInput(std::bind(&netInstPage::onInput, this->netinstPage, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
        this->sdinstPage->SetOnInput(std::bind(&sdInstPage::onInput, this->sdinstPage, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
        this->usbinstPage->SetOnInput(std::bind(&usbInstPage::onInput, this->usbinstPage, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
        this->usbhddinstPage->SetOnInput(std::bind(&usbHDDInstPage::onInput, this->usbhddinstPage, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
        this->instpage->SetOnInput(std::bind(&InstallerPage::onInput, this->instpage, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
        this->optionspage->SetOnInput(std::bind(&OptionsPage::onInput, this->optionspage, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
        _UI_MAINAPP_MENU_SET_BASE(this->mainPage);
        _UI_MAINAPP_MENU_SET_BASE(this->optionspage);
        _UI_MAINAPP_MENU_SET_BASE(this->instpage);
        _UI_MAINAPP_MENU_SET_BASE(this->netinstPage);
        _UI_MAINAPP_MENU_SET_BASE(this->sdinstPage);
        _UI_MAINAPP_MENU_SET_BASE(this->usbinstPage);
        _UI_MAINAPP_MENU_SET_BASE(this->usbhddinstPage);

        this->AddRenderCallback(std::bind(&MainApplication::UpdateStats, this));
        this->LoadLayout(this->mainPage);
    }
}
