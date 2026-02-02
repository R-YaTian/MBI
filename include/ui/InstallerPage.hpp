#pragma once

#include <pu/Plutonium>

namespace app::ui
{
    class InstallerPage : public pu::ui::Layout
    {
        public:
            InstallerPage();
            PU_SMART_CTOR(InstallerPage)
            void SetInstallBarText(std::string text);
            void SetProgressBar(double percent);
            void AppendInstallInfoText(std::string newText);
            void Prepare();
        private:
            pu::ui::elm::TextBlock::Ref installInfoText;
            pu::ui::elm::TextBlock::Ref installBarText;
            pu::ui::elm::ProgressBar::Ref installBar;
    };
}
