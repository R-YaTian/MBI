#include "nx/content.hpp"
#include "nx/nca.hpp"

namespace nx
{
    const void* Content::GetFileEntryByName(std::string name)
    {
        // returns only the .nca and .cnmt.nca filenames
        for (unsigned int i = 0; i < this->GetBaseHeader()->numFiles; i++)
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
        std::string ncaIdStr = nx::nca::GetNcaIdString(ncaId);

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

        for (unsigned int i = 0; i < this->GetBaseHeader()->numFiles; i++)
        {
            const void* fileEntry = this->GetFileEntry(i);
            std::string name(this->GetFileEntryName(fileEntry));
            auto foundExtension = name.substr(name.find(".") + 1);

            // fix cert filename extension becoming corrupted when xcz/nsz is installing certs
            std::string cert("cert");
            std::size_t found = name.find(cert);
            if (found != std::string::npos)
            {
                int pos = 0;
                std::string mystr = name;
                pos = mystr.find_last_of('.');
                mystr = mystr.substr(5, pos);
                foundExtension = mystr.substr(mystr.find(".") + 1);
            }

            if (foundExtension == extension)
            {
                entryList.push_back(fileEntry);
            }
        }

        return entryList;
    }
}
