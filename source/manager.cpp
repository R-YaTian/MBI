#include <filesystem>
#include <switch-ipcext.h>
#include "manager.hpp"
#include "nx/usb.hpp"
#include "nx/udisk.hpp"
#include "nx/error.hpp"
#include "util/config.hpp"

namespace app::manager
{
    void initApp()
    {
        nx::usb::usbDeviceInitialize();

        if (!std::filesystem::exists("sdmc:/config"))
            std::filesystem::create_directory("sdmc:/config");
        if (!std::filesystem::exists(app::config::storagePath))
            std::filesystem::create_directory(app::config::storagePath);
        app::config::ParseSettings();
        app::config::ParseThemeColor();

        socketInitializeDefault();

#ifdef __DEBUG__
        nxlinkStdio();
#endif

        if (nx::usb::usbDeviceIsInitialized())
            nx::udisk::init();

        Mix_Init(MIX_INIT_FLAC | MIX_INIT_MOD | MIX_INIT_MP3 | MIX_INIT_OGG);
        if (R_FAILED(ncmInitialize()))
            LOG_DEBUG("Failed to initialize ncm\n");
    }

    void deinitApp()
    {
        ncmExit();
        Mix_Quit();

        nx::udisk::exit();

        socketExit();

        nx::usb::usbDeviceExit();
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
        int audio_rate = 44100;
        Uint16 audio_format = AUDIO_S16SYS;
        int audio_channels = 2;
        int audio_buffers = 4096;

        if (Mix_OpenAudio(audio_rate, audio_format, audio_channels, audio_buffers) != 0)
            return;

        Mix_Chunk *sound = NULL;
        sound = Mix_LoadWAV(audioPath.c_str());
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

    void lightningStart()
    {
        padConfigureInput(8, HidNpadStyleSet_NpadStandard);
        PadState pad;
        padInitializeDefault(&pad);
        padUpdate(&pad);

        Result rc = 0;
        HidsysUniquePadId unique_pad_ids[2] = {0};
        HidsysNotificationLedPattern pattern;

        rc = hidsysInitialize();
        if (R_SUCCEEDED(rc))
        {
            memset(&pattern, 0, sizeof(pattern));

            pattern.baseMiniCycleDuration = 0x1;
            pattern.totalMiniCycles = 0xF;
            pattern.totalFullCycles = 0x0;
            pattern.startIntensity = 0x0;

            pattern.miniCycles[0].ledIntensity = 0xF;
            pattern.miniCycles[0].transitionSteps = 0xF;
            pattern.miniCycles[0].finalStepDuration = 0x0;
            pattern.miniCycles[1].ledIntensity = 0x0;
            pattern.miniCycles[1].transitionSteps = 0xF;
            pattern.miniCycles[1].finalStepDuration = 0x0;
            pattern.miniCycles[2].ledIntensity = 0xF;
            pattern.miniCycles[2].transitionSteps = 0xF;
            pattern.miniCycles[2].finalStepDuration = 0x0;
            pattern.miniCycles[3].ledIntensity = 0x0;
            pattern.miniCycles[3].transitionSteps = 0xF;
            pattern.miniCycles[3].finalStepDuration = 0x0;

            s32 total_entries = 0;
            memset(unique_pad_ids, 0, sizeof(unique_pad_ids));
            rc = hidsysGetUniquePadsFromNpad(padIsHandheld(&pad) ? HidNpadIdType_Handheld : HidNpadIdType_No1, unique_pad_ids, 2, &total_entries);

            if (R_SUCCEEDED(rc))
            {
                for (s32 i = 0; i < total_entries; i++)
                {
                    rc = hidsysSetNotificationLedPattern(&pattern, unique_pad_ids[i]);
                }
            }

            hidsysExit();
        }
    }

    void lightningStop()
    {
        padConfigureInput(8, HidNpadStyleSet_NpadStandard);
        PadState pad;
        padInitializeDefault(&pad);
        padUpdate(&pad);

        Result rc = 0;
        HidsysUniquePadId unique_pad_ids[2] = {0};
        HidsysNotificationLedPattern pattern;

        rc = hidsysInitialize();
        if (R_SUCCEEDED(rc))
        {
            memset(&pattern, 0, sizeof(pattern));

            s32 total_entries = 0;
            memset(unique_pad_ids, 0, sizeof(unique_pad_ids));
            rc = hidsysGetUniquePadsFromNpad(padIsHandheld(&pad) ? HidNpadIdType_Handheld : HidNpadIdType_No1, unique_pad_ids, 2, &total_entries);

            if (R_SUCCEEDED(rc))
            {
                for (s32 i = 0; i < total_entries; i++)
                {
                    rc = hidsysSetNotificationLedPattern(&pattern, unique_pad_ids[i]);
                }
            }

            hidsysExit();
        }
    }
}
