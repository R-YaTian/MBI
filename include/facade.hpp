#pragma once

#include <string>
#include <vector>
#include <switch/types.h>

namespace app::facade
{
    void SendBottomText(std::string text);
    void SendPageInfoText(std::string text);
    void SendPageInfoTextAndRender(std::string text);
    bool SendRenderRequest();
    void SendInstallInfoText(std::string text);
    void SendInstallBarText(std::string text);
    void SendInstallProgress(double percent);
    void SendInstallFinished();
    void ShowInstaller(std::string sourceString);
    void ShowFullTouchButtonArea();
    s32 ShowDialog(const std::string &title, const std::string &content, const std::vector<std::string> &opts, const bool last_opt_is_cancel, std::string icon_name = "");
    void PlayAudio(const std::string& filename);
    void NotifyInstallSuccess(const size_t count, const std::string& msg);
    void NotifyInstallFailed(const std::exception& e, const std::string& msg);
}
