#include "ui/MainApplication.hpp"
#include "ui/InstallerPage.hpp"
#include "util/config.hpp"

namespace app::ui
{
    InstallerPage::InstallerPage() : BaseMenuPage()
    {
        this->installInfoText = pu::ui::elm::TextBlock::New(15, 160, "");
        this->installInfoText->SetFont("DefaultFont@30");
        this->installInfoText->SetColor(COLOR(app::config::InstallerInfoTextColor));
        this->installBarText = pu::ui::elm::TextBlock::New(1300, 926, "");
        this->installBarText->SetFont("DefaultFont@30");
        this->installBarText->SetColor(COLOR(app::config::InstallerInfoTextColor));
        this->installBar = pu::ui::elm::ProgressBar::New(10, 914, 1275, 60, 100.0f);
        this->installBar->SetRadius(30);
        this->installBar->SetBackgroundColor(COLOR("#222222FF"));
        this->Add(this->installBarText);
        this->Add(this->installInfoText);
        this->Add(this->installBar);
        this->SetOnInput(std::bind(&InstallerPage::onInput, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
    }

    void InstallerPage::AppendInstallInfoText(std::string newText)
    {
        std::string previousText = this->installInfoText->GetText();
        if (this->installInfoText->GetHeight() >= 720 || newText == "")
        {
            previousText = "";
        }
        if (previousText != "")
        {
            previousText += "\n";
        }
        this->installInfoText->SetText(previousText + newText);
    }

    void InstallerPage::SetInstallBarText(std::string text)
    {
        this->installBarText->SetText(text);
    }

    void InstallerPage::SetProgressBar(double percent)
    {
        if (percent == 0.0f)
        {
            this->installBar->SetVisible(true);
        }
        this->installBar->SetProgress(percent);
    }

    void InstallerPage::Prepare()
    {
        this->isFinished = false;
        this->installBarText->SetText("");
        this->installInfoText->SetText("");
        this->installBar->SetProgress(0);
        this->installBar->SetVisible(false);
    }

    void InstallerPage::onCancel()
    {
        SceneJump(Scene::Main);
    }

    void InstallerPage::onInput(const u64 Down, const u64 Up, const u64 Held, const pu::ui::TouchPoint Pos)
    {
        if (!isFinished)
        {
            return;
        }

        if (Down & HidNpadButton_B)
        {
            onCancel();
        }
    }
}
