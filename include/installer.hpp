#pragma once

#include <vector>
#include <string>
#include "nx/ncm.hpp"

namespace app::installer
{
    namespace Network
    {
        std::vector<std::string> WaitingForNetworkData();
        void InstallFromUrl(std::vector<std::string> ourUrlList, NcmStorageId destStorageId, std::string ourSource);
    }
}
