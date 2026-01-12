#include <filesystem>
#include <switch.h>
#include "ui/MainApplication.hpp"
#include "ui/MainPage.hpp"
#include "ui/instPage.hpp"
#include "ui/optionsPage.hpp"
#include "util/config.hpp"
#include "util/lang.hpp"
#include "ui/instPage.hpp"

#define COLOR(hex) pu::ui::Color::FromHex(hex)

namespace app::ui
{
    extern MainApplication *mainApp;
    std::vector<std::string> languageStrings = {"English", "日本語", "Français", "Deutsch", "Italiano", "Español", "한국전통", "Português", "Русский", "简体中文","正體中文"};
    static s32 prev_touchcount = 0;

    OptionsPage::OptionsPage() : Layout::Layout()
    {
        this->infoRect = pu::ui::elm::Rectangle::New(0, 94, 1920, 60, COLOR("#17090980"));
        this->botRect = pu::ui::elm::Rectangle::New(0, 660 * pu::ui::render::ScreenFactor, 1920, 60 * pu::ui::render::ScreenFactor, COLOR("#17090980"));
        this->pageInfoText = pu::ui::elm::TextBlock::New(10, 109, "options.title"_lang);
        this->pageInfoText->SetFont("DefaultFont@30");
        this->pageInfoText->SetColor(COLOR(app::config::themeColorTextTopInfo));
        this->butText = pu::ui::elm::TextBlock::New(10, 678 * pu::ui::render::ScreenFactor, "options.buttons"_lang);
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
        this->setMenuText();
        this->Add(this->menu);
    }

    pu::sdl2::TextureHandle::Ref OptionsPage::getMenuOptionIcon(bool ourBool)
    {
        if(ourBool)
            return mainApp->checkboxTick;
        else
            return mainApp->checkboxBlank;
    }

    std::string OptionsPage::getMenuLanguage(int ourLangCode)
    {
        switch (ourLangCode)
        {
            case 1:
            case 12:
                return languageStrings[0];
            case 0:
                return languageStrings[1];
            case 2:
            case 13:
                return languageStrings[2];
            case 3:
                return languageStrings[3];
            case 4:
                return languageStrings[4];
            case 5:
            case 14:
                return languageStrings[5];
            case 7:
                return languageStrings[6];
            case 9:
                return languageStrings[7];
            case 10:
                return languageStrings[8];
            case 6:
                return languageStrings[9];
            case 11:
                return languageStrings[10];
            default:
                return "options.language.system_language"_lang;
        }
    }

    void OptionsPage::setMenuText()
    {
        this->menu->ClearItems();
        auto ignoreFirmOption = pu::ui::elm::MenuItem::New("options.menu_items.ignore_firm"_lang);
        ignoreFirmOption->SetColor(COLOR(app::config::themeColorTextMenu));
        ignoreFirmOption->SetIcon(this->getMenuOptionIcon(app::config::ignoreReqVers));
        this->menu->AddItem(ignoreFirmOption);
        auto validateOption = pu::ui::elm::MenuItem::New("options.menu_items.nca_verify"_lang);
        validateOption->SetColor(COLOR(app::config::themeColorTextMenu));
        validateOption->SetIcon(this->getMenuOptionIcon(app::config::validateNCAs));
        this->menu->AddItem(validateOption);
        auto overclockOption = pu::ui::elm::MenuItem::New("options.menu_items.boost_mode"_lang);
        overclockOption->SetColor(COLOR(app::config::themeColorTextMenu));
        overclockOption->SetIcon(this->getMenuOptionIcon(app::config::overClock));
        this->menu->AddItem(overclockOption);
        auto deletePromptOption = pu::ui::elm::MenuItem::New("options.menu_items.ask_delete"_lang);
        deletePromptOption->SetColor(COLOR(app::config::themeColorTextMenu));
        deletePromptOption->SetIcon(this->getMenuOptionIcon(app::config::deletePrompt));
        this->menu->AddItem(deletePromptOption);
        auto enableSoundOption = pu::ui::elm::MenuItem::New("options.menu_items.enableSound"_lang);
        enableSoundOption->SetColor(COLOR(app::config::themeColorTextMenu));
        enableSoundOption->SetIcon(this->getMenuOptionIcon(app::config::enableSound));
        this->menu->AddItem(enableSoundOption);
        auto enableLightningOption = pu::ui::elm::MenuItem::New("options.menu_items.enableLightning"_lang);
        enableLightningOption->SetColor(COLOR(app::config::themeColorTextMenu));
        enableLightningOption->SetIcon(this->getMenuOptionIcon(app::config::enableLightning));
        this->menu->AddItem(enableLightningOption);
        auto fixTicketOption = pu::ui::elm::MenuItem::New("options.menu_items.fix_ticket"_lang);
        fixTicketOption->SetColor(COLOR(app::config::themeColorTextMenu));
        fixTicketOption->SetIcon(this->getMenuOptionIcon(app::config::fixTicket));
        this->menu->AddItem(fixTicketOption);
        auto languageOption = pu::ui::elm::MenuItem::New("options.menu_items.language"_lang + this->getMenuLanguage(app::config::languageSetting));
        languageOption->SetColor(COLOR(app::config::themeColorTextMenu));
        this->menu->AddItem(languageOption);
        auto creditsOption = pu::ui::elm::MenuItem::New("options.menu_items.credits"_lang);
        creditsOption->SetColor(COLOR(app::config::themeColorTextMenu));
        this->menu->AddItem(creditsOption);
    }

