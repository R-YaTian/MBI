#pragma once

#include <pu/Plutonium>

namespace app::ui
{
    class InstallerPage : public pu::ui::Layout
    {
        public:
            InstallerPage();
            PU_SMART_CTOR(InstallerPage)
            void SetInstllInfoText(std::string text);
            void SetProgressBar(double percent);
            void Prepare();
        private:
            pu::ui::elm::TextBlock::Ref installInfoText;
            pu::ui::elm::ProgressBar::Ref installBar;
    };
}
