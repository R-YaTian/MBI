#pragma once

#include <filesystem>
#include <vector>
#include <string>
#include "nx/ncm.hpp"

namespace app::installer
{
    namespace Local
    {
        enum class StorageSource : u8
        {
            SD,
            UDISK
        };

        void InstallFromFile(std::vector<std::filesystem::path> ourTitleList, NcmStorageId destStorageId, StorageSource storageSrc);
    }

    namespace Usb
    {
        std::vector<std::string> WaitingForFileList();
        void InstallTitles(std::vector<std::string> ourTitleList, NcmStorageId destStorageId);
    }

    namespace Network
    {
        std::vector<std::string> WaitingForNetworkData();
        void PushExitCommand(std::string url);
        void Cleanup();
        void InstallFromUrl(std::vector<std::string> ourUrlList, NcmStorageId destStorageId, std::string ourSource);
    }
}