    void OptionsPage::onInput(u64 Down, u64 Up, u64 Held, pu::ui::TouchPoint Pos)
    {
        if (Down & HidNpadButton_B)
        {
            mainApp->LoadLayout(mainApp->mainPage);
        }

        if ((Down & HidNpadButton_A) || (mainApp->GetTouchState().count == 0 && prev_touchcount == 1))
        {
            prev_touchcount = 0;
            std::string keyboardResult;
            int rc;
            std::vector<std::string> languageList;
            switch (this->menu->GetSelectedIndex())
            {
                case 0:
                    app::config::ignoreReqVers = !app::config::ignoreReqVers;
                    app::config::setConfig();
                    this->setMenuText();
                    this->menu->SetSelectedIndex(0);
                    break;
                case 1:
                    if (app::config::validateNCAs) {
                        if (app::ui::mainApp->CreateShowDialog("options.nca_warn.title"_lang, "options.nca_warn.desc"_lang, {"common.cancel"_lang, "options.nca_warn.opt1"_lang}, false) == 1) app::config::validateNCAs = false;
                    } else app::config::validateNCAs = true;
                    app::config::setConfig();
                    this->setMenuText();
                    this->menu->SetSelectedIndex(1);
                    break;
                case 2:
                    app::config::overClock = !app::config::overClock;
                    app::config::setConfig();
                    this->setMenuText();
                    this->menu->SetSelectedIndex(2);
                    break;
                case 3:
                    app::config::deletePrompt = !app::config::deletePrompt;
                    app::config::setConfig();
                    this->setMenuText();
                    this->menu->SetSelectedIndex(3);
                    break;
                case 4:
                    app::config::enableSound = !app::config::enableSound;
                    app::config::setConfig();
                    this->setMenuText();
                    this->menu->SetSelectedIndex(4);
                    break;
                case 5:
                    app::config::enableLightning = !app::config::enableLightning;
                    app::config::setConfig();
                    this->setMenuText();
                    this->menu->SetSelectedIndex(5);
                    break;
                case 6:
                    app::config::fixTicket = !app::config::fixTicket;
                    app::config::setConfig();
                    this->setMenuText();
                    this->menu->SetSelectedIndex(6);
                    break;
                case 7:
                    for (int i = 0; i < 11; i++)
                    {
                        languageList.push_back(languageStrings[i]);
                    }
                    languageList.push_back("options.language.system_language"_lang);
                    rc = app::ui::mainApp->CreateShowDialog("options.language.title"_lang, "options.language.desc"_lang, languageList, false);
                    if (rc == -1) break;
                    switch(rc) {
                        case 0:
                            app::config::languageSetting = 1;
                            break;
                        case 1:
                            app::config::languageSetting = 0;
                            break;
                        case 2:
                            app::config::languageSetting = 2;
                            break;
                        case 3:
                            app::config::languageSetting = 3;
                            break;
                        case 4:
                            app::config::languageSetting = 4;
                            break;
                        case 5:
                            app::config::languageSetting = 14;
                            break;
                        case 6:
                            app::config::languageSetting = 7;
                            break;
                        case 7:
                            app::config::languageSetting = 9;
                            break;
                        case 8:
                            app::config::languageSetting = 10;
                            break;
                        case 9:
                            app::config::languageSetting = 6;
                            break;
                        case 10:
                            app::config::languageSetting = 11;
                            break;
                        default:
                            app::config::languageSetting = 99;
                    }
                    app::config::setConfig();
                    mainApp->FadeOut();
                    mainApp->Close();
                    break;
                case 8:
                    app::ui::mainApp->CreateShowDialog("options.credits.title"_lang, "options.credits.desc"_lang, {"common.close"_lang}, true);
                    break;
                default:
                    break;
            }
        }
        if (mainApp->GetTouchState().count == 1)
            prev_touchcount = 1;
    }
}
