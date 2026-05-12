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
        app::ui::installerPage->AppendInstallInfoText(text);
        SendRenderRequest();
    }

    void SendInstallBarText(std::string text)
    {
        app::ui::installerPage->SetInstallBarText(text);
        SendRenderRequest();
    }

    void SendInstallProgress(double percent)
    {
        app::ui::installerPage->SetProgressBar(percent);
        SendRenderRequest();
    }

    void SendInstallFinished()
    {
        app::ui::installerPage->SetFinished();
        app::ui::mainApp->SetTouchButtonAreaType(app::ui::TouchButtonAreaType::Base);
    }

    void ShowInstaller()
    {
        app::ui::SceneJump(app::ui::Scene::Installer);
    }

    s32 ShowDialog(const std::string &title, const std::string &content, const std::vector<std::string> &opts, const bool last_opt_is_cancel, std::string icon_name)
    {
        auto dialog = pu::ui::Dialog::New(title, content);
        dialog->SetSpaceBetweenOptions(35);
        dialog->SetOptionHorizontalMargin(40);
        if (icon_name != "")
        {
            pu::sdl2::TextureHandle::Ref icon = ui::LoadTexture("romfs:/images/icons/" + icon_name + ".webp");
            if (icon->Get() != nullptr)
            {
                dialog->SetIcon(icon);
            }
        }

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

    void PlayAudio(const std::string& filename)
    {
        std::string finalPath = app::ui::mainApp->GetFinalResourcePath("audio", filename);
        if (finalPath == "")
        {
            return;
        }

        pu::audio::Initialize(pu::audio::INIT_ALL);
        pu::audio::Music mus = pu::audio::OpenMusic(finalPath);
        if (mus != nullptr)
        {
            pu::audio::PlayMusic(mus, 0);
            while (pu::audio::IsPlayingMusic());
            pu::audio::DestroyMusic(mus);
        }
        pu::audio::Finalize();
    }
}
