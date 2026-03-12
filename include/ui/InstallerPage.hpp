#pragma once

#include "ui/BaseMenuPage.hpp"

namespace app::ui
{
    class InstallerPage : public BaseMenuPage
    {
        public:
            InstallerPage();
            PU_SMART_CTOR(InstallerPage)
            void SetInstallBarText(std::string text);
            void SetProgressBar(double percent);
            void SetFinished() { this->isFinished = true; }
            void AppendInstallInfoText(std::string newText);
            void Prepare();
            void onCancel() override;
        private:
            bool isFinished = false;
            pu::ui::elm::TextBlock::Ref installInfoText;
            pu::ui::elm::TextBlock::Ref installBarText;
            pu::ui::elm::ProgressBar::Ref installBar;
            void onInput(const u64 Down, const u64 Up, const u64 Held, const pu::ui::TouchPoint Pos);
    };
}
