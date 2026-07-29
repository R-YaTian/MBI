#pragma once

#include <vector>
#include <string>
#include "nx/fs.hpp"
#include "nx/ncm.hpp"

namespace app::installer
{
    void OnSuccess(const size_t count, const std::string& msg);
    void OnFailed(const std::string& msg, const std::exception& e);

    namespace Local
    {
        enum class StorageSource : u8
        {
            SD,
            UDISK
        };

        void InstallFromFile(std::vector<nx::fs::Path> ourTitleList, NcmStorageId destStorageId, StorageSource storageSrc);
    }

    namespace Usb
    {
        std::vector<std::string> WaitingForFileList();
        void InstallTitles(std::vector<std::string> ourTitleList, NcmStorageId destStorageId);
    }

    namespace Network
    {
        std::vector<std::string> WaitingForNetworkData();
        void InstallFromUrl(std::vector<std::string> ourUrlList, NcmStorageId destStorageId, std::string ourSource);
    }
}
