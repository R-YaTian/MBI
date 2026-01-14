#include <switch.h>
#include "ui/MainApplication.hpp"
#include "ui/MainPage.hpp"
#include "ui/netInstPage.hpp"
#include "util/util.hpp"
#include "util/config.hpp"
#include "util/i18n.hpp"
#include "netInstall.hpp"
#include "nx/network.hpp"
#include "nx/misc.hpp"
#include "manager.hpp"

namespace app::ui {
    extern MainApplication *mainApp;
    static s32 prev_touchcount = 0;
    std::string lastFileID = "";
    std::string sourceString = "";

    netInstPage::netInstPage() : Layout::Layout()
    {
        this->infoRect = pu::ui::elm::Rectangle::New(0, 94, 1920, 60, COLOR("#17090980"));
        this->botRect = pu::ui::elm::Rectangle::New(0, 660 * pu::ui::render::ScreenFactor, 1920, 60 * pu::ui::render::ScreenFactor, COLOR("#17090980"));
        this->pageInfoText = pu::ui::elm::TextBlock::New(10, 109, "");
        this->pageInfoText->SetFont("DefaultFont@30");
        this->pageInfoText->SetColor(COLOR(app::config::TopInfoTextColor));
        this->butText = pu::ui::elm::TextBlock::New(10, 678 * pu::ui::render::ScreenFactor, "");
        this->butText->SetFont("DefaultFont@30");
        this->butText->SetColor(COLOR(app::config::BottomInfoTextColor));
        this->menu = pu::ui::elm::Menu::New(0, 154, 1920, COLOR("#FFFFFF00"), COLOR("#00000033"), app::config::subMenuItemSize, (836 / app::config::subMenuItemSize));
        this->menu->SetScrollbarColor(COLOR("#17090980"));
        this->menu->SetItemAlphaIncrementSteps(1);
        this->menu->SetShadowBaseAlpha(0);
        this->infoImage = pu::ui::elm::Image::New(780, 292 * pu::ui::render::ScreenFactor, app::manager::LoadTexture("romfs:/images/icons/lan-connection-waiting.png"));
        this->Add(this->infoRect);
        this->Add(this->botRect);
        this->Add(this->butText);
        this->Add(this->pageInfoText);
        this->Add(this->menu);
        this->Add(this->infoImage);
    }

    void netInstPage::drawMenuItems(bool clearItems) {
        s32 menuIndex = this->menu->GetSelectedIndex();
        if (clearItems) this->selectedUrls = {};
        if (clearItems) this->alternativeNames = {};
        this->menu->ClearItems();
        this->menuIndices = {};

        for (long unsigned int i = 0; i < this->ourUrls.size(); i++) {
            auto& url = this->ourUrls[i];

            std::string formattedURL = nx::network::formatUrlString(url);
            std::string itm = app::util::shortenString(formattedURL, 56, true);
            auto ourEntry = pu::ui::elm::MenuItem::New(itm);
            ourEntry->SetColor(COLOR(app::config::FileTextColor));
            ourEntry->SetIcon(mainApp->checkboxBlank);
            for (long unsigned int j = 0; j < this->selectedUrls.size(); j++) {
                if (this->selectedUrls[j] == url) {
                    ourEntry->SetIcon(mainApp->checkboxTick);
                }
            }
            this->menu->AddItem(ourEntry);
            this->menu->SetSelectedIndex(menuIndex);
            this->menuIndices.push_back(i);
        }
    }

    void netInstPage::selectTitle(int selectedIndex, bool redraw) {
        long unsigned int urlIndex = 0;
        if (this->menuIndices.size() > 0) urlIndex = this->menuIndices[selectedIndex];

        if (this->menu->GetItems()[selectedIndex]->GetIconTexture() == mainApp->checkboxTick) {
            for (long unsigned int i = 0; i < this->selectedUrls.size(); i++) {
                if (this->selectedUrls[i] == this->ourUrls[urlIndex])
                {
                    this->selectedUrls.erase(this->selectedUrls.begin() + i);
                    break;
                }
            }
        } else this->selectedUrls.push_back(this->ourUrls[urlIndex]);
        if (redraw) this->drawMenuItems(false);
    }

