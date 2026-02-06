#include "ui/MainApplication.hpp"
#include "ui/OptionsPage.hpp"
#include "util/config.hpp"
#include "util/i18n.hpp"
#include "facade.hpp"

namespace app::ui
{
    OptionsPage::OptionsPage() : Layout::Layout()
    {
        this->SetOnInput(std::bind(&OptionsPage::onInput, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
        languageStrings.push_back("options.language.system_language"_lang);
        languageStrings.push_back("common.cancel"_lang);
        this->menu = pu::ui::elm::Menu::New(0, 154, 1920, COLOR("#FFFFFF00"), COLOR("#00000033"), app::config::subMenuItemSize, (836 / app::config::subMenuItemSize));
        this->menu->SetScrollbarColor(COLOR("#17090980"));
        this->menu->SetShadowBaseAlpha(0);

        ignoreFirmOption = pu::ui::elm::MenuItem::New("options.menu_items.ignore_firm"_lang);
        ignoreFirmOption->SetColor(COLOR(app::config::MenuTextColor));
        ignoreFirmOption->AddOnKey(std::bind(&OptionsPage::IgnoreFirmOption_Click, this), HidNpadButton_A | HidNpadButton_Verification);
        overclockOption = pu::ui::elm::MenuItem::New("options.menu_items.boost_mode"_lang);
        overclockOption->SetColor(COLOR(app::config::MenuTextColor));
        overclockOption->AddOnKey(std::bind(&OptionsPage::OverclockOption_Click, this), HidNpadButton_A | HidNpadButton_Verification);
        deletePromptOption = pu::ui::elm::MenuItem::New("options.menu_items.ask_delete"_lang);
        deletePromptOption->SetColor(COLOR(app::config::MenuTextColor));
        deletePromptOption->AddOnKey(std::bind(&OptionsPage::DeletePromptOption_Click, this), HidNpadButton_A | HidNpadButton_Verification);
        enableSoundOption = pu::ui::elm::MenuItem::New("options.menu_items.enableSound"_lang);
        enableSoundOption->SetColor(COLOR(app::config::MenuTextColor));
        enableSoundOption->AddOnKey(std::bind(&OptionsPage::EnableSoundOption_Click, this), HidNpadButton_A | HidNpadButton_Verification);
        enableLightningOption = pu::ui::elm::MenuItem::New("options.menu_items.enableLightning"_lang);
        enableLightningOption->SetColor(COLOR(app::config::MenuTextColor));
        enableLightningOption->AddOnKey(std::bind(&OptionsPage::EnableLightningOption_Click, this), HidNpadButton_A | HidNpadButton_Verification);
        fixTicketOption = pu::ui::elm::MenuItem::New("options.menu_items.fix_ticket"_lang);
        fixTicketOption->SetColor(COLOR(app::config::MenuTextColor));
        fixTicketOption->AddOnKey(std::bind(&OptionsPage::FixTicketOption_Click, this), HidNpadButton_A | HidNpadButton_Verification);
        languageOption = pu::ui::elm::MenuItem::New("options.menu_items.language"_lang + this->getMenuLanguage(app::config::languageSetting));
        languageOption->SetColor(COLOR(app::config::MenuTextColor));
        languageOption->AddOnKey(std::bind(&OptionsPage::LanguageOption_Click, this), HidNpadButton_A | HidNpadButton_Verification);
        creditsOption = pu::ui::elm::MenuItem::New("options.menu_items.credits"_lang);
        creditsOption->SetColor(COLOR(app::config::MenuTextColor));
        creditsOption->AddOnKey(std::bind(&OptionsPage::CreditsOption_Click, this), HidNpadButton_A | HidNpadButton_Verification);

        this->menu->AddItem(ignoreFirmOption);
        this->menu->AddItem(overclockOption);
        this->menu->AddItem(deletePromptOption);
        this->menu->AddItem(enableSoundOption);
        this->menu->AddItem(enableLightningOption);
        this->menu->AddItem(fixTicketOption);
        this->menu->AddItem(languageOption);
        this->menu->AddItem(creditsOption);
        ignoreFirmOption->SetIcon(this->getMenuOptionIcon(app::config::ignoreReqVers));
        overclockOption->SetIcon(this->getMenuOptionIcon(app::config::overClock));
        deletePromptOption->SetIcon(this->getMenuOptionIcon(app::config::deletePrompt));
        enableSoundOption->SetIcon(this->getMenuOptionIcon(app::config::enableSound));
        enableLightningOption->SetIcon(this->getMenuOptionIcon(app::config::enableLightning));
        fixTicketOption->SetIcon(this->getMenuOptionIcon(app::config::fixTicket));
        this->Add(this->menu);
    }

    pu::sdl2::TextureHandle::Ref OptionsPage::getMenuOptionIcon(bool ourBool)
    {
        if (ourBool)
            return GetResource(Resources::CheckedImage);
        else
            return GetResource(Resources::UncheckedImage);
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
            case 17:
                return languageStrings[7];
            case 10:
                return languageStrings[8];
            case 6:
            case 15:
                return languageStrings[9];
            case 11:
            case 16:
                return languageStrings[10];
            default:
                return "options.language.system_language"_lang;
        }
    }

    void OptionsPage::IgnoreFirmOption_Click()
    {
        if (inputGuard)
        {
            return;
        }
        app::config::ignoreReqVers = !app::config::ignoreReqVers;
        ignoreFirmOption->SetIcon(this->getMenuOptionIcon(app::config::ignoreReqVers));
    }

    void OptionsPage::OverclockOption_Click()
    {
        if (inputGuard)
        {
            return;
        }
        app::config::overClock = !app::config::overClock;
        overclockOption->SetIcon(this->getMenuOptionIcon(app::config::overClock));
    }

    void OptionsPage::DeletePromptOption_Click()
    {
        if (inputGuard)
        {
            return;
        }
        app::config::deletePrompt = !app::config::deletePrompt;
        deletePromptOption->SetIcon(this->getMenuOptionIcon(app::config::deletePrompt));
    }

    void OptionsPage::EnableSoundOption_Click()
    {
        if (inputGuard)
        {
            return;
        }
        app::config::enableSound = !app::config::enableSound;
        enableSoundOption->SetIcon(this->getMenuOptionIcon(app::config::enableSound));
    }

    void OptionsPage::EnableLightningOption_Click()
    {
        if (inputGuard)
        {
            return;
        }
        app::config::enableLightning = !app::config::enableLightning;
        enableLightningOption->SetIcon(this->getMenuOptionIcon(app::config::enableLightning));
    }

    void OptionsPage::FixTicketOption_Click()
    {
        if (inputGuard)
        {
            return;
        }
        app::config::fixTicket = !app::config::fixTicket;
        fixTicketOption->SetIcon(this->getMenuOptionIcon(app::config::fixTicket));
    }

    void OptionsPage::CreditsOption_Click()
    {
        if (inputGuard)
        {
            return;
        }
        app::facade::ShowDialog("options.credits.title"_lang, "options.credits.desc"_lang, {"common.close"_lang}, true);
    }

    void OptionsPage::LanguageOption_Click()
    {
        if (inputGuard)
        {
            return;
        }
        s32 rc = app::facade::CreateDialogSimple("options.language.title"_lang, "options.language.desc"_lang, languageStrings, false);
        if (rc >= 0 && rc < (s32)(languageStrings.size() - 1))
        {
            switch (rc)
            {
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
                    app::config::languageSetting = -1;
                    break;
            }
            inputGuard = true;
            app::config::SaveSettings();
            CloseWithFadeOut();
        }
    }

    void OptionsPage::onReturn()
    {
        if (inputGuard)
        {
            return;
        }
        app::config::SaveSettings();
        SceneJump(Scene::Main);
    }

    void OptionsPage::onInput(const u64 Down, const u64 Up, const u64 Held, const pu::ui::TouchPoint Pos)
    {
        if (inputGuard)
        {
            return;
        }

        if (Down & HidNpadButton_B)
        {
            onReturn();
        }
    }
}
