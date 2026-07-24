#pragma once

#include <string>

namespace app::manager
{
    void initApp(const char* argv0 = nullptr);
    void deinitApp();
    void initInstallServices();
    void deinitInstallServices();
    const char* getAppPath();
}
