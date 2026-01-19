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
}
