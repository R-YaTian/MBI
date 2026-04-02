#include "ui/UsbInstallPage.hpp"
#include "ui/MainApplication.hpp"
#include "util/config.hpp"
#include "util/i18n.hpp"
#include "nx/usb.hpp"
#include "installer.hpp"
#include "facade.hpp"

namespace app::ui
{
    UsbInstallPage::UsbInstallPage() : BaseMenuPage()
    {
        this->SetOnInput(std::bind(&UsbInstallPage::onInput, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
        this->menu = pu::ui::elm::Menu::New(0, 154, 1920, COLOR("#FFFFFF00"), COLOR("#00000033"), app::config::subMenuItemSize, (836 / app::config::subMenuItemSize));
        this->menu->SetScrollbarColor(COLOR("#17090980"));
        this->menu->SetShadowBaseAlpha(0);
        this->infoImage = pu::ui::elm::Image::New(780, 332 * pu::ui::render::ScreenFactor, LoadTexture("romfs:/images/icons/usb-connection-waiting.png"));
        this->Add(this->menu);
        this->Add(this->infoImage);
    }

    void UsbInstallPage::drawMenuItems()
    {
        this->selectedTitles = {};
        for (auto& itm: this->ourTitles)
        {
            auto ourEntry = pu::ui::elm::MenuItem::New(itm);
            ourEntry->SetColor(COLOR(app::config::FileTextColor));
            ourEntry->SetIcon(GetResource(Resources::UncheckedImage));
            ourEntry->SetPreserveTailLength(4);
            ourEntry->SetTruncationMarker("(...)");
            this->menu->AddItem(ourEntry);
        }
        this->menu->SetSelectedIndex(0);
    }

    void UsbInstallPage::selectTitle(int selectedIndex)
    {
        if (this->menu->GetItems()[selectedIndex]->GetIconTexture() == GetResource(Resources::CheckedImage))
        {
            this->menu->GetItems()[selectedIndex]->SetIcon(GetResource(Resources::UncheckedImage));
            this->selectedTitles.erase(selectedIndex);
        }
        else
        {
            this->menu->GetItems()[selectedIndex]->SetIcon(GetResource(Resources::CheckedImage));
            this->selectedTitles[selectedIndex] = this->ourTitles[selectedIndex];
        }
    }

    bool UsbInstallPage::startUsb()
    {
        this->menu->SetVisible(false);
        this->menu->ClearItems();
        this->infoImage->SetVisible(true);
        app::facade::SendRenderRequest();
        this->ourTitles = app::installer::Usb::WaitingForFileList();
        if (!this->ourTitles.size())
        {
            return false;
        }
        else
        {
            app::facade::SendPageInfoText("inst.usb.top_info2"_lang);
            app::facade::SendBottomText("inst.usb.buttons2"_lang);
            this->drawMenuItems();
            this->infoImage->SetVisible(false);
            this->menu->SetVisible(true);
        }
        return true;
    }

    void UsbInstallPage::startInstall()
    {
        int dialogResult = -1;
        dialogResult = app::facade::ShowDialog("inst.target.desc00"_lang +
                                               std::to_string(this->selectedTitles.size()) +
                                               "inst.target.desc01"_lang,
                                               "common.cancel_desc"_lang,
                                              {"inst.target.opt0"_lang, "inst.target.opt1"_lang, "common.cancel"_lang}, true,
                                               static_cast<int>(Resources::InstallDiskImage));
        if (dialogResult < 0)
        {
            return;
        }
        std::vector<std::string> fileList;
        for (const auto& pair : this->selectedTitles)
        {
            fileList.push_back(pair.second);
        }
        app::installer::Usb::InstallTitles(fileList, dialogResult ? NcmStorageId_BuiltInUser : NcmStorageId_SdCard);
    }

    void UsbInstallPage::onCancel()
    {
        nx::usb::USBCommandManager::SendExitCommand();
        SceneJump(Scene::Main);
        nx::usb::usbDeviceReset();
    }

    void UsbInstallPage::onConfirm()
    {
        if (this->menu->GetItems().size() > 0)
        {
            if (this->selectedTitles.size() == 0)
            {
                this->selectTitle(this->menu->GetSelectedIndex());
            }
            this->startInstall();
        }
    }

    void UsbInstallPage::onSelectAll()
    {
        if (this->selectedTitles.size() == this->menu->GetItems().size())
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

    void UsbInstallPage::onInput(const u64 Down, const u64 Up, const u64 Held, const pu::ui::TouchPoint Pos)
    {
        if (Down & HidNpadButton_B)
        {
            onCancel();
        }

        if (this->menu->GetItems().size() == 0)
        {
            return;
        }

        if ((Down & HidNpadButton_A) || IsTouchUp())
        {
            this->selectTitle(this->menu->GetSelectedIndex());
            if (this->menu->GetItems().size() == 1 && this->selectedTitles.size() == 1)
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
