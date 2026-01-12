#include "ui/MainApplication.hpp"
#include "ui/InstallerPage.hpp"
#include "util/config.hpp"

#define COLOR(hex) pu::ui::Color::FromHex(hex)

namespace app::ui {
    extern MainApplication *mainApp;

    InstallerPage::InstallerPage() : Layout::Layout() {
        this->infoRect = pu::ui::elm::Rectangle::New(0, 94, 1920, 60 * pu::ui::render::ScreenFactor, COLOR("#17090980"));
        this->pageInfoText = pu::ui::elm::TextBlock::New(10, 121, "");
        this->pageInfoText->SetFont("DefaultFont@30");
        this->pageInfoText->SetColor(COLOR(app::config::themeColorTextTopInfo));
        this->installInfoText = pu::ui::elm::TextBlock::New(15, 568 * pu::ui::render::ScreenFactor, "");
        this->installInfoText->SetFont("DefaultFont@30");
        this->installInfoText->SetColor(COLOR(app::config::themeColorTextInstall));
        this->installBar = pu::ui::elm::ProgressBar::New(10, 600 * pu::ui::render::ScreenFactor, 850 * pu::ui::render::ScreenFactor, 40 * pu::ui::render::ScreenFactor, 100.0f);
        this->installBar->SetBackgroundColor(COLOR("#222222FF"));
        this->Add(this->infoRect);
        this->Add(this->pageInfoText);
        this->Add(this->installInfoText);
        this->Add(this->installBar);
    }

    void InstallerPage::setTopInstInfoText(std::string ourText){
        mainApp->instpage->pageInfoText->SetText(ourText);
        mainApp->CallForRender();
    }

    void InstallerPage::setInstInfoText(std::string ourText){
        mainApp->instpage->installInfoText->SetText(ourText);
        mainApp->CallForRender();
    }

    void InstallerPage::setInstBarPerc(double ourPercent){
        mainApp->instpage->installBar->SetVisible(true);
        mainApp->instpage->installBar->SetProgress(ourPercent < 2.0f ? 2.0f : ourPercent);
        mainApp->CallForRender();
    }

    void InstallerPage::loadMainMenu(){
        mainApp->LoadLayout(mainApp->mainPage);
    }

    void InstallerPage::loadInstallScreen(){
        mainApp->instpage->pageInfoText->SetText("");
        mainApp->instpage->installInfoText->SetText("");
        mainApp->instpage->installBar->SetProgress(0);
        mainApp->instpage->installBar->SetVisible(false);
        mainApp->LoadLayout(mainApp->instpage);
        mainApp->CallForRender();
    }

    void InstallerPage::onInput(u64 Down, u64 Up, u64 Held, pu::ui::TouchPoint Pos)
    {
    }
}
