#include "ui/usbInstPage.hpp"
#include "ui/MainApplication.hpp"
#include "util/util.hpp"
#include "util/config.hpp"
#include "util/i18n.hpp"
#include "usbInstall.hpp"
#include "nx/usb.hpp"
#include "manager.hpp"

namespace app::ui {
    extern MainApplication *mainApp;
    static s32 prev_touchcount = 0;

    usbInstPage::usbInstPage() : Layout::Layout()
    {
        this->infoRect = pu::ui::elm::Rectangle::New(0, 94, 1920, 60, COLOR("#17090980"));
        this->botRect = pu::ui::elm::Rectangle::New(0, 660 * pu::ui::render::ScreenFactor, 1920, 60 * pu::ui::render::ScreenFactor, COLOR("#17090980"));
        this->pageInfoText = pu::ui::elm::TextBlock::New(10, 103, "");
        this->pageInfoText->SetFont("DefaultFont@30");
        this->pageInfoText->SetColor(COLOR(app::config::TopInfoTextColor));
        this->botText = pu::ui::elm::TextBlock::New(10 * pu::ui::render::ScreenFactor, 678 * pu::ui::render::ScreenFactor, "");
        this->botText->SetFont("DefaultFont@30");
        this->botText->SetColor(COLOR(app::config::BottomInfoTextColor));
        this->menu = pu::ui::elm::Menu::New(0, 154, 1920, COLOR("#FFFFFF00"), COLOR("#00000033"), app::config::subMenuItemSize, (836 / app::config::subMenuItemSize));
        this->menu->SetScrollbarColor(COLOR("#17090980"));
        this->menu->SetShadowBaseAlpha(0);
        this->infoImage = pu::ui::elm::Image::New(780, 332 * pu::ui::render::ScreenFactor, LoadTexture("romfs:/images/icons/usb-connection-waiting.png"));
        this->Add(this->infoRect);
        this->Add(this->botRect);
        this->Add(this->botText);
        this->Add(this->pageInfoText);
        this->Add(this->menu);
        this->Add(this->infoImage);
    }

    void usbInstPage::drawMenuItems(bool clearItems) {
        s32 menuIndex = this->menu->GetSelectedIndex();
        if (clearItems) this->selectedTitles = {};
        this->menu->ClearItems();
        for (auto& url: this->ourTitles) {
            std::string itm = app::util::shortenString(url, 56, true);
            auto ourEntry = pu::ui::elm::MenuItem::New(itm);
            ourEntry->SetColor(COLOR(app::config::FileTextColor));
            ourEntry->SetIcon(mainApp->checkboxBlank);
            for (long unsigned int i = 0; i < this->selectedTitles.size(); i++) {
                if (this->selectedTitles[i] == url) {
                    ourEntry->SetIcon(mainApp->checkboxTick);
                }
            }
            this->menu->AddItem(ourEntry);
            this->menu->SetSelectedIndex(menuIndex);
        }
    }

    void usbInstPage::selectTitle(int selectedIndex, bool redraw) {
        if (this->menu->GetItems()[selectedIndex]->GetIconTexture() == mainApp->checkboxTick) {
            for (long unsigned int i = 0; i < this->selectedTitles.size(); i++) {
                if (this->selectedTitles[i] == this->ourTitles[selectedIndex])
                {
                    this->selectedTitles.erase(this->selectedTitles.begin() + i);
                    break;
                }
            }
        } else this->selectedTitles.push_back(this->ourTitles[selectedIndex]);
        if (redraw) this->drawMenuItems(false);
    }

    void usbInstPage::startUsb() {
        this->pageInfoText->SetText("inst.usb.top_info"_lang);
        this->botText->SetText("inst.usb.buttons"_lang);
        this->menu->SetVisible(false);
        this->menu->ClearItems();
        this->infoImage->SetVisible(true);
        mainApp->LoadLayout(mainApp->usbinstPage);
        mainApp->CallForRender();
        this->ourTitles = usbInstStuff::OnSelected();
        if (!this->ourTitles.size()) {
            mainApp->LoadLayout(mainApp->mainPage);
            return;
        } else {
            mainApp->CallForRender(); // If we re-render a few times during this process the main screen won't flicker
            this->pageInfoText->SetText("inst.usb.top_info2"_lang);
            this->botText->SetText("inst.usb.buttons2"_lang);
            this->drawMenuItems(true);
            this->menu->SetSelectedIndex(0);
            mainApp->CallForRender();
            this->infoImage->SetVisible(false);
            this->menu->SetVisible(true);
        }
        return;
    }

    void usbInstPage::startInstall() {
        int dialogResult = -1;
        if (this->selectedTitles.size() == 1) dialogResult = mainApp->CreateShowDialog("inst.target.desc0"_lang + app::util::shortenString(this->selectedTitles[0], 32, true) + "inst.target.desc1"_lang, "common.cancel_desc"_lang, {"inst.target.opt0"_lang, "inst.target.opt1"_lang}, false);
        else dialogResult = mainApp->CreateShowDialog("inst.target.desc00"_lang + std::to_string(this->selectedTitles.size()) + "inst.target.desc01"_lang, "common.cancel_desc"_lang, {"inst.target.opt0"_lang, "inst.target.opt1"_lang}, false);
        if (dialogResult == -1) return;
        usbInstStuff::installTitleUsb(this->selectedTitles, dialogResult);
        return;
    }

    void usbInstPage::onInput(u64 Down, u64 Up, u64 Held, pu::ui::TouchPoint Pos) {
        if (Down & HidNpadButton_B) {
            nx::usb::USBCommandManager::SendExitCommand();
            mainApp->LoadLayout(mainApp->mainPage);
            nx::usb::usbDeviceReset();
        }
        if ((Down & HidNpadButton_A) || (mainApp->GetTouchState().count == 0 && prev_touchcount == 1)) {
            prev_touchcount = 0;
            this->selectTitle(this->menu->GetSelectedIndex());
            if (this->menu->GetItems().size() == 1 && this->selectedTitles.size() == 1) {
                this->startInstall();
            }
        }
        if ((Down & HidNpadButton_Y)) {
            if (this->selectedTitles.size() == this->menu->GetItems().size()) this->drawMenuItems(true);
            else {
                for (long unsigned int i = 0; i < this->menu->GetItems().size(); i++) {
                    if (this->menu->GetItems()[i]->GetIconTexture() == mainApp->checkboxTick) continue;
                    else this->selectTitle(i, false);
                }
                this->drawMenuItems(false);
            }
        }
        if (Down & HidNpadButton_Plus) {
            if (this->selectedTitles.size() == 0) {
                this->selectTitle(this->menu->GetSelectedIndex());
                this->startInstall();
                return;
            }
            this->startInstall();
        }
        if (mainApp->GetTouchState().count == 1)
            prev_touchcount = 1;
    }
}
