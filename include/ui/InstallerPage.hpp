#pragma once

#include <pu/Plutonium>

namespace app::ui
{
    class InstallerPage : public pu::ui::Layout
    {
        public:
            InstallerPage();
            PU_SMART_CTOR(InstallerPage)
            void SetInstallInfoText(std::string text);
            void SetProgressBar(double percent);
            void AppendInstallWarningText(std::string newText);
            void Prepare();
        private:
            pu::ui::elm::TextBlock::Ref installInfoText;
            pu::ui::elm::TextBlock::Ref installWarningText;
            pu::ui::elm::ProgressBar::Ref installBar;
    };
}
