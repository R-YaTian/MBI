#include <switch-ipcext.h>
#include "manager.hpp"
#include "nx/fs.hpp"
#include "nx/udisk.hpp"
#include "nx/error.hpp"
#include "util/config.hpp"

#ifdef ENABLE_NET
#include <curl/curl.h>
#endif

namespace app::manager
{
    void initApp()
    {
        if (!nx::fs::Exists("sdmc:/config"))
            nx::fs::MakeDir("sdmc:/config");
        if (!nx::fs::Exists(app::config::storagePath))
            nx::fs::MakeDir(app::config::storagePath);
        app::config::ParseSettings();
        app::config::ParseThemeColor();
        nx::udisk::init();

#ifdef ENABLE_NET
        socketInitializeDefault();
        // Initialize libcurl globally on main thread before any threads use it
        curl_global_init(CURL_GLOBAL_ALL);

#ifdef __DEBUG__
        nxlinkStdio();
#endif
#endif

        accountInitialize(AccountServiceType_Administrator);
        if (R_FAILED(ncmInitialize()))
            LOG_DEBUG("Failed to initialize ncm\n");
        if (R_FAILED(romfsInit()))
            LOG_DEBUG("Failed to mount romfs\n");
    }

    void deinitApp()
    {
        ncmExit();
        accountExit();
        romfsExit();
        nx::udisk::exit();

#ifdef ENABLE_NET
        // Clean up libcurl globally when exit
        curl_global_cleanup();
        socketExit();
#endif
    }

    void initInstallServices()
    {
        nsInitialize();
        esInitialize();
        splCryptoInitialize();
        splInitialize();
    }

    void deinitInstallServices()
    {
        nsExit();
        esExit();
        splCryptoExit();
        splExit();
    }
}
