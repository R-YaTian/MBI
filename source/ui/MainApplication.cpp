#include "ui/MainApplication.hpp"
#include "util/lang.hpp"
#include "util/config.hpp"
#include "util/util.hpp"

namespace app::ui {
    MainApplication *mainApp;

    void MainApplication::OnLoad() {
        mainApp = this;

        Language::Load();

        this->checkboxBlank = app::util::LoadTexture("romfs:/images/icons/checkbox-blank-outline.png");
        this->checkboxTick = app::util::LoadTexture("romfs:/images/icons/check-box-outline.png");
        this->bgImg = app::util::LoadBackground(app::config::appDir);
        this->logoImg = app::util::LoadTexture("romfs:/images/logo.png");
        this->dirbackImg = app::util::LoadTexture("romfs:/images/icons/folder-upload.png");
        this->dirImg = app::util::LoadTexture("romfs:/images/icons/folder.png");

        this->mainPage = MainPage::New();
        this->netinstPage = netInstPage::New();
        this->sdinstPage = sdInstPage::New();
        this->usbinstPage = usbInstPage::New();
        this->usbhddinstPage = usbHDDInstPage::New();
        this->instpage = instPage::New();
        this->optionspage = optionsPage::New();
        this->mainPage->SetOnInput(std::bind(&MainPage::onInput, this->mainPage, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
        this->netinstPage->SetOnInput(std::bind(&netInstPage::onInput, this->netinstPage, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
        this->sdinstPage->SetOnInput(std::bind(&sdInstPage::onInput, this->sdinstPage, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
        this->usbinstPage->SetOnInput(std::bind(&usbInstPage::onInput, this->usbinstPage, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
        this->usbhddinstPage->SetOnInput(std::bind(&usbHDDInstPage::onInput, this->usbhddinstPage, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
        this->instpage->SetOnInput(std::bind(&instPage::onInput, this->instpage, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
        this->optionspage->SetOnInput(std::bind(&optionsPage::onInput, this->optionspage, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
        this->LoadLayout(this->mainPage);
    }
}