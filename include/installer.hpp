#pragma once

#include <filesystem>
#include <vector>

namespace app::installer
{
    namespace Local
    {
        enum class StorageSource : u8
        {
            SD,
            UDISK
        };

        void installFromFile(std::vector<std::filesystem::path> ourTitleList, int whereToInstall, StorageSource storageSrc);
    }
}
