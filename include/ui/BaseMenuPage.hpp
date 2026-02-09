#pragma once

#include <pu/Plutonium>

namespace app::ui
{
    class BaseMenuPage : public pu::ui::Layout
    {
        public:
            BaseMenuPage() : pu::ui::Layout() {}
            virtual ~BaseMenuPage() = default;

            virtual void onPageUp() 
            {
                auto menu = GetMenu();
                if (menu)
                {
                    menu->SetSelectedIndex(std::max(0, menu->GetSelectedIndex() - 11));
                }
            }

            virtual void onPageDown()
            {
                auto menu = GetMenu();
                if (menu)
                {
                    menu->SetSelectedIndex(std::min((s32)menu->GetItems().size() - 1, menu->GetSelectedIndex() + 11));
                }
            }

            virtual void onCancel() { return; };
            virtual void onConfirm() { return; };
            virtual void onSelectAll() { return; };
        private:
            virtual pu::ui::elm::Menu::Ref GetMenu() { return nullptr; }
    };
}
