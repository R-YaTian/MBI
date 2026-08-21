#include "facade.hpp"
#include "util/i18n.hpp"
#include "util/config.hpp"
#include "ui/InstallerPage.hpp"
#include "ui/MainApplication.hpp"

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
        auto lyt = app::ui::mainApp->GetLayout<app::ui::InstallerPage>();
        if (lyt)
        {
            lyt->AppendInstallInfoText(text);
        }
        SendRenderRequest();
    }

    void SendInstallBarText(std::string text)
    {
        auto lyt = app::ui::mainApp->GetLayout<app::ui::InstallerPage>();
        if (lyt)
        {
            lyt->SetInstallBarText(text);
        }
        SendRenderRequest();
    }

    void SendInstallProgress(double percent)
    {
        auto lyt = app::ui::mainApp->GetLayout<app::ui::InstallerPage>();
        if (lyt)
        {
            lyt->SetProgressBar(percent);
        }
        SendRenderRequest();
    }

    void SendInstallFinished()
    {
        app::ui::installerPage->SetFinished();
        app::ui::mainApp->SetTouchButtonAreaType(app::ui::TouchButtonAreaType::Base);
    }

    void ShowInstaller(std::string sourceString)
    {
        app::ui::SceneJump(app::ui::Scene::Installer);
        app::facade::SendBottomText(sourceString);
    }

    void ShowFullTouchButtonArea()
    {
        app::ui::mainApp->SetTouchButtonAreaType(app::ui::TouchButtonAreaType::Full);
    }

    s32 ShowDialog(const std::string &title, const std::string &content, const std::vector<std::string> &opts, const bool last_opt_is_cancel, std::string icon_name)
    {
        auto dialog = pu::ui::Dialog::New(title, content);
        dialog->SetSpaceBetweenOptions(35_dp);
        dialog->SetOptionHorizontalMargin(40_dp);

        dialog->SetDialogBorderRadius(52_dp);
        dialog->SetSpaceBetweenOptionRows(15_dp);
        dialog->SetTitleExtraWidth(135_dp);
        dialog->SetContentExtraWidth(135_dp);
        dialog->SetSpaceBetweenContentAndOptions(210_dp);
        dialog->SetTitleTopMargin(30_dp);
        dialog->SetTitleX(67_dp);
        dialog->SetTitleY(82_dp);
        dialog->SetContentX(67_dp);
        dialog->SetContentY(210_dp);
        dialog->SetOptionsBaseHorizontalMargin(67_dp);
        dialog->SetOptionHeight(90_dp);
        dialog->SetOptionBorderRadius(30_dp);
        dialog->SetOptionBottomMargin(37_dp);
        dialog->SetIconMargin(45_dp);

        if (icon_name != "")
        {
            pu::sdl2::TextureHandle::Ref icon = ui::LoadTexture("romfs:/images/icons/" + icon_name + ".webp");
            if (icon->Get() != nullptr)
            {
                dialog->SetIcon(icon);
                dialog->SetIconWidth(dialog->GetIconWidth() / app::config::GetScreenScaleFactor());
                dialog->SetIconHeight(dialog->GetIconHeight() / app::config::GetScreenScaleFactor());
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

    void NotifyInstallSuccess(const size_t count, const std::string& msg)
    {
        SendInstallBarText("inst.info_page.complete"_lang);
        SendInstallInfoText(count > 1 ?
                            std::to_string(count) + "inst.info_page.desc0"_lang :
                            msg + "inst.info_page.desc1"_lang);
        SendInstallProgress(100);
        SendInstallInfoText(i18n::GetRandomMsg());
        if (config::enableSound && appletGetAppletType() != AppletType_LibraryApplet)
        {
            std::thread audioThread(PlayAudio, "success.mp3");
            audioThread.join();
        }
    }

    void NotifyInstallFailed(const std::exception& e, const std::string& msg)
    {
        SendInstallBarText("inst.info_page.failed"_lang);
        SendInstallInfoText("inst.info_page.failed"_lang + msg + "!\n" + "inst.info_page.failed_desc"_lang);
        SendInstallProgress(0);
        SendInstallInfoText((std::string)e.what());
        if (config::enableSound && appletGetAppletType() != AppletType_LibraryApplet)
        {
            std::thread audioThread(PlayAudio, "fail.mp3");
            audioThread.join();
        }
    }
}
