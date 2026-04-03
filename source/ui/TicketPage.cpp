#include "ui/TicketPage.hpp"
#include "ui/MainApplication.hpp"
#include "util/config.hpp"
#include "util/i18n.hpp"
#include "nx/misc.hpp"
#include "facade.hpp"

namespace app::ui
{
    struct TicketPage::InternalData
    {
        std::vector<nx::misc::Ticket> ticketsList;
        std::map<size_t, nx::misc::Ticket> selectedTickets;
    };

    TicketPage::~TicketPage() = default;

    TicketPage::TicketPage() : BaseMenuPage()
    {
        this->SetOnInput(std::bind(&TicketPage::onInput, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
        this->menu = pu::ui::elm::Menu::New(0, 154, 1920, COLOR("#FFFFFF00"), COLOR("#00000033"), app::config::subMenuItemSize, (836 / app::config::subMenuItemSize));
        this->menu->SetScrollbarColor(COLOR("#17090980"));
        this->menu->SetShadowBaseAlpha(0);
        this->infoImage = pu::ui::elm::Image::New(780, 332 * pu::ui::render::ScreenFactor, LoadTexture("romfs:/images/icons/ticket-waiting.png"));
        this->Add(this->menu);
        this->Add(this->infoImage);
        pageData = std::make_unique<InternalData>();
    }

    void TicketPage::drawMenuItems()
    {
        for (auto& itm: this->pageData->ticketsList)
        {
            auto ourEntry = pu::ui::elm::MenuItem::New(itm.ToString());
            ourEntry->SetColor(COLOR(app::config::FileTextColor));
            ourEntry->SetIcon(GetResource(Resources::UncheckedImage));
            this->menu->AddItem(ourEntry);
        }
        this->menu->SetSelectedIndex(0);
    }

    void TicketPage::selectTicket(int selectedIndex)
    {
        if (this->menu->GetItems()[selectedIndex]->GetIconTexture() == GetResource(Resources::CheckedImage))
        {
            this->menu->GetItems()[selectedIndex]->SetIcon(GetResource(Resources::UncheckedImage));
            this->pageData->selectedTickets.erase(selectedIndex);
        }
        else
        {
            this->menu->GetItems()[selectedIndex]->SetIcon(GetResource(Resources::CheckedImage));
            this->pageData->selectedTickets[selectedIndex] = this->pageData->ticketsList[selectedIndex];
        }
    }

    bool TicketPage::LoadTickets()
    {
        this->menu->SetVisible(false);
        this->infoImage->SetVisible(true);
        app::facade::SendRenderRequest();
        this->pageData->ticketsList = nx::misc::ScanTickets();
        if (!this->pageData->ticketsList.size())
        {
            return false;
        }
        else
        {
            app::facade::SendPageInfoText("ticket_manager.top_info"_lang);
            app::facade::SendBottomText("ticket_manager.buttons"_lang);
            this->drawMenuItems();
            this->infoImage->SetVisible(false);
            this->menu->SetVisible(true);
        }
        return true;
    }

    void TicketPage::onCancel()
    {
        this->pageData->ticketsList.clear();
        this->pageData->selectedTickets.clear();
        this->menu->ClearItems();
        SceneJump(Scene::Main);
    }

    void TicketPage::onConfirm()
    {
        if (this->menu->GetItems().size() > 0)
        {
            if (this->pageData->selectedTickets.size() == 0)
            {
                this->selectTicket(this->menu->GetSelectedIndex());
            }
            // Todo: Implement ticket deletion
        }
    }

    void TicketPage::onSelectAll()
    {
        if (this->pageData->selectedTickets.size() == this->menu->GetItems().size())
        {
            for (size_t i = 0; i < this->menu->GetItems().size(); i++)
            {
                this->selectTicket(i);
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
                    this->selectTicket(i);
                }
            }
        }
    }

    void TicketPage::onInput(const u64 Down, const u64 Up, const u64 Held, const pu::ui::TouchPoint Pos)
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
            this->selectTicket(this->menu->GetSelectedIndex());
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
