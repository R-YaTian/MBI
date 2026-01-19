#include "ui/UsbInstallPage.hpp"
#include "ui/MainApplication.hpp"
#include "util/util.hpp"
#include "util/config.hpp"
#include "util/i18n.hpp"
#include "usbInstall.hpp"
#include "nx/usb.hpp"
#include "manager.hpp"
#include "facade.hpp"

namespace app::ui
{
    static s32 prev_touchcount = 0;

    UsbInstallPage::UsbInstallPage() : Layout::Layout()
    {
        this->menu = pu::ui::elm::Menu::New(0, 154, 1920, COLOR("#FFFFFF00"), COLOR("#00000033"), app::config::subMenuItemSize, (836 / app::config::subMenuItemSize));
        this->menu->SetScrollbarColor(COLOR("#17090980"));
        this->menu->SetShadowBaseAlpha(0);
        this->infoImage = pu::ui::elm::Image::New(780, 332 * pu::ui::render::ScreenFactor, LoadTexture("romfs:/images/icons/usb-connection-waiting.png"));
        this->Add(this->menu);
        this->Add(this->infoImage);
    }

    void UsbInstallPage::drawMenuItems(bool clearItems)
    {
        s32 menuIndex = this->menu->GetSelectedIndex();
        if (clearItems)
        {
            this->selectedTitles = {};
        }
        this->menu->ClearItems();
        for (auto& url: this->ourTitles)
        {
            std::string itm = app::util::shortenString(url, 56, true);
            auto ourEntry = pu::ui::elm::MenuItem::New(itm);
            ourEntry->SetColor(COLOR(app::config::FileTextColor));
            ourEntry->SetIcon(GetResource(Resources::UncheckedImage));
            for (long unsigned int i = 0; i < this->selectedTitles.size(); i++)
            {
                if (this->selectedTitles[i] == url)
                {
                    ourEntry->SetIcon(GetResource(Resources::CheckedImage));
                }
            }
            this->menu->AddItem(ourEntry);
            this->menu->SetSelectedIndex(menuIndex);
        }
    }

    void UsbInstallPage::selectTitle(int selectedIndex, bool redraw)
    {
        if (this->menu->GetItems()[selectedIndex]->GetIconTexture() == GetResource(Resources::CheckedImage))
        {
            for (long unsigned int i = 0; i < this->selectedTitles.size(); i++)
            {
                if (this->selectedTitles[i] == this->ourTitles[selectedIndex])
                {
                    this->selectedTitles.erase(this->selectedTitles.begin() + i);
                    break;
                }
            }
        }
        else 
        {
            this->selectedTitles.push_back(this->ourTitles[selectedIndex]);
        }
        if (redraw)
        {
            this->drawMenuItems(false);
        }
    }

    void UsbInstallPage::startUsb()
    {
        this->menu->SetVisible(false);
        this->menu->ClearItems();
        this->infoImage->SetVisible(true);
        this->ourTitles = usbInstStuff::OnSelected();
        if (!this->ourTitles.size())
        {
            SceneJump(Scene::Main);
            return;
        }
        else
        {
            app::facade::SendPageInfoText("inst.usb.top_info2"_lang);
            app::facade::SendBottomText("inst.usb.buttons2"_lang);
            this->drawMenuItems(true);
            this->menu->SetSelectedIndex(0);
            this->infoImage->SetVisible(false);
            this->menu->SetVisible(true);
        }
        return;
    }

    void UsbInstallPage::startInstall()
    {
        int dialogResult = -1;
        if (this->selectedTitles.size() == 1)
        {
            dialogResult = app::facade::ShowDialog("inst.target.desc0"_lang + app::util::shortenString(this->selectedTitles[0], 32, true) + "inst.target.desc1"_lang, "common.cancel_desc"_lang, {"inst.target.opt0"_lang, "inst.target.opt1"_lang}, false);
        }
        else
        {
            dialogResult = app::facade::ShowDialog("inst.target.desc00"_lang + std::to_string(this->selectedTitles.size()) + "inst.target.desc01"_lang, "common.cancel_desc"_lang, {"inst.target.opt0"_lang, "inst.target.opt1"_lang}, false);
        }
        if (dialogResult == -1)
        {
            return;
        }
        usbInstStuff::installTitleUsb(this->selectedTitles, dialogResult);
    }

    void UsbInstallPage::onInput(u64 Down, u64 Up, u64 Held, pu::ui::TouchPoint Pos)
    {
        if (Down & HidNpadButton_B)
        {
            nx::usb::USBCommandManager::SendExitCommand();
            SceneJump(Scene::Main);
            nx::usb::usbDeviceReset();
        }
        if ((Down & HidNpadButton_A) || (!IsTouched() && prev_touchcount == 1))
        {
            prev_touchcount = 0;
            this->selectTitle(this->menu->GetSelectedIndex());
            if (this->menu->GetItems().size() == 1 && this->selectedTitles.size() == 1)
            {
                this->startInstall();
            }
        }
        if ((Down & HidNpadButton_Y))
        {
            if (this->selectedTitles.size() == this->menu->GetItems().size())
            {
                this->drawMenuItems(true);
            }
            else
            {
                for (long unsigned int i = 0; i < this->menu->GetItems().size(); i++)
                {
                    if (this->menu->GetItems()[i]->GetIconTexture() == GetResource(Resources::CheckedImage))
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
        if (Down & HidNpadButton_Plus)
        {
            if (this->selectedTitles.size() == 0)
            {
                this->selectTitle(this->menu->GetSelectedIndex());
                this->startInstall();
                return;
            }
            this->startInstall();
        }
        if (IsTouched())
        {
            prev_touchcount = 1;
        }
    }
}
