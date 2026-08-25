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
    std::string app_path;

    void initApp(const char* argv0)
    {
        if (!nx::fs::Exists("sdmc:/config"))
        {
            nx::fs::MakeDir("sdmc:/config");
        }
        if (!nx::fs::Exists(app::config::storagePath))
        {
            nx::fs::MakeDir(app::config::storagePath);
        }
        const auto is_applet = appletGetAppletType() == AppletType_LibraryApplet;
        if (is_applet)
        {
            app::config::SetScreenScaleFactor(1.5f);
        }
        app::config::ParseSettings();
        app::config::ParseThemeColor();
        nx::udisk::Init();

#ifdef ENABLE_NET
        // https://github.com/mtheall/ftpd/blob/e27898f0c3101522311f330e82a324861e0e3f7e/source/switch/init.c#L31
        const SocketInitConfig socket_config_application = {
            .tcp_tx_buf_size = 1024 * 64,
            .tcp_rx_buf_size = 1024 * 64,
            .tcp_tx_buf_max_size = 1024 * 1024 * 4,
            .tcp_rx_buf_max_size = 1024 * 1024 * 4,
            .udp_tx_buf_size = 0x2400, // same as default
            .udp_rx_buf_size = 0xA500, // same as default
            .sb_efficiency = 8,
            .num_bsd_sessions = 3,
            .bsd_service_type = BsdServiceType_Auto,
        };

        const SocketInitConfig socket_config_applet = {
            .tcp_tx_buf_size = 1024 * 32,
            .tcp_rx_buf_size = 1024 * 64,
            .tcp_tx_buf_max_size = 1024 * 256,
            .tcp_rx_buf_max_size = 1024 * 256,
            .udp_tx_buf_size = 0x2400, // same as default
            .udp_rx_buf_size = 0xA500, // same as default
            .sb_efficiency = 4,
            .num_bsd_sessions = 3,
            .bsd_service_type = BsdServiceType_Auto,
        };

        const auto socket_config = is_applet ? socket_config_applet : socket_config_application;
        socketInitialize(&socket_config);
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

        if (!std::strncmp(argv0, "sdmc:/", 6))
        {
            app_path = argv0 + 5;
        }
        else
        {
            app_path = argv0;
        }
    }

    void deinitApp()
    {
        ncmExit();
        accountExit();
        romfsExit();
        nx::udisk::Exit();

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

    const char* getAppPath()
    {
        return app_path.c_str();
    }
}
