#pragma once

#include <filesystem>
#include <vector>
#include <string>

namespace app::installer
{
    namespace Local
    {
        enum class StorageSource : u8
        {
            SD,
            UDISK
        };

        void InstallFromFile(std::vector<std::filesystem::path> ourTitleList, int whereToInstall, StorageSource storageSrc);
    }

    namespace Usb
    {
        std::vector<std::string> WaitingForFileList();
        void InstallTitles(std::vector<std::string> ourTitleList, int ourStorage);
    }

    namespace Network
    {
        std::vector<std::string> WaitingForNetworkData();
        void PushExitCommand(std::string url);
        void Cleanup();
        void InstallFromUrl(std::vector<std::string> ourUrlList, int ourStorage, std::string ourSource);
    }
}
