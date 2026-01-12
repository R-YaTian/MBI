#include "ui/MainApplication.hpp"
#include "ui/MainPage.hpp"
#include "ui/sdInstPage.hpp"
#include "sdInstall.hpp"
#include "util/util.hpp"
#include "util/config.hpp"
#include "util/lang.hpp"

namespace app::ui {
    extern MainApplication *mainApp;
    static s32 prev_touchcount = 0;

    static std::vector <int> lastIndex;
    static int subPathCounter = 0;

    sdInstPage::sdInstPage() : Layout::Layout() {
        this->infoRect = pu::ui::elm::Rectangle::New(0, 94, 1920, 60, COLOR("#17090980"));
        this->botRect = pu::ui::elm::Rectangle::New(0, 660 * pu::ui::render::ScreenFactor, 1920, 60 * pu::ui::render::ScreenFactor, COLOR("#17090980"));
        this->pageInfoText = pu::ui::elm::TextBlock::New(10, 109, "inst.sd.top_info"_lang);
        this->pageInfoText->SetFont("DefaultFont@30");
        this->pageInfoText->SetColor(COLOR(app::config::themeColorTextTopInfo));
        this->butText = pu::ui::elm::TextBlock::New(10, 678 * pu::ui::render::ScreenFactor, "inst.sd.buttons"_lang);
        this->butText->SetFont("DefaultFont@30");
        this->butText->SetColor(COLOR(app::config::themeColorTextBottomInfo));
        this->menu = pu::ui::elm::Menu::New(0, 154, 1920, COLOR("#FFFFFF00"), COLOR("#00000033"), app::config::subMenuItemSize, (836 / app::config::subMenuItemSize));
        this->menu->SetScrollbarColor(COLOR("#17090980"));
        this->menu->SetItemAlphaIncrementSteps(1);
        this->menu->SetShadowBaseAlpha(0);
        this->Add(this->infoRect);
        this->Add(this->botRect);
        this->Add(this->butText);
        this->Add(this->pageInfoText);
        this->Add(this->menu);
    }

    void sdInstPage::drawMenuItems(bool clearItems, std::filesystem::path ourPath) {
        s32 menuIndex = this->menu->GetSelectedIndex();
        if (clearItems) this->selectedTitles = {};
        if (ourPath == "sdmc:") this->currentDir = std::filesystem::path(ourPath.string() + "/");
        else this->currentDir = ourPath;
        this->menu->ClearItems();
        this->menuIndices = {};

        try {
            this->ourDirectories = util::getDirsAtPath(this->currentDir);
            this->ourFiles = util::getDirectoryFiles(this->currentDir, {".nsp", ".nsz", ".xci", ".xcz"});
        } catch (std::exception& e) {
            this->drawMenuItems(false, this->currentDir.parent_path());
            return;
        }
        if (this->currentDir != "sdmc:/") {
            std::string itm = "..";
            auto ourEntry = pu::ui::elm::MenuItem::New(itm);
            ourEntry->SetColor(COLOR(app::config::themeColorTextDir));
            ourEntry->SetIcon(mainApp->dirbackImg);
            this->menu->AddItem(ourEntry);
        }
        for (auto& file: this->ourDirectories) {
            if (file == "..") break;
            std::string itm = file.filename().string();
            auto ourEntry = pu::ui::elm::MenuItem::New(itm);
            ourEntry->SetColor(COLOR(app::config::themeColorTextDir));
            ourEntry->SetIcon(mainApp->dirImg);
            this->menu->AddItem(ourEntry);
        }
        for (long unsigned int i = 0; i < this->ourFiles.size(); i++) {
            auto& file = this->ourFiles[i];

            std::string itm = file.filename().string();
            auto ourEntry = pu::ui::elm::MenuItem::New(itm);
            ourEntry->SetColor(COLOR(app::config::themeColorTextFile));
            ourEntry->SetIcon(mainApp->checkboxBlank);
            for (long unsigned int j = 0; j < this->selectedTitles.size(); j++) {
                if (this->selectedTitles[j] == file) {
                    ourEntry->SetIcon(mainApp->checkboxTick);
                }
            }
            this->menu->AddItem(ourEntry);
            this->menu->SetSelectedIndex(menuIndex);
            this->menuIndices.push_back(i);
        }
    }

    void sdInstPage::followDirectory() {
        int selectedIndex = this->menu->GetSelectedIndex();
        int dirListSize = this->ourDirectories.size();
        int selectNewIndex = 0;
        if (this->currentDir != "sdmc:/") {
            dirListSize++;
            selectedIndex--;
        }
        if (selectedIndex < dirListSize) {
            if (this->menu->GetItems()[this->menu->GetSelectedIndex()]->GetName() == ".." && this->menu->GetSelectedIndex() == 0) {
                this->drawMenuItems(true, this->currentDir.parent_path());
                if (subPathCounter > 0) {
                    subPathCounter--;
                    selectNewIndex = lastIndex[subPathCounter];
                    lastIndex.pop_back();
                }
            } else {
                this->drawMenuItems(true, this->ourDirectories[selectedIndex]);
                if (subPathCounter > 0) {
                    lastIndex.push_back(selectedIndex + 1);
                } else {
                    lastIndex.push_back(selectedIndex);
                }
                subPathCounter++;
            }
            this->menu->SetSelectedIndex(selectNewIndex);
        }
    }

