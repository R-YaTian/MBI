#include "ui/MainApplication.hpp"
#include "ui/InstallerPage.hpp"
#include "util/config.hpp"

namespace app::ui
{
    InstallerPage::InstallerPage() : Layout::Layout()
    {
        this->installWarningText = pu::ui::elm::TextBlock::New(15, 156, "");
        this->installWarningText->SetFont("DefaultFont@30");
        this->installWarningText->SetColor(COLOR(app::config::InstallerInfoTextColor));
        this->installInfoText = pu::ui::elm::TextBlock::New(15, 568 * pu::ui::render::ScreenFactor, "");
        this->installInfoText->SetFont("DefaultFont@30");
        this->installInfoText->SetColor(COLOR(app::config::InstallerInfoTextColor));
        this->installBar = pu::ui::elm::ProgressBar::New(10, 600 * pu::ui::render::ScreenFactor, 850 * pu::ui::render::ScreenFactor, 40 * pu::ui::render::ScreenFactor, 100.0f);
        this->installBar->SetBackgroundColor(COLOR("#222222FF"));
        this->Add(this->installWarningText);
        this->Add(this->installInfoText);
        this->Add(this->installBar);
    }

    void InstallerPage::AppendInstallWarningText(std::string newText)
    {
        std::string previousText = this->installWarningText->GetText();
        if (previousText != "")
        {
            previousText += "\n";
        }
        this->installWarningText->SetText(previousText + newText);
    }

    void InstallerPage::SetInstallInfoText(std::string text)
    {
        this->installInfoText->SetText(text);
    }

    void InstallerPage::SetProgressBar(double percent)
    {
        if (percent == 0.0f)
        {
            this->installBar->SetVisible(true);
        }
        this->installBar->SetProgress(percent < 2.0f ? 2.0f : percent);
    }

    void InstallerPage::Prepare()
    {
        this->installWarningText->SetText("");
        this->installInfoText->SetText("");
        this->installBar->SetProgress(0);
        this->installBar->SetVisible(false);
    }
}
