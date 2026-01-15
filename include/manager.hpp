#pragma once

#include <string>

namespace app::manager
{
    void initApp();
    void deinitApp();
    void initInstallServices();
    void deinitInstallServices();
    void playAudio(std::string audioPath);
    void lightningStart();
    void lightningStop();
}