    void netInstPage::startNetwork() {
        this->butText->SetText("inst.net.buttons"_lang);
        this->menu->SetVisible(false);
        this->menu->ClearItems();
        this->infoImage->SetVisible(true);
        mainApp->LoadLayout(mainApp->netinstPage);
        this->ourUrls = netInstStuff::OnSelected();
        if (!this->ourUrls.size()) {
            mainApp->LoadLayout(mainApp->mainPage);
            return;
        } else if (this->ourUrls[0] == "supplyUrl") {
            std::string keyboardResult = nx::misc::OpenSoftwareKeyboard("inst.net.url.hint"_lang, app::config::lastNetUrl, 500);
            if (keyboardResult.size() > 0) {
                if (nx::network::formatUrlString(keyboardResult) == "" || keyboardResult == "https://" || keyboardResult == "http://") {
                    mainApp->CreateShowDialog("inst.net.url.invalid"_lang, "", {"common.ok"_lang}, false);
                    this->startNetwork();
                    return;
                }
                app::config::lastNetUrl = keyboardResult;
                app::config::SaveSettings();
                sourceString = "inst.net.url.source_string"_lang;
                this->selectedUrls = {keyboardResult};
                this->startInstall(true);
                return;
            }
            this->startNetwork();
            return;
        } else {
            mainApp->CallForRender(); // If we re-render a few times during this process the main screen won't flicker
            sourceString = "inst.net.source_string"_lang;
            netConnected = true;
            this->pageInfoText->SetText("inst.net.top_info"_lang);
            this->butText->SetText("inst.net.buttons1"_lang);
            this->drawMenuItems(true);
            this->menu->SetSelectedIndex(0);
            mainApp->CallForRender();
            this->infoImage->SetVisible(false);
            this->menu->SetVisible(true);
        }
        return;
    }

    void netInstPage::startInstall(bool urlMode) {
        int dialogResult = -1;
        if (this->selectedUrls.size() == 1) {
            std::string ourUrlString;
            if (this->alternativeNames.size() > 0) ourUrlString = app::util::shortenString(this->alternativeNames[0], 32, true);
            else ourUrlString = app::util::shortenString(nx::network::formatUrlString(this->selectedUrls[0]), 32, true);
            dialogResult = mainApp->CreateShowDialog("inst.target.desc0"_lang + ourUrlString + "inst.target.desc1"_lang, "common.cancel_desc"_lang, {"inst.target.opt0"_lang, "inst.target.opt1"_lang}, false);
        } else dialogResult = mainApp->CreateShowDialog("inst.target.desc00"_lang + std::to_string(this->selectedUrls.size()) + "inst.target.desc01"_lang, "common.cancel_desc"_lang, {"inst.target.opt0"_lang, "inst.target.opt1"_lang}, false);
        if (dialogResult == -1 && !urlMode) return;
        else if (dialogResult == -1 && urlMode) {
            this->startNetwork();
            return;
        }
        netInstStuff::installTitleNet(this->selectedUrls, dialogResult, this->alternativeNames, sourceString);
        return;
    }

    void netInstPage::onInput(u64 Down, u64 Up, u64 Held, pu::ui::TouchPoint Pos) {
        if (Down & HidNpadButton_B) {
            if (this->menu->GetItems().size() > 0){
                if (this->selectedUrls.size() == 0) {
                    this->selectTitle(this->menu->GetSelectedIndex());
                }
                netInstStuff::pushExitCommand(app::util::getUrlHost(this->selectedUrls[0]));
            }
            netInstStuff::OnUnwound();
            mainApp->LoadLayout(mainApp->mainPage);
        }
        if (netConnected) {
            if ((Down & HidNpadButton_A) || (mainApp->GetTouchState().count == 0 && prev_touchcount == 1)) {
                prev_touchcount = 0;
                this->selectTitle(this->menu->GetSelectedIndex());
                if (this->menu->GetItems().size() == 1 && this->selectedUrls.size() == 1) {
                    this->startInstall(false);
                }
            }
            if ((Down & HidNpadButton_Y)) {
                if (this->selectedUrls.size() == this->menu->GetItems().size()) this->drawMenuItems(true);
                else {
                    for (long unsigned int i = 0; i < this->menu->GetItems().size(); i++) {
                        if (this->menu->GetItems()[i]->GetIconTexture() == mainApp->checkboxTick) continue;
                        else this->selectTitle(i, false);
                    }
                    this->drawMenuItems(false);
                }
            }

            if (Down & HidNpadButton_ZL)
                this->menu->SetSelectedIndex(std::max(0, this->menu->GetSelectedIndex() - 6));
            if (Down & HidNpadButton_ZR)
                this->menu->SetSelectedIndex(std::min((s32)this->menu->GetItems().size() - 1, this->menu->GetSelectedIndex() + 6));

            if (Down & HidNpadButton_Plus) {
                if (this->selectedUrls.size() == 0) {
                    this->selectTitle(this->menu->GetSelectedIndex());
                }
                this->startInstall(false);
            }
        }
        if (mainApp->GetTouchState().count == 1)
            prev_touchcount = 1;
    }
}
