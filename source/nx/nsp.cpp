/*
Copyright (c) 2017-2018 Adubbz

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "nx/nsp.hpp"
#include "nx/error.hpp"

namespace nx
{
    NSP::NSP() {}

    void NSP::CommitHeader(std::vector<u8>&& headerBytes)
    {
        m_headerBytes = std::move(headerBytes);
    }

    const XFS0BaseHeader* NSP::GetBaseHeader()
    {
        if (m_headerBytes.empty())
        {
            THROW_FORMAT("Cannot retrieve header as header bytes are empty. Have you commited it yet?\n");
        }

        return reinterpret_cast<XFS0BaseHeader*>(m_headerBytes.data());
    }

    const u64 NSP::GetDataOffset()
    {
        if (m_headerBytes.empty())
        {
            THROW_FORMAT("Cannot get data offset as header is empty. Have you commited it yet?\n");
        }

        return m_headerBytes.size();
    }

    const u64 NSP::GetFileEntrySize(const void *fileEntry)
    {
        return ((PFS0FileEntry *)fileEntry)->fileSize;
    }

    const u64 NSP::GetFileEntryOffset(const void *fileEntry)
    {
        return GetDataOffset() + ((PFS0FileEntry *)fileEntry)->dataOffset;
    }

    const void* NSP::GetFileEntry(unsigned int index)
    {
        if (index >= this->GetBaseHeader()->numFiles)
        {
            THROW_FORMAT("File entry index is out of bounds\n");
        }

        size_t fileEntryOffset = sizeof(XFS0BaseHeader) + index * sizeof(PFS0FileEntry);

        if (m_headerBytes.size() < fileEntryOffset + sizeof(PFS0FileEntry))
        {
            THROW_FORMAT("Header bytes is too small to get file entry!");
        }

        return reinterpret_cast<PFS0FileEntry*>(m_headerBytes.data() + fileEntryOffset);
    }

    const char* NSP::GetFileEntryName(const void* fileEntry)
    {
        u64 stringTableStart = sizeof(XFS0BaseHeader) + this->GetBaseHeader()->numFiles * sizeof(PFS0FileEntry);
        return reinterpret_cast<const char*>(m_headerBytes.data() + stringTableStart + ((PFS0FileEntry *)fileEntry)->stringTableOffset);
    }
}
