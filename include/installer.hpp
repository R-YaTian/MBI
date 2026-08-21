#pragma once

#include <vector>
#include <string>
#include "nx/fs.hpp"
#include "nx/ncm.hpp"

namespace app::installer
{
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
