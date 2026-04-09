#include <SDL2/SDL_mixer.h>
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

#ifdef ENABLE_NET
        socketInitializeDefault();
        // Initialize libcurl globally on main thread before any threads use it
        curl_global_init(CURL_GLOBAL_ALL);

#ifdef __DEBUG__
        nxlinkStdio();
#endif
#endif

        Mix_Init(MIX_INIT_FLAC | MIX_INIT_MOD | MIX_INIT_MP3 | MIX_INIT_OGG);
        if (R_FAILED(ncmInitialize()))
            LOG_DEBUG("Failed to initialize ncm\n");
    }

    void deinitApp()
    {
        ncmExit();
        Mix_Quit();

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

    void playAudio(std::string audioPath)
    {
        std::string finalPath = "romfs:/audio"  + audioPath;
        if (nx::fs::Exists(app::config::storagePath + audioPath))
        {
            finalPath = app::config::storagePath + audioPath;
        }

        int audio_rate = 44100;
        Uint16 audio_format = AUDIO_S16SYS;
        int audio_channels = 2;
        int audio_buffers = 4096;

        if (Mix_OpenAudio(audio_rate, audio_format, audio_channels, audio_buffers) != 0)
            return;

        Mix_Chunk *sound = NULL;
        sound = Mix_LoadWAV(finalPath.c_str());
        if (sound == NULL || !app::config::enableSound)
        {
            Mix_FreeChunk(sound);
            Mix_CloseAudio();
            return;
        }

        int channel = Mix_PlayChannel(-1, sound, 0);
        if (channel == -1)
        {
            Mix_FreeChunk(sound);
            Mix_CloseAudio();
            return;
        }

        while (Mix_Playing(channel) != 0);

        Mix_FreeChunk(sound);
        Mix_CloseAudio();

        return;
    }
}
