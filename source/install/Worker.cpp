#include "install/Worker.hpp"
#include "nx/error.hpp"
#include "nx/xfs0.hpp"
#include "nx/nnsp.hpp"
#include "nx/nxci.hpp"

namespace app::install
{
    void RetrieveNSPHeader(Worker& worker, nx::NSP& nsp)
    {
        LOG_DEBUG("Retrieving remote NSP header...\n");

        // Retrieve the base header
        std::vector<u8> headerBytes(sizeof(nx::XFS0BaseHeader), 0);
        worker.BufferData(headerBytes.data(), 0x0, sizeof(nx::XFS0BaseHeader));

        // Commit it, so we can read back
        nsp.CommitHeader(std::vector<u8>(headerBytes));

        // Retrieve the full header
        size_t remainingHeaderSize = nsp.GetBaseHeader()->numFiles * sizeof(nx::PFS0FileEntry) + nsp.GetBaseHeader()->stringTableSize;
        headerBytes.resize(sizeof(nx::XFS0BaseHeader) + remainingHeaderSize, 0);
        worker.BufferData(headerBytes.data() + sizeof(nx::XFS0BaseHeader), sizeof(nx::XFS0BaseHeader), remainingHeaderSize);

        nsp.CommitHeader(std::move(headerBytes));
    }

    void RetrieveXCIHeader(Worker& worker, nx::XCI& xci)
    {
        LOG_DEBUG("Retrieving HFS0 header...\n");

        // Retrieve hfs0 offset
        u64 hfs0Offset = 0xf000;

        // Retrieve main hfs0 header
        std::vector<u8> m_headerBytes;
        m_headerBytes.resize(sizeof(nx::XFS0BaseHeader), 0);
        worker.BufferData(m_headerBytes.data(), hfs0Offset, sizeof(nx::XFS0BaseHeader));

        // Retrieve full header
        nx::XFS0BaseHeader* header = reinterpret_cast<nx::XFS0BaseHeader*>(m_headerBytes.data());
        if (header->magic != MAGIC_HFS0)
        {
            THROW_FORMAT("hfs0 magic doesn't match at 0x%lx\n", hfs0Offset);
        }
        size_t remainingHeaderSize = header->numFiles * sizeof(nx::HFS0FileEntry) + header->stringTableSize;
        m_headerBytes.resize(sizeof(nx::XFS0BaseHeader) + remainingHeaderSize, 0);
        worker.BufferData(m_headerBytes.data() + sizeof(nx::XFS0BaseHeader), hfs0Offset + sizeof(nx::XFS0BaseHeader), remainingHeaderSize);

        // Find Secure partition
        header = reinterpret_cast<nx::XFS0BaseHeader*>(m_headerBytes.data());
        for (unsigned int i = 0; i < header->numFiles; i++)
        {
            const nx::HFS0FileEntry* entry = hfs0GetFileEntry(header, i);
            std::string entryName(hfs0GetFileName(header, entry));

            if (entryName != "secure")
            {
                continue;
            }

            std::vector<u8> secureHeaderBytes;
            u64 secureHeaderOffset = hfs0Offset + remainingHeaderSize + 0x10 + entry->dataOffset;
            secureHeaderBytes.resize(sizeof(nx::XFS0BaseHeader), 0);
            worker.BufferData(secureHeaderBytes.data(), secureHeaderOffset, sizeof(nx::XFS0BaseHeader));

            // Commit it, so we can read back
            xci.CommitHeader(std::vector<u8>(secureHeaderBytes), secureHeaderOffset);

            if (xci.GetBaseHeader()->magic != MAGIC_HFS0)
            {
                THROW_FORMAT("hfs0 magic doesn't match at 0x%lx\n", secureHeaderOffset);
            }

            // Retrieve full header
            remainingHeaderSize = xci.GetBaseHeader()->numFiles * sizeof(nx::HFS0FileEntry) + xci.GetBaseHeader()->stringTableSize;
            secureHeaderBytes.resize(sizeof(nx::XFS0BaseHeader) + remainingHeaderSize, 0);
            worker.BufferData(secureHeaderBytes.data() + sizeof(nx::XFS0BaseHeader), secureHeaderOffset + sizeof(nx::XFS0BaseHeader), remainingHeaderSize);

            xci.CommitHeader(std::move(secureHeaderBytes), secureHeaderOffset);
            return;
        }
        THROW_FORMAT("couldn't optain secure hfs0 header\n");
    }

    void Worker::RetrieveHeader()
    {
        switch (m_content->GetType())
        {
            case nx::Content::Type::NSP:
                RetrieveNSPHeader(*this, static_cast<nx::NSP&>(*m_content));
                break;

            case nx::Content::Type::XCI:
                RetrieveXCIHeader(*this, static_cast<nx::XCI&>(*m_content));
                break;

            default:
                THROW_FORMAT("Logic error: invalid content type");
        }
    }
}
