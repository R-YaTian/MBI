#pragma once

#include <string>
#include <vector>
#include <switch/types.h>

namespace app::facade
{
    s32 ShowDialog(const std::string &title, const std::string &content, const std::vector<std::string> &opts, const bool use_last_opt_as_cancel);
    s32 CreateDialogSimple(const std::string &title, const std::string &content, const std::vector<std::string> &opts, const bool last_opt_is_cancel);
    void SendBottomText(std::string text);
    void SendPageInfoText(std::string text);
    void SendPageInfoTextAndRender(std::string text);
    bool SendRenderRequest();
    void SendInstallInfoText(std::string text);
    void SendInstallProgress(double percent);
    void SendInstallFinished();
    void ShowInstaller();
}
