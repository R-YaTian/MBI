#include "ui/MainApplication.hpp"
#include "ui/NetInstallPage.hpp"
#include "util/config.hpp"
#include "util/i18n.hpp"
#include "nx/network.hpp"
#include "nx/misc.hpp"
#include "installer.hpp"
#include "facade.hpp"

namespace app::ui
{
    NetInstallPage::NetInstallPage() : BaseMenuPage()
    {
        this->SetOnInput(std::bind(&NetInstallPage::onInput, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
        this->menu = pu::ui::elm::Menu::New(0, 154, 1920, COLOR("#FFFFFF00"), COLOR("#00000033"), app::config::subMenuItemSize, (836 / app::config::subMenuItemSize));
        this->menu->SetScrollbarColor(COLOR("#17090980"));
        this->menu->SetShadowBaseAlpha(0);
        this->infoImage = pu::ui::elm::Image::New(780, 292 * pu::ui::render::ScreenFactor, LoadTexture("romfs:/images/icons/lan-connection-waiting.webp"));
        this->Add(this->menu);
        this->Add(this->infoImage);
    }

    void NetInstallPage::drawMenuItems()
    {
        for (size_t i = 0; i < this->ourUrls.size(); i++)
        {
            const std::string url = this->ourUrls[i];
            std::string formattedURL = nx::network::FormatUrlString(url);
            auto ourEntry = pu::ui::elm::MenuItem::New(formattedURL);
            ourEntry->SetColor(COLOR(app::config::FileTextColor));
            ourEntry->SetIcon(GetResource(Resources::UncheckedImage));
            ourEntry->SetPreserveTailLength(4);
            ourEntry->SetTruncationMarker("(...)");
            this->menu->AddItem(ourEntry);
        }
        this->menu->SetSelectedIndex(0);
    }

    void NetInstallPage::selectTitle(int selectedIndex)
    {
        if (this->menu->GetItems()[selectedIndex]->GetIconTexture() == GetResource(Resources::CheckedImage))
        {
            this->menu->GetItems()[selectedIndex]->SetIcon(GetResource(Resources::UncheckedImage));
            this->selectedUrls.erase(selectedIndex);
        }
        else
        {
            this->menu->GetItems()[selectedIndex]->SetIcon(GetResource(Resources::CheckedImage));
            this->selectedUrls[selectedIndex] = this->ourUrls[selectedIndex];
        }
    }

    bool NetInstallPage::startNetwork()
    {
        this->menu->SetVisible(false);
        this->infoImage->SetVisible(true);
        this->ourUrls = app::installer::Network::WaitingForNetworkData();
        if (!this->ourUrls.size())
        {
            onCancel();
            return false;
        }
        else if (this->ourUrls[0] == "supplyUrl")
        {
            std::string keyboardResult = nx::misc::OpenSoftwareKeyboard("inst.net.url.hint"_lang, app::config::lastNetUrl, 500);
            if (keyboardResult.size() > 0)
            {
                if (nx::network::FormatUrlString(keyboardResult) == "" || keyboardResult == "https://" || keyboardResult == "http://")
                {
                    app::facade::ShowDialog("common.warning"_lang, "inst.net.url.invalid"_lang, {"common.ok"_lang}, false, "warning");
                    return startNetwork();
                }
                app::config::lastNetUrl = keyboardResult;
                this->selectedUrls[0] = keyboardResult;
                this->startInstall(true);
                return false;
            }
            return startNetwork();
        }
        else
        {
            app::facade::SendPageInfoText("inst.net.top_info"_lang);
            app::facade::SendBottomText("inst.net.buttons1"_lang);
            this->drawMenuItems();
            this->infoImage->SetVisible(false);
            this->menu->SetVisible(true);
        }
        return true;
    }

    void NetInstallPage::startInstall(bool urlMode)
    {
        int dialogResult = -1;
        dialogResult = app::facade::ShowDialog("inst.target.desc00"_lang + std::to_string(this->selectedUrls.size()) +
                                               "inst.target.desc01"_lang,
                                               "common.cancel_desc"_lang,
                                              {"inst.target.opt0"_lang, "inst.target.opt1"_lang, "common.cancel"_lang}, true,
                                               "install-disk");
        if (dialogResult < 0)
        {
            if (urlMode)
            {
                onCancel();
            }
            return;
        }
        std::vector<std::string> urlList;
        for (const auto& pair : this->selectedUrls)
        {
            urlList.push_back(pair.second);
        }
        std::string sourceString = urlMode ? "inst.net.url.source_string"_lang : "inst.net.source_string"_lang;
        this->selectedUrls.clear();
        this->ourUrls.clear();
        this->menu->ClearItems();
        app::installer::Network::InstallFromUrl(urlList, dialogResult ? NcmStorageId_BuiltInUser : NcmStorageId_SdCard, sourceString);
    }

    void NetInstallPage::onCancel()
    {
        if (this->menu->GetItems().size() > 0)
        {
            if (this->selectedUrls.size() == 0)
            {
                this->selectTitle(this->menu->GetSelectedIndex());
            }
            nx::network::PushExitCommand(this->selectedUrls.begin()->second);
        }
        nx::network::Finalize();
        this->selectedUrls.clear();
        this->ourUrls.clear();
        this->menu->ClearItems();
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

    void NetInstallPage::onSelectAll()
    {
        if (this->selectedUrls.size() == this->menu->GetItems().size())
        {
            for (size_t i = 0; i < this->menu->GetItems().size(); i++)
            {
                this->selectTitle(i);
            }
        }
        else
        {
            for (size_t i = 0; i < this->menu->GetItems().size(); i++)
            {
                if (this->menu->GetItems()[i]->GetIconTexture() == GetResource(Resources::CheckedImage))
                {
                    continue;
                }
                else
                {
                    this->selectTitle(i);
                }
            }
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
                onSelectAll();
            }

            if (Down & HidNpadButton_ZL)
            {
                onPageUp();
            }

            if (Down & HidNpadButton_ZR)
            {
                onPageDown();
            }

            if (Down & HidNpadButton_Plus)
            {
                onConfirm();
            }

            UpdateTouchState(Pos, 0, 154, 1920, std::min(this->menu->GetItems().size() * app::config::subMenuItemSize, (size_t)836));
        }
    }
}