    void sdInstPage::selectNsp(int selectedIndex, bool redraw) {
        int dirListSize = this->ourDirectories.size();
        if (this->currentDir != "sdmc:/") dirListSize++;

        long unsigned int nspIndex = 0;
        if (this->menuIndices.size() > 0) nspIndex = this->menuIndices[selectedIndex - dirListSize];

        if (this->menu->GetItems()[selectedIndex]->GetIconTexture() == mainApp->checkboxTick) {
            for (long unsigned int i = 0; i < this->selectedTitles.size(); i++) {
                if (this->selectedTitles[i] == this->ourFiles[nspIndex])
                {
                    this->selectedTitles.erase(this->selectedTitles.begin() + i);
                    break;
                }
            }
        } else if (this->menu->GetItems()[selectedIndex]->GetIconTexture() == mainApp->checkboxBlank) this->selectedTitles.push_back(this->ourFiles[nspIndex]);
        else {
            this->followDirectory();
            return;
        }
        if (redraw)
            this->drawMenuItems(false, currentDir);
    }

    void sdInstPage::startInstall() {
        int dialogResult = -1;
        if (this->selectedTitles.size() == 1) {
            dialogResult = mainApp->CreateShowDialog("inst.target.desc0"_lang + app::util::shortenString(std::filesystem::path(this->selectedTitles[0]).filename().string(), 32, true) + "inst.target.desc1"_lang, "common.cancel_desc"_lang, {"inst.target.opt0"_lang, "inst.target.opt1"_lang}, false);
        } else dialogResult = mainApp->CreateShowDialog("inst.target.desc00"_lang + std::to_string(this->selectedTitles.size()) + "inst.target.desc01"_lang, "common.cancel_desc"_lang, {"inst.target.opt0"_lang, "inst.target.opt1"_lang}, false);
        if (dialogResult == -1) return;
        nspInstStuff::installNspFromFile(this->selectedTitles, dialogResult);
        subPathCounter = 0;
        lastIndex.clear();
    }

    void sdInstPage::onInput(u64 Down, u64 Up, u64 Held, pu::ui::TouchPoint Pos) {
        if (Down & HidNpadButton_B) {
            if (subPathCounter > 0) {
                this->menu->SetSelectedIndex(0);
                this->followDirectory();
            } else {
                mainApp->LoadLayout(mainApp->mainPage);
            }
        }
        if ((Down & HidNpadButton_A) || (mainApp->GetTouchState().count == 0 && prev_touchcount == 1)) {
            prev_touchcount = 0;
            this->selectNsp(this->menu->GetSelectedIndex());
            if (this->ourFiles.size() == 1 && this->selectedTitles.size() == 1) {
                this->startInstall();
            }
        }
        if ((Down & HidNpadButton_Y)) {
            if (this->selectedTitles.size() == this->ourFiles.size()) this->drawMenuItems(true, currentDir);
            else {
                int topDir = 0;
                if (this->currentDir != "sdmc:/") topDir++;
                for (long unsigned int i = this->ourDirectories.size() + topDir; i < this->menu->GetItems().size(); i++) {
                    if (this->menu->GetItems()[i]->GetIconTexture() == mainApp->checkboxTick) continue;
                    else this->selectNsp(i, false);
                }
                this->drawMenuItems(false, currentDir);
            }
        }

        if ((Down & HidNpadButton_Minus)) {
            app::ui::mainApp->CreateShowDialog("inst.sd.help.title"_lang, "inst.sd.help.desc"_lang, {"common.ok"_lang}, true);
        }

        if (Down & HidNpadButton_ZL)
            this->menu->SetSelectedIndex(std::max(0, this->menu->GetSelectedIndex() - 6));
        if (Down & HidNpadButton_ZR)
            this->menu->SetSelectedIndex(std::min((s32)this->menu->GetItems().size() - 1, this->menu->GetSelectedIndex() + 6));

        if (Down & HidNpadButton_Plus) {
            if (this->selectedTitles.size() == 0 && this->menu->GetItems()[this->menu->GetSelectedIndex()]->GetIconTexture() == mainApp->checkboxBlank) {
                this->selectNsp(this->menu->GetSelectedIndex());
            }
            if (this->selectedTitles.size() > 0) this->startInstall();
        }
        if (mainApp->GetTouchState().count == 1)
            prev_touchcount = 1;
    }
}
