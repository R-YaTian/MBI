#include "ui/UsbInstallPage.hpp"
#include "ui/MainApplication.hpp"
#include "util/config.hpp"
#include "util/i18n.hpp"
#include "util/util.hpp"
#include "installer.hpp"
#include "nx/usb.hpp"
#include "facade.hpp"
#include <malloc.h>

namespace app::ui
{
    UsbInstallPage::UsbInstallPage() : BaseMenuPage()
    {
        this->SetOnInput(std::bind(&UsbInstallPage::onInput, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
        this->menu = pu::ui::elm::Menu::New(0, 154_dp, 1920_dp, COLOR("#FFFFFF00"), COLOR("#5F5F5FFF"), config::GetSubMenuItemSize(), config::GetSubMenuHeight() / config::GetSubMenuItemSize());
        this->menu->SetScrollbarWidth(this->menu->GetScrollbarWidth() / config::GetScreenScaleFactor());
        this->menu->SetScrollbarMargin(this->menu->GetScrollbarMargin() / config::GetScreenScaleFactor());
        this->menu->SetIconMargin(20_dp);
        this->menu->SetTextMargin(20_dp);
        this->infoImage = pu::ui::elm::Image::New(780_dp, 498_dp, LoadTexture("romfs:/images/icons/usb-connection-waiting.webp"));
        this->infoImage->SetWidth(this->infoImage->GetWidth() / config::GetScreenScaleFactor());
        this->infoImage->SetHeight(this->infoImage->GetHeight() / config::GetScreenScaleFactor());
        this->Add(this->menu);
        this->Add(this->infoImage);
        this->AddRenderCallback(std::bind(&UsbInstallPage::waitingForFileList, this));
    }

    void UsbInstallPage::waitingForFileList()
    {
        if (!this->ourTitles.size())
        {
            char msg[256] = {};
            UsbState usbState = nx::usb::usbDeviceGetState();
            nx::usb::DeviceSpeed usbSpeed = nx::usb::usbDeviceGetSpeed();
            std::snprintf(msg, sizeof(msg), "usbds.message"_lang.c_str(),
                                            app::i18n::GetRelativeMsgAt("usbds.states", usbState).c_str(),
                                            app::i18n::GetRelativeMsgAt("usbds.speed", usbSpeed).c_str());
            app::facade::SendPageInfoText(msg);

            if (usbState != UsbState_Configured)
            {
                return;
            }

            u8* tempBuffer = (u8*)memalign(0x1000, sizeof(nx::usb::FileListHeader));
            if (!tempBuffer)
            {
                return;
            }

            nx::usb::FileListHeader header;
            if (nx::usb::USBReadData(tempBuffer, sizeof(nx::usb::FileListHeader), 500000000) == 0)
            {
                free(tempBuffer);
                return;
            }
            std::memcpy(&header, tempBuffer, sizeof(nx::usb::FileListHeader));
            free(tempBuffer);

            if (header.magic != 0x304C5554)
            {
                return;
            }

            char* titleNameBuffer = (char*)memalign(0x1000, header.titleListSize + 1);
            if (titleNameBuffer != nullptr)
            {
                std::vector<std::string> titleNames;
                memset(titleNameBuffer, 0, header.titleListSize + 1);
                size_t ret = nx::usb::USBReadData(titleNameBuffer, header.titleListSize, 10000000000);
                if (ret == 0)
                {
                    free(titleNameBuffer);
                    return;
                }

                // Split the string up into individual title names
                std::stringstream titleNameStream(titleNameBuffer);
                std::string segment;
                while (std::getline(titleNameStream, segment, '\n'))
                {
                    titleNames.push_back(segment);
                }
                free(titleNameBuffer);

                std::sort(titleNames.begin(), titleNames.end(), app::util::IgnoreCaseCompare);

                this->ourTitles = titleNames;

                facade::SendPageInfoText("inst.usb.source_string"_lang + "inst.top_info"_lang);
                facade::SendBottomText("inst.buttons"_lang);
                this->drawMenuItems();
                this->infoImage->SetVisible(false);
                this->menu->SetVisible(true);
                facade::ShowFullTouchButtonArea();
            }
        }
    }

    void UsbInstallPage::drawMenuItems()
    {
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

    void UsbInstallPage::startInstall()
    {
        int dialogResult = -1;
        dialogResult = app::facade::ShowDialog("inst.target.desc00"_lang +
                                               std::to_string(this->selectedTitles.size()) +
                                               "inst.target.desc01"_lang,
                                               "common.cancel_desc"_lang,
                                              {"inst.target.opt0"_lang, "inst.target.opt1"_lang, "common.cancel"_lang}, true,
                                               "install-disk");
        if (dialogResult < 0)
        {
            return;
        }
        std::vector<std::string> fileList;
        for (const auto& pair : this->selectedTitles)
        {
            fileList.push_back(pair.second);
        }
        this->ourTitles.clear();
        this->selectedTitles.clear();
        this->menu->ClearItems();
        app::installer::Usb::InstallTitles(fileList, dialogResult ? NcmStorageId_BuiltInUser : NcmStorageId_SdCard);
    }

    void UsbInstallPage::onCancel()
    {
        nx::usb::USBCommandManager::SendExitCommand();
        this->ourTitles.clear();
        this->selectedTitles.clear();
        this->menu->ClearItems();
        this->menu->SetVisible(false);
        this->infoImage->SetVisible(true);
        nx::usb::usbDeviceExit();
        SceneJump(Scene::Main);
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

        if ((Down & HidNpadButton_X) && !this->ourTitles.size())
        {
            facade::ShowDialog("common.help"_lang, "inst.usb.help_desc"_lang, {"common.ok"_lang}, true, "information");
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

        UpdateTouchState(Pos, 0, 154_dp, 1920_dp, std::min(this->menu->GetItems().size() * config::GetSubMenuItemSize(), (size_t)config::GetSubMenuHeight()));
    }
}
