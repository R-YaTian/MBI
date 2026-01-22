#include "facade.hpp"
#include "ui/MainApplication.hpp"
#include "ui/InstallerPage.hpp"

namespace app::ui
{
    extern MainApplication *mainApp;
    extern InstallerPage::Ref installerPage;
}

namespace app::facade
{
    s32 ShowDialog(const std::string &title, const std::string &content, const std::vector<std::string> &opts, const bool use_last_opt_as_cancel)
    {
        return app::ui::mainApp->CreateShowDialog(title, content, opts, use_last_opt_as_cancel);
    }

    void SendBottomText(std::string text)
    {
        app::ui::mainApp->SetBottomText(text);
    }

    void SendPageInfoText(std::string text)
    {
        app::ui::mainApp->SetPageInfoText(text);
    }

    bool SendRenderRequest()
    {
        return app::ui::mainApp->CallForRender();
    }

    void SendPageInfoTextAndRender(std::string text)
    {
        SendPageInfoText(text);
        SendRenderRequest();
    }

    void SendInstallInfoText(std::string text)
    {
        app::ui::installerPage->SetInstllInfoText(text);
        SendRenderRequest();
    }

    void SendInstallProgress(double percent)
    {
        app::ui::installerPage->SetProgressBar(percent);
        SendRenderRequest();
    }

    void SendInstallFinished()
    {
        app::ui::SceneJump(app::ui::Scene::Main);
    }

    void ShowInstaller()
    {
        app::ui::SceneJump(app::ui::Scene::Installer);
    }

    s32 CreateDialogSimple(const std::string &title, const std::string &content, const std::vector<std::string> &opts, const bool last_opt_is_cancel)
    {
        auto dialog = pu::ui::Dialog::New(title, content);
        dialog->SetSpaceBetweenOptions(35);
        dialog->SetOptionHorizontalMargin(40);

        for (u32 i = 0; i < opts.size(); i++)
        {
            const auto &opt = opts.at(i);
            if (last_opt_is_cancel && (i == (opts.size() - 1)))
            {
                dialog->SetCancelOption(opt);
            }
            else
            {
                dialog->AddOption(opt);
            }
        }

        const auto opt = app::ui::mainApp->ShowDialog(dialog);
        if (dialog->UserCancelled())
        {
            return -1;
        }
        else if (!dialog->IsOk())
        {
            return -2;
        }
        else
        {
            return opt;
        }
    }
}
