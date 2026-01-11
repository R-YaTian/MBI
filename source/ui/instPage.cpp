#include <filesystem>
#include "ui/MainApplication.hpp"
#include "ui/instPage.hpp"
#include "util/util.hpp"
#include "util/config.hpp"
#include "util/lang.hpp"
#include "nx/fs.hpp"

#define COLOR(hex) pu::ui::Color::FromHex(hex)

namespace app::ui {
    extern MainApplication *mainApp;
    static std::string getFreeSpaceText = nx::fs::GetFreeStorageSpace();
    static std::string getFreeSpaceOldText = getFreeSpaceText;
    static std::string* getBatteryChargeText = app::util::getBatteryCharge();
    static std::string* getBatteryChargeOldText = getBatteryChargeText;

    instPage::instPage() : Layout::Layout() {
        this->SetBackgroundColor(COLOR("#670000FF"));
        this->SetBackgroundImage(mainApp->bgImg);
        this->topRect = Rectangle::New(0, 0, 1920, 94, COLOR("#170909FF"));
        this->infoRect = Rectangle::New(0, 94, 1920, 60 * pu::ui::render::ScreenFactor, COLOR("#17090980"));
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
        this->pageInfoText = TextBlock::New(10, 121, "");
        this->pageInfoText->SetFont("DefaultFont@30");
        this->pageInfoText->SetColor(COLOR(app::config::themeColorTextTopInfo));
        this->installInfoText = TextBlock::New(15, 568 * pu::ui::render::ScreenFactor, "");
        this->installInfoText->SetFont("DefaultFont@30");
        this->installInfoText->SetColor(COLOR(app::config::themeColorTextInstall));
        this->installBar = pu::ui::elm::ProgressBar::New(10, 600 * pu::ui::render::ScreenFactor, 850 * pu::ui::render::ScreenFactor, 40 * pu::ui::render::ScreenFactor, 100.0f);
        this->installBar->SetBackgroundColor(COLOR("#222222FF"));
        this->Add(this->topRect);
        this->Add(this->infoRect);
        this->Add(this->titleImage);
        this->Add(this->appVersionText);
        this->Add(this->batteryValueText);
        this->Add(this->freeSpaceText);
        this->Add(this->pageInfoText);
        this->Add(this->installInfoText);
        this->Add(this->installBar);
        this->updateStatsThread();
        this->AddRenderCallback(std::bind(&instPage::updateStatsThread, this));
    }

    void instPage::setTopInstInfoText(std::string ourText){
        mainApp->instpage->pageInfoText->SetText(ourText);
        mainApp->CallForRender();
    }

    void instPage::setInstInfoText(std::string ourText){
        mainApp->instpage->installInfoText->SetText(ourText);
        mainApp->CallForRender();
    }

    void instPage::setInstBarPerc(double ourPercent){
        mainApp->instpage->installBar->SetVisible(true);
        mainApp->instpage->installBar->SetProgress(ourPercent < 2.0f ? 2.0f : ourPercent);
        mainApp->CallForRender();
    }

    void instPage::loadMainMenu(){
        mainApp->LoadLayout(mainApp->mainPage);
    }

    void instPage::loadInstallScreen(){
        mainApp->instpage->pageInfoText->SetText("");
        mainApp->instpage->installInfoText->SetText("");
        mainApp->instpage->installBar->SetProgress(0);
        mainApp->instpage->installBar->SetVisible(false);
        mainApp->LoadLayout(mainApp->instpage);
        mainApp->CallForRender();
    }

    void instPage::onInput(u64 Down, u64 Up, u64 Held, pu::ui::TouchPoint Pos) {
    }

    void instPage::updateStatsThread() {
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
