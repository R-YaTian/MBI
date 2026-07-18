#include "nx/content.hpp"
#include <algorithm>

namespace nx
{
    const void* Content::GetFileEntryByName(std::string name)
    {
        // returns only the .nca and .cnmt.nca filenames
        for (u32 i = 0; i < this->GetBaseHeader()->numFiles; i++)
        {
            const void* fileEntry = this->GetFileEntry(i);
            std::string foundName(this->GetFileEntryName(fileEntry));

            if (foundName == name)
            {
                return fileEntry;
            }
        }

        return nullptr;
    }

    const void* Content::GetFileEntryByNcaId(const NcmContentId& ncaId)
    {
        const void* fileEntry = nullptr;
        std::string ncaIdStr = nx::ncm::GetContentIdString(ncaId);

        if ((fileEntry = this->GetFileEntryByName(ncaIdStr + ".nca")) == nullptr)
        {
            if ((fileEntry = this->GetFileEntryByName(ncaIdStr + ".cnmt.nca")) == nullptr)
            {
                if ((fileEntry = this->GetFileEntryByName(ncaIdStr + ".ncz")) == nullptr)
                {
                    if ((fileEntry = this->GetFileEntryByName(ncaIdStr + ".cnmt.ncz")) == nullptr)
                    {
                        return nullptr;
                    }
                }
            }
        }

        return fileEntry;
    }

    std::vector<const void*> Content::GetFileEntriesByExtension(std::string extension)
    {
        std::vector<const void*> entryList;

        for (u32 i = 0; i < this->GetBaseHeader()->numFiles; i++)
        {
            const void* fileEntry = this->GetFileEntry(i);
            std::string name(this->GetFileEntryName(fileEntry));
            auto foundExtension = name.substr(name.find(".") + 1);

            if (foundExtension == extension)
            {
                entryList.push_back(fileEntry);
            }
        }

        return entryList;
    }

    void Content::GenerateCollections()
    {
        for (u32 i = 0; i < this->GetBaseHeader()->numFiles; i++)
        {
            const void* fileEntry = this->GetFileEntry(i);
            ContentCollectionEntry entry;
            entry.name = this->GetFileEntryName(fileEntry);
            entry.offset = this->GetFileEntryOffset(fileEntry);
            entry.size = this->GetFileEntrySize(fileEntry);
            entry.content_id = nx::ncm::GetContentIdFromString(entry.name.substr(0, entry.name.find(".")));
            if (entry.name.ends_with(".cnmt.nca") || entry.name.ends_with(".cnmt.ncz"))
            {
                entry.type = ContentCollectionType::META;
            }
            else if (entry.name.ends_with(".tik"))
            {
                entry.type = ContentCollectionType::TIK;
            }
            else if (entry.name.ends_with(".cert"))
            {
                entry.type = ContentCollectionType::CERT;
            }
            m_collections.emplace_back(entry);
        }

        const auto sorter = [](const ContentCollectionEntry& lhs, const ContentCollectionEntry& rhs) -> bool {
            return lhs.offset < rhs.offset;
        };

        std::sort(m_collections.begin(), m_collections.end(), sorter);
    }
}
