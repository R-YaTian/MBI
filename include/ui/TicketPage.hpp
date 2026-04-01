#pragma once

#include "ui/BaseMenuPage.hpp"
#include <memory>

namespace app::ui
{
    class TicketPage : public BaseMenuPage
    {
        public:
            TicketPage();
            ~TicketPage();
            PU_SMART_CTOR(TicketPage)
            bool LoadTickets();
            void onCancel() override;
            void onConfirm() override;
            void onSelectAll() override;
        private:
            struct InternalData;
            std::unique_ptr<InternalData> pageData;
            pu::ui::elm::Menu::Ref menu;
            pu::ui::elm::Menu::Ref GetMenu() override { return this->menu; }
            pu::ui::elm::Image::Ref infoImage;
            void drawMenuItems(bool clearItems);
            void selectTicket(int selectedIndex, bool redraw = true);
            void onInput(const u64 Down, const u64 Up, const u64 Held, const pu::ui::TouchPoint Pos);
    };
}
