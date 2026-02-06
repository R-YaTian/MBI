#include "ui/MainApplication.hpp"
#include "ui/NetInstallPage.hpp"
#include "util/util.hpp"
#include "util/config.hpp"
#include "util/i18n.hpp"
#include "nx/network.hpp"
#include "nx/misc.hpp"
#include "installer.hpp"
#include "facade.hpp"

namespace app::ui
{
    NetInstallPage::NetInstallPage() : Layout::Layout()
    {
        this->SetOnInput(std::bind(&NetInstallPage::onInput, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
        this->menu = pu::ui::elm::Menu::New(0, 154, 1920, COLOR("#FFFFFF00"), COLOR("#00000033"), app::config::subMenuItemSize, (836 / app::config::subMenuItemSize));
        this->menu->SetScrollbarColor(COLOR("#17090980"));
        this->menu->SetShadowBaseAlpha(0);
        this->infoImage = pu::ui::elm::Image::New(780, 292 * pu::ui::render::ScreenFactor, LoadTexture("romfs:/images/icons/lan-connection-waiting.png"));
        this->Add(this->menu);
        this->Add(this->infoImage);
    }

    void NetInstallPage::drawMenuItems(bool clearItems)
    {
        s32 menuIndex = this->menu->GetSelectedIndex();
        if (clearItems) this->selectedUrls = {};
        this->menu->ClearItems();
        this->menuIndices = {};

        for (long unsigned int i = 0; i < this->ourUrls.size(); i++)
        {
            auto& url = this->ourUrls[i];

            std::string formattedURL = nx::network::formatUrlString(url);
            std::string itm = app::util::shortenString(formattedURL, 56, true);
            auto ourEntry = pu::ui::elm::MenuItem::New(itm);
            ourEntry->SetColor(COLOR(app::config::FileTextColor));
            ourEntry->SetIcon(GetResource(app::ui::Resources::UncheckedImage));
            for (long unsigned int j = 0; j < this->selectedUrls.size(); j++)
            {
                if (this->selectedUrls[j] == url)
                {
                    ourEntry->SetIcon(GetResource(app::ui::Resources::CheckedImage));
                }
            }
            this->menu->AddItem(ourEntry);
            this->menu->SetSelectedIndex(menuIndex);
            this->menuIndices.push_back(i);
        }
    }

    void NetInstallPage::selectTitle(int selectedIndex, bool redraw)
    {
        long unsigned int urlIndex = 0;
        if (this->menuIndices.size() > 0)
        {
            urlIndex = this->menuIndices[selectedIndex];
        }

        if (this->menu->GetItems()[selectedIndex]->GetIconTexture() == GetResource(app::ui::Resources::CheckedImage))
        {
            for (long unsigned int i = 0; i < this->selectedUrls.size(); i++)
            {
                if (this->selectedUrls[i] == this->ourUrls[urlIndex])
                {
                    this->selectedUrls.erase(this->selectedUrls.begin() + i);
                    break;
                }
            }
        }
        else
        {
            this->selectedUrls.push_back(this->ourUrls[urlIndex]);
        }

        if (redraw)
        {
            this->drawMenuItems(false);
        }
    }

    bool NetInstallPage::startNetwork()
    {
        this->menu->SetVisible(false);
        this->menu->ClearItems();
        this->infoImage->SetVisible(true);
        this->ourUrls = app::installer::Network::WaitingForNetworkData();
        if (!this->ourUrls.size())
        {
            return false;
        }
        else if (this->ourUrls[0] == "supplyUrl")
        {
            std::string keyboardResult = nx::misc::OpenSoftwareKeyboard("inst.net.url.hint"_lang, app::config::lastNetUrl, 500);
            if (keyboardResult.size() > 0)
            {
                if (nx::network::formatUrlString(keyboardResult) == "" || keyboardResult == "https://" || keyboardResult == "http://")
                {
                    app::facade::ShowDialog("inst.net.url.warn"_lang, "inst.net.url.invalid"_lang, {"common.ok"_lang}, false);
                    return startNetwork();
                }
                app::config::lastNetUrl = keyboardResult;
                sourceString = "inst.net.url.source_string"_lang;
                this->selectedUrls = {keyboardResult};
                this->startInstall(true);
                return false;
            }
            return startNetwork();
        }
        else
        {
            sourceString = "inst.net.source_string"_lang;
            app::facade::SendPageInfoText("inst.net.top_info"_lang);
            app::facade::SendBottomText("inst.net.buttons1"_lang);
            this->drawMenuItems(true);
            this->menu->SetSelectedIndex(0);
            this->infoImage->SetVisible(false);
            this->menu->SetVisible(true);
        }
        return true;
    }

    void NetInstallPage::startInstall(bool urlMode)
    {
        int dialogResult = -1;
        if (this->selectedUrls.size() == 1)
        {
            std::string ourUrlString;
            ourUrlString = app::util::shortenString(nx::network::formatUrlString(this->selectedUrls[0]), 32, true);
            dialogResult = app::facade::ShowDialog("inst.target.desc0"_lang + ourUrlString + "inst.target.desc1"_lang,
                                                   "common.cancel_desc"_lang, {"inst.target.opt0"_lang,
                                                   "inst.target.opt1"_lang}, false);
        }
        else
        {
            dialogResult = app::facade::ShowDialog("inst.target.desc00"_lang + std::to_string(this->selectedUrls.size()) + "inst.target.desc01"_lang,
                                                   "common.cancel_desc"_lang, {"inst.target.opt0"_lang,
                                                   "inst.target.opt1"_lang}, false);
        }
        if (dialogResult == -1 && !urlMode)
        {
            return;
        }
        else if (dialogResult == -1 && urlMode)
        {
            onCancel();
            return;
        }
        app::installer::Network::InstallFromUrl(this->selectedUrls, dialogResult ? NcmStorageId_BuiltInUser : NcmStorageId_SdCard, sourceString);
    }

    void NetInstallPage::onCancel()
    {
        if (this->menu->GetItems().size() > 0)
        {
            if (this->selectedUrls.size() == 0)
            {
                this->selectTitle(this->menu->GetSelectedIndex());
            }
            app::installer::Network::PushExitCommand(app::util::getUrlHost(this->selectedUrls[0]));
        }
        app::installer::Network::Cleanup();
        SceneJump(Scene::Main);
    }

    void NetInstallPage::onConfirm()
    {
        if (this->menu->GetItems().size() > 0)
        {
            if (this->selectedUrls.size() == 0)
            {
                this->selectTitle(this->menu->GetSelectedIndex());
            }
            this->startInstall();
        }
    }

    void NetInstallPage::onInput(const u64 Down, const u64 Up, const u64 Held, const pu::ui::TouchPoint Pos)
    {
        if (Down & HidNpadButton_B)
        {
            onCancel();
        }

        if (this->menu->GetItems().size() > 0)
        {
            if ((Down & HidNpadButton_A) || IsTouchUp())
            {
                this->selectTitle(this->menu->GetSelectedIndex());
                if (this->menu->GetItems().size() == 1 && this->selectedUrls.size() == 1)
                {
                    this->startInstall();
                }
            }
            if ((Down & HidNpadButton_Y))
            {
                if (this->selectedUrls.size() == this->menu->GetItems().size())
                {
                    this->drawMenuItems(true);
                }
                else
                {
                    for (long unsigned int i = 0; i < this->menu->GetItems().size(); i++)
                    {
                        if (this->menu->GetItems()[i]->GetIconTexture() == GetResource(app::ui::Resources::CheckedImage))
                        {
                            continue;
                        }
                        else
                        {
                            this->selectTitle(i, false);
                        }
                    }
                    this->drawMenuItems(false);
                }
            }

            if (Down & HidNpadButton_ZL)
            {
                this->menu->SetSelectedIndex(std::max(0, this->menu->GetSelectedIndex() - 11));
            }
            if (Down & HidNpadButton_ZR)
            {
                this->menu->SetSelectedIndex(std::min((s32)this->menu->GetItems().size() - 1, this->menu->GetSelectedIndex() + 11));
            }

            if (Down & HidNpadButton_Plus)
            {
                onConfirm();
            }
            UpdateTouchState(Pos, 0, 154, 1920, this->menu->GetItems().size() * app::config::subMenuItemSize);
        }
    }
}
